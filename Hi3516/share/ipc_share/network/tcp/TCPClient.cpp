
#include "TCPClient.h"

#include "AsioTCPClient.h"
#include "BlTCPClient.h"
#include <cstring>
#include <unistd.h>

namespace Net
{

TCPClient::TCPClient(Param_S &stParam)
    : m_stParam(stParam)
    , m_bExit(false)
    , m_msgQueue(stParam.stInitParam.nQueueSize)
{
    // 创建客户端
    auto fnMessageCallback = std::bind(static_cast<void(TCPClient::*)(Message_S&, UserParam_S&)>(&TCPClient::receive), this, std::placeholders::_1, std::placeholders::_2);
    m_client = std::make_shared<AsioTCPClient>(stParam.stInitParam.ip, stParam.stInitParam.nPort);
    m_client->start();
    
    // 初始化心跳相关设置
    m_heartbeat = std::make_shared<Heartbeat>(this, stParam.stInitParam.nHeartbeatInterval);
    std::string heartbeatMessage = "This is client heartbeat";
    m_heartbeat->set_messge(heartbeatMessage.c_str(), heartbeatMessage.length() + 1);
    m_heartbeat->set_observer(std::bind(&TCPClient::heartbeat_status, this, std::placeholders::_1));
    m_heartbeat->set_actionCode(stParam.stInitParam.nHearbeatCode);
    m_heartbeat->start();

    // 启动接收线程
    m_tid = std::thread([this]{
        receive();
    });
    m_queTid = std::thread([this]{
        while (!m_bExit)
        {
            Message_S stMessage;
            int nRet = m_msgQueue.pop(stMessage);
            if (nRet < 0)
            {
                continue;
            }
            receive(stMessage, m_stParam.stUserParam);
            if (stMessage.pData)
            {
                delete[] const_cast<char*>(static_cast<const char*>(stMessage.pData));
                stMessage.pData = nullptr;
            }
        }
        Message_S stMessage;
        while (m_msgQueue.size() > 0)
        {
            Message_S stMessage;
            int nRet = m_msgQueue.pop(stMessage);
            if (nRet < 0)
            {
                continue;
            }
            if (stMessage.pData)
            {
                delete[] const_cast<char*>(static_cast<const char*>(stMessage.pData));
                stMessage.pData = nullptr;
            }
        }
    });
    m_queTid.detach();
    if (m_client->is_connected())
    {
        throw_status(Net::STATUS_SUCCESS);
    }
}

TCPClient::~TCPClient()
{
    m_client->stop();
    m_heartbeat->stop();
    m_bExit = true;
    if (m_tid.joinable())
    {
        m_tid.join();
    }
}
int TCPClient::send(const Message_S stMessage)
{
    if (!m_client) 
    {
        return -1;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    Net::MessageHead_S stHead;
    stHead.nActionCode = stMessage.nActionCode;
    stHead.nDataLength = stMessage.nDataLength;
    /* 发数据头 */
    int nRet = m_client->send(&stHead, sizeof(stHead));
    if (nRet < 0)
    {
        //dlog_error("发送数据头失败 %d", nRet);
        return nRet;
    }
    /* 发数据 */
    nRet = m_client->send(stMessage.pData, stMessage.nDataLength);
    if (nRet < 0)
    {
        dlog_error("发送数据失败 %d", nRet);
        return nRet;
    }
    return nRet;
}
void TCPClient::reconnect()
{
    throw_status(Net::STATUS_DISCONNECT);
    if (m_bReconnect == true)
    {
        return;
    }
    m_bReconnect = true;
    do {
        int nRet = m_client->reconnect();
        if (nRet < 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
        else
        {
            m_bReconnect = false;
            break;
        }
    } while (!m_bExit);
    if (m_heartbeat)
    {
        m_heartbeat->update_response();
    }   
    std::thread thr(
    [this]() {
        receive();   // 接收数据
        }
    );
    thr.detach();
    throw_status(Net::STATUS_SUCCESS);
}

int TCPClient::receive(Message_S &stMessage)
{
    if (!m_client) 
    {
        return -1;
    }
    return 0;
}
void TCPClient::set_heartbeat(const void *pData, size_t nLength)
{
    if (m_heartbeat)
    {
        m_heartbeat->set_messge(pData, nLength);
    }
}

void TCPClient::heartbeat_status(bool bOK)
{
    if (!bOK)
    {
        reconnect();
    }
}


void TCPClient::receive()
{
    do
    {
        Net::MessageHead_S stHead;
        int nRecvLen = m_client->receive(&stHead, sizeof(Net::MessageHead_S));
        if (nRecvLen != sizeof(Net::MessageHead_S))
        {
            if (nRecvLen == -EBADF || nRecvLen == -EINVAL)
            {
                return;
            }
            if (nRecvLen < 0)
            {
                reconnect(); 
                return;
            }
            /* 数据包不完整 */
            dlog_error("Header packet incomplete. nRecvLen[%d]", nRecvLen);
            continue;
        }
        auto pData = new (std::nothrow) char[stHead.nDataLength];
        if (pData == nullptr)
        {
            dlog_error("Memory allocation failed");
            continue;
        }
        nRecvLen = m_client->receive(pData, stHead.nDataLength);
        if (nRecvLen != stHead.nDataLength)
        {
            delete[] pData;
            if (nRecvLen == -EBADF || nRecvLen == -EINVAL)
            {
                return;
            }
            if (nRecvLen < 0)
            {
                reconnect(); 
                return;
            }
            /* 数据包不完整 */
            dlog_error("Data packet incomplete. nRecvLen [%d] stHead.nDataLength [%d]", nRecvLen, stHead.nDataLength);
            continue;
        }
        Net::Message_S stMessage;
        stMessage.nActionCode = stHead.nActionCode;
        stMessage.pData = pData;
        stMessage.nDataLength = nRecvLen;
        int nRet = m_msgQueue.push(stMessage, SafeQueue<TCPClient>::TIMEOUT_NONE);
        if (nRet < 0)
        {
            dlog_error("Message queue push failed. port [%d] size [%d]", m_stParam.stInitParam.nPort, m_msgQueue.size());
            delete[] pData;
        }
        // receive(stMessage, m_stParam.stUserParam);
        if (m_heartbeat)
        {
            m_heartbeat->update_response();
        }
    } while (!m_bExit);
}

void TCPClient::receive(Message_S& stMessage, UserParam_S &stUserParam)
{
    if (m_stParam.stInitParam.callbackMap.find(stMessage.nActionCode) == m_stParam.stInitParam.callbackMap.end())
    {
        if (m_stParam.stInitParam.fnDefaultCallback == nullptr)
        {
            dlog_error("未设置回调函数");
            return;
        }
        stMessage.pHandle = this;
        m_stParam.stInitParam.fnDefaultCallback(stMessage, m_stParam.stUserParam);
        return;
    }
    auto fn = m_stParam.stInitParam.callbackMap.at(stMessage.nActionCode);
    if (fn)
        fn(stMessage, m_stParam.stUserParam);
    return;

}

void TCPClient::throw_status(int nStatus)
{
    Net::Message_S stMessage;
    stMessage.nActionCode = m_stParam.stInitParam.nStatusCode;
    stMessage.pData       = &nStatus;
    stMessage.nDataLength = sizeof(nStatus);

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

}