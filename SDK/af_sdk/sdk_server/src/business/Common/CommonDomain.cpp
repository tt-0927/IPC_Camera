/*
 * @FilePath     : sdk_new/sdk_server/src/business/Common/CommonDomain.cpp
 * @Author       : chenchl
 * @Date         : 2026-08-20
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-20
 * @Description  : 通用配置域实现
 *                 注册通用命令码→处理函数映射，实现特殊处理函数。
 */
#include "CommonDomain.h"
#include "DeviceBusiness.h"

CCommonDomain::CCommonDomain()
{
    /* ===== Get 命令注册 ===== */

    /* 设备基本信息（走设备回调，特殊处理） */
    m_getTable[NET_GET_DEVICECFG] = &CCommonDomain::HandleGetDeviceBasicInfo;

    /* 设备存储信息（走设备回调，特殊处理） */
    m_getTable[NET_GET_STORAGE_INFO] = &CCommonDomain::HandleGetStorageInfo;

    /* NTP/网络/安全服务/SSH/音频 */
    m_getTable[NET_GET_NTPCFG]                = &CCommonDomain::TemplatedGet<NET_SystemNtpInfo_S>;
    m_getTable[NET_GET_AUDIOCFG]              = &CCommonDomain::TemplatedGet<NET_AudioCfg_S>;
    m_getTable[NET_GET_NETWORKCFG]            = &CCommonDomain::TemplatedGet<NET_NetworkCfgList_S>;
    m_getTable[NET_GET_SECURITY_SERVICES_INFO]= &CCommonDomain::TemplatedGet<NET_SecurityServicesInfo_S>;
    m_getTable[NET_GET_SSH_COUNTDOWN]         = &CCommonDomain::TemplatedGet<NET_SshCountdownInfo_S>;

    /* 注册信息 */
    m_getTable[NET_GET_REGISTERINFO]          = &CCommonDomain::TemplatedGet<NET_RegisterInfo_S>;

    /* 日志查询（委托 DeviceBusiness，需要 url_param 分页参数） */
    m_getTable[NET_FIND_LOG] = [](INT32 ch, INT32 cmd, const std::string&, const std::string& url) -> std::string {
        return CDeviceBusiness::instance()->HandleGetLogList(ch, cmd, url);
    };
    m_getTable[NET_EXPORT_LOG] = [](INT32 ch, INT32 cmd, const std::string&, const std::string& url) -> std::string {
        return CDeviceBusiness::instance()->HandleGetLogList(ch, cmd, url);
    };

    /* 日志服务器 */
    m_getTable[NET_GET_LOG_SERVER]            = &CCommonDomain::TemplatedGet<NET_LogServerInfo_S>;

    /* 升级状态/版本（委托 DeviceBusiness） */
    m_getTable[NET_GET_UPGRADESTATUS] = [](INT32 ch, INT32 cmd, const std::string&, const std::string&) -> std::string {
        return CDeviceBusiness::instance()->HandleGetUpgradeStatus(ch, cmd);
    };
    m_getTable[NET_GET_UPGRADEVERSION] = [](INT32 ch, INT32 cmd, const std::string&, const std::string&) -> std::string {
        return CDeviceBusiness::instance()->HandleGetUpgradeVersion(ch, cmd);
    };

    /* ===== Set 命令注册 ===== */

    /* 设备基本信息（仅 strDeviceName 可写，走设备回调） */
    m_setTable[NET_SET_DEVICECFG] = &CCommonDomain::HandleSetDeviceBasicInfo;

    /* NTP/网络/安全服务/音频 */
    m_setTable[NET_SET_NTPCFG]                 = &CCommonDomain::TemplatedSet<NET_SystemNtpInfo_S>;
    m_setTable[NET_SET_SYSTEM_TIME]            = &CCommonDomain::TemplatedSet<NET_SystemTime_S>;
    m_setTable[NET_SET_AUDIOCFG]               = &CCommonDomain::TemplatedSet<NET_AudioCfg_S>;
    m_setTable[NET_SET_NETWORKCFG]             = &CCommonDomain::TemplatedSet<NET_NetworkCfgList_S>;
    m_setTable[NET_SET_SECURITY_SERVICES_INFO] = &CCommonDomain::TemplatedSet<NET_SecurityServicesInfo_S>;

    /* 注册信息 */
    m_setTable[NET_SET_REGISTERINFO]           = &CCommonDomain::TemplatedSet<NET_RegisterInfo_S>;
    

    /* 日志服务器设置/测试 */
    m_setTable[NET_SET_LOG_SERVER]            = &CCommonDomain::TemplatedSet<NET_LogServerInfo_S>;
    m_setTable[NET_TEST_LOG_SERVER]           = &CCommonDomain::TemplatedSet<NET_LogServerInfo_S>;

    /* 升级（委托 DeviceBusiness） */
    m_setTable[NET_SET_UPGRADE] = [](INT32 ch, INT32 cmd, const std::string& req, const std::string&) -> std::string {
        return CDeviceBusiness::instance()->HandleSetUpgrade(ch, cmd, req);
    };
}

/* ===== 特殊处理函数实现 ===== */

std::string CCommonDomain::HandleGetDeviceBasicInfo(INT32 nChannelId, INT32 nCommand,
                                                    const std::string& req_data,
                                                    const std::string& url_param)
{
    (void)req_data;
    (void)url_param;

    NET_DeviceBasicInfo_S stCfg;
    memset(&stCfg, 0, sizeof(NET_DeviceBasicInfo_S));

    NETSDK_LOG_MESSAGE_INFO("GetDeviceBasicInfo callback START");
    int nRespCode = executeGetDeviceBasicInfoCb(&stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetDeviceBasicInfo callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetDeviceBasicInfo callback cmd=%d, ret=%d", nCommand, nRespCode);
    NETSDK_LOG_MESSAGE_INFO("GetDeviceBasicInfo callback END");
    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}

std::string CCommonDomain::HandleSetDeviceBasicInfo(INT32 nChannelId, INT32 nCommand,
                                                    const std::string& req_data,
                                                    const std::string& url_param)
{
    (void)nChannelId;
    (void)url_param;

    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    NET_DeviceBasicInfo_S stCfg;
    memset(&stCfg, 0, sizeof(NET_DeviceBasicInfo_S));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    SDKConvert::deal(pRoot, stCfg, true);
    Json::deinit(pRoot);

    int nRespCode = executeSetDeviceBasicInfoCb(&stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("SetDeviceBasicInfo callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }

    return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
}

std::string CCommonDomain::HandleGetStorageInfo(INT32 nChannelId, INT32 nCommand,
                                                const std::string& req_data,
                                                const std::string& url_param)
{
    (void)req_data;
    (void)url_param;

    NET_DeviceStorageInfo_S stCfg;
    memset(&stCfg, 0, sizeof(NET_DeviceStorageInfo_S));

    NETSDK_LOG_MESSAGE_INFO("GetStorageInfo callback START");
    int nRespCode = executeGetDeviceStorageInfoCb(&stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetStorageInfo callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetStorageInfo callback cmd=%d, ret=%d", nCommand, nRespCode);
    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}
