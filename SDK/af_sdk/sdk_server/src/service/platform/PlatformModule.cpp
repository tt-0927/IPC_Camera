/**
 * @file PlatformModule.cpp
 * @author Codex
 * @date 2026-08-22
 * @brief Implements the SDK-owned platform communication runtime.
 * @change 2026-08-22 Codex Initial implementation for complete MQTT migration.
 */

#include "PlatformModule.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <utility>
#include <vector>

#include <openssl/crypto.h>

#include "NetSdkLog.h"
#include "PlatformProtocol.h"
#include "PlatformRegisterCrypto.h"

namespace
{
constexpr std::size_t PLATFORM_MODULE_MAX_COMMAND_QUEUE_SIZE = 32U;
constexpr std::size_t PLATFORM_MODULE_MAX_TRANSFER_QUEUE_SIZE = 64U;
constexpr unsigned int PLATFORM_MODULE_DEFAULT_HEARTBEAT_SEC = 30U;
constexpr unsigned int PLATFORM_MODULE_HTTP_RETRY_SEC = 30U;
constexpr unsigned int PLATFORM_MODULE_MAINTENANCE_POLL_MS = 500U;
constexpr unsigned int PLATFORM_MODULE_DEFAULT_IMAGE_WAIT_MS = 3000U;
constexpr unsigned int PLATFORM_MODULE_DEFAULT_IMAGE_POLL_MS = 500U;
constexpr const char *PLATFORM_MODULE_IMAGE_EVENT_COMMAND = "NET_TV_EVENT_IMAGE_UPLOAD";

/**
 * @brief Copies a bounded ABI character array into a C++ string.
 * @author Codex
 * @param [IN] pValue Source character array.
 * @param [IN] uCapacity Source capacity in bytes.
 * @return Bounded string value.
 */
static std::string CopyAbiString(const char *pValue, std::size_t uCapacity)
{
    if (pValue == nullptr || uCapacity == 0)
    {
        return std::string();
    }

    std::size_t uLength = 0;
    while (uLength < uCapacity && pValue[uLength] != '\0')
    {
        ++uLength;
    }
    return std::string(pValue, uLength);
}

/**
 * @brief Copies text into a fixed ABI buffer and always terminates it.
 * @author Codex
 * @param [IN] strValue Source text.
 * @param [OUT] pBuffer Destination buffer.
 * @param [IN] uCapacity Destination capacity in bytes.
 * @return No return value.
 */
static void CopyToAbiBuffer(const std::string &strValue,
                            char *pBuffer,
                            std::size_t uCapacity)
{
    if (pBuffer == nullptr || uCapacity == 0)
    {
        return;
    }
    const std::size_t uCopyLength = std::min(strValue.size(), uCapacity - 1U);
    std::memcpy(pBuffer, strValue.data(), uCopyLength);
    pBuffer[uCopyLength] = '\0';
}

/**
 * @brief Forces every character array in a device profile to terminate.
 * @author Codex
 * @param [INOUT] stProfile Device profile.
 * @return No return value.
 */
static void TerminateDeviceProfile(NET_PlatformDeviceProfile_S &stProfile)
{
    stProfile.strSerialNumber[sizeof(stProfile.strSerialNumber) - 1U] = '\0';
    stProfile.strDeviceName[sizeof(stProfile.strDeviceName) - 1U] = '\0';
    stProfile.strFirmwareVersion[sizeof(stProfile.strFirmwareVersion) - 1U] = '\0';
    stProfile.strMacAddress[sizeof(stProfile.strMacAddress) - 1U] = '\0';
    stProfile.strLocalIp[sizeof(stProfile.strLocalIp) - 1U] = '\0';
    stProfile.strResolution[sizeof(stProfile.strResolution) - 1U] = '\0';
    stProfile.strStorage[sizeof(stProfile.strStorage) - 1U] = '\0';
    stProfile.strUseStorage[sizeof(stProfile.strUseStorage) - 1U] = '\0';
    stProfile.strLocation[sizeof(stProfile.strLocation) - 1U] = '\0';
}

/**
 * @brief Forces every character array in a stream profile to terminate.
 * @author Codex
 * @param [INOUT] stProfile Stream profile.
 * @return No return value.
 */
static void TerminateStreamProfile(NET_PlatformStreamProfile_S &stProfile)
{
    stProfile.strUplinkInterface[sizeof(stProfile.strUplinkInterface) - 1U] = '\0';
    stProfile.strLocalIp[sizeof(stProfile.strLocalIp) - 1U] = '\0';
    stProfile.strRtspMainUrl[sizeof(stProfile.strRtspMainUrl) - 1U] = '\0';
    stProfile.strRtspSubUrl[sizeof(stProfile.strRtspSubUrl) - 1U] = '\0';
    stProfile.strRtspAccount[sizeof(stProfile.strRtspAccount) - 1U] = '\0';
    stProfile.strRtspPassword[sizeof(stProfile.strRtspPassword) - 1U] = '\0';
}

/**
 * @brief Returns whether a local path names a nonempty regular file.
 * @author Codex
 * @param [IN] strPath File path.
 * @return True when the file can be used for upload.
 */
static bool IsReadableImageFile(const std::string &strPath)
{
    struct stat stInfo;
    return !strPath.empty() && stat(strPath.c_str(), &stInfo) == 0 &&
           S_ISREG(stInfo.st_mode) && stInfo.st_size > 0;
}

/**
 * @brief Converts a host uplink enum to the MQTT protocol value.
 * @author Codex
 * @param [IN] nUplinkType NET_PlatformUplinkType_EN value.
 * @return Stable protocol text.
 */
static std::string GetUplinkTypeText(int nUplinkType)
{
    switch (nUplinkType)
    {
    case NET_PLATFORM_UPLINK_WIRED:
        return "wired";
    case NET_PLATFORM_UPLINK_WIRELESS:
        return "wireless";
    case NET_PLATFORM_UPLINK_CELLULAR:
        return "cellular";
    default:
        return "unknown";
    }
}

/**
 * @brief Returns whether platform playback should pull the device RTSP stream.
 * @author Codex
 * @param [IN] nUplinkType NET_PlatformUplinkType_EN value.
 * @return True only for a confirmed wired uplink.
 */
static bool ShouldUseRtsp(int nUplinkType)
{
    return nUplinkType == NET_PLATFORM_UPLINK_WIRED;
}

/**
 * @brief Builds the platform RTMP main-stream URL.
 * @author Codex
 * @param [IN] strHost Platform host.
 * @param [IN] nPort RTMP service port.
 * @param [IN] strDeviceSn Device serial number.
 * @return RTMP URL or an empty string for invalid input.
 */
static std::string BuildRtmpUrl(const std::string &strHost,
                                int nPort,
                                const std::string &strDeviceSn)
{
    if (strHost.empty() || strDeviceSn.empty() || nPort <= 0 || nPort > 65535)
    {
        return std::string();
    }
    return "rtmp://" + strHost + ":" + std::to_string(nPort) +
           "/live/" + strDeviceSn + "-main";
}

}

CPlatformModule &CPlatformModule::Instance()
{
    static CPlatformModule s_stInstance;
    return s_stInstance;
}

CPlatformModule::CPlatformModule()
{
    std::memset(&m_stConfig, 0, sizeof(m_stConfig));
    std::memset(&m_stHostCallbacks, 0, sizeof(m_stHostCallbacks));
    std::memset(&m_stDeviceProfile, 0, sizeof(m_stDeviceProfile));
    std::memset(&m_stStreamProfile, 0, sizeof(m_stStreamProfile));
    std::memset(&m_stStatus, 0, sizeof(m_stStatus));
    m_stStatus.uStructSize = sizeof(m_stStatus);
    m_stStatus.uVersion = NET_PLATFORM_ABI_VERSION;
}

CPlatformModule::~CPlatformModule()
{
    Stop();
    OPENSSL_cleanse(&m_stConfig, sizeof(m_stConfig));
    OPENSSL_cleanse(&m_stStreamProfile, sizeof(m_stStreamProfile));
}

bool CPlatformModule::ValidateConfig(const NET_PlatformConfig_S &stConfig,
                                     std::string &strError)
{
    strError.clear();
    if (stConfig.uStructSize < sizeof(NET_PlatformConfig_S) ||
        stConfig.uVersion != NET_PLATFORM_ABI_VERSION)
    {
        strError = "platform configuration ABI is incompatible";
        return false;
    }
    if (stConfig.bEnable == FALSE)
    {
        return true;
    }
    if (CopyAbiString(stConfig.strHttpHost, sizeof(stConfig.strHttpHost)).empty() ||
        stConfig.nHttpPort <= 0 || stConfig.nHttpPort > 65535 ||
        CopyAbiString(stConfig.strMqttHost, sizeof(stConfig.strMqttHost)).empty() ||
        stConfig.nMqttPort <= 0 || stConfig.nMqttPort > 65535 ||
        stConfig.nRtmpPort <= 0 || stConfig.nRtmpPort > 65535 ||
        CopyAbiString(stConfig.strPlatformUser, sizeof(stConfig.strPlatformUser)).empty() ||
        CopyAbiString(stConfig.strPlatformPassword,
                      sizeof(stConfig.strPlatformPassword)).empty())
    {
        strError = "platform endpoint or HTTP credentials are invalid";
        return false;
    }
    return true;
}

bool CPlatformModule::RegisterHostCallbacks(
    const NET_PlatformHostCallbacks_S *pCallbacks)
{
    if (pCallbacks == nullptr ||
        pCallbacks->uStructSize < sizeof(NET_PlatformHostCallbacks_S) ||
        pCallbacks->uVersion != NET_PLATFORM_ABI_VERSION ||
        pCallbacks->fnGetDeviceProfile == nullptr ||
        pCallbacks->fnGetStreamProfile == nullptr)
    {
        return false;
    }

    const bool bRestart = m_bRunning.load();
    if (bRestart)
    {
        Stop();
    }
    {
        std::lock_guard<std::mutex> stHostLock(m_mtxHost);
        m_stHostCallbacks = *pCallbacks;
    }
    return !bRestart || Start();
}

bool CPlatformModule::ApplyConfig(const NET_PlatformConfig_S *pConfig)
{
    if (pConfig == nullptr)
    {
        return false;
    }

    std::string strError;
    if (!ValidateConfig(*pConfig, strError))
    {
        SetLastError(-1, strError);
        return false;
    }

    const bool bRestart = m_bRunning.load();
    if (bRestart)
    {
        Stop();
    }
    {
        std::lock_guard<std::mutex> stLifecycleLock(m_mtxLifecycle);
        OPENSSL_cleanse(&m_stConfig, sizeof(m_stConfig));
        m_stConfig = *pConfig;
    }
    return !bRestart || Start();
}

bool CPlatformModule::Start()
{
    std::lock_guard<std::mutex> stLifecycleLock(m_mtxLifecycle);
    if (m_bRunning.load())
    {
        return true;
    }

    std::string strError;
    if (!ValidateConfig(m_stConfig, strError))
    {
        SetLastError(-1, strError);
        return false;
    }
    if (m_stConfig.bEnable == FALSE)
    {
        NETSDK_LOG_MESSAGE_INFO("Platform runtime remains disabled by configuration");
        return true;
    }
    {
        std::lock_guard<std::mutex> stHostLock(m_mtxHost);
        if (m_stHostCallbacks.fnGetDeviceProfile == nullptr ||
            m_stHostCallbacks.fnGetStreamProfile == nullptr)
        {
            SetLastError(-1, "platform host callbacks are not registered");
            return false;
        }
    }
    if (!RefreshProfiles(strError))
    {
        SetLastError(-1, strError);
        return false;
    }

    CPlatformHttpClient::Config_S stHttpConfig;
    stHttpConfig.strHost = CopyAbiString(m_stConfig.strHttpHost,
                                         sizeof(m_stConfig.strHttpHost));
    stHttpConfig.nPort = m_stConfig.nHttpPort;
    stHttpConfig.strUser = CopyAbiString(m_stConfig.strPlatformUser,
                                         sizeof(m_stConfig.strPlatformUser));
    stHttpConfig.strPassword = CopyAbiString(m_stConfig.strPlatformPassword,
                                             sizeof(m_stConfig.strPlatformPassword));
    if (!m_stHttpClient.Configure(stHttpConfig))
    {
        SetLastError(-1, "platform HTTP client configuration failed");
        return false;
    }

    const std::string strDownloadDirectory =
        CopyAbiString(m_stConfig.strImageDownloadDirectory,
                      sizeof(m_stConfig.strImageDownloadDirectory));
    if (!m_stCommandRouter.Configure(
            &m_stHttpClient,
            strDownloadDirectory.empty() ? "/tmp" : strDownloadDirectory,
            [this](const std::string &strCommand,
                   const std::string &strDataJson,
                   std::string &strResultJson)
            {
                return ExecuteFallbackCommand(strCommand, strDataJson, strResultJson);
            }))
    {
        SetLastError(-1, "platform command router configuration failed");
        return false;
    }

    NET_PlatformDeviceProfile_S stDeviceProfile;
    {
        std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
        stDeviceProfile = m_stDeviceProfile;
    }
    const std::string strDeviceSn = CopyAbiString(stDeviceProfile.strSerialNumber,
                                                  sizeof(stDeviceProfile.strSerialNumber));
    CPlatformMqttTransport::Config_S stMqttConfig;
    stMqttConfig.strHost = CopyAbiString(m_stConfig.strMqttHost,
                                         sizeof(m_stConfig.strMqttHost));
    stMqttConfig.nPort = m_stConfig.nMqttPort;
    stMqttConfig.strUser = CopyAbiString(m_stConfig.strMqttUser,
                                         sizeof(m_stConfig.strMqttUser));
    stMqttConfig.strPassword = CopyAbiString(m_stConfig.strMqttPassword,
                                             sizeof(m_stConfig.strMqttPassword));
    stMqttConfig.strClientId = strDeviceSn;
    stMqttConfig.strRuntimeLibrary =
        CopyAbiString(m_stConfig.strMqttRuntimeLibrary,
                      sizeof(m_stConfig.strMqttRuntimeLibrary));
    stMqttConfig.strWillTopic = CPlatformProtocol::BuildStatusTopic(strDeviceSn);
    stMqttConfig.strWillPayload =
        CPlatformProtocol::BuildStatus(strDeviceSn, false, "lwt", 0);
    stMqttConfig.nWillQos = 1;
    stMqttConfig.bWillRetain = false;

    {
        std::lock_guard<std::mutex> stCommandLock(m_mtxCommandQueue);
        m_aCommandQueue.clear();
    }
    {
        std::lock_guard<std::mutex> stTransferLock(m_mtxTransferQueue);
        m_aTransferQueue.clear();
    }
    m_bRunning.store(true);
    m_bMqttConnected.store(false);
    m_bRefreshRequested.store(true);
    m_bMqttRegistrationPending.store(false);
    m_bHttpRegistrationPending.store(true);
    {
        std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
        std::memset(&m_stStatus, 0, sizeof(m_stStatus));
        m_stStatus.uStructSize = sizeof(m_stStatus);
        m_stStatus.uVersion = NET_PLATFORM_ABI_VERSION;
        m_stStatus.bRunning = TRUE;
    }

    m_stCommandThread = std::thread(&CPlatformModule::CommandLoop, this);
    m_stTransferThread = std::thread(&CPlatformModule::TransferLoop, this);
    m_stMaintenanceThread = std::thread(&CPlatformModule::MaintenanceLoop, this);
    if (!m_stMqttTransport.Start(
            stMqttConfig,
            [this](const std::string &strTopic, const std::string &strPayload)
            {
                OnMqttMessage(strTopic, strPayload);
            },
            [this](bool bConnected, const std::string &strReason)
            {
                OnMqttConnectionChanged(bConnected, strReason);
            }))
    {
        m_bRunning.store(false);
        m_cvCommandQueue.notify_all();
        m_cvTransferQueue.notify_all();
        m_cvMaintenance.notify_all();
        if (m_stCommandThread.joinable())
        {
            m_stCommandThread.join();
        }
        if (m_stTransferThread.joinable())
        {
            m_stTransferThread.join();
        }
        if (m_stMaintenanceThread.joinable())
        {
            m_stMaintenanceThread.join();
        }
        m_stHttpClient.ClearSession();
        {
            std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
            m_stStatus.bRunning = FALSE;
            m_stStatus.bHttpAuthenticated = FALSE;
            m_stStatus.bMqttConnected = FALSE;
            m_stStatus.bDeviceRegistered = FALSE;
        }
        SetLastError(-1, "platform MQTT transport start failed");
        return false;
    }
    m_stMqttTransport.Subscribe(CPlatformProtocol::BuildCommandTopic(strDeviceSn), 1);
    m_cvMaintenance.notify_all();
    NotifyRuntimeStatus();
    NETSDK_LOG_MESSAGE_INFO("Platform runtime started: device=%s", strDeviceSn.c_str());
    return true;
}

bool CPlatformModule::Stop()
{
    std::lock_guard<std::mutex> stLifecycleLock(m_mtxLifecycle);
    if (!m_bRunning.load())
    {
        return true;
    }

    if (m_bMqttConnected.load())
    {
        PublishStatus(false, "shutdown");
    }
    m_bRunning.store(false);
    m_cvCommandQueue.notify_all();
    m_cvTransferQueue.notify_all();
    m_cvMaintenance.notify_all();
    m_stMqttTransport.Stop();

    if (m_stCommandThread.joinable())
    {
        m_stCommandThread.join();
    }
    if (m_stTransferThread.joinable())
    {
        m_stTransferThread.join();
    }
    if (m_stMaintenanceThread.joinable())
    {
        m_stMaintenanceThread.join();
    }

    NET_PlatformHostCallbacks_S stCallbacks;
    {
        std::lock_guard<std::mutex> stHostLock(m_mtxHost);
        stCallbacks = m_stHostCallbacks;
    }
    if (m_bRtmpApplied && stCallbacks.fnApplyRtmpStream != nullptr)
    {
        stCallbacks.fnApplyRtmpStream(stCallbacks.pUserData, "", FALSE);
    }
    m_bRtmpApplied = false;
    m_strAppliedRtmpUrl.clear();
    m_stHttpClient.ClearSession();
    m_bMqttConnected.store(false);

    {
        std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
        m_stStatus.bRunning = FALSE;
        m_stStatus.bHttpAuthenticated = FALSE;
        m_stStatus.bMqttConnected = FALSE;
        m_stStatus.bDeviceRegistered = FALSE;
    }
    NotifyRuntimeStatus();
    NETSDK_LOG_MESSAGE_INFO("Platform runtime stopped");
    return true;
}

bool CPlatformModule::NotifyNetworkChanged()
{
    if (!m_bRunning.load())
    {
        return false;
    }
    m_stHttpClient.ClearSession();
    {
        std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
        m_stStatus.bHttpAuthenticated = FALSE;
        m_stStatus.bDeviceRegistered = FALSE;
    }
    m_bRefreshRequested.store(true);
    m_bHttpRegistrationPending.store(true);
    m_bMqttRegistrationPending.store(true);
    m_stMqttTransport.RequestReconnect();
    m_cvMaintenance.notify_all();
    return true;
}

bool CPlatformModule::ReportEvent(const NET_PlatformEventReport_S *pEvent)
{
    if (!m_bRunning.load() || pEvent == nullptr ||
        pEvent->uStructSize < sizeof(NET_PlatformEventReport_S) ||
        pEvent->uVersion != NET_PLATFORM_ABI_VERSION || pEvent->pCommand == nullptr ||
        pEvent->pCommand[0] == '\0' || pEvent->pRequestId == nullptr ||
        pEvent->pRequestId[0] == '\0')
    {
        return false;
    }

    EventJob_S stJob;
    stJob.strCommand.assign(pEvent->pCommand);
    stJob.strRequestId.assign(pEvent->pRequestId);
    stJob.strDataJson = pEvent->pDataJson != nullptr && pEvent->pDataJson[0] != '\0'
                            ? pEvent->pDataJson
                            : "{}";
    stJob.nEventType = pEvent->nEventType;
    stJob.strEventName = pEvent->pEventName != nullptr ? pEvent->pEventName : "";
    stJob.nChannel = pEvent->nChannel;
    stJob.llTimestampMs = pEvent->llTimestampMs > 0
                              ? pEvent->llTimestampMs
                              : CPlatformProtocol::GetCurrentTimestampMs();
    stJob.strImagePath = pEvent->pImagePath != nullptr ? pEvent->pImagePath : "";
    stJob.bUploadImage = pEvent->bUploadImage != FALSE;
    stJob.bResolveImageIfMissing = pEvent->bResolveImageIfMissing != FALSE;

    NET_PlatformDeviceProfile_S stDeviceProfile;
    {
        std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
        stDeviceProfile = m_stDeviceProfile;
    }
    const std::string strDeviceSn = CopyAbiString(stDeviceProfile.strSerialNumber,
                                                  sizeof(stDeviceProfile.strSerialNumber));
    const std::string strPayload = CPlatformProtocol::BuildEvent(
        stJob.strCommand,
        stJob.strRequestId,
        stJob.strDataJson);
    if (strPayload.empty() || !m_stMqttTransport.Publish(
                                  CPlatformProtocol::BuildEventTopic(strDeviceSn),
                                  strPayload,
                                  0))
    {
        SetLastError(-1, "platform event publication failed");
        return false;
    }
    {
        std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
        ++m_stStatus.uPublishedEventCount;
    }

    if (stJob.bUploadImage)
    {
        bool bQueued = false;
        {
            std::lock_guard<std::mutex> stTransferLock(m_mtxTransferQueue);
            if (m_aTransferQueue.size() < PLATFORM_MODULE_MAX_TRANSFER_QUEUE_SIZE)
            {
                m_aTransferQueue.push_back(stJob);
                bQueued = true;
            }
        }
        if (bQueued)
        {
            m_cvTransferQueue.notify_one();
        }
        else
        {
            const std::string strQueueError = "platform image transfer queue is full";
            CPlatformHttpClient::EventImageResponse_S stResponse;
            PublishEventImageResult(stJob, stResponse, false, strQueueError);
            SetLastError(-1, strQueueError);
        }
    }
    return true;
}

bool CPlatformModule::DownloadImage(const char *pUrl,
                                    const char *pLocalPath,
                                    std::int64_t llExpectedSize)
{
    if (!m_bRunning.load() || pUrl == nullptr || pLocalPath == nullptr ||
        pUrl[0] == '\0' || pLocalPath[0] == '\0' || llExpectedSize < 0)
    {
        return false;
    }
    std::string strError;
    if (!m_stHttpClient.DownloadFile(pUrl, pLocalPath, llExpectedSize, strError))
    {
        m_stHttpClient.ClearSession();
        m_bHttpRegistrationPending.store(true);
        m_cvMaintenance.notify_all();
        SetLastError(-1, strError);
        return false;
    }
    return true;
}

bool CPlatformModule::GetStatus(NET_PlatformRuntimeStatus_S *pStatus) const
{
    if (pStatus == nullptr ||
        (pStatus->uStructSize != 0 &&
         pStatus->uStructSize < sizeof(NET_PlatformRuntimeStatus_S)))
    {
        return false;
    }
    {
        std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
        *pStatus = m_stStatus;
    }
    pStatus->uReconnectCount = m_stMqttTransport.GetReconnectCount();
    return true;
}

bool CPlatformModule::RefreshProfiles(std::string &strError)
{
    NET_PlatformHostCallbacks_S stCallbacks;
    {
        std::lock_guard<std::mutex> stHostLock(m_mtxHost);
        stCallbacks = m_stHostCallbacks;
    }

    NET_PlatformDeviceProfile_S stDeviceProfile;
    std::memset(&stDeviceProfile, 0, sizeof(stDeviceProfile));
    stDeviceProfile.uStructSize = sizeof(stDeviceProfile);
    stDeviceProfile.uVersion = NET_PLATFORM_ABI_VERSION;
    if (stCallbacks.fnGetDeviceProfile == nullptr ||
        stCallbacks.fnGetDeviceProfile(stCallbacks.pUserData, &stDeviceProfile) != 0)
    {
        strError = "host device profile callback failed";
        return false;
    }
    TerminateDeviceProfile(stDeviceProfile);
    if (CopyAbiString(stDeviceProfile.strSerialNumber,
                      sizeof(stDeviceProfile.strSerialNumber)).empty())
    {
        strError = "host device profile has no serial number";
        return false;
    }

    NET_PlatformStreamProfile_S stStreamProfile;
    std::memset(&stStreamProfile, 0, sizeof(stStreamProfile));
    stStreamProfile.uStructSize = sizeof(stStreamProfile);
    stStreamProfile.uVersion = NET_PLATFORM_ABI_VERSION;
    if (stCallbacks.fnGetStreamProfile == nullptr ||
        stCallbacks.fnGetStreamProfile(stCallbacks.pUserData, &stStreamProfile) != 0)
    {
        strError = "host stream profile callback failed";
        return false;
    }
    TerminateStreamProfile(stStreamProfile);

    {
        std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
        OPENSSL_cleanse(&m_stStreamProfile, sizeof(m_stStreamProfile));
        m_stDeviceProfile = stDeviceProfile;
        m_stStreamProfile = stStreamProfile;
    }
    return true;
}

bool CPlatformModule::ApplySelectedStream(std::string &strError)
{
    NET_PlatformDeviceProfile_S stDeviceProfile;
    NET_PlatformStreamProfile_S stStreamProfile;
    {
        std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
        stDeviceProfile = m_stDeviceProfile;
        stStreamProfile = m_stStreamProfile;
    }

    NET_PlatformHostCallbacks_S stCallbacks;
    {
        std::lock_guard<std::mutex> stHostLock(m_mtxHost);
        stCallbacks = m_stHostCallbacks;
    }
    const bool bUseRtsp = ShouldUseRtsp(stStreamProfile.enUplinkType);
    if (bUseRtsp)
    {
        /*
         * The host may have started RTMP from persisted IPC configuration before
         * the SDK runtime was initialized. Always issue the idempotent stop callback
         * for a wired route instead of relying only on SDK-local state.
         */
        if (stCallbacks.fnApplyRtmpStream != nullptr &&
            stCallbacks.fnApplyRtmpStream(stCallbacks.pUserData, "", FALSE) != 0)
        {
            strError = "host RTMP stop callback failed";
            return false;
        }
        m_bRtmpApplied = false;
        m_strAppliedRtmpUrl.clear();
        return true;
    }

    if (stCallbacks.fnApplyRtmpStream == nullptr)
    {
        strError = "host RTMP callback is unavailable for non-wired uplink";
        return false;
    }
    const std::string strDeviceSn = CopyAbiString(stDeviceProfile.strSerialNumber,
                                                  sizeof(stDeviceProfile.strSerialNumber));
    const std::string strRtmpUrl = BuildRtmpUrl(
        CopyAbiString(m_stConfig.strHttpHost, sizeof(m_stConfig.strHttpHost)),
        m_stConfig.nRtmpPort,
        strDeviceSn);
    if (strRtmpUrl.empty())
    {
        strError = "RTMP URL construction failed";
        return false;
    }
    if (m_bRtmpApplied && m_strAppliedRtmpUrl == strRtmpUrl)
    {
        return true;
    }
    if (stCallbacks.fnApplyRtmpStream(stCallbacks.pUserData,
                                      strRtmpUrl.c_str(),
                                      TRUE) != 0)
    {
        strError = "host RTMP apply callback failed";
        return false;
    }
    m_bRtmpApplied = true;
    m_strAppliedRtmpUrl = strRtmpUrl;
    return true;
}

bool CPlatformModule::AuthenticateAndRegister(std::string &strError)
{
    if (!m_stHttpClient.IsAuthenticated() && !m_stHttpClient.Login(strError))
    {
        return false;
    }
    {
        std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
        m_stStatus.bHttpAuthenticated = TRUE;
    }

    NET_PlatformDeviceProfile_S stDeviceProfile;
    NET_PlatformStreamProfile_S stStreamProfile;
    {
        std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
        stDeviceProfile = m_stDeviceProfile;
        stStreamProfile = m_stStreamProfile;
    }
    const bool bUseRtsp = ShouldUseRtsp(stStreamProfile.enUplinkType);
    const std::string strDeviceSn = CopyAbiString(stDeviceProfile.strSerialNumber,
                                                  sizeof(stDeviceProfile.strSerialNumber));
    const std::string strLiveUrl = bUseRtsp
                                       ? CopyAbiString(stStreamProfile.strRtspMainUrl,
                                                       sizeof(stStreamProfile.strRtspMainUrl))
                                       : BuildRtmpUrl(
                                             CopyAbiString(m_stConfig.strHttpHost,
                                                           sizeof(m_stConfig.strHttpHost)),
                                             m_stConfig.nRtmpPort,
                                             strDeviceSn);
    if (strLiveUrl.empty())
    {
        strError = "selected platform live URL is empty";
        return false;
    }
    if (!m_stHttpClient.RegisterDevice(stDeviceProfile,
                                       strLiveUrl,
                                       bUseRtsp ? "rtsp" : "rtmp",
                                       strError))
    {
        return false;
    }
    {
        std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
        m_stStatus.bDeviceRegistered = TRUE;
    }
    return true;
}

bool CPlatformModule::PublishMqttRegistration()
{
    NET_PlatformDeviceProfile_S stDeviceProfile;
    NET_PlatformStreamProfile_S stStreamProfile;
    {
        std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
        stDeviceProfile = m_stDeviceProfile;
        stStreamProfile = m_stStreamProfile;
    }

    CPlatformProtocol::RegisterInput_S stInput;
    stInput.strDeviceSn = CopyAbiString(stDeviceProfile.strSerialNumber,
                                        sizeof(stDeviceProfile.strSerialNumber));
    stInput.strUplinkType = GetUplinkTypeText(stStreamProfile.enUplinkType);
    stInput.strUplinkInterface =
        CopyAbiString(stStreamProfile.strUplinkInterface,
                      sizeof(stStreamProfile.strUplinkInterface));
    stInput.strLocalIp = CopyAbiString(stStreamProfile.strLocalIp,
                                       sizeof(stStreamProfile.strLocalIp));
    if (stInput.strLocalIp.empty())
    {
        stInput.strLocalIp = CopyAbiString(stDeviceProfile.strLocalIp,
                                           sizeof(stDeviceProfile.strLocalIp));
    }
    const bool bUseRtsp = ShouldUseRtsp(stStreamProfile.enUplinkType);
    stInput.strStreamMode = bUseRtsp ? "rtsp" : "rtmp";
    stInput.strRtspUrl = CopyAbiString(stStreamProfile.strRtspMainUrl,
                                       sizeof(stStreamProfile.strRtspMainUrl));
    stInput.strRtspAccount = CopyAbiString(stStreamProfile.strRtspAccount,
                                           sizeof(stStreamProfile.strRtspAccount));
    stInput.strRtspPassword = CopyAbiString(stStreamProfile.strRtspPassword,
                                            sizeof(stStreamProfile.strRtspPassword));
    stInput.strRtmpUrl = BuildRtmpUrl(
        CopyAbiString(m_stConfig.strHttpHost, sizeof(m_stConfig.strHttpHost)),
        m_stConfig.nRtmpPort,
        stInput.strDeviceSn);
    stInput.strPublicKeyPath =
        CopyAbiString(m_stConfig.strRegisterPublicKeyPath,
                      sizeof(m_stConfig.strRegisterPublicKeyPath));
    stInput.strPublicKeyId =
        CopyAbiString(m_stConfig.strRegisterPublicKeyId,
                      sizeof(m_stConfig.strRegisterPublicKeyId));
    stInput.llTimestampMs = CPlatformProtocol::GetCurrentTimestampMs();

    std::string strRequestId;
    std::string strPayload;
    std::string strWarning;
    const bool bBuilt = CPlatformProtocol::BuildRegistration(stInput,
                                                             strRequestId,
                                                             strPayload,
                                                             strWarning);
    CPlatformRegisterCrypto::CleanseString(stInput.strRtspPassword);
    if (!bBuilt)
    {
        SetLastError(-1, strWarning.empty() ? "MQTT registration build failed" : strWarning);
        return false;
    }
    if (!strWarning.empty())
    {
        NETSDK_LOG_MESSAGE_WARN("Platform registration credential warning: %s",
                                strWarning.c_str());
    }
    return m_stMqttTransport.Publish(
        CPlatformProtocol::BuildRegisterTopic(stInput.strDeviceSn),
        strPayload,
        1);
}

bool CPlatformModule::PublishStatus(bool bOnline, const std::string &strReason)
{
    NET_PlatformDeviceProfile_S stDeviceProfile;
    {
        std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
        stDeviceProfile = m_stDeviceProfile;
    }
    const std::string strDeviceSn = CopyAbiString(stDeviceProfile.strSerialNumber,
                                                  sizeof(stDeviceProfile.strSerialNumber));
    const std::string strPayload = CPlatformProtocol::BuildStatus(
        strDeviceSn,
        bOnline,
        strReason,
        CPlatformProtocol::GetCurrentTimestampMs());
    return !strPayload.empty() && m_stMqttTransport.Publish(
                                      CPlatformProtocol::BuildStatusTopic(strDeviceSn),
                                      strPayload,
                                      1);
}

void CPlatformModule::OnMqttMessage(const std::string &strTopic,
                                    const std::string &strPayload)
{
    if (!m_bRunning.load() || strPayload.empty())
    {
        return;
    }
    NET_PlatformDeviceProfile_S stDeviceProfile;
    {
        std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
        stDeviceProfile = m_stDeviceProfile;
    }
    const std::string strExpectedTopic = CPlatformProtocol::BuildCommandTopic(
        CopyAbiString(stDeviceProfile.strSerialNumber,
                      sizeof(stDeviceProfile.strSerialNumber)));
    if (strTopic != strExpectedTopic)
    {
        return;
    }

    bool bQueued = false;
    {
        std::lock_guard<std::mutex> stCommandLock(m_mtxCommandQueue);
        if (m_aCommandQueue.size() < PLATFORM_MODULE_MAX_COMMAND_QUEUE_SIZE)
        {
            m_aCommandQueue.push_back({strTopic, strPayload});
            bQueued = true;
        }
    }
    if (bQueued)
    {
        m_cvCommandQueue.notify_one();
    }
    else
    {
        {
            std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
            ++m_stStatus.uDroppedCommandCount;
        }
        SetLastError(-1, "platform command queue is full");
    }
}

void CPlatformModule::OnMqttConnectionChanged(bool bConnected,
                                              const std::string &strReason)
{
    m_bMqttConnected.store(bConnected);
    {
        std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
        m_stStatus.bMqttConnected = bConnected ? TRUE : FALSE;
    }
    if (bConnected)
    {
        m_bMqttRegistrationPending.store(true);
        m_cvMaintenance.notify_all();
    }
    else if (m_bRunning.load())
    {
        SetLastError(-1, strReason.empty() ? "MQTT disconnected" : strReason);
    }
    NotifyRuntimeStatus();
}

void CPlatformModule::CommandLoop()
{
    while (m_bRunning.load())
    {
        CommandJob_S stJob;
        {
            std::unique_lock<std::mutex> stCommandLock(m_mtxCommandQueue);
            m_cvCommandQueue.wait(stCommandLock,
                                  [this]()
                                  {
                                      return !m_bRunning.load() || !m_aCommandQueue.empty();
                                  });
            if (!m_bRunning.load())
            {
                break;
            }
            stJob = std::move(m_aCommandQueue.front());
            m_aCommandQueue.pop_front();
        }

        CPlatformProtocol::Command_S stCommand;
        std::string strError;
        if (!CPlatformProtocol::ParseCommand(stJob.strPayload, stCommand, strError))
        {
            SetLastError(-1, strError);
            continue;
        }

        CPlatformCommandRouter::Result_S stResult;
        m_stCommandRouter.Execute(stCommand.strCommand, stCommand.strData, stResult);
        const std::string strResponse = CPlatformProtocol::BuildResponse(
            stCommand.strCommand,
            stCommand.strRequestId,
            stResult.nReturn,
            stResult.strData,
            stResult.strMessage);
        NET_PlatformDeviceProfile_S stDeviceProfile;
        {
            std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
            stDeviceProfile = m_stDeviceProfile;
        }
        const std::string strDeviceSn = CopyAbiString(stDeviceProfile.strSerialNumber,
                                                      sizeof(stDeviceProfile.strSerialNumber));
        if (strResponse.empty() || !m_stMqttTransport.Publish(
                                       CPlatformProtocol::BuildResponseTopic(strDeviceSn),
                                       strResponse,
                                       1))
        {
            SetLastError(-1, "platform command response publication failed");
        }
    }
}

void CPlatformModule::TransferLoop()
{
    while (m_bRunning.load())
    {
        EventJob_S stJob;
        {
            std::unique_lock<std::mutex> stTransferLock(m_mtxTransferQueue);
            m_cvTransferQueue.wait(stTransferLock,
                                   [this]()
                                   {
                                       return !m_bRunning.load() || !m_aTransferQueue.empty();
                                   });
            if (!m_bRunning.load())
            {
                break;
            }
            stJob = std::move(m_aTransferQueue.front());
            m_aTransferQueue.pop_front();
        }
        ProcessEventImage(stJob);
    }
}

void CPlatformModule::MaintenanceLoop()
{
    const unsigned int uHeartbeatSec = m_stConfig.uHeartbeatIntervalSec > 0
                                           ? m_stConfig.uHeartbeatIntervalSec
                                           : PLATFORM_MODULE_DEFAULT_HEARTBEAT_SEC;
    auto stNextHeartbeat = std::chrono::steady_clock::now() +
                           std::chrono::seconds(uHeartbeatSec);
    auto stNextHttpAttempt = std::chrono::steady_clock::now();

    while (m_bRunning.load())
    {
        if (m_bRefreshRequested.exchange(false))
        {
            std::string strError;
            if (!RefreshProfiles(strError) || !ApplySelectedStream(strError))
            {
                SetLastError(-1, strError);
            }
            m_bHttpRegistrationPending.store(true);
            if (m_bMqttConnected.load())
            {
                m_bMqttRegistrationPending.store(true);
            }
        }

        if (m_bMqttConnected.load() &&
            m_bMqttRegistrationPending.exchange(false))
        {
            if (!PublishMqttRegistration())
            {
                SetLastError(-1, "MQTT device registration publication failed");
            }
            if (!PublishStatus(true, "connect"))
            {
                SetLastError(-1, "MQTT online status publication failed");
            }
        }

        const auto stNow = std::chrono::steady_clock::now();
        if (m_bHttpRegistrationPending.load() && stNow >= stNextHttpAttempt)
        {
            std::string strError;
            if (AuthenticateAndRegister(strError))
            {
                m_bHttpRegistrationPending.store(false);
                NotifyRuntimeStatus();
            }
            else
            {
                m_stHttpClient.ClearSession();
                {
                    std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
                    m_stStatus.bHttpAuthenticated = FALSE;
                    m_stStatus.bDeviceRegistered = FALSE;
                }
                SetLastError(-1, strError);
                stNextHttpAttempt = stNow +
                                    std::chrono::seconds(PLATFORM_MODULE_HTTP_RETRY_SEC);
            }
        }

        if (stNow >= stNextHeartbeat)
        {
            if (m_bMqttConnected.load() && !PublishStatus(true, "heartbeat"))
            {
                SetLastError(-1, "MQTT heartbeat publication failed");
            }
            stNextHeartbeat = stNow + std::chrono::seconds(uHeartbeatSec);
        }

        std::unique_lock<std::mutex> stMaintenanceLock(m_mtxMaintenance);
        m_cvMaintenance.wait_for(
            stMaintenanceLock,
            std::chrono::milliseconds(PLATFORM_MODULE_MAINTENANCE_POLL_MS),
            [this]()
            {
                return !m_bRunning.load() || m_bRefreshRequested.load() ||
                       m_bMqttRegistrationPending.load();
            });
    }
}

void CPlatformModule::ProcessEventImage(const EventJob_S &stJob)
{
    std::string strImagePath = stJob.strImagePath;
    CPlatformHttpClient::EventImageResponse_S stResponse;
    bool bUploadOk = false;
    std::string strError;

    if (!ResolveEventImage(stJob, strImagePath))
    {
        strError = "event image was not available before timeout";
    }
    else
    {
        const auto stAuthenticationDeadline = std::chrono::steady_clock::now() +
                                              std::chrono::seconds(PLATFORM_MODULE_HTTP_RETRY_SEC);
        while (m_bRunning.load() && !m_stHttpClient.IsAuthenticated() &&
               std::chrono::steady_clock::now() < stAuthenticationDeadline)
        {
            m_bHttpRegistrationPending.store(true);
            m_cvMaintenance.notify_all();
            std::unique_lock<std::mutex> stTransferLock(m_mtxTransferQueue);
            m_cvTransferQueue.wait_for(
                stTransferLock,
                std::chrono::milliseconds(PLATFORM_MODULE_MAINTENANCE_POLL_MS),
                [this]()
                {
                    return !m_bRunning.load();
                });
        }

        NET_PlatformDeviceProfile_S stDeviceProfile;
        {
            std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
            stDeviceProfile = m_stDeviceProfile;
        }
        CPlatformHttpClient::EventImageRequest_S stRequest;
        stRequest.strDeviceSn = CopyAbiString(stDeviceProfile.strSerialNumber,
                                              sizeof(stDeviceProfile.strSerialNumber));
        stRequest.nEventType = stJob.nEventType;
        stRequest.strEventName = stJob.strEventName;
        stRequest.nChannel = stJob.nChannel;
        stRequest.llTimestampMs = stJob.llTimestampMs;
        stRequest.strRequestId = stJob.strRequestId;
        stRequest.strImagePath = strImagePath;
        bUploadOk = m_stHttpClient.UploadEventImage(stRequest, stResponse, strError);
        if (bUploadOk)
        {
            std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
            ++m_stStatus.uUploadedImageCount;
        }
        else
        {
            m_stHttpClient.ClearSession();
            m_bHttpRegistrationPending.store(true);
            m_cvMaintenance.notify_all();
        }
    }

    PublishEventImageResult(stJob, stResponse, bUploadOk, strError);
}

bool CPlatformModule::PublishEventImageResult(
    const EventJob_S &stJob,
    const CPlatformHttpClient::EventImageResponse_S &stResponse,
    bool bUploadOk,
    const std::string &strError)
{
    const NET_PlatformEventReport_S stEventView = BuildEventView(stJob);
    const std::string strData = CPlatformProtocol::BuildImageUploadData(
        stEventView,
        stJob.strRequestId,
        stResponse,
        bUploadOk,
        strError);
    const std::string strImageRequestId = stJob.strRequestId + "-image";
    const std::string strPayload = CPlatformProtocol::BuildEvent(
        PLATFORM_MODULE_IMAGE_EVENT_COMMAND,
        strImageRequestId,
        strData);

    NET_PlatformDeviceProfile_S stDeviceProfile;
    {
        std::lock_guard<std::mutex> stProfileLock(m_mtxProfiles);
        stDeviceProfile = m_stDeviceProfile;
    }
    const std::string strDeviceSn = CopyAbiString(stDeviceProfile.strSerialNumber,
                                                  sizeof(stDeviceProfile.strSerialNumber));
    if (strPayload.empty() || !m_stMqttTransport.Publish(
                                   CPlatformProtocol::BuildEventTopic(strDeviceSn),
                                   strPayload,
                                   0))
    {
        SetLastError(-1, "event image result publication failed");
        return false;
    }
    return true;
}

bool CPlatformModule::ResolveEventImage(const EventJob_S &stJob,
                                        std::string &strImagePath)
{
    if (IsReadableImageFile(strImagePath))
    {
        return true;
    }
    if (!stJob.bResolveImageIfMissing)
    {
        return false;
    }

    NET_PlatformHostCallbacks_S stCallbacks;
    {
        std::lock_guard<std::mutex> stHostLock(m_mtxHost);
        stCallbacks = m_stHostCallbacks;
    }
    if (stCallbacks.fnResolveEventImage == nullptr)
    {
        return false;
    }

    const unsigned int uTimeoutMs = m_stConfig.uImageWaitTimeoutMs > 0
                                        ? m_stConfig.uImageWaitTimeoutMs
                                        : PLATFORM_MODULE_DEFAULT_IMAGE_WAIT_MS;
    const unsigned int uPollMs = m_stConfig.uImageWaitIntervalMs > 0
                                     ? m_stConfig.uImageWaitIntervalMs
                                     : PLATFORM_MODULE_DEFAULT_IMAGE_POLL_MS;
    const auto stDeadline = std::chrono::steady_clock::now() +
                            std::chrono::milliseconds(uTimeoutMs);
    const NET_PlatformEventReport_S stEventView = BuildEventView(stJob);
    std::vector<char> aPath(NET_PLATFORM_PATH_LENGTH, '\0');

    while (m_bRunning.load())
    {
        std::fill(aPath.begin(), aPath.end(), '\0');
        if (stCallbacks.fnResolveEventImage(stCallbacks.pUserData,
                                            &stEventView,
                                            aPath.data(),
                                            static_cast<UINT32>(aPath.size())) == 0)
        {
            aPath.back() = '\0';
            strImagePath.assign(aPath.data());
            if (IsReadableImageFile(strImagePath))
            {
                return true;
            }
        }
        if (std::chrono::steady_clock::now() >= stDeadline)
        {
            break;
        }
        std::unique_lock<std::mutex> stTransferLock(m_mtxTransferQueue);
        m_cvTransferQueue.wait_for(stTransferLock,
                                   std::chrono::milliseconds(uPollMs),
                                   [this]()
                                   {
                                       return !m_bRunning.load();
                                   });
    }
    return false;
}

NET_PlatformEventReport_S CPlatformModule::BuildEventView(const EventJob_S &stJob)
{
    NET_PlatformEventReport_S stEvent;
    std::memset(&stEvent, 0, sizeof(stEvent));
    stEvent.uStructSize = sizeof(stEvent);
    stEvent.uVersion = NET_PLATFORM_ABI_VERSION;
    stEvent.pCommand = stJob.strCommand.c_str();
    stEvent.pRequestId = stJob.strRequestId.c_str();
    stEvent.pDataJson = stJob.strDataJson.c_str();
    stEvent.nEventType = stJob.nEventType;
    stEvent.pEventName = stJob.strEventName.c_str();
    stEvent.nChannel = stJob.nChannel;
    stEvent.llTimestampMs = stJob.llTimestampMs;
    stEvent.pImagePath = stJob.strImagePath.c_str();
    stEvent.bUploadImage = stJob.bUploadImage ? TRUE : FALSE;
    stEvent.bResolveImageIfMissing = stJob.bResolveImageIfMissing ? TRUE : FALSE;
    return stEvent;
}

int CPlatformModule::ExecuteFallbackCommand(const std::string &strCommand,
                                            const std::string &strDataJson,
                                            std::string &strResultJson)
{
    NET_PlatformHostCallbacks_S stCallbacks;
    {
        std::lock_guard<std::mutex> stHostLock(m_mtxHost);
        stCallbacks = m_stHostCallbacks;
    }
    if (stCallbacks.fnExecuteCommand == nullptr)
    {
        return static_cast<int>(NET_E_CMD_NOT_SUPPORT);
    }

    std::vector<char> aResult(NET_PLATFORM_COMMAND_RESULT_LENGTH, '\0');
    const int nReturn = stCallbacks.fnExecuteCommand(
        stCallbacks.pUserData,
        strCommand.c_str(),
        strDataJson.c_str(),
        aResult.data(),
        static_cast<UINT32>(aResult.size()));
    aResult.back() = '\0';
    strResultJson.assign(aResult.data());
    return nReturn;
}

void CPlatformModule::SetLastError(int nError, const std::string &strError)
{
    {
        std::lock_guard<std::mutex> stStatusLock(m_mtxStatus);
        m_stStatus.nLastError = nError;
        std::memset(m_stStatus.strLastError, 0, sizeof(m_stStatus.strLastError));
        CopyToAbiBuffer(strError,
                        m_stStatus.strLastError,
                        sizeof(m_stStatus.strLastError));
    }
    if (!strError.empty())
    {
        NETSDK_LOG_MESSAGE_WARN("Platform runtime state: %s", strError.c_str());
    }
    NotifyRuntimeStatus();
}

void CPlatformModule::NotifyRuntimeStatus()
{
    NET_PlatformHostCallbacks_S stCallbacks;
    {
        std::lock_guard<std::mutex> stHostLock(m_mtxHost);
        stCallbacks = m_stHostCallbacks;
    }
    if (stCallbacks.fnRuntimeStatus == nullptr)
    {
        return;
    }

    NET_PlatformRuntimeStatus_S stStatus;
    std::memset(&stStatus, 0, sizeof(stStatus));
    stStatus.uStructSize = sizeof(stStatus);
    if (GetStatus(&stStatus))
    {
        stCallbacks.fnRuntimeStatus(stCallbacks.pUserData, &stStatus);
    }
}
