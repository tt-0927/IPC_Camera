/**
 * @file operation_communicate.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-05-14
 *
 * @brief 运维平台通讯
 */
#include "dlog.h"
#include "common_define.h"
#include "path_define.h"
#include "action_code.h"
#include "operation_communicate.h"
#include "system_convert.h"
#include "convert_interface.h"

#include "TCPServer.h"
#include "UDSServer.h"

extern "C"
{
#include "http_communicate.h"
}

int OperationServer::init()
{
    Net::Param_S stParam;
    /* 设置心跳码、状态码 */
    stParam.stInitParam.nHearbeatCode = AC_HEARTBIT;
    stParam.stInitParam.nStatusCode = AC_STATUS;
    /* 设置命令码回调函数 */
    using namespace std::placeholders;
    stParam.stInitParam.fnDefaultCallback = std::bind(&OperationServer::deal_message, this, _1, _2);
    stParam.stInitParam.callbackMap[stParam.stInitParam.nHearbeatCode] = std::bind(&OperationServer::deal_heartbeat, this, _1, _2);
    ;
    stParam.stInitParam.callbackMap[stParam.stInitParam.nStatusCode] = std::bind(&OperationServer::deal_status, this, _1, _2);
    ;
    /* 设置ip、端口号 */
    stParam.stInitParam.ip = std::string("127.0.0.1");
    stParam.stInitParam.nPort = IN_CONTROL_OPERATION_PROT;
    /* 创建客户端 */
    m_pHandler = std::make_shared<Net::UDSServer>(stParam);
    /* mqtt配置信息初始化 */
    mqtt_load_config();
    /* 创建mqtt句柄 */
    System::LogServerInfo_S stLogServerInfo;
    Convert::read_file(LOG_SERVER_INFO_FILE, stLogServerInfo);
    mqtt_init(stLogServerInfo);
    /* 运维平台信息初始化 */
    init_maintenace();
    return 0;
}

void OperationServer::deinit()
{
}

int OperationServer::send(
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

void OperationServer::deal_status(
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

void OperationServer::deal_heartbeat(Net::Message_S &stMessage, Net::UserParam_S &stUserParam)
{
    // dlog_debug("接收到心跳消息：%s", stMessage.pData);
}

void OperationServer::deal_message(
    Net::Message_S &stMessage,
    Net::UserParam_S &stUserParam)
{
    std::string data;
    /* 发送的信息 */
    std::string strSend;
    std::string strData;
    if (stMessage.nActionCode != AC_UPLOAD_OPERATION_LOG)
    {
        dlog_debug("接收到[%d]消息：%s", stMessage.nActionCode, stMessage.pData);

        /* 接收到的数据 */
        std::string strMsgData(static_cast<const char *>(stMessage.pData));

        Json::Object *pJsonRoot = Json::init(strMsgData);
        if (!pJsonRoot)
        {
            dlog_error("参数错误,不是json数据");
            return;
        }

        Json::Object *pJsonData = Json::get(pJsonRoot, "Data");
        data = Json::to_string(pJsonData);
        Json::deinit(pJsonRoot);
    }

    /* TODO 异步处理消息 */
    switch (stMessage.nActionCode)
    {
    /* 测试服务器 */
    case AC_TEST_LOG_SERVER:
    {
        int nRet;
        System::LogServerInfo_S stLogServerInfo;
        Convert::to_struct(data, stLogServerInfo);
        /* 直接上抛到指定Topic */
        dlog_debug("测试运维服务器");
        nRet = mqtt_test(stLogServerInfo);
        fill_returnHead(strSend, AC_TEST_LOG_SERVER, nRet);
        send(strSend, AC_TEST_LOG_SERVER);
        break;
    }
    case AC_SET_LOG_SERVER:
    {
        int nRet;
        System::LogServerInfo_S stLogServerInfo;
        Convert::to_struct(data, stLogServerInfo);
        dlog_debug("设置运维服务器信息");
        mqtt_init(stLogServerInfo);
        break;
    }
    case AC_UPLOAD_OPERATION_LOG:
    {

        MqttMsg_S *stMsg = reinterpret_cast<MqttMsg_S *>(const_cast<void *>(stMessage.pData));
        const char *pActualData = reinterpret_cast<const char *>(stMsg) + sizeof(MqttMsg_S);
        dlog_debug("日志上报运维平台 内容[%s] 长度[%d]", pActualData, stMsg->nLen);
        stMsg->pMsg = new char[stMsg->nLen];
        memcpy(stMsg->pMsg, pActualData, stMsg->nLen);
        mqtt_publish(*stMsg);
        free(stMsg->pMsg);
        break;
    }
    /* 获取最新升级包版本 */
    case AC_CHECK_UPGRADE:
    {
        dlog_debug("获取运维平台升级包版本");
        int nId = 0, nType = 0;
        int nPageId = 1;
        char achVersion[128];
        char achUrl[128];
        char pKeyWord[128] = MQTT_DEVICE_NAME;
        System::UpgradeVersion_S stUpgradeVersion;

        UpgradeDataItem_S *pUpgradeData = NULL;
        size_t itemCount = 0;
        // 动态分配空间
        itemCount = 10;
        pUpgradeData = (UpgradeDataItem_S *)malloc(itemCount * sizeof(UpgradeDataItem_S));
        if (pUpgradeData == NULL)
        {
            // 处理分配失败
            dlog_error("Memory allocation failed");
            fill_returnHead(strSend, AC_CHECK_UPGRADE, 0);
            send(strSend, AC_CHECK_UPGRADE);
            break;
        }
        m_reqNormalThread.requeryLogin(MQTT_LOGIN, MQTT_PASSWORD);

        int nDataSize = 1;
        int nRet = get_upgradePackInfoCopy(MQTT_LOGIN, MQTT_PASSWORD, pKeyWord, nType, nPageId, pUpgradeData, &nDataSize);
        dlog_info("nDataSize:%d", nDataSize);
        if (nRet == 0)
        {
            get_latestVersion(pUpgradeData, nDataSize, achVersion, &m_upgradeId, achUrl);
            dlog_debug("获取到最新版本号:%s 对应id:%d 下载链接:%s", achVersion, m_upgradeId, achUrl);
            stUpgradeVersion.strVersion = achVersion;
            m_newPackurl = achUrl;
            free(pUpgradeData);
            strSend = Convert::to_string(stUpgradeVersion);
        }

        fill_returnHead(strSend, AC_CHECK_UPGRADE, nRet);
        send(strSend, AC_CHECK_UPGRADE);
        break;
    }
    /* 下载升级包 */
    case AC_DOWNLOAD_GRADEPACK:
    {
        dlog_debug("下载升级包");
        char achFile[128];
        ::System::UpgradeInfo_S stUpgradeInfo;
        int nRet;
        nRet = get_upgradePack(MQTT_LOGIN, MQTT_PASSWORD, m_upgradeId, UPLOAD_PATH, achFile);

        if (nRet == 0)
        {
            stUpgradeInfo.strUpgradePath = achFile;
            strSend = Convert::to_string(stUpgradeInfo);
        }

        fill_returnHead(strSend, AC_DOWNLOAD_GRADEPACK, nRet);
        send(strSend, AC_DOWNLOAD_GRADEPACK);
        break;
    }
    default:
        break;
    }
}

void OperationServer::fill_head(std::string &data, int nActionCode)
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
}

void OperationServer::fill_returnHead(std::string &data, int nActionCode, int nRet)
{
    Json::Object *pJsonRoot = Json::init();
    Json::add(pJsonRoot, "ActionCode", nActionCode);
    Json::add(pJsonRoot, "DeviceName", "");
    Json::add(pJsonRoot, "UserName", "");
    Json::add(pJsonRoot, "Return", nRet);

    Json::Object *pJsonData = Json::init(data);
    if (pJsonData)
    {
        Json::add(pJsonRoot, "Data", pJsonData);
    }

    data = Json::to_string(pJsonRoot);
    Json::deinit(pJsonRoot);
}

void OperationServer::init_maintenace()
{
    /* 初始化配置 */
    if (!MaintenanceNS::CMaintenanceData::getInstance()->isInit())
    {
        MaintenanceNS::CMaintenanceData::getInstance()->init_config();
    }

    /* 开始检查文件线程 */
    // if(!m_checkFileThread.isInit())
    //{
    //     m_checkFileThread.init();
    // }
    // if(!m_checkFileThread.isRuning())
    //{
    //     m_checkFileThread.start();
    // }

    /* 开始请求线程 */
    if (!m_reqNormalThread.isInit())
    {
        if (m_reqNormalThread.init())
        {
            std::string strUser = MQTT_LOGIN;
            std::string strPwd = MQTT_PASSWORD;
            dlog_debug("用户名:%s 密码:%s", strUser.c_str(), strPwd.c_str());
            /* 开始请求登录 */
            m_reqNormalThread.requeryLogin(strUser, strPwd);
        }
    }
    if (!m_reqNormalThread.isRuning())
    {
        m_reqNormalThread.start();
    }

    /* 开始上传请求线程 */
    // if(!m_uploadThread.isInit())
    //{
    //     m_uploadThread.init();
    // }
    // if(!m_uploadThread.isRuning())
    //{
    //     m_uploadThread.start();
    // }
}

/* 外部手动请求登录 */
void OperationServer::login_maintemamce_communicate()
{
    if (!m_reqNormalThread.isInit())
    {
        if (m_reqNormalThread.init())
        {
            std::string strUser = MQTT_LOGIN;
            std::string strPwd = MQTT_PASSWORD;
            /* 开始请求登录 */
            m_reqNormalThread.requeryLogin(strUser, strPwd);
        }
    }
}
