/**
 * @file VoiceComClient.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CVoiceComClient 模块实现
 * 功能说明：
 * 1. 实现 CVoiceComClient 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : CVoiceComClient.cpp
 * @Description  : 语音对讲客户端实现，负责与设备建立TCP音频通道、收发音频数据
 */

#include "VoiceComClient.h"

#include <cstring>
#include <utility>
#include <vector>
#include <cstdio>

#include "NetSdkLog.h"
#include "PlatformCompat.h"

namespace tvsdk {
namespace {

constexpr uint32_t kVoiceComParamMagic = 0x56435031;  /*/< 音频参数帧魔数（VCP1） */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 音频参数帧结构体
 * @details 首帧发送的音频参数协商帧，包含魔数和音频参数
 */
struct VoiceComParamFrame_S {
    uint32_t magic;                            /*/< 魔数，固定为0x56435031（VCP1） */
    NET_VoiceComAudioParam_S audio_param; /*/< 音频参数（采样率、通道数、编码格式等） */
};

#ifdef VOICECOM_DEBUG_PRINT

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 检查音频数据是否包含ADTS头
 * @details ADTS头以0xFF 0xF0开头，用于识别AAC音频帧
 * @param data 音频数据指针
 * @param size 音频数据大小（字节）
 * @return true表示包含ADTS头，false表示不包含
 */
static bool has_adts_header(const void* data, size_t size) {
    if (data == nullptr || size < 2) {
        return false;
    }

    const auto* p = static_cast<const unsigned char*>(data);
    return p[0] == 0xFF && (p[1] & 0xF0) == 0xF0;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 格式化数据头部为十六进制字符串
 * @details 最多显示前16字节的十六进制值，用于调试打印
 * @param data 数据指针
 * @param size 数据大小（字节）
 * @param output 输出缓冲区
 * @param outputSize 输出缓冲区大小
 */
static void format_head_hex(const void* data, size_t size, char* output, size_t outputSize) {
    if (output == nullptr || outputSize == 0) {
        return;
    }

    output[0] = '\0';
    if (data == nullptr || size == 0) {
        return;
    }

    const auto* p = static_cast<const unsigned char*>(data);
    const size_t dumpSize = size < 16 ? size : 16;
    size_t used = 0;
    for (size_t i = 0; i < dumpSize && used < outputSize; ++i) {
        int written = std::snprintf(output + used, outputSize - used,
                                    (i == 0) ? "%02X" : " %02X", p[i]);
        if (written <= 0) {
            break;
        }
        used += static_cast<size_t>(written);
        if (used >= outputSize) {
            break;
        }
    }
}

#endif /* VOICECOM_DEBUG_PRINT */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 精确接收指定长度的数据
 * @details 循环调用recv直到接收完指定长度或出错
 * @param fd socket文件描述符
 * @param data 接收缓冲区指针
 * @param size 需要接收的字节数
 * @return true表示成功接收全部数据，false表示失败
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

} /* namespace */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 构造函数
 */
CVoiceComClient::CVoiceComClient() = default;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 析构函数
 * @details 自动停止对讲并释放资源
 */
CVoiceComClient::~CVoiceComClient() { stop(); }

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 连接设备音频端口并启动收发
 * @details 完整流程：
 *          1. 创建TCP socket
 *          2. 设置超时时间（5秒）
 *          3. 连接到设备音频端口
 *          4. 启用TCP_NODELAY（禁用Nagle算法）
 *          5. 发送音频参数协商帧（VCP1）
 *          6. 启动接收线程
 * @param host 设备IP地址
 * @param port 设备音频端口号
 * @param audio_param 音频参数（采样率、通道数、编码格式等）
 * @param callback 音频数据回调函数，用于接收设备发送的音频数据
 * @return true表示成功，false表示失败
 */
bool CVoiceComClient::start(const std::string& host,
                           int port,
                           const NET_VoiceComAudioParam_S& audio_param,
                           VoiceComCallback callback) {
    if (m_bRunning) {
        NETSDK_LOG_MESSAGE_WARN("CVoiceComClient: already running");
        return false;
    }

    m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nSocket == INVALID_SOCKET_FD) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComClient: socket failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        return false;
    }

    /* 设置5秒超时 */
    timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(m_nSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    setsockopt(m_nSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComClient: invalid host %s", host.c_str());
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    if (connect(m_nSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComClient: connect %s:%d failed, errno=%d",
                       host.c_str(), port, NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    int tcp_no_delay = 1;
    if (setsockopt(m_nSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&tcp_no_delay), sizeof(tcp_no_delay)) != 0) {
        NETSDK_LOG_MESSAGE_WARN("CVoiceComClient: set TCP_NODELAY failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
    }

    if (!send_audio_param(audio_param)) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComClient: send audio param failed");
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
        return false;
    }

    m_fnCallback = std::move(callback);
    m_bRunning = true;
    m_stReceiveThread = std::thread(&CVoiceComClient::recv_loop, this);

    NETSDK_LOG_MESSAGE_INFO("CVoiceComClient: connected to %s:%d", host.c_str(), port);
    return true;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送音频参数协商帧
 * @details 将音频参数封装为VCP1帧格式，包含魔数和参数数据
 * @param audio_param 音频参数结构体
 * @return true表示成功，false表示失败
 */
bool CVoiceComClient::send_audio_param(const NET_VoiceComAudioParam_S& audio_param) {
    VoiceComParamFrame_S frame{};
    frame.magic = htonl(kVoiceComParamMagic);
    frame.audio_param = audio_param;
    return send_frame(reinterpret_cast<const char*>(&frame), sizeof(frame));
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送一帧数据（带长度头）
 * @details 帧格式：[2字节长度（大端）][音频负载]，首帧负载为VCP1参数帧
 * @param data 帧数据指针
 * @param size 帧数据大小（字节）
 * @return true表示成功，false表示失败
 */
bool CVoiceComClient::send_frame(const char* data, size_t size) {
    if (m_nSocket == INVALID_SOCKET_FD || data == nullptr || size == 0 || size > 0xFFFFu) {
        return false;
    }

    uint16_t len_be = htons(static_cast<uint16_t>(size));
    std::vector<char> frame(sizeof(len_be) + size);
    std::memcpy(frame.data(), &len_be, sizeof(len_be));
    std::memcpy(frame.data() + sizeof(len_be), data, size);

#ifdef _WIN32
    int send_flags = 0;
#else
    int send_flags = MSG_NOSIGNAL;
#endif

    std::lock_guard<std::mutex> lock(m_stSendMutex);
    size_t sent = 0;
    while (sent < frame.size()) {
        ssize_t n = ::send(m_nSocket, frame.data() + sent, static_cast<int>(frame.size() - sent), send_flags);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送音频数据到设备
 * @param data 音频数据指针
 * @param size 音频数据大小（字节）
 * @return true表示成功，false表示失败
 */
bool CVoiceComClient::send(const char* data, size_t size) {
    if (!m_bRunning || data == nullptr || size == 0) {
        return false;
    }

#ifdef VOICECOM_DEBUG_PRINT
    const uint64_t frame_count = ++m_uSentFrameCount;
    if (frame_count <= 10 || (frame_count % 100) == 0) {
        char head_hex[64];
        format_head_hex(data, size, head_hex, sizeof(head_hex));
        NETSDK_LOG_MESSAGE_INFO("CVoiceComClient: send audio frame=%llu bytes=%zu has_adts=%d head=%s",
                      static_cast<unsigned long long>(frame_count),
                      size,
                      has_adts_header(data, size) ? 1 : 0,
                      head_hex);
    }
#endif

    return send_frame(data, size);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止并关闭连接
 * @details 完整流程：
 *          1. 设置运行标志为false
 *          2. 关闭socket连接
 *          3. 等待接收线程退出
 */
void CVoiceComClient::stop() {
    if (!m_bRunning) return;

    m_bRunning = false;
    if (m_nSocket != INVALID_SOCKET_FD) {
        shutdown(m_nSocket, SHUT_RDWR);
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
    }
    if (m_stReceiveThread.joinable()) {
        m_stReceiveThread.join();
    }
    NETSDK_LOG_MESSAGE_INFO("CVoiceComClient: stopped");
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 接收数据循环
 * @details 持续从socket读取音频帧，解析后通过回调转发给上层：
 *          1. 读取2字节帧头（大端长度）
 *          2. 根据长度读取帧数据
 *          3. 通过回调函数转发给上层
 */
void CVoiceComClient::recv_loop() {
    char header[2];
    char buffer[4096];

    while (m_bRunning) {
        /* 读帧头: 2字节长度（大端） */
        if (!recv_exact(m_nSocket, header, sizeof(header))) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("CVoiceComClient: recv header failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            break;
        }

        uint16_t len_be;
        std::memcpy(&len_be, header, sizeof(len_be));
        size_t frame_len = ntohs(len_be);

        if (frame_len == 0 || frame_len > sizeof(buffer)) {
            NETSDK_LOG_MESSAGE_WARN("CVoiceComClient: invalid frame len %zu", frame_len);
            continue;
        }

        /* 读帧数据 */
        if (recv_exact(m_nSocket, buffer, frame_len) && m_fnCallback) {
#ifdef VOICECOM_DEBUG_PRINT
            const uint64_t frame_count = ++m_uReceivedFrameCount;
            if (frame_count <= 10 || (frame_count % 100) == 0) {
                char head_hex[64];
                format_head_hex(buffer, frame_len, head_hex, sizeof(head_hex));
                NETSDK_LOG_MESSAGE_INFO("CVoiceComClient: recv audio frame=%llu bytes=%zu has_adts=%d head=%s",
                              static_cast<unsigned long long>(frame_count),
                              frame_len,
                              has_adts_header(buffer, frame_len) ? 1 : 0,
                              head_hex);
            }
#endif
            m_fnCallback(buffer, frame_len);
        }
    }
}

}  /* namespace tvsdk */
