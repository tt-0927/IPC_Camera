/**
 * @file NetTVSDKServerImpl.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVSDKServerImpl 模块实现
 * 功能说明：
 * 1. 实现 NetTVSDKServerImpl 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include <cstring>
#include <cstdint>
#include <limits>
#include "NetTVSDKServerImpl.h"
#include "modules/ServerModule.h"
#include "modules/SessionModule.h"
#include "modules/RouteModule.h"
#include "modules/VisualSecurity/AlarmModule.h"
#include "DiscoveryResponder.h"
#include "NetSdkLog.h"

#define NETSDK_MAKE_VERSION(major, minor, rev1, rev2) \
    ((uint32_t)( \
        ((major & 0xFF) << 24) | \
        ((minor & 0xFF) << 16) | \
        ((rev1 & 0xFF) << 8) | \
        (rev2 & 0xFF) \
    ))

#define NETSDK_VERSION  NETSDK_MAKE_VERSION(1, 0, 0, 0)

CNetTVSDKServerImpl::CNetTVSDKServerImpl()
    : m_bInitialized(false)
{
    NETSDK_LOG_MESSAGE_DEBUG("CNetTVSDKServerImpl: Creating all modules...");

    /* 创建所有模块 */
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoInit 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

BOOL CNetTVSDKServerImpl::DoInit(UINT32 udwPort,
                                CHAR szUserName[NET_TV_LEN_132],
                                CHAR szPassword[NET_TV_LEN_132])
{
    if (m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_WARN("SDK Server already initialized");
        return NET_TV_FALSE;
    }

    NETSDK_LOG_MESSAGE_INFO("=== SDK Server Initialization Started ===");

    /* 1. 设置鉴权信息 */
    std::string username = (strlen(szUserName) > 0) ? szUserName : "admin";
    std::string password = (strlen(szPassword) > 0) ? szPassword : "admin@123";

    if (!m_pSessionModule->SetAuthInfo("NetTVSDK", username, password))
    {
        NETSDK_LOG_MESSAGE_ERROR("Failed to set authentication info");
        return NET_TV_FALSE;
    }

    /* 2. 注册路由 */
    if (!m_pRouteModule->RegisterAllRoutes())
    {
        NETSDK_LOG_MESSAGE_ERROR("Failed to register routes");
        return NET_TV_FALSE;
    }

    /* 3. 启动HTTP服务器 */
    if (!m_pServerModule->Start(udwPort))
    {
        NETSDK_LOG_MESSAGE_ERROR("Failed to start HTTP server");
        m_pRouteModule->ClearRoutes();
        return NET_TV_FALSE;
    }

    m_bInitialized = true;
    NETSDK_LOG_MESSAGE_INFO("=== SDK Server Initialized Successfully (Port: %u) ===", udwPort);
    return NET_TV_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoCleanup 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

BOOL CNetTVSDKServerImpl::DoCleanup()
{
    if (!m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_DEBUG("SDK Server not initialized, skip cleanup");
        return NET_TV_TRUE;
    }

    NETSDK_LOG_MESSAGE_INFO("=== SDK Server Cleanup Started ===");

    /* 按相反顺序清理模块 */
    /* 1. 停止HTTP服务器（阻止新请求） */
    m_pServerModule->Stop();

    /* 2. 清理会话 */
    m_pSessionModule->Cleanup();

    /* 3. 清理路由和业务单例 */
    m_pRouteModule->ClearRoutes();

    m_bInitialized = false;
    NETSDK_LOG_MESSAGE_INFO("=== SDK Server Cleanup Completed ===");
    return NET_TV_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoSetLogToFile 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

BOOL CNetTVSDKServerImpl::DoSetLogToFile(INT32 dwLogLevel, CHAR* strLogDir,
                                        INT32 dwLogFileSize, INT32 dwLogFileNum)
{
    if (strLogDir == NULL)
    {
        return NET_TV_FALSE;
    }

    /* 构造完整日志路径 */
   char szLogPath[512] = {0};
#ifdef _WIN32
    sprintf(szLogPath, "%s\\NetTVSDKServer.log", strLogDir);
#else
    sprintf(szLogPath, "%s/NetTVSDKServer.log", strLogDir);
#endif

    if (dwLogFileSize <= 0)
    {
        dwLogFileSize = 5 * 1024 * 1024; /* Default 5MB */
    }

    if (dwLogFileNum <= 0)
    {
        dwLogFileNum = 10; /* Default 10 files */
    }

    /* 初始化日志 */
    if (initSdkLogBySize("NetTVSDKServer", szLogPath, dwLogFileSize, dwLogFileNum) != 0)
    {
        return NET_TV_FALSE;
    }

    /* 设置日志输出同步输出控制台 */
    syncPrintf(true);

    /* 设置日志等级 */
    setLogLevel(dwLogLevel);

    return NET_TV_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoGetSDKVersion 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

INT32 CNetTVSDKServerImpl::DoGetSDKVersion()
{
    return NETSDK_VERSION;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoGetClientCount 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoSetUserPasswd 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

BOOL CNetTVSDKServerImpl::DoSetUserPasswd(CHAR szUserName[NET_TV_LEN_132],
                                         CHAR szPassword[NET_TV_LEN_132])
{
    if (!strlen(szUserName) || !strlen(szPassword))
    {
        NETSDK_LOG_MESSAGE_ERROR("SetUserPasswd: Invalid parameters (empty string)");
        return NET_TV_FALSE;
    }

    return m_pSessionModule->UpdatePassword(szUserName, szPassword);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoPushAlarmInfo 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

BOOL CNetTVSDKServerImpl::DoPushAlarmInfo(NET_Alarmer_S* pAlarmer,
                                         INT32 lCommand,
                                         LPVOID pAlarmInfo,
                                         INT32 dwBufLen)
{
    if (!m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_ERROR("PushAlarmInfo: SDK Server not initialized");
        return NET_TV_FALSE;
    }

    return m_pAlarmModule->PushAlarmInfo(pAlarmer, lCommand, pAlarmInfo, dwBufLen);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoPushChannelStatusInfo 定义的内部处理。
 * @param [in,out] pChannelInfo 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

BOOL CNetTVSDKServerImpl::DoPushChannelStatusInfo(NET_ChannelInfo_S* pChannelInfo)
{
    if (!m_bInitialized)
    {
        NETSDK_LOG_MESSAGE_ERROR("PushChannelStatusInfo: SDK Server not initialized");
        return NET_TV_FALSE;
    }

    return m_pAlarmModule->PushChannelStatusInfo(pChannelInfo);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoRegisterCb_GetDiscoveryDeviceInfo 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

BOOL CNetTVSDKServerImpl::DoRegisterCb_GetDiscoveryDeviceInfo(
    NET_TV_CB_GetDiscoveryDeviceInfo cbFunc)
{
    m_fnDiscoveryDeviceInfoCallback = cbFunc;
    return (cbFunc != nullptr) ? NET_TV_TRUE : NET_TV_FALSE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoDiscoveryStart 定义的内部处理。
 * @param [in] szInterfaceName 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

BOOL CNetTVSDKServerImpl::DoDiscoveryStart(const CHAR* szInterfaceName)
{
    if (!m_fnDiscoveryDeviceInfoCallback) {
        NETSDK_LOG_MESSAGE_ERROR("DiscoveryStart: callback not registered");
        return NET_TV_FALSE;
    }
    if (m_pDiscoveryResponder && m_pDiscoveryResponder->is_running()) {
        return NET_TV_TRUE;  /* already running */
    }

    m_pDiscoveryResponder = std::make_unique<CDiscoveryResponder>();

    /* 注册回调：C 回调 → C++ lambda */
    NET_TV_CB_GetDiscoveryDeviceInfo cb = m_fnDiscoveryDeviceInfoCallback;
    m_pDiscoveryResponder->set_device_info_callback(
        [cb](NET_DiscoveryDeviceInfo_S* pInfo) {
            if (cb) cb(pInfo);
        });

    if (m_pDiscoveryResponder->init(szInterfaceName) < 0) {
        NETSDK_LOG_MESSAGE_ERROR("DiscoveryStart: init failed for iface[%s]", szInterfaceName);
        m_pDiscoveryResponder.reset();
        return NET_TV_FALSE;
    }

    if (m_pDiscoveryResponder->start() < 0) {
        NETSDK_LOG_MESSAGE_ERROR("DiscoveryStart: start failed");
        m_pDiscoveryResponder.reset();
        return NET_TV_FALSE;
    }

    NETSDK_LOG_MESSAGE_INFO("Discovery started on iface[%s]", szInterfaceName);
    return NET_TV_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 DoDiscoveryStop 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

BOOL CNetTVSDKServerImpl::DoDiscoveryStop()
{
    if (m_pDiscoveryResponder) {
        m_pDiscoveryResponder->stop();
        m_pDiscoveryResponder.reset();
        NETSDK_LOG_MESSAGE_INFO("Discovery stopped");
    }
    return NET_TV_TRUE;
}
