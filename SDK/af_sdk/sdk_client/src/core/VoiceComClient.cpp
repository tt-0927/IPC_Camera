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
#include "VoiceComClient.h"

#include <cstring>
#include <utility>
#include <vector>

#include "NetSdkLog.h"
#include "PlatformCompat.h"

namespace tvsdk {
namespace {

constexpr uint32_t kVoiceComParamMagic = 0x56435031;

struct VoiceComParamFrame_S {
    uint32_t magic;
    NET_VOICECOM_AUDIO_PARAM_S audio_param;
};
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

} /* namespace */

CVoiceComClient::CVoiceComClient() = default;

CVoiceComClient::~CVoiceComClient() { stop(); }
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 start 对应的处理。
 * @return 返回该处理的状态或结果。
 */

bool CVoiceComClient::start(const std::string& host,
                           int port,
                           const NET_VOICECOM_AUDIO_PARAM_S& audio_param,
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

    /* 设置超时 */
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
 * @brief 执行 send_audio_param 对应的处理。
 * @param [in] audio_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CVoiceComClient::send_audio_param(const NET_VOICECOM_AUDIO_PARAM_S& audio_param) {
    VoiceComParamFrame_S frame{};
    frame.magic = htonl(kVoiceComParamMagic);
    frame.audio_param = audio_param;
    return send_frame(reinterpret_cast<const char*>(&frame), sizeof(frame));
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 send_frame 对应的处理。
 * @param [in] data 函数处理参数。
 * @param [in] size 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CVoiceComClient::send_frame(const char* data, size_t size) {
    if (m_nSocket == INVALID_SOCKET_FD || data == nullptr || size == 0 || size > 0xFFFFu) {
        return false;
    }

    /* 帧格式: [2B长度 大端][音频负载]，首帧负载为 VCP1 参数帧。 */
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
 * @brief 执行 send 对应的处理。
 * @param [in] data 函数处理参数。
 * @param [in] size 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CVoiceComClient::send(const char* data, size_t size) {
    if (!m_bRunning || data == nullptr || size == 0) {
        return false;
    }

    return send_frame(data, size);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 stop 对应的处理。
 * @return 无返回值。
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
 * @brief 执行 recv_loop 定义的内部处理。
 * @return 无返回值。
 */

void CVoiceComClient::recv_loop() {
    char header[2];
    char buffer[4096];

    while (m_bRunning) {
        /* 读帧头: 2字节长度 */
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
            m_fnCallback(buffer, frame_len);
        }
    }
}

}  /* namespace tvsdk */