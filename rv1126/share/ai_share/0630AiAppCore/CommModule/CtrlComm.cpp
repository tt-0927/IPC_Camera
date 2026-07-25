#include "CtrlComm.hpp"

#include "CommShareTcp.hpp"
#include "ConvertInterface.h"
#include "ConvertJson.hpp"
#include "DataComm.hpp"
#include "DevComm.hpp"
#include "DevManageJson.hpp"
#include "edukit_network.h"
#include "edukit_port.h"
#include "ini_disposed.h"
#include "Intern.hpp"
#include "share_device.h"

using namespace Ai0630_NS;

#ifndef AI_SERVER_INI
    #define AI_SERVER_INI ("/opt/bl/.config/design_data/aiServer.ini")
#endif

CtrlComm::CtrlComm()
{
    std::string strIp;
    m_stAiServerInfo.clear();

    CIni ini(AI_SERVER_INI);
    ini.read("SERVER", "IP", strIp);
    /* 初始化TCP通讯 */
    init(strIp);
}

CtrlComm::~CtrlComm()
{
    if (m_pComm)
    {
        delete m_pComm;
        m_pComm = nullptr;
    }
}

/* 初始化TCP通讯 */
void CtrlComm::init(std::string strIp)
{
    if (m_stAiServerInfo.strIp == strIp)
    {
        dlog(LOG_INFO, "【控制通讯】 服务器IP地址没有变化 不用操作");
        return;
    }

    if (nullptr != m_pComm)
    {
        dlog(LOG_INFO, "【控制通讯】 服务器已被初始化, 进行反初始化");
        delete m_pComm;
        m_pComm = nullptr;
    }

    m_stAiServerInfo.strIp = strIp;
    CIni ini(AI_SERVER_INI);
    ini.write("SERVER", "IP", strIp);

    if (strIp.empty())
    {
        return;
    }

    m_stParam.stNeedParam.enType      = COMM_NS::COMM_SHARE_TCP;
    m_stParam.stNeedParam.strIP       = strIp;
    m_stParam.stNeedParam.nPort       = C_CONTROL_AI;
    m_stParam.stNeedParam.bServerMode = false;

    m_stParam.stExParam.bAutoReconnect     = true;
    m_stParam.stExParam.nReconnectCount    = 0;
    m_stParam.stExParam.nReconnectInterval = 1000;

    m_stParam.stExParam.dataCallback = std::bind(
        &CtrlComm::dataCallback,
        this,
        std::placeholders::_1);
    m_stParam.stExParam.statusCallback = std::bind(
        &CtrlComm::statusCallback,
        this,
        std::placeholders::_1);

    m_pComm = new COMM_NS::CCommShareTcp(m_stParam);
    if (m_pComm)
    {
        dlog(LOG_INFO, "【控制通讯】 服务器初始化成功");
    }
    else
    {
        dlog(LOG_ERROR, "【控制通讯】 服务器初始化失败");
    }
}

/* 发送数据 */
BlError_E CtrlComm::send(CommInfo_S stInfo, void* pHandle)
{
    if (!m_pComm)
    {
        dlog(LOG_ERROR, "【设备TCP通讯】 未初始化");
        return ERR_UNINIT;
    }

    std::string strSendData;
    strSendData = to_string(stInfo);

    COMM_NS::SendDataInfo_S stSendInfo;

    int nLen             = strSendData.size();
    stSendInfo.nDataSize = nLen;
    stSendInfo.pDate     = strSendData.data();
    stSendInfo.nCode     = stInfo.nCode;

    dlog(LOG_INFO, "【设备TCP通讯】 发送内容：\n%s", stSendInfo.pDate);
    BlError_E enRetCode = m_pComm->send(stSendInfo, pHandle);
    if (enRetCode < 0)
    {
        dlog(LOG_ERROR, "【设备TCP通讯】 发送失败");
    }

    return enRetCode;
}

/* 获取设备信息 */
BlError_E Ai0630_NS::CtrlComm::get_device_info(DevDataInfo_S& stDevInfo)
{
    int  nRet       = 0;
    char achIp[32]  = { 0 };
    char achMac[64] = { 0 };
    if (nRet < 0)
    {
        dlog(LOG_ERROR, "获取设备型号失败");
        return NOK;
    }
    nRet = ReachGetIPaddrstring(ETH0_INTERFACE, achIp);
    if (nRet < 0)
    {
        dlog(LOG_ERROR, "获取设备Ip失败");
        return NOK;
    }
    nRet = get_mac(achMac, sizeof(achMac));
    if (nRet < 0)
    {
        dlog(LOG_ERROR, "获取设备Mac失败");
        return NOK;
    }

    Device_Info_t stModelInfo;
    nRet = share_get_deviceInfo(&stModelInfo);
    if (nRet >= 0)
    {
        stDevInfo.strDevName  = stModelInfo.device_name;
        stDevInfo.strDevModel = stModelInfo.device_name;
    }

    stDevInfo.strIP  = achIp;
    stDevInfo.strMac = achMac;

    return OK;
}

/* 回调函数-数据返回 */
BlError_E CtrlComm::dataCallback(COMM_NS::DataParam_S stInfo)
{
    if (nullptr == stInfo.pchMessege ||
        nullptr == stInfo.pHandle)
    {
        dlog(LOG_ERROR, "【控制通讯】 传入参数异常");
        return ERR_PARAM;
    }

    if (stInfo.nCode == toInt(CommCode_E::HEARTBEAT_STATUS))
    {
        /*心跳跳过*/
        return OK;
    }

    dlog(LOG_INFO, "【控制通讯】 接收到的数据 = %s", stInfo.pchMessege);

    BlError_E enRetCode = OK;

    CommInfo_S stCommInfo;
    to_struct(stInfo.pchMessege, stCommInfo);

    std::string strOutJson;

    switch (stCommInfo.nCode)
    {
        /* 收到设备发送过来的设备信息 */
        case toInt(CommCode_E::AI_GET_DEV_INFO):
        {
            DevDataInfo_S stOutDevInfo;
            stOutDevInfo.clear();
            enRetCode = get_device_info(stOutDevInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "【控制通讯】 设备信息读取失败");
                break;
            }

            strOutJson = to_string(stOutDevInfo);

            break;
        }
        /* 收到发送过来的删除设备命令 */
        case toInt(CommCode_E::AI_DELETE_DEV):
        {
            /* 清空服务器信息 */
            m_stAiServerInfo.clear();
            CIni ini(AI_SERVER_INI);
            ini.write("SERVER", "IP", m_stAiServerInfo.strIp);
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "【控制通讯】 未定义处理该操作类型 [%d]", stCommInfo.nCode);
            enRetCode = ERR_CMD_OPT;
            break;
        }
    }

    if (enRetCode != OK_NO_RETURN)
    {
        /* 返回 */
        stCommInfo.strData = strOutJson;
        stCommInfo.nReturn = enRetCode;
        send(stCommInfo, stInfo.pHandle);
    }

    return OK;
}

/* 回调函数-链接状态 */
BlError_E Ai0630_NS::CtrlComm::statusCallback(COMM_NS::StatusParam_S stInfo)
{
    if (stInfo.enType == COMM_NS::SUCCESS)
    {
        m_stAiServerInfo.nNetStatus = 1;
        dlog(LOG_INFO, "【控制通讯】 回调函数-链接状态 [连接成功]");
    }
    else
    {
        m_stAiServerInfo.nNetStatus = 0;
        dlog(LOG_ERROR, "【控制通讯】 回调函数-链接状态 [连接失败]");
    }
    return OK;
}
