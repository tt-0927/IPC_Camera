
#include "TCPServer.h"

#include "AsioTCPServer.h"
#include "BlTCPServer.h"
#include "dlog.h"
#include <cstring>
#include <unistd.h>
#include "TcpAdapter.h"

namespace Net
{

TCPServer::TCPServer(Param_S &stParam)
    : m_stParam(stParam)
    , m_bExit(false)
{
    m_server = std::make_shared<AsioTCPServer>(stParam.stInitParam.nPort);
   

    // 设置回调函数，用于处理连接、断开和错误
    Callback callback;
    callback.set_connectObserver(std::bind(&TCPServer::deal_connect, this, std::placeholders::_1));
    callback.set_closeObserver(std::bind(&TCPServer::deal_disconnect, this, std::placeholders::_1));
    callback.set_errorObserver(std::bind(&TCPServer::deal_error, this, std::placeholders::_1));
        
    // 将回调函数设置到服务器并启动服务器
    m_server->set_callback(callback);
    m_server->start(); 

    // 初始化心跳相关设置
    m_heartbeat = std::make_shared<Heartbeat>(this, stParam.stInitParam.nHeartbeatInterval);
    std::string heartbeatMessage = "This is server heartbeat";
    m_heartbeat->set_messge(heartbeatMessage.c_str(), heartbeatMessage.length() + 1);
    m_heartbeat->set_observer(std::bind(&TCPServer::heartbeat_status, this, std::placeholders::_1));
    m_heartbeat->set_actionCode(stParam.stInitParam.nHearbeatCode);
    m_heartbeat->start();
}
TCPServer::~TCPServer()
{
    m_bExit = true;
    m_tid.join();
}


int TCPServer::send(const Message_S stMessage)
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
            /* Transport endpoint is not connected (Error code: 107) */
            /* Broken pipe (Error code: 32) */
            if (nRet == -32 || nRet == -107)
            {
                disconnectSession.push_back(pHandle);
            }
            dlog_error("发送数据头失败 %d", nRet);
            return nRet;
        }
        /* 发数据 */
        nRet = m_server->send(stMessage.pData, stMessage.nDataLength, pHandle);
        if (nRet < 0)
        {
            /* Transport endpoint is not connected (Error code: 107) */
            /* Broken pipe (Error code: 32) */
            if (nRet == -32 || nRet == -107)
            {
                disconnectSession.push_back(pHandle);
            }
            dlog_error("发送数据失败 %d", nRet);
            return nRet;
        }
        return nRet;
    };
    int nRet = 0;
    if (stMessage.pHandle == nullptr)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
            /* 发给所有客户端 */
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it)
        {
            nRet = send_data(*it);    
        }
        if (m_sessions.size() == 0)
        {
            dlog_error("[%d]没有客户端连接", m_stParam.stInitParam.nPort);
            nRet = -1;
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(m_mutex);
            /* 发给指定客户端 */
        nRet = send_data(stMessage.pHandle);   
    }
    /* 清理掉已断开的客户端 */
    for (auto it = disconnectSession.begin(); it != disconnectSession.end(); ++it)
    {
        dlog_error("清理掉已断开的客户端");
        deal_disconnect(*it);
    }
    return nRet;
}

int TCPServer::receive(Message_S &stMessage)
{
    return 0;
}

void TCPServer::set_heartbeat(const void *pData, size_t nLength)
{
    if (!pData)
    {
        return;
    }
    m_heartbeat->set_messge(pData, nLength);
}

void TCPServer::heartbeat_status(bool bStatus)
{
}

void TCPServer::receive(TcpAdapter* pSession)
{
    while (true)
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
        stMessage.ip = pSession->get_ip();
        receive(stMessage, m_stParam.stUserParam);
        m_heartbeat->update_response();
    }
}
void TCPServer::throw_status(int nStatus, void *pHandle)
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
void TCPServer::receive(Message_S &stMessage, UserParam_S &stUserParam)
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

void TCPServer::deal_data(const void *pData, int nDataLen)
{
}


/**
 * @brief 处理新的 TCP 连接
 * 
 * @param pHandle 连接句柄
 */
void TCPServer::deal_connect(void *pHandle)
{
    {    
        std::lock_guard<std::mutex> lock(m_mutex);
        TcpAdapter* pSession = static_cast<TcpAdapter*>(pHandle);
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
void TCPServer::deal_disconnect(void *pHandle)
{
    if (pHandle == nullptr)
    {
        return;
    }
    TcpAdapter* pSession = static_cast<TcpAdapter*>(pHandle);
    pSession->stop();
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
void TCPServer::deal_error(int nError)
{
}

}