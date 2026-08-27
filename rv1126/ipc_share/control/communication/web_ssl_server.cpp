/**
 * @file web_ssl_server.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-01-14
 * 
 * @brief 
 */
#include "web_ssl_server.h"
#include "dlog.h"
#include "common_define.h"
#include "action_code.h"
#include "libwebsockets.h"

int CWebSslServer::init(std::string strCert,std::string strKey)
{
    dlog_info("开启ssl加密服务器");
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&CWebSslServer::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&CWebSslServer::deal_heartbeat, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&CWebSslServer::deal_status, this, _1, _2);
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = IN_WEB_CONTROL_SSL_PROT;
    /* 加密信息 */
    stParam.stInitParam.bEnssl = true;
    stParam.stInitParam.strCert = strCert;
    stParam.stInitParam.strKey = strKey;
    stParam.stInitParam.strVhost = "sslhost";
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::WebSocketServer>(stParam);
    m_EnInit = true;
    return 0;
}
void CWebSslServer::deinit()
{
    dlog_info("关闭ssl加密服务器");
    m_pHandler->disconnect();
    m_EnInit = false;
}

void CWebSslServer::set_taskManage(std::shared_ptr<CTaskManage> pTaskManage)
{
    m_pTaskManage = pTaskManage;
    
    /* 订阅设置 */
    std::vector<int> actionCode;
    actionCode.push_back(AC_FACE_DETECT_EVENT);
    actionCode.push_back(AC_PUSH_FACE_CAPTURE_INFO);
    auto fnResultCallbacks = std::bind(&CWebSslServer::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    m_pTaskManage->register_subscribe(actionCode, fnResultCallbacks);
}
int CWebSslServer::send(const void *pData, int nDataLen, int nActionCode, void *pHandle)
{
    if (!m_pHandler)
    {
        return -1;
    }
    Net::Param_S stParam;
    Net::Message_S stMessage;
    stMessage.pHandle = pHandle;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = pData;
    stMessage.nDataLength = nDataLen;
    return m_pHandler->send(stMessage);
}
void CWebSslServer::deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    // dlog_debug("接收到心跳消息：%s", stMessage.pData);
}
void CWebSslServer::deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    int nStatus = *(const int *)stMessage.pData;
    if (nStatus == Net::STATUS_SUCCESS)
    {
        dlog_debug("客户端已接入");
    }
    else
    {
        dlog_debug("客户端已断开");
    }
}
void CWebSslServer::deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    if (m_pTaskManage == nullptr)
    {
        dlog_error("无绑定任务");
        return;
    }
    Json::get(static_cast<const char *>(stMessage.pData), "ActionCode", (int &)stMessage.nActionCode);
    if (stMessage.nActionCode <= 0)
    {
        return;
    }

    /* 获取登录设备的ip */
    if (stMessage.nActionCode  == AC_LOGIN)
    {
        char achName[64];
        struct lws * pLws = (struct lws *)stMessage.pHandle;
        dlog_info("开始获取登录设备的IP");
       
        const char *pResult = lws_get_peer_simple(pLws,achName,sizeof(achName));
        if(pResult)
        {
            dlog_info("登录的设备ip：[%s],设备名称[%s]",pResult,achName);
        }
        else
        {
            dlog_error("获取登录设备ip失败");
        }
        m_LoginDeviceIp = std::string(pResult);
    }

    dlog_debug("接收到[%d]消息：%s", stMessage.nActionCode, stMessage.pData);
    Task::Info_S stInfo;
    stInfo.pHandler = stMessage.pHandle;
    stInfo.data = static_cast<const char*>(stMessage.pData);
    stInfo.enRequester = Common::REQUESTER_SSL_WEB;
    stInfo.fnResultCallbacks = std::bind(&CWebSslServer::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    int ret = m_pTaskManage->execute(stMessage.nActionCode, stInfo);
    if (ret < 0)
    {
        dlog_error("未绑定任务");
        return;
    }
}

 std::string CWebSslServer::get_loginclient_ip()
 {
    return m_LoginDeviceIp;
 }


 bool CWebSslServer::is_init()
 {
    return m_EnInit;
 }