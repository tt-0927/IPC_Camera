/**
 * @FilePath     : tvsdk_server.cpp
 * @Description  : TVSDK 服务端封装实现，对接 NetTVSDKServer.h C 接口；能力集通过 control_manage 命令码获取
 */

#include "tvsdk_server.h"
#include "dlog.h"
#include "action_code.h"
#include "user_manage.h"
#include "user_define.h"

#include "callbacks/tvsdk_callbacks.h"

#include "NetTVSDKServer.h"
#include "system_manage.h"
#include "network_manage.h"

/* 回调实现/注册已拆分到 callbacks/ 目录 */

/* ---------- CTvSdkServer 实现 ---------- */

int CTvSdkServer::init()
{
    if (m_bInit)
    {
        dlog_warn("TVSDK server already inited");
        return OK;
    }

    UINT32 port = (m_udwPort > 0) ? (UINT32)m_udwPort : (UINT32)IN_CONTROL_SDK_PROT;
    CHAR szUser[NET_TV_LEN_132] = {0};
    CHAR szPass[NET_TV_LEN_132] = {0};
    /* 通过用户管理获取管理员用户名和密码 */
    std::string strAdminUser = USER_DEFAULT_NAME;
    std::string strAdminPass = CUserManage::instance()->get_passwd(USER_DEFAULT_NAME);
    if (strAdminPass.empty() && !m_strPassword.empty())
        strAdminPass = m_strPassword;
    if (!m_strUser.empty())
        strAdminUser = m_strUser;
    strncpy(szUser, strAdminUser.c_str(), sizeof(szUser) - 1);
    szUser[sizeof(szUser) - 1] = '\0';
    strncpy(szPass, strAdminPass.c_str(), sizeof(szPass) - 1);
    szPass[sizeof(szPass) - 1] = '\0';

    BOOL bRet = NET_TV_SERVER_Init(port, szUser, szPass);
    if (!bRet)
    {
        dlog_error("NET_TV_SERVER_Init failed, port=%u", port);
        return -1;
    }

    /* 设置 TaskManage 并注册所有回调 */
    TvSdkCallbacks::set_task_manage(m_pTaskManage);
    TvSdkCallbacks::register_all();

    m_bInit = true;
    dlog_info("TVSDK server init ok, port=%u", port);
    return OK;
}

void CTvSdkServer::deinit()
{
    if (!m_bInit)
        return;
    NET_TV_SERVER_Cleanup();
    TvSdkCallbacks::clear_task_manage();
    m_bInit = false;
    dlog_info("TVSDK server deinit");
}

void CTvSdkServer::set_taskManage(std::shared_ptr<CTaskManage> pTaskManage)
{
    m_pTaskManage = std::move(pTaskManage);
    TvSdkCallbacks::set_task_manage(m_pTaskManage);
}

int CTvSdkServer::push_alarm(const void *pAlarmer, int lCommand, const void *pAlarmInfo, int dwBufLen)
{
    if (!m_bInit || !pAlarmInfo || dwBufLen <= 0)
        return -1;
     // SDK 内部通常要求 pAlarmer 非空，这里按需填默认告警设备信息
    NET_TV_ALARMER_S stAlarmer;
    NET_TV_ALARMER_S *pUseAlarmer = (NET_TV_ALARMER_S *)pAlarmer;
    if (!pUseAlarmer)
    {
        memset(&stAlarmer, 0, sizeof(stAlarmer));

        // device info
        ::System::DeviceInfo_S stDev;
        if (SystemManage::instance()->get_device_info(stDev) == 0)
        {
            strncpy((char *)stAlarmer.szSerialNumber, stDev.serialNumber.c_str(), sizeof(stAlarmer.szSerialNumber) - 1);
            strncpy(stAlarmer.szDeviceName, stDev.deviceName.c_str(), sizeof(stAlarmer.szDeviceName) - 1);
        }

        // ip + mac
        Network::Info_S stNet;
        if (CNetworkManage::instance()->get_ip_and_dns(stNet) == 0)
        {
            strncpy(stAlarmer.szDeviceIP, stNet.stIp.ipv4Ip.c_str(), sizeof(stAlarmer.szDeviceIP) - 1);
        }
        std::string mac = CNetworkManage::instance()->get_macAddress("eth0");
        unsigned int b[6] = {0};
        if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) == 6)
        {
            for (int i = 0; i < 6; ++i)
                stAlarmer.byMacAddr[i] = (BYTE)(b[i] & 0xFF);
        }

        pUseAlarmer = &stAlarmer;
    }
    // if ((lCommand & 0xF000) == NET_TV_ALARM_BASE_STATISTICS)
    // {
    //     dlog_info("TVSDK转发统计告警: cmd[0x%x] buf_len[%d] expect_size[%zu]", lCommand, dwBufLen,
    //               sizeof(NET_TV_ALARM_STATISTICS_INFO_S));
    //     if (dwBufLen >= static_cast<int>(sizeof(NET_TV_ALARM_STATISTICS_INFO_S)))
    //     {
    //         const NET_TV_ALARM_STATISTICS_INFO_S *pStatistics =
    //             static_cast<const NET_TV_ALARM_STATISTICS_INFO_S *>(pAlarmInfo);
    //         dlog_info("TVSDK转发统计内容: alarm_type[0x%x] 通道[%u] 类型[%u] 规则[%u] 时间戳[%lld] 序号[%u] "
    //                   "进入[%u] 离开[%u] 总数[%u] 当前人数[%u] 目标数[%u] 全景图长度[%u]",
    //                   pStatistics->dwAlarmType,
    //                   pStatistics->dwChannel,
    //                   pStatistics->dwStatisticsType,
    //                   pStatistics->dwRuleID,
    //                   static_cast<long long>(pStatistics->llTimestampMs),
    //                   pStatistics->dwReportSeq,
    //                   pStatistics->dwEnterCount,
    //                   pStatistics->dwLeaveCount,
    //                   pStatistics->dwTotalCount,
    //                   pStatistics->dwCurrentPeopleCount,
    //                   pStatistics->dwTargetCount,
    //                   pStatistics->dwPanoramaImgLen);
    //     }
    //     else
    //     {
    //         dlog_warn("TVSDK转发统计告警缓冲区过小: cmd[0x%x] buf_len[%d] expect_size[%zu]", lCommand, dwBufLen,
    //                   sizeof(NET_TV_ALARM_STATISTICS_INFO_S));
    //     }
    // }

    BOOL bRet = NET_TV_SERVER_PushAlarmInfo(
         pUseAlarmer,
        (INT32)lCommand,
        (LPVOID)pAlarmInfo,
        (INT32)dwBufLen);
    return bRet ? OK : -1;
}

int CTvSdkServer::get_client_count() const
{
    if (!m_bInit)
    {
        dlog_error("TVSDK server not inited, get client count failed");
        return -1;
    }

    return (int)NET_TV_SERVER_GetClientCount();
}

void CTvSdkServer::set_port(unsigned int port)
{
    m_udwPort = port;
}

void CTvSdkServer::set_user_passwd(const std::string &user, const std::string &passwd)
{
    m_strUser = user;
    m_strPassword = passwd;
}
