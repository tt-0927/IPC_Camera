/**
 * @FilePath     : record_server.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-30 11:38:52
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-02 09:20:04
 * @Description  : 录制通讯服务端
 */

#include "record_server.h"
#include "dlog.h"
#include "action_code.h"

#include "UDSServer.h"

int CRecordServer::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&CRecordServer::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&CRecordServer::deal_heartbeat, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&CRecordServer::deal_status, this, _1, _2);
    stParam.stInitParam.callbackMap[30032] = std::bind(&CRecordServer::deal_heartbeat, this, _1, _2);
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = IN_CONTROL_RECORD_PROT;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::UDSServer>(stParam);
    return 0;
}

void CRecordServer::deinit()
{
}

void CRecordServer::set_taskManage(std::shared_ptr<CTaskManage> pTaskManage)
{
    m_pTaskManage = pTaskManage;
}

int CRecordServer::send(const void *pData, int nDataLen, int nActionCode, void *pHandle)
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

int CRecordServer::send_withHead(std::string data, int nActionCode, void *pHandle)
{
    fill_head(data, nActionCode);
    return send(data.c_str(), data.length() + 1, nActionCode, pHandle);
}

void CRecordServer::deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    // dlog_debug("接收到心跳消息：%s", stMessage.pData);
}

void CRecordServer::deal_status(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    int nStatus = *(const int *)stMessage.pData;
    if (nStatus == Net::STATUS_SUCCESS)
    {
        dlog_debug("已接入服务端");
        if (m_statusObserver)
        {
            m_statusObserver(Common::REQUESTER_RECORD, true);
        }
    }
    else
    {
        dlog_debug("已断开服务端");
        if (m_statusObserver)
        {
            m_statusObserver(Common::REQUESTER_RECORD, false);
        }
        // Task::Info_S stInfo;
        // stInfo.pHandler = stMessage.pHandle;
        // stInfo.enRequester = Common::REQUESTER_RECORD;
        // stInfo.fnResultCallbacks = std::bind(&CRecordServer::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
        // int ret = m_pTaskManage->execute(AC_NOTICE_RECORD_EXCEPTION, stInfo);
        // if (ret < 0)
        // {
        //     dlog_error("未绑定任务");
        //     return;
        // }
    }
}

void CRecordServer::deal_message(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    if (m_pTaskManage == nullptr)
    {
        dlog_error("无绑定任务");
        return;
    }
    dlog_debug("接收到[%d]消息：%s", stMessage.nActionCode, stMessage.pData);
    Task::Info_S stInfo;
    stInfo.pHandler = stMessage.pHandle;
    stInfo.data = static_cast<const char *>(stMessage.pData);
    stInfo.enRequester = Common::REQUESTER_RECORD;
    stInfo.fnResultCallbacks = std::bind(&CRecordServer::send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    int ret = m_pTaskManage->execute(stMessage.nActionCode, stInfo);
    if (ret < 0)
    {
        dlog_error("未绑定任务");
        return;
    }
}

void CRecordServer::fill_head(std::string &data, int nActionCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "UserName", "");

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return;
}

void CRecordServer::set_statusObserver(Common::StatusCallback observer)
{
    m_statusObserver = observer;
}
