#include "VoiceComClient.h"

#include <cstring>
#include <utility>
#include <vector>

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

} // namespace

VoiceComClient::VoiceComClient() = default;

VoiceComClient::~VoiceComClient() { stop(); }

bool VoiceComClient::start(const std::string& host,
                           int port,
                           const NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param,
                           VoiceComCallback callback) {
    if (m_running) {
        NSDK_LOG_WARN("VoiceComClient: already running");
        return false;
    }

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET_FD) {
        NSDK_LOG_ERROR("VoiceComClient: socket failed, errno=%d", socket_errno());
        return false;
    }

    // 设置超时
    timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        NSDK_LOG_ERROR("VoiceComClient: invalid host %s", host.c_str());
        socket_close(m_socket);
        m_socket = INVALID_SOCKET_FD;
        return false;
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        NSDK_LOG_ERROR("VoiceComClient: connect %s:%d failed, errno=%d",
                       host.c_str(), port, socket_errno());
        socket_close(m_socket);
        m_socket = INVALID_SOCKET_FD;
        return false;
    }

    int tcp_no_delay = 1;
    if (setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&tcp_no_delay), sizeof(tcp_no_delay)) != 0) {
        NSDK_LOG_WARN("VoiceComClient: set TCP_NODELAY failed, errno=%d", socket_errno());
    }

    if (!send_audio_param(audio_param)) {
        NSDK_LOG_ERROR("VoiceComClient: send audio param failed");
        socket_close(m_socket);
        m_socket = INVALID_SOCKET_FD;
        return false;
    }

    m_callback = std::move(callback);
    m_running = true;
    m_recv_thread = std::thread(&VoiceComClient::recv_loop, this);

    NSDK_LOG_INFO("VoiceComClient: connected to %s:%d", host.c_str(), port);
    return true;
}

bool VoiceComClient::send_audio_param(const NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) {
    VoiceComParamFrame frame{};
    frame.magic = htonl(kVoiceComParamMagic);
    frame.audio_param = audio_param;
    return send_frame(reinterpret_cast<const char*>(&frame), sizeof(frame));
}

bool VoiceComClient::send_frame(const char* data, size_t size) {
    if (m_socket == INVALID_SOCKET_FD || data == nullptr || size == 0 || size > 0xFFFFu) {
        return false;
    }

    // 帧格式: [2B长度 大端][音频负载]，首帧负载为 VCP1 参数帧。
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
    size_t sent = 0;
    while (sent < frame.size()) {
        ssize_t n = ::send(m_socket, frame.data() + sent, static_cast<int>(frame.size() - sent), send_flags);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool VoiceComClient::send(const char* data, size_t size) {
    if (!m_running || data == nullptr || size == 0) {
        return false;
    }

    return send_frame(data, size);
}

void VoiceComClient::stop() {
    if (!m_running) return;

    m_running = false;
    if (m_socket != INVALID_SOCKET_FD) {
        shutdown(m_socket, SHUT_RDWR);
        socket_close(m_socket);
        m_socket = INVALID_SOCKET_FD;
    }
    if (m_recv_thread.joinable()) {
        m_recv_thread.join();
    }
    NSDK_LOG_INFO("VoiceComClient: stopped");
}

void VoiceComClient::recv_loop() {
    char header[2];
    char buffer[4096];

    while (m_running) {
        // 读帧头: 2字节长度
        if (!recv_exact(m_socket, header, sizeof(header))) {
            if (m_running) {
                NSDK_LOG_WARN("VoiceComClient: recv header failed, errno=%d", socket_errno());
            }
            break;
        }

        uint16_t len_be;
        std::memcpy(&len_be, header, sizeof(len_be));
        size_t frame_len = ntohs(len_be);

        if (frame_len == 0 || frame_len > sizeof(buffer)) {
            NSDK_LOG_WARN("VoiceComClient: invalid frame len %zu", frame_len);
            continue;
        }

        // 读帧数据
        if (recv_exact(m_socket, buffer, frame_len) && m_callback) {
            m_callback(buffer, frame_len);
        }
    }
}

}  // namespace tvsdk