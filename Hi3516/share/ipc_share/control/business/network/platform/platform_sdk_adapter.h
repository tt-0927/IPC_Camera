/**
 * @file platform_sdk_adapter.h
 * @author Codex
 * @date 2026-08-22
 * @brief Declares the IPC host adapter for the SDK-owned platform runtime.
 * @change 2026-08-22 Codex Initial implementation for complete platform migration.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

#include "Singleton.h"
#include "network_define.h"
#include "tvsdk_platform_api.h"

#if CAP_GARBAGE_STATION_PLATFORM

/**
 * @class CPlatformSdkAdapter
 * @brief Bridges IPC-private capabilities to the SDK platform communication ABI.
 */
class CPlatformSdkAdapter : public CSingleton<CPlatformSdkAdapter>
{
private:
    /**
     * @brief Constructs an adapter without starting SDK workers.
     * @author Codex
     */
    CPlatformSdkAdapter();

public:
    /**
     * @brief Stops the SDK platform runtime before adapter destruction.
     * @author Codex
     */
    ~CPlatformSdkAdapter();

    friend class CSingleton<CPlatformSdkAdapter>;

    /**
     * @brief Registers IPC host capability callbacks with the SDK.
     * @author Codex
     * @return OK on success, otherwise an IPC error code.
     */
    int register_host_callbacks();

    /**
     * @brief Converts and applies one persisted IPC platform configuration.
     * @author Codex
     * @param [in] stInfo IPC platform configuration.
     * @return OK on success, otherwise an IPC error code.
     */
    int apply_config(const Network::Platform_Info_t &stInfo);

    /**
     * @brief Starts SDK authentication, MQTT and transfer workers.
     * @author Codex
     * @return OK on success or when disabled, otherwise an IPC error code.
     */
    int start_runtime();

    /**
     * @brief Stops and joins every SDK platform worker.
     * @author Codex
     * @return OK after shutdown, otherwise an IPC error code.
     */
    int stop_runtime();

    /**
     * @brief Recreates the SDK runtime from the currently applied configuration.
     * @author Codex
     * @return OK on success, otherwise an IPC error code.
     */
    int restart_runtime();

    /**
     * @brief Notifies the SDK that the active device network route changed.
     * @author Codex
     * @return OK when refresh is accepted, otherwise an IPC error code.
     */
    int notify_network_changed();

    /**
     * @brief Publishes one event and optionally queues its image for SDK upload.
     * @author Codex
     * @param [in] strCommand MQTT event command.
     * @param [in] strRequestId Correlation request identifier.
     * @param [in] strDataJson Event Data JSON object.
     * @param [in] nEventType Device event type.
     * @param [in] strEventName Human-readable event name.
     * @param [in] nChannel Event channel.
     * @param [in] llTimestampMs Event Unix timestamp in milliseconds.
     * @param [in] strImagePath Optional immediately available local image path.
     * @param [in] bUploadImage Whether the SDK transfer worker uploads an image.
     * @param [in] bResolveImageIfMissing Whether delayed image resolution is allowed.
     * @return OK when accepted, otherwise an IPC error code.
     */
    int report_event(const std::string &strCommand,
                     const std::string &strRequestId,
                     const std::string &strDataJson,
                     int nEventType,
                     const std::string &strEventName,
                     int nChannel,
                     long long llTimestampMs,
                     const std::string &strImagePath,
                     bool bUploadImage,
                     bool bResolveImageIfMissing);

    /**
     * @brief Downloads one platform image through the SDK HTTP session.
     * @author Codex
     * @param [in] strUrl Absolute or platform-relative source URL.
     * @param [in] strLocalPath Local destination path.
     * @param [in] llExpectedSize Expected byte count or zero.
     * @return OK on success, otherwise an IPC error code.
     */
    int download_image(const std::string &strUrl,
                       const std::string &strLocalPath,
                       long long llExpectedSize);

    /**
     * @brief Reads the current SDK platform status.
     * @author Codex
     * @param [out] stStatus Runtime status output.
     * @return true on success, otherwise false.
     */
    bool get_status(NET_PlatformRuntimeStatus_S &stStatus) const;

    /**
     * @brief Waits until HTTP authentication and device registration complete.
     * @author Codex
     * @param [in] uTimeoutMs Maximum wait duration in milliseconds.
     * @return true when the device is registered before timeout.
     */
    bool wait_until_registered(unsigned int uTimeoutMs);

    /**
     * @brief Tests whether the SDK currently reports a registered device.
     * @author Codex
     * @return true only after successful platform HTTP registration.
     */
    bool is_device_registered() const;

private:
    /**
     * @brief Supplies current IPC device identity to the SDK.
     * @author Codex
     * @param [in] pUserData Adapter instance.
     * @param [out] pProfile Device profile output.
     * @return Zero on success, otherwise an IPC error code.
     */
    static INT32 STDCALL get_device_profile_callback(
        LPVOID pUserData,
        pNET_PlatformDeviceProfile_S pProfile);

    /**
     * @brief Supplies current route, RTSP URLs and credentials to the SDK.
     * @author Codex
     * @param [in] pUserData Adapter instance.
     * @param [out] pProfile Stream profile output.
     * @return Zero on success, otherwise an IPC error code.
     */
    static INT32 STDCALL get_stream_profile_callback(
        LPVOID pUserData,
        pNET_PlatformStreamProfile_S pProfile);

    /**
     * @brief Applies the SDK-selected RTMP state through the IPC push module.
     * @author Codex
     * @param [in] pUserData Adapter instance.
     * @param [in] pRtmpUrl SDK-selected RTMP URL.
     * @param [in] bEnable TRUE starts and FALSE stops RTMP publishing.
     * @return Zero on success, otherwise an IPC error code.
     */
    static INT32 STDCALL apply_rtmp_stream_callback(
        LPVOID pUserData,
        const CHAR *pRtmpUrl,
        BOOL bEnable);

    /**
     * @brief Resolves a readable image path for one delayed event transfer.
     * @author Codex
     * @param [in] pUserData Adapter instance.
     * @param [in] pEvent Event metadata.
     * @param [out] pImagePath Image path output buffer.
     * @param [in] uImagePathSize Output buffer capacity.
     * @return Zero when an image is available, otherwise an IPC error code.
     */
    static INT32 STDCALL resolve_event_image_callback(
        LPVOID pUserData,
        const NET_PlatformEventReport_S *pEvent,
        CHAR *pImagePath,
        UINT32 uImagePathSize);

    /**
     * @brief Stores and logs significant SDK platform runtime transitions.
     * @author Codex
     * @param [in] pUserData Adapter instance.
     * @param [in] pStatus Runtime status snapshot.
     * @return No return value.
     */
    static VOID STDCALL runtime_status_callback(
        LPVOID pUserData,
        const NET_PlatformRuntimeStatus_S *pStatus);

    /**
     * @brief Executes IPC-private commands not handled directly by SDK business code.
     * @author Codex
     * @param [in] pUserData Adapter instance.
     * @param [in] pCommand Platform command.
     * @param [in] pDataJson Command Data JSON.
     * @param [out] pResultJson Result JSON output buffer.
     * @param [in] uResultCapacity Output buffer capacity.
     * @return Device command return code.
     */
    static INT32 STDCALL execute_command_callback(
        LPVOID pUserData,
        const CHAR *pCommand,
        const CHAR *pDataJson,
        CHAR *pResultJson,
        UINT32 uResultCapacity);

    std::recursive_mutex m_mtxLifecycle;
    mutable std::mutex m_mtxConfig;
    mutable std::mutex m_mtxStatus;
    std::condition_variable m_cvStatus;
    Network::Platform_Info_t m_stPlatformInfo;
    NET_PlatformRuntimeStatus_S m_stLastStatus;
    std::atomic<bool> m_bCallbacksRegistered;
};

#endif
