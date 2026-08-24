/**
 * @file PlatformCommandRouter.cpp
 * @author Codex
 * @date 2026-08-22
 * @brief Implements MQTT command routing into the SDK configuration business layer.
 * @change 2026-08-22 Codex Initial implementation for platform migration.
 */

#include "PlatformCommandRouter.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <sys/stat.h>
#include <utility>

#include "DeviceConfigBusiness.h"
#include "NetSdkLog.h"
#include "NetTVSDKServerInterface.h"
#include "PlatformHttpClient.h"
#include "cJSON.h"

namespace
{
/**
 * @struct PlatformCommandMap_S
 * @brief Canonical SDK command-name mapping used by MQTT transport.
 */
struct PlatformCommandMap_S
{
    const char *pName;
    int nCommand;
    bool bGet;
};

#define PLATFORM_GET_COMMAND(command) {#command, command, true}
#define PLATFORM_SET_COMMAND(command) {#command, command, false}

const PlatformCommandMap_S gs_aPlatformCommands[] = {
    PLATFORM_GET_COMMAND(NET_GET_DEVICECFG),
    PLATFORM_SET_COMMAND(NET_SET_DEVICECFG),
    PLATFORM_GET_COMMAND(NET_GET_UPGRADESTATUS),
    PLATFORM_SET_COMMAND(NET_SET_UPGRADE),
    PLATFORM_GET_COMMAND(NET_GET_UPGRADEVERSION),
    PLATFORM_GET_COMMAND(NET_GET_NTPCFG),
    PLATFORM_SET_COMMAND(NET_SET_NTPCFG),
    PLATFORM_GET_COMMAND(NET_GET_STREAMCFG),
    PLATFORM_SET_COMMAND(NET_SET_STREAMCFG),
    PLATFORM_GET_COMMAND(NET_GET_RTSPURLCFG),
    PLATFORM_GET_COMMAND(NET_GET_REPLAY_URLCFG),
    PLATFORM_GET_COMMAND(NET_GET_REPLAY_RECORD_LIST),
    PLATFORM_SET_COMMAND(NET_SET_REPLAY_CTRL),
    PLATFORM_GET_COMMAND(NET_GET_AUDIOCFG),
    PLATFORM_SET_COMMAND(NET_SET_AUDIOCFG),
    PLATFORM_GET_COMMAND(NET_GET_OSDCAPCFG),
    PLATFORM_SET_COMMAND(NET_SET_OSDCAPCFG),
    PLATFORM_GET_COMMAND(NET_GET_IMAGECFG),
    PLATFORM_SET_COMMAND(NET_SET_IMAGECFG),
    PLATFORM_GET_COMMAND(NET_GET_NETWORKCFG),
    PLATFORM_SET_COMMAND(NET_SET_NETWORKCFG),
    PLATFORM_GET_COMMAND(NET_GET_PRIVACYMASKCFG),
    PLATFORM_SET_COMMAND(NET_SET_PRIVACYMASKCFG),
    PLATFORM_GET_COMMAND(NET_GET_TAMPERALARM),
    PLATFORM_SET_COMMAND(NET_SET_TAMPERALARM),
    PLATFORM_GET_COMMAND(NET_GET_MOTIONALARM),
    PLATFORM_SET_COMMAND(NET_SET_MOTIONALARM),
    PLATFORM_GET_COMMAND(NET_GET_CROSSLINEALARM),
    PLATFORM_SET_COMMAND(NET_SET_CROSSLINEALARM),
    PLATFORM_GET_COMMAND(NET_GET_INTRUSIONALARM),
    PLATFORM_SET_COMMAND(NET_SET_INTRUSIONALARM),
    PLATFORM_GET_COMMAND(NET_GET_LOITERINGALARM),
    PLATFORM_SET_COMMAND(NET_SET_LOITERINGALARM),
    PLATFORM_GET_COMMAND(NET_GET_CAPTURE_PLAN_INFO),
    PLATFORM_SET_COMMAND(NET_SET_CAPTURE_PLAN_INFO),
    PLATFORM_GET_COMMAND(NET_GET_CAPTURE_PARAM_INFO),
    PLATFORM_SET_COMMAND(NET_SET_CAPTURE_PARAM_INFO),
    PLATFORM_GET_COMMAND(NET_GET_EXPOSURE_INFO),
    PLATFORM_SET_COMMAND(NET_SET_EXPOSURE_INFO),
    PLATFORM_GET_COMMAND(NET_GET_DAYNIGHT_INFO),
    PLATFORM_SET_COMMAND(NET_SET_DAYNIGHT_INFO),
    PLATFORM_GET_COMMAND(NET_GET_BACKLIGHT_INFO),
    PLATFORM_SET_COMMAND(NET_SET_BACKLIGHT_INFO),
    PLATFORM_GET_COMMAND(NET_GET_DENOISE_INFO),
    PLATFORM_SET_COMMAND(NET_SET_DENOISE_INFO),
    PLATFORM_GET_COMMAND(NET_GET_WHITEBALANCE_INFO),
    PLATFORM_SET_COMMAND(NET_SET_WHITEBALANCE_INFO),
    PLATFORM_GET_COMMAND(NET_GET_AUDIOANOMALYALARM),
    PLATFORM_SET_COMMAND(NET_SET_AUDIOANOMALYALARM),
    PLATFORM_GET_COMMAND(NET_GET_PREVIEW_INFO),
    PLATFORM_SET_COMMAND(NET_SET_PREVIEW_INFO),
    PLATFORM_GET_COMMAND(NET_GET_SCENECHANGEALARM),
    PLATFORM_SET_COMMAND(NET_SET_SCENECHANGEALARM),
    PLATFORM_GET_COMMAND(NET_GET_CROWDGATHERINGALARM),
    PLATFORM_SET_COMMAND(NET_SET_CROWDGATHERINGALARM),
    PLATFORM_GET_COMMAND(NET_GET_PARKINGALARM),
    PLATFORM_SET_COMMAND(NET_SET_PARKINGALARM),
    PLATFORM_GET_COMMAND(NET_GET_UNATTENDEDOBJECTALARM),
    PLATFORM_SET_COMMAND(NET_SET_UNATTENDEDOBJECTALARM),
    PLATFORM_GET_COMMAND(NET_GET_OBJECTREMOVALALARM),
    PLATFORM_SET_COMMAND(NET_SET_OBJECTREMOVALALARM),
    PLATFORM_SET_COMMAND(NET_SET_CONFIG_WIFI_STA),
    PLATFORM_SET_COMMAND(NET_CONNECT_WIFI_STA),
    PLATFORM_SET_COMMAND(NET_DISCONNECT_WIFI_STA),
    PLATFORM_GET_COMMAND(NET_GET_4G_INFO),
    PLATFORM_SET_COMMAND(NET_SET_4G_INFO),
    PLATFORM_SET_COMMAND(NET_SET_HOTSPOT_INFO),
    PLATFORM_GET_COMMAND(NET_GET_ENTERREGIONALARM),
    PLATFORM_SET_COMMAND(NET_SET_ENTERREGIONALARM),
    PLATFORM_GET_COMMAND(NET_GET_LEAVEREGIONALARM),
    PLATFORM_SET_COMMAND(NET_SET_LEAVEREGIONALARM),
    PLATFORM_GET_COMMAND(NET_GET_FACECAPTUREINFO),
    PLATFORM_SET_COMMAND(NET_SET_FACECAPTUREINFO),
    PLATFORM_GET_COMMAND(NET_GET_FACECAPTUREOVERLAYINFO),
    PLATFORM_SET_COMMAND(NET_SET_FACECAPTUREOVERLAYINFO),
    PLATFORM_GET_COMMAND(NET_GET_HOTSPOT_CONN),
    PLATFORM_GET_COMMAND(NET_GET_CHANNEL_INFO),
    PLATFORM_GET_COMMAND(NET_GET_GARBAGE_EXPOSURE_CFG),
    PLATFORM_SET_COMMAND(NET_SET_GARBAGE_EXPOSURE_CFG),
    PLATFORM_GET_COMMAND(NET_GET_GARBAGE_OVERFLOW_CFG),
    PLATFORM_SET_COMMAND(NET_SET_GARBAGE_OVERFLOW_CFG),
    PLATFORM_GET_COMMAND(NET_GET_PEOPLE_FLOW_STATISTICS_CFG),
    PLATFORM_SET_COMMAND(NET_SET_PEOPLE_FLOW_STATISTICS_CFG),
    PLATFORM_SET_COMMAND(NET_RESET_PEOPLE_FLOW_STATISTICS),
    PLATFORM_GET_COMMAND(NET_GET_PEOPLE_DENSITY_DETECTION_CFG),
    PLATFORM_SET_COMMAND(NET_SET_PEOPLE_DENSITY_DETECTION_CFG),
    PLATFORM_GET_COMMAND(NET_GET_MANHOLE_COVER_ABNORMAL_CFG),
    PLATFORM_SET_COMMAND(NET_SET_MANHOLE_COVER_ABNORMAL_CFG),
    PLATFORM_GET_COMMAND(NET_GET_SLEEP_ON_DUTY_CFG),
    PLATFORM_SET_COMMAND(NET_SET_SLEEP_ON_DUTY_CFG),
    PLATFORM_GET_COMMAND(NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG),
    PLATFORM_SET_COMMAND(NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG),
    PLATFORM_GET_COMMAND(NET_GET_PERSON_FALL_DOWN_CFG),
    PLATFORM_SET_COMMAND(NET_SET_PERSON_FALL_DOWN_CFG),
    PLATFORM_GET_COMMAND(NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG),
    PLATFORM_SET_COMMAND(NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG),
    PLATFORM_GET_COMMAND(NET_GET_CONGESTION_CFG),
    PLATFORM_SET_COMMAND(NET_SET_CONGESTION_CFG),
    PLATFORM_GET_COMMAND(NET_GET_LICENSE_PLATE_RECOGNITION_CFG),
    PLATFORM_SET_COMMAND(NET_SET_LICENSE_PLATE_RECOGNITION_CFG),
    PLATFORM_GET_COMMAND(NET_GET_HIGH_ALTITUDE_SEATBELT_CFG),
    PLATFORM_SET_COMMAND(NET_SET_HIGH_ALTITUDE_SEATBELT_CFG),
    PLATFORM_GET_COMMAND(NET_GET_SAFETY_HELMET_CFG),
    PLATFORM_SET_COMMAND(NET_SET_SAFETY_HELMET_CFG),
    PLATFORM_GET_COMMAND(NET_GET_PERSON_FALL_CFG),
    PLATFORM_SET_COMMAND(NET_SET_PERSON_FALL_CFG),
    PLATFORM_GET_COMMAND(NET_GET_PHONE_USAGE_CFG),
    PLATFORM_SET_COMMAND(NET_SET_PHONE_USAGE_CFG),
    PLATFORM_GET_COMMAND(NET_GET_SMOKING_CFG),
    PLATFORM_SET_COMMAND(NET_SET_SMOKING_CFG),
    PLATFORM_GET_COMMAND(NET_GET_OPEN_FLAME_CFG),
    PLATFORM_SET_COMMAND(NET_SET_OPEN_FLAME_CFG),
    PLATFORM_GET_COMMAND(NET_GET_BARE_SOIL_CFG),
    PLATFORM_SET_COMMAND(NET_SET_BARE_SOIL_CFG),
    PLATFORM_GET_COMMAND(NET_GET_HOLE_PROTECTION_BAR_CFG),
    PLATFORM_SET_COMMAND(NET_SET_HOLE_PROTECTION_BAR_CFG),
    PLATFORM_GET_COMMAND(NET_GET_REFLECTIVE_CLOTHING_CFG),
    PLATFORM_SET_COMMAND(NET_SET_REFLECTIVE_CLOTHING_CFG),
    PLATFORM_GET_COMMAND(NET_GET_PET_RECOGNITION_INFO),
    PLATFORM_SET_COMMAND(NET_SET_PET_RECOGNITION_INFO),
    PLATFORM_GET_COMMAND(NET_GET_CLIMB_FENCE_INFO),
    PLATFORM_SET_COMMAND(NET_SET_CLIMB_FENCE_INFO),
    PLATFORM_GET_COMMAND(NET_GET_DIMISSION_INFO),
    PLATFORM_SET_COMMAND(NET_SET_DIMISSION_INFO),
    PLATFORM_GET_COMMAND(NET_GET_ILLEGAL_LANE_INFO),
    PLATFORM_SET_COMMAND(NET_SET_ILLEGAL_LANE_INFO),
    PLATFORM_GET_COMMAND(NET_GET_RETROGRADE_INFO),
    PLATFORM_SET_COMMAND(NET_SET_RETROGRADE_INFO),
    PLATFORM_GET_COMMAND(NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO),
    PLATFORM_SET_COMMAND(NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO),
    PLATFORM_GET_COMMAND(NET_GET_OCCUPATION_EMERGENCY_INFO),
    PLATFORM_SET_COMMAND(NET_SET_OCCUPATION_EMERGENCY_INFO),
    PLATFORM_GET_COMMAND(NET_GET_PEDESTRIAN_INTRUSION_INFO),
    PLATFORM_SET_COMMAND(NET_SET_PEDESTRIAN_INTRUSION_INFO),
    PLATFORM_GET_COMMAND(NET_GET_SMOKE_FIRE_CFG),
    PLATFORM_SET_COMMAND(NET_SET_SMOKE_FIRE_CFG),
    PLATFORM_GET_COMMAND(NET_GET_ROAD_PONDING_CFG),
    PLATFORM_SET_COMMAND(NET_SET_ROAD_PONDING_CFG),
    PLATFORM_GET_COMMAND(NET_GET_SECURITY_SERVICES_INFO),
    PLATFORM_SET_COMMAND(NET_SET_SECURITY_SERVICES_INFO),
    PLATFORM_GET_COMMAND(NET_GET_SSH_COUNTDOWN),
    PLATFORM_GET_COMMAND(NET_FIND_LOG),
    PLATFORM_GET_COMMAND(NET_EXPORT_LOG),
    PLATFORM_GET_COMMAND(NET_GET_LOG_SERVER),
    PLATFORM_SET_COMMAND(NET_SET_LOG_SERVER),
    PLATFORM_SET_COMMAND(NET_TEST_LOG_SERVER),
    PLATFORM_SET_COMMAND(NET_CONTROL_RECORD_INFO),
    PLATFORM_GET_COMMAND(NET_GET_RECORD_STATUS),
    PLATFORM_GET_COMMAND(NET_GET_RECORD_SCHEDULE),
    PLATFORM_SET_COMMAND(NET_SET_RECORD_SCHEDULE),
    PLATFORM_GET_COMMAND(NET_GET_RECORD_ADVANCED_PARAM),
    PLATFORM_SET_COMMAND(NET_SET_RECORD_ADVANCED_PARAM),
    PLATFORM_GET_COMMAND(NET_FIND_RECORD_FILE_INFO),
    PLATFORM_SET_COMMAND(NET_DOWNLOAD_RECORD_FILE),
    PLATFORM_SET_COMMAND(NET_SET_FACE_COMPARE_INFO),
    PLATFORM_SET_COMMAND(NET_ADD_TARGET_LIB),
    PLATFORM_SET_COMMAND(NET_DEL_TARGET_LIB),
    PLATFORM_SET_COMMAND(NET_SET_TARGET_LIB),
    PLATFORM_GET_COMMAND(NET_GET_TARGET_LIB),
    PLATFORM_SET_COMMAND(NET_ADD_FACE_INFO),
    PLATFORM_SET_COMMAND(NET_DEL_FACE_INFO),
    PLATFORM_SET_COMMAND(NET_SET_FACE_INFO),
    PLATFORM_GET_COMMAND(NET_GET_FACE_INFO),
    PLATFORM_GET_COMMAND(NET_GET_VOICECOM_AUDIO_CFG),
    PLATFORM_SET_COMMAND(NET_SET_VOICECOM_AUDIO_CFG),
    PLATFORM_GET_COMMAND(NET_GET_SD_CARD_STATUS),
    PLATFORM_GET_COMMAND(NET_GET_AUDIBLE_ALARM_INFO),
    PLATFORM_SET_COMMAND(NET_SET_AUDIBLE_ALARM_INFO),
    PLATFORM_GET_COMMAND(NET_GET_ALARM_INPUT_INFO),
    PLATFORM_SET_COMMAND(NET_SET_ALARM_INPUT_INFO),
    PLATFORM_GET_COMMAND(NET_GET_ALARM_OUTPUT_INFO),
    PLATFORM_SET_COMMAND(NET_SET_ALARM_OUTPUT_INFO),
    PLATFORM_GET_COMMAND(NET_GET_FLASHING_LIGHT_ALARM_INFO),
    PLATFORM_SET_COMMAND(NET_SET_FLASHING_LIGHT_ALARM_INFO),
    PLATFORM_GET_COMMAND(NET_GET_PIR_ALARM_INFO),
    PLATFORM_SET_COMMAND(NET_SET_PIR_ALARM_INFO),
    PLATFORM_GET_COMMAND(NET_GET_STORAGE_INFO),
    PLATFORM_GET_COMMAND(NET_GET_REGISTERINFO),
    PLATFORM_SET_COMMAND(NET_SET_REGISTERINFO),
    {"NET_GET_FACE_COMPARE_INFO", NET_SET_FACE_COMPARE_INFO, true},
    {"NET_GET_CHANNEL_LIST", NET_GET_CHANNEL_INFO, true}};

#undef PLATFORM_GET_COMMAND
#undef PLATFORM_SET_COMMAND

/**
 * @brief Normalizes whitespace, case and the historical NET_TV prefix.
 * @author Codex
 * @param [IN] strCommand Raw command text.
 * @return Canonical command text.
 */
static std::string NormalizeCommand(const std::string &strCommand)
{
    std::string strResult;
    strResult.reserve(strCommand.size());
    for (char chValue : strCommand)
    {
        const unsigned char byValue = static_cast<unsigned char>(chValue);
        if (std::isspace(byValue) == 0)
        {
            strResult.push_back(static_cast<char>(std::toupper(byValue)));
        }
    }

    constexpr const char *pLegacyPrefix = "NET_TV_";
    if (strResult.compare(0, std::strlen(pLegacyPrefix), pLegacyPrefix) == 0)
    {
        strResult.replace(0, std::strlen(pLegacyPrefix), "NET_");
    }
    return strResult;
}

/**
 * @brief Finds a canonical command map by text or decimal command number.
 * @author Codex
 * @param [IN] strCommand Raw command text.
 * @return Matching map entry or nullptr.
 */
static const PlatformCommandMap_S *FindCommand(const std::string &strCommand)
{
    const std::string strNormalized = NormalizeCommand(strCommand);
    if (!strNormalized.empty() &&
        std::all_of(strNormalized.begin(),
                    strNormalized.end(),
                    [](char chValue)
                    {
                        return std::isdigit(static_cast<unsigned char>(chValue)) != 0;
                    }))
    {
        const int nCommand = std::atoi(strNormalized.c_str());
        for (const PlatformCommandMap_S &stItem : gs_aPlatformCommands)
        {
            if (stItem.nCommand == nCommand)
            {
                return &stItem;
            }
        }
        return nullptr;
    }

    for (const PlatformCommandMap_S &stItem : gs_aPlatformCommands)
    {
        if (strNormalized == stItem.pName)
        {
            return &stItem;
        }
    }
    return nullptr;
}

/**
 * @brief Reads an integer from one of several JSON field aliases.
 * @author Codex
 * @return True when a numeric field is found.
 */
static bool GetJsonInt(cJSON *pRoot,
                       const char *pFirstName,
                       const char *pSecondName,
                       int &nValue)
{
    cJSON *pItem = cJSON_GetObjectItemCaseSensitive(pRoot, pFirstName);
    if (!cJSON_IsNumber(pItem) && pSecondName != nullptr)
    {
        pItem = cJSON_GetObjectItemCaseSensitive(pRoot, pSecondName);
    }
    if (!cJSON_IsNumber(pItem))
    {
        return false;
    }
    nValue = pItem->valueint;
    return true;
}

/**
 * @brief Reads a string from one JSON field.
 * @author Codex
 * @return True when a nonempty string is found.
 */
static bool GetJsonString(cJSON *pRoot, const char *pName, std::string &strValue)
{
    cJSON *pItem = cJSON_GetObjectItemCaseSensitive(pRoot, pName);
    if (!cJSON_IsString(pItem) || pItem->valuestring == nullptr || pItem->valuestring[0] == '\0')
    {
        return false;
    }
    strValue.assign(pItem->valuestring);
    return true;
}

/**
 * @brief Adds or replaces one string JSON field.
 * @author Codex
 * @return True when the field is stored.
 */
static bool SetJsonString(cJSON *pRoot, const char *pName, const std::string &strValue)
{
    cJSON *pString = cJSON_CreateString(strValue.c_str());
    if (pString == nullptr)
    {
        return false;
    }
    if (cJSON_HasObjectItem(pRoot, pName))
    {
        if (cJSON_ReplaceItemInObjectCaseSensitive(pRoot, pName, pString) == 0)
        {
            cJSON_Delete(pString);
            return false;
        }
        return true;
    }
    cJSON_AddItemToObject(pRoot, pName, pString);
    return true;
}

/**
 * @brief Adds or replaces one integer JSON field.
 * @author Codex
 * @return True when the field is stored.
 */
static bool SetJsonInt(cJSON *pRoot, const char *pName, int nValue)
{
    cJSON *pNumber = cJSON_CreateNumber(nValue);
    if (pNumber == nullptr)
    {
        return false;
    }
    if (cJSON_HasObjectItem(pRoot, pName))
    {
        if (cJSON_ReplaceItemInObjectCaseSensitive(pRoot, pName, pNumber) == 0)
        {
            cJSON_Delete(pNumber);
            return false;
        }
        return true;
    }
    cJSON_AddItemToObject(pRoot, pName, pNumber);
    return true;
}

/**
 * @brief Serializes one JSON node without changing ownership.
 * @author Codex
 * @return Compact JSON text or an empty string.
 */
static std::string PrintJson(cJSON *pRoot)
{
    char *pText = pRoot != nullptr ? cJSON_PrintUnformatted(pRoot) : nullptr;
    std::string strResult = pText != nullptr ? pText : std::string();
    if (pText != nullptr)
    {
        cJSON_free(pText);
    }
    return strResult;
}

/**
 * @brief Extracts Return, Data and Message from a SDK business response.
 * @author Codex
 * @return True when the response is valid JSON with a numeric Return field.
 */
static bool ParseBusinessResponse(const std::string &strResponse,
                                  CPlatformCommandRouter::Result_S &stResult)
{
    cJSON *pRoot = cJSON_Parse(strResponse.c_str());
    if (pRoot == nullptr)
    {
        return false;
    }

    cJSON *pReturn = cJSON_GetObjectItemCaseSensitive(pRoot, "Return");
    if (!cJSON_IsNumber(pReturn))
    {
        cJSON_Delete(pRoot);
        return false;
    }
    stResult.nReturn = pReturn->valueint;

    cJSON *pData = cJSON_GetObjectItemCaseSensitive(pRoot, "Data");
    stResult.strData = pData != nullptr ? PrintJson(pData) : "{}";
    if (stResult.strData.empty())
    {
        stResult.strData = "{}";
    }

    cJSON *pMessage = cJSON_GetObjectItemCaseSensitive(pRoot, "Message");
    if (cJSON_IsString(pMessage) && pMessage->valuestring != nullptr)
    {
        stResult.strMessage.assign(pMessage->valuestring);
    }
    cJSON_Delete(pRoot);
    return true;
}

/**
 * @brief Returns a file size or negative one when the file is absent.
 * @author Codex
 * @param [IN] strPath File path.
 * @return File size in bytes or negative one.
 */
static long long GetFileSize(const std::string &strPath)
{
    struct stat stInfo;
    if (stat(strPath.c_str(), &stInfo) != 0)
    {
        return -1;
    }
    return static_cast<long long>(stInfo.st_size);
}

/**
 * @brief Extracts a safe base name from a URL or path.
 * @author Codex
 * @param [IN] strPathOrUrl URL or path.
 * @return Base-name component without query or fragment text.
 */
static std::string GetBaseName(const std::string &strPathOrUrl)
{
    const std::size_t uQueryPosition = strPathOrUrl.find_first_of("?#");
    const std::string strPath = uQueryPosition == std::string::npos
                                    ? strPathOrUrl
                                    : strPathOrUrl.substr(0, uQueryPosition);
    const std::size_t uSlashPosition = strPath.find_last_of("/\\");
    const std::string strName = uSlashPosition == std::string::npos
                                    ? strPath
                                    : strPath.substr(uSlashPosition + 1);
    if (strName == "." || strName == "..")
    {
        return std::string();
    }
    return strName;
}
}

bool CPlatformCommandRouter::Configure(CPlatformHttpClient *pHttpClient,
                                       const std::string &strImageDownloadDirectory,
                                       FallbackCommand_FN fnFallbackCommand)
{
    if (pHttpClient == nullptr || strImageDownloadDirectory.empty())
    {
        return false;
    }
    m_pHttpClient = pHttpClient;
    m_strImageDownloadDirectory = strImageDownloadDirectory;
    m_fnFallbackCommand = std::move(fnFallbackCommand);
    return true;
}

bool CPlatformCommandRouter::Execute(const std::string &strCommand,
                                     const std::string &strData,
                                     Result_S &stResult)
{
    stResult = Result_S();
    const PlatformCommandMap_S *pCommand = FindCommand(strCommand);
    if (pCommand == nullptr)
    {
        if (!m_fnFallbackCommand)
        {
            stResult.strMessage = "unsupported command";
            return false;
        }

        std::string strFallbackResult;
        stResult.nReturn = m_fnFallbackCommand(strCommand, strData, strFallbackResult);
        if (!strFallbackResult.empty() && !ParseBusinessResponse(strFallbackResult, stResult))
        {
            stResult.strData = strFallbackResult;
        }
        return true;
    }

    stResult.nCommand = pCommand->nCommand;
    std::string strPreparedData = strData.empty() ? "{}" : strData;
    std::string strPrepareError;
    if (!PrepareFaceImage(pCommand->nCommand, strPreparedData, strPrepareError))
    {
        stResult.nReturn = -1;
        stResult.strMessage = strPrepareError;
        return true;
    }

    int nChannel = 0;
    cJSON *pDataRoot = cJSON_Parse(strPreparedData.c_str());
    if (pDataRoot != nullptr)
    {
        GetJsonInt(pDataRoot, "Channel", "channel", nChannel);
        cJSON_Delete(pDataRoot);
    }

    const std::string strUrlParameters =
        "channel=" + std::to_string(nChannel) +
        "&nCommand=" + std::to_string(pCommand->nCommand);
    const std::string strBusinessResponse = pCommand->bGet
                                                ? CDeviceConfigBusiness::instance()->GetDevConfig(
                                                      strPreparedData,
                                                      strUrlParameters)
                                                : CDeviceConfigBusiness::instance()->SetDevConfig(
                                                      strPreparedData,
                                                      strUrlParameters);
    if (!ParseBusinessResponse(strBusinessResponse, stResult))
    {
        stResult.nReturn = -1;
        stResult.strMessage = "SDK command response is invalid";
        return true;
    }

    if (stResult.nReturn == static_cast<int>(NET_E_CMD_NOT_SUPPORT) && m_fnFallbackCommand)
    {
        std::string strFallbackResult;
        stResult.nReturn = m_fnFallbackCommand(strCommand, strPreparedData, strFallbackResult);
        if (!strFallbackResult.empty() && !ParseBusinessResponse(strFallbackResult, stResult))
        {
            stResult.strData = strFallbackResult;
        }
    }

    NETSDK_LOG_MESSAGE_INFO("Platform command executed: command=%s code=%d return=%d",
                            strCommand.c_str(),
                            pCommand->nCommand,
                            stResult.nReturn);
    return true;
}

bool CPlatformCommandRouter::PrepareFaceImage(int nCommand,
                                              std::string &strData,
                                              std::string &strError)
{
    if (nCommand != NET_ADD_FACE_INFO && nCommand != NET_SET_FACE_INFO)
    {
        return true;
    }
    if (m_pHttpClient == nullptr)
    {
        strError = "platform HTTP client is unavailable";
        return false;
    }

    cJSON *pRoot = cJSON_Parse(strData.c_str());
    if (!cJSON_IsObject(pRoot))
    {
        if (pRoot != nullptr)
        {
            cJSON_Delete(pRoot);
        }
        strError = "face command Data is not a JSON object";
        return false;
    }

    int nLowerId = 0;
    if (cJSON_GetObjectItemCaseSensitive(pRoot, "Id") == nullptr &&
        GetJsonInt(pRoot, "id", nullptr, nLowerId))
    {
        SetJsonInt(pRoot, "Id", nLowerId);
    }

    int nWidth = 0;
    int nHeight = 0;
    const bool bHasDimensions =
        GetJsonInt(pRoot, "PicWidth", "pic_width", nWidth) &&
        GetJsonInt(pRoot, "PicHeight", "pic_height", nHeight) &&
        nWidth > 0 && nHeight > 0;

    /*
     * Face metadata updates retain the historical IPC behavior: normalize the
     * identifier, but do not require a replacement NV21 image.
     */
    if (nCommand == NET_SET_FACE_INFO)
    {
        strData = PrintJson(pRoot);
        cJSON_Delete(pRoot);
        if (strData.empty())
        {
            strError = "face command JSON serialization failed";
            return false;
        }
        return true;
    }

    if (!bHasDimensions)
    {
        cJSON_Delete(pRoot);
        strError = "face NV21 dimensions are invalid";
        return false;
    }

    std::string strDownloadUrl;
    const char *aUrlFields[] = {"PicUrl", "DownloadUrl", "Nv21Url", "FileUrl", "PicPath"};
    for (const char *pField : aUrlFields)
    {
        if (GetJsonString(pRoot, pField, strDownloadUrl))
        {
            break;
        }
    }

    std::string strBinPath;
    GetJsonString(pRoot, "BinPath", strBinPath);
    if (strBinPath.empty() && !strDownloadUrl.empty())
    {
        std::string strFileName = GetBaseName(strDownloadUrl);
        if (strFileName.empty())
        {
            strFileName = "face.nv21";
        }
        strBinPath = m_strImageDownloadDirectory;
        if (!strBinPath.empty() && strBinPath.back() != '/' && strBinPath.back() != '\\')
        {
            strBinPath.push_back('/');
        }
        strBinPath += strFileName;
        SetJsonString(pRoot, "BinPath", strBinPath);
    }

    const long long llExpectedSize = static_cast<long long>(nWidth) *
                                     static_cast<long long>(nHeight) * 3LL / 2LL;

    const long long llCurrentSize = strBinPath.empty() ? -1 : GetFileSize(strBinPath);
    const bool bLocalFileValid =
        llCurrentSize >= 0 && (llExpectedSize == 0 || llCurrentSize == llExpectedSize);
    if (!bLocalFileValid)
    {
        if (strDownloadUrl.empty() || strBinPath.empty())
        {
            cJSON_Delete(pRoot);
            strError = "face NV21 source URL or destination path is missing";
            return false;
        }
        if (!m_pHttpClient->DownloadFile(strDownloadUrl,
                                         strBinPath,
                                         llExpectedSize,
                                         strError))
        {
            cJSON_Delete(pRoot);
            return false;
        }
    }

    const long long llFinalSize = GetFileSize(strBinPath);
    if (llFinalSize < 0 || (llExpectedSize > 0 && llFinalSize != llExpectedSize))
    {
        cJSON_Delete(pRoot);
        strError = "face NV21 file validation failed";
        return false;
    }
    if (llFinalSize <= INT_MAX)
    {
        SetJsonInt(pRoot, "PicSize", static_cast<int>(llFinalSize));
    }

    strData = PrintJson(pRoot);
    cJSON_Delete(pRoot);
    if (strData.empty())
    {
        strError = "face command JSON serialization failed";
        return false;
    }
    return true;
}
