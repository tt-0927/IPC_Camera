/**
 * @file WebSocketServer.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-08
 * 
 * @brief 
 */

#include "WebSocketServer.h"

#include "LibWSServer.h"
#include "dlog.h"
#include <cstring>
#include <unistd.h>

namespace Net
{

WebSocketServer::WebSocketServer(Param_S &stParam)
    : m_stParam(stParam)
    , m_bExit(false)
{
    std::string heartbeatMessage = "This is server heartbeat";
    m_heartbeat.assign(heartbeatMessage.begin(), heartbeatMessage.end());
    m_heartbeat.push_back('\0');
    auto fnMessageCallback = std::bind(static_cast<void(WebSocketServer::*)(Message_S&, UserParam_S&)>(&WebSocketServer::receive), this, std::placeholders::_1, std::placeholders::_2);
    m_server = std::make_shared<LibWSServer>(stParam, fnMessageCallback);
    m_tid = std::thread(std::bind(&WebSocketServer::thr_heartbeat, this));
}
WebSocketServer::~WebSocketServer()
{
    m_bExit = true;
    m_server->disconnect();

    if (m_tid.joinable())
    {
        const auto timeout = std::chrono::milliseconds(100);
        if (m_tid.joinable())
        {  // 双重检查防止竞争
            if (m_tid.joinable()) // 三度检查确保线程状态
            {
                m_tid.join();
            }
        }
    }
}


int WebSocketServer::send(const Message_S stMessage)
{
    if (!m_server) 
    {
        return -1;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_server->send(stMessage);
    return 0;
}

int WebSocketServer::receive(Message_S &stMessage)
{
    return 0;
}

void WebSocketServer::set_heartbeat(const void *pData, size_t nLength)
{
    if (!pData)
    {
        return;
    }
    std::unique_lock<std::mutex> mtx(m_mutex);
    m_heartbeat.resize(nLength);
    memcpy(m_heartbeat.data(), pData, nLength);
}

std::vector<char> WebSocketServer::get_heartbeat()
{
    std::unique_lock<std::mutex> mtx(m_mutex);
    return m_heartbeat;
}
void WebSocketServer::receive(Message_S& stMessage, UserParam_S &stUserParam)
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


void WebSocketServer::thr_heartbeat()
{
    pthread_setname_np(pthread_self(), "WS_heartbeat");

    const auto interval = std::chrono::milliseconds(m_stParam.stInitParam.nHeartbeatInterval);
    const auto sleep_chunk = std::chrono::milliseconds(100); /* 100ms细粒度检查 */
    while (!m_bExit)
    {
        // 细粒度睡眠+退出检查
        const auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < interval) 
        {
            /* 实时检查退出标志 */
            if (m_bExit)
            {
                return;
            }
            std::this_thread::sleep_for(sleep_chunk);
        }

        std::vector<char> buffer = get_heartbeat();
        Message_S stMessage;
        stMessage.nActionCode = m_stParam.stInitParam.nHearbeatCode;
        stMessage.pData = buffer.data();
        stMessage.nDataLength = buffer.size();
        send(stMessage);
    }
}


void WebSocketServer::disconnect()
{
    dlog_info("websocketServer断开连接");
    m_bExit = true;
    if (m_tid.joinable()) 
    {
        m_tid.join();
    }
    m_server->disconnect();
}

void WebSocketServer::set_file_upload_path(const std::string &strFilePath)
{
    m_server->set_file_upload_path(strFilePath);
}

std::string WebSocketServer::get_upload_fileName()
{
    return m_server->get_upload_filename();
}

}