#include "UDSServerExample.h"

int UDSServerExample::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = 65533;
    stParam.stInitParam.nStatusCode = 65534;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&UDSServerExample::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&UDSServerExample::deal_heartbeat, this, _1, _2);;
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&UDSServerExample::deal_status, this, _1, _2);;
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = 8888;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::UDSServer>(stParam);
    return 0;
}
void UDSServerExample::deinit()
{

}

void UDSServerExample::set_heartbeat(const void *pData, size_t nLength)
{
    if (!m_pHandler)
    {
        return;
    }
    m_pHandler->set_heartbeat(pData, nLength);
}

int UDSServerExample::send(std::string data, int nActionCode, void *pHandle)
{
    if (!m_pHandler)
    {
        return -1;
    }
    Net::Param_S stParam;
    Net::Message_S stMessage;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = data.c_str();
    stMessage.nDataLength = data.length() + 1;
    stMessage.pHandle = pHandle;
    return m_pHandler->send(stMessage);
}

void UDSServerExample::deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到心跳消息：%s", stMessage.pData);
}
void UDSServerExample::deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{ 
    int nStatus = *(const int*)stMessage.pData;
    if (nStatus == Net::STATUS_SUCCESS)
    {
        dlog_debug("客户端已接入");
        std::string strMsg = "欢迎来到服务端";
        send(strMsg, 1);
        send(strMsg, 2);
        send(strMsg, 3);

    }
    else
    {
        dlog_debug("客户端已断开");
    }
}
void UDSServerExample::deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到[%d]消息：%s", stMessage.nActionCode, stMessage.pData);
    send("这里是服务端 这里是服务端 已收到 已收到 over over", stMessage.nActionCode, stMessage.pHandle);
}