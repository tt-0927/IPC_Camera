/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : CRecordFrameServer.cpp
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
 * @param nFd socket文件描述符
 * @param pstrData 发送数据缓冲区
 * @param uSize 需要发送的字节数
 * @return true表示成功发送指定字节数，false表示发送失败或连接断开
 */
bool send_all(socket_fd_t nFd, const char* pstrData, size_t uSize) {
#ifdef _WIN32
    int nSendFlags = 0;
#else
    int nSendFlags = MSG_NOSIGNAL;
#endif

    size_t uSent = 0;
    while (uSent < uSize) {
        ssize_t lRet = ::send(nFd, pstrData + uSent, static_cast<int>(uSize - uSent), nSendFlags);
        if (lRet <= 0) {
            return false;
        }
        uSent += static_cast<size_t>(lRet);
    }
    return true;
}

/**
 * @brief 将主机字节序16位整数转换为大端序
 * @param uValue 主机字节序16位整数
 * @return 大端序16位整数
 */
uint16_t to_be16(uint16_t uValue) {
    return htons(uValue);
}

/**
 * @brief 将主机字节序32位整数转换为大端序
 * @param uValue 主机字节序32位整数
 * @return 大端序32位整数
 */
uint32_t to_be32(uint32_t uValue) {
    return htonl(uValue);
}

} // namespace

/**
 * @brief 获取单例实例
 * @return 单例指针
 */
CRecordFrameServer* CRecordFrameServer::instance() {
    static CRecordFrameServer s_instance;
    return &s_instance;
}

/**
 * @brief 构造函数（私有，单例模式）
 */
CRecordFrameServer::CRecordFrameServer() = default;

/**
 * @brief 析构函数（私有，单例模式）
 * @details 自动调用stop()停止服务并释放资源
 */
CRecordFrameServer::~CRecordFrameServer() { stop(); }

/**
 * @brief 启动服务端
 * @details 创建监听socket，设置SO_REUSEADDR，绑定端口，开始监听，启动接收线程
 * @param nPort 监听端口，默认9007
 * @return true表示成功，false表示失败
 */
bool CRecordFrameServer::start(int nPort) {
    if (m_bRunning) {
        return true;
    }

    if (nPort <= 0 || nPort > 65535) {
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameServer: invalid port %d", nPort);
        return false;
    }

    m_nListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nListenFd == INVALID_SOCKET_FD) {
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameServer: socket failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        return false;
    }

    int nOpt = 1;
    setsockopt(m_nListenFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&nOpt), sizeof(nOpt));

    sockaddr_in stAddr{};
    stAddr.sin_family = AF_INET;
    stAddr.sin_addr.s_addr = INADDR_ANY;
    stAddr.sin_port = htons(static_cast<uint16_t>(nPort));

    if (bind(m_nListenFd, reinterpret_cast<sockaddr*>(&stAddr), sizeof(stAddr)) != 0) {
        NETSDK_LOG_MESSAGE_ERROR("CRecordFrameServer: bind port %d failed, errno=%d", nPort, NETSDK_SOCKET_GET_ERROR());
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

    m_nPort = nPort;
    m_bRunning = true;
    m_stAcceptThread = std::thread(&CRecordFrameServer::accept_loop, this);

    NETSDK_LOG_MESSAGE_INFO("CRecordFrameServer: listening on port %d", nPort);
    return true;
}

/**
 * @brief 停止服务端
 * @details 设置停止标志，关闭监听socket和客户端socket，等待接收线程和发送线程结束
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
 * @brief 设置流启动回调函数
 * @param cb 流启动回调函数
 */
void CRecordFrameServer::set_start_callback(RecordFrameStartCallback cb) {
    std::lock_guard<std::mutex> lock(m_stCallbackMutex);
    m_fnStartCallback = std::move(cb);
}

/**
 * @brief 设置帧读取回调函数
 * @param cb 帧读取回调函数
 */
void CRecordFrameServer::set_read_callback(RecordFrameReadCallback cb) {
    std::lock_guard<std::mutex> lock(m_stCallbackMutex);
    m_fnReadCallback = std::move(cb);
}

/**
 * @brief 设置流停止回调函数
 * @param cb 流停止回调函数
 */
void CRecordFrameServer::set_stop_callback(RecordFrameStopCallback cb) {
    std::lock_guard<std::mutex> lock(m_stCallbackMutex);
    m_fnStopCallback = std::move(cb);
}

/**
 * @brief 开启录像帧流
 * @details 调用流启动回调函数，生成流ID，启动服务端（如果未运行），返回流信息
 * @param stCond 流启动条件（通道、时间范围、媒体类型等）
 * @param stInfo 流信息（输出参数，包含流ID、端口、媒体类型等）
 * @return 错误码，NET_E_SUCCEED表示成功，其他值表示失败
 */
NET_COMMON_ECODE_E CRecordFrameServer::open_stream(const NET_RecordFrameStreamCond_S& stCond,
                                                     NET_RecordFrameStreamInfo_S& stInfo) {
    RecordFrameStartCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_stCallbackMutex);
        cb = m_fnStartCallback;
    }
    if (!cb) {
        return NET_E_NOT_SUPPORT;
    }

    std::memset(&stInfo, 0, sizeof(stInfo));
    stInfo.uSize = sizeof(stInfo);
    NET_COMMON_ECODE_E enCode = cb(stCond, stInfo);
    if (enCode != NET_E_SUCCEED) {
        return enCode;
    }

    if (stInfo.uTcpPort == 0) {
        stInfo.uTcpPort = static_cast<UINT32>(m_nPort);
    }
    if (stInfo.uMediaType == 0) {
        stInfo.uMediaType = NET_RECORD_FRAME_MEDIA_VIDEO;
    }
    if (stInfo.strStreamId[0] == '\0') {
        const auto lTicks = std::chrono::steady_clock::now().time_since_epoch().count();
#ifdef _WIN32
        _snprintf(stInfo.strStreamId, sizeof(stInfo.strStreamId) - 1, "rf-%llu", static_cast<unsigned long long>(lTicks));
#else
        std::snprintf(stInfo.strStreamId, sizeof(stInfo.strStreamId), "rf-%llu", static_cast<unsigned long long>(lTicks));
#endif
    }

    {
        std::lock_guard<std::mutex> lock(m_stStreamMutex);
        m_strStreamId = stInfo.strStreamId;
        ++m_uSsrc;
        if (m_uSsrc == 0) {
            m_uSsrc = 1;
        }
    }

    if (!m_bRunning && !start(static_cast<int>(stInfo.uTcpPort))) {
        return NET_E_SYSCALL_FALIED;
    }

    return NET_E_SUCCEED;
}

/**
 * @brief 关闭录像帧流
 * @details 调用流停止回调函数，清除当前流ID
 * @param strStreamId 流ID，标识要关闭的播放会话
 * @return 错误码，NET_E_SUCCEED表示成功，其他值表示失败
 */
NET_COMMON_ECODE_E CRecordFrameServer::close_stream(const std::string& strStreamId) {
    if (strStreamId.empty()) {
        return NET_E_INVALID_PARAM;
    }

    RecordFrameStopCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_stCallbackMutex);
        cb = m_fnStopCallback;
    }

    NET_COMMON_ECODE_E enCode = NET_E_SUCCEED;
    if (cb) {
        enCode = cb(strStreamId);
    }

    {
        std::lock_guard<std::mutex> lock(m_stStreamMutex);
        if (m_strStreamId == strStreamId) {
            m_strStreamId.clear();
        }
    }

    return enCode;
}

/**
 * @brief 获取当前流ID
 * @return 当前流ID字符串
 */
std::string CRecordFrameServer::current_stream_id() const {
    std::lock_guard<std::mutex> lock(m_stStreamMutex);
    return m_strStreamId;
}

/**
 * @brief 客户端连接接收循环
 * @details 持续监听端口，接收客户端连接，关闭旧连接（如果存在），开启新的发送线程
 */
void CRecordFrameServer::accept_loop() {
    while (m_bRunning) {
        sockaddr_in stCliAddr{};
        socklen_t uCliLen = sizeof(stCliAddr);
        socket_fd_t nFd = accept(m_nListenFd, reinterpret_cast<sockaddr*>(&stCliAddr), &uCliLen);
        if (nFd == INVALID_SOCKET_FD) {
            if (m_bRunning) {
                NETSDK_LOG_MESSAGE_WARN("CRecordFrameServer: accept failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            }
            continue;
        }

        int nTcpNoDelay = 1;
        if (setsockopt(nFd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nTcpNoDelay), sizeof(nTcpNoDelay)) != 0) {
            NETSDK_LOG_MESSAGE_WARN("CRecordFrameServer: set TCP_NODELAY failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
        }

        {
            std::lock_guard<std::mutex> lock(m_stClientMutex);
            if (m_nClientFd != INVALID_SOCKET_FD) {
                shutdown(m_nClientFd, SHUT_RDWR);
                NETSDK_SOCKET_CLOSE(m_nClientFd);
            }
            m_nClientFd = nFd;
        }

        if (m_stClientThread.joinable()) {
            m_stClientThread.join();
        }
        m_stClientThread = std::thread(&CRecordFrameServer::client_send_loop, this, nFd);
    }
}

/**
 * @brief 客户端数据发送循环
 * @details 持续从回调函数获取录像帧数据，封装为RTP-like协议格式发送给客户端，
 *          遇到流结束标志或发送失败时退出循环，关闭客户端连接
 * @param nClientFd 客户端socket文件描述符
 */
void CRecordFrameServer::client_send_loop(socket_fd_t nClientFd) {
    std::vector<char> vecBuffer(NET_RECORD_FRAME_MAX_PAYLOAD_SIZE);

    while (m_bRunning) {
        const std::string strStreamId = current_stream_id();
        if (strStreamId.empty()) {
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

        NET_RecordFrameInfo_S stFrameInfo{};
        stFrameInfo.uSize = sizeof(stFrameInfo);
        const int nReadLen = cb(strStreamId, stFrameInfo, vecBuffer.data(), vecBuffer.size());
        if (nReadLen < 0) {
            NETSDK_LOG_MESSAGE_WARN("CRecordFrameServer: read callback returned %d", nReadLen);
            break;
        }
        if (nReadLen == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        stFrameInfo.uPayloadLen = static_cast<UINT32>(nReadLen);
        if (!send_packet(nClientFd, stFrameInfo, vecBuffer.data())) {
            NETSDK_LOG_MESSAGE_WARN("CRecordFrameServer: send packet failed, errno=%d", NETSDK_SOCKET_GET_ERROR());
            break;
        }

        if ((stFrameInfo.uFlags & NET_RECORD_FRAME_FLAG_STREAM_END) != 0 ||
            stFrameInfo.uMediaType == NET_RECORD_FRAME_MEDIA_END) {
            close_stream(strStreamId);
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_stClientMutex);
        if (m_nClientFd == nClientFd) {
            shutdown(m_nClientFd, SHUT_RDWR);
            NETSDK_SOCKET_CLOSE(m_nClientFd);
            m_nClientFd = INVALID_SOCKET_FD;
        }
    }
}

/**
 * @brief 发送帧数据包
 * @details 将帧信息和帧数据封装为RTP-like协议格式（版本号、负载类型、序号、时间戳、SSRC、数据长度、标志位等），
 *          先发送帧头，再发送帧数据
 * @param nFd socket文件描述符
 * @param stFrameInfo 帧信息（序号、时间戳、媒体类型、数据长度等）
 * @param pstrPayload 帧数据指针
 * @return true表示成功，false表示失败
 */
bool CRecordFrameServer::send_packet(socket_fd_t nFd,
                                    const NET_RecordFrameInfo_S& stFrameInfo,
                                    const char* pstrPayload) {
    if (nFd == INVALID_SOCKET_FD || stFrameInfo.uPayloadLen > NET_RECORD_FRAME_MAX_PAYLOAD_SIZE) {
        return false;
    }

    uint32_t uSsrc = 0;
    {
        std::lock_guard<std::mutex> lock(m_stStreamMutex);
        uSsrc = m_uSsrc;
    }

    NET_RecordFrameRtpHeader_S stHeader{};
    stHeader.byVersion = 2;
    stHeader.byPayloadType = static_cast<UCHAR>(
        stFrameInfo.uMediaType == NET_RECORD_FRAME_MEDIA_AUDIO ?
            NET_RECORD_FRAME_PAYLOAD_TYPE_AUDIO :
        stFrameInfo.uMediaType == NET_RECORD_FRAME_MEDIA_END ?
            NET_RECORD_FRAME_PAYLOAD_TYPE_END :
            NET_RECORD_FRAME_PAYLOAD_TYPE_VIDEO);
    stHeader.wSeq = to_be16(static_cast<uint16_t>(stFrameInfo.uSeq));
    stHeader.uTimestamp = to_be32(stFrameInfo.uTimestamp);
    stHeader.uSsrc = to_be32(uSsrc);
    stHeader.uPayloadLen = to_be32(stFrameInfo.uPayloadLen);
    stHeader.uFlags = to_be32(stFrameInfo.uFlags);

    if (!send_all(nFd, reinterpret_cast<const char*>(&stHeader), sizeof(stHeader))) {
        return false;
    }
    if (stFrameInfo.uPayloadLen == 0) {
        return true;
    }
    return pstrPayload && send_all(nFd, pstrPayload, stFrameInfo.uPayloadLen);
}

}  // namespace tvsdk
