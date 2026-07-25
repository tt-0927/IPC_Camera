#include "DevComm.hpp"

#include "CommShareTcp.hpp"
#include "ConvertInterface.h"
#include "ConvertJson.hpp"
#include "DevManageJson.hpp"
#include "edukit_network.h"
#include "edukit_port.h"
#include "share_device.h"

using namespace Ai0630_NS;

DevComm::DevComm()
{
    init();
}

DevComm::~DevComm()
{
    disconnect(&sig_setServerIp);
}

/* 初始化 */
BlError_E DevComm::init()
{
    if (m_pTcpComm)
    {
        dlog(LOG_INFO, "【设备TCP通讯】 已初始化, 进行反初始化");
        delete m_pTcpComm;
        m_pTcpComm = nullptr;
    }

    COMM_NS::CommInParam_S stInfo;
    stInfo.clear();
    stInfo.stNeedParam.enType      = COMM_NS::COMM_SHARE_TCP;
    stInfo.stNeedParam.strIP       = LOCAL_IP;
    stInfo.stNeedParam.nPort       = C_CONTROL_AI_DEV;
    stInfo.stNeedParam.bServerMode = true;

    stInfo.stExParam.bAutoReconnect     = true;
    stInfo.stExParam.nReconnectCount    = 0;
    stInfo.stExParam.nReconnectInterval = 1000;

    stInfo.stExParam.dataCallback = std::bind(&DevComm::dataCallback, this, std::placeholders::_1);

    m_pTcpComm = new COMM_NS::CCommShareTcp(stInfo);
    if (!m_pTcpComm)
    {
        dlog(LOG_ERROR, "【设备TCP通讯】 创建TCP通信对象失败");
        return NOK;
    }

    return OK;
}

/* 发送数据 */
BlError_E DevComm::send(CommInfo_S stInfo, void* pHandle)
{
    if (!m_pTcpComm)
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
    BlError_E enRetCode = m_pTcpComm->send(stSendInfo, pHandle);
    if (enRetCode < 0)
    {
        dlog(LOG_ERROR, "【设备TCP通讯】 发送失败");
    }

    return enRetCode;
}

/* 获取设备信息 */
BlError_E DevComm::get_device_info(DevDataInfo_S& stDevInfo)
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

/* 通讯数据回调函数 */
BlError_E DevComm::dataCallback(COMM_NS::DataParam_S stInfo)
{
    if (!stInfo.pchMessege)
    {
        dlog(LOG_ERROR, "【设备TCP通讯】 传入参数异常");
        return ERR_PARAM;
    }

    if (stInfo.nCode == toInt(CommCode_E::HEARTBEAT_STATUS))
    {
        /*心跳跳过*/
        return OK;
    }

    BlError_E enRetCode = OK;
    bool      bRet;
    int       nActionCode;
    int       nOptType = 0;

    std::string strOutJson;

    dlog(LOG_TRACE, "【设备TCP通讯】 接受到数据：%s", stInfo.pchMessege);
    CommInfo_S stCommInfo;
    to_struct(stInfo.pchMessege, stCommInfo);

    switch (stCommInfo.nCode)
    {
        /* 请求本机设备信息 */
        case toInt(CommCode_E::AI_GET_DEV_INFO):
        {
            DevDataInfo_S stOutDevInfo;
            stOutDevInfo.clear();
            enRetCode = get_device_info(stOutDevInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "【设备TCP通讯】 设备信息读取失败");
                break;
            }

            strOutJson = to_string(stOutDevInfo);
            break;
        }
        /* 设置AI服务器IP */
        case toInt(CommCode_E::AI_SET_IP_INFO):
        {
            AddDevInfo_S stAddDevInfo;
            /* 解析 */
            to_struct(stCommInfo.strData, stAddDevInfo);

            /* 通知设置IP */
            sig_setServerIp.emit(stAddDevInfo.strIp);
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "【设备TCP通讯】 未定义处理该操作类型 [%d]", stCommInfo.nCode);
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
