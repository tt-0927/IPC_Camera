/**
 * @file tvsdk_platform_api.h
 * @author Codex
 * @date 2026-08-22
 * @brief Declares the SDK platform communication ABI for the IPC host adapter.
 * @details This compatibility header is active only while the generated TVSDK
 * header does not yet contain NETTVSDK_PLATFORM_ABI_H.
 * @change 2026-08-22 Codex Initial SDK platform migration ABI declaration.
 */
#pragma once

#include "NetTVSDKServer.h"

#ifndef NETTVSDK_PLATFORM_ABI_H
#define NETTVSDK_PLATFORM_ABI_H

#define NET_PLATFORM_ABI_VERSION                 1U
#define NET_PLATFORM_HOST_LENGTH                 256U
#define NET_PLATFORM_ACCOUNT_LENGTH              128U
#define NET_PLATFORM_PASSWORD_LENGTH             256U
#define NET_PLATFORM_PATH_LENGTH                 512U
#define NET_PLATFORM_KEY_ID_LENGTH               64U
#define NET_PLATFORM_REQUEST_ID_LENGTH            128U
#define NET_PLATFORM_COMMAND_LENGTH              128U
#define NET_PLATFORM_COMMAND_RESULT_LENGTH       65536U
#define NET_PLATFORM_ERROR_LENGTH                256U
#define NET_PLATFORM_DEVICE_SN_LENGTH            128U

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @enum tagNETPlatformUplinkType
 * @brief Defines the active IPC uplink network type.
 */
typedef enum tagNETPlatformUplinkType
{
    NET_PLATFORM_UPLINK_UNKNOWN = 0,
    NET_PLATFORM_UPLINK_WIRED = 1,
    NET_PLATFORM_UPLINK_WIRELESS = 2,
    NET_PLATFORM_UPLINK_CELLULAR = 3
} NET_PlatformUplinkType_EN;

/**
 * @enum tagNETPlatformStreamMode
 * @brief Defines the platform stream delivery mode.
 */
typedef enum tagNETPlatformStreamMode
{
    NET_PLATFORM_STREAM_MODE_UNKNOWN = 0,
    NET_PLATFORM_STREAM_MODE_RTSP = 1,
    NET_PLATFORM_STREAM_MODE_RTMP = 2
} NET_PlatformStreamMode_EN;

/**
 * @struct tagNETPlatformConfig
 * @brief Defines platform HTTP, MQTT, crypto and transfer configuration.
 */
typedef struct tagNETPlatformConfig
{
    UINT32 uStructSize;
    UINT32 uVersion;
    BOOL bEnable;
    CHAR strHttpHost[NET_PLATFORM_HOST_LENGTH];
    INT32 nHttpPort;
    CHAR strMqttHost[NET_PLATFORM_HOST_LENGTH];
    INT32 nMqttPort;
    INT32 nRtmpPort;
    CHAR strPlatformUser[NET_PLATFORM_ACCOUNT_LENGTH];
    CHAR strPlatformPassword[NET_PLATFORM_PASSWORD_LENGTH];
    CHAR strMqttUser[NET_PLATFORM_ACCOUNT_LENGTH];
    CHAR strMqttPassword[NET_PLATFORM_PASSWORD_LENGTH];
    CHAR strRegisterPublicKeyPath[NET_PLATFORM_PATH_LENGTH];
    CHAR strRegisterPublicKeyId[NET_PLATFORM_KEY_ID_LENGTH];
    CHAR strMqttRuntimeLibrary[NET_PLATFORM_PATH_LENGTH];
    CHAR strImageDownloadDirectory[NET_PLATFORM_PATH_LENGTH];
    UINT32 uHeartbeatIntervalSec;
    UINT32 uImageWaitTimeoutMs;
    UINT32 uImageWaitIntervalMs;
    BYTE byRes[256];
} NET_PlatformConfig_S, *pNET_PlatformConfig_S;

/**
 * @struct tagNETPlatformDeviceProfile
 * @brief Defines device identity used by HTTP and MQTT registration.
 */
typedef struct tagNETPlatformDeviceProfile
{
    UINT32 uStructSize;
    UINT32 uVersion;
    CHAR strSerialNumber[NET_PLATFORM_DEVICE_SN_LENGTH];
    CHAR strDeviceName[NET_LEN_128];
    CHAR strFirmwareVersion[NET_LEN_128];
    CHAR strMacAddress[NET_LEN_64];
    CHAR strLocalIp[NET_LEN_64];
    INT32 nServicePort;
    CHAR strResolution[NET_LEN_64];
    CHAR strStorage[NET_LEN_64];
    CHAR strUseStorage[NET_LEN_64];
    CHAR strLocation[NET_LEN_128];
    BYTE byRes[128];
} NET_PlatformDeviceProfile_S, *pNET_PlatformDeviceProfile_S;

/**
 * @struct tagNETPlatformStreamProfile
 * @brief Defines current route, RTSP URLs and camera credentials.
 */
typedef struct tagNETPlatformStreamProfile
{
    UINT32 uStructSize;
    UINT32 uVersion;
    INT32 enUplinkType;
    CHAR strUplinkInterface[NET_LEN_64];
    CHAR strLocalIp[NET_LEN_64];
    CHAR strRtspMainUrl[NET_MAX_URL_LEN];
    CHAR strRtspSubUrl[NET_MAX_URL_LEN];
    CHAR strRtspAccount[NET_PLATFORM_ACCOUNT_LENGTH];
    CHAR strRtspPassword[NET_PLATFORM_PASSWORD_LENGTH];
    BYTE byRes[128];
} NET_PlatformStreamProfile_S, *pNET_PlatformStreamProfile_S;

/**
 * @struct tagNETPlatformEventReport
 * @brief Defines one MQTT event and optional event image upload.
 */
typedef struct tagNETPlatformEventReport
{
    UINT32 uStructSize;
    UINT32 uVersion;
    const CHAR *pCommand;
    const CHAR *pRequestId;
    const CHAR *pDataJson;
    INT32 nEventType;
    const CHAR *pEventName;
    INT32 nChannel;
    INT64 llTimestampMs;
    const CHAR *pImagePath;
    BOOL bUploadImage;
    BOOL bResolveImageIfMissing;
    BYTE byRes[128];
} NET_PlatformEventReport_S, *pNET_PlatformEventReport_S;

/**
 * @struct tagNETPlatformRuntimeStatus
 * @brief Defines an SDK platform runtime status snapshot.
 */
typedef struct tagNETPlatformRuntimeStatus
{
    UINT32 uStructSize;
    UINT32 uVersion;
    BOOL bRunning;
    BOOL bHttpAuthenticated;
    BOOL bMqttConnected;
    BOOL bDeviceRegistered;
    INT32 nLastError;
    UINT64 uReconnectCount;
    UINT64 uPublishedEventCount;
    UINT64 uUploadedImageCount;
    UINT64 uDroppedCommandCount;
    CHAR strLastError[NET_PLATFORM_ERROR_LENGTH];
    BYTE byRes[128];
} NET_PlatformRuntimeStatus_S, *pNET_PlatformRuntimeStatus_S;

/**
 * @typedef NET_CB_PlatformGetDeviceProfile
 * @brief Obtains the current device registration profile from the IPC host.
 * @param [in] pUserData Host callback context.
 * @param [out] pProfile Device profile output.
 * @return Zero on success, otherwise an IPC error code.
 */
typedef INT32(STDCALL *NET_CB_PlatformGetDeviceProfile)(
    IN LPVOID pUserData,
    OUT pNET_PlatformDeviceProfile_S pProfile);

/**
 * @typedef NET_CB_PlatformGetStreamProfile
 * @brief Obtains the current route and stream profile from the IPC host.
 * @param [in] pUserData Host callback context.
 * @param [out] pProfile Stream profile output.
 * @return Zero on success, otherwise an IPC error code.
 */
typedef INT32(STDCALL *NET_CB_PlatformGetStreamProfile)(
    IN LPVOID pUserData,
    OUT pNET_PlatformStreamProfile_S pProfile);

/**
 * @typedef NET_CB_PlatformApplyRtmpStream
 * @brief Starts, replaces or stops IPC RTMP publishing.
 * @param [in] pUserData Host callback context.
 * @param [in] pRtmpUrl SDK-selected RTMP URL.
 * @param [in] bEnable TRUE starts publishing and FALSE stops publishing.
 * @return Zero on success, otherwise an IPC error code.
 */
typedef INT32(STDCALL *NET_CB_PlatformApplyRtmpStream)(
    IN LPVOID pUserData,
    IN const CHAR *pRtmpUrl,
    IN BOOL bEnable);

/**
 * @typedef NET_CB_PlatformResolveEventImage
 * @brief Resolves a delayed IPC event image path for the SDK transfer worker.
 * @param [in] pUserData Host callback context.
 * @param [in] pEvent Event metadata.
 * @param [out] pImagePath Image path output buffer.
 * @param [in] uImagePathSize Output buffer capacity.
 * @return Zero when a readable image is available, otherwise an IPC error code.
 */
typedef INT32(STDCALL *NET_CB_PlatformResolveEventImage)(
    IN LPVOID pUserData,
    IN const NET_PlatformEventReport_S *pEvent,
    OUT CHAR *pImagePath,
    IN UINT32 uImagePathSize);

/**
 * @typedef NET_CB_PlatformRuntimeStatus
 * @brief Receives significant SDK platform runtime state changes.
 * @param [in] pUserData Host callback context.
 * @param [in] pStatus Runtime status snapshot.
 * @return No return value.
 */
typedef VOID(STDCALL *NET_CB_PlatformRuntimeStatus)(
    IN LPVOID pUserData,
    IN const NET_PlatformRuntimeStatus_S *pStatus);

/**
 * @typedef NET_CB_PlatformExecuteCommand
 * @brief Executes an IPC-private fallback command outside the SDK command map.
 * @param [in] pUserData Host callback context.
 * @param [in] pCommand Platform command name.
 * @param [in] pDataJson Command Data JSON.
 * @param [out] pResultJson Result JSON output buffer.
 * @param [in] uResultCapacity Result buffer capacity.
 * @return Device command return code.
 */
typedef INT32(STDCALL *NET_CB_PlatformExecuteCommand)(
    IN LPVOID pUserData,
    IN const CHAR *pCommand,
    IN const CHAR *pDataJson,
    OUT CHAR *pResultJson,
    IN UINT32 uResultCapacity);

/**
 * @struct tagNETPlatformHostCallbacks
 * @brief Defines all IPC capabilities consumed by the SDK platform runtime.
 */
typedef struct tagNETPlatformHostCallbacks
{
    UINT32 uStructSize;
    UINT32 uVersion;
    LPVOID pUserData;
    NET_CB_PlatformGetDeviceProfile fnGetDeviceProfile;
    NET_CB_PlatformGetStreamProfile fnGetStreamProfile;
    NET_CB_PlatformApplyRtmpStream fnApplyRtmpStream;
    NET_CB_PlatformResolveEventImage fnResolveEventImage;
    NET_CB_PlatformRuntimeStatus fnRuntimeStatus;
    NET_CB_PlatformExecuteCommand fnExecuteCommand;
    BYTE byRes[128];
} NET_PlatformHostCallbacks_S, *pNET_PlatformHostCallbacks_S;

/**
 * @brief Registers IPC host capability callbacks in the SDK.
 * @author Codex
 * @param [in] pCallbacks Callback table copied by the SDK.
 * @return TRUE on success, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverRegisterPlatformHostCallbacks(
    IN const NET_PlatformHostCallbacks_S *pCallbacks);

/**
 * @brief Applies platform runtime configuration in the SDK.
 * @author Codex
 * @param [in] pConfig Configuration copied by the SDK.
 * @return TRUE on success, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformApplyConfig(
    IN const NET_PlatformConfig_S *pConfig);

/**
 * @brief Starts the SDK platform runtime.
 * @author Codex
 * @return TRUE on success or when disabled, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformStart(void);

/**
 * @brief Stops the SDK platform runtime and joins all workers.
 * @author Codex
 * @return TRUE after the runtime stops.
 */
NET_API BOOL STDCALL NET_serverPlatformStop(void);

/**
 * @brief Notifies the SDK that the active network route changed.
 * @author Codex
 * @return TRUE when the notification is accepted, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformNotifyNetworkChanged(void);

/**
 * @brief Reports one event and optionally queues its image upload.
 * @author Codex
 * @param [in] pEvent Event data copied by the SDK.
 * @return TRUE when accepted, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformReportEvent(
    IN const NET_PlatformEventReport_S *pEvent);

/**
 * @brief Downloads one platform image through the SDK HTTP session.
 * @author Codex
 * @param [in] pUrl Absolute or platform-relative image URL.
 * @param [in] pLocalPath Local destination path.
 * @param [in] llExpectedSize Expected bytes or zero.
 * @return TRUE on success, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformDownloadImage(
    IN const CHAR *pUrl,
    IN const CHAR *pLocalPath,
    IN INT64 llExpectedSize);

/**
 * @brief Obtains the current SDK platform runtime status.
 * @author Codex
 * @param [out] pStatus Caller-owned status structure.
 * @return TRUE on success, otherwise FALSE.
 */
NET_API BOOL STDCALL NET_serverPlatformGetStatus(
    OUT NET_PlatformRuntimeStatus_S *pStatus);

#ifdef __cplusplus
}
#endif

#endif
