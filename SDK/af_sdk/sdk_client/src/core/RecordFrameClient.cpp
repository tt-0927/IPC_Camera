/**
 * @file RecordFrameClient.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CRecordFrameClient 模块实现
 * 功能说明：
 * 1. 实现 CRecordFrameClient 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "RecordFrameClient.h"

#include <cstring>
#include <utility>
#include <vector>

#include "NetSdkLog.h"
#include "PlatformCompat.h"

namespace tvsdk {
namespace {
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 recv_exact 定义的内部处理。
 * @param [in] fd 函数处理参数。
 * @param [in,out] data 函数处理参数。
 * @param [in] size 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool recv_exact(socket_fd_t fd, char* data, size_t size) {
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
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 from_be16 定义的内部处理。
 * @param [in] value 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static uint16_t from_be16(uint16_t value) {
    return ntohs(value);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 from_be32 定义的内部处理。
 * @param [in] value 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static uint32_t from_be32(uint32_t value) {
    return ntohl(value);
}

} /* namespace */

CRecordFrameClient::CRecordFrameClient() = default;

CRecordFrameClient::~CRecordFrameClient() { stop(); }
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 start 对应的处理。
 * @return 返回该处理的状态或结果。
 */

bool CRecordFrameClient::start(const std::string& host,
                              int port,
                              const std::string& stream_id,
                              RecordFrameCallback callback) {
    if (m_bRunning) {
        NETSDK_LOG_MESSAGE_WARN("CRecordFrameClient: already running");
        return false;
    }
    if (host.empty() || port <= 0 || port > 65535 || stream_id.empty()) {
        return false;
    }

    m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nSocket == INVALID_SOCKET_FD) {
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameClient: socket failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
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
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameClient: invalid host %s", host.c_str());
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    if (connect(m_nSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameClient: connect %s:%d failed, errno=%d", host.c_str(), port, NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    int tcp_no_delay = 1;
    if (setsockopt(m_nSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&tcp_no_delay), sizeof(tcp_no_delay)) != 0) {
        NETSDK_LOG_MESSAGE_WARN("CRecordFrameClient: set TCP_NODELAY failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
    }

    m_strStreamId = stream_id;
    m_fnCallback = std::move(callback);
    m_bRunning = true;
    m_stReceiveThread = std::thread(&CRecordFrameClient::recv_loop, this);

    NETSDK_LOG_MESSAGE_INFO("CRecordFrameClient: connected to %s:%d, stream_id=%s", host.c_str(), port, stream_id.c_str());
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 stop 对应的处理。
 * @return 无返回值。
 */

void CRecordFrameClient::stop() {
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
    NETSDK_LOG_MESSAGE_INFO("CRecordFrameClient: stopped, stream_id=%s", m_strStreamId.c_str());
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 recv_loop 定义的内部处理。
 * @return 无返回值。
 */

void CRecordFrameClient::recv_loop() {
    while (m_bRunning) {
        NET_TV_RECORD_FRAME_RTP_HEADER_S header{};
        if (!recv_exact(m_nSocket, reinterpret_cast<char*>(&header), sizeof(header))) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("CRecordFrameClient: recv header failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            break;
        }

        if (header.byVersion != 2) {
            NETSDK_LOG_MESSAGE_WARN("CRecordFrameClient: invalid rtp-like version %u", static_cast<unsigned>(header.byVersion));
            break;
        }

        const uint32_t payload_len = from_be32(header.dwPayloadLen);
        if (payload_len > NET_TV_RECORD_FRAME_MAX_PAYLOAD_SIZE) {
            NETSDK_LOG_MESSAGE_WARN("CRecordFrameClient: invalid payload len %u", payload_len);
            break;
        }

        std::vector<char> payload(payload_len);
        if (payload_len > 0 && !recv_exact(m_nSocket, payload.data(), payload_len)) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("CRecordFrameClient: recv payload failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            break;
        }

        NET_TV_RECORD_FRAME_INFO_S frame_info{};
        frame_info.dwSize = sizeof(frame_info);
        frame_info.dwSeq = from_be16(header.wSeq);
        frame_info.dwTimestamp = from_be32(header.dwTimestamp);
        frame_info.dwPayloadLen = payload_len;
        frame_info.dwFlags = from_be32(header.dwFlags);
        switch (header.byPayloadType) {
            case NET_TV_RECORD_FRAME_PAYLOAD_TYPE_AUDIO:
                frame_info.dwMediaType = NET_TV_RECORD_FRAME_MEDIA_AUDIO;
                break;
            case NET_TV_RECORD_FRAME_PAYLOAD_TYPE_END:
                frame_info.dwMediaType = NET_TV_RECORD_FRAME_MEDIA_END;
                frame_info.dwFlags |= NET_TV_RECORD_FRAME_FLAG_STREAM_END;
                break;
            case NET_TV_RECORD_FRAME_PAYLOAD_TYPE_VIDEO:
            default:
                frame_info.dwMediaType = NET_TV_RECORD_FRAME_MEDIA_VIDEO;
                break;
        }

        if (m_fnCallback) {
            m_fnCallback(frame_info, payload_len > 0 ? payload.data() : nullptr, payload_len);
        }

        if ((frame_info.dwFlags & NET_TV_RECORD_FRAME_FLAG_STREAM_END) != 0) {
            break;
        }
    }

    m_bRunning = false;
}

}  /* namespace tvsdk */
