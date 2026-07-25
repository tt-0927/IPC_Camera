#include "RecordFrameServer.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <utility>

#include "NetSdkLog.h"
#include "PlatformCompat.h"

namespace tvsdk {
namespace {

bool send_all(socket_fd_t fd, const char* data, size_t size) {
#ifdef _WIN32
    int send_flags = 0;
#else
    int send_flags = MSG_NOSIGNAL;
#endif

    size_t sent = 0;
    while (sent < size) {
        ssize_t n = ::send(fd, data + sent, static_cast<int>(size - sent), send_flags);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

uint16_t to_be16(uint16_t value) {
    return htons(value);
}

uint32_t to_be32(uint32_t value) {
    return htonl(value);
}

} // namespace

RecordFrameServer* RecordFrameServer::instance() {
    static RecordFrameServer s_instance;
    return &s_instance;
}

RecordFrameServer::RecordFrameServer() = default;

RecordFrameServer::~RecordFrameServer() { stop(); }

bool RecordFrameServer::start(int port) {
    if (m_running) {
        return true;
    }

    if (port <= 0 || port > 65535) {
        NSDK_LOG_ERROR("RecordFrameServer: invalid port %d", port);
        return false;
    }

    m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listen_fd == INVALID_SOCKET_FD) {
        NSDK_LOG_ERROR("RecordFrameServer: socket failed, errno=%d", socket_errno());
        return false;
    }

    int opt = 1;
    setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(m_listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        NSDK_LOG_ERROR("RecordFrameServer: bind port %d failed, errno=%d", port, socket_errno());
        socket_close(m_listen_fd);
        m_listen_fd = INVALID_SOCKET_FD;
        return false;
    }

    if (listen(m_listen_fd, 1) != 0) {
        NSDK_LOG_ERROR("RecordFrameServer: listen failed, errno=%d", socket_errno());
        socket_close(m_listen_fd);
        m_listen_fd = INVALID_SOCKET_FD;
        return false;
    }

    m_port = port;
    m_running = true;
    m_accept_thread = std::thread(&RecordFrameServer::accept_loop, this);

    NSDK_LOG_INFO("RecordFrameServer: listening on port %d", port);
    return true;
}

void RecordFrameServer::stop() {
    m_running = false;

    if (m_listen_fd != INVALID_SOCKET_FD) {
        shutdown(m_listen_fd, SHUT_RDWR);
        socket_close(m_listen_fd);
        m_listen_fd = INVALID_SOCKET_FD;
    }

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        if (m_client_fd != INVALID_SOCKET_FD) {
            shutdown(m_client_fd, SHUT_RDWR);
            socket_close(m_client_fd);
            m_client_fd = INVALID_SOCKET_FD;
        }
    }

    if (m_accept_thread.joinable()) {
        m_accept_thread.join();
    }
    if (m_client_thread.joinable()) {
        m_client_thread.join();
    }

    NSDK_LOG_INFO("RecordFrameServer: stopped");
}

void RecordFrameServer::set_start_callback(RecordFrameStartCallback cb) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_start_cb = std::move(cb);
}

void RecordFrameServer::set_read_callback(RecordFrameReadCallback cb) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_read_cb = std::move(cb);
}

void RecordFrameServer::set_stop_callback(RecordFrameStopCallback cb) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_stop_cb = std::move(cb);
}

NET_TV_COMMON_ECODE_E RecordFrameServer::open_stream(const NET_TV_RECORD_FRAME_STREAM_COND_S& cond,
                                                     NET_TV_RECORD_FRAME_STREAM_INFO_S& info) {
    RecordFrameStartCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        cb = m_start_cb;
    }
    if (!cb) {
        return NET_TV_E_NOT_SUPPORT;
    }

    std::memset(&info, 0, sizeof(info));
    info.dwSize = sizeof(info);
    NET_TV_COMMON_ECODE_E code = cb(cond, info);
    if (code != NET_TV_E_SUCCEED) {
        return code;
    }

    if (info.dwTcpPort == 0) {
        info.dwTcpPort = static_cast<UINT32>(m_port);
    }
    if (info.dwMediaType == 0) {
        info.dwMediaType = NET_TV_RECORD_FRAME_MEDIA_VIDEO;
    }
    if (info.szStreamId[0] == '\0') {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
#ifdef _WIN32
        _snprintf(info.szStreamId, sizeof(info.szStreamId) - 1, "rf-%llu", static_cast<unsigned long long>(ticks));
#else
        std::snprintf(info.szStreamId, sizeof(info.szStreamId), "rf-%llu", static_cast<unsigned long long>(ticks));
#endif
    }

    {
        std::lock_guard<std::mutex> lock(m_stream_mutex);
        m_stream_id = info.szStreamId;
        ++m_ssrc;
        if (m_ssrc == 0) {
            m_ssrc = 1;
        }
    }

    if (!m_running && !start(static_cast<int>(info.dwTcpPort))) {
        return NET_TV_E_SYSCALL_FALIED;
    }

    return NET_TV_E_SUCCEED;
}

NET_TV_COMMON_ECODE_E RecordFrameServer::close_stream(const std::string& stream_id) {
    if (stream_id.empty()) {
        return NET_TV_E_INVALID_PARAM;
    }

    RecordFrameStopCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        cb = m_stop_cb;
    }

    NET_TV_COMMON_ECODE_E code = NET_TV_E_SUCCEED;
    if (cb) {
        code = cb(stream_id);
    }

    {
        std::lock_guard<std::mutex> lock(m_stream_mutex);
        if (m_stream_id == stream_id) {
            m_stream_id.clear();
        }
    }

    return code;
}

std::string RecordFrameServer::current_stream_id() const {
    std::lock_guard<std::mutex> lock(m_stream_mutex);
    return m_stream_id;
}

void RecordFrameServer::accept_loop() {
    while (m_running) {
        sockaddr_in cli_addr{};
        socklen_t cli_len = sizeof(cli_addr);
        socket_fd_t fd = accept(m_listen_fd, reinterpret_cast<sockaddr*>(&cli_addr), &cli_len);
        if (fd == INVALID_SOCKET_FD) {
            if (m_running) {
                NSDK_LOG_WARN("RecordFrameServer: accept failed, errno=%d", socket_errno());
            }
            continue;
        }

        int tcp_no_delay = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&tcp_no_delay), sizeof(tcp_no_delay)) != 0) {
            NSDK_LOG_WARN("RecordFrameServer: set TCP_NODELAY failed, errno=%d", socket_errno());
        }

        {
            std::lock_guard<std::mutex> lock(m_client_mutex);
            if (m_client_fd != INVALID_SOCKET_FD) {
                shutdown(m_client_fd, SHUT_RDWR);
                socket_close(m_client_fd);
            }
            m_client_fd = fd;
        }

        if (m_client_thread.joinable()) {
            m_client_thread.join();
        }
        m_client_thread = std::thread(&RecordFrameServer::client_send_loop, this, fd);
    }
}

void RecordFrameServer::client_send_loop(socket_fd_t client_fd) {
    std::vector<char> buffer(NET_TV_RECORD_FRAME_MAX_PAYLOAD_SIZE);

    while (m_running) {
        const std::string stream_id = current_stream_id();
        if (stream_id.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        RecordFrameReadCallback cb;
        {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            cb = m_read_cb;
        }
        if (!cb) {
            NSDK_LOG_WARN("RecordFrameServer: read callback is not registered");
            break;
        }

        NET_TV_RECORD_FRAME_INFO_S frame_info{};
        frame_info.dwSize = sizeof(frame_info);
        const int read_len = cb(stream_id, frame_info, buffer.data(), buffer.size());
        if (read_len < 0) {
            NSDK_LOG_WARN("RecordFrameServer: read callback returned %d", read_len);
            break;
        }
        if (read_len == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        frame_info.dwPayloadLen = static_cast<UINT32>(read_len);
        if (!send_packet(client_fd, frame_info, buffer.data())) {
            NSDK_LOG_WARN("RecordFrameServer: send packet failed, errno=%d", socket_errno());
            break;
        }

        if ((frame_info.dwFlags & NET_TV_RECORD_FRAME_FLAG_STREAM_END) != 0 ||
            frame_info.dwMediaType == NET_TV_RECORD_FRAME_MEDIA_END) {
            close_stream(stream_id);
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        if (m_client_fd == client_fd) {
            shutdown(m_client_fd, SHUT_RDWR);
            socket_close(m_client_fd);
            m_client_fd = INVALID_SOCKET_FD;
        }
    }
}

bool RecordFrameServer::send_packet(socket_fd_t fd,
                                    const NET_TV_RECORD_FRAME_INFO_S& frame_info,
                                    const char* payload) {
    if (fd == INVALID_SOCKET_FD || frame_info.dwPayloadLen > NET_TV_RECORD_FRAME_MAX_PAYLOAD_SIZE) {
        return false;
    }

    uint32_t ssrc = 0;
    {
        std::lock_guard<std::mutex> lock(m_stream_mutex);
        ssrc = m_ssrc;
    }

    NET_TV_RECORD_FRAME_RTP_HEADER_S header{};
    header.byVersion = 2;
    header.byPayloadType = static_cast<UCHAR>(
        frame_info.dwMediaType == NET_TV_RECORD_FRAME_MEDIA_AUDIO ?
            NET_TV_RECORD_FRAME_PAYLOAD_TYPE_AUDIO :
        frame_info.dwMediaType == NET_TV_RECORD_FRAME_MEDIA_END ?
            NET_TV_RECORD_FRAME_PAYLOAD_TYPE_END :
            NET_TV_RECORD_FRAME_PAYLOAD_TYPE_VIDEO);
    header.wSeq = to_be16(static_cast<uint16_t>(frame_info.dwSeq));
    header.dwTimestamp = to_be32(frame_info.dwTimestamp);
    header.dwSsrc = to_be32(ssrc);
    header.dwPayloadLen = to_be32(frame_info.dwPayloadLen);
    header.dwFlags = to_be32(frame_info.dwFlags);

    if (!send_all(fd, reinterpret_cast<const char*>(&header), sizeof(header))) {
        return false;
    }
    if (frame_info.dwPayloadLen == 0) {
        return true;
    }
    return payload && send_all(fd, payload, frame_info.dwPayloadLen);
}

}  // namespace tvsdk
