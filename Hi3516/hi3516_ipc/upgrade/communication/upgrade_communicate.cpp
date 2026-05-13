/*
 * @FilePath     : upgrade_communicate.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-08-26 11:10:07
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2025-02-18 11:25:53
 * @Description  : 通讯模块
 */
#include "upgrade_communicate.h"
#include "dlog.h"
#include "Json.h"
#include "action_code.h"
#include "system_define.h"
#include "system_convert.h"
#include "convert_interface.h"
#include "data_manage.h"
#include "upgrade_logic.h"

int UpgradeServer::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&UpgradeServer::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&UpgradeServer::deal_heartbeat, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&UpgradeServer::deal_status, this, _1, _2);
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = IN_CONTROL_UPGRADE_PROT;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::TCPServer>(stParam);

    return 0;
}

void UpgradeServer::deinit()
{
    dlog_info("关闭升级服务器");
}

int UpgradeServer::send(
    std::string data,
    int nActionCode,
    void *pHdndler)
{
    if (!m_pHandler)
    {
        return -1;
    }
    Net::Param_S stParam;
    Net::Message_S stMessage;
    stMessage.pHandle = pHdndler;
    stMessage.nActionCode = nActionCode;
    stMessage.pData = data.c_str();
    stMessage.nDataLength = data.length();
    dlog_debug("发送指令码【%d】\n%s", nActionCode, data.data());
    return m_pHandler->send(stMessage);
}

void UpgradeServer::set_heartbeat(
    const void *pData,
    size_t nLength)
{
}

void UpgradeServer::deal_heartbeat(
    Net::Message_S &stMessage,
    Net::UserParam_S &stUserParam)
{
}

void UpgradeServer::deal_status(
    Net::Message_S &stMessage,
    Net::UserParam_S &stUserParam)
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

void UpgradeServer::deal_message(
    Net::Message_S &stMessage,
    Net::UserParam_S &stUserParam)
{
    dlog_debug("接收到[%d]消息：%s", stMessage.nActionCode, stMessage.pData);

    /* 接收到的数据 */
    std::string strMsgData(static_cast<const char *>(stMessage.pData));
    /* 发送的信息 */
    std::string strSend;
    std::string strData;

    Json::Object *pJsonRoot = Json::init(strMsgData);
    if (!pJsonRoot)
    {
        dlog_error("参数错误,不是json数据");
        return;
    }

    Json::Object *pJsonData = Json::get(pJsonRoot, "Data");
    std::string data = Json::to_string(pJsonData);
    Json::deinit(pJsonRoot);

    /* TODO 异步处理消息 */
    switch (stMessage.nActionCode)
    {
        /* 系统升级 */
    case AC_DO_UPGRADE:
    {
        dlog_info("系统升级");
        IpcRet_E enRet = IpcRet_E::OK;
        System::UpgradeInfo_S stInfo;
        Convert::to_struct(data, stInfo);
        UpgradeInfo_S stUpgradeInfo;
        memset(&stUpgradeInfo, 0, sizeof(UpgradeInfo_S));
        snprintf(stUpgradeInfo.achPath, sizeof(stUpgradeInfo.achPath), "%s", stInfo.strUpgradePath.c_str());
        enRet = upgrade_start(stUpgradeInfo);
        /* 回复接收成功 */
        fill_returnHead(strSend, strData, AC_DO_UPGRADE, enRet);
        send(strSend, AC_DO_UPGRADE);
        break;
    }
    /* 2 获取升级状态 */
    case AC_GET_UPGRADE_STATUS:
    {
        System::UpgradeStatus_S stStatus;
        upgrade_getStatus(stStatus.nUpgradeStatus);
        strData = Convert::to_string(stStatus);
        /* 返回升级状态 */
        fill_returnHead(strSend, strData, AC_GET_UPGRADE_STATUS, IpcRet_E::OK);
        send(strSend, AC_GET_UPGRADE_STATUS);
        break;
    }
    default:
        break;
    }
}

/* 填充json返回头数据 */
void UpgradeServer::fill_returnHead(
    std::string &strSend,
    std::string strData,
    int nActionCode,
    int nRetCode)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "NVR");
    if (strData.size() > 0)
    {
        Json::Object *pJsonData = Json::init(strData);
        Json::add(pJsonRoot, "Data", pJsonData);
    }
    Json::add(pJsonRoot, "Return", nRetCode);

    strSend = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
    return;
}

void UpgradeServer::upgrade_getStatus(int &nStatus)
{
    TiUpgradeRuslut_E enStatus = TI_UPGRADE_NULL;
    dataManage_get_upgradeStatus(&enStatus);
    nStatus = (int)enStatus;
}
