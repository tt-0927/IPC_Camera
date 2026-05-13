
#include "UDSClient.h"

#include "AsioUDSClient.h"
#include <cstring>
#include <future>
#include <unistd.h>

extern int random_num;
namespace Net
{
UDSClient::UDSClient(Param_S &stParam)
    : m_stParam(stParam), m_bExit(false)
{
    // 创建客户端
    auto fnMessageCallback = std::bind(static_cast<void(UDSClient::*)(Message_S&, UserParam_S&)>(&UDSClient::receive), this, std::placeholders::_1, std::placeholders::_2);
    // m_client = std::make_shared<AsioUDSClient>(stParam.stInitParam.ip, stParam.stInitParam.nPort);

    try 
    {
        m_client = std::make_shared<AsioUDSClient>(stParam.stInitParam.ip, stParam.stInitParam.nPort);
    }
    catch (const std::exception& e) 
    {
        dlog_error("创建客户端失败: %s", e.what());
        throw;
    }

    while(!m_client->is_connected())
    {
        m_client->reconnect();
        usleep(10*1000);
    }

    m_client->start();
    
    // 初始化心跳相关设置
    m_heartbeat = std::make_shared<Heartbeat>(this, stParam.stInitParam.nHeartbeatInterval);
    std::string heartbeatMessage = "This is client heartbeat";
    m_heartbeat->set_messge(heartbeatMessage.c_str(), heartbeatMessage.length() + 1);
    m_heartbeat->set_observer(std::bind(&UDSClient::heartbeat_status, this, std::placeholders::_1));
    m_heartbeat->set_actionCode(stParam.stInitParam.nHearbeatCode);
    m_heartbeat->start();

    dlog_info("UDS客户端启动成功, port: %d", stParam.stInitParam.nPort);
    /*启动接收线程*/
    m_tid = std::thread([this]{
        receive();
    });
    // /*分离线程 否则在析构中，等待线程，无法正常销毁*/
    // if (m_tid.joinable())
    // {
    //     m_tid.detach();
    // }
    if (m_client->is_connected())
    {
        throw_status(Net::STATUS_SUCCESS);
    }
}

UDSClient::~UDSClient()
{
    cleanup();
    dlog_info("UDS客户端释放成功, port: %d", m_stParam.stInitParam.nPort);
}

void UDSClient::cleanup()
{
    if (m_bExit.exchange(true))
    {
        /* 已经在清理中，避免重复清理 */
        return;
    }

    dlog_info("开始清理UDS客户端, port: %d", m_stParam.stInitParam.nPort);

    /* 停止心跳 */
    if (m_heartbeat)
    {
        m_heartbeat->stop();
        m_heartbeat.reset();
    }

    /* 停止客户端连接 */
    if (m_client)
    {
        m_client->stop();
    }

    /* 通知条件变量，唤醒可能在等待的线程 */
    m_cv.notify_all();

    /* 等待接收线程结束（带超时） */
    if (m_tid.joinable())
    {
        /* 3秒超时 */
        auto timeout = std::chrono::milliseconds(3000);
        auto future = std::async(std::launch::async,
                                 [&]()
                                 {
                                     m_tid.join();
                                 });

        if (future.wait_for(timeout) == std::future_status::timeout)
        {
            dlog_error("接收线程超时未退出, port: %d", m_stParam.stInitParam.nPort);
            /* 注意：这里不能detach，因为线程可能还在访问成员变量 */
        }
        else
        {
            dlog_info("接收线程正常退出, port: %d", m_stParam.stInitParam.nPort);
        }
    }

    /* 重置客户端 */
    m_client.reset();
}

int UDSClient::send(const Message_S stMessage)
{
    if (!m_client)
    {
        return ERR;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    Net::MessageHead_S stHead;
    stHead.nActionCode = stMessage.nActionCode;
    stHead.nDataLength = stMessage.nDataLength;
    /* 发数据头 */
    m_client->send(&stHead, sizeof(stHead));
    /* 发数据 */
    m_client->send(stMessage.pData, stMessage.nDataLength);
    return OK;
}

void UDSClient::reconnect()
{
    /*如果正在退出，不进行重连*/
    if (m_bExit) {
        return;
    }

    throw_status(Net::STATUS_DISCONNECT);
    if (m_bReconnect == true)
    {
        return;
    }
    m_bReconnect = true;
    do
    {
        int nRet = m_client->reconnect();
        if (nRet < 0)
        {
            /*使用条件变量和退出标志进行可中断的等待*/
            // std::unique_lock<std::mutex> lock(m_mutex);
            // m_cv.wait_for(lock, std::chrono::seconds(3), [this] { return m_bExit.load(); });
            /*使用更短的睡眠时间，并检查退出标志*/
            for (int i = 0; i < 20; ++i) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                if(m_bExit)
                {
                    break;
                }
            }
        }
        else
        {
            m_bReconnect = false;
            break;
        }
    } while (!m_bExit);

    /*如果不是因为退出而结束循环，则启动接收线程*/
    if (!m_bExit && m_client && m_client->is_connected()) {
        if (m_heartbeat) {
            m_heartbeat->update_response();
        }
        
        std::thread thr([this]() {
            receive();   // 接收数据
        });
        thr.detach(); // 这个线程仍然detach，因为主接收线程会处理退出
        
        throw_status(Net::STATUS_SUCCESS);
    }
}

int UDSClient::receive(Message_S &stMessage)
{
    if (!m_client)
    {
        return ERR;
    }
    return OK;
}

void UDSClient::set_heartbeat(const void *pData, size_t nLength)
{
    if (m_heartbeat)
    {
        m_heartbeat->set_messge(pData, nLength);
    }
}

void UDSClient::heartbeat_status(bool bOK)
{
    if (!bOK)
    {
        reconnect();
    }
}

void UDSClient::receive()
{
    while (!m_bExit && m_client && m_client->is_connected())
    {
        try {
            Net::MessageHead_S stHead;
            int nRecvLen = m_client->receive(&stHead, sizeof(Net::MessageHead_S));
            if (nRecvLen != sizeof(Net::MessageHead_S))
            {
                if (nRecvLen < 0)
                {
                    /*检查是否是因为正在退出而导致的错误*/
                    if (!m_bExit) {
                        reconnect();
                    }
                    return;
                }
                /* 数据包不完整 */
                dlog_error("头部数据包不完整. nRecvLen[%d]", nRecvLen);
                continue;
            }
            
            auto pData = std::shared_ptr<char[]>(new char[stHead.nDataLength]);
            if (pData == nullptr)
            {
                dlog_error("内存分配失败");
                continue;
            }
            
            nRecvLen = m_client->receive(pData.get(), stHead.nDataLength);
            
            /*再次检查退出标志*/
            if (m_bExit) {
                break;
            }
            
            if (nRecvLen != stHead.nDataLength)
            {
                if (nRecvLen < 0)
                {
                    /*检查是否是因为正在退出而导致的错误*/
                    if (!m_bExit) {
                        reconnect();
                    }
                    return;
                }
                /* 数据包不完整 */
                dlog_error("数据包不完整. nRecvLen [%d] stHead.nDataLength [%d]", nRecvLen, stHead.nDataLength);
                continue;
            }
            
            Net::Message_S stMessage;
            stMessage.nActionCode = stHead.nActionCode;
            stMessage.pData = pData.get();
            stMessage.nDataLength = nRecvLen;
            receive(stMessage, m_stParam.stUserParam);
            
            /*只有在未退出时才更新心跳*/
            if (!m_bExit && m_heartbeat) {
                m_heartbeat->update_response();
            }
        }
        catch (const std::exception& e) {
            if (!m_bExit) {
                dlog_error("接收循环中出现异常: %s", e.what());
                // 可能需要重连
                reconnect();
            }
            break;
        }
        catch (...) {
            if (!m_bExit) {
                dlog_error("接收循环中出现未知错误");
                reconnect();
            }
            break;
        }
    }
    dlog_info("receive 接收线程退出, port: %d", m_stParam.stInitParam.nPort);
}

void UDSClient::receive(Message_S &stMessage, UserParam_S &stUserParam)
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

void UDSClient::throw_status(int nStatus)
{
    Net::Message_S stMessage;
    stMessage.nActionCode = m_stParam.stInitParam.nStatusCode;
    stMessage.pData = &nStatus;
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