/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : RecordFrameServer.cpp
 * @Description  : 录像帧服务端实现，负责监听TCP端口，接收客户端连接，发送录像帧数据（视频/音频）
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
 * @brief 精确发送指定字节数的数据
 * @param fd socket文件描述符
 * @param data 发送数据缓冲区
 * @param size 需要发送的字节数
 * @return true表示成功发送指定字节数，false表示发送失败或连接断开
 */
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

/**
 * @brief 将主机字节序16位整数转换为大端序
 * @param value 主机字节序16位整数
 * @return 大端序16位整数
 */
uint16_t to_be16(uint16_t value) {
    return htons(value);
}

/**
 * @brief 将主机字节序32位整数转换为大端序
 * @param value 主机字节序32位整数
 * @return 大端序32位整数
 */
uint32_t to_be32(uint32_t value) {
    return htonl(value);
}

} // namespace

/**
 * @brief 获取单例实例
 * @return 单例指针
 */
RecordFrameServer* RecordFrameServer::instance() {
    static RecordFrameServer s_instance;
    return &s_instance;
}

/**
 * @brief 构造函数（私有，单例模式）
 */
RecordFrameServer::RecordFrameServer() = default;

/**
 * @brief 析构函数（私有，单例模式）
 * @details 自动调用stop()停止服务并释放资源
 */
RecordFrameServer::~RecordFrameServer() { stop(); }

/**
 * @brief 启动服务端
 * @details 创建监听socket，设置SO_REUSEADDR，绑定端口，开始监听，启动接收线程
 * @param port 监听端口，默认9007
 * @return true表示成功，false表示失败
 */
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

/**
 * @brief 停止服务端
 * @details 设置停止标志，关闭监听socket和客户端socket，等待接收线程和发送线程结束
 */
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

/**
 * @brief 设置流启动回调函数
 * @param cb 流启动回调函数
 */
void RecordFrameServer::set_start_callback(RecordFrameStartCallback cb) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_start_cb = std::move(cb);
}

/**
 * @brief 设置帧读取回调函数
 * @param cb 帧读取回调函数
 */
void RecordFrameServer::set_read_callback(RecordFrameReadCallback cb) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_read_cb = std::move(cb);
}

/**
 * @brief 设置流停止回调函数
 * @param cb 流停止回调函数
 */
void RecordFrameServer::set_stop_callback(RecordFrameStopCallback cb) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_stop_cb = std::move(cb);
}

/**
 * @brief 开启录像帧流
 * @details 调用流启动回调函数，生成流ID，启动服务端（如果未运行），返回流信息
 * @param cond 流启动条件（通道、时间范围、媒体类型等）
 * @param info 流信息（输出参数，包含流ID、端口、媒体类型等）
 * @return 错误码，NET_TV_E_SUCCEED表示成功，其他值表示失败
 */
NET_TV_COMMON_ECODE_E RecordFrameServer::open_stream(const NET_RecordFrameStreamCond_S& cond,
                                                     NET_RecordFrameStreamInfo_S& info) {
    RecordFrameStartCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        cb = m_start_cb;
    }
    if (!cb) {
        return NET_TV_E_NOT_SUPPORT;
    }

    std::memset(&info, 0, sizeof(info));
    info.uSize = sizeof(info);
    NET_TV_COMMON_ECODE_E code = cb(cond, info);
    if (code != NET_TV_E_SUCCEED) {
        return code;
    }

    if (info.uTcpPort == 0) {
        info.uTcpPort = static_cast<UINT32>(m_port);
    }
    if (info.uMediaType == 0) {
        info.uMediaType = NET_TV_RECORD_FRAME_MEDIA_VIDEO;
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

    if (!m_running && !start(static_cast<int>(info.uTcpPort))) {
        return NET_TV_E_SYSCALL_FALIED;
    }

    return NET_TV_E_SUCCEED;
}

/**
 * @brief 关闭录像帧流
 * @details 调用流停止回调函数，清除当前流ID
 * @param stream_id 流ID，标识要关闭的播放会话
 * @return 错误码，NET_TV_E_SUCCEED表示成功，其他值表示失败
 */
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

/**
 * @brief 获取当前流ID
 * @return 当前流ID字符串
 */
std::string RecordFrameServer::current_stream_id() const {
    std::lock_guard<std::mutex> lock(m_stream_mutex);
    return m_stream_id;
}

/**
 * @brief 客户端连接接收循环
 * @details 持续监听端口，接收客户端连接，关闭旧连接（如果存在），开启新的发送线程
 */
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

/**
 * @brief 客户端数据发送循环
 * @details 持续从回调函数获取录像帧数据，封装为RTP-like协议格式发送给客户端，
 *          遇到流结束标志或发送失败时退出循环，关闭客户端连接
 * @param client_fd 客户端socket文件描述符
 */
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

        NET_RecordFrameInfo_S frame_info{};
        frame_info.uSize = sizeof(frame_info);
        const int read_len = cb(stream_id, frame_info, buffer.data(), buffer.size());
        if (read_len < 0) {
            NSDK_LOG_WARN("RecordFrameServer: read callback returned %d", read_len);
            break;
        }
        if (read_len == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        frame_info.uPayloadLen = static_cast<UINT32>(read_len);
        if (!send_packet(client_fd, frame_info, buffer.data())) {
            NSDK_LOG_WARN("RecordFrameServer: send packet failed, errno=%d", socket_errno());
            break;
        }

        if ((frame_info.uFlags & NET_TV_RECORD_FRAME_FLAG_STREAM_END) != 0 ||
            frame_info.uMediaType == NET_TV_RECORD_FRAME_MEDIA_END) {
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

/**
 * @brief 发送帧数据包
 * @details 将帧信息和帧数据封装为RTP-like协议格式（版本号、负载类型、序号、时间戳、SSRC、数据长度、标志位等），
 *          先发送帧头，再发送帧数据
 * @param fd socket文件描述符
 * @param frame_info 帧信息（序号、时间戳、媒体类型、数据长度等）
 * @param payload 帧数据指针
 * @return true表示成功，false表示失败
 */
bool RecordFrameServer::send_packet(socket_fd_t fd,
                                    const NET_RecordFrameInfo_S& frame_info,
                                    const char* payload) {
    if (fd == INVALID_SOCKET_FD || frame_info.uPayloadLen > NET_TV_RECORD_FRAME_MAX_PAYLOAD_SIZE) {
        return false;
    }

    uint32_t ssrc = 0;
    {
        std::lock_guard<std::mutex> lock(m_stream_mutex);
        ssrc = m_ssrc;
    }

    NET_RecordFrameRtpHeader_S header{};
    header.byVersion = 2;
    header.byPayloadType = static_cast<UCHAR>(
        frame_info.uMediaType == NET_TV_RECORD_FRAME_MEDIA_AUDIO ?
            NET_TV_RECORD_FRAME_PAYLOAD_TYPE_AUDIO :
        frame_info.uMediaType == NET_TV_RECORD_FRAME_MEDIA_END ?
            NET_TV_RECORD_FRAME_PAYLOAD_TYPE_END :
            NET_TV_RECORD_FRAME_PAYLOAD_TYPE_VIDEO);
    header.wSeq = to_be16(static_cast<uint16_t>(frame_info.uSeq));
    header.dwTimestamp = to_be32(frame_info.uTimestamp);
    header.dwSsrc = to_be32(ssrc);
    header.dwPayloadLen = to_be32(frame_info.uPayloadLen);
    header.dwFlags = to_be32(frame_info.uFlags);

    if (!send_all(fd, reinterpret_cast<const char*>(&header), sizeof(header))) {
        return false;
    }
    if (frame_info.uPayloadLen == 0) {
        return true;
    }
    return payload && send_all(fd, payload, frame_info.uPayloadLen);
}

}  // namespace tvsdk
