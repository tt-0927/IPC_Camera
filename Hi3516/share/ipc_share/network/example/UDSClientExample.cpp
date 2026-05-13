
#include "UDSClientExample.h"

int UDSClientExample::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = 65533;
    stParam.stInitParam.nStatusCode = 65534;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&UDSClientExample::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&UDSClientExample::deal_heartbeat, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&UDSClientExample::deal_status, this, _1, _2);
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = 8888;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::UDSClient>(stParam);
    return 0;
}
void UDSClientExample::deinit()
{

}
void UDSClientExample::set_heartbeat(const void *pData, size_t nLength)
{
    if (!m_pHandler)
    {
        return;
    }
    m_pHandler->set_heartbeat(pData, nLength);
}
int UDSClientExample::send(std::string data, int nActionCode)
{
    if (!m_pHandler)
    {
        return -1;
    }
    Net::Message_S stMessage;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = data.c_str();
    stMessage.nDataLength = data.length() + 1;
    return m_pHandler->send(stMessage);
}

void UDSClientExample::deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到心跳消息：%s", stMessage.pData);
}
void UDSClientExample::deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到状态消息：%d", *(const int *)stMessage.pData);
}
void UDSClientExample::deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到[%d]消息：%s", stMessage.nActionCode, stMessage.pData);
}