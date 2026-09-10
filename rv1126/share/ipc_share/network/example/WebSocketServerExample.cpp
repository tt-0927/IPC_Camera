/**
 * @file WebSocketServerExample.cpp
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-08
 * 
 * @brief 
 */
#include "WebSocketServerExample.h"

int WebSocketServerExample::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = 996;
    stParam.stInitParam.nStatusCode = 997;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&WebSocketServerExample::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&WebSocketServerExample::deal_heartbeat, this, _1, _2);;
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&WebSocketServerExample::deal_status, this, _1, _2);;
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = 8888;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::WebSocketServer>(stParam);
    return 0;
}
void WebSocketServerExample::deinit()
{

}

void WebSocketServerExample::set_heartbeat(const void *pData, size_t nLength)
{
    if (!m_pHandler)
    {
        return;
    }
    m_pHandler->set_heartbeat(pData, nLength);
}

int WebSocketServerExample::send(std::string data, int nActionCode)
{
    if (!m_pHandler)
    {
        return -1;
    }
    Net::Param_S stParam;
    Net::Message_S stMessage;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = data.c_str();
    stMessage.nDataLength = data.length();
    return m_pHandler->send(stMessage);
}

void WebSocketServerExample::deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到心跳消息：%s", stMessage.pData);
}
void WebSocketServerExample::deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到状态消息：%d", *(const int *)stMessage.pData);
}
void WebSocketServerExample::deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到[%d]消息：%s", stMessage.nActionCode, stMessage.pData);
    send("这里是服务端 这里是服务端 已收到 已收到 over over", stMessage.nActionCode);
}