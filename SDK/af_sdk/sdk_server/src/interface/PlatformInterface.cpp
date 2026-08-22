/**
 * @file PlatformInterface.cpp
 * @author Codex
 * @date 2026-08-22
 * @brief Exposes the SDK platform runtime through the stable server C ABI.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */

#include "NetTVSDKServerInterface.h"

#include "PlatformModule.h"

/**
 * @brief Registers host callbacks used by the SDK platform runtime.
 * @author Codex
 * @param [IN] pCallbacks Host callback table.
 * @return TRUE when callbacks are accepted, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverRegisterPlatformHostCallbacks(
    const NET_PlatformHostCallbacks_S *pCallbacks)
{
    return CPlatformModule::Instance().RegisterHostCallbacks(pCallbacks) ? TRUE : FALSE;
}

/**
 * @brief Applies HTTP, MQTT, crypto and transfer configuration.
 * @author Codex
 * @param [IN] pConfig Platform configuration.
 * @return TRUE when configuration is accepted, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformApplyConfig(
    const NET_PlatformConfig_S *pConfig)
{
    return CPlatformModule::Instance().ApplyConfig(pConfig) ? TRUE : FALSE;
}

/**
 * @brief Starts platform communication workers.
 * @author Codex
 * @return TRUE when startup succeeds or the module is disabled, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformStart(void)
{
    return CPlatformModule::Instance().Start() ? TRUE : FALSE;
}

/**
 * @brief Stops platform communication workers.
 * @author Codex
 * @return TRUE after all workers are stopped.
 */
NET_API BOOL STDCALL NET_serverPlatformStop(void)
{
    return CPlatformModule::Instance().Stop() ? TRUE : FALSE;
}

/**
 * @brief Notifies the runtime that the active network route changed.
 * @author Codex
 * @return TRUE when the reconnect request is accepted, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformNotifyNetworkChanged(void)
{
    return CPlatformModule::Instance().NotifyNetworkChanged() ? TRUE : FALSE;
}

/**
 * @brief Publishes one platform event and optionally queues its image.
 * @author Codex
 * @param [IN] pEvent Event request copied by the runtime.
 * @return TRUE when the event is accepted, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformReportEvent(
    const NET_PlatformEventReport_S *pEvent)
{
    return CPlatformModule::Instance().ReportEvent(pEvent) ? TRUE : FALSE;
}

/**
 * @brief Downloads one platform image to a local file.
 * @author Codex
 * @param [IN] pUrl Absolute or platform-relative URL.
 * @param [IN] pLocalPath Destination path.
 * @param [IN] llExpectedSize Expected byte size, or zero to skip validation.
 * @return TRUE when the file is installed, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformDownloadImage(
    const CHAR *pUrl,
    const CHAR *pLocalPath,
    INT64 llExpectedSize)
{
    return CPlatformModule::Instance().DownloadImage(
               pUrl,
               pLocalPath,
               static_cast<std::int64_t>(llExpectedSize))
               ? TRUE
               : FALSE;
}

/**
 * @brief Reads one consistent platform runtime status snapshot.
 * @author Codex
 * @param [OUT] pStatus Caller-owned status structure.
 * @return TRUE when status is copied, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformGetStatus(
    NET_PlatformRuntimeStatus_S *pStatus)
{
    return CPlatformModule::Instance().GetStatus(pStatus) ? TRUE : FALSE;
}
