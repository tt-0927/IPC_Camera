/**
 * @file PlatformModule.h
 * @author Codex
 * @date 2026-08-22
 * @brief Declares the SDK-owned platform communication runtime.
 * @change 2026-08-22 Codex Initial implementation for complete MQTT migration.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include "NetTVSDKServerInterface.h"
#include "PlatformCommandRouter.h"
#include "PlatformHttpClient.h"
#include "PlatformMqttTransport.h"

/**
 * @class CPlatformModule
 * @brief Owns platform authentication, MQTT, commands, heartbeat and image transfer.
 */
class CPlatformModule
{
public:
    /**
     * @brief Returns the process-wide platform runtime.
     * @author Codex
     * @return Platform runtime singleton.
     */
    static CPlatformModule &Instance();

    CPlatformModule(const CPlatformModule &) = delete;
    CPlatformModule &operator=(const CPlatformModule &) = delete;

    /**
     * @brief Copies the host capability callback table.
     * @author Codex
     * @param [IN] pCallbacks Callback table.
     * @return True when the ABI size and required callbacks are valid.
     */
    bool RegisterHostCallbacks(const NET_PlatformHostCallbacks_S *pCallbacks);

    /**
     * @brief Copies platform endpoint and worker configuration.
     * @author Codex
     * @param [IN] pConfig Platform configuration.
     * @return True when the configuration is valid and applied.
     */
    bool ApplyConfig(const NET_PlatformConfig_S *pConfig);

    /**
     * @brief Starts all platform communication workers.
     * @author Codex
     * @return True when enabled workers start or the module is disabled.
     */
    bool Start();

    /**
     * @brief Stops platform workers and releases communication resources.
     * @author Codex
     * @return True after all workers are joined.
     */
    bool Stop();

    /**
     * @brief Requests profile refresh, HTTP reauthentication and MQTT reconnect.
     * @author Codex
     * @return True when the request is accepted by a running module.
     */
    bool NotifyNetworkChanged();

    /**
     * @brief Publishes an event and optionally queues one image upload.
     * @author Codex
     * @param [IN] pEvent Event request whose strings are copied synchronously.
     * @return True when the event publication is accepted.
     */
    bool ReportEvent(const NET_PlatformEventReport_S *pEvent);

    /**
     * @brief Downloads one platform image through the authenticated HTTP client.
     * @author Codex
     * @param [IN] pUrl Absolute or platform-relative URL.
     * @param [IN] pLocalPath Destination path.
     * @param [IN] llExpectedSize Expected bytes, or zero to skip size validation.
     * @return True when the file is atomically installed.
     */
    bool DownloadImage(const char *pUrl,
                       const char *pLocalPath,
                       std::int64_t llExpectedSize);

    /**
     * @brief Copies the current runtime status.
     * @author Codex
     * @param [OUT] pStatus Caller-owned status structure.
     * @return True when pStatus is valid.
     */
    bool GetStatus(NET_PlatformRuntimeStatus_S *pStatus) const;

private:
    /**
     * @struct CommandJob_S
     * @brief Deep-copied MQTT command queued outside the Paho callback.
     */
    struct CommandJob_S
    {
        std::string strTopic;
        std::string strPayload;
    };

    /**
     * @struct EventJob_S
     * @brief Owned event metadata used by the asynchronous transfer worker.
     */
    struct EventJob_S
    {
        std::string strCommand;
        std::string strRequestId;
        std::string strDataJson;
        int nEventType{0};
        std::string strEventName;
        int nChannel{0};
        std::int64_t llTimestampMs{0};
        std::string strImagePath;
        bool bUploadImage{false};
        bool bResolveImageIfMissing{false};
    };

    /**
     * @brief Constructs an inactive runtime.
     * @author Codex
     */
    CPlatformModule();

    /**
     * @brief Stops all workers before static destruction.
     * @author Codex
     */
    ~CPlatformModule();

    /**
     * @brief Validates an enabled platform configuration.
     * @author Codex
     * @param [IN] stConfig Configuration to validate.
     * @param [OUT] strError Validation failure description.
     * @return True when the runtime can start.
     */
    static bool ValidateConfig(const NET_PlatformConfig_S &stConfig,
                               std::string &strError);

    /**
     * @brief Queries and stores the latest device and stream profiles.
     * @author Codex
     * @param [OUT] strError Host callback failure description.
     * @return True when both profiles are valid.
     */
    bool RefreshProfiles(std::string &strError);

    /**
     * @brief Applies RTMP for non-wired uplinks and stops stale RTMP for wired uplinks.
     * @author Codex
     * @param [OUT] strError Host callback failure description.
     * @return True when no stream change is required or the callback succeeds.
     */
    bool ApplySelectedStream(std::string &strError);

    /**
     * @brief Performs platform HTTP login and device registration.
     * @author Codex
     * @param [OUT] strError HTTP failure description.
     * @return True when registration succeeds.
     */
    bool AuthenticateAndRegister(std::string &strError);

    /**
     * @brief Publishes encrypted stream registration to MQTT.
     * @author Codex
     * @return True when the publication is submitted.
     */
    bool PublishMqttRegistration();

    /**
     * @brief Publishes one online or offline status message.
     * @author Codex
     * @param [IN] bOnline Desired state.
     * @param [IN] strReason State reason.
     * @return True when the publication is submitted.
     */
    bool PublishStatus(bool bOnline, const std::string &strReason);

    /**
     * @brief Receives deep-copied MQTT messages from the transport.
     * @author Codex
     * @param [IN] strTopic Source topic.
     * @param [IN] strPayload Message payload.
     * @return No return value.
     */
    void OnMqttMessage(const std::string &strTopic,
                       const std::string &strPayload);

    /**
     * @brief Receives stable MQTT connection transitions.
     * @author Codex
     * @param [IN] bConnected Current connection state.
     * @param [IN] strReason Transition reason.
     * @return No return value.
     */
    void OnMqttConnectionChanged(bool bConnected,
                                 const std::string &strReason);

    /**
     * @brief Executes queued MQTT commands serially.
     * @author Codex
     * @return No return value.
     */
    void CommandLoop();

    /**
     * @brief Resolves and uploads queued event images serially.
     * @author Codex
     * @return No return value.
     */
    void TransferLoop();

    /**
     * @brief Maintains HTTP registration, stream selection and status heartbeat.
     * @author Codex
     * @return No return value.
     */
    void MaintenanceLoop();

    /**
     * @brief Uploads one event image and publishes its result event.
     * @author Codex
     * @param [IN] stJob Owned event transfer job.
     * @return No return value.
     */
    void ProcessEventImage(const EventJob_S &stJob);

    /**
     * @brief Publishes one event-image upload result to the MQTT event topic.
     * @author Codex
     * @param [IN] stJob Owned event transfer job.
     * @param [IN] stResponse HTTP image response details.
     * @param [IN] bUploadOk Whether the upload succeeded.
     * @param [IN] strError Failure description when the upload did not succeed.
     * @return True when the MQTT result publication is submitted.
     */
    bool PublishEventImageResult(
        const EventJob_S &stJob,
        const CPlatformHttpClient::EventImageResponse_S &stResponse,
        bool bUploadOk,
        const std::string &strError);

    /**
     * @brief Resolves one delayed event image through the host callback.
     * @author Codex
     * @param [IN] stJob Event transfer job.
     * @param [OUT] strImagePath Resolved local image path.
     * @return True when a readable file is available before timeout.
     */
    bool ResolveEventImage(const EventJob_S &stJob,
                           std::string &strImagePath);

    /**
     * @brief Creates an ABI event view whose pointers refer to one owned event job.
     * @author Codex
     * @param [IN] stJob Owned event job.
     * @return Callback-safe event view valid while stJob remains alive.
     */
    static NET_PlatformEventReport_S BuildEventView(const EventJob_S &stJob);

    /**
     * @brief Invokes the optional host fallback command callback.
     * @author Codex
     * @param [IN] strCommand Original command name.
     * @param [IN] strDataJson Command Data JSON.
     * @param [OUT] strResultJson Host response JSON.
     * @return Host return code.
     */
    int ExecuteFallbackCommand(const std::string &strCommand,
                               const std::string &strDataJson,
                               std::string &strResultJson);

    /**
     * @brief Updates the last runtime error and notifies the host.
     * @author Codex
     * @param [IN] nError Error code.
     * @param [IN] strError Non-sensitive diagnostic text.
     * @return No return value.
     */
    void SetLastError(int nError, const std::string &strError);

    /**
     * @brief Sends one consistent status snapshot to the optional host callback.
     * @author Codex
     * @return No return value.
     */
    void NotifyRuntimeStatus();

    mutable std::mutex m_mtxLifecycle;
    mutable std::mutex m_mtxHost;
    mutable std::mutex m_mtxProfiles;
    mutable std::mutex m_mtxStatus;
    NET_PlatformConfig_S m_stConfig;
    NET_PlatformHostCallbacks_S m_stHostCallbacks;
    NET_PlatformDeviceProfile_S m_stDeviceProfile;
    NET_PlatformStreamProfile_S m_stStreamProfile;
    NET_PlatformRuntimeStatus_S m_stStatus;
    CPlatformMqttTransport m_stMqttTransport;
    CPlatformHttpClient m_stHttpClient;
    CPlatformCommandRouter m_stCommandRouter;
    std::thread m_stCommandThread;
    std::thread m_stTransferThread;
    std::thread m_stMaintenanceThread;
    std::atomic<bool> m_bRunning{false};
    std::atomic<bool> m_bMqttConnected{false};
    std::atomic<bool> m_bRefreshRequested{false};
    std::atomic<bool> m_bMqttRegistrationPending{false};
    std::atomic<bool> m_bHttpRegistrationPending{false};
    std::mutex m_mtxCommandQueue;
    std::condition_variable m_cvCommandQueue;
    std::deque<CommandJob_S> m_aCommandQueue;
    std::mutex m_mtxTransferQueue;
    std::condition_variable m_cvTransferQueue;
    std::deque<EventJob_S> m_aTransferQueue;
    std::mutex m_mtxMaintenance;
    std::condition_variable m_cvMaintenance;
    bool m_bRtmpApplied{false};
    std::string m_strAppliedRtmpUrl;
};
