/**
 * @file VoiceComServer.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CVoiceComServer 模块实现
 * 功能说明：
 * 1. 实现 CVoiceComServer 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "VoiceComServer.h"

#include <cstring>
#include <utility>
#include <vector>
#include <chrono>

#include "NetSdkLog.h"
#include "PlatformCompat.h"

namespace tvsdk {
namespace {

constexpr uint32_t kVoiceComParamMagic = 0x56435031;

struct VoiceComParamFrame_S {
    uint32_t magic;
    NET_VoiceComAudioParam_S audio_param;
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 fill_default_voicecom_audio_param 对应的处理。
 * @param [in,out] audio_param 函数处理参数。
 * @return 无返回值。
 */

static void fill_default_voicecom_audio_param(NET_VoiceComAudioParam_S& audio_param) {
    std::memset(&audio_param, 0, sizeof(audio_param));
    audio_param.enFormat = NET_AUDIO_FORMAT_PCM;
    audio_param.uSampleRate = NET_AUDIO_SAMPRATE_16000;
    audio_param.uBitDepth = 16;
    audio_param.uChannels = 1;
    audio_param.uFrameIntervalMs = 20;
    audio_param.uFrameBytes = 640;
    audio_param.uBitRate = 256000;
    audio_param.bLittleEndian = NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 normalize_voicecom_audio_param 对应的处理。
 * @param [in,out] audio_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool normalize_voicecom_audio_param(NET_VoiceComAudioParam_S& audio_param) {
    if (audio_param.uChannels != 1) {
        return false;
    }

    int bytes_per_sample = 0;
    switch (audio_param.enFormat) {
        case NET_AUDIO_FORMAT_PCM:
            if (audio_param.uBitDepth <= 0) {
                audio_param.uBitDepth = 16;
            }
            if (audio_param.uBitDepth != 16) {
                return false;
            }
            switch (audio_param.uSampleRate) {
                case NET_AUDIO_SAMPRATE_8000:
                case NET_AUDIO_SAMPRATE_16000:
                    break;
                default:
                    return false;
            }
            bytes_per_sample = audio_param.uBitDepth / 8;
            break;
        case NET_AUDIO_FORMAT_G711A:
        case NET_AUDIO_FORMAT_G711U:
            if (audio_param.uSampleRate != NET_AUDIO_SAMPRATE_8000) {
                return false;
            }
            if (audio_param.uBitDepth <= 0) {
                audio_param.uBitDepth = 8;
            }
            if (audio_param.uBitDepth != 8) {
                return false;
            }
            bytes_per_sample = 1;
            break;
        default:
            return false;
    }

    if (audio_param.uFrameIntervalMs <= 0) {
        audio_param.uFrameIntervalMs = 20;
    }
    if (audio_param.uFrameIntervalMs < 10 || audio_param.uFrameIntervalMs > 1000) {
        return false;
    }

    const int frame_bytes = audio_param.uSampleRate * audio_param.uChannels *
                            bytes_per_sample * audio_param.uFrameIntervalMs / 1000;
    if (frame_bytes <= 0 || frame_bytes > NET_LEN_4096) {
        return false;
    }

    if (audio_param.uFrameBytes <= 0) {
        audio_param.uFrameBytes = frame_bytes;
    }
    if (audio_param.uFrameBytes != frame_bytes) {
        return false;
    }

    audio_param.uBitRate = audio_param.uSampleRate * audio_param.uChannels * audio_param.uBitDepth;
    audio_param.bLittleEndian = NET_TRUE;
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 normalize_frame_interval_ms 对应的处理。
 * @param [in] frame_interval_ms 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static int normalize_frame_interval_ms(int frame_interval_ms) {
    if (frame_interval_ms <= 0) {
        return 20;
    }
    if (frame_interval_ms < 10) {
        return 10;
    }
    if (frame_interval_ms > 1000) {
        return 1000;
    }
    return frame_interval_ms;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 is_voicecom_param_frame 对应的数据。
 * @param [in] data 函数处理参数。
 * @param [in] size 函数处理参数。
 * @param [in,out] audio_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool is_voicecom_param_frame(const char* data, size_t size, NET_VoiceComAudioParam_S& audio_param) {
    if (!data || size != sizeof(VoiceComParamFrame_S)) {
        return false;
    }

    VoiceComParamFrame_S frame{};
    std::memcpy(&frame, data, sizeof(frame));
    if (ntohl(frame.magic) != kVoiceComParamMagic) {
        return false;
    }

    audio_param = frame.audio_param;
    return normalize_voicecom_audio_param(audio_param);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 has_voicecom_param_magic 对应的数据。
 * @param [in] data 函数处理参数。
 * @param [in] size 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool has_voicecom_param_magic(const char* data, size_t size) {
    if (!data || size != sizeof(VoiceComParamFrame_S)) {
        return false;
    }

    uint32_t magic = 0;
    std::memcpy(&magic, data, sizeof(magic));
    return ntohl(magic) == kVoiceComParamMagic;
}

} /* namespace */

CVoiceComServer* CVoiceComServer::instance() {
    static CVoiceComServer s_instance;
    return &s_instance;
}

CVoiceComServer::CVoiceComServer() {
    fill_default_voicecom_audio_param(m_stAudioParameter);
}

CVoiceComServer::~CVoiceComServer() { stop(); }
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 start 对应的处理。
 * @param [in] port 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CVoiceComServer::start(int port) {
    if (m_bRunning) return true;

    m_nListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nListenFd == INVALID_SOCKET_FD) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComServer: socket failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        return false;
    }

    int opt = 1;
    setsockopt(m_nListenFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(m_nListenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComServer: bind port %d failed, errno=%d", port, NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nListenFd);
        m_nListenFd = INVALID_SOCKET_FD;
        return false;
    }

    if (listen(m_nListenFd, 1) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CVoiceComServer: listen failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nListenFd);
        m_nListenFd = INVALID_SOCKET_FD;
        return false;
    }

    m_bRunning = true;
    m_stAcceptThread = std::thread(&CVoiceComServer::accept_loop, this);
    m_stCaptureThread = std::thread(&CVoiceComServer::capture_loop, this);

    NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: listening on port %d", port);
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 stop 对应的处理。
 * @return 无返回值。
 */

void CVoiceComServer::stop() {
    m_bRunning = false;
    m_stParameterCondition.notify_all();

    if (m_nListenFd != INVALID_SOCKET_FD) {
        shutdown(m_nListenFd, SHUT_RDWR);
        NETSDK_SOCKET_CLOSE(m_nListenFd);
        m_nListenFd = INVALID_SOCKET_FD;
    }
    {
        std::lock_guard<std::mutex> lock(m_stSendMutex);
        if (m_nClientFd != INVALID_SOCKET_FD) {
            shutdown(m_nClientFd, SHUT_RDWR);
            m_nClientFd = INVALID_SOCKET_FD;
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_stParameterMutex);
        fill_default_voicecom_audio_param(m_stAudioParameter);
        m_bHasAudioParameter = false;
    }
    m_stParameterCondition.notify_all();

    if (m_stAcceptThread.joinable()) m_stAcceptThread.join();
    if (m_stReceiveThread.joinable()) m_stReceiveThread.join();
    if (m_stCaptureThread.joinable()) m_stCaptureThread.join();

    NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: stopped");
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 send_to_client 对应的处理。
 * @param [in] data 函数处理参数。
 * @param [in] size 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CVoiceComServer::send_to_client(const char* data, size_t size) {
    if (data == nullptr || size == 0 || size > 0xFFFFu) {
        return false;
    }

    /* 帧格式: [2B长度 大端][音频帧], 音频格式由首帧参数协商。 */
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
    const socket_fd_t fd = m_nClientFd;
    if (fd == INVALID_SOCKET_FD) {
        return false;
    }

    size_t sent = 0;
    while (sent < frame.size()) {
        ssize_t n = ::send(fd, frame.data() + sent, static_cast<int>(frame.size() - sent), send_flags);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 set_play_callback 对应的处理。
 * @param [in] cb 函数处理参数。
 * @return 无返回值。
 */

void CVoiceComServer::set_play_callback(VoiceComPlayCallback cb) {
    std::lock_guard<std::mutex> lock(m_stCallbackMutex);
    m_fnPlayCallback = std::move(cb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 set_capture_callback 对应的处理。
 * @param [in] cb 函数处理参数。
 * @return 无返回值。
 */

void CVoiceComServer::set_capture_callback(VoiceComCaptureCallback cb) {
    {
        std::lock_guard<std::mutex> lock(m_stCallbackMutex);
        m_fnCaptureCallback = std::move(cb);
    }
    m_stParameterCondition.notify_all();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get_audio_param 对应的数据。
 * @param [in,out] audio_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CVoiceComServer::get_audio_param(NET_VoiceComAudioParam_S& audio_param) const {
    return snapshot_audio_param(audio_param);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 snapshot_audio_param 定义的内部处理。
 * @param [in,out] audio_param 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CVoiceComServer::snapshot_audio_param(NET_VoiceComAudioParam_S& audio_param) const {
    std::lock_guard<std::mutex> lock(m_stParameterMutex);
    if (!m_bHasAudioParameter) {
        return false;
    }

    audio_param = m_stAudioParameter;
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 notify_audio_param_ready 定义的内部处理。
 * @return 无返回值。
 */

void CVoiceComServer::notify_audio_param_ready() {
    m_stParameterCondition.notify_all();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 capture_loop 定义的内部处理。
 * @return 无返回值。
 */

void CVoiceComServer::capture_loop() {
    bool logged_wait_source = false;
    bool logged_send_failed = false;
    uint64_t frame_count = 0;

    /*
     * VoiceCom协议层只负责会话参数、拉帧节拍和网络发送。
     * 实际音频采集由业务注册的采集回调提供，避免SDK强依赖具体芯片或音频驱动。
     */
    while (m_bRunning) {
        NET_VoiceComAudioParam_S audio_param{};
        {
            std::unique_lock<std::mutex> lock(m_stParameterMutex);
            m_stParameterCondition.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !m_bRunning || m_bHasAudioParameter;
            });

            if (!m_bRunning) {
                break;
            }
            if (!m_bHasAudioParameter) {
                continue;
            }
            audio_param = m_stAudioParameter;
        }

        VoiceComCaptureCallback capture_cb;
        {
            std::lock_guard<std::mutex> lock(m_stCallbackMutex);
            capture_cb = m_fnCaptureCallback;
        }
        if (!capture_cb) {
            if (!logged_wait_source) {
                NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: waiting voice capture callback");
                logged_wait_source = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        logged_wait_source = false;

        const size_t frame_bytes = static_cast<size_t>(audio_param.uFrameBytes);
        if (frame_bytes == 0 || frame_bytes > NET_LEN_4096) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::vector<char> frame(frame_bytes);
        auto next_send_time = std::chrono::steady_clock::now();

        while (m_bRunning) {
            {
                std::lock_guard<std::mutex> lock(m_stCallbackMutex);
                capture_cb = m_fnCaptureCallback;
            }
            if (!capture_cb) {
                break;
            }

            NET_VoiceComAudioParam_S current_param{};
            if (!snapshot_audio_param(current_param) ||
                current_param.enFormat != audio_param.enFormat ||
                current_param.uSampleRate != audio_param.uSampleRate ||
                current_param.uBitDepth != audio_param.uBitDepth ||
                current_param.uChannels != audio_param.uChannels ||
                current_param.uFrameBytes != audio_param.uFrameBytes ||
                current_param.uFrameIntervalMs != audio_param.uFrameIntervalMs) {
                break;
            }

            int read_bytes = capture_cb(current_param, frame.data(), frame.size());
            if (read_bytes <= 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(normalize_frame_interval_ms(current_param.uFrameIntervalMs)));
                next_send_time = std::chrono::steady_clock::now();
                continue;
            }

            if (static_cast<size_t>(read_bytes) > frame.size()) {
                NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: capture callback returned oversized frame, bytes=%d, max=%zu",
                              read_bytes, frame.size());
                read_bytes = static_cast<int>(frame.size());
            }

            std::this_thread::sleep_until(next_send_time);
            next_send_time += std::chrono::milliseconds(
                normalize_frame_interval_ms(current_param.uFrameIntervalMs));

            if (!send_to_client(frame.data(), static_cast<size_t>(read_bytes))) {
                if (!logged_send_failed) {
                    NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: send captured voice frame failed");
                    logged_send_failed = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                next_send_time = std::chrono::steady_clock::now();
                continue;
            }

            logged_send_failed = false;
            ++frame_count;
            if (frame_count <= 5 || (frame_count % 100) == 0) {
                NETSDK_LOG_MESSAGE_DEBUG("CVoiceComServer: sent captured voice frame=%llu bytes=%d",
                               static_cast<unsigned long long>(frame_count),
                               read_bytes);
            }
        }
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 accept_loop 定义的内部处理。
 * @return 无返回值。
 */

void CVoiceComServer::accept_loop() {
    while (m_bRunning) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        socket_fd_t fd = accept(m_nListenFd, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (fd == INVALID_SOCKET_FD) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: accept failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            continue;
        }

        int tcp_no_delay = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&tcp_no_delay), sizeof(tcp_no_delay)) != 0) {
            NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: set TCP_NODELAY failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        }

        {
            std::lock_guard<std::mutex> lock(m_stSendMutex);
            if (m_nClientFd != INVALID_SOCKET_FD) {
                /* 关闭旧连接，唤醒旧接收线程从 recv_exact() 中退出。 */
                shutdown(m_nClientFd, SHUT_RDWR);
            }
        }

        /* std::thread 对象即使线程函数已经返回，在 join 前仍然是 joinable。 */
        /* 必须在创建新接收线程前回收旧线程，否则重新赋值会触发 std::terminate。 */
        if (m_stReceiveThread.joinable()) m_stReceiveThread.join();

        {
            std::lock_guard<std::mutex> lock(m_stParameterMutex);
            fill_default_voicecom_audio_param(m_stAudioParameter);
            m_bHasAudioParameter = false;
        }
        {
            std::lock_guard<std::mutex> lock(m_stSendMutex);
            m_nClientFd = fd;
        }
        m_stReceiveThread = std::thread(&CVoiceComServer::client_recv_loop, this, fd);

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: client connected from %s:%d",
                      ip_str, ntohs(client_addr.sin_port));
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 client_recv_loop 定义的内部处理。
 * @param [in] fd 函数处理参数。
 * @return 无返回值。
 */

void CVoiceComServer::client_recv_loop(socket_fd_t fd) {
    char header[2];
    char buffer[4096];

    while (m_bRunning && fd != INVALID_SOCKET_FD) {
        if (!recv_exact(fd, header, sizeof(header))) break;

        uint16_t len_be;
        std::memcpy(&len_be, header, sizeof(len_be));
        size_t frame_len = ntohs(len_be);

        if (frame_len == 0 || frame_len > sizeof(buffer)) continue;

        if (!recv_exact(fd, buffer, frame_len)) {
            break;
        }

        NET_VoiceComAudioParam_S audio_param{};
        if (is_voicecom_param_frame(buffer, frame_len, audio_param)) {
            {
                std::lock_guard<std::mutex> lock(m_stParameterMutex);
                m_stAudioParameter = audio_param;
                m_bHasAudioParameter = true;
            }
            NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: audio param format=%d sampleRate=%d bitDepth=%d channels=%d frameMs=%d frameBytes=%d",
                          audio_param.enFormat,
                          audio_param.uSampleRate,
                          audio_param.uBitDepth,
                          audio_param.uChannels,
                          audio_param.uFrameIntervalMs,
                          audio_param.uFrameBytes);
            notify_audio_param_ready();
            continue;
        }
        if (has_voicecom_param_magic(buffer, frame_len)) {
            NETSDK_LOG_MESSAGE_WARN("CVoiceComServer: invalid audio param frame");
            break;
        }

        VoiceComPlayCallback play_cb;
        {
            std::lock_guard<std::mutex> lock(m_stCallbackMutex);
            play_cb = m_fnPlayCallback;
        }
        if (play_cb) {
            play_cb(buffer, frame_len);
        }
    }

    NETSDK_LOG_MESSAGE_INFO("CVoiceComServer: client disconnected");
    if (fd != INVALID_SOCKET_FD) {
        NETSDK_SOCKET_CLOSE(fd);
    }

    bool clear_param = false;
    {
        std::lock_guard<std::mutex> lock(m_stSendMutex);
        if (m_nClientFd == fd) {
            m_nClientFd = INVALID_SOCKET_FD;
            clear_param = true;
        }
    }
    if (clear_param) {
        std::lock_guard<std::mutex> lock(m_stParameterMutex);
        fill_default_voicecom_audio_param(m_stAudioParameter);
        m_bHasAudioParameter = false;
    }
    if (clear_param) {
        notify_audio_param_ready();
    }
}

}  /* namespace tvsdk */
