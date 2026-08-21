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

struct VoiceComParamFrame {
    uint32_t magic;
    NET_TV_VOICECOM_AUDIO_PARAM_S audio_param;
};

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

void fill_default_voicecom_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) {
    std::memset(&audio_param, 0, sizeof(audio_param));
    audio_param.enFormat = NET_TV_AUDIO_FORMAT_PCM;
    audio_param.dwSampleRate = NET_TV_AUDIO_SAMPRATE_16000;
    audio_param.dwBitDepth = 16;
    audio_param.dwChannels = 1;
    audio_param.dwFrameIntervalMs = 20;
    audio_param.dwFrameBytes = 640;
    audio_param.dwBitRate = 256000;
    audio_param.bLittleEndian = TRUE;
}

bool normalize_voicecom_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) {
    if (audio_param.dwChannels != 1) {
        return false;
    }

    int bytes_per_sample = 0;
    switch (audio_param.enFormat) {
        case NET_TV_AUDIO_FORMAT_PCM:
            if (audio_param.dwBitDepth <= 0) {
                audio_param.dwBitDepth = 16;
            }
            if (audio_param.dwBitDepth != 16) {
                return false;
            }
            switch (audio_param.dwSampleRate) {
                case NET_TV_AUDIO_SAMPRATE_8000:
                case NET_TV_AUDIO_SAMPRATE_16000:
                    break;
                default:
                    return false;
            }
            bytes_per_sample = audio_param.dwBitDepth / 8;
            break;
        case NET_TV_AUDIO_FORMAT_G711A:
        case NET_TV_AUDIO_FORMAT_G711U:
            if (audio_param.dwSampleRate != NET_TV_AUDIO_SAMPRATE_8000) {
                return false;
            }
            if (audio_param.dwBitDepth <= 0) {
                audio_param.dwBitDepth = 8;
            }
            if (audio_param.dwBitDepth != 8) {
                return false;
            }
            bytes_per_sample = 1;
            break;
        default:
            return false;
    }

    if (audio_param.dwFrameIntervalMs <= 0) {
        audio_param.dwFrameIntervalMs = 20;
    }
    if (audio_param.dwFrameIntervalMs < 10 || audio_param.dwFrameIntervalMs > 1000) {
        return false;
    }

    const int frame_bytes = audio_param.dwSampleRate * audio_param.dwChannels *
                            bytes_per_sample * audio_param.dwFrameIntervalMs / 1000;
    if (frame_bytes <= 0 || frame_bytes > NET_TV_LEN_4096) {
        return false;
    }

    if (audio_param.dwFrameBytes <= 0) {
        audio_param.dwFrameBytes = frame_bytes;
    }
    if (audio_param.dwFrameBytes != frame_bytes) {
        return false;
    }

    audio_param.dwBitRate = audio_param.dwSampleRate * audio_param.dwChannels * audio_param.dwBitDepth;
    audio_param.bLittleEndian = TRUE;
    return true;
}

int normalize_frame_interval_ms(int frame_interval_ms) {
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

bool is_voicecom_param_frame(const char* data, size_t size, NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) {
    if (!data || size != sizeof(VoiceComParamFrame)) {
        return false;
    }

    VoiceComParamFrame frame{};
    std::memcpy(&frame, data, sizeof(frame));
    if (ntohl(frame.magic) != kVoiceComParamMagic) {
        return false;
    }

    audio_param = frame.audio_param;
    return normalize_voicecom_audio_param(audio_param);
}

bool has_voicecom_param_magic(const char* data, size_t size) {
    if (!data || size != sizeof(VoiceComParamFrame)) {
        return false;
    }

    uint32_t magic = 0;
    std::memcpy(&magic, data, sizeof(magic));
    return ntohl(magic) == kVoiceComParamMagic;
}

} // namespace

VoiceComServer* VoiceComServer::instance() {
    static VoiceComServer s_instance;
    return &s_instance;
}

VoiceComServer::VoiceComServer() {
    fill_default_voicecom_audio_param(m_audio_param);
}

VoiceComServer::~VoiceComServer() { stop(); }

bool VoiceComServer::start(int port) {
    if (m_running) return true;

    m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listen_fd == INVALID_SOCKET_FD) {
        NSDK_LOG_ERROR("VoiceComServer: socket failed, errno=%d", socket_errno());
        return false;
    }

    int opt = 1;
    setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(m_listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        NSDK_LOG_ERROR("VoiceComServer: bind port %d failed, errno=%d", port, socket_errno());
        socket_close(m_listen_fd);
        m_listen_fd = INVALID_SOCKET_FD;
        return false;
    }

    if (listen(m_listen_fd, 1) != 0) {
        NSDK_LOG_ERROR("VoiceComServer: listen failed, errno=%d", socket_errno());
        socket_close(m_listen_fd);
        m_listen_fd = INVALID_SOCKET_FD;
        return false;
    }

    m_running = true;
    m_accept_thread = std::thread(&VoiceComServer::accept_loop, this);
    m_capture_thread = std::thread(&VoiceComServer::capture_loop, this);

    NSDK_LOG_INFO("VoiceComServer: listening on port %d", port);
    return true;
}

void VoiceComServer::stop() {
    m_running = false;
    m_param_cv.notify_all();

    if (m_listen_fd != INVALID_SOCKET_FD) {
        shutdown(m_listen_fd, SHUT_RDWR);
        socket_close(m_listen_fd);
        m_listen_fd = INVALID_SOCKET_FD;
    }
    {
        std::lock_guard<std::mutex> lock(m_send_mutex);
        if (m_client_fd != INVALID_SOCKET_FD) {
            shutdown(m_client_fd, SHUT_RDWR);
            m_client_fd = INVALID_SOCKET_FD;
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_param_mutex);
        fill_default_voicecom_audio_param(m_audio_param);
        m_has_audio_param = false;
    }
    m_param_cv.notify_all();

    if (m_accept_thread.joinable()) m_accept_thread.join();
    if (m_recv_thread.joinable()) m_recv_thread.join();
    if (m_capture_thread.joinable()) m_capture_thread.join();

    NSDK_LOG_INFO("VoiceComServer: stopped");
}

bool VoiceComServer::send_to_client(const char* data, size_t size) {
    if (data == nullptr || size == 0 || size > 0xFFFFu) {
        return false;
    }

    // 帧格式: [2B长度 大端][音频帧], 音频格式由首帧参数协商。
    uint16_t len_be = htons(static_cast<uint16_t>(size));
    std::vector<char> frame(sizeof(len_be) + size);
    std::memcpy(frame.data(), &len_be, sizeof(len_be));
    std::memcpy(frame.data() + sizeof(len_be), data, size);

#ifdef _WIN32
    int send_flags = 0;
#else
    int send_flags = MSG_NOSIGNAL;
#endif

    std::lock_guard<std::mutex> lock(m_send_mutex);
    const socket_fd_t fd = m_client_fd;
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

void VoiceComServer::set_play_callback(VoiceComPlayCallback cb) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_play_cb = std::move(cb);
}

void VoiceComServer::set_capture_callback(VoiceComCaptureCallback cb) {
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        m_capture_cb = std::move(cb);
    }
    m_param_cv.notify_all();
}

bool VoiceComServer::get_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) const {
    return snapshot_audio_param(audio_param);
}

bool VoiceComServer::snapshot_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) const {
    std::lock_guard<std::mutex> lock(m_param_mutex);
    if (!m_has_audio_param) {
        return false;
    }

    audio_param = m_audio_param;
    return true;
}

void VoiceComServer::notify_audio_param_ready() {
    m_param_cv.notify_all();
}

void VoiceComServer::capture_loop() {
    bool logged_wait_source = false;
    bool logged_send_failed = false;
    uint64_t frame_count = 0;

    /*
     * VoiceCom协议层只负责会话参数、拉帧节拍和网络发送。
     * 实际音频采集由业务注册的采集回调提供，避免SDK强依赖具体芯片或音频驱动。
     */
    while (m_running) {
        NET_TV_VOICECOM_AUDIO_PARAM_S audio_param{};
        {
            std::unique_lock<std::mutex> lock(m_param_mutex);
            m_param_cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !m_running || m_has_audio_param;
            });

            if (!m_running) {
                break;
            }
            if (!m_has_audio_param) {
                continue;
            }
            audio_param = m_audio_param;
        }

        VoiceComCaptureCallback capture_cb;
        {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            capture_cb = m_capture_cb;
        }
        if (!capture_cb) {
            if (!logged_wait_source) {
                NSDK_LOG_INFO("VoiceComServer: waiting voice capture callback");
                logged_wait_source = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        logged_wait_source = false;

        const size_t frame_bytes = static_cast<size_t>(audio_param.dwFrameBytes);
        if (frame_bytes == 0 || frame_bytes > NET_TV_LEN_4096) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::vector<char> frame(frame_bytes);
        auto next_send_time = std::chrono::steady_clock::now();

        while (m_running) {
            {
                std::lock_guard<std::mutex> lock(m_callback_mutex);
                capture_cb = m_capture_cb;
            }
            if (!capture_cb) {
                break;
            }

            NET_TV_VOICECOM_AUDIO_PARAM_S current_param{};
            if (!snapshot_audio_param(current_param) ||
                current_param.enFormat != audio_param.enFormat ||
                current_param.dwSampleRate != audio_param.dwSampleRate ||
                current_param.dwBitDepth != audio_param.dwBitDepth ||
                current_param.dwChannels != audio_param.dwChannels ||
                current_param.dwFrameBytes != audio_param.dwFrameBytes ||
                current_param.dwFrameIntervalMs != audio_param.dwFrameIntervalMs) {
                break;
            }

            int read_bytes = capture_cb(current_param, frame.data(), frame.size());
            if (read_bytes <= 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(normalize_frame_interval_ms(current_param.dwFrameIntervalMs)));
                next_send_time = std::chrono::steady_clock::now();
                continue;
            }

            if (static_cast<size_t>(read_bytes) > frame.size()) {
                NSDK_LOG_WARN("VoiceComServer: capture callback returned oversized frame, bytes=%d, max=%zu",
                              read_bytes, frame.size());
                read_bytes = static_cast<int>(frame.size());
            }

            std::this_thread::sleep_until(next_send_time);
            next_send_time += std::chrono::milliseconds(
                normalize_frame_interval_ms(current_param.dwFrameIntervalMs));

            if (!send_to_client(frame.data(), static_cast<size_t>(read_bytes))) {
                if (!logged_send_failed) {
                    NSDK_LOG_WARN("VoiceComServer: send captured voice frame failed");
                    logged_send_failed = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                next_send_time = std::chrono::steady_clock::now();
                continue;
            }

            logged_send_failed = false;
            ++frame_count;
            if (frame_count <= 5 || (frame_count % 100) == 0) {
                NSDK_LOG_DEBUG("VoiceComServer: sent captured voice frame=%llu bytes=%d",
                               static_cast<unsigned long long>(frame_count),
                               read_bytes);
            }
        }
    }
}

void VoiceComServer::accept_loop() {
    while (m_running) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        socket_fd_t fd = accept(m_listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (fd == INVALID_SOCKET_FD) {
            if (m_running) {
                NSDK_LOG_WARN("VoiceComServer: accept failed, errno=%d", socket_errno());
            }
            continue;
        }

        int tcp_no_delay = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&tcp_no_delay), sizeof(tcp_no_delay)) != 0) {
            NSDK_LOG_WARN("VoiceComServer: set TCP_NODELAY failed, errno=%d", socket_errno());
        }

        {
            std::lock_guard<std::mutex> lock(m_send_mutex);
            if (m_client_fd != INVALID_SOCKET_FD) {
                // 关闭旧连接，唤醒旧接收线程从 recv_exact() 中退出。
                shutdown(m_client_fd, SHUT_RDWR);
            }
        }

        // std::thread 对象即使线程函数已经返回，在 join 前仍然是 joinable。
        // 必须在创建新接收线程前回收旧线程，否则重新赋值会触发 std::terminate。
        if (m_recv_thread.joinable()) m_recv_thread.join();

        {
            std::lock_guard<std::mutex> lock(m_param_mutex);
            fill_default_voicecom_audio_param(m_audio_param);
            m_has_audio_param = false;
        }
        {
            std::lock_guard<std::mutex> lock(m_send_mutex);
            m_client_fd = fd;
        }
        m_recv_thread = std::thread(&VoiceComServer::client_recv_loop, this, fd);

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        NSDK_LOG_INFO("VoiceComServer: client connected from %s:%d",
                      ip_str, ntohs(client_addr.sin_port));
    }
}

void VoiceComServer::client_recv_loop(socket_fd_t fd) {
    char header[2];
    char buffer[4096];

    while (m_running && fd != INVALID_SOCKET_FD) {
        if (!recv_exact(fd, header, sizeof(header))) break;

        uint16_t len_be;
        std::memcpy(&len_be, header, sizeof(len_be));
        size_t frame_len = ntohs(len_be);

        if (frame_len == 0 || frame_len > sizeof(buffer)) continue;

        if (!recv_exact(fd, buffer, frame_len)) {
            break;
        }

        NET_TV_VOICECOM_AUDIO_PARAM_S audio_param{};
        if (is_voicecom_param_frame(buffer, frame_len, audio_param)) {
            {
                std::lock_guard<std::mutex> lock(m_param_mutex);
                m_audio_param = audio_param;
                m_has_audio_param = true;
            }
            NSDK_LOG_INFO("VoiceComServer: audio param format=%d sampleRate=%d bitDepth=%d channels=%d frameMs=%d frameBytes=%d",
                          audio_param.enFormat,
                          audio_param.dwSampleRate,
                          audio_param.dwBitDepth,
                          audio_param.dwChannels,
                          audio_param.dwFrameIntervalMs,
                          audio_param.dwFrameBytes);
            notify_audio_param_ready();
            continue;
        }
        if (has_voicecom_param_magic(buffer, frame_len)) {
            NSDK_LOG_WARN("VoiceComServer: invalid audio param frame");
            break;
        }

        VoiceComPlayCallback play_cb;
        {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            play_cb = m_play_cb;
        }
        if (play_cb) {
            play_cb(buffer, frame_len);
        }
    }

    NSDK_LOG_INFO("VoiceComServer: client disconnected");
    if (fd != INVALID_SOCKET_FD) {
        socket_close(fd);
    }

    bool clear_param = false;
    {
        std::lock_guard<std::mutex> lock(m_send_mutex);
        if (m_client_fd == fd) {
            m_client_fd = INVALID_SOCKET_FD;
            clear_param = true;
        }
    }
    if (clear_param) {
        std::lock_guard<std::mutex> lock(m_param_mutex);
        fill_default_voicecom_audio_param(m_audio_param);
        m_has_audio_param = false;
    }
    if (clear_param) {
        notify_audio_param_ready();
    }
}

}  // namespace tvsdk
