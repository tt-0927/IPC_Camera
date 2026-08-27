/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : RecordFrameStream.cpp
 * @Description  : 录像帧客户端实现，负责与设备建立TCP连接，接收录像帧数据（视频/音频）
 */

#include "RecordFrameStream.h"

#include <cstring>
#include <utility>
#include <vector>

#include "NetSdkLog.h"
#include "PlatformCompat.h"

namespace tvsdk {
namespace {

/**
 * @brief 精确接收指定字节数的数据
 * @param fd socket文件描述符
 * @param data 接收数据缓冲区
 * @param size 需要接收的字节数
 * @return true表示成功接收指定字节数，false表示接收失败或连接断开
 */
bool recv_exact(socket_fd_t fd, char* data, size_t size) {
    size_t received = 0;
    while (received < size) {
        ssize_t n = recv(fd, data + received, static_cast<int>(size - received), 0);
        if (n <= 0) {
            return false;
        }
        received += static_cast<size_t>(n);
    }
    return true;
}

/**
 * @brief 将大端序16位整数转换为主机字节序
 * @param value 大端序16位整数
 * @return 主机字节序16位整数
 */
uint16_t from_be16(uint16_t value) {
    return ntohs(value);
}

/**
 * @brief 将大端序32位整数转换为主机字节序
 * @param value 大端序32位整数
 * @return 主机字节序32位整数
 */
uint32_t from_be32(uint32_t value) {
    return ntohl(value);
}

} // namespace

/**
 * @brief 构造函数
 */
CRecordFrameStream::CRecordFrameStream() = default;

/**
 * @brief 析构函数
 * @details 自动调用stop()停止接收并释放资源
 */
CRecordFrameStream::~CRecordFrameStream() { stop(); }

/**
 * @brief 连接设备录像帧端口并启动接收
 * @details 创建TCP socket，设置超时时间，连接设备，开启TCP_NODELAY，
 *          启动接收线程循环接收录像帧数据
 * @param host 设备IP地址
 * @param port 设备录像帧端口号
 * @param stream_id 流ID，标识当前播放会话
 * @param callback 帧数据回调函数，用于接收设备发送的录像帧数据
 * @return true表示成功，false表示失败
 */
bool CRecordFrameStream::start(const std::string& host,
                              int port,
                              const std::string& stream_id,
                              RecordFrameCallback callback) {
    if (m_bRunning) {
        NETSDK_LOG_MESSAGE_WARN("RecordFrameStream: already running");
        return false;
    }
    if (host.empty() || port <= 0 || port > 65535 || stream_id.empty()) {
        return false;
    }

    m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nSocket == INVALID_SOCKET_FD) {
        NETSDK_LOG_MESSAGE_ERROR("RecordFrameStream: socket failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        return false;
    }

    timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(m_nSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    setsockopt(m_nSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        NETSDK_LOG_MESSAGE_ERROR("RecordFrameStream: invalid host %s", host.c_str());
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    if (connect(m_nSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("RecordFrameStream: connect %s:%d failed, errno=%d", host.c_str(), port, NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    int tcp_no_delay = 1;
    if (setsockopt(m_nSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&tcp_no_delay), sizeof(tcp_no_delay)) != 0) {
        NETSDK_LOG_MESSAGE_WARN("RecordFrameStream: set TCP_NODELAY failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
    }

    m_strStreamId = stream_id;
    m_fnCallback = std::move(callback);
    m_bRunning = true;
    m_stReceiveThread = std::thread(&CRecordFrameStream::recv_loop, this);

    NETSDK_LOG_MESSAGE_INFO("RecordFrameStream: connected to %s:%d, stream_id=%s", host.c_str(), port, stream_id.c_str());
    return true;
}

/**
 * @brief 停止并关闭连接
 * @details 设置停止标志，关闭socket，等待接收线程结束
 */
void CRecordFrameStream::stop() {
    std::lock_guard<std::mutex> lock(m_stStopMutex);
    if (!m_bRunning && m_nSocket == INVALID_SOCKET_FD) {
        return;
    }

    m_bRunning = false;
    if (m_nSocket != INVALID_SOCKET_FD) {
        shutdown(m_nSocket, SHUT_RDWR);
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
    }
    if (m_stReceiveThread.joinable()) {
        m_stReceiveThread.join();
    }
    NETSDK_LOG_MESSAGE_INFO("RecordFrameStream: stopped, stream_id=%s", m_strStreamId.c_str());
}

/**
 * @brief 接收数据循环
 * @details 持续从socket读取录像帧，解析RTP-like帧头（版本号、负载类型、序号、时间戳、数据长度等），
 *          根据负载类型判断媒体类型（视频/音频/结束帧），组装帧信息后通过回调函数转发给上层，
 *          遇到流结束标志或接收失败时退出循环
 */
void CRecordFrameStream::recv_loop() {
    while (m_bRunning) {
        NET_RecordFrameRtpHeader_S header{};
        if (!recv_exact(m_nSocket, reinterpret_cast<char*>(&header), sizeof(header))) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("RecordFrameStream: recv header failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            break;
        }

        if (header.byVersion != 2) {
            NETSDK_LOG_MESSAGE_WARN("RecordFrameStream: invalid rtp-like version %u", static_cast<unsigned>(header.byVersion));
            break;
        }

        const uint32_t payload_len = from_be32(header.uPayloadLen);
        if (payload_len > NET_RECORD_FRAME_MAX_PAYLOAD_SIZE) {
            NETSDK_LOG_MESSAGE_WARN("RecordFrameStream: invalid payload len %u", payload_len);
            break;
        }

        std::vector<char> payload(payload_len);
        if (payload_len > 0 && !recv_exact(m_nSocket, payload.data(), payload_len)) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("RecordFrameStream: recv payload failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            break;
        }

        NET_RecordFrameInfo_S frame_info{};
        frame_info.uSize = sizeof(frame_info);
        frame_info.uSeq = from_be16(header.wSeq);
        frame_info.uTimestamp = from_be32(header.uTimestamp);
        frame_info.uPayloadLen = payload_len;
        frame_info.uFlags = from_be32(header.uFlags);
        switch (header.byPayloadType) {
            case NET_RECORD_FRAME_PAYLOAD_TYPE_AUDIO:
                frame_info.uMediaType = NET_RECORD_FRAME_MEDIA_AUDIO;
                break;
            case NET_RECORD_FRAME_PAYLOAD_TYPE_END:
                frame_info.uMediaType = NET_RECORD_FRAME_MEDIA_END;
                frame_info.uFlags |= NET_RECORD_FRAME_FLAG_STREAM_END;
                break;
            case NET_RECORD_FRAME_PAYLOAD_TYPE_VIDEO:
            default:
                frame_info.uMediaType = NET_RECORD_FRAME_MEDIA_VIDEO;
                break;
        }

        if (m_fnCallback) {
            m_fnCallback(frame_info, payload_len > 0 ? payload.data() : nullptr, payload_len);
        }

        if ((frame_info.uFlags & NET_RECORD_FRAME_FLAG_STREAM_END) != 0) {
            break;
        }
    }

    m_bRunning = false;
}

}  // namespace tvsdk
