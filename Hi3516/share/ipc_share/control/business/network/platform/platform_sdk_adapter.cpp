/**
 * @file platform_sdk_adapter.cpp
 * @author Codex
 * @date 2026-08-22
 * @brief Implements the IPC host adapter for the SDK-owned platform runtime.
 * @change 2026-08-22 Codex Initial implementation for complete platform migration.
 */

#if CAP_GARBAGE_STATION_PLATFORM

#include "platform_sdk_adapter.h"

#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ifaddrs.h>
#include <netdb.h>
#include <set>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "IpcRet.h"
#include "av_configure.h"
#include "capture_ctrl.h"
#include "capture_database.h"
#include "dlog.h"
#include "event_define.h"
#include "mqtt_sdk_gateway.h"
#include "mqtt_topic_define.h"
#include "network_manage.h"
#include "path_define.h"
#include "push_stream.h"
#include "rtsp_server.h"
#include "storage_manage.h"
#include "system_manage.h"
#include "user_define.h"
#include "user_manage.h"

namespace
{
constexpr const char *PLATFORM_SDK_DEFAULT_HTTP_HOST = "183.129.224.253";
constexpr int PLATFORM_SDK_DEFAULT_HTTP_PORT = 4910;
constexpr int PLATFORM_SDK_DEFAULT_RTMP_PORT = 4920;
constexpr const char *PLATFORM_SDK_MQTT_LIBRARY = "/opt/cam/lib/libpaho-mqtt3a.so.1";
constexpr const char *PLATFORM_SDK_IMAGE_DIRECTORY = "/opt/course/upload";
constexpr const char *PLATFORM_SDK_REGISTER_PUBLIC_KEY =
    CA_MQTT_TRUST_KEY "platform_register_public.pem";
constexpr const char *PLATFORM_SDK_REGISTER_KEY_ID = "platform-rsa-2026-08";
constexpr unsigned int PLATFORM_SDK_HEARTBEAT_INTERVAL_SEC = 30U;
constexpr unsigned int PLATFORM_SDK_IMAGE_WAIT_TIMEOUT_MS = 3000U;
constexpr unsigned int PLATFORM_SDK_IMAGE_WAIT_INTERVAL_MS = 500U;

/**
 * @brief Copies text into a fixed character buffer with guaranteed termination.
 * @author Codex
 * @param [in] strValue Source text.
 * @param [out] pBuffer Destination buffer.
 * @param [in] uCapacity Destination capacity in bytes.
 * @return true when the complete value fits, otherwise false after truncation.
 */
static bool copy_to_fixed_buffer(const std::string &strValue,
                                 char *pBuffer,
                                 std::size_t uCapacity)
{
    if (pBuffer == nullptr || uCapacity == 0U)
    {
        return false;
    }

    const std::size_t uCopyLength = std::min(strValue.size(), uCapacity - 1U);
    std::memset(pBuffer, 0, uCapacity);
    if (uCopyLength > 0U)
    {
        std::memcpy(pBuffer, strValue.data(), uCopyLength);
    }
    return uCopyLength == strValue.size();
}

/**
 * @brief Tests whether one path names a nonempty regular file.
 * @author Codex
 * @param [in] strPath Candidate path.
 * @return true when the file is suitable for image upload.
 */
static bool is_readable_regular_file(const std::string &strPath)
{
    struct stat stFileInfo;
    return !strPath.empty() && stat(strPath.c_str(), &stFileInfo) == 0 &&
           S_ISREG(stFileInfo.st_mode) && stFileInfo.st_size > 0;
}

/**
 * @brief Tests a case-insensitive network-interface prefix.
 * @author Codex
 * @param [in] strValue Interface name.
 * @param [in] pPrefix Prefix text.
 * @return true when the interface starts with the prefix.
 */
static bool starts_with_case_insensitive(const std::string &strValue,
                                         const char *pPrefix)
{
    if (pPrefix == nullptr)
    {
        return false;
    }

    const std::size_t uPrefixLength = std::strlen(pPrefix);
    if (strValue.size() < uPrefixLength)
    {
        return false;
    }
    for (std::size_t i = 0; i < uPrefixLength; ++i)
    {
        const unsigned char chValue = static_cast<unsigned char>(strValue[i]);
        const unsigned char chPrefix = static_cast<unsigned char>(pPrefix[i]);
        if (std::tolower(chValue) != std::tolower(chPrefix))
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief Classifies an active route interface for stream-mode selection.
 * @author Codex
 * @param [in] strInterface Network-interface name.
 * @return NET_PlatformUplinkType_EN value.
 */
static int classify_uplink_interface(const std::string &strInterface)
{
    if (starts_with_case_insensitive(strInterface, "wl"))
    {
        return NET_PLATFORM_UPLINK_WIRELESS;
    }
    if (starts_with_case_insensitive(strInterface, "wwan") ||
        starts_with_case_insensitive(strInterface, "ppp") ||
        starts_with_case_insensitive(strInterface, "rmnet") ||
        starts_with_case_insensitive(strInterface, "usb"))
    {
        return NET_PLATFORM_UPLINK_CELLULAR;
    }
    if (starts_with_case_insensitive(strInterface, "eth") ||
        starts_with_case_insensitive(strInterface, "en") ||
        starts_with_case_insensitive(strInterface, "bond") ||
        starts_with_case_insensitive(strInterface, "br") ||
        starts_with_case_insensitive(strInterface, "lan"))
    {
        return NET_PLATFORM_UPLINK_WIRED;
    }
    return NET_PLATFORM_UPLINK_UNKNOWN;
}

/**
 * @brief Compares two socket addresses of the same address family.
 * @author Codex
 * @param [in] pLeft First address.
 * @param [in] pRight Second address.
 * @return true when both addresses identify the same local IP.
 */
static bool socket_addresses_equal(const struct sockaddr *pLeft,
                                   const struct sockaddr *pRight)
{
    if (pLeft == nullptr || pRight == nullptr || pLeft->sa_family != pRight->sa_family)
    {
        return false;
    }
    if (pLeft->sa_family == AF_INET)
    {
        const struct sockaddr_in *pLeftIpv4 =
            reinterpret_cast<const struct sockaddr_in *>(pLeft);
        const struct sockaddr_in *pRightIpv4 =
            reinterpret_cast<const struct sockaddr_in *>(pRight);
        return pLeftIpv4->sin_addr.s_addr == pRightIpv4->sin_addr.s_addr;
    }
    if (pLeft->sa_family == AF_INET6)
    {
        const struct sockaddr_in6 *pLeftIpv6 =
            reinterpret_cast<const struct sockaddr_in6 *>(pLeft);
        const struct sockaddr_in6 *pRightIpv6 =
            reinterpret_cast<const struct sockaddr_in6 *>(pRight);
        return std::memcmp(&pLeftIpv6->sin6_addr,
                           &pRightIpv6->sin6_addr,
                           sizeof(pLeftIpv6->sin6_addr)) == 0;
    }
    return false;
}

/**
 * @brief Resolves the actual interface and local IP used to reach a platform endpoint.
 * @author Codex
 * @param [in] strHost Platform host name or IP.
 * @param [in] nPort Platform port.
 * @param [out] strInterface Route interface name.
 * @param [out] strLocalIp Route source IP.
 * @return true when the operating system route can be mapped to an interface.
 */
static bool resolve_route_interface(const std::string &strHost,
                                    int nPort,
                                    std::string &strInterface,
                                    std::string &strLocalIp)
{
    strInterface.clear();
    strLocalIp.clear();
    if (strHost.empty() || nPort <= 0 || nPort > 65535)
    {
        return false;
    }

    struct addrinfo stHints;
    std::memset(&stHints, 0, sizeof(stHints));
    stHints.ai_family = AF_UNSPEC;
    stHints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *pAddressList = nullptr;
    const std::string strPort = std::to_string(nPort);
    if (getaddrinfo(strHost.c_str(), strPort.c_str(), &stHints, &pAddressList) != 0)
    {
        return false;
    }

    bool bResolved = false;
    for (struct addrinfo *pAddress = pAddressList;
         pAddress != nullptr && !bResolved;
         pAddress = pAddress->ai_next)
    {
        const int nSocket = socket(pAddress->ai_family, SOCK_DGRAM, 0);
        if (nSocket < 0)
        {
            continue;
        }
        if (connect(nSocket, pAddress->ai_addr, pAddress->ai_addrlen) != 0)
        {
            close(nSocket);
            continue;
        }

        struct sockaddr_storage stLocalAddress;
        std::memset(&stLocalAddress, 0, sizeof(stLocalAddress));
        socklen_t uLocalLength = sizeof(stLocalAddress);
        if (getsockname(nSocket,
                        reinterpret_cast<struct sockaddr *>(&stLocalAddress),
                        &uLocalLength) != 0)
        {
            close(nSocket);
            continue;
        }
        close(nSocket);

        char aLocalIp[NI_MAXHOST] = {0};
        if (getnameinfo(reinterpret_cast<struct sockaddr *>(&stLocalAddress),
                        uLocalLength,
                        aLocalIp,
                        sizeof(aLocalIp),
                        nullptr,
                        0,
                        NI_NUMERICHOST) != 0)
        {
            continue;
        }

        struct ifaddrs *pInterfaces = nullptr;
        if (getifaddrs(&pInterfaces) != 0)
        {
            continue;
        }
        for (struct ifaddrs *pInterface = pInterfaces;
             pInterface != nullptr;
             pInterface = pInterface->ifa_next)
        {
            if (pInterface->ifa_addr == nullptr || pInterface->ifa_name == nullptr)
            {
                continue;
            }
            if (socket_addresses_equal(
                    reinterpret_cast<struct sockaddr *>(&stLocalAddress),
                    pInterface->ifa_addr))
            {
                strInterface.assign(pInterface->ifa_name);
                strLocalIp.assign(aLocalIp);
                bResolved = true;
                break;
            }
        }
        freeifaddrs(pInterfaces);
    }

    freeaddrinfo(pAddressList);
    return bResolved;
}

/**
 * @brief Parses a finite nonnegative storage value.
 * @author Codex
 * @param [in] strValue Storage text.
 * @param [out] fValue Parsed value.
 * @return true when the complete string contains a valid value.
 */
static bool parse_storage_value(const std::string &strValue, float &fValue)
{
    if (strValue.empty())
    {
        return false;
    }
    errno = 0;
    char *pEnd = nullptr;
    const float fParsedValue = std::strtof(strValue.c_str(), &pEnd);
    if (errno != 0 || pEnd == strValue.c_str() || pEnd == nullptr || *pEnd != '\0' ||
        !std::isfinite(fParsedValue) || fParsedValue < 0.0F)
    {
        return false;
    }
    fValue = fParsedValue;
    return true;
}

/**
 * @brief Calculates used storage while preserving the legacy decimal format.
 * @author Codex
 * @param [in] strTotal Total storage text.
 * @param [in] strRemaining Remaining storage text.
 * @return Used storage text or a conservative legacy fallback.
 */
static std::string calculate_used_storage(const std::string &strTotal,
                                          const std::string &strRemaining)
{
    float fTotal = 0.0F;
    float fRemaining = 0.0F;
    if (!parse_storage_value(strTotal, fTotal) ||
        !parse_storage_value(strRemaining, fRemaining))
    {
        return "10";
    }
    const float fUsed = std::max(0.0F, fTotal - fRemaining);
    return std::to_string(fUsed);
}

/**
 * @brief Builds the SDK ABI configuration from one IPC configuration.
 * @author Codex
 * @param [in] stInfo IPC platform configuration.
 * @return Fully initialized SDK platform configuration.
 */
static NET_PlatformConfig_S build_sdk_platform_config(
    const Network::Platform_Info_t &stInfo)
{
    NET_PlatformConfig_S stConfig;
    std::memset(&stConfig, 0, sizeof(stConfig));
    stConfig.uStructSize = sizeof(stConfig);
    stConfig.uVersion = NET_PLATFORM_ABI_VERSION;
    stConfig.bEnable = stInfo.enable ? TRUE : FALSE;

    const std::string strHttpHost = stInfo.Custom
                                        ? stInfo.server_ip
                                        : PLATFORM_SDK_DEFAULT_HTTP_HOST;
    const int nHttpPort = stInfo.Custom
                              ? stInfo.server_port
                              : PLATFORM_SDK_DEFAULT_HTTP_PORT;
    const std::string strMqttHost = stInfo.Custom
                                        ? stInfo.server_ip
                                        : MQTT_PLATFORM_DEFAULT_BROKER;
    const int nMqttPort = stInfo.Custom
                              ? stInfo.mqtt_port
                              : MQTT_PLATFORM_DEFAULT_PORT;
    const std::string strMqttUser = stInfo.Custom
                                        ? stInfo.user
                                        : MQTT_PLATFORM_DEFAULT_USERNAME;
    const std::string strMqttPassword = stInfo.Custom
                                            ? stInfo.password
                                            : MQTT_PLATFORM_DEFAULT_PASSWORD;

    copy_to_fixed_buffer(strHttpHost, stConfig.strHttpHost, sizeof(stConfig.strHttpHost));
    copy_to_fixed_buffer(strMqttHost, stConfig.strMqttHost, sizeof(stConfig.strMqttHost));
    copy_to_fixed_buffer(stInfo.user,
                         stConfig.strPlatformUser,
                         sizeof(stConfig.strPlatformUser));
    copy_to_fixed_buffer(stInfo.password,
                         stConfig.strPlatformPassword,
                         sizeof(stConfig.strPlatformPassword));
    copy_to_fixed_buffer(strMqttUser,
                         stConfig.strMqttUser,
                         sizeof(stConfig.strMqttUser));
    copy_to_fixed_buffer(strMqttPassword,
                         stConfig.strMqttPassword,
                         sizeof(stConfig.strMqttPassword));
    copy_to_fixed_buffer(PLATFORM_SDK_REGISTER_PUBLIC_KEY,
                         stConfig.strRegisterPublicKeyPath,
                         sizeof(stConfig.strRegisterPublicKeyPath));
    copy_to_fixed_buffer(PLATFORM_SDK_REGISTER_KEY_ID,
                         stConfig.strRegisterPublicKeyId,
                         sizeof(stConfig.strRegisterPublicKeyId));
    copy_to_fixed_buffer(PLATFORM_SDK_MQTT_LIBRARY,
                         stConfig.strMqttRuntimeLibrary,
                         sizeof(stConfig.strMqttRuntimeLibrary));
    copy_to_fixed_buffer(PLATFORM_SDK_IMAGE_DIRECTORY,
                         stConfig.strImageDownloadDirectory,
                         sizeof(stConfig.strImageDownloadDirectory));

    stConfig.nHttpPort = nHttpPort;
    stConfig.nMqttPort = nMqttPort;
    stConfig.nRtmpPort = stInfo.rtmp_port > 0
                             ? stInfo.rtmp_port
                             : PLATFORM_SDK_DEFAULT_RTMP_PORT;
    stConfig.uHeartbeatIntervalSec = PLATFORM_SDK_HEARTBEAT_INTERVAL_SEC;
    stConfig.uImageWaitTimeoutMs = PLATFORM_SDK_IMAGE_WAIT_TIMEOUT_MS;
    stConfig.uImageWaitIntervalMs = PLATFORM_SDK_IMAGE_WAIT_INTERVAL_MS;
    return stConfig;
}

/**
 * @brief Returns the endpoint used for actual MQTT route selection.
 * @author Codex
 * @param [in] stInfo IPC platform configuration.
 * @param [out] strHost MQTT host.
 * @param [out] nPort MQTT port.
 * @return No return value.
 */
static void get_mqtt_route_endpoint(const Network::Platform_Info_t &stInfo,
                                    std::string &strHost,
                                    int &nPort)
{
    strHost = stInfo.Custom ? stInfo.server_ip : MQTT_PLATFORM_DEFAULT_BROKER;
    nPort = stInfo.Custom ? stInfo.mqtt_port : MQTT_PLATFORM_DEFAULT_PORT;
}

/**
 * @brief Queries the latest face-capture image from the capture database.
 * @author Codex
 * @param [in] enEventType Event type stored with the image.
 * @param [out] strImagePath Latest image path.
 * @return true when a readable image is found.
 */
static bool query_latest_capture_image(Event::Type_E enEventType,
                                       std::string &strImagePath)
{
    Db::Element stElement(Db::INFO_CAPTURE_EVENT_TYPE,
                          static_cast<int>(enEventType));
    std::vector<Capture_NS::CaptureInfo_S> aCaptureInfos;
    if (Db::CCaptureDatabase::instance()->find(stElement, aCaptureInfos) != OK ||
        aCaptureInfos.empty())
    {
        return false;
    }
    strImagePath = aCaptureInfos.back().strImagePath;
    return is_readable_regular_file(strImagePath);
}
}

CPlatformSdkAdapter::CPlatformSdkAdapter()
    : m_bCallbacksRegistered(false)
{
    std::memset(&m_stLastStatus, 0, sizeof(m_stLastStatus));
    m_stLastStatus.uStructSize = sizeof(m_stLastStatus);
    m_stLastStatus.uVersion = NET_PLATFORM_ABI_VERSION;
}

CPlatformSdkAdapter::~CPlatformSdkAdapter()
{
    stop_runtime();
}

int CPlatformSdkAdapter::register_host_callbacks()
{
    std::lock_guard<std::recursive_mutex> stLifecycleLock(m_mtxLifecycle);
    if (m_bCallbacksRegistered.load())
    {
        return OK;
    }

    NET_PlatformHostCallbacks_S stCallbacks;
    std::memset(&stCallbacks, 0, sizeof(stCallbacks));
    stCallbacks.uStructSize = sizeof(stCallbacks);
    stCallbacks.uVersion = NET_PLATFORM_ABI_VERSION;
    stCallbacks.pUserData = this;
    stCallbacks.fnGetDeviceProfile = get_device_profile_callback;
    stCallbacks.fnGetStreamProfile = get_stream_profile_callback;
    stCallbacks.fnApplyRtmpStream = apply_rtmp_stream_callback;
    stCallbacks.fnResolveEventImage = resolve_event_image_callback;
    stCallbacks.fnRuntimeStatus = runtime_status_callback;
    stCallbacks.fnExecuteCommand = execute_command_callback;

    if (NET_serverRegisterPlatformHostCallbacks(&stCallbacks) == FALSE)
    {
        dlog_error("SDK 平台主机回调注册失败");
        return ERR;
    }
    m_bCallbacksRegistered.store(true);
    return OK;
}

int CPlatformSdkAdapter::apply_config(const Network::Platform_Info_t &stInfo)
{
    std::lock_guard<std::recursive_mutex> stLifecycleLock(m_mtxLifecycle);
    Network::Platform_Info_t stPreviousInfo;
    {
        std::lock_guard<std::mutex> stConfigLock(m_mtxConfig);
        stPreviousInfo = m_stPlatformInfo;
        m_stPlatformInfo = stInfo;
    }

    const NET_PlatformConfig_S stConfig = build_sdk_platform_config(stInfo);
    if (NET_serverPlatformApplyConfig(&stConfig) == FALSE)
    {
        std::lock_guard<std::mutex> stConfigLock(m_mtxConfig);
        m_stPlatformInfo = stPreviousInfo;
        dlog_error("SDK 平台配置应用失败: enable[%d], custom[%d]",
                   stInfo.enable ? 1 : 0,
                   stInfo.Custom ? 1 : 0);
        return ERR_PARAM;
    }
    return OK;
}

int CPlatformSdkAdapter::start_runtime()
{
    std::lock_guard<std::recursive_mutex> stLifecycleLock(m_mtxLifecycle);
    if (!m_bCallbacksRegistered.load() && register_host_callbacks() != OK)
    {
        return ERR;
    }
    if (NET_serverPlatformStart() == FALSE)
    {
        dlog_error("SDK 平台运行时启动失败");
        return ERR;
    }
    return OK;
}

int CPlatformSdkAdapter::stop_runtime()
{
    std::lock_guard<std::recursive_mutex> stLifecycleLock(m_mtxLifecycle);
    if (NET_serverPlatformStop() == FALSE)
    {
        dlog_error("SDK 平台运行时停止失败");
        return ERR;
    }
    return OK;
}

int CPlatformSdkAdapter::restart_runtime()
{
    std::lock_guard<std::recursive_mutex> stLifecycleLock(m_mtxLifecycle);
    const int nStopResult = stop_runtime();
    if (nStopResult != OK)
    {
        return nStopResult;
    }
    return start_runtime();
}

int CPlatformSdkAdapter::notify_network_changed()
{
    std::lock_guard<std::recursive_mutex> stLifecycleLock(m_mtxLifecycle);
    Network::Platform_Info_t stInfo;
    {
        std::lock_guard<std::mutex> stConfigLock(m_mtxConfig);
        stInfo = m_stPlatformInfo;
    }
    if (!stInfo.enable)
    {
        return OK;
    }

    NET_PlatformRuntimeStatus_S stStatus;
    std::memset(&stStatus, 0, sizeof(stStatus));
    stStatus.uStructSize = sizeof(stStatus);
    if (!get_status(stStatus) || stStatus.bRunning == FALSE)
    {
        return start_runtime();
    }
    if (NET_serverPlatformNotifyNetworkChanged() == FALSE)
    {
        dlog_warn("SDK 平台运行时未接受网络切换通知");
        return ERR;
    }
    return OK;
}

int CPlatformSdkAdapter::report_event(const std::string &strCommand,
                                      const std::string &strRequestId,
                                      const std::string &strDataJson,
                                      int nEventType,
                                      const std::string &strEventName,
                                      int nChannel,
                                      long long llTimestampMs,
                                      const std::string &strImagePath,
                                      bool bUploadImage,
                                      bool bResolveImageIfMissing)
{
    NET_PlatformEventReport_S stEvent;
    std::memset(&stEvent, 0, sizeof(stEvent));
    stEvent.uStructSize = sizeof(stEvent);
    stEvent.uVersion = NET_PLATFORM_ABI_VERSION;
    stEvent.pCommand = strCommand.c_str();
    stEvent.pRequestId = strRequestId.c_str();
    stEvent.pDataJson = strDataJson.empty() ? "{}" : strDataJson.c_str();
    stEvent.nEventType = nEventType;
    stEvent.pEventName = strEventName.c_str();
    stEvent.nChannel = nChannel;
    stEvent.llTimestampMs = static_cast<INT64>(llTimestampMs);
    stEvent.pImagePath = strImagePath.c_str();
    stEvent.bUploadImage = bUploadImage ? TRUE : FALSE;
    stEvent.bResolveImageIfMissing = bResolveImageIfMissing ? TRUE : FALSE;
    return NET_serverPlatformReportEvent(&stEvent) != FALSE ? OK : ERR;
}

int CPlatformSdkAdapter::download_image(const std::string &strUrl,
                                        const std::string &strLocalPath,
                                        long long llExpectedSize)
{
    if (strUrl.empty() || strLocalPath.empty() || llExpectedSize < 0)
    {
        return ERR_PARAM;
    }
    return NET_serverPlatformDownloadImage(strUrl.c_str(),
                                           strLocalPath.c_str(),
                                           static_cast<INT64>(llExpectedSize)) != FALSE
               ? OK
               : ERR;
}

bool CPlatformSdkAdapter::get_status(NET_PlatformRuntimeStatus_S &stStatus) const
{
    std::memset(&stStatus, 0, sizeof(stStatus));
    stStatus.uStructSize = sizeof(stStatus);
    stStatus.uVersion = NET_PLATFORM_ABI_VERSION;
    return NET_serverPlatformGetStatus(&stStatus) != FALSE;
}

bool CPlatformSdkAdapter::wait_until_registered(unsigned int uTimeoutMs)
{
    NET_PlatformRuntimeStatus_S stStatus;
    if (get_status(stStatus) && stStatus.bDeviceRegistered != FALSE)
    {
        return true;
    }

    std::unique_lock<std::mutex> stStatusLock(m_mtxStatus);
    return m_cvStatus.wait_for(
        stStatusLock,
        std::chrono::milliseconds(uTimeoutMs),
        [this]()
        {
            return m_stLastStatus.bHttpAuthenticated != FALSE &&
                   m_stLastStatus.bDeviceRegistered != FALSE;
        });
}

bool CPlatformSdkAdapter::is_device_registered() const
{
    NET_PlatformRuntimeStatus_S stStatus;
    return get_status(stStatus) && stStatus.bDeviceRegistered != FALSE;
}

INT32 STDCALL CPlatformSdkAdapter::get_device_profile_callback(
    LPVOID pUserData,
    pNET_PlatformDeviceProfile_S pProfile)
{
    if (pUserData == nullptr || pProfile == nullptr)
    {
        return ERR_PARAM_NULL;
    }

    System::DeviceInfo_S stDeviceInfo{};
    Network::Info_S stNetworkInfo{};
    Network::PortConfig_S stPortConfig{};
    Video_NS::VideoConfig_S stVideoConfig{};
    StorageManage_NS::StorageManage_S stStorageInfo{};
    if (SystemManage::instance()->get_device_info(stDeviceInfo) != OK ||
        CNetworkManage::instance()->get_system_networkInfo(stNetworkInfo) != OK)
    {
        return ERR;
    }

    stVideoConfig.nId = 0;
    const int nVideoResult = CAVConfigure::instance()->get_configure(stVideoConfig);
    const int nPortResult = CNetworkManage::instance()->get_network_port(stPortConfig);
    const int nStorageResult =
        CStorageManage::instance()->get_storageManage_param(stStorageInfo);

    if (nVideoResult != OK)
    {
        dlog_warn("SDK 平台设备资料读取视频配置失败: ret[%d]", nVideoResult);
    }
    if (nPortResult != OK)
    {
        dlog_warn("SDK 平台设备资料读取端口配置失败: ret[%d]", nPortResult);
    }
    if (nStorageResult != OK)
    {
        dlog_warn("SDK 平台设备资料读取存储配置失败: ret[%d]", nStorageResult);
    }

    const std::string strSerialNumber = stDeviceInfo.serialNumber.empty()
                                            ? std::to_string(stDeviceInfo.deviceID)
                                            : stDeviceInfo.serialNumber;
    const std::string strResolution = nVideoResult == OK
                                          ? std::to_string(
                                                stVideoConfig.stVideoResolution.nWidth) +
                                                "x" +
                                                std::to_string(
                                                    stVideoConfig.stVideoResolution.nHeight)
                                          : std::string();
    const std::string strStorage = stStorageInfo.strAvailableSpace.empty()
                                       ? "10"
                                       : stStorageInfo.strAvailableSpace;
    const std::string strUsedStorage = calculate_used_storage(
        stStorageInfo.strAvailableSpace,
        stStorageInfo.strRecordRemainingSpace);

    copy_to_fixed_buffer(strSerialNumber,
                         pProfile->strSerialNumber,
                         sizeof(pProfile->strSerialNumber));
    copy_to_fixed_buffer(stDeviceInfo.deviceName,
                         pProfile->strDeviceName,
                         sizeof(pProfile->strDeviceName));
    copy_to_fixed_buffer(stDeviceInfo.systemVersion,
                         pProfile->strFirmwareVersion,
                         sizeof(pProfile->strFirmwareVersion));
    copy_to_fixed_buffer(stNetworkInfo.stIp.physicalAddress,
                         pProfile->strMacAddress,
                         sizeof(pProfile->strMacAddress));
    copy_to_fixed_buffer(stNetworkInfo.stIp.ipv4Ip,
                         pProfile->strLocalIp,
                         sizeof(pProfile->strLocalIp));
    copy_to_fixed_buffer(strResolution,
                         pProfile->strResolution,
                         sizeof(pProfile->strResolution));
    copy_to_fixed_buffer(strStorage,
                         pProfile->strStorage,
                         sizeof(pProfile->strStorage));
    copy_to_fixed_buffer(strUsedStorage,
                         pProfile->strUseStorage,
                         sizeof(pProfile->strUseStorage));
    pProfile->nServicePort = nPortResult == OK && stPortConfig.nRtspPort > 0
                                 ? stPortConfig.nRtspPort
                                 : 554;
    return OK;
}

INT32 STDCALL CPlatformSdkAdapter::get_stream_profile_callback(
    LPVOID pUserData,
    pNET_PlatformStreamProfile_S pProfile)
{
    if (pUserData == nullptr || pProfile == nullptr)
    {
        return ERR_PARAM_NULL;
    }

    CPlatformSdkAdapter *pAdapter =
        static_cast<CPlatformSdkAdapter *>(pUserData);
    Network::Platform_Info_t stPlatformInfo;
    {
        std::lock_guard<std::mutex> stConfigLock(pAdapter->m_mtxConfig);
        stPlatformInfo = pAdapter->m_stPlatformInfo;
    }

    std::string strMqttHost;
    int nMqttPort = 0;
    get_mqtt_route_endpoint(stPlatformInfo, strMqttHost, nMqttPort);

    std::string strInterface;
    std::string strLocalIp;
    const bool bRouteResolved = resolve_route_interface(strMqttHost,
                                                        nMqttPort,
                                                        strInterface,
                                                        strLocalIp);
    pProfile->enUplinkType = bRouteResolved
                                 ? classify_uplink_interface(strInterface)
                                 : NET_PLATFORM_UPLINK_UNKNOWN;

    if (!bRouteResolved)
    {
        Network::Info_S stNetworkInfo;
        if (CNetworkManage::instance()->get_system_networkInfo(stNetworkInfo) == OK)
        {
            strInterface = stNetworkInfo.stIp.netName;
            strLocalIp = stNetworkInfo.stIp.ipv4Ip;
        }
        dlog_warn("无法识别 MQTT 实际出口，SDK 将按主动 RTMP 推流处理");
    }

    std::string strRtspMainUrl;
    std::string strRtspSubUrl;
    if (CRtspServer::instance()->isInit())
    {
        const char *pMainUrl = CRtspServer::instance()->getRtspUrl(RTSP_CHN_MAIN, false);
        const char *pSubUrl = CRtspServer::instance()->getRtspUrl(RTSP_CHN_SUB, false);
        strRtspMainUrl = pMainUrl != nullptr ? pMainUrl : "";
        strRtspSubUrl = pSubUrl != nullptr ? pSubUrl : "";
    }

    const std::string strAccount = USER_DEFAULT_NAME;
    const std::string strPassword = CUserManage::instance()->get_passwd(strAccount);
    copy_to_fixed_buffer(strInterface,
                         pProfile->strUplinkInterface,
                         sizeof(pProfile->strUplinkInterface));
    copy_to_fixed_buffer(strLocalIp,
                         pProfile->strLocalIp,
                         sizeof(pProfile->strLocalIp));
    copy_to_fixed_buffer(strRtspMainUrl,
                         pProfile->strRtspMainUrl,
                         sizeof(pProfile->strRtspMainUrl));
    copy_to_fixed_buffer(strRtspSubUrl,
                         pProfile->strRtspSubUrl,
                         sizeof(pProfile->strRtspSubUrl));
    copy_to_fixed_buffer(strAccount,
                         pProfile->strRtspAccount,
                         sizeof(pProfile->strRtspAccount));
    copy_to_fixed_buffer(strPassword,
                         pProfile->strRtspPassword,
                         sizeof(pProfile->strRtspPassword));
    return OK;
}

INT32 STDCALL CPlatformSdkAdapter::apply_rtmp_stream_callback(
    LPVOID pUserData,
    const CHAR *pRtmpUrl,
    BOOL bEnable)
{
    if (pUserData == nullptr)
    {
        return ERR_PARAM_NULL;
    }

    CPlatformSdkAdapter *pAdapter =
        static_cast<CPlatformSdkAdapter *>(pUserData);
    Network::Platform_Info_t stPlatformInfo;
    {
        std::lock_guard<std::mutex> stConfigLock(pAdapter->m_mtxConfig);
        stPlatformInfo = pAdapter->m_stPlatformInfo;
    }
    stPlatformInfo.enable = bEnable != FALSE;
    if (!stPlatformInfo.Custom)
    {
        stPlatformInfo.server_ip = PLATFORM_SDK_DEFAULT_HTTP_HOST;
        stPlatformInfo.rtmp_port = PLATFORM_SDK_DEFAULT_RTMP_PORT;
    }

    if (pRtmpUrl != nullptr && pRtmpUrl[0] != '\0')
    {
        dlog_info("SDK 选择 RTMP 主码流地址: %s", pRtmpUrl);
    }

#if CAP_RTMP_PUSH
    return CPushStream::instance()->restart_rtmp_stream(stPlatformInfo);
#else
    if (bEnable != FALSE)
    {
        dlog_warn("当前产品未启用 RTMP 推流能力，忽略 SDK 启动请求");
    }
    return OK;
#endif
}

INT32 STDCALL CPlatformSdkAdapter::resolve_event_image_callback(
    LPVOID pUserData,
    const NET_PlatformEventReport_S *pEvent,
    CHAR *pImagePath,
    UINT32 uImagePathSize)
{
    if (pUserData == nullptr || pEvent == nullptr || pImagePath == nullptr ||
        uImagePathSize == 0U)
    {
        return ERR_PARAM_NULL;
    }

    std::string strImagePath = pEvent->pImagePath != nullptr
                                   ? pEvent->pImagePath
                                   : "";
    if (!is_readable_regular_file(strImagePath))
    {
        const Event::Type_E enEventType =
            static_cast<Event::Type_E>(pEvent->nEventType);
        if (enEventType == Event::Type_E::FACE_CAPTURE)
        {
            query_latest_capture_image(enEventType, strImagePath);
        }
        else if (enEventType == Event::Type_E::FACE_COMPARE_SUCCESS ||
                 enEventType == Event::Type_E::FACE_COMPARE_FAIL)
        {
            return ERR;
        }
        else
        {
            CCaptureCtrl::instance()->get_event_first_capture_status(
                enEventType,
                strImagePath);
        }
    }

    if (!is_readable_regular_file(strImagePath) ||
        !copy_to_fixed_buffer(strImagePath,
                              pImagePath,
                              static_cast<std::size_t>(uImagePathSize)))
    {
        return ERR;
    }
    return OK;
}

VOID STDCALL CPlatformSdkAdapter::runtime_status_callback(
    LPVOID pUserData,
    const NET_PlatformRuntimeStatus_S *pStatus)
{
    if (pUserData == nullptr || pStatus == nullptr)
    {
        return;
    }

    CPlatformSdkAdapter *pAdapter =
        static_cast<CPlatformSdkAdapter *>(pUserData);
    bool bStateChanged = false;
    bool bErrorChanged = false;
    {
        std::lock_guard<std::mutex> stStatusLock(pAdapter->m_mtxStatus);
        bStateChanged =
            pAdapter->m_stLastStatus.bRunning != pStatus->bRunning ||
            pAdapter->m_stLastStatus.bHttpAuthenticated != pStatus->bHttpAuthenticated ||
            pAdapter->m_stLastStatus.bMqttConnected != pStatus->bMqttConnected ||
            pAdapter->m_stLastStatus.bDeviceRegistered != pStatus->bDeviceRegistered;
        bErrorChanged =
            pAdapter->m_stLastStatus.nLastError != pStatus->nLastError ||
            std::strncmp(pAdapter->m_stLastStatus.strLastError,
                         pStatus->strLastError,
                         sizeof(pStatus->strLastError)) != 0;
        pAdapter->m_stLastStatus = *pStatus;
    }
    pAdapter->m_cvStatus.notify_all();

    if (bStateChanged)
    {
        dlog_info("SDK 平台状态: running[%d], http[%d], mqtt[%d], registered[%d]",
                  pStatus->bRunning != FALSE ? 1 : 0,
                  pStatus->bHttpAuthenticated != FALSE ? 1 : 0,
                  pStatus->bMqttConnected != FALSE ? 1 : 0,
                  pStatus->bDeviceRegistered != FALSE ? 1 : 0);
    }
    if (bErrorChanged && pStatus->nLastError != 0 && pStatus->strLastError[0] != '\0')
    {
        dlog_warn("SDK 平台运行时错误: code[%d], reason[%s]",
                  pStatus->nLastError,
                  pStatus->strLastError);
    }
}

INT32 STDCALL CPlatformSdkAdapter::execute_command_callback(
    LPVOID pUserData,
    const CHAR *pCommand,
    const CHAR *pDataJson,
    CHAR *pResultJson,
    UINT32 uResultCapacity)
{
    if (pUserData == nullptr || pCommand == nullptr || pCommand[0] == '\0' ||
        pResultJson == nullptr || uResultCapacity == 0U)
    {
        return ERR_PARAM_NULL;
    }

    const std::string strCommand(pCommand);
    const std::string strDataJson = pDataJson != nullptr && pDataJson[0] != '\0'
                                        ? pDataJson
                                        : "{}";
    if (!CMqttSdkGateway::is_command_supported(strCommand))
    {
        pResultJson[0] = '\0';
        return static_cast<INT32>(NET_E_CMD_NOT_SUPPORT);
    }

    std::string strResultJson;
    const int nResult = CMqttSdkGateway::is_get_command(strCommand)
                            ? CMqttSdkGateway::execute_get(
                                  strCommand,
                                  strDataJson,
                                  strResultJson)
                            : CMqttSdkGateway::execute_set(
                                  strCommand,
                                  strDataJson,
                                  strResultJson);
    if (!copy_to_fixed_buffer(strResultJson,
                              pResultJson,
                              static_cast<std::size_t>(uResultCapacity)))
    {
        return ERR;
    }
    return nResult;
}

#endif
