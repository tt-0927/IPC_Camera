
#include "UDSServer.h"

#include "AsioUDSServer.h"
#include "dlog.h"
#include <cstring>
#include <unistd.h>
#include <future>
#include "UDSAdapter.h"

namespace Net
{

UDSServer::UDSServer(Param_S &stParam)
    : m_stParam(stParam)
    , m_bExit(false)
{
    m_server = std::make_shared<AsioUDSServer>(stParam.stInitParam.nPort);
   

    // 设置回调函数，用于处理连接、断开和错误
    Callback callback;
    callback.set_connectObserver(std::bind(&UDSServer::deal_connect, this, std::placeholders::_1));
    callback.set_closeObserver(std::bind(&UDSServer::deal_disconnect, this, std::placeholders::_1));
    callback.set_errorObserver(std::bind(&UDSServer::deal_error, this, std::placeholders::_1));
        
    // 将回调函数设置到服务器并启动服务器
    m_server->set_callback(callback);
    m_server->start(); 

    // 初始化心跳相关设置
    m_heartbeat = std::make_shared<Heartbeat>(this, stParam.stInitParam.nHeartbeatInterval);
    std::string heartbeatMessage = "This is server heartbeat";
    m_heartbeat->set_messge(heartbeatMessage.c_str(), heartbeatMessage.length() + 1);
    m_heartbeat->set_observer(std::bind(&UDSServer::heartbeat_status, this, std::placeholders::_1));
    m_heartbeat->set_actionCode(stParam.stInitParam.nHearbeatCode);
    m_heartbeat->start();
    dlog_info("UDS服务端启动成功, port: %d", stParam.stInitParam.nPort);
}

UDSServer::~UDSServer()
{
    cleanup();
    dlog_info("UDS服务端释放成功, port: %d", m_stParam.stInitParam.nPort);
}

void UDSServer::cleanup()
{
    if (m_bExit.exchange(true))
    {
        /* 已经在清理中 */
        return;
    }

    dlog_info("开始清理UDS服务端, port: %d", m_stParam.stInitParam.nPort);

    /* 停止心跳 */
    if (m_heartbeat)
    {
        m_heartbeat->stop();
        m_heartbeat.reset();
    }

    /* 停止服务器 */
    if (m_server)
    {
        /* 创建一个future来异步等待stop完成 */
        auto future = std::async(std::launch::async,
                                 [&]()
                                 {
                                     m_server->stop();
                                 });

        /* 等待最多3秒 */
        if (future.wait_for(std::chrono::seconds(3)) == std::future_status::timeout)
        {
            dlog_error("服务器停止超时, port: %d", m_stParam.stInitParam.nPort);
        }

        m_server.reset();
    }

    /* 清理会话（如果还有的话） */
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.clear();
    }
}

int UDSServer::send(const Message_S stMessage)
{
    if (!m_server) 
    {
        return -1;
    }
    Net::MessageHead_S stHead;
    stHead.nActionCode = stMessage.nActionCode;
    stHead.nDataLength = stMessage.nDataLength;
    std::vector<void *> disconnectSession;
    /* 发送数据 */
    auto send_data = [this, &stHead, &stMessage, &disconnectSession](void *pHandle) {
        /* 发数据头 */
        int nRet = m_server->send(&stHead, sizeof(stHead), pHandle);
        if (nRet < 0)
        {
            disconnectSession.push_back(pHandle);
            dlog_error("发送数据头失败");
            return;
        }
        /* 发数据 */
        nRet = m_server->send(stMessage.pData, stMessage.nDataLength, pHandle);
        if (nRet < 0)
        {
            disconnectSession.push_back(pHandle);
            dlog_error("发送数据失败");
            return;
        }
    };

    if (stMessage.pHandle == nullptr)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        /* 发给所有客户端 */
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it)
        {
            send_data(*it);    
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        /* 发给指定客户端 */
        send_data(stMessage.pHandle);   
    }
    /* 清理掉已断开的客户端 */
    for (auto it = disconnectSession.begin(); it != disconnectSession.end(); ++it)
    {
        dlog_error("清理掉已断开的客户端");
        m_server->stop_session(*it);
    }
    return 0;
}

int UDSServer::receive(Message_S &stMessage)
{
    return 0;
}

void UDSServer::set_heartbeat(const void *pData, size_t nLength)
{
    if (!pData)
    {
        return;
    }
    m_heartbeat->set_messge(pData, nLength);
}

void UDSServer::heartbeat_status(bool bStatus)
{
}

void UDSServer::receive(UDSAdapter* pSession)
{
    while (true)
    {
        try
        {
            Net::MessageHead_S stHead;
            int nRecvLen = pSession->receive(&stHead, sizeof(Net::MessageHead_S));
            if (nRecvLen != sizeof(Net::MessageHead_S))
            {
                dlog_error("%d 数据头长度错误nRecvLen %d sizeof(Net::MessageHead_S) %d", m_stParam.stInitParam.nPort, nRecvLen, sizeof(Net::MessageHead_S));
                return;
            }
            auto pData = std::shared_ptr<char[]>(new char[stHead.nDataLength]);
            if (pData == nullptr)
            {
                dlog_error("Memory allocation failed");
                continue;
            }
            nRecvLen = pSession->receive(pData.get(), stHead.nDataLength);
            if (nRecvLen != stHead.nDataLength)
            {
                /* 数据包不完整 */
                dlog_error("Data packet incomplete. nRecvLen [%d] stHead.nDataLength [%d]", nRecvLen, stHead.nDataLength);
                return;
            }

            Net::Message_S stMessage;
            stMessage.nActionCode = stHead.nActionCode;
            stMessage.pHandle = pSession;
            stMessage.pData = pData.get();
            stMessage.nDataLength = nRecvLen;
            receive(stMessage, m_stParam.stUserParam);
            m_heartbeat->update_response();
        }
        catch (const asio::system_error &e)
        {
            dlog_error("Asio system error: %s (Error code: %d)", e.what(), e.code().value());
            return;
        }
        catch (const std::bad_alloc &e)
        {
            dlog_error("Memory allocation failed: %s", e.what());
            return;
        }
        catch (const std::exception &e)
        {
            dlog_error("Standard exception: %s", e.what());
            return;
        }
        catch (...)
        {
            dlog_error("Unknown exception caught!");
            return;
        }
    }
}
void UDSServer::throw_status(int nStatus, void *pHandle)
{
    Net::Message_S stMessage;
    stMessage.nActionCode = m_stParam.stInitParam.nStatusCode;
    stMessage.pData       = &nStatus;
    stMessage.nDataLength = sizeof(nStatus);
    stMessage.pHandle     = pHandle;

    Net::UserParam_S stUserParam;
    if (m_stParam.stInitParam.callbackMap.find(m_stParam.stInitParam.nStatusCode) != m_stParam.stInitParam.callbackMap.end())
    {
        m_stParam.stInitParam.callbackMap.at(m_stParam.stInitParam.nStatusCode)(stMessage, m_stParam.stUserParam);
    }
    else if (m_stParam.stInitParam.fnDefaultCallback)
    {
        m_stParam.stInitParam.fnDefaultCallback(stMessage, m_stParam.stUserParam);
    }
}
void UDSServer::receive(Message_S &stMessage, UserParam_S &stUserParam)
{
    if (m_stParam.stInitParam.callbackMap.find(stMessage.nActionCode)  == m_stParam.stInitParam.callbackMap.end())
    {
        if (m_stParam.stInitParam.fnDefaultCallback == nullptr)
        {
            dlog_error("未设置回调函数");
            return;
        }
        m_stParam.stInitParam.fnDefaultCallback(stMessage, m_stParam.stUserParam);
        return;
    }

    auto fnCallback = m_stParam.stInitParam.callbackMap.at(stMessage.nActionCode);
    if (fnCallback)
        fnCallback(stMessage, m_stParam.stUserParam);
    else
        dlog_error("命令码[%u], 设置的回调函数为空", stMessage.nActionCode);
}

void UDSServer::deal_data(const void *pData, int nDataLen)
{
}


/**
 * @brief 处理新的 UDS 连接
 * 
 * @param pHandle 连接句柄
 */
void UDSServer::deal_connect(void *pHandle)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        UDSAdapter* pSession = static_cast<UDSAdapter*>(pHandle);
        m_sessions.insert(pSession);
        std::thread thr(
            [this, pSession]() {
                receive(pSession);   // 接收数据
                }
            );
        thr.detach();
    }
    throw_status(Net::STATUS_SUCCESS, pHandle);
}

/**
 * @brief 处理断开连接的逻辑
 * 
 * @param pHandle 处理句柄
 */
void UDSServer::deal_disconnect(void *pHandle)
{
    if (pHandle == nullptr)
    {
        return;
    }
    UDSAdapter* pSession = static_cast<UDSAdapter*>(pHandle);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.erase(pSession);
    }
    throw_status(Net::STATUS_DISCONNECT, pHandle);
}
 
/**
 * @brief 处理错误信息
 * 
 * @param nError 错误代码
 */
void UDSServer::deal_error(int nError)
{
}

}
