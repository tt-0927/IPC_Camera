/**
 * @FilePath     : tvsdk_callbacks.cpp
 * @Description  : TVSDK 回调实现与注册（使用 action_code.h 命令码对接 control_manage）
 */

#include "tvsdk_callbacks.h"

#include <string>
#include <algorithm>
#include <cstring>
#include <set>
#include <vector>
#include <fstream>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <cstdio>
#include <cmath>
#include <memory>
#include <mutex>

#include "task_manage.h"
#include "task.h"
#include "dlog.h"
#include "action_code.h"
#include "system_manage.h"
#include "system_define.h"
#include "time_manage.h"
#include "network_define.h"
#include "alarm_define.h"
#include "preview_define.h"
#include "osd_manage.h"
#include "preview_manage.h"
#include "Json.h"
#include "convert_interface.h"
#include "video_define.h"
#include "path_define.h"
#include "IpcRet.h"

#include "convert/tvsdk_convert.h"
#include "rtsp_server.h"
#include "upgrade_client.h"

namespace TvSdkCallbacks
{
static CTaskManage *s_taskManage = nullptr;

// 移动侦测 / 遮挡报警 使用的 TVSDK 中间缓存，避免直接在 SDK 传入缓冲区上做复杂写入
static NET_MotionAlarmInfo_S g_tvMotionAlarmInfo;
static NET_TamperAlarmInfo_S g_tvTamperAlarmInfo;
static NET_AudioAnomalyAlarmInfo_S g_tvAudioAnomalyAlarmInfo;

static int execute_get_result(int actionCode, const std::string &inJson, std::string &outJson);
static NET_COMMON_ECODE_E validate_video_resolution(const NET_VideoEncodeOption_S &stConfig);

static const char *kDefaultUpgradeDir = "/opt/course/";
static constexpr UINT32 TVSDK_CAPTURE_INTERVAL_MAX_MILLISECONDS = 86400000U;
static constexpr UINT32 TVSDK_CAPTURE_INTERVAL_MAX_SECONDS = 86400U;
static constexpr UINT32 TVSDK_CAPTURE_INTERVAL_MAX_MINUTES = 1440U;
static constexpr UINT32 TVSDK_CAPTURE_INTERVAL_MAX_HOURS = 24U;
static constexpr UINT32 TVSDK_CAPTURE_INTERVAL_MAX_DAYS = 365U;

static bool is_absolute_path(const std::string &path)
{
    return !path.empty() && path[0] == '/';
}

static bool is_valid_upgrade_path(const std::string &path)
{
    if (path.empty() || path.size() >= NET_FILE_NAME_LEN)
        return false;
    for (char ch : path)
    {
        if (ch == '\r' || ch == '\n')
            return false;
    }
    return true;
}

static std::string normalize_upgrade_local_path(const std::string &srcPath)
{
    if (srcPath.empty())
        return std::string(kDefaultUpgradeDir);

    if (srcPath.find("://") != std::string::npos)
        return "";

    if (is_absolute_path(srcPath))
        return srcPath;

    return std::string(kDefaultUpgradeDir) + srcPath;
}

/* 把 {"Return":0,"Data":{...}} 归一化成 {...}，便于 Convert::to_struct 使用 */
static std::string normalize_data_json(const std::string &srcJson)
{
    // 1. 入参为空，直接返回空字符串，而不是原样返回
    if (srcJson.empty())
        return "";

    Json::Object *pRoot = Json::init(srcJson.c_str());
    if (!pRoot)
        return ""; // 2. 解析失败，返回空字符串

    std::string out = ""; // 3. 默认输出为空
    
    // 取出 Data 节点
    Json::Object *pData = Json::get(pRoot, "Data");
    if (pData)
    {
        // 将 Data 节点序列化为字符串
        // 注意：如果想要和你的示例一样有换行缩进，
        // 需要确认你的 Json::to_string 是否有支持格式化的重载，例如 Json::to_string(pData, true)
        out = Json::to_string(pData); 
    }

    // 释放根节点内存
    Json::deinit(pRoot);
    
    return out;
}

/* 严格校验日期时间，避免 strptime 或 mktime 自动归一化非法日期。 */
static bool is_valid_date_time(const char *pDateTime, INT32 nDateFormat)
{
    if (pDateTime == nullptr || nDateFormat < 0 || nDateFormat > 5)
    {
        return false;
    }

    const size_t nDateTimeLength = strnlen(pDateTime, NET_MAX_DATE_STRING_LEN);
    if (nDateTimeLength == 0 || nDateTimeLength >= NET_MAX_DATE_STRING_LEN)
    {
        return false;
    }
    const std::string strDateTime(pDateTime, nDateTimeLength);

    /* strptime 允许部分字段省略，这里先校验 SDK 约定的固定长度和分隔符。 */
    const char chDateSeparator = (nDateFormat == static_cast<INT32>(::System::DateFormat_E::YYYYMMDD) ||
                                  nDateFormat == static_cast<INT32>(::System::DateFormat_E::MMDDYYYY) ||
                                  nDateFormat == static_cast<INT32>(::System::DateFormat_E::DDMMYYYY))
                                     ? '/'
                                     : '-';
    if (strDateTime.size() != 19 || strDateTime[4] != chDateSeparator ||
        strDateTime[7] != chDateSeparator || strDateTime[10] != ' ' ||
        strDateTime[13] != ':' || strDateTime[16] != ':')
    {
        return false;
    }

    const char *pFormat = ::System::to_string(::System::Language::ENGLISH,
                                               static_cast<::System::DateFormat_E>(nDateFormat));
    if (pFormat == nullptr || std::strcmp(pFormat, "Unknown time") == 0)
    {
        return false;
    }

    std::tm stParsedTime = {};
    char *pEnd = strptime(strDateTime.c_str(), pFormat, &stParsedTime);
    if (pEnd == nullptr || *pEnd != '\0' || stParsedTime.tm_year < 70 ||
        stParsedTime.tm_hour < 0 || stParsedTime.tm_hour > 23 ||
        stParsedTime.tm_min < 0 || stParsedTime.tm_min > 59 ||
        stParsedTime.tm_sec < 0 || stParsedTime.tm_sec > 59)
    {
        return false;
    }

    const int nYear = stParsedTime.tm_year;
    const int nMonth = stParsedTime.tm_mon;
    const int nDay = stParsedTime.tm_mday;
    const int nHour = stParsedTime.tm_hour;
    const int nMinute = stParsedTime.tm_min;
    const int nSecond = stParsedTime.tm_sec;
    if (mktime(&stParsedTime) == static_cast<time_t>(-1))
    {
        return false;
    }

    return nYear == stParsedTime.tm_year && nMonth == stParsedTime.tm_mon &&
           nDay == stParsedTime.tm_mday && nHour == stParsedTime.tm_hour &&
           nMinute == stParsedTime.tm_min && nSecond == stParsedTime.tm_sec;
}

/* 校验视频分辨率必须存在于 IPC 当前上报的编码能力列表中。 */
static NET_COMMON_ECODE_E validate_video_resolution(const NET_VideoEncodeOption_S &stConfig)
{
    std::string strResult;
    if (execute_get_result(AC_GET_VIDEO_CAPABILITY_SET, "{}", strResult) != 0 || strResult.empty())
    {
        return NET_E_GET_CFG_FAILED;
    }

    int nReturn = -1;
    if (!Json::get(strResult.c_str(), "Return", nReturn) || nReturn != 0)
    {
        return NET_E_GET_CFG_FAILED;
    }

    const std::string strData = normalize_data_json(strResult);
    if (strData.empty())
    {
        return NET_E_GET_CFG_FAILED;
    }

    Video_NS::VideoCapabilitySet_S stCapabilitySet;
    Convert::to_struct(strData, stCapabilitySet);
    const Video_NS::VideoCapability_S *pCapability =
        (stConfig.nId == NET_LIVE_STREAM_INDEX_MAIN) ? &stCapabilitySet.stMain : &stCapabilitySet.stSub;
    if (pCapability == nullptr || pCapability->aResolution.empty())
    {
        return NET_E_GET_CFG_FAILED;
    }

    const size_t nResolutionCount = pCapability->nResolutionNum > 0
                                        ? std::min<size_t>(static_cast<size_t>(pCapability->nResolutionNum),
                                                           pCapability->aResolution.size())
                                        : pCapability->aResolution.size();
    for (size_t nIndex = 0; nIndex < nResolutionCount; ++nIndex)
    {
        int nWidth = 0;
        int nHeight = 0;
        if (std::sscanf(pCapability->aResolution[nIndex].strName.c_str(),
                        "%d*%d", &nWidth, &nHeight) != 2 &&
            std::sscanf(pCapability->aResolution[nIndex].strName.c_str(),
                        "%dx%d", &nWidth, &nHeight) != 2)
        {
            continue;
        }
        if (nWidth == stConfig.stVideoResolution.uWidth &&
            nHeight == stConfig.stVideoResolution.uHeight)
        {
            return NET_E_SUCCEED;
        }
    }

    return NET_E_INVALID_PARAM;
}

/* 校验抓图参数的枚举值、分辨率和业务允许范围。 */
static bool is_valid_capture_config(const NET_CaptureConfig_S &stConfig)
{
    UINT32 unMaxInterval = 0;
    switch (stConfig.enTimeUnit)
    {
    case NET_CAPTURE_TIME_UNIT_MILLISECONDS:
        unMaxInterval = TVSDK_CAPTURE_INTERVAL_MAX_MILLISECONDS;
        break;
    case NET_CAPTURE_TIME_UNIT_SECONDS:
        unMaxInterval = TVSDK_CAPTURE_INTERVAL_MAX_SECONDS;
        break;
    case NET_CAPTURE_TIME_UNIT_MINUTES:
        unMaxInterval = TVSDK_CAPTURE_INTERVAL_MAX_MINUTES;
        break;
    case NET_CAPTURE_TIME_UNIT_HOURS:
        unMaxInterval = TVSDK_CAPTURE_INTERVAL_MAX_HOURS;
        break;
    case NET_CAPTURE_TIME_UNIT_DAYS:
        unMaxInterval = TVSDK_CAPTURE_INTERVAL_MAX_DAYS;
        break;
    default:
        return false;
    }

    return stConfig.enPictureFormat >= NET_CAPTURE_PICTURE_FORMAT_JPEG &&
           stConfig.enPictureFormat <= NET_CAPTURE_PICTURE_FORMAT_BMP &&
           stConfig.nWidth > 0 && stConfig.nWidth <= 8192 &&
           stConfig.nHeight > 0 && stConfig.nHeight <= 8192 &&
           stConfig.enImageQuality >= NET_CAPTURE_IMAGE_QUALITY_LOW &&
           stConfig.enImageQuality <= NET_CAPTURE_IMAGE_QUALITY_HIGH &&
           stConfig.unInterval > 0 && stConfig.unInterval <= unMaxInterval &&
           stConfig.unNumber >= 1 && stConfig.unNumber <= 120;
}

/* 校验越界和入侵规则数量及检测目标数量，保证 SDK 与 IPC 使用同一上限。 */
static bool is_valid_region_alarm_rule_count(const NET_CrossLineAlarmInfo_S &stConfig)
{
    if (stConfig.uRuleCount < 0 || stConfig.uRuleCount > 4)
    {
        return false;
    }
    for (INT32 nIndex = 0; nIndex < stConfig.uRuleCount; ++nIndex)
    {
        const NET_BoundaryPlane_S &stRule = stConfig.stRule[nIndex];
        if (!std::isfinite(stRule.fStartPosX) || !std::isfinite(stRule.fStartPosY) ||
            !std::isfinite(stRule.fEndPosX) || !std::isfinite(stRule.fEndPosY) ||
            stRule.fStartPosX < 0.0F || stRule.fStartPosX > 1.0F ||
            stRule.fStartPosY < 0.0F || stRule.fStartPosY > 1.0F ||
            stRule.fEndPosX < 0.0F || stRule.fEndPosX > 1.0F ||
            stRule.fEndPosY < 0.0F || stRule.fEndPosY > 1.0F ||
            stRule.enCrossDirection < 0 || stRule.enCrossDirection > 2 ||
            stRule.uDetectionTargetCount < 0 ||
            stRule.uDetectionTargetCount > 8 ||
            stRule.nSensitivity < 1 || stRule.nSensitivity > 100)
        {
            return false;
        }
        for (INT32 nTargetIndex = 0; nTargetIndex < stRule.uDetectionTargetCount; ++nTargetIndex)
        {
            if (stRule.auDetectionTarget[nTargetIndex] < NET_TARGET_ALL ||
                stRule.auDetectionTarget[nTargetIndex] > NET_TARGET_HUMAN_AND_VEHICLE)
            {
                return false;
            }
        }
    }
    return true;
}

static bool is_valid_region_alarm_rule_count(const NET_IntrusionAlarmInfo_S &stConfig)
{
    if (stConfig.uRuleCount < 0 || stConfig.uRuleCount > 4)
    {
        return false;
    }
    for (INT32 nIndex = 0; nIndex < stConfig.uRuleCount; ++nIndex)
    {
        const NET_IntrusionRule_S &stRule = stConfig.stRule[nIndex];
        if (stRule.uPointCount < 0 || stRule.uPointCount > 32 ||
            stRule.uDetectionTargetCount < 0 || stRule.uDetectionTargetCount > 8 ||
            stRule.nTimeThreshold < 0 || stRule.nTimeThreshold > 100 ||
            stRule.nSensitivity < 1 || stRule.nSensitivity > 100)
        {
            return false;
        }
        for (INT32 nPointIndex = 0; nPointIndex < stRule.uPointCount; ++nPointIndex)
        {
            if (!std::isfinite(stRule.afPointX[nPointIndex]) ||
                !std::isfinite(stRule.afPointY[nPointIndex]) ||
                stRule.afPointX[nPointIndex] < 0.0F || stRule.afPointX[nPointIndex] > 1.0F ||
                stRule.afPointY[nPointIndex] < 0.0F || stRule.afPointY[nPointIndex] > 1.0F)
            {
                return false;
            }
        }
        for (INT32 nTargetIndex = 0; nTargetIndex < stRule.uDetectionTargetCount; ++nTargetIndex)
        {
            if (stRule.auDetectionTarget[nTargetIndex] < NET_TARGET_ALL ||
                stRule.auDetectionTarget[nTargetIndex] > NET_TARGET_HUMAN_AND_VEHICLE)
            {
                return false;
            }
        }
    }
    return true;
}

/* 校验日夜切换参数的枚举值、时间字段和亮度范围。 */
static bool is_valid_daynight_config(const NET_DayNightInfo_S &stConfig)
{
    const auto bValidTime = [](INT32 nHour, INT32 nMinute, INT32 nSecond, INT32 nMilliSecond)
    {
        return nHour >= 0 && nHour <= 23 && nMinute >= 0 && nMinute <= 59 &&
               nSecond >= 0 && nSecond <= 59 && nMilliSecond >= 0 && nMilliSecond <= 999;
    };
    const bool bTimingMode = (stConfig.enDayNightMode == NET_DAYNIGHT_MODE_TIMING);
    return stConfig.enDayNightMode >= NET_DAYNIGHT_MODE_DAY && stConfig.enDayNightMode <= NET_DAYNIGHT_MODE_TIMING &&
           (!bTimingMode || (bValidTime(stConfig.nBeginHour, stConfig.nBeginMinute, stConfig.nBeginSecond, stConfig.nBeginMilliSec) &&
                             bValidTime(stConfig.nEndHour, stConfig.nEndMinute, stConfig.nEndSecond, stConfig.nEndMilliSec))) &&
           stConfig.nSensitivityLevel >= 1 && stConfig.nSensitivityLevel <= 7 &&
           stConfig.nFilterTime >= 5 && stConfig.nFilterTime <= 120 &&
           stConfig.enLightMode >= NET_LIGHT_BRIGHT_MANUAL && stConfig.enLightMode <= NET_LIGHT_BRIGHT_AUTO &&
           stConfig.enLightType >= NET_LIGHT_TYPE_WHITE && stConfig.enLightType <= NET_LIGHT_TYPE_WHITE_ON_RED_OFF &&
           stConfig.nWhiteLightLevel >= 0 && stConfig.nWhiteLightLevel <= 100 &&
           stConfig.nRedLightLevel >= 0 && stConfig.nRedLightLevel <= 100;
}

/* 校验背光区域、宽动态和强光抑制参数范围。 */
static bool is_valid_backlight_config(const NET_BackLightInfo_S &stConfig)
{
    return stConfig.enBackLightArea >= NET_BACKLIGHT_AREA_CLOSE &&
           stConfig.enBackLightArea <= NET_BACKLIGHT_AREA_CENTER &&
           stConfig.nWdrLevel >= 0 && stConfig.nWdrLevel <= 100 &&
           stConfig.nHlsLevel >= 0 && stConfig.nHlsLevel <= 100;
}

static const Video_NS::VideoConfig_S *FindVideoConfigById(const std::vector<Video_NS::VideoConfig_S> &vecCfg, int nId)
{
    for (const auto &stCfg : vecCfg)
    {
        if (stCfg.nId == nId)
        {
            return &stCfg;
        }
    }

    return vecCfg.empty() ? nullptr : &vecCfg.front();
}

static bool is_valid_live_stream_id(INT32 nId)
{
    return nId == NET_LIVE_STREAM_INDEX_MAIN || nId == NET_LIVE_STREAM_INDEX_AUX;
}

static bool is_valid_video_type(INT32 enVideoType)
{
    return enVideoType == static_cast<INT32>(Video_NS::VideoType_E::COMPOSITE_STREAM) ||
           enVideoType == static_cast<INT32>(Video_NS::VideoType_E::VIDEO_STREAM);
}

static bool is_valid_video_codec(INT32 enVideoCodec)
{
    return enVideoCodec >= NET_VIDEO_CODE_H264 && enVideoCodec <= NET_VIDEO_CODE_MPEG4;
}

static NET_COMMON_ECODE_E validate_set_stream_cfg(const NET_VideoEncodeOption_S &cfg)
{
    if (!is_valid_live_stream_id(cfg.nId))
    {
        dlog_warn("TVSDK设置视频编码参数失败: 非法码流ID[%d]", cfg.nId);
        return NET_E_INVALID_PARAM;
    }

    if (!is_valid_video_type(cfg.enVideoType))
    {
        dlog_warn("TVSDK设置视频编码参数失败: 非法视频类型[%d], 仅支持0-复合流/1-视频流", cfg.enVideoType);
        return NET_E_INVALID_PARAM;
    }

    if (!is_valid_video_codec(cfg.enVideoCodec))
    {
        dlog_warn("TVSDK设置视频编码参数失败: 非法视频编码[%d]", cfg.enVideoCodec);
        return NET_E_INVALID_PARAM;
    }

    const NET_COMMON_ECODE_E nResolutionRet = validate_video_resolution(cfg);
    if (nResolutionRet != NET_E_SUCCEED)
    {
        dlog_warn("TVSDK设置视频编码参数失败: 不支持的分辨率[%d x %d]",
                  cfg.stVideoResolution.uWidth,
                  cfg.stVideoResolution.uHeight);
        return nResolutionRet;
    }

    return NET_E_SUCCEED;
}

static std::string wrap_data_json(const std::string &srcJson)
{
    if (srcJson.empty())
        return "";

    // 1. 解析原始 JSON 字符串为对象
    Json::Object *pConfig = Json::init(srcJson.c_str());
    if (!pConfig)
        return srcJson;

    // 2. 创建一个新的根对象
    Json::Object *pRoot = Json::init(); 
    if (!pRoot) {
        Json::deinit(pConfig);
        return srcJson;
    }

    // 3. 将原始配置对象作为 "Data" 的值添加进去
    // 注意：这里取决于你的库函数名，通常是 add 或 set
    Json::add(pRoot, "Data", pConfig);

    // 4. 序列化为字符串
    std::string out = Json::to_string(pRoot);

    // 5. 释放资源 
    // 注意：如果 Json::add 会接管 pConfig 的内存，则只需释放 pRoot
    Json::deinit(pRoot); 

    return out;
}

static bool json_get_string_alias(Json::Object *pObj, const char *key1, const char *key2, std::string &out)
{
    if (!pObj)
        return false;
    if (key1 && Json::get(pObj, key1, out))
        return true;
    if (key2 && Json::get(pObj, key2, out))
        return true;
    return false;
}

static bool json_get_int_alias(Json::Object *pObj, const char *key1, const char *key2, int &out)
{
    if (!pObj)
        return false;
    if (key1 && Json::get(pObj, key1, out))
        return true;
    if (key2 && Json::get(pObj, key2, out))
        return true;
    return false;
}

static std::string wifi_sta_info_to_json(const Network::WifiStaInfo_S &cfg)
{
    Json::Object *pRoot = Json::init();
    if (!pRoot)
        return "{}";

    Json::add(pRoot, "EnableWifi", cfg.bEnableWifi);
    Json::add(pRoot, "EnableBoost", cfg.bEnableBoost);
    std::string out = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return out;
}

static std::string wifi_sta_connect_to_json(const Network::WifiStaConncet_S &cfg)
{
    Json::Object *pRoot = Json::init();
    if (!pRoot)
        return "{}";

    Json::add(pRoot, "Ssid", cfg.ssid);
    Json::add(pRoot, "Mode", (int)cfg.mode);
    Json::add(pRoot, "IpAddress", cfg.ip_address);
    Json::add(pRoot, "Password", cfg.password);
    Json::add(pRoot, "Pairwise", cfg.pairwise);
    Json::add(pRoot, "WepKeyLen", cfg.wep_key_len);
    Json::add(pRoot, "WepIsHex", cfg.wep_is_hex);
    Json::add(pRoot, "AuthAlg", cfg.auth_alg);

    Json::Object *pWepKeys = Json::Array::init();
    if (pWepKeys)
    {
        for (size_t i = 0; i < cfg.wep_keys.size() && i < 4; ++i)
        {
            Json::Object *pItem = Json::init();
            if (!pItem)
                continue;
            Json::add(pItem, "Index", cfg.wep_keys[i].index);
            Json::add(pItem, "Value", cfg.wep_keys[i].value);
            Json::Array::add(pWepKeys, pItem);
        }
        Json::add(pRoot, "WepKeys", pWepKeys);
    }

    Json::add(pRoot, "EapIdentity", cfg.eap_identity);
    Json::add(pRoot, "EapPassword", cfg.eap_password);
    Json::add(pRoot, "PeapVersion", cfg.peap_version);
    Json::add(pRoot, "Phase2", cfg.phase2);
    Json::add(pRoot, "AnonymousIdentity", cfg.anonymous_identity);
    Json::add(pRoot, "CaCertPath", cfg.ca_cert_path);
    Json::add(pRoot, "PeapLabel", cfg.peap_label);
    Json::add(pRoot, "TlsIdentity", cfg.tls_identity);
    Json::add(pRoot, "PrivateKeyPasswd", cfg.private_key_passwd);
    Json::add(pRoot, "EapolVersion", cfg.eapol_version);
    Json::add(pRoot, "ClientCertPath", cfg.client_cert_path);
    Json::add(pRoot, "PrivateKeyPath", cfg.private_key_path);
    Json::add(pRoot, "CtrlInterface", cfg.ctrl_interface);
    Json::add(pRoot, "InterfaceName", cfg.interface_name);

    

    std::string out = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return out;
}

static std::string config_4g_to_json(const Network::Network_4G_Config_t &cfg)
{
    Json::Object *pRoot = Json::init();
    if (!pRoot)
        return "{}";

    Json::add(pRoot, "Apn", cfg.apn);
    Json::add(pRoot, "UserName", cfg.username);
    Json::add(pRoot, "Password", cfg.password);
    Json::add(pRoot, "CallNumber", cfg.call_number);
    Json::add(pRoot, "Mtu", cfg.mtu);
    Json::add(pRoot, "AuthMode", cfg.auth_mode);
    Json::add(pRoot, "NetworkMode", cfg.network_mode);
    Json::add(pRoot, "DialMode", cfg.dial_mode);

    std::string out = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return out;
}

static bool parse_4g_from_json(const std::string &json, Network::Network_4G_Config_t &cfg)
{
    Json::Object *pRoot = Json::init(json.c_str());
    if (!pRoot)
        return false;

    bool bParsed = false;
    bParsed = json_get_string_alias(pRoot, "Apn", "apn", cfg.apn) || bParsed;
    bParsed = json_get_string_alias(pRoot, "UserName", "username", cfg.username) || bParsed;
    bParsed = json_get_string_alias(pRoot, "Password", "password", cfg.password) || bParsed;
    bParsed = json_get_string_alias(pRoot, "CallNumber", "call_number", cfg.call_number) || bParsed;
    bParsed = json_get_int_alias(pRoot, "Mtu", "mtu", cfg.mtu) || bParsed;
    bParsed = json_get_int_alias(pRoot, "AuthMode", "auth_mode", cfg.auth_mode) || bParsed;
    bParsed = json_get_int_alias(pRoot, "NetworkMode", "network_mode", cfg.network_mode) || bParsed;
    bParsed = json_get_int_alias(pRoot, "DialMode", "dial_mode", cfg.dial_mode) || bParsed;

    Json::deinit(pRoot);
    (void)bParsed;
    return true;
}

static std::string hotspot_to_json(const Network::HotspotConfig &cfg)
{
    Json::Object *pRoot = Json::init();
    if (!pRoot)
        return "{}";

    Json::add(pRoot, "Enabled", cfg.enabled);
    Json::add(pRoot, "Ssid", cfg.ssid);
    Json::add(pRoot, "SecurityMode", cfg.securityMode);
    Json::add(pRoot, "EncryptionType", cfg.encryptionType);
    Json::add(pRoot, "Password", cfg.password);
    Json::add(pRoot, "ConfirmPassword", cfg.confirmPassword);

    std::string out = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return out;
}

static int execute_action_expect_success(int actionCode, const std::string &inJson, std::string *pOutJson = nullptr)
{
    std::string outJson;
    if (execute_get_result(actionCode, inJson, outJson) != 0)
        return -1;

    /* 某些下游动作只返回执行码，不返回 JSON，此时 outJson 为空也视作成功。 */
    if (!outJson.empty())
    {
        int nRet = -1;
        Json::get(outJson.c_str(), "Return", nRet);
        if (nRet != 0)
        {
            dlog_error("[TVSDK][Upgrade] action=%d failed, ret=%d, body=%s", actionCode, nRet, outJson.c_str());
            return -1;
        }
    }

    if (pOutJson)
        *pOutJson = outJson;
    return 0;
}

/* 执行同步任务并提取统一响应中的 Data，避免各配置回调重复处理 Return 字段。 */
static bool execute_get_success_data(int actionCode, const std::string &inJson, std::string &dataJson)
{
    std::string outJson;
    if (execute_get_result(actionCode, inJson, outJson) != 0 || outJson.empty())
    {
        return false;
    }

    int nRet = -1;
    if (!Json::get(outJson.c_str(), "Return", nRet) || nRet != 0)
    {
        dlog_warn("[TVSDK] action=%d get failed, ret=%d, body=%s", actionCode, nRet, outJson.c_str());
        return false;
    }

    dataJson = normalize_data_json(outJson);
    return !dataJson.empty();
}

void set_task_manage(const std::shared_ptr<CTaskManage> &taskManage)
{
    s_taskManage = taskManage ? taskManage.get() : nullptr;
}

void clear_task_manage()
{
    s_taskManage = nullptr;
}

static int execute_get_result(int actionCode, const std::string &inJson, std::string &outJson)
{
    if (!s_taskManage)
        return -1;
    outJson.clear();

    struct ResultState
    {
        std::mutex mtx;
        std::condition_variable cv;
        bool hasResult = false;
        bool expired = false;
        std::string outJson;
    };
    std::shared_ptr<ResultState> state = std::make_shared<ResultState>();

    Task::Info_S stInfo;
    stInfo.data = inJson;
    stInfo.fnResultCallbacks = [state](const void *pData, int nLen, int /*nActionCode*/, void * /*pHandler*/) -> int {
        std::lock_guard<std::mutex> lock(state->mtx);
        if (state->expired)
            return 0;
        if (pData && nLen > 0)
            state->outJson.assign(static_cast<const char *>(pData), static_cast<size_t>(nLen));
        state->hasResult = true;
        state->cv.notify_one();
        return 0;
    };
    if (s_taskManage->execute(actionCode, stInfo) != 0)
        return -1;

    std::unique_lock<std::mutex> lock(state->mtx);
    if (!state->cv.wait_for(lock, std::chrono::milliseconds(5000), [state]() { return state->hasResult; }))
    {
        state->expired = true;
        dlog_warn("[TVSDK] action=%d wait result timeout", actionCode);
        return -1;
    }

    outJson = state->outJson;
    return 0;
}

/* ---------- GetDeviceInfo：由 SystemManage 填充 ---------- */
static NET_COMMON_ECODE_E cb_get_device_info_impl(pNET_DeviceInfo_S pInfo)
{
    if (!pInfo)
        return NET_E_NULL_POINT;
    ::System::DeviceInfo_S stDeviceInfo;
    if (SystemManage::instance()->get_device_info(stDeviceInfo) != 0)
        return NET_E_GET_CFG_FAILED;
    TvSdkConvert::FillDeviceInfo(stDeviceInfo, *pInfo);
    return NET_E_SUCCEED;
}

/* ---------- DeviceControl: TVSDK 声光控制 IPC 控制任务 ---------- */
static NET_COMMON_ECODE_E cb_device_control(pNET_DeviceControlInfo_S pCtrlInfo)
{
    if (!pCtrlInfo)
    {
        return NET_E_NULL_POINT;
    }

    if (pCtrlInfo->uSize != sizeof(*pCtrlInfo) || pCtrlInfo->uChannelID <= 0 ||
        pCtrlInfo->uDurationMs < 0 || pCtrlInfo->uDurationMs > 3000000)
    {
        return NET_E_INVALID_PARAM;
    }

    if (pCtrlInfo->uControlType != NET_DEVICE_CTRL_TYPE_ALARM_LIGHT)
    {
        return NET_E_NOT_SUPPORT;
    }

    if (pCtrlInfo->uCommand != NET_ALARM_LIGHT_CTRL_START && pCtrlInfo->uCommand != NET_ALARM_LIGHT_CTRL_STOP && pCtrlInfo->uCommand != NET_ALARM_LIGHT_CTRL_SET_MODE)
    {
        return NET_E_INVALID_PARAM;
    }

#if !CAP_ALARM_IO
    return NET_E_NOT_SUPPORT;
#else
    ::Preview::DeviceControl_S stControl;
    stControl.nChannelId = pCtrlInfo->uChannelID - 1;
    stControl.nControlType = pCtrlInfo->uControlType;
    stControl.nCommand = pCtrlInfo->uCommand;
    stControl.nDurationMs = pCtrlInfo->uDurationMs;
    stControl.nParam1 = pCtrlInfo->uParam1;
    stControl.nParam2 = pCtrlInfo->uParam2;
    stControl.strExt.assign(pCtrlInfo->szExt, strnlen(pCtrlInfo->szExt, sizeof(pCtrlInfo->szExt)));

    std::string outJson;
    if(execute_get_result(AC_DEVICE_CONTROL, wrap_data_json(Convert::to_string(stControl)), outJson) != 0)
    {
        return NET_E_SET_CFG_FAILED;
    }

    int nRet = ERR;
    if(outJson.empty() || !Json::get(outJson.c_str(), "Return", nRet) || nRet != OK)
    {
        dlog_warn("TVSDK声光控制任务失败： return[%d], body[%s]", nRet, outJson.c_str());
        return NET_E_SET_CFG_FAILED;
    }

    return NET_E_SUCCEED;
#endif
}

/* ---------- GetVideoEncodeCap：AC_GET_VIDEO_CAPABILITY_SET ---------- */
static NET_COMMON_ECODE_E cb_get_video_encode_cap(INT32 dwChannelID, pNET_VideoEncodeCap_S pCap)
{
    if (!pCap)
        return NET_E_NULL_POINT;
    memset(pCap, 0, sizeof(NET_VideoEncodeCap_S));
    (void)dwChannelID;

    std::string outJson;
    if (execute_get_result(AC_GET_VIDEO_CAPABILITY_SET, "{}", outJson) != 0 || outJson.empty())
    {
        pCap->uStreamCount = 0;
        return NET_E_SUCCEED;
    }

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;

    Video_NS::VideoCapabilitySet_S stCapSet;
    Convert::to_struct(strJson, stCapSet);
    TvSdkConvert::FillVideoEncodeCap(stCapSet, *pCap);

    return NET_E_SUCCEED;
}

/* ---------- GetAudioEncodeCap：AC_GET_AUDIO_CAPABILITY_SET ---------- */
static NET_COMMON_ECODE_E cb_get_audio_encode_cap(INT32 dwChannelID, pNET_AudioCap_S pCap)
{
    if (!pCap)
        return NET_E_NULL_POINT;

    memset(pCap, 0, sizeof(NET_AudioCap_S));
    (void)dwChannelID;

    std::string outJson;
    if (execute_get_result(AC_GET_AUDIO_CAPABILITY_SET, "{}", outJson) != 0 || outJson.empty())
        return NET_E_SUCCEED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Audio_NS::AudioCapabilitySet_S stCapSet;
    Convert::to_struct(outJson, stCapSet);
    TvSdkConvert::FillAudioEncodeCap(stCapSet, *pCap);

    return NET_E_SUCCEED;
}

/* ---------- GetOsdCap：OSD能力集 ---------- */
static NET_COMMON_ECODE_E cb_get_osd_cap(INT32 dwChannelID, pNET_OsdCap_S pCap)
{
    (void)dwChannelID;
    if (!pCap)
        return NET_E_NULL_POINT;
    memset(pCap, 0, sizeof(NET_OsdCap_S));

    pCap->bSupportOsd = TRUE;
    pCap->bSupportName = TRUE;
    pCap->bSupportTime = TRUE;
    pCap->bSupportWeek = TRUE;
    pCap->bSupportCustomColor = TRUE;
    pCap->udwMaxOsdNum = 4;

    pCap->udwSupportedFontSizeNum = 4;
    pCap->audwSupportedFontSizeList[0] = NET_OSD_FONT_SIZE_ADAPTIVE;
    pCap->audwSupportedFontSizeList[1] = NET_OSD_FONT_SIZE_16;
    pCap->audwSupportedFontSizeList[2] = NET_OSD_FONT_SIZE_32;
    pCap->audwSupportedFontSizeList[3] = NET_OSD_FONT_SIZE_48;

    pCap->udwSupportedDateFormatNum = 9;
    pCap->audwSupportedDateFormatList[0] = NET_OSD_DATE_YYYY_MM_DD;
    pCap->audwSupportedDateFormatList[1] = NET_OSD_DATE_MM_DD_YYYY;
    pCap->audwSupportedDateFormatList[2] = NET_OSD_DATE_DD_MM_YYYY;
    pCap->audwSupportedDateFormatList[3] = NET_OSD_DATE_YYYY_MM_DD_CHN;
    pCap->audwSupportedDateFormatList[4] = NET_OSD_DATE_MM_DD_YYYY_CHN;
    pCap->audwSupportedDateFormatList[5] = NET_OSD_DATE_DD_MM_YYYY_CHN;
    pCap->audwSupportedDateFormatList[6] = NET_OSD_DATE_YYYY_MM_DD_SLASH;
    pCap->audwSupportedDateFormatList[7] = NET_OSD_DATE_MM_DD_YYYY_SLASH;
    pCap->audwSupportedDateFormatList[8] = NET_OSD_DATE_DD_MM_YYYY_SLASH;

    pCap->udwSupportedTimeFormatNum = 2;
    pCap->audwSupportedTimeFormatList[0] = NET_OSD_TIME_FORMAT_24;
    pCap->audwSupportedTimeFormatList[1] = NET_OSD_TIME_FORMAT_12;

    pCap->udwSupportedAlignNum = 6;
    pCap->audwSupportedAlignList[0] = NET_OSD_ALIGN_CUSTOMIZE;
    pCap->audwSupportedAlignList[1] = NET_OSD_ALIGN_CHAR_LEFT;
    pCap->audwSupportedAlignList[2] = NET_OSD_ALIGN_CHAR_RIGHT;
    pCap->audwSupportedAlignList[3] = NET_OSD_ALIGN_ALL_LEFT;
    pCap->audwSupportedAlignList[4] = NET_OSD_ALIGN_ALL_RIGHT;
    pCap->audwSupportedAlignList[5] = NET_OSD_ALIGN_GB_MODE;

    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_get_device_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_DeviceBasicInfo_S pOut = (pNET_DeviceBasicInfo_S)lpOutBuffer;

    ::System::DeviceInfo_S stDeviceInfo;
    if (SystemManage::instance()->get_device_info(stDeviceInfo) != 0)
        return NET_E_GET_CFG_FAILED;

    TvSdkConvert::FillDeviceBasicInfo(stDeviceInfo, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_device_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    pNET_DeviceBasicInfo_S pIn = (pNET_DeviceBasicInfo_S)lpInBuffer;
    ::System::DeviceInfo_S stDeviceInfo;
    // 先读取当前信息，避免覆盖其他字段
    (void)SystemManage::instance()->get_device_info(stDeviceInfo);
    TvSdkConvert::ToDeviceInfo(*pIn, stDeviceInfo);

    int nRet = SystemManage::instance()->set_device_info(stDeviceInfo);
    return (nRet == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* 其余配置仍通过命令码 + JSON 透传，后续若有 SDK 结构体定义，可按上面的方式继续细化 */

static NET_COMMON_ECODE_E get_cfg_by_action(INT32 dwChannelID, int actionCode, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_NULL_POINT;
    if (!s_taskManage)
        return NET_E_GET_CFG_FAILED;

    std::string outJson;
    if (execute_get_result(actionCode, "{}", outJson) != 0)
        return NET_E_GET_CFG_FAILED;

    size_t len = outJson.size() + 1;
    if (len > NET_LEN_4096)
        return NET_E_NOENOUGH_BUF;
    memcpy(lpOutBuffer, outJson.c_str(), len);
    return NET_E_SUCCEED;
}

/**
 * @brief 校验 SDK 布尔值是否可以转换为 IPC 布尔值。
 * @author ITC
 * @param [in] bValue 待校验的 SDK 布尔值。
 * @return 合法返回 true，否则返回 false。
 */
static bool is_valid_sdk_bool(BOOL bValue)
{
    return bValue == FALSE || bValue == TRUE;
}

/**
 * @brief 校验 SDK 调用方传入的固定长度周布防时间表。
 * @author ITC
 * @param [in] stSchedule 待校验的布防时间表。
 * @return 合法返回 true，否则返回 false。
 */
static bool is_valid_alarm_schedule(const NET_AlarmSchedule_S& stSchedule)
{
    for (INT32 nDay = 0; nDay < NET_ALARM_SCHEDULE_DAY_COUNT; ++nDay)
    {
        const INT32 nSectionCount = stSchedule.uTimeSectionCount[nDay];
        if (nSectionCount < 0 || nSectionCount > NET_PLAN_SECTION_NUM)
        {
            return false;
        }

        for (INT32 nSection = 0; nSection < nSectionCount; ++nSection)
        {
            const NET_SchedTime_S& stTime = stSchedule.astTimeSection[nDay][nSection];
            if (stTime.nStartHour < NET_ALARM_SCHEDULE_HOUR_MIN ||
                stTime.nStartHour > NET_ALARM_SCHEDULE_HOUR_MAX ||
                stTime.nEndHour < NET_ALARM_SCHEDULE_HOUR_MIN ||
                stTime.nEndHour > NET_ALARM_SCHEDULE_HOUR_MAX ||
                stTime.nStartMinute < NET_ALARM_SCHEDULE_MINUTE_MIN ||
                stTime.nStartMinute > NET_ALARM_SCHEDULE_MINUTE_MAX ||
                stTime.nEndMinute < NET_ALARM_SCHEDULE_MINUTE_MIN ||
                stTime.nEndMinute > NET_ALARM_SCHEDULE_MINUTE_MAX)
            {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 校验联动列表的元素数量是否在固定数组容量范围内。
 * @author ITC
 * @param [in] stLinkageList 待校验的联动列表。
 * @return 合法返回 true，否则返回 false。
 */
static bool is_valid_alarm_linkage(const NET_LinkageList_S& stLinkageList)
{
    if (stLinkageList.uAlarmOutputCount < 0 ||
        stLinkageList.uAlarmOutputCount > NET_MAX_ALARM_OUT_NUM ||
        stLinkageList.uRecordChannelCount < 0 ||
        stLinkageList.uRecordChannelCount > NET_CHANNEL_MAX ||
        stLinkageList.uSnapshotChannelCount < 0 ||
        stLinkageList.uSnapshotChannelCount > NET_CHANNEL_MAX)
    {
        return false;
    }

    return true;
}

/**
 * @brief 校验复制到通道列表的元素数量是否在允许范围内。
 * @author ITC
 * @param [in] nCopyToCount 待校验的复制到通道数量。
 * @return 合法返回 true，否则返回 false。
 */
static bool is_valid_alarm_copy_to_count(INT32 nCopyToCount)
{
    return nCopyToCount >= 0 && nCopyToCount <= NET_ALARM_COPY_TO_MAX_NUM;
}

/**
 * @brief 校验 SDK 传入的声音告警配置。
 * @author ITC
 * @param [in] stInfo 待校验的声音告警配置。
 * @return 合法返回 true，否则返回 false。
 */
static bool is_valid_audible_alarm_info(const NET_AudibleAlarmInfo_S& stInfo)
{
    if (stInfo.enSoundType < NET_AUDIBLE_ALARM_SOUND_TYPE_WARNING ||
        stInfo.enSoundType > NET_AUDIBLE_ALARM_SOUND_TYPE_CUSTOM ||
        stInfo.enAlertSound < NET_AUDIBLE_ALARM_ALERT_SOUND_WARNING_ZONE_LEAVE_IMMEDIATELY ||
        stInfo.enAlertSound > NET_AUDIBLE_ALARM_ALERT_SOUND_GENERAL_WARNING_TONE ||
        stInfo.nTimes < NET_AUDIBLE_ALARM_PLAY_TIMES_MIN ||
        stInfo.nTimes > NET_AUDIBLE_ALARM_PLAY_TIMES_MAX ||
        stInfo.nCustomAudioCount < 0 ||
        stInfo.nCustomAudioCount > NET_AUDIBLE_ALARM_CUSTOM_AUDIO_MAX_NUM ||
        !is_valid_alarm_schedule(stInfo.stAlarmSchedule))
    {
        return false;
    }

    for (INT32 nIndex = 0; nIndex < stInfo.nCustomAudioCount; ++nIndex)
    {
        if (!is_valid_sdk_bool(stInfo.astCustomAudios[nIndex].bSelected))
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief 校验 SDK 传入的一路报警输入配置。
 * @author ITC
 * @param [in] stInfo 待校验的报警输入配置。
 * @return 合法返回 true，否则返回 false。
 */
static bool is_valid_alarm_input_info(const NET_AlarmInputInfo_S& stInfo)
{
    return stInfo.nAlarmNumber >= 0 && stInfo.nAlarmNumber < NET_MAX_ALARM_IN_NUM &&
           is_valid_sdk_bool(stInfo.bNormallyOpen) &&
           (stInfo.nDealType == NET_ALARM_INPUT_DEAL_TYPE_DISABLED ||
            stInfo.nDealType == NET_ALARM_INPUT_DEAL_TYPE_ENABLED) &&
           is_valid_alarm_schedule(stInfo.stAlarmSchedule) &&
           is_valid_alarm_linkage(stInfo.stLinkageList) &&
           is_valid_alarm_copy_to_count(stInfo.nCopyToCount);
}

/**
 * @brief 校验 SDK 传入的一路报警输出配置。
 * @author ITC
 * @param [in] stInfo 待校验的报警输出配置。
 * @return 合法返回 true，否则返回 false。
 */
static bool is_valid_alarm_output_info(const NET_AlarmOutputInfo_S& stInfo)
{
    return stInfo.nAlarmNumber >= 0 && stInfo.nAlarmNumber < NET_MAX_ALARM_OUT_NUM &&
           stInfo.nDelayTime >= 0 &&
           stInfo.enState >= NET_ALARM_OUTPUT_STATE_OFF &&
           stInfo.enState <= NET_ALARM_OUTPUT_STATE_HUMAN_ON &&
           is_valid_alarm_schedule(stInfo.stAlarmSchedule) &&
           is_valid_alarm_copy_to_count(stInfo.nCopyToCount);
}

/**
 * @brief 校验 SDK 传入的闪光灯告警配置。
 * @author ITC
 * @param [in] stInfo 待校验的闪光灯告警配置。
 * @return 合法返回 true，否则返回 false。
 */
static bool is_valid_flashing_light_alarm_info(const NET_FlashingLightAlarmInfo_S& stInfo)
{
    return stInfo.nFlashTime >= NET_FLASHING_LIGHT_ALARM_TIME_MIN &&
           stInfo.nFlashTime <= NET_FLASHING_LIGHT_ALARM_TIME_MAX &&
           stInfo.enFlashFrequency >= NET_FLASHING_LIGHT_FREQUENCY_STEADY_ON &&
           stInfo.enFlashFrequency <= NET_FLASHING_LIGHT_FREQUENCY_HIGH &&
           is_valid_alarm_schedule(stInfo.stAlarmSchedule) &&
           is_valid_alarm_copy_to_count(stInfo.nCopyToCount);
}

/**
 * @brief 校验 SDK 传入的 PIR 告警配置。
 * @author ITC
 * @param [in] stInfo 待校验的 PIR 告警配置。
 * @return 合法返回 true，否则返回 false。
 */
static bool is_valid_pir_alarm_info(const NET_PirAlarmInfo_S& stInfo)
{
    return is_valid_sdk_bool(stInfo.bEnable) &&
           is_valid_alarm_schedule(stInfo.stAlarmSchedule) &&
           is_valid_alarm_linkage(stInfo.stLinkageList) &&
           is_valid_alarm_copy_to_count(stInfo.nCopyToCount);
}

/**
 * @brief 获取并转换 SDK 服务端所需的 SD 卡物理状态。
 * @author ITC
 * @param [in] nChannelId 设备通道标识。SD 卡状态为设备级信息，本参数不参与查询。
 * @param [out] pOutBuffer 输出的 NET_SdCardStatus_S 状态结构体缓冲区。
 * @return 成功返回 NET_E_SUCCEED；否则返回配置获取失败码。
 */
static NET_COMMON_ECODE_E cb_get_sd_card_status(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SdCardStatus_S pSdCardStatus = static_cast<pNET_SdCardStatus_S>(pOutBuffer);
    *pSdCardStatus = {};

    std::string strOutJson;
    if (execute_get_result(AC_GET_SD_CARD_STATUS, "{}", strOutJson) != 0 || strOutJson.empty())
    {
        return NET_E_GET_CFG_FAILED;
    }

    int nTaskResult = ERR;
    if (!Json::get(strOutJson.c_str(), "Return", nTaskResult) || nTaskResult != OK)
    {
        return NET_E_GET_CFG_FAILED;
    }

    const std::string strDataJson = normalize_data_json(strOutJson);
    if (strDataJson.empty())
    {
        return NET_E_GET_CFG_FAILED;
    }

    Json::Object *pDataJson = Json::init(strDataJson.c_str());
    if (!pDataJson)
    {
        return NET_E_GET_CFG_FAILED;
    }

    int nStatus = NET_SD_CARD_STATUS_UNPLUGGED;
    bool bReady = false;
    const bool bHasStatus = Json::get(pDataJson, "Status", nStatus);
    const bool bHasStatusText = Json::get(pDataJson,
                                          "StatusText",
                                          sizeof(pSdCardStatus->strStatusText),
                                          pSdCardStatus->strStatusText);
    const bool bHasReady = Json::get(pDataJson, "Ready", bReady);
    Json::deinit(pDataJson);

    if (!bHasStatus || !bHasStatusText || !bHasReady)
    {
        return NET_E_GET_CFG_FAILED;
    }

    pSdCardStatus->nStatus = nStatus;
    pSdCardStatus->bReady = bReady ? TRUE : FALSE;
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E set_cfg_by_action(INT32 dwChannelID, int actionCode, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_NULL_POINT;
    if (!s_taskManage)
        return NET_E_SET_CFG_FAILED;

    Task::Info_S stInfo;
    stInfo.data = std::string(static_cast<const char *>(lpInBuffer));
    int nRet = s_taskManage->execute(actionCode, stInfo);
    return (nRet == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
static NET_COMMON_ECODE_E cb_get_ntp_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_NULL_POINT;

    std::string outJson;
    if (execute_get_result(AC_GET_TIME_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;

    ::System::TimeInfo_S stTimeInfo;
    Convert::to_struct(strJson, stTimeInfo);
    TvSdkConvert::FillSystemNtpInfo(stTimeInfo, *(pNET_SystemNtpInfo_S)lpOutBuffer);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_ntp_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_NULL_POINT;
    if (!s_taskManage)
        return NET_E_SET_CFG_FAILED;

    const NET_SystemNtpInfo_S *pIn = (const NET_SystemNtpInfo_S *)lpInBuffer;
    const size_t nDateTimeLength = strnlen(pIn->strDateTime, sizeof(pIn->strDateTime));
    if (nDateTimeLength > 0 &&
        !is_valid_date_time(pIn->strDateTime, pIn->enDateFormat))
    {
        dlog_warn("TVSDK设置NTP参数失败: 手动同步日期时间格式无效");
        return NET_E_INVALID_PARAM;
    }
    ::System::TimeInfo_S stTimeInfo;
    TvSdkConvert::ToTimeInfo(*pIn, stTimeInfo);
    if (stTimeInfo.bManualSync)
    {
        /* 手动校时时关闭 NTP，避免校准后的时间立即被自动校时覆盖。 */
        stTimeInfo.bEnableNTPSync = false;
    }

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stTimeInfo));
    int nRet = s_taskManage->execute(AC_SET_TIME_INFO, stInfo);
    return (nRet == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/**
 * @brief 处理 NVR 直接下发的系统时间设置请求。
 * @param [in] dwChannelID SDK 通道号，单通道 IPC 当前不使用该参数。
 * @param [in] lpInBuffer 指向 NET_SystemTime_S 的输入缓冲区。
 * @return 设置成功返回 NET_E_SUCCEED，否则返回对应错误码。
 */
static NET_COMMON_ECODE_E cb_set_system_time(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
    {
        return NET_E_NULL_POINT;
    }
    if (!s_taskManage)
    {
        return NET_E_SET_CFG_FAILED;
    }

    const NET_SystemTime_S *pIn = static_cast<const NET_SystemTime_S *>(lpInBuffer);
    if (!is_valid_date_time(pIn->strDateTime,
                            static_cast<INT32>(::System::DateFormat_E::YYYY_MM_DD)))
    {
        return NET_E_INVALID_PARAM;
    }

    ::System::TimeInfo_S stTimeInfo;
    CTimeManage::instance()->get_time_info(stTimeInfo);
    stTimeInfo.enDateFormat = ::System::DateFormat_E::YYYY_MM_DD;
    stTimeInfo.bManualSync = true;
    stTimeInfo.bEnableNTPSync = false;
    stTimeInfo.strDateTime.assign(pIn->strDateTime,
                                  strnlen(pIn->strDateTime, sizeof(pIn->strDateTime)));

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stTimeInfo));
    const int nRet = s_taskManage->execute(AC_SET_TIME_INFO, stInfo);
    return (nRet == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_stream_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_VideoEncodeOption_S pOut = (pNET_VideoEncodeOption_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_VIDEO_CONFIG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;

    std::vector<Video_NS::VideoConfig_S> vecCfg;
    Convert::to_struct(strJson, vecCfg);

    const Video_NS::VideoConfig_S *pSelectedCfg = FindVideoConfigById(vecCfg, NET_LIVE_STREAM_INDEX_MAIN);
    if (!pSelectedCfg)
        return NET_E_GET_CFG_FAILED;

    TvSdkConvert::FillVideoEncodeOption(*pSelectedCfg, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_stream_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_VideoEncodeOption_S *pIn = (const NET_VideoEncodeOption_S *)lpInBuffer;
    NET_COMMON_ECODE_E nValid = validate_set_stream_cfg(*pIn);
    if (nValid != NET_E_SUCCEED)
        return nValid;

    Video_NS::VideoConfig_S stCfg;
    TvSdkConvert::ToVideoConfig(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_VIDEO_CONFIG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
static NET_COMMON_ECODE_E cb_get_osd_cap_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_VideoOsdCfg_S pOut = (pNET_VideoOsdCfg_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_OSD_CONFIG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;

    Osd::OsdConfig_S stCfg;
    stCfg.clear();
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillOsdConfig(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_osd_cap_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_VideoOsdCfg_S *pIn = (const NET_VideoOsdCfg_S *)lpInBuffer;
    Osd::OsdConfig_S stCfg;
    TvSdkConvert::ToOsdConfig(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_OSD_CONFIG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
static NET_COMMON_ECODE_E cb_get_image_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_ImageSetting_S pOut = (pNET_ImageSetting_S)lpOutBuffer;
    std::memset(pOut, 0, sizeof(*pOut));

    std::string outJson;
    if (execute_get_result(AC_GET_VIDEO_EFFECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;

    ISP::ImageParam_S stCfg;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillImageSetting(stCfg, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_image_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_ImageSetting_S *pIn = (const NET_ImageSetting_S *)lpInBuffer;
    ISP::ImageParam_S stCfg;
    TvSdkConvert::ToImageParam(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_VIDEO_EFFECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
static NET_COMMON_ECODE_E cb_get_network_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_NetworkCfg_S pOut = (pNET_NetworkCfg_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_NETWORK_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    /* 获取网络配置时先提取统一响应中的 Data 节点，避免结构体保留默认地址。 */
    const std::string strNetworkJson = normalize_data_json(outJson);
    if (strNetworkJson.empty())
    {
        dlog_error("获取网络配置响应缺少 Data 节点：%s", outJson.c_str());
        return NET_E_GET_CFG_FAILED;
    }

    Network::Info_S stNetInfo{};
    Convert::to_struct(strNetworkJson, stNetInfo);
    TvSdkConvert::FillNetworkCfg(stNetInfo, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_network_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    pNET_NetworkCfg_S pIn = (pNET_NetworkCfg_S)lpInBuffer;

    Network::Info_S stNetInfo;
    TvSdkConvert::ToNetworkInfo(*pIn, stNetInfo);

    std::string inJson = Convert::to_string(stNetInfo);
    Task::Info_S stInfo;
    stInfo.data = inJson;
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_NETWORK_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

int apply_discovery_network(const tagNET_PoeNetworkConfig *pConfig)
{
    if (!pConfig || !s_taskManage)
        return NET_E_INVALID_PARAM;

    if (pConfig->szTargetIP[0] == '\0' || pConfig->szSubnetMask[0] == '\0' ||
        (pConfig->bSetGateway == TRUE && pConfig->szGateway[0] == '\0'))
    {
        dlog_error("组播改网参数无效：目标IP[%s] 子网掩码[%s] 网关[%s]",
                   pConfig->szTargetIP,
                   pConfig->szSubnetMask,
                   pConfig->szGateway);
        return NET_E_INVALID_PARAM;
    }

    std::string outJson;
    if (execute_get_result(AC_GET_NETWORK_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    /* GET 网络配置返回的是统一响应，必须先提取 Data 节点再转换网络结构体。 */
    const std::string strNetworkJson = normalize_data_json(outJson);
    if (strNetworkJson.empty())
    {
        dlog_error("组播改网获取网络配置缺少 Data 节点：%s", outJson.c_str());
        return NET_E_GET_CFG_FAILED;
    }

    Network::Info_S stNetworkInfo{};
    Convert::to_struct(strNetworkJson, stNetworkInfo);
    stNetworkInfo.stIp.bEnableDhcp = (pConfig->bIPv4DHCP == TRUE);
    stNetworkInfo.stIp.ipv4Ip = pConfig->szTargetIP;
    stNetworkInfo.stIp.ipv4Mask = pConfig->szSubnetMask;
    if (pConfig->bSetGateway == TRUE)
        stNetworkInfo.stIp.ipv4Gateway = pConfig->szGateway;

    dlog_info("组播改网准备设置：网卡[%s] IP[%s] 掩码[%s] 网关[%s] DHCP[%d]",
              stNetworkInfo.stIp.netName.c_str(),
              stNetworkInfo.stIp.ipv4Ip.c_str(),
              stNetworkInfo.stIp.ipv4Mask.c_str(),
              stNetworkInfo.stIp.ipv4Gateway.c_str(),
              stNetworkInfo.stIp.bEnableDhcp ? 1 : 0);

    Task::Info_S stInfo;
    /* 任务框架会从 Data 节点提取任务参数，设置网络时必须使用统一包装格式。 */
    stInfo.data = wrap_data_json(Convert::to_string(stNetworkInfo));
    std::string setResult;
    if (execute_get_result(AC_SET_NETWORK_INFO, stInfo.data, setResult) != 0 || setResult.empty())
        return NET_E_SET_CFG_FAILED;

    nRet = -1;
    Json::get(setResult.c_str(), "Return", nRet);
    if (nRet != 0 && nRet != IpcRet_E::OK_SETNETWORK_AND_REBOOT)
        return NET_E_SET_CFG_FAILED;

    /* 网络配置已保存且需要重启时，启动设备延时重启流程。 */
    if (nRet == IpcRet_E::OK_SETNETWORK_AND_REBOOT)
    {
        const int nRebootRet = SystemManage::instance()->system_reboot([](int) {});
        if (nRebootRet != 0)
            return NET_E_SET_CFG_FAILED;
    }

    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_config_wifi_sta(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_WifiStaCfg_S *pIn = (const NET_WifiStaCfg_S *)lpInBuffer;
    Network::WifiStaInfo_S stCfg;
    TvSdkConvert::ToWifiStaInfo(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(wifi_sta_info_to_json(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CONFIG_WIFI_STA, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_connect_wifi_sta(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_WifiStaConnect_S *pIn = (const NET_WifiStaConnect_S *)lpInBuffer;
    Network::WifiStaConncet_S stCfg;
    TvSdkConvert::ToWifiStaConnect(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(wifi_sta_connect_to_json(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_CONNECT_WIFI_STA, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_disconnect_wifi_sta(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    (void)lpInBuffer;

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json("{}");
    int nExec = s_taskManage ? s_taskManage->execute(AC_DISCONNECT_WIFI_STA, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_4g_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_4GInfo_S pOut = (pNET_4GInfo_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_4G_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Network::Network_4G_Config_t stCfg;
    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        dataJson = outJson;
    (void)parse_4g_from_json(dataJson, stCfg);

    TvSdkConvert::Fill4GInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_4g_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_4GInfo_S *pIn = (const NET_4GInfo_S *)lpInBuffer;
    Network::Network_4G_Config_t stCfg;
    TvSdkConvert::To4GConfig(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(config_4g_to_json(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_4G_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_set_hotspot_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_HotspotInfo_S *pIn = (const NET_HotspotInfo_S *)lpInBuffer;
    Network::HotspotConfig stCfg;
    TvSdkConvert::ToHotspotConfig(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(hotspot_to_json(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_HOTSPOT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_preview_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_PreviewInfo_S pOut = (pNET_PreviewInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PREVIEW_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Preview::PreviewInfo_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPreviewInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_preview_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_PreviewInfo_S *pIn = (const NET_PreviewInfo_S *)lpInBuffer;

    Preview::PreviewInfo_S stCfg;
    TvSdkConvert::ToPreviewInfo(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PREVIEW_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_privacy_mask_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_PrivacyMaskCfg_S pOut = (pNET_PrivacyMaskCfg_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_SHELTER_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;

    Osd::CoverConfig_S stCfg;
    stCfg.clear();
    stCfg.vecCoverAttr.clear();
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPrivacyMaskCfg(stCfg, COsdManage::instance()->get_cover_max_area_count(), *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_privacy_mask_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_PrivacyMaskCfg_S *pIn = (const NET_PrivacyMaskCfg_S *)lpInBuffer;
    Osd::CoverConfig_S stCfg;
    const size_t maxAreaCount = COsdManage::instance()->get_cover_max_area_count();
    if (!TvSdkConvert::ToPrivacyMaskCfg(*pIn, maxAreaCount, stCfg))
    {
        dlog_warn("TVSDK隐私遮盖区域数非法, request:%d, max:%zu", pIn->uAreaCount, maxAreaCount);
        return NET_E_INVALID_PARAM;
    }

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SHELTER_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
static NET_COMMON_ECODE_E cb_get_tamper_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_TamperAlarmInfo_S pOut = (pNET_TamperAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AG_GET_HIDE_ALARM_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::HideAlarm_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillTamperAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_tamper_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_TamperAlarmInfo_S *pIn = (const NET_TamperAlarmInfo_S *)lpInBuffer;
    if (pIn->uSensitivity < 0 || pIn->uSensitivity > 3)
    {
        return NET_E_INVALID_PARAM;
    }
    Alarm::HideAlarm_S stCfg;
    TvSdkConvert::ToHideAlarm(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AG_SET_HIDE_ALARM_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
static NET_COMMON_ECODE_E cb_get_motion_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_MotionAlarmInfo_S pOut = (pNET_MotionAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_MOTION_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::MotionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);

    // 使用全局中间变量填充 TVSDK 结构体，再 memcpy 到 SDK 缓冲区
    std::memset(&g_tvMotionAlarmInfo, 0, sizeof(g_tvMotionAlarmInfo));
    TvSdkConvert::FillMotionAlarmInfo(stCfg, g_tvMotionAlarmInfo);
    std::memcpy(pOut, &g_tvMotionAlarmInfo, sizeof(g_tvMotionAlarmInfo));
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_motion_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_MotionAlarmInfo_S *pIn = (const NET_MotionAlarmInfo_S *)lpInBuffer;

    Alarm::MotionDetection_S stCfg;
    TvSdkConvert::ToMotionDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    dlog_debug("\ncb_set_motion_alarm :stInfo.data result:%s\n", stInfo.data.c_str());
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_MOTION_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_cross_line_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_CrossLineAlarmInfo_S pOut = (pNET_CrossLineAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_LINE_CROSSING_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::BoundaryDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillCrossLineAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_cross_line_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_CrossLineAlarmInfo_S *pIn = (const NET_CrossLineAlarmInfo_S *)lpInBuffer;
    if (!is_valid_region_alarm_rule_count(*pIn))
    {
        return NET_E_INVALID_PARAM;
    }
    Alarm::BoundaryDetection_S stCfg;
    TvSdkConvert::ToBoundaryDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
     stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_LINE_CROSSING_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
static NET_COMMON_ECODE_E cb_get_intrusion_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_IntrusionAlarmInfo_S pOut = (pNET_IntrusionAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_REGIONAL_INTRUSION_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::FieldDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillIntrusionAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_intrusion_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_IntrusionAlarmInfo_S *pIn = (const NET_IntrusionAlarmInfo_S *)lpInBuffer;
    if (!is_valid_region_alarm_rule_count(*pIn))
    {
        return NET_E_INVALID_PARAM;
    }
    Alarm::FieldDetection_S stCfg;
    TvSdkConvert::ToFieldDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
     stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_REGIONAL_INTRUSION_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
/*-----------------------------------获取/设置徘徊侦测-------------------------------------*/
static NET_COMMON_ECODE_E cb_get_loitering_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_LoiteringAlarmInfo_S pOut = (pNET_LoiteringAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_LOITERING_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::LoiteringDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillLoiteringAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_loitering_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_LoiteringAlarmInfo_S *pIn = (const NET_LoiteringAlarmInfo_S *)lpInBuffer;
    Alarm::LoiteringDetection_S stCfg;
    TvSdkConvert::ToLoiteringDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
     stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_LOITERING_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* ---------- Get/SetSceneChangeAlarm：AC_GET/SET_SCENE_CHANGE_DETECT_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_scene_change_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_SceneChangeAlarmInfo_S pOut = (pNET_SceneChangeAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SCENE_CHANGE_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::SceneChange_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSceneChangeAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_scene_change_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_SceneChangeAlarmInfo_S *pIn = (const NET_SceneChangeAlarmInfo_S *)lpInBuffer;

    Alarm::SceneChange_S stCfg;
    TvSdkConvert::ToSceneChange(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SCENE_CHANGE_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* ---------- Get/SetCrowdGatheringAlarm：AC_GET/SET_CROWD_GATHERING_DETECT_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_crowd_gathering_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_CrowdGatheringAlarmInfo_S pOut = (pNET_CrowdGatheringAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    // 添加日志1：检查 execute_get_result 返回
    int nExecResult = execute_get_result(AC_GET_CROWD_GATHERING_DETECT_INFO, "{}", outJson);
    dlog_info("DEBUG: execute_get_result={%d}, outJson.size()={%d}", nExecResult, outJson.size());

    if (nExecResult != 0 || outJson.empty())
    {
        dlog_error("DEBUG: execute_get_result failed, outJson={%s}", outJson.c_str());
        return NET_E_GET_CFG_FAILED;
    }

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::CrowdGathering_S stCfg;
    strJson = normalize_data_json(outJson);
    // 添加日志2：检查 normalize 后的 JSON
    dlog_info("DEBUG: normalize后的JSON={%s}", strJson.c_str());

    Convert::to_struct(strJson, stCfg);
    dlog_info("DEBUG: stCfg address=%p, bEnable=%d, aRule.size()=%zu",
      (void*)&stCfg, stCfg.bEnable, stCfg.aRule.size());

    // 添加日志3：检查转换后的结构体
    dlog_info("DEBUG: stCfg.bEnable={%d}, stCfg.aRule.size()={%d}", stCfg.bEnable, stCfg.aRule.size());

    // 添加日志4：检查每个规则的详细内容
    for (size_t i = 0; i < stCfg.aRule.size(); ++i) {
        dlog_info("DEBUG: Rule[{%d}]: ObjectOccup={%d}, Region.PointNum={%d}",
            i, stCfg.aRule[i].nObjectOccup, stCfg.aRule[i].stRegion.nPointNum);
    }

    TvSdkConvert::FillCrowdGatheringAlarmInfo(stCfg, *pOut);
    dlog_info("DEBUG: pOut address=%p, dwRuleCount=%u", (void*)pOut, pOut->uRuleCount);
    dlog_info("DEBUG: pOut->uRuleCount={%d}", pOut->uRuleCount);

    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_crowd_gathering_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_CrowdGatheringAlarmInfo_S *pIn = (const NET_CrowdGatheringAlarmInfo_S *)lpInBuffer;

    Alarm::CrowdGathering_S stCfg;
    TvSdkConvert::ToCrowdGathering(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CROWD_GATHERING_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}


#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
/* ---------- Get/SetGarbageExposureCfg：AC_GET/SET_GARBAGE_EXPOSURE_DETECT_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_garbage_exposure_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_GarbageExposureCfg_S pOut = (pNET_GarbageExposureCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_GARBAGE_EXPOSURE_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;


    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::GarbageExposureDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillGarbageExposureCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_garbage_exposure_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_GarbageExposureCfg_S *pIn = (const NET_GarbageExposureCfg_S *)lpInBuffer;

    Alarm::GarbageExposureDetection_S stCfg;
    TvSdkConvert::ToGarbageExposure(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_GARBAGE_EXPOSURE_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
/* ---------- Get/SetGarbageOverflowCfg：AC_GET/SET_GARBAGE_OVERFLOW_DETECT_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_garbage_overflow_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_GarbageOverflowCfg_S pOut = (pNET_GarbageOverflowCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_GARBAGE_OVERFLOW_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::GarbageOverflowDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillGarbageOverflowCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_garbage_overflow_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_GarbageOverflowCfg_S *pIn = (const NET_GarbageOverflowCfg_S *)lpInBuffer;

    Alarm::GarbageOverflowDetection_S stCfg;
    TvSdkConvert::ToGarbageOverflow(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_GARBAGE_OVERFLOW_CFG, stInfo) : -1;
   return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
#endif

#ifdef SCENE_INTELLIGENCE
static NET_COMMON_ECODE_E cb_get_manhole_cover_abnormal_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_ManholeCoverAbnormalCfg_S pOut = (pNET_ManholeCoverAbnormalCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_MANHOLE_COVER_ABNORMAL_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::ManholeCoverAbnormalDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillManholeCoverAbnormalCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_manhole_cover_abnormal_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_ManholeCoverAbnormalCfg_S *pIn = (const NET_ManholeCoverAbnormalCfg_S *)lpInBuffer;

    Alarm::ManholeCoverAbnormalDetection_S stCfg;
    TvSdkConvert::ToManholeCoverAbnormal(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_MANHOLE_COVER_ABNORMAL_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_sleep_on_duty_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_SleepOnDutyCfg_S pOut = (pNET_SleepOnDutyCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SLEEP_ON_DUTY_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::SleepOnDutyDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSleepOnDutyCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_sleep_on_duty_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_SleepOnDutyCfg_S *pIn = (const NET_SleepOnDutyCfg_S *)lpInBuffer;

    Alarm::SleepOnDutyDetection_S stCfg;
    TvSdkConvert::ToSleepOnDuty(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SLEEP_ON_DUTY_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_electric_vehicle_in_elevator_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_ElectricVehicleInElevatorCfg_S pOut = (pNET_ElectricVehicleInElevatorCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::ElectricScooterDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillElectricVehicleInElevatorCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_electric_vehicle_in_elevator_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_ElectricVehicleInElevatorCfg_S *pIn = (const NET_ElectricVehicleInElevatorCfg_S *)lpInBuffer;

    Alarm::ElectricScooterDetection_S stCfg;
    TvSdkConvert::ToElectricVehicleInElevator(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_person_fall_down_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_PersonFallDownCfg_S pOut = (pNET_PersonFallDownCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PERSON_FALL_DOWN_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::PersonFallDownDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPersonFallDownCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_person_fall_down_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_PersonFallDownCfg_S *pIn = (const NET_PersonFallDownCfg_S *)lpInBuffer;

    Alarm::PersonFallDownDetection_S stCfg;
    TvSdkConvert::ToPersonFallDown(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PERSON_FALL_DOWN_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_construction_occupy_road_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_ConstructionOccupyRoadCfg_S pOut = (pNET_ConstructionOccupyRoadCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_CONSTRUCTION_OCCUPY_ROAD_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::ConstructionEncroachmentRoadDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillConstructionOccupyRoadCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_construction_occupy_road_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_ConstructionOccupyRoadCfg_S *pIn = (const NET_ConstructionOccupyRoadCfg_S *)lpInBuffer;

    Alarm::ConstructionEncroachmentRoadDetection_S stCfg;
    TvSdkConvert::ToConstructionOccupyRoad(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CONSTRUCTION_OCCUPY_ROAD_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_congestion_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_CongestionCfg_S pOut = (pNET_CongestionCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_CONGESTION_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::CongestionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillCongestionCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_congestion_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_CongestionCfg_S *pIn = (const NET_CongestionCfg_S *)lpInBuffer;

    Alarm::CongestionDetection_S stCfg;
    TvSdkConvert::ToCongestion(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CONGESTION_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_license_plate_recognition_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_LicensePlateRecognitionCfg_S pOut = (pNET_LicensePlateRecognitionCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_LICENSE_PLATE_RECOGNITION_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::LicensePlateCognitionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillLicensePlateRecognitionCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_license_plate_recognition_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_LicensePlateRecognitionCfg_S *pIn = (const NET_LicensePlateRecognitionCfg_S *)lpInBuffer;

    Alarm::LicensePlateCognitionDetection_S stCfg;
    TvSdkConvert::ToLicensePlateRecognition(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_LICENSE_PLATE_RECOGNITION_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_high_altitude_seatbelt_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_HighAltitudeSeatbeltCfg_S pOut = (pNET_HighAltitudeSeatbeltCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_HIGH_ALTITUDE_SEATBELT_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::HighAltitudeSeatbeltDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillHighAltitudeSeatbeltCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_high_altitude_seatbelt_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_HighAltitudeSeatbeltCfg_S *pIn = (const NET_HighAltitudeSeatbeltCfg_S *)lpInBuffer;

    Alarm::HighAltitudeSeatbeltDetection_S stCfg;
    TvSdkConvert::ToHighAltitudeSeatbelt(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_HIGH_ALTITUDE_SEATBELT_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_safety_helmet_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_SafetyHelmetCfg_S pOut = (pNET_SafetyHelmetCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SAFETY_HELMET_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::SafetyHelmetDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSafetyHelmetCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_safety_helmet_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_SafetyHelmetCfg_S *pIn = (const NET_SafetyHelmetCfg_S *)lpInBuffer;

    Alarm::SafetyHelmetDection_S stCfg;
    TvSdkConvert::ToSafetyHelmet(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SAFETY_HELMET_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_person_fall_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_PersonFallCfg_S pOut = (pNET_PersonFallCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PERSON_FALL_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::TripDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPersonFallCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_person_fall_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_PersonFallCfg_S *pIn = (const NET_PersonFallCfg_S *)lpInBuffer;

    Alarm::TripDetection_S stCfg;
    TvSdkConvert::ToPersonFall(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PERSON_FALL_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_phone_usage_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_PhoneUsageCfg_S pOut = (pNET_PhoneUsageCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PHONE_USAGE_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::PhoneUsageDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPhoneUsageCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_phone_usage_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_PhoneUsageCfg_S *pIn = (const NET_PhoneUsageCfg_S *)lpInBuffer;

    Alarm::PhoneUsageDetection_S stCfg;
    TvSdkConvert::ToPhoneUsage(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PHONE_USAGE_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_smoking_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_SmokingCfg_S pOut = (pNET_SmokingCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SMOKING_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::SmokingDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSmokingCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_smoking_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_SmokingCfg_S *pIn = (const NET_SmokingCfg_S *)lpInBuffer;

    Alarm::SmokingDection_S stCfg;
    TvSdkConvert::ToSmoking(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SMOKING_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_open_flame_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_OpenFlameCfg_S pOut = (pNET_OpenFlameCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_OPEN_FLAME_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::OpenFlameDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillOpenFlameCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_open_flame_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_OpenFlameCfg_S *pIn = (const NET_OpenFlameCfg_S *)lpInBuffer;

    Alarm::OpenFlameDetection_S stCfg;
    TvSdkConvert::ToOpenFlame(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_OPEN_FLAME_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_bare_soil_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_BareSoilCfg_S pOut = (pNET_BareSoilCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_BARE_SOIL_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::BareSoiletDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillBareSoilCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_bare_soil_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_BareSoilCfg_S *pIn = (const NET_BareSoilCfg_S *)lpInBuffer;

    Alarm::BareSoiletDection_S stCfg;
    TvSdkConvert::ToBareSoil(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_BARE_SOIL_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_hole_protection_bar_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_HoleProtectionBarCfg_S pOut = (pNET_HoleProtectionBarCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_HOLE_PROTECTION_BAR_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::HoleProtectionBarDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillHoleProtectionBarCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_hole_protection_bar_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_HoleProtectionBarCfg_S *pIn = (const NET_HoleProtectionBarCfg_S *)lpInBuffer;

    Alarm::HoleProtectionBarDection_S stCfg;
    TvSdkConvert::ToHoleProtectionBar(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_HOLE_PROTECTION_BAR_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_reflective_clothing_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_ReflectiveClothingCfg_S pOut = (pNET_ReflectiveClothingCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_REFLECTIVE_CLOTHING_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::ReflectiveClothingDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillReflectiveClothingCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_reflective_clothing_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_ReflectiveClothingCfg_S *pIn = (const NET_ReflectiveClothingCfg_S *)lpInBuffer;

    Alarm::ReflectiveClothingDection_S stCfg;
    TvSdkConvert::ToReflectiveClothing(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_REFLECTIVE_CLOTHING_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
#endif

static NET_COMMON_ECODE_E cb_get_pet_recognition_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_PetRecognitionInfo_S pOut = (pNET_PetRecognitionInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PET_RECOGNITION_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::PetRecognition_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPetRecognitionInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_pet_recognition_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_PetRecognitionInfo_S *pIn = (const NET_PetRecognitionInfo_S *)lpInBuffer;

    Alarm::PetRecognition_S stCfg;
    TvSdkConvert::ToPetRecognition(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PET_RECOGNITION_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

#ifdef SCENE_INTELLIGENCE
static NET_COMMON_ECODE_E cb_get_climb_fence_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_ClimbFenceInfo_S pOut = (pNET_ClimbFenceInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_CLIMB_FENCE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::FenceClimbingDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillClimbFenceInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_climb_fence_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_ClimbFenceInfo_S *pIn = (const NET_ClimbFenceInfo_S *)lpInBuffer;

    Alarm::FenceClimbingDetection_S stCfg;
    TvSdkConvert::ToClimbFence(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CLIMB_FENCE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_dimission_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_DimissionInfo_S pOut = (pNET_DimissionInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_DIMISSION_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::LeavePostDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillDimissionInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_dimission_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_DimissionInfo_S *pIn = (const NET_DimissionInfo_S *)lpInBuffer;

    Alarm::LeavePostDetection_S stCfg;
    TvSdkConvert::ToDimission(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_DIMISSION_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_illegal_lane_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_IllegalLaneInfo_S pOut = (pNET_IllegalLaneInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_ILLEGAL_LANE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::IllegalLaneChangeDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillIllegalLaneInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_illegal_lane_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_IllegalLaneInfo_S *pIn = (const NET_IllegalLaneInfo_S *)lpInBuffer;

    Alarm::IllegalLaneChangeDetection_S stCfg;
    TvSdkConvert::ToIllegalLane(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_ILLEGAL_LANE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_retrograde_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_RetrogradeInfo_S pOut = (pNET_RetrogradeInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_RETROGRADE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::DrivingAgainstTrafficDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillRetrogradeInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_retrograde_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_RetrogradeInfo_S *pIn = (const NET_RetrogradeInfo_S *)lpInBuffer;

    Alarm::DrivingAgainstTrafficDetection_S stCfg;
    TvSdkConvert::ToRetrograde(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_RETROGRADE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_nonmotor_vehicle_intrusion_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_NonmotorVehicleIntrusionInfo_S pOut = (pNET_NonmotorVehicleIntrusionInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_NONMOROT_VEHIINTRU_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::NonMotorVehicleIntrusionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillNonmotorVehicleIntrusionInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_nonmotor_vehicle_intrusion_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_NonmotorVehicleIntrusionInfo_S *pIn = (const NET_NonmotorVehicleIntrusionInfo_S *)lpInBuffer;

    Alarm::NonMotorVehicleIntrusionDetection_S stCfg;
    TvSdkConvert::ToNonmotorVehicleIntrusion(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_NONMOROT_VEHIINTRU_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_occupation_emergency_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_OccupationEmergencyInfo_S pOut = (pNET_OccupationEmergencyInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_OCCUPATION_EMERGENCY_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::EmergencyLaneOccupancyDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillOccupationEmergencyInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_occupation_emergency_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_OccupationEmergencyInfo_S *pIn = (const NET_OccupationEmergencyInfo_S *)lpInBuffer;

    Alarm::EmergencyLaneOccupancyDetection_S stCfg;
    TvSdkConvert::ToOccupationEmergency(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_OCCUPATION_EMERGENCY_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_pedestrian_intrusion_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_PedestrianIntrusionInfo_S pOut = (pNET_PedestrianIntrusionInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PEDESTRAN_INTRUSION_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::PedestrianIntrusionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPedestrianIntrusionInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_pedestrian_intrusion_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_PedestrianIntrusionInfo_S *pIn = (const NET_PedestrianIntrusionInfo_S *)lpInBuffer;

    Alarm::PedestrianIntrusionDetection_S stCfg;
    TvSdkConvert::ToPedestrianIntrusion(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PEDESTRAN_INTRUSION_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_smoke_fire_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_SmokeFireCfg_S pOut = (pNET_SmokeFireCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SMOKE_FIRE_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::SmokeFireDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSmokeFireCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_smoke_fire_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_SmokeFireCfg_S *pIn = (const NET_SmokeFireCfg_S *)lpInBuffer;

    Alarm::SmokeFireDetection_S stCfg;
    TvSdkConvert::ToSmokeFire(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SMOKE_FIRE_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_road_ponding_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_RoadPondingCfg_S pOut = (pNET_RoadPondingCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_ROAD_PONDING_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::RoadPondingDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillRoadPondingCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_road_ponding_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_RoadPondingCfg_S *pIn = (const NET_RoadPondingCfg_S *)lpInBuffer;

    Alarm::RoadPondingDetection_S stCfg;
    TvSdkConvert::ToRoadPonding(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_ROAD_PONDING_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
#endif

#if CAP_AI_PEOPLE_STATISTICS
/* ---------- Get/SetPeopleFlowStatisticsCfg：AC_GET/SET_PEOPLE_FLOW_STATISTICS_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_people_flow_statistics_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_PeopleFlowStatisticsCfg_S pOut = (pNET_PeopleFlowStatisticsCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PEOPLE_FLOW_STATISTICS_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::PeopleFlowStatistics_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPeopleFlowStatisticsCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_people_flow_statistics_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_PeopleFlowStatisticsCfg_S *pIn = (const NET_PeopleFlowStatisticsCfg_S *)lpInBuffer;

    Alarm::PeopleFlowStatistics_S stCfg;
    TvSdkConvert::ToPeopleFlowStatistics(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PEOPLE_FLOW_STATISTICS_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_reset_people_flow_statistics(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    (void)lpInBuffer;
    Task::Info_S stInfo;
    stInfo.data = "{}";
    int nExec = s_taskManage ? s_taskManage->execute(AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* ---------- Get/SetPeopleDensityDetectionCfg：AC_GET/SET_PEOPLE_DENSITY_DETECTION_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_people_density_detection_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_PeopleDensityDetectionCfg_S pOut = (pNET_PeopleDensityDetectionCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PEOPLE_DENSITY_DETECTION_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::PeopleDensityDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPeopleDensityDetectionCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_people_density_detection_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_PeopleDensityDetectionCfg_S *pIn = (const NET_PeopleDensityDetectionCfg_S *)lpInBuffer;

    Alarm::PeopleDensityDetection_S stCfg;
    TvSdkConvert::ToPeopleDensityDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PEOPLE_DENSITY_DETECTION_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

#endif

/* ---------- Get/SetParkingDetectAlarm：AC_GET/SET_PARKING_DETECT_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_parking_detect_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_ParkingAlarmInfo_S pOut = (pNET_ParkingAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PARKING_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::ParkingDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillParkingDetectAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_parking_detect_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_ParkingAlarmInfo_S *pIn = (const NET_ParkingAlarmInfo_S *)lpInBuffer;

    Alarm::ParkingDetection_S stCfg;
    TvSdkConvert::ToParkingDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PARKING_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* ---------- Get/SetUnattendedObjectAlarm：AC_GET/SET_UNATTENDED_OBJECT_DETECT_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_unattended_object_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_UnattendedObjectAlarmInfo_S pOut = (pNET_UnattendedObjectAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_UNATTENDED_OBJECT_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::UnattendedObject_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillUnattendedObjectAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_unattended_object_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_UnattendedObjectAlarmInfo_S *pIn = (const NET_UnattendedObjectAlarmInfo_S *)lpInBuffer;

    Alarm::UnattendedObject_S stCfg;
    TvSdkConvert::ToUnattendedObject(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_UNATTENDED_OBJECT_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* ---------- Get/SetObjectRemovalAlarm：AC_GET/SET_OBJECT_REMOVAL_DETECT_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_object_removal_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_ObjectRemovalAlarmInfo_S pOut = (pNET_ObjectRemovalAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_OBJECT_REMOVAL_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::ObjectRemoval_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillObjectRemovalAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_object_removal_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_ObjectRemovalAlarmInfo_S *pIn = (const NET_ObjectRemovalAlarmInfo_S *)lpInBuffer;

    Alarm::ObjectRemoval_S stCfg;
    TvSdkConvert::ToObjectRemoval(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_OBJECT_REMOVAL_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* ---------- Get/SetAudioAnomalyAlarm：AC_GET/SET_AUDIO_ANOMALY_DETECT_INFO ---------- */
static NET_COMMON_ECODE_E cb_get_audio_anomaly_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_AudioAnomalyAlarmInfo_S pOut = (pNET_AudioAnomalyAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_AUDIO_ANOMALY_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::AudioAnomaly_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);

    std::memset(&g_tvAudioAnomalyAlarmInfo, 0, sizeof(g_tvAudioAnomalyAlarmInfo));
    TvSdkConvert::FillAudioAnomalyAlarmInfo(stCfg, g_tvAudioAnomalyAlarmInfo);
    std::memcpy(pOut, &g_tvAudioAnomalyAlarmInfo, sizeof(g_tvAudioAnomalyAlarmInfo));
    return NET_E_SUCCEED;
}

/**
 * @brief 获取音频异常侦测实时音量。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，当前 IPC 仅使用单通道。
 * @param [out] pOutBuffer 指向 NET_AudioAnomalyCurrentDb_S 的输出缓冲区。
 * @return 获取成功返回 NET_E_SUCCEED；参数非法或获取失败时返回对应错误码。
 */
static NET_COMMON_ECODE_E cb_get_audio_anomaly_current_db(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_AudioAnomalyCurrentDb_S pCurrentDbInfo =
        static_cast<pNET_AudioAnomalyCurrentDb_S>(pOutBuffer);
    std::memset(pCurrentDbInfo, 0, sizeof(*pCurrentDbInfo));

    std::string strResultJson;
    if ((execute_get_result(AC_GET_AUDIO_ANOMALY_DETECT_CURRENT_DB, wrap_data_json("{}"), strResultJson) != 0) ||
        strResultJson.empty())
    {
        return NET_E_GET_CFG_FAILED;
    }

    int nResult = -1;
    Json::get(strResultJson.c_str(), "Return", nResult);
    if (nResult != 0)
    {
        return NET_E_GET_CFG_FAILED;
    }

    const std::string strCurrentDbJson = normalize_data_json(strResultJson);
    Json::Object *pCurrentDbJson = Json::init(strCurrentDbJson.c_str());
    if (!pCurrentDbJson)
    {
        return NET_E_GET_CFG_FAILED;
    }

    const bool bParsed = Json::get(pCurrentDbJson, "CurrentDb", pCurrentDbInfo->fCurrentDb);
    Json::deinit(pCurrentDbJson);
    if (!bParsed)
    {
        return NET_E_GET_CFG_FAILED;
    }

    pCurrentDbInfo->bValid = TRUE;
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_audio_anomaly_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_AudioAnomalyAlarmInfo_S *pIn = (const NET_AudioAnomalyAlarmInfo_S *)lpInBuffer;
    Alarm::AudioAnomaly_S stCfg;
    TvSdkConvert::ToAudioAnomaly(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_AUDIO_ANOMALY_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/**
 * @brief 执行 IPC 获取命令并提取告警配置数据。
 * @author ITC
 * @param [in] nActionCode IPC 获取命令号。
 * @param [out] strDataJson 获取成功后返回的 Data JSON 字符串。
 * @return 成功返回 NET_E_SUCCEED，否则返回获取配置失败码。
 */
static NET_COMMON_ECODE_E get_alarm_config_data(INT32 nActionCode, std::string& strDataJson)
{
    std::string strResultJson;
    if (execute_get_result(nActionCode, "{}", strResultJson) != 0 || strResultJson.empty())
    {
        return NET_E_GET_CFG_FAILED;
    }

    INT32 nTaskResult = ERR;
    Json::get(strResultJson.c_str(), "Return", nTaskResult);
    if (nTaskResult != OK)
    {
        return NET_E_GET_CFG_FAILED;
    }

    strDataJson = normalize_data_json(strResultJson);
    return strDataJson.empty() ? NET_E_GET_CFG_FAILED : NET_E_SUCCEED;
}

/**
 * @brief 执行 IPC 设置命令以保存告警配置数据。
 * @author ITC
 * @param [in] nActionCode IPC 设置命令号。
 * @param [in] strDataJson 待设置的 Data JSON 字符串。
 * @return 成功返回 NET_E_SUCCEED，否则返回设置配置失败码。
 */
static NET_COMMON_ECODE_E set_alarm_config_data(INT32 nActionCode, const std::string& strDataJson)
{
    if (!s_taskManage || strDataJson.empty())
    {
        return NET_E_SET_CFG_FAILED;
    }

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(strDataJson);
    const INT32 nExecuteResult = s_taskManage->execute(nActionCode, stInfo);
    return nExecuteResult == OK ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/**
 * @brief 获取 IPC 的声音告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与查询。
 * @param [out] pOutBuffer 用于接收 NET_AudibleAlarmInfo_S 配置的缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_get_audible_alarm_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_E_NULL_POINT;
    }

    pNET_AudibleAlarmInfo_S pOutput = static_cast<pNET_AudibleAlarmInfo_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_AUDIBLE_ALARM_INFO, strDataJson);
    if (enResult != NET_E_SUCCEED)
    {
        return enResult;
    }

    Alarm::SoundOutputAlarm_S stAlarmInfo;
    Convert::to_struct(strDataJson, stAlarmInfo);
    TvSdkConvert::FillAudibleAlarmInfo(stAlarmInfo, *pOutput);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的声音告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_AudibleAlarmInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_set_audible_alarm_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_E_NULL_POINT;
    }

    const pNET_AudibleAlarmInfo_S pInput = static_cast<pNET_AudibleAlarmInfo_S>(pInBuffer);
    if (!is_valid_audible_alarm_info(*pInput))
    {
        return NET_E_INVALID_PARAM;
    }

    Alarm::SoundOutputAlarm_S stAlarmInfo;
    TvSdkConvert::ToAudibleAlarm(*pInput, stAlarmInfo);
    std::string strDataJson = Convert::to_string(stAlarmInfo);
    return set_alarm_config_data(AC_SET_AUDIBLE_ALARM_INFO, strDataJson);
}

/**
 * @brief 获取 IPC 的全部报警输入配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与查询。
 * @param [out] pOutBuffer 用于接收 NET_AlarmInputInfoList_S 配置的缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_get_alarm_input_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_E_NULL_POINT;
    }

    pNET_AlarmInputInfoList_S pOutput = static_cast<pNET_AlarmInputInfoList_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_ALARM_INPUT_INFO, strDataJson);
    if (enResult != NET_E_SUCCEED)
    {
        return enResult;
    }

    std::set<Alarm::IoInputInfo_S> stAlarmInputs;
    Convert::to_struct(strDataJson, stAlarmInputs);
    TvSdkConvert::FillAlarmInputInfoList(stAlarmInputs, *pOutput);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的一路报警输入配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_AlarmInputInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_set_alarm_input_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_E_NULL_POINT;
    }

    const pNET_AlarmInputInfo_S pInput = static_cast<pNET_AlarmInputInfo_S>(pInBuffer);
    if (!is_valid_alarm_input_info(*pInput))
    {
        return NET_E_INVALID_PARAM;
    }

    Alarm::IoInputInfo_S stAlarmInput;
    TvSdkConvert::ToAlarmInputInfo(*pInput, stAlarmInput);
    std::string strDataJson = Convert::to_string(stAlarmInput);
    return set_alarm_config_data(AC_SET_ALARM_INPUT_INFO, strDataJson);
}

/**
 * @brief 获取 IPC 的全部报警输出配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与查询。
 * @param [out] pOutBuffer 用于接收 NET_AlarmOutputInfoList_S 配置的缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_get_alarm_output_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_E_NULL_POINT;
    }

    pNET_AlarmOutputInfoList_S pOutput = static_cast<pNET_AlarmOutputInfoList_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_ALARM_OUTPUT_INFO, strDataJson);
    if (enResult != NET_E_SUCCEED)
    {
        return enResult;
    }

    std::set<Alarm::IoOutputInfo_S> stAlarmOutputs;
    Convert::to_struct(strDataJson, stAlarmOutputs);
    TvSdkConvert::FillAlarmOutputInfoList(stAlarmOutputs, *pOutput);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的一路报警输出配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_AlarmOutputInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_set_alarm_output_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_E_NULL_POINT;
    }

    const pNET_AlarmOutputInfo_S pInput = static_cast<pNET_AlarmOutputInfo_S>(pInBuffer);
    if (!is_valid_alarm_output_info(*pInput))
    {
        return NET_E_INVALID_PARAM;
    }

    Alarm::IoOutputInfo_S stAlarmOutput;
    TvSdkConvert::ToAlarmOutputInfo(*pInput, stAlarmOutput);
    std::string strDataJson = Convert::to_string(stAlarmOutput);
    return set_alarm_config_data(AC_SET_ALARM_OUTPUT_INFO, strDataJson);
}

/**
 * @brief 获取 IPC 的闪光灯告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与查询。
 * @param [out] pOutBuffer 用于接收 NET_FlashingLightAlarmInfo_S 配置的缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_get_flashing_light_alarm_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_E_NULL_POINT;
    }

    pNET_FlashingLightAlarmInfo_S pOutput = static_cast<pNET_FlashingLightAlarmInfo_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_FLASHING_LIGHT_ALARM_INFO, strDataJson);
    if (enResult != NET_E_SUCCEED)
    {
        return enResult;
    }

    Alarm::FlashInfo_S stAlarmInfo;
    Convert::to_struct(strDataJson, stAlarmInfo);
    TvSdkConvert::FillFlashingLightAlarmInfo(stAlarmInfo, *pOutput);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的闪光灯告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_FlashingLightAlarmInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_set_flashing_light_alarm_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_E_NULL_POINT;
    }

    const pNET_FlashingLightAlarmInfo_S pInput = static_cast<pNET_FlashingLightAlarmInfo_S>(pInBuffer);
    if (!is_valid_flashing_light_alarm_info(*pInput))
    {
        return NET_E_INVALID_PARAM;
    }

    Alarm::FlashInfo_S stAlarmInfo;
    TvSdkConvert::ToFlashingLightAlarm(*pInput, stAlarmInfo);
    std::string strDataJson = Convert::to_string(stAlarmInfo);
    return set_alarm_config_data(AC_SET_FLASHING_LIGHT_ALARM_INFO, strDataJson);
}

/**
 * @brief 获取 IPC 的 PIR 告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与查询。
 * @param [out] pOutBuffer 用于接收 NET_PirAlarmInfo_S 配置的缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_get_pir_alarm_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_E_NULL_POINT;
    }

    pNET_PirAlarmInfo_S pOutput = static_cast<pNET_PirAlarmInfo_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_PIR_ALARM_INFO, strDataJson);
    if (enResult != NET_E_SUCCEED)
    {
        return enResult;
    }

    Alarm::PirAlarmInfo_S stAlarmInfo;
    Convert::to_struct(strDataJson, stAlarmInfo);
    TvSdkConvert::FillPirAlarmInfo(stAlarmInfo, *pOutput);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的 PIR 告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_PirAlarmInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_E_SUCCEED，否则返回相应错误码。
 */
static NET_COMMON_ECODE_E cb_set_pir_alarm_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_E_NULL_POINT;
    }

    const pNET_PirAlarmInfo_S pInput = static_cast<pNET_PirAlarmInfo_S>(pInBuffer);
    if (!is_valid_pir_alarm_info(*pInput))
    {
        return NET_E_INVALID_PARAM;
    }

    Alarm::PirAlarmInfo_S stAlarmInfo;
    TvSdkConvert::ToPirAlarmInfo(*pInput, stAlarmInfo);
    std::string strDataJson = Convert::to_string(stAlarmInfo);
    return set_alarm_config_data(AC_SET_PIR_ALARM_INFO, strDataJson);
}

/* ---------- GetRtspUrl：获取主/子码流 RTSP URL ---------- */
static NET_COMMON_ECODE_E cb_get_rtsp_url(INT32 dwChannelID, pNET_RtspUrlInfo_S pInfo)
{
    if (!pInfo)
        return NET_E_INVALID_PARAM;

    const int streamIndex = pInfo->uStreamIndex; // 调用方指定需要的码流
    std::memset(pInfo, 0, sizeof(*pInfo));
    pInfo->uChannel = dwChannelID;
    pInfo->uStreamIndex = streamIndex;

    int rtspChn = -1;
    switch (streamIndex)
    {
    case NET_LIVE_STREAM_INDEX_MAIN: rtspChn = RTSP_CHN_MAIN; break;
    case NET_LIVE_STREAM_INDEX_AUX:  rtspChn = RTSP_CHN_SUB;  break;
    default:
        dlog_error("GetRtspUrl不支持的码流索引 channel:%d stream:%d", dwChannelID, streamIndex);
        return NET_E_INVALID_PARAM;
    }

    dlog_info("GetRtspUrl请求 channel:%d stream:%d 映射rtspChn:%d",
              dwChannelID,
              streamIndex,
              rtspChn);

    const char *pUrl = CRtspServer::instance()->getRtspUrl(rtspChn, false);
    if (!pUrl || pUrl[0] == '\0')
    {
        dlog_error("GetRtspUrl获取失败 channel:%d stream:%d rtspChn:%d",
                   dwChannelID,
                   streamIndex,
                   rtspChn);
        return NET_E_GET_CFG_FAILED;
    }

    std::strncpy(pInfo->szRtspUrl, pUrl, sizeof(pInfo->szRtspUrl) - 1);
    pInfo->szRtspUrl[sizeof(pInfo->szRtspUrl) - 1] = '\0';
    return NET_E_SUCCEED;
}

/* --------------------------- 获取升级状态 --------------------------- */

static NET_COMMON_ECODE_E cb_get_upgrade_status(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_UpgradeStatus_S pOut = (pNET_UpgradeStatus_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_UPGRADE_STATUS, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    ::System::UpgradeStatus_S stCfg;
    const std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillUpgradeStatus(stCfg, *pOut);
    return NET_E_SUCCEED;
}

/* --------------------------- 设置升级包路径后进行升级 --------------------------- */

static NET_COMMON_ECODE_E cb_set_upgrade(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    if (!s_taskManage)
        return NET_E_SET_CFG_FAILED;

    const NET_UpgradeInfo_S *pIn = (const NET_UpgradeInfo_S *)lpInBuffer;
    ::System::UpgradeInfo_S stCfg;
    TvSdkConvert::ToUpgradeInfo(*pIn, stCfg);
    const std::string reqPath = stCfg.strUpgradePath;
    stCfg.strUpgradePath = normalize_upgrade_local_path(stCfg.strUpgradePath);
    if (reqPath.empty())
        dlog_info("[TVSDK][Upgrade] request path empty, use default dir=%s", kDefaultUpgradeDir);
    if (!is_valid_upgrade_path(stCfg.strUpgradePath))
    {
        dlog_warn("[TVSDK][Upgrade] invalid path");
        return NET_E_INVALID_PARAM;
    }

    std::string wrappedJson = wrap_data_json(Convert::to_string(stCfg));
    dlog_info("[TVSDK][Upgrade] recv local-path=%s", stCfg.strUpgradePath.c_str());

    if (execute_action_expect_success(AC_SET_UPGRADE, wrappedJson, nullptr) != 0)
        return NET_E_SET_CFG_FAILED;

    std::string checkOutJson;
    if (execute_action_expect_success(AC_CHECK_UPGRADE, "{}", &checkOutJson) != 0)
        return NET_E_SET_CFG_FAILED;

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));

    int nExec = s_taskManage->execute(AC_DO_UPGRADE, stInfo);
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* --------------------------- 获取升级包版本 --------------------------- */

static NET_COMMON_ECODE_E cb_get_upgrade_version(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_UpgradeVersion_S pOut = (pNET_UpgradeVersion_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_CHECK_UPGRADE, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    ::System::UpgradeVersion_S stCfg;
    const std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillUpgradeVersion(stCfg, *pOut);
    return NET_E_SUCCEED;
}

/* --------------------------- 获取抓图计划信息 --------------------------- */

static NET_COMMON_ECODE_E cb_get_capture_plan_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_CapturePlanInfo_S pOut = (pNET_CapturePlanInfo_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_CAPTURE_PLAN_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Capture_NS::CapturePlan_S stCfg;
    const std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillCapturePlan(stCfg, *pOut);
    return NET_E_SUCCEED;
}

/* --------------------------- 设置抓图计划信息 --------------------------- */

static NET_COMMON_ECODE_E cb_set_capture_plan_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_CapturePlanInfo_S *pIn = (const NET_CapturePlanInfo_S *)lpInBuffer;
    Capture_NS::CapturePlan_S stCfg;
    TvSdkConvert::ToCapturePlan(*pIn, stCfg);

    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CAPTURE_PLAN_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* --------------------------- 获取抓图参数信息 --------------------------- */

static NET_COMMON_ECODE_E cb_get_capture_param_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_CaptureParamInfo_S pOut = (pNET_CaptureParamInfo_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_CAPTURE_PARAM_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Capture_NS::CaptureParam_S stCfg;
    const std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillCaptureParam(stCfg, *pOut);
    return NET_E_SUCCEED;
}

/* --------------------------- 设置抓图参数信息 --------------------------- */

static NET_COMMON_ECODE_E cb_set_capture_param_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_CaptureParamInfo_S *pIn = (const NET_CaptureParamInfo_S *)lpInBuffer;
    if (!is_valid_capture_config(pIn->stCaptureTimingConfig) ||
        !is_valid_capture_config(pIn->stCaptureEventConfig))
    {
        return NET_E_INVALID_PARAM;
    }
    Capture_NS::CaptureParam_S stCfg;
    TvSdkConvert::ToCaptureParam(*pIn, stCfg);

    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CAPTURE_PARAM_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* --------------------------- 设置抓图参数信息 --------------------------- */

typedef struct _IspPayload_S_
{
    bool bAllScene;
    ISP::AllSceneParams_S stAllScene;
    ISP::SceneParams_S stScene;
} IspPayload_S;

/* --------------------------- ISP相关信息 --------------------------- */

static NET_COMMON_ECODE_E cb_get_exposure_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_ExposureInfo_S pOut = (pNET_ExposureInfo_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_EXPOSURE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_E_GET_CFG_FAILED;
    ISP::ExposureAttr_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillExposureInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_exposure_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    
    const NET_ExposureInfo_S *pIn = (const NET_ExposureInfo_S *)lpInBuffer;
    ISP::ExposureAttr_S stCfg;
    TvSdkConvert::ToExposureAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_EXPOSURE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_daynight_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_DayNightInfo_S pOut = (pNET_DayNightInfo_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_DAY_NIGHT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_E_GET_CFG_FAILED;
    ISP::DayNightAttr_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillDayNightInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_daynight_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    
    const NET_DayNightInfo_S *pIn = (const NET_DayNightInfo_S *)lpInBuffer;
    if (!is_valid_daynight_config(*pIn))
    {
        return NET_E_INVALID_PARAM;
    }
    ISP::DayNightAttr_S stCfg;
    TvSdkConvert::ToDayNightAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_DAY_NIGHT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_backlight_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_BackLightInfo_S pOut = (pNET_BackLightInfo_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_BACK_LIGHT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_E_GET_CFG_FAILED;
    ISP::BackLightArrt_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillBackLightInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_backlight_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    
    const NET_BackLightInfo_S *pIn = (const NET_BackLightInfo_S *)lpInBuffer;
    if (!is_valid_backlight_config(*pIn))
    {
        return NET_E_INVALID_PARAM;
    }
    ISP::BackLightArrt_S stCfg;
    TvSdkConvert::ToBackLightAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_BACK_LIGHT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_denoise_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_DenoiseInfo_S pOut = (pNET_DenoiseInfo_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_NOISE_REMOVE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_E_GET_CFG_FAILED;
    ISP::DnrAttr_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillDenoiseInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_denoise_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    
    const NET_DenoiseInfo_S *pIn = (const NET_DenoiseInfo_S *)lpInBuffer;
    ISP::DnrAttr_S stCfg;
    TvSdkConvert::ToDnrAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_NOISE_REMOVE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_whitebalance_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_WhiteBalanceInfo_S pOut = (pNET_WhiteBalanceInfo_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_WHITE_BALANCE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_E_GET_CFG_FAILED;
    ISP::AwbAttr_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillWhiteBalanceInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_whitebalance_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    
    const NET_WhiteBalanceInfo_S *pIn = (const NET_WhiteBalanceInfo_S *)lpInBuffer;
    ISP::AwbAttr_S stCfg;
    TvSdkConvert::ToAwbAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_WHITE_BALANCE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_set_talkback_state(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_TalkbackStateInfo_S *pIn = (const NET_TalkbackStateInfo_S *)lpInBuffer;
    Preview::IntercomInfo_S stCfg;
    TvSdkConvert::ToIntercomInfo(*pIn, stCfg);
    if (stCfg.bEnable && stCfg.strUrl.empty())
        stCfg.strSdp = "tvsdk_voicecom";
    if (!stCfg.bEnable && stCfg.strLocalIp.empty())
    {
        // TVSDK VoiceCom 关闭请求可能不携带 LocalIp，补当前对讲IP以通过 preview 的归属校验。
        stCfg.strLocalIp = CPreviewManage::instance()->get_intercom_ip();
    }

    Task::Info_S stInfo;
    stInfo.strIp = stCfg.strLocalIp;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_INTERCOM_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_set_talkback_to_stream(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_TalkbackStreamInfo_S *pIn = (const NET_TalkbackStreamInfo_S *)lpInBuffer;
    Replay::Stream::Info_S stCfg;
    TvSdkConvert::ToReplayStreamInfo(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_TO_STREAM_TALKBACK, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_talkback_from_stream(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_TalkbackStreamInfo_S pOut = (pNET_TalkbackStreamInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_FROM_STREAM_TALKBACK, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Replay::Stream::Info_S stCfg;
    strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillTalkbackStreamInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_replay_talkback(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_ReplayTalkbackInfo_S *pIn = (const NET_ReplayTalkbackInfo_S *)lpInBuffer;
    Replay::Stream::ReplayRtpInfo_S stCfg;
    TvSdkConvert::ToReplayRtpInfo(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_REPLAY_TALKBACK, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/*-----------------------------------获取/设置音频配置-------------------------------------*/
static NET_COMMON_ECODE_E cb_get_audio_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_AudioCfg_S pOut = (pNET_AudioCfg_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_AUDIO_CONFIG, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Audio_NS::AudioConfig_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillAudioCfg(stCfg, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_audio_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    const NET_AudioCfg_S *pIn = (const NET_AudioCfg_S *)lpInBuffer;
    Audio_NS::AudioConfig_S stCfg;
    TvSdkConvert::ToAudioConfig(*pIn, stCfg);

    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_AUDIO_CONFIG, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/*-----------------------------------获取/设置进入区域侦测-------------------------------------*/
static NET_COMMON_ECODE_E cb_get_enter_region_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_EnterRegionAlarmInfo_S pOut = (pNET_EnterRegionAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_ENTER_REGION_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::EntranceDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillEnterRegionAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_enter_region_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_EnterRegionAlarmInfo_S *pIn = (const NET_EnterRegionAlarmInfo_S *)lpInBuffer;
    Alarm::EntranceDetection_S stCfg;
    TvSdkConvert::ToEntranceDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_ENTER_REGION_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}
static NET_COMMON_ECODE_E cb_get_leave_region_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_LeaveRegionAlarmInfo_S pOut = (pNET_LeaveRegionAlarmInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_LEAVE_REGION_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::ExitingDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillLeaveRegionAlarmInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}
/*-----------------------------------获取/设置离开区域侦测-------------------------------------*/
static NET_COMMON_ECODE_E cb_set_leave_region_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_LeaveRegionAlarmInfo_S *pIn = (const NET_LeaveRegionAlarmInfo_S *)lpInBuffer;
    Alarm::ExitingDetection_S stCfg;
    TvSdkConvert::ToExitingDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_LEAVE_REGION_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_face_capture_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;
    pNET_FaceCaptureInfo_S pOut = (pNET_FaceCaptureInfo_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_FACE_CAPTURE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_E_GET_CFG_FAILED;

    Alarm::FaceCapture_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillFaceCaptureInfo(stCfg, *pOut);
    return NET_E_SUCCEED;
}
static NET_COMMON_ECODE_E cb_set_face_capture_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;
    const NET_FaceCaptureInfo_S *pIn = (const NET_FaceCaptureInfo_S *)lpInBuffer;
    Alarm::FaceCapture_S stCfg;
    TvSdkConvert::ToFaceCapture(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_FACE_CAPTURE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}

/* 人脸抓拍叠加配置仍使用原 IPC 任务码，仅将 SDK ABI 迁移为 NET_* 命名。 */
static NET_COMMON_ECODE_E cb_get_face_capture_overlay_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    std::string outJson;
    if (execute_get_result(AC_GET_FACE_CAPTURE_OVERLAY_INFO_INFO, "{}", outJson) != 0 || outJson.empty())
    {
        return NET_E_GET_CFG_FAILED;
    }

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
    {
        return NET_E_GET_CFG_FAILED;
    }

    Alarm::OverlayInfo_S stCfg;
    Convert::to_struct(normalize_data_json(outJson), stCfg);
    TvSdkConvert::FillFaceCaptureOverlayInfo(stCfg,
        *static_cast<pNET_FaceCaptureOverlayInfo_S>(lpOutBuffer));
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_face_capture_overlay_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    Alarm::OverlayInfo_S stCfg;
    TvSdkConvert::ToFaceCaptureOverlayInfo(
        *static_cast<const NET_FaceCaptureOverlayInfo_S *>(lpInBuffer), stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    const int nExec = s_taskManage ?
        s_taskManage->execute(AC_SET_FACE_CAPTURE_OVERLAY_INFO_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_E_SUCCEED : NET_E_SET_CFG_FAILED;
}


/* ---------- 安全服务与日志（465-472） ---------- */
static NET_COMMON_ECODE_E cb_get_security_services_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    std::string dataJson;
    if (!execute_get_success_data(AC_GET_SECURITY_SERVICES_INFO, "{}", dataJson))
        return NET_E_GET_CFG_FAILED;

    System::SecurityServices_S stConfig;
    Convert::to_struct(dataJson, stConfig);
    TvSdkConvert::FillSecurityServicesInfo(stConfig,
                                           *static_cast<pNET_SecurityServicesInfo_S>(lpOutBuffer));
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_security_services_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    System::SecurityServices_S stConfig;
    TvSdkConvert::ToSecurityServicesInfo(
        *static_cast<const NET_SecurityServicesInfo_S *>(lpInBuffer), stConfig);
    return execute_action_expect_success(AC_SET_SECURITY_SERVICES_INFO,
                                         wrap_data_json(Convert::to_string(stConfig))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_ssh_countdown(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    std::string dataJson;
    if (!execute_get_success_data(AC_GET_SSH_COUNTDOWN, "{}", dataJson))
        return NET_E_GET_CFG_FAILED;

    System::SshCountdown_S stCountdown;
    Convert::to_struct(dataJson, stCountdown);
    TvSdkConvert::FillSshCountdownInfo(stCountdown,
                                       *static_cast<pNET_SshCountdownInfo_S>(lpOutBuffer));
    return NET_E_SUCCEED;
}

/*
 * 查询和导出共用相同的入参/出参 ABI；区别只在 IPC ActionCode，
 * 由 actionCode 分别路由至 AC_FIND_LOG 和 AC_EXPORT_LOG。
 */
static NET_COMMON_ECODE_E cb_find_log_impl(INT32 dwChannelID, LPVOID lpOutBuffer, int actionCode)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_LogList_S pOut = static_cast<pNET_LogList_S>(lpOutBuffer);
    Log::RetrievalCond_S stCond;
    Common::PageInfo_S stPage;
    TvSdkConvert::ToLogListRequest(*pOut, stCond, stPage);

    std::string dataJson;
    if (!execute_get_success_data(actionCode, wrap_data_json(Convert::to_string(stCond, stPage)), dataJson))
        return NET_E_GET_CFG_FAILED;

    std::vector<Log::Info_S> logInfos;
    Convert::to_struct(dataJson, logInfos, stPage);
    TvSdkConvert::FillLogList(stCond, stPage, logInfos, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_find_log(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    return cb_find_log_impl(dwChannelID, lpOutBuffer, AC_FIND_LOG);
}

static NET_COMMON_ECODE_E cb_export_log(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    return cb_find_log_impl(dwChannelID, lpOutBuffer, AC_EXPORT_LOG);
}

static NET_COMMON_ECODE_E cb_get_log_server(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    std::string dataJson;
    if (!execute_get_success_data(AC_GET_LOG_SERVER, "{}", dataJson))
        return NET_E_GET_CFG_FAILED;

    System::LogServerInfo_S stConfig;
    Convert::to_struct(dataJson, stConfig);
    TvSdkConvert::FillLogServerInfo(stConfig,
                                    *static_cast<pNET_LogServerInfo_S>(lpOutBuffer));
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_log_server(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    /* SDK ABI 没有 bLogUpload 字段，先读取现有配置后覆盖可见字段，避免意外关闭上传。 */
    System::LogServerInfo_S stConfig;
    std::string dataJson;
    if (!execute_get_success_data(AC_GET_LOG_SERVER, "{}", dataJson))
        return NET_E_GET_CFG_FAILED;
    Convert::to_struct(dataJson, stConfig);
    TvSdkConvert::ToLogServerInfo(*static_cast<const NET_LogServerInfo_S *>(lpInBuffer), stConfig);

    return execute_action_expect_success(AC_SET_LOG_SERVER,
                                         wrap_data_json(Convert::to_string(stConfig))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_test_log_server(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    System::LogServerInfo_S stConfig;
    TvSdkConvert::ToLogServerInfo(*static_cast<const NET_LogServerInfo_S *>(lpInBuffer), stConfig);
    return execute_action_expect_success(AC_TEST_LOG_SERVER,
                                         wrap_data_json(Convert::to_string(stConfig))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
}

/* ---------- 录像控制、计划、检索与下载（473-481） ---------- */
static NET_COMMON_ECODE_E cb_control_record_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    Record_NS::Info_S stRecord;
    TvSdkConvert::ToRecordInfo(*static_cast<const NET_RecordInfo_S *>(lpInBuffer), stRecord);
    return execute_action_expect_success(AC_SET_HUMAN_RECORD,
                                         wrap_data_json(Convert::to_string(stRecord))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_record_status(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    std::string dataJson;
    if (!execute_get_success_data(AC_GET_RECORD_STATUS, "{}", dataJson))
        return NET_E_GET_CFG_FAILED;

    Record_NS::RecordStatusInfo_S stStatus;
    Convert::to_struct(dataJson, stStatus);
    TvSdkConvert::FillRecordStatusInfo(stStatus,
                                       *static_cast<pNET_RecordStatusInfo_S>(lpOutBuffer));
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_get_record_schedule(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    std::string dataJson;
    if (!execute_get_success_data(AC_GET_RECORD_SCHEDULE, "{}", dataJson))
        return NET_E_GET_CFG_FAILED;

    Record_NS::Schedule_S stSchedule;
    Convert::to_struct(dataJson, stSchedule);
    TvSdkConvert::FillRecordSchedule(stSchedule,
                                     *static_cast<pNET_RecordSchedule_S>(lpOutBuffer));
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_record_schedule(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    Record_NS::Schedule_S stSchedule;
    TvSdkConvert::ToRecordSchedule(*static_cast<const NET_RecordSchedule_S *>(lpInBuffer), stSchedule);
    return execute_action_expect_success(AC_SET_RECORD_SCHEDULE,
                                         wrap_data_json(Convert::to_string(stSchedule))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_get_record_advanced_param(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    std::string dataJson;
    if (!execute_get_success_data(AC_GET_RECORD_ADVANCED_PARAM, "{}", dataJson))
        return NET_E_GET_CFG_FAILED;

    Record_NS::AdvancedParam_S stParam;
    Convert::to_struct(dataJson, stParam);
    TvSdkConvert::FillRecordAdvancedParam(stParam,
                                          *static_cast<pNET_RecordAdvancedParam_S>(lpOutBuffer));
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_set_record_advanced_param(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    Record_NS::AdvancedParam_S stParam;
    TvSdkConvert::ToRecordAdvancedParam(*static_cast<const NET_RecordAdvancedParam_S *>(lpInBuffer), stParam);
    return execute_action_expect_success(AC_SET_RECORD_ADVANCED_PARAM,
                                         wrap_data_json(Convert::to_string(stParam))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
}

static NET_COMMON_ECODE_E cb_find_record_file_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_RecordFileList_S pOut = static_cast<pNET_RecordFileList_S>(lpOutBuffer);
    Record_NS::Find_S stFind;
    TvSdkConvert::ToRecordFind(*pOut, stFind);

    std::string dataJson;
    if (!execute_get_success_data(AC_FIND_RECORD_FILE_INFO,
                                  wrap_data_json(Convert::to_string(stFind)), dataJson))
        return NET_E_GET_CFG_FAILED;

    std::vector<Record_NS::FindResult_S> results;
    Convert::to_struct(dataJson, results);
    TvSdkConvert::FillRecordFileList(stFind, results, *pOut);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E cb_download_record_file(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    std::vector<Record_NS::DownloadInfo_S> downloads;
    TvSdkConvert::ToRecordDownloadList(*static_cast<const NET_RecordDownloadList_S *>(lpInBuffer), downloads);
    if (downloads.empty())
        return NET_E_INVALID_PARAM;

    /* 下载任务只负责受理；实际进度由 CTvSdkServer 订阅任务总线后以 481 异步上报。 */
    return execute_action_expect_success(AC_DOWNLOAD_RECORD_FILE,
                                         wrap_data_json(Convert::to_string(downloads))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
}

/* ---------- 人脸比对、目标库与人脸人员信息（482-490） ---------- */
static NET_COMMON_ECODE_E cb_set_face_compare_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

#if !CAP_AI_FACE_COMPARE
    return NET_E_CMD_NOT_SUPPORT;
#else
    /* 该 ABI 未携带关联库，读取当前配置后仅覆盖 SDK 可配置字段。 */
    Alarm::FaceCompare_S stConfig;
    std::string dataJson;
    if (!execute_get_success_data(AC_GET_FACE_COMPARE_INFO, "{}", dataJson))
        return NET_E_GET_CFG_FAILED;
    Convert::to_struct(dataJson, stConfig);
    TvSdkConvert::ToFaceCompareInfo(*static_cast<const NET_FaceCompareInfo_S *>(lpInBuffer), stConfig);

    return execute_action_expect_success(AC_SET_FACE_COMPARE_INFO,
                                         wrap_data_json(Convert::to_string(stConfig))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
#endif
}

static NET_COMMON_ECODE_E cb_add_target_lib(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

#if !CAP_AI_FACE_COMPARE
    return NET_E_CMD_NOT_SUPPORT;
#else
    Event::FaceLibInfo_S stInfo;
    TvSdkConvert::ToFaceLibInfo(*static_cast<const NET_FaceLibInfo_S *>(lpInBuffer), stInfo);
    if (stInfo.strFaceLibName.empty())
        return NET_E_INVALID_PARAM;

    return execute_action_expect_success(AC_ADD_TARGET_LIB,
                                         wrap_data_json(Convert::to_string(stInfo))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
#endif
}

static NET_COMMON_ECODE_E cb_del_target_lib(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

#if !CAP_AI_FACE_COMPARE
    return NET_E_CMD_NOT_SUPPORT;
#else
    Event::FaceLibInfo_S stInfo;
    TvSdkConvert::ToFaceLibInfo(*static_cast<const NET_FaceLibInfo_S *>(lpInBuffer), stInfo);
    if (stInfo.strFaceLibName.empty())
        return NET_E_INVALID_PARAM;

    return execute_action_expect_success(AC_DEL_TARGET_LIB,
                                         wrap_data_json(Convert::to_string(stInfo))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
#endif
}

static NET_COMMON_ECODE_E cb_set_target_lib(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

#if !CAP_AI_FACE_COMPARE
    return NET_E_CMD_NOT_SUPPORT;
#else
    Event::FaceLibInfo_S stInfo;
    TvSdkConvert::ToFaceLibInfo(*static_cast<const NET_FaceLibInfo_S *>(lpInBuffer), stInfo);
    if (stInfo.strFaceLibName.empty())
        return NET_E_INVALID_PARAM;

    /* 内部重命名动作要求 old/new 两个库名；公开 ABI 只携带新名称，旧名称取当前关联库。 */
    Alarm::FaceCompare_S stFaceCompare;
    std::string dataJson;
    if (!execute_get_success_data(AC_GET_FACE_COMPARE_INFO, "{}", dataJson))
        return NET_E_GET_CFG_FAILED;
    Convert::to_struct(dataJson, stFaceCompare);
    if (stFaceCompare.TargetLibInfos.LibId.empty())
        return NET_E_INVALID_PARAM;

    Json::Object *pRoot = Json::init();
    if (!pRoot)
        return NET_E_SET_CFG_FAILED;
    Json::add(pRoot, "LibId_old", stFaceCompare.TargetLibInfos.LibId);
    Json::add(pRoot, "LibId_new", stInfo.strFaceLibName);
    const std::string requestJson = Json::to_string(pRoot);
    Json::deinit(pRoot);

    return execute_action_expect_success(AC_SET_TARGET_LIB, wrap_data_json(requestJson)) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
#endif
}

static NET_COMMON_ECODE_E cb_get_target_lib(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

#if !CAP_AI_FACE_COMPARE
    return NET_E_CMD_NOT_SUPPORT;
#else
    std::string dataJson;
    if (!execute_get_success_data(AC_GET_TARGET_LIB, wrap_data_json("{}"), dataJson))
        return NET_E_GET_CFG_FAILED;

    std::vector<Event::FaceLibInfo_S> targetLibs;
    Convert::to_struct(dataJson, targetLibs);
    TvSdkConvert::FillFaceLibList(targetLibs, *static_cast<pNET_FaceLibList_S>(lpOutBuffer));
    return NET_E_SUCCEED;
#endif
}

static NET_COMMON_ECODE_E cb_add_face_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

#if !CAP_AI_FACE_COMPARE
    return NET_E_CMD_NOT_SUPPORT;
#else
    Event::FaceInfo_S stInfo;
    TvSdkConvert::ToFaceInfo(*static_cast<const NET_FaceInfo_S *>(lpInBuffer), stInfo);
    if (stInfo.strFaceLibName.empty())
        return NET_E_INVALID_PARAM;

    std::string resultJson;
    if (execute_action_expect_success(AC_ADD_FACE_INFO,
                                      wrap_data_json(Convert::to_string(stInfo)),
                                      &resultJson) != 0)
        return NET_E_SET_CFG_FAILED;

    const std::string dataJson = normalize_data_json(resultJson);
    int nRet = 0;
    if (!dataJson.empty() && Json::get(dataJson.c_str(), "nRet", nRet) && nRet != 0)
        return NET_E_SET_CFG_FAILED;
    return NET_E_SUCCEED;
#endif
}

static NET_COMMON_ECODE_E cb_del_face_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

#if !CAP_AI_FACE_COMPARE
    return NET_E_CMD_NOT_SUPPORT;
#else
    Event::FaceIdInfo_S stInfo;
    TvSdkConvert::ToFaceIdInfo(*static_cast<const NET_FaceIdInfo_S *>(lpInBuffer), stInfo);
    if (stInfo.ids.empty())
        return NET_E_INVALID_PARAM;

    return execute_action_expect_success(AC_DEL_FACE_INFO,
                                         wrap_data_json(Convert::to_string(stInfo))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
#endif
}

static NET_COMMON_ECODE_E cb_set_face_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

#if !CAP_AI_FACE_COMPARE
    return NET_E_CMD_NOT_SUPPORT;
#else
    Event::FaceInfo_S stInfo;
    TvSdkConvert::ToFaceInfo(*static_cast<const NET_FaceInfo_S *>(lpInBuffer), stInfo);
    if (stInfo.nId < 0)
        return NET_E_INVALID_PARAM;

    return execute_action_expect_success(AC_SET_FACE_INFO,
                                         wrap_data_json(Convert::to_string(stInfo))) == 0
               ? NET_E_SUCCEED
               : NET_E_SET_CFG_FAILED;
#endif
}

static NET_COMMON_ECODE_E cb_get_face_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

#if !CAP_AI_FACE_COMPARE
    return NET_E_CMD_NOT_SUPPORT;
#else
    /* NET_FaceInfoList_S 仅承载结果，当前 SDK 协议没有独立检索条件，默认查询全部。 */
    Event::FaceFind_S stFind;
    std::string dataJson;
    if (!execute_get_success_data(AC_GET_FACE_INFO,
                                  wrap_data_json(Convert::to_string(stFind)), dataJson))
        return NET_E_GET_CFG_FAILED;

    std::vector<Event::FaceInfo_S> faceInfos;
    Convert::to_struct(dataJson, faceInfos);
    TvSdkConvert::FillFaceInfoList(faceInfos, *static_cast<pNET_FaceInfoList_S>(lpOutBuffer));
    return NET_E_SUCCEED;
#endif
}

void register_all()
{
    NET_serverRegisterGetDeviceInfoCb(cb_get_device_info_impl);
    NET_serverRegisterDeviceControlCb(cb_device_control);
    NET_serverRegisterGetVideoEncodeCapCb(cb_get_video_encode_cap);
    NET_serverRegisterGetAudioEncodeCapCb(cb_get_audio_encode_cap);
    NET_serverRegisterGetOsdCapCb(cb_get_osd_cap);
    NET_serverRegisterGetDeviceConfigCb(cb_get_device_cfg);
    NET_serverRegisterSetDeviceConfigCb(cb_set_device_cfg);
    NET_serverRegisterGetNtpConfigCb(cb_get_ntp_cfg);
    NET_serverRegisterSetNtpConfigCb(cb_set_ntp_cfg);
    NET_serverRegisterSetSystemTimeCb(cb_set_system_time);
    NET_serverRegisterGetSdCardStatusCb(cb_get_sd_card_status);
    NET_serverRegisterGetStreamConfigCb(cb_get_stream_cfg);
    NET_serverRegisterSetStreamConfigCb(cb_set_stream_cfg);
    if (!NET_serverRegisterGetRtspUrlCb(cb_get_rtsp_url))
    {
        dlog_error("TVSDK RTSP URL回调注册失败");
    }
    NET_serverRegisterGetOsdCapConfigCb(cb_get_osd_cap_cfg);
    NET_serverRegisterSetOsdCapConfigCb(cb_set_osd_cap_cfg);

    NET_serverRegisterGetImageConfigCb(cb_get_image_cfg);
    NET_serverRegisterSetImageConfigCb(cb_set_image_cfg);
    NET_serverRegisterGetNetworkConfigCb(cb_get_network_cfg);
    NET_serverRegisterSetNetworkConfigCb(cb_set_network_cfg);

    /* 安全服务、日志及录像接口。481 为异步通知，在 CTvSdkServer 中桥接，不注册请求回调。 */
    NET_serverRegisterGetSecurityServicesInfoCb(cb_get_security_services_info);
    NET_serverRegisterSetSecurityServicesInfoCb(cb_set_security_services_info);
    NET_serverRegisterGetSshCountdownCb(cb_get_ssh_countdown);
    NET_serverRegisterFindLogCb(cb_find_log);
    NET_serverRegisterExportLogCb(cb_export_log);
    NET_serverRegisterGetLogServerCb(cb_get_log_server);
    NET_serverRegisterSetLogServerCb(cb_set_log_server);
    NET_serverRegisterTestLogServerCb(cb_test_log_server);
    NET_serverRegisterControlRecordInfoCb(cb_control_record_info);
    NET_serverRegisterGetRecordStatusCb(cb_get_record_status);
    NET_serverRegisterGetPrivacyMaskConfigCb(cb_get_privacy_mask_cfg);
    NET_serverRegisterSetPrivacyMaskConfigCb(cb_set_privacy_mask_cfg);
    NET_serverRegisterGetPreviewInfoCb(cb_get_preview_info);
    NET_serverRegisterSetPreviewInfoCb(cb_set_preview_info);
    NET_serverRegisterGetTamperAlarmCb(cb_get_tamper_alarm);
    NET_serverRegisterSetTamperAlarmCb(cb_set_tamper_alarm);
    NET_serverRegisterGetMotionAlarmCb(cb_get_motion_alarm);
    NET_serverRegisterSetMotionAlarmCb(cb_set_motion_alarm);
    NET_serverRegisterGetSceneChangeAlarmCb(cb_get_scene_change_alarm);
    NET_serverRegisterSetSceneChangeAlarmCb(cb_set_scene_change_alarm);
    NET_serverRegisterGetCrowGatheringAlarmCb(cb_get_crowd_gathering_alarm);
    NET_serverRegisterSetCrowGatheringAlarmCb(cb_set_crowd_gathering_alarm);
    NET_serverRegisterGetCrossLineAlarmCb(cb_get_cross_line_alarm);
    NET_serverRegisterSetCrossLineAlarmCb(cb_set_cross_line_alarm);

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    NET_serverRegisterGetGarbageExposureConfigCb(cb_get_garbage_exposure_cfg);
    NET_serverRegisterSetGarbageExposureConfigCb(cb_set_garbage_exposure_cfg);
    NET_serverRegisterGetGarbageOverflowConfigCb(cb_get_garbage_overflow_cfg);
    NET_serverRegisterSetGarbageOverflowConfigCb(cb_set_garbage_overflow_cfg);
#endif

#ifdef SCENE_INTELLIGENCE
    NET_serverRegisterGetManholeCoverAbnormalConfigCb(cb_get_manhole_cover_abnormal_cfg);
    NET_serverRegisterSetManholeCoverAbnormalConfigCb(cb_set_manhole_cover_abnormal_cfg);
    NET_serverRegisterGetSleepOnDutyConfigCb(cb_get_sleep_on_duty_cfg);
    NET_serverRegisterSetSleepOnDutyConfigCb(cb_set_sleep_on_duty_cfg);
    NET_serverRegisterGetElectricVehicleInElevatorConfigCb(cb_get_electric_vehicle_in_elevator_cfg);
    NET_serverRegisterSetElectricVehicleInElevatorConfigCb(cb_set_electric_vehicle_in_elevator_cfg);
    NET_serverRegisterGetPersonFallDownConfigCb(cb_get_person_fall_down_cfg);
    NET_serverRegisterSetPersonFallDownConfigCb(cb_set_person_fall_down_cfg);
    NET_serverRegisterGetConstructionOccupyRoadConfigCb(cb_get_construction_occupy_road_cfg);
    NET_serverRegisterSetConstructionOccupyRoadConfigCb(cb_set_construction_occupy_road_cfg);
    NET_serverRegisterGetCongestionConfigCb(cb_get_congestion_cfg);
    NET_serverRegisterSetCongestionConfigCb(cb_set_congestion_cfg);
    NET_serverRegisterGetLicensePlateRecognitionConfigCb(cb_get_license_plate_recognition_cfg);
    NET_serverRegisterSetLicensePlateRecognitionConfigCb(cb_set_license_plate_recognition_cfg);
    NET_serverRegisterGetHighAltitudeSeatbeltConfigCb(cb_get_high_altitude_seatbelt_cfg);
    NET_serverRegisterSetHighAltitudeSeatbeltConfigCb(cb_set_high_altitude_seatbelt_cfg);
    NET_serverRegisterGetSafetyHelmetConfigCb(cb_get_safety_helmet_cfg);
    NET_serverRegisterSetSafetyHelmetConfigCb(cb_set_safety_helmet_cfg);
    NET_serverRegisterGetPersonFallConfigCb(cb_get_person_fall_cfg);
    NET_serverRegisterSetPersonFallConfigCb(cb_set_person_fall_cfg);
    NET_serverRegisterGetPhoneUsageConfigCb(cb_get_phone_usage_cfg);
    NET_serverRegisterSetPhoneUsageConfigCb(cb_set_phone_usage_cfg);
    NET_serverRegisterGetSmokingConfigCb(cb_get_smoking_cfg);
    NET_serverRegisterSetSmokingConfigCb(cb_set_smoking_cfg);
    NET_serverRegisterGetOpenFlameConfigCb(cb_get_open_flame_cfg);
    NET_serverRegisterSetOpenFlameConfigCb(cb_set_open_flame_cfg);
    NET_serverRegisterGetBareSoilConfigCb(cb_get_bare_soil_cfg);
    NET_serverRegisterSetBareSoilConfigCb(cb_set_bare_soil_cfg);
    NET_serverRegisterGetHoleProtectionBarConfigCb(cb_get_hole_protection_bar_cfg);
    NET_serverRegisterSetHoleProtectionBarConfigCb(cb_set_hole_protection_bar_cfg);
    NET_serverRegisterGetReflectiveClothingConfigCb(cb_get_reflective_clothing_cfg);
    NET_serverRegisterSetReflectiveClothingConfigCb(cb_set_reflective_clothing_cfg);
#endif

    NET_serverRegisterGetPetRecognitionInfoCb(cb_get_pet_recognition_info);
    NET_serverRegisterSetPetRecognitionInfoCb(cb_set_pet_recognition_info);
#ifdef SCENE_INTELLIGENCE
    NET_serverRegisterGetClimbFenceInfoCb(cb_get_climb_fence_info);
    NET_serverRegisterSetClimbFenceInfoCb(cb_set_climb_fence_info);
    NET_serverRegisterGetDimissionInfoCb(cb_get_dimission_info);
    NET_serverRegisterSetDimissionInfoCb(cb_set_dimission_info);
    NET_serverRegisterGetIllegalLaneInfoCb(cb_get_illegal_lane_info);
    NET_serverRegisterSetIllegalLaneInfoCb(cb_set_illegal_lane_info);
    NET_serverRegisterGetRetrogradeInfoCb(cb_get_retrograde_info);
    NET_serverRegisterSetRetrogradeInfoCb(cb_set_retrograde_info);
    NET_serverRegisterGetNonmotorVehicleIntrusionInfoCb(cb_get_nonmotor_vehicle_intrusion_info);
    NET_serverRegisterSetNonmotorVehicleIntrusionInfoCb(cb_set_nonmotor_vehicle_intrusion_info);
    NET_serverRegisterGetOccupationEmergencyInfoCb(cb_get_occupation_emergency_info);
    NET_serverRegisterSetOccupationEmergencyInfoCb(cb_set_occupation_emergency_info);
    NET_serverRegisterGetPedestrianIntrusionInfoCb(cb_get_pedestrian_intrusion_info);
    NET_serverRegisterSetPedestrianIntrusionInfoCb(cb_set_pedestrian_intrusion_info);
    NET_serverRegisterGetSmokeFireConfigCb(cb_get_smoke_fire_cfg);
    NET_serverRegisterSetSmokeFireConfigCb(cb_set_smoke_fire_cfg);
    NET_serverRegisterGetRoadPondingConfigCb(cb_get_road_ponding_cfg);
    NET_serverRegisterSetRoadPondingConfigCb(cb_set_road_ponding_cfg);
#endif

#if CAP_AI_PEOPLE_STATISTICS
    NET_serverRegisterGetPeopleFlowStatisticsConfigCb(cb_get_people_flow_statistics_cfg);
    NET_serverRegisterSetPeopleFlowStatisticsConfigCb(cb_set_people_flow_statistics_cfg);
    NET_serverRegisterResetPeopleFlowStatisticsCb(cb_reset_people_flow_statistics);
    NET_serverRegisterGetPeopleDensityDetectionConfigCb(cb_get_people_density_detection_cfg);
    NET_serverRegisterSetPeopleDensityDetectionConfigCb(cb_set_people_density_detection_cfg);
#endif

    NET_serverRegisterGetIntrusionAlarmCb(cb_get_intrusion_alarm);
    NET_serverRegisterSetIntrusionAlarmCb(cb_set_intrusion_alarm);
    NET_serverRegisterGetLoiteringAlarmCb(cb_get_loitering_alarm);
    NET_serverRegisterSetLoiteringAlarmCb(cb_set_loitering_alarm);
    NET_serverRegisterGetAudioAnomalyAlarmCb(cb_get_audio_anomaly_alarm);
    NET_serverRegisterSetAudioAnomalyAlarmCb(cb_set_audio_anomaly_alarm);
    NET_serverRegisterGetAudioAnomalyCurrentDbCb(cb_get_audio_anomaly_current_db);

    NET_serverRegisterGetAudibleAlarmInfoCb(cb_get_audible_alarm_info);
    NET_serverRegisterSetAudibleAlarmInfoCb(cb_set_audible_alarm_info);
    NET_serverRegisterGetAlarmInputInfoCb(cb_get_alarm_input_info);
    NET_serverRegisterSetAlarmInputInfoCb(cb_set_alarm_input_info);
    NET_serverRegisterGetAlarmOutputInfoCb(cb_get_alarm_output_info);
    NET_serverRegisterSetAlarmOutputInfoCb(cb_set_alarm_output_info);
    NET_serverRegisterGetFlashingLightAlarmInfoCb(cb_get_flashing_light_alarm_info);
    NET_serverRegisterSetFlashingLightAlarmInfoCb(cb_set_flashing_light_alarm_info);
    NET_serverRegisterGetPirAlarmInfoCb(cb_get_pir_alarm_info);
    NET_serverRegisterSetPirAlarmInfoCb(cb_set_pir_alarm_info);
    NET_serverRegisterGetRecordScheduleCb(cb_get_record_schedule);
    NET_serverRegisterSetRecordScheduleCb(cb_set_record_schedule);
    NET_serverRegisterGetRecordAdvancedParamCb(cb_get_record_advanced_param);
    NET_serverRegisterSetRecordAdvancedParamCb(cb_set_record_advanced_param);
    NET_serverRegisterFindRecordFileInfoCb(cb_find_record_file_info);
    NET_serverRegisterDownloadRecordFileCb(cb_download_record_file);

    NET_serverRegisterGetParkingAlarmCb(cb_get_parking_detect_alarm);
    NET_serverRegisterSetParkingAlarmCb(cb_set_parking_detect_alarm);
    NET_serverRegisterGetUnattendedObjectAlarmCb(cb_get_unattended_object_alarm);
    NET_serverRegisterSetUnattendedObjectAlarmCb(cb_set_unattended_object_alarm);
    NET_serverRegisterGetObjectRemovalAlarmCb(cb_get_object_removal_alarm);
    NET_serverRegisterSetObjectRemovalAlarmCb(cb_set_object_removal_alarm);

    NET_serverRegisterGetUpgradeStatusCb(cb_get_upgrade_status);
    NET_serverRegisterSetUpgradeCb(cb_set_upgrade);
    NET_serverRegisterGetUpgradeVersionCb(cb_get_upgrade_version);

    NET_serverRegisterGetCapturePlanInfoCb(cb_get_capture_plan_info);
    NET_serverRegisterSetCapturePlanInfoCb(cb_set_capture_plan_info);
    NET_serverRegisterGetCaptureParamInfoCb(cb_get_capture_param_info);
    NET_serverRegisterSetCaptureParamInfoCb(cb_set_capture_param_info);

    NET_serverRegisterGetExposureInfoCb(cb_get_exposure_info);
    NET_serverRegisterSetExposureInfoCb(cb_set_exposure_info);
    NET_serverRegisterGetDayNightInfoCb(cb_get_daynight_info);
    NET_serverRegisterSetDayNightInfoCb(cb_set_daynight_info);
    NET_serverRegisterGetBackLightInfoCb(cb_get_backlight_info);
    NET_serverRegisterSetBackLightInfoCb(cb_set_backlight_info);
    NET_serverRegisterGetDenoiseInfoCb(cb_get_denoise_info);
    NET_serverRegisterSetDenoiseInfoCb(cb_set_denoise_info);
    NET_serverRegisterGetWhiteBalanceInfoCb(cb_get_whitebalance_info);
    NET_serverRegisterSetWhiteBalanceInfoCb(cb_set_whitebalance_info);
    
    NET_serverRegisterSetTalkbackStateCb(cb_set_talkback_state);
    NET_serverRegisterSetTalkbackToStreamCb(cb_set_talkback_to_stream);
    NET_serverRegisterGetTalkbackFromStreamCb(cb_get_talkback_from_stream);
    NET_serverRegisterSetReplayTalkbackCb(cb_set_replay_talkback);

    NET_serverRegisterGetAudioConfigCb(cb_get_audio_cfg);
    NET_serverRegisterSetAudioConfigCb(cb_set_audio_cfg);
    NET_serverRegisterGetEnterRegionAlarmCb(cb_get_enter_region_alarm);
    NET_serverRegisterSetEnterRegionAlarmCb(cb_set_enter_region_alarm);
    NET_serverRegisterGetLeaveRegionAlarmCb(cb_get_leave_region_alarm);
    NET_serverRegisterSetLeaveRegionAlarmCb(cb_set_leave_region_alarm);

    NET_serverRegisterGetFaceCaptureInfoCb(cb_get_face_capture_info);
    NET_serverRegisterSetFaceCaptureInfoCb(cb_set_face_capture_info);
    //NET_serverRegisterGetFaceCaptureOverlayInfoCb(cb_get_face_capture_overlay_info);
    //NET_serverRegisterSetFaceCaptureOverlayInfoCb(cb_set_face_capture_overlay_info);

    /* 人脸库和人员信息动作在未编译人脸算法能力时由对应回调返回“不支持”。 */
    NET_serverRegisterSetFaceCompareInfoCb(cb_set_face_compare_info);
    NET_serverRegisterAddTargetLibCb(cb_add_target_lib);
    NET_serverRegisterDelTargetLibCb(cb_del_target_lib);
    NET_serverRegisterSetTargetLibCb(cb_set_target_lib);
    NET_serverRegisterGetTargetLibCb(cb_get_target_lib);
    NET_serverRegisterAddFaceInfoCb(cb_add_face_info);
    NET_serverRegisterDelFaceInfoCb(cb_del_face_info);
    NET_serverRegisterSetFaceInfoCb(cb_set_face_info);
    NET_serverRegisterGetFaceInfoCb(cb_get_face_info);
}

} // namespace TvSdkCallbacks

