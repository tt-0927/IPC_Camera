#include <cstring>
#include <cstdint>
#include <limits>
#include "NetTVSDKServerImpl.h"
#include "modules/Common/ServerModule.h"
#include "modules/Common/SessionModule.h"
#include "modules/Common/RouteModule.h"
#include "modules/BG6_ZHSJ/AlarmModule.h"
#include "BG6_ZHSJ/DiscoveryResponder.h"
#include "NetSdkLog.h"

#define NETTVSDK_MAKE_VERSION(major, minor, rev1, rev2) \
    ((uint32_t)( \
        ((major & 0xFF) << 24) | \
        ((minor & 0xFF) << 16) | \
        ((rev1 & 0xFF) << 8) | \
        (rev2 & 0xFF) \
    ))

#define NETTVSDK_VERSION  NETTVSDK_MAKE_VERSION(1, 0, 0, 0)

CNetTVSDKServerImpl::CNetTVSDKServerImpl()
    : m_bInitialized(false)
{
    NETSDK_LOG_MESSAGE_DEBUG("CNetTVSDKServerImpl: Creating all modules...");

    // 创建所有模块
    m_pServerModule = std::make_unique<CServerModule>();
    m_pSessionModule = std::make_unique<CSessionModule>();
    m_pRouteModule = std::make_unique<CRouteModule>();
    m_pAlarmModule = std::make_unique<CAlarmModule>(m_pSessionModule.get());

    NETSDK_LOG_MESSAGE_DEBUG("CNetTVSDKServerImpl: All modules created");
}

CNetTVSDKServerImpl::~CNetTVSDKServerImpl()
{
    if (m_bInitialized)
    {
        DoCleanup();
    }
    NETSDK_LOG_MESSAGE_DEBUG("CNetTVSDKServerImpl: Destroyed");
}

BOOL CNetTVSDKServerImpl::DoInit(UINT32 udwPort,
                                CHAR szUserName[NET_LEN_132],
                                CHAR szPassword[NET_LEN_132])
{
    if (m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_WARN("SDK Server already initialized");
        return FALSE;
    }

    NETSDK_LOG_MESSAGE_INFO("=== SDK Server Initialization Started ===");

    // 1. 设置鉴权信息
    std::string username = (strlen(szUserName) > 0) ? szUserName : "admin";
    std::string password = (strlen(szPassword) > 0) ? szPassword : "admin@123";

    if (!m_pSessionModule->SetAuthInfo("NetTVSDK", username, password))
    {
        NETSDK_LOG_MESSAGE_ERROR("Failed to set authentication info");
        return FALSE;
    }

    // 2. 注册路由
    if (!m_pRouteModule->RegisterAllRoutes())
    {
        NETSDK_LOG_MESSAGE_ERROR("Failed to register routes");
        return FALSE;
    }

    // 3. 启动HTTP服务器
    if (!m_pServerModule->Start(udwPort))
    {
        NETSDK_LOG_MESSAGE_ERROR("Failed to start HTTP server");
        m_pRouteModule->ClearRoutes();
        return FALSE;
    }

    m_bInitialized = true;
    NETSDK_LOG_MESSAGE_INFO("=== SDK Server Initialized Successfully (Port: %u) ===", udwPort);
    return TRUE;
}

BOOL CNetTVSDKServerImpl::DoCleanup()
{
    if (!m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_DEBUG("SDK Server not initialized, skip cleanup");
        return TRUE;
    }

    NETSDK_LOG_MESSAGE_INFO("=== SDK Server Cleanup Started ===");

    // 按相反顺序清理模块
    // 1. 停止HTTP服务器（阻止新请求）
    m_pServerModule->Stop();

    // 2. 清理会话
    m_pSessionModule->Cleanup();

    // 3. 清理路由和业务单例
    m_pRouteModule->ClearRoutes();

    m_bInitialized = false;
    NETSDK_LOG_MESSAGE_INFO("=== SDK Server Cleanup Completed ===");
    return TRUE;
}

BOOL CNetTVSDKServerImpl::DoSetLogToFile(INT32 dwLogLevel, CHAR* strLogDir,
                                        INT32 dwLogFileSize, INT32 dwLogFileNum)
{
    if (strLogDir == NULL)
    {
        return FALSE;
    }

    // 构造完整日志路径
   char szLogPath[512] = {0};
#ifdef _WIN32
    snprintf(szLogPath, sizeof(szLogPath), "%s\\NetTVSDKServer.log", strLogDir);
#else
    snprintf(szLogPath, sizeof(szLogPath), "%s/NetTVSDKServer.log", strLogDir);
#endif

    if (dwLogFileSize <= 0)
    {
        dwLogFileSize = 5 * 1024 * 1024; // Default 5MB
    }

    if (dwLogFileNum <= 0)
    {
        dwLogFileNum = 10; // Default 10 files
    }

    // 初始化日志
    if (initSdkLogBySize("NetTVSDKServer", szLogPath, dwLogFileSize, dwLogFileNum) != 0)
    {
        return FALSE;
    }

    // 设置日志输出同步输出控制台
    syncPrintf(true);

    // 设置日志等级
    setLogLevel(dwLogLevel);

    return TRUE;
}

INT32 CNetTVSDKServerImpl::DoGetSDKVersion()
{
    return NETTVSDK_VERSION;
}

INT32 CNetTVSDKServerImpl::DoGetClientCount()
{
    if (!m_bInitialized)
    {
        return 0;
    }

    size_t sessionCount = m_pSessionModule->GetActiveSessionCount();
    if (sessionCount > static_cast<size_t>(std::numeric_limits<INT32>::max()))
    {
        return std::numeric_limits<INT32>::max();
    }

    return static_cast<INT32>(sessionCount);
}

BOOL CNetTVSDKServerImpl::DoSetUserPasswd(CHAR szUserName[NET_LEN_132],
                                         CHAR szPassword[NET_LEN_132])
{
    if (!strlen(szUserName) || !strlen(szPassword))
    {
        NETSDK_LOG_MESSAGE_ERROR("SetUserPasswd: Invalid parameters (empty string)");
        return FALSE;
    }

    return m_pSessionModule->UpdatePassword(szUserName, szPassword);
}

BOOL CNetTVSDKServerImpl::DoPushAlarmInfo(NET_Alarmer_S* pAlarmer,
                                         INT32 lCommand,
                                         LPVOID pAlarmInfo,
                                         INT32 dwBufLen)
{
    if (!m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_ERROR("PushAlarmInfo: SDK Server not initialized");
        return FALSE;
    }

    return m_pAlarmModule->PushAlarmInfo(pAlarmer, lCommand, pAlarmInfo, dwBufLen);
}

/**
 * @brief 推送动态图片 V2 告警。
 * @param [in] pAlarmer 告警设备信息。
 * @param [in] lCommand 告警命令码。
 * @param [in] pAlarmInfo V2 告警结构体。
 * @param [in] dwBufLen V2 告警结构体长度。
 * @return 成功返回 TRUE，失败返回 FALSE。
 */
BOOL CNetTVSDKServerImpl::DoPushAlarmInfoV2(NET_Alarmer_S* pAlarmer,
                                            INT32 lCommand,
                                            LPVOID pAlarmInfo,
                                            INT32 dwBufLen)
{
    if (!m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_ERROR("PushAlarmInfoV2: SDK server is not initialized");
        return FALSE;
    }

    return m_pAlarmModule->PushAlarmInfoV2(pAlarmer, lCommand, pAlarmInfo, dwBufLen);
}

BOOL CNetTVSDKServerImpl::DoPushChannelStatusInfo(NET_ChannelInfo_S* pChannelInfo)
{
    if (!m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_ERROR("PushChannelStatusInfo: SDK Server not initialized");
        return FALSE;
    }

    return m_pAlarmModule->PushChannelStatusInfo(pChannelInfo);
}

BOOL CNetTVSDKServerImpl::DoRegisterCb_GetDiscoveryDeviceInfo(
    NET_CB_GetDiscoveryDeviceInfo cbFunc)
{
    m_cbDiscoveryDeviceInfo = cbFunc;
    return (cbFunc != nullptr) ? TRUE : FALSE;
}

BOOL CNetTVSDKServerImpl::DoDiscoveryStart(const CHAR* szInterfaceName)
{
    if (!m_cbDiscoveryDeviceInfo) {
        NETSDK_LOG_MESSAGE_ERROR("DiscoveryStart: callback not registered");
        return FALSE;
    }
    if (m_pDiscoveryResponder && m_pDiscoveryResponder->is_running()) {
        return TRUE;  /* already running */
    }

    m_pDiscoveryResponder = std::make_unique<CDiscoveryResponder>();

    /* 注册回调：C 回调 → C++ lambda */
    NET_CB_GetDiscoveryDeviceInfo cb = m_cbDiscoveryDeviceInfo;
    m_pDiscoveryResponder->set_device_info_callback(
        [cb](NET_DiscoveryDeviceInfo_S* pInfo) {
            if (cb) cb(pInfo);
        });

    if (m_pDiscoveryResponder->init(szInterfaceName) < 0) {
        NETSDK_LOG_MESSAGE_ERROR("DiscoveryStart: init failed for iface[%s]", szInterfaceName);
        m_pDiscoveryResponder.reset();
        return FALSE;
    }

    if (m_pDiscoveryResponder->start() < 0) {
        NETSDK_LOG_MESSAGE_ERROR("DiscoveryStart: start failed");
        m_pDiscoveryResponder.reset();
        return FALSE;
    }

    NETSDK_LOG_MESSAGE_INFO("Discovery started on iface[%s]", szInterfaceName);
    return TRUE;
}

BOOL CNetTVSDKServerImpl::DoDiscoveryStop()
{
    if (m_pDiscoveryResponder) {
        m_pDiscoveryResponder->stop();
        m_pDiscoveryResponder.reset();
        NETSDK_LOG_MESSAGE_INFO("Discovery stopped");
    }
    return TRUE;
}
