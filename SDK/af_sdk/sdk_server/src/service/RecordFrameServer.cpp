/**
 * @file RecordFrameServer.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CRecordFrameServer 模块实现
 * 功能说明：
 * 1. 实现 CRecordFrameServer 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "RecordFrameServer.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <utility>

#include "NetSdkLog.h"
#include "PlatformCompat.h"

namespace tvsdk {
namespace {
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 send_all 对应的处理。
 * @param [in] fd 函数处理参数。
 * @param [in] data 函数处理参数。
 * @param [in] size 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool send_all(socket_fd_t fd, const char* data, size_t size) {
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 to_be16 定义的内部处理。
 * @param [in] value 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static uint16_t to_be16(uint16_t value) {
    return htons(value);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 to_be32 定义的内部处理。
 * @param [in] value 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static uint32_t to_be32(uint32_t value) {
    return htonl(value);
}

} /* namespace */

CRecordFrameServer* CRecordFrameServer::instance() {
    static CRecordFrameServer s_instance;
    return &s_instance;
}

CRecordFrameServer::CRecordFrameServer() = default;

CRecordFrameServer::~CRecordFrameServer() { stop(); }
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 start 对应的处理。
 * @param [in] port 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CRecordFrameServer::start(int port) {
    if (m_bRunning) {
        return true;
    }

    if (port <= 0 || port > 65535) {
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameServer: invalid port %d", port);
        return false;
    }

    m_nListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nListenFd == INVALID_SOCKET_FD) {
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameServer: socket failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        return false;
    }

    int opt = 1;
    setsockopt(m_nListenFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(m_nListenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameServer: bind port %d failed, errno=%d", port, NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nListenFd);
        m_nListenFd = INVALID_SOCKET_FD;
        return false;
    }

    if (listen(m_nListenFd, 1) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameServer: listen failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        NETSDK_SOCKET_CLOSE(m_nListenFd);
        m_nListenFd = INVALID_SOCKET_FD;
        return false;
    }

    m_nPort = port;
    m_bRunning = true;
    m_stAcceptThread = std::thread(&CRecordFrameServer::accept_loop, this);

    NETSDK_LOG_MESSAGE_INFO("CRecordFrameServer: listening on port %d", port);
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 stop 对应的处理。
 * @return 无返回值。
 */

void CRecordFrameServer::stop() {
    m_bRunning = false;

    if (m_nListenFd != INVALID_SOCKET_FD) {
        shutdown(m_nListenFd, SHUT_RDWR);
        NETSDK_SOCKET_CLOSE(m_nListenFd);
        m_nListenFd = INVALID_SOCKET_FD;
    }

    {
        std::lock_guard<std::mutex> lock(m_stClientMutex);
        if (m_nClientFd != INVALID_SOCKET_FD) {
            shutdown(m_nClientFd, SHUT_RDWR);
            NETSDK_SOCKET_CLOSE(m_nClientFd);
            m_nClientFd = INVALID_SOCKET_FD;
        }
    }

    if (m_stAcceptThread.joinable()) {
        m_stAcceptThread.join();
    }
    if (m_stClientThread.joinable()) {
        m_stClientThread.join();
    }

    NETSDK_LOG_MESSAGE_INFO("CRecordFrameServer: stopped");
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 set_start_callback 对应的处理。
 * @param [in] cb 函数处理参数。
 * @return 无返回值。
 */

void CRecordFrameServer::set_start_callback(RecordFrameStartCallback cb) {
    std::lock_guard<std::mutex> lock(m_stCallbackMutex);
    m_fnStartCallback = std::move(cb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 set_read_callback 对应的处理。
 * @param [in] cb 函数处理参数。
 * @return 无返回值。
 */

void CRecordFrameServer::set_read_callback(RecordFrameReadCallback cb) {
    std::lock_guard<std::mutex> lock(m_stCallbackMutex);
    m_fnReadCallback = std::move(cb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 set_stop_callback 对应的处理。
 * @param [in] cb 函数处理参数。
 * @return 无返回值。
 */

void CRecordFrameServer::set_stop_callback(RecordFrameStopCallback cb) {
    std::lock_guard<std::mutex> lock(m_stCallbackMutex);
    m_fnStopCallback = std::move(cb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 open_stream 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

NET_TV_COMMON_ECODE_E CRecordFrameServer::open_stream(const NET_TV_RECORD_FRAME_STREAM_COND_S& cond,
                                                     NET_TV_RECORD_FRAME_STREAM_INFO_S& info) {
    RecordFrameStartCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_stCallbackMutex);
        cb = m_fnStartCallback;
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
        info.dwTcpPort = static_cast<UINT32>(m_nPort);
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
        std::lock_guard<std::mutex> lock(m_stStreamMutex);
        m_strStreamId = info.szStreamId;
        ++m_uSsrc;
        if (m_uSsrc == 0) {
            m_uSsrc = 1;
        }
    }

    if (!m_bRunning && !start(static_cast<int>(info.dwTcpPort))) {
        return NET_TV_E_SYSCALL_FALIED;
    }

    return NET_TV_E_SUCCEED;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 close_stream 定义的内部处理。
 * @param [in] stream_id 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_TV_COMMON_ECODE_E CRecordFrameServer::close_stream(const std::string& stream_id) {
    if (stream_id.empty()) {
        return NET_TV_E_INVALID_PARAM;
    }

    RecordFrameStopCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_stCallbackMutex);
        cb = m_fnStopCallback;
    }

    NET_TV_COMMON_ECODE_E code = NET_TV_E_SUCCEED;
    if (cb) {
        code = cb(stream_id);
    }

    {
        std::lock_guard<std::mutex> lock(m_stStreamMutex);
        if (m_strStreamId == stream_id) {
            m_strStreamId.clear();
        }
    }

    return code;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 current_stream_id 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

std::string CRecordFrameServer::current_stream_id() const {
    std::lock_guard<std::mutex> lock(m_stStreamMutex);
    return m_strStreamId;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 accept_loop 定义的内部处理。
 * @return 无返回值。
 */

void CRecordFrameServer::accept_loop() {
    while (m_bRunning) {
        sockaddr_in cli_addr{};
        socklen_t cli_len = sizeof(cli_addr);
        socket_fd_t fd = accept(m_nListenFd, reinterpret_cast<sockaddr*>(&cli_addr), &cli_len);
        if (fd == INVALID_SOCKET_FD) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("CRecordFrameServer: accept failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            continue;
        }

        int tcp_no_delay = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&tcp_no_delay), sizeof(tcp_no_delay)) != 0) {
            NETSDK_LOG_MESSAGE_WARN("CRecordFrameServer: set TCP_NODELAY failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        }

        {
            std::lock_guard<std::mutex> lock(m_stClientMutex);
            if (m_nClientFd != INVALID_SOCKET_FD) {
                shutdown(m_nClientFd, SHUT_RDWR);
                NETSDK_SOCKET_CLOSE(m_nClientFd);
            }
            m_nClientFd = fd;
        }

        if (m_stClientThread.joinable()) {
            m_stClientThread.join();
        }
        m_stClientThread = std::thread(&CRecordFrameServer::client_send_loop, this, fd);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 client_send_loop 定义的内部处理。
 * @param [in] client_fd 函数处理参数。
 * @return 无返回值。
 */

void CRecordFrameServer::client_send_loop(socket_fd_t client_fd) {
    std::vector<char> buffer(NET_TV_RECORD_FRAME_MAX_PAYLOAD_SIZE);

    while (m_bRunning) {
        const std::string stream_id = current_stream_id();
        if (stream_id.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        RecordFrameReadCallback cb;
        {
            std::lock_guard<std::mutex> lock(m_stCallbackMutex);
            cb = m_fnReadCallback;
        }
        if (!cb) {
            NETSDK_LOG_MESSAGE_WARN("CRecordFrameServer: read callback is not registered");
            break;
        }

        NET_TV_RECORD_FRAME_INFO_S frame_info{};
        frame_info.dwSize = sizeof(frame_info);
        const int read_len = cb(stream_id, frame_info, buffer.data(), buffer.size());
        if (read_len < 0) {
            NETSDK_LOG_MESSAGE_WARN("CRecordFrameServer: read callback returned %d", read_len);
            break;
        }
        if (read_len == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        frame_info.dwPayloadLen = static_cast<UINT32>(read_len);
        if (!send_packet(client_fd, frame_info, buffer.data())) {
            NETSDK_LOG_MESSAGE_WARN("CRecordFrameServer: send packet failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            break;
        }

        if ((frame_info.dwFlags & NET_TV_RECORD_FRAME_FLAG_STREAM_END) != 0 ||
            frame_info.dwMediaType == NET_TV_RECORD_FRAME_MEDIA_END) {
            close_stream(stream_id);
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_stClientMutex);
        if (m_nClientFd == client_fd) {
            shutdown(m_nClientFd, SHUT_RDWR);
            NETSDK_SOCKET_CLOSE(m_nClientFd);
            m_nClientFd = INVALID_SOCKET_FD;
        }
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 send_packet 对应的处理。
 * @return 返回该处理的状态或结果。
 */

bool CRecordFrameServer::send_packet(socket_fd_t fd,
                                    const NET_TV_RECORD_FRAME_INFO_S& frame_info,
                                    const char* payload) {
    if (fd == INVALID_SOCKET_FD || frame_info.dwPayloadLen > NET_TV_RECORD_FRAME_MAX_PAYLOAD_SIZE) {
        return false;
    }

    uint32_t ssrc = 0;
    {
        std::lock_guard<std::mutex> lock(m_stStreamMutex);
        ssrc = m_uSsrc;
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

}  /* namespace tvsdk */
