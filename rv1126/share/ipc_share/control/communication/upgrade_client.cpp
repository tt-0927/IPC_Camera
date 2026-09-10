/**
 * @file upgrade_client.cpp
 * @author lih (lih@kfb.cn)
 * @date 2025-02-17
 * 
 * @brief 
 */

#include "upgrade_client.h"
#include "dlog.h"
#include "action_code.h"

IpcRet_E CUpgradeClient::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&CUpgradeClient::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&CUpgradeClient::deal_heartbeat, this, _1, _2);;
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&CUpgradeClient::deal_status, this, _1, _2);;
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = IN_CONTROL_UPGRADE_PROT;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::TCPClient>(stParam);
    return OK;
}
IpcRet_E CUpgradeClient::deinit()
{
    return OK;
}

void CUpgradeClient::set_taskManage(std::shared_ptr<CTaskManage> pTaskManage)
{
    m_pTaskManage = pTaskManage;
}
int CUpgradeClient::send(std::string data, int nActionCode, void *pHdndler)
{
    return send(static_cast<const void*>(data.c_str()), data.length() + 1, nActionCode, pHdndler);
}
int CUpgradeClient::send(const void *pData, int nDataLen, int nActionCode, void *pHdndler)
{
    if (!m_pHandler)
    {
        return -1;
    }
    Net::Param_S stParam;
    Net::Message_S stMessage;
    stMessage.pHandle = pHdndler;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = pData;
    stMessage.nDataLength = nDataLen;
    return m_pHandler->send(stMessage);
}

void CUpgradeClient::deal_heartbeat(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    // dlog_debug("接收到心跳消息：%s", stMessage.pData);
}
void CUpgradeClient::deal_status(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    int nStatus = *(const int*)stMessage.pData;
    if (nStatus == Net::STATUS_SUCCESS)
    {
        dlog_debug("已接入服务端");
        if (m_statusObserver)
        {
            m_statusObserver(Common::REQUESTER_UPGRADE, true);
        }
    }
    else
    {
        dlog_debug("已断开服务端");
        if (m_statusObserver)
        {
            m_statusObserver(Common::REQUESTER_UPGRADE, false);
        }
    }

}
void CUpgradeClient::deal_message(Net::Message_S& stMessage, Net::UserParam_S &stUserParam)
{
    if (m_pTaskManage == nullptr)
    {
        dlog_error("无绑定任务");
        return;
    }
    dlog_debug("接收到[%d]消息：%s", stMessage.nActionCode, stMessage.pData);
    Task::Info_S stInfo;
    stInfo.pHandler = stMessage.pHandle;
    stInfo.data = static_cast<const char*>(stMessage.pData);
    stInfo.enRequester = Common::REQUESTER_UPGRADE;
    stInfo.fnResultCallbacks = std::bind(static_cast<int(CUpgradeClient::*)(const void*, int, int, void*)>(&CUpgradeClient::send), this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    int ret = m_pTaskManage->execute(stMessage.nActionCode, stInfo);
    if (ret < 0)
    {
        dlog_error("未绑定任务");
        return;
    }
}

void CUpgradeClient::fill_head(std::string &strData, int nActionCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");

    if (!strData.empty())
    {
        Json::Object *pJsonData = Json::init(strData);
        if (pJsonData)
        {
            Json::add(pJsonRoot, "Data", pJsonData);
        }
    }

    strData = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return;
}

bool CUpgradeClient::get_connect()
{
   return bEnConnect;
}
void CUpgradeClient::set_statusObserver(Common::StatusCallback observer)
{
    m_statusObserver = observer;
}
 