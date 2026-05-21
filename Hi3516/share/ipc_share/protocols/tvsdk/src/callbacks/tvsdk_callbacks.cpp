/**
 * @FilePath     : tvsdk_callbacks.cpp
 * @Description  : TVSDK 回调实现与注册（使用 action_code.h 命令码对接 control_manage）
 */

#include "tvsdk_callbacks.h"

#include <string>
#include <cstring>
#include <vector>

#include "task_manage.h"
#include "task.h"
#include "dlog.h"
#include "action_code.h"
#include "system_manage.h"
#include "system_define.h"
#include "network_define.h"
#include "alarm_define.h"
#include "event_define.h"
#include "preview_define.h"
#include "Json.h"
#include "convert_interface.h"
#include "video_define.h"

#include "convert/tvsdk_convert.h"
#include "rtsp_server.h"
#include "upgrade_client.h"

namespace TvSdkCallbacks
{
static CTaskManage *s_taskManage = nullptr;

// 移动侦测 / 遮挡报警 使用的 TVSDK 中间缓存，避免直接在 SDK 传入缓冲区上做复杂写入
static NET_TV_MOTION_ALARM_INFO_S g_tvMotionAlarmInfo;
static NET_TV_TAMPER_ALARM_INFO_S g_tvTamperAlarmInfo;
static NET_TV_AUDIO_ANOMALY_ALARM_INFO_S g_tvAudioAnomalyAlarmInfo;

static int execute_get_result(int actionCode, const std::string &inJson, std::string &outJson);

static const char *kDefaultUpgradeDir = "/opt/course/";

static bool is_absolute_path(const std::string &path)
{
    return !path.empty() && path[0] == '/';
}

static bool is_valid_upgrade_path(const std::string &path)
{
    if (path.empty() || path.size() >= NET_TV_FILE_NAME_LEN)
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

    // 某些下游动作只返回执行码，不返回 JSON，此时 outJson 为空也视作成功。
    if (!outJson.empty())
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
    Task::Info_S stInfo;
    stInfo.data = inJson;
    stInfo.fnResultCallbacks = [&outJson](const void *pData, int nLen, int /*nActionCode*/, void * /*pHandler*/) -> int {
        if (pData && nLen > 0)
            outJson.assign(static_cast<const char *>(pData), static_cast<size_t>(nLen));
        return 0;
    };
    return s_taskManage->execute(actionCode, stInfo);
}

/* ---------- GetDeviceInfo：由 SystemManage 填充 ---------- */
static NET_TV_COMMON_ECODE_E cb_get_device_info_impl(LPNET_TV_DEVICE_INFO_S pInfo)
{
    if (!pInfo)
        return NET_TV_E_NULL_POINT;
    ::System::DeviceInfo_S stDeviceInfo;
    if (SystemManage::instance()->get_device_info(stDeviceInfo) != 0)
        return NET_TV_E_GET_CFG_FAILED;
    TvSdkConvert::FillDeviceInfo(stDeviceInfo, *pInfo);
    return NET_TV_E_SUCCEED;
}

/* ---------- GetVideoEncodeCap：AC_GET_VIDEO_CAPABILITY_SET ---------- */
static NET_TV_COMMON_ECODE_E cb_get_video_encode_cap(INT32 dwChannelID, LPNET_TV_VIDEO_ENCODE_CAP_S pCap)
{
    if (!pCap)
        return NET_TV_E_NULL_POINT;
    memset(pCap, 0, sizeof(NET_TV_VIDEO_ENCODE_CAP_S));
    (void)dwChannelID;

    std::string outJson;
    if (execute_get_result(AC_GET_VIDEO_CAPABILITY_SET, "{}", outJson) != 0 || outJson.empty())
    {
        pCap->dwStreamCount = 0;
        return NET_TV_E_SUCCEED;
    }

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Video_NS::VideoCapabilitySet_S stCapSet;
    Convert::to_struct(dataJson, stCapSet);
    TvSdkConvert::FillVideoEncodeCap(stCapSet, *pCap);

    return NET_TV_E_SUCCEED;
}

/* ---------- GetAudioEncodeCap：AC_GET_AUDIO_CAPABILITY_SET ---------- */
static NET_TV_COMMON_ECODE_E cb_get_audio_encode_cap(INT32 dwChannelID, LPNET_TV_AUDIO_CAP_S pCap)
{
    if (!pCap)
        return NET_TV_E_NULL_POINT;

    memset(pCap, 0, sizeof(NET_TV_AUDIO_CAP_S));
    (void)dwChannelID;

    std::string outJson;
    if (execute_get_result(AC_GET_AUDIO_CAPABILITY_SET, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_SUCCEED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Audio_NS::AudioCapabilitySet_S stCapSet;
    Convert::to_struct(dataJson, stCapSet);
    TvSdkConvert::FillAudioEncodeCap(stCapSet, *pCap);

    return NET_TV_E_SUCCEED;
}

/* ---------- GetOsdCap：AC_GET_OSD_CONFIG ---------- */
static NET_TV_COMMON_ECODE_E cb_get_osd_cap(INT32 dwChannelID, LPNET_TV_OSD_CAP_S pCap)
{
    (void)dwChannelID;
    if (!pCap)
        return NET_TV_E_NULL_POINT;
    memset(pCap, 0, sizeof(NET_TV_OSD_CAP_S));

    std::string outJson;
    if (execute_get_result(AC_GET_OSD_CONFIG, "{}", outJson) == 0 && !outJson.empty())
    {
        int nRet = -1;
        Json::get(outJson.c_str(), "Return", nRet);
        if (nRet == 0)
        {
            pCap->bSupportOsd = TRUE;
            pCap->bSupportName = TRUE;
            pCap->bSupportTime = TRUE;
            pCap->bSupportWeek = TRUE;
            pCap->bSupportCustomColor = TRUE;
            pCap->udwMaxOsdNum = NET_TV_OSD_MAX_NUM_EX;
            pCap->udwSupportedFontSizeNum = 4;
            pCap->audwSupportedFontSizeList[0] = OSD_FONT_SIZE_ADAPTIVE;
            pCap->audwSupportedFontSizeList[1] = OSD_FONT_SIZE_16;
            pCap->audwSupportedFontSizeList[2] = OSD_FONT_SIZE_32;
            pCap->audwSupportedFontSizeList[3] = OSD_FONT_SIZE_48;
            pCap->udwSupportedDateFormatNum = 9;
            pCap->audwSupportedDateFormatList[0] = ENGLISH_YYYY_MM_DD;
            pCap->audwSupportedDateFormatList[1] = ENGLISH_MM_DD_YYYY;
            pCap->audwSupportedDateFormatList[2] = ENGLISH_DD_MM_YYYY;
            pCap->audwSupportedDateFormatList[3] = CHINESE_YYYYMMDD;
            pCap->audwSupportedDateFormatList[4] = CHINESE_MMDDYYYY;
            pCap->audwSupportedDateFormatList[5] = CHINESE_DDMMYYYY;
            pCap->audwSupportedDateFormatList[6] = ENGLISH_YYYYMMDD;
            pCap->audwSupportedDateFormatList[7] = ENGLISH_MMDDYYYY;
            pCap->audwSupportedDateFormatList[8] = ENGLISH_DDMMYYYY;
            pCap->udwSupportedTimeFormatNum = 2;
            pCap->audwSupportedTimeFormatList[0] = OSD_TIME_FORMAT_24;
            pCap->audwSupportedTimeFormatList[1] = OSD_TIME_FORMAT_12;
            pCap->udwSupportedAlignNum = 6;
            pCap->audwSupportedAlignList[0] = OSD_ALIFN_CUSTOMIZE;
            pCap->audwSupportedAlignList[1] = OSD_ALIFN_CHARACTER_LEFT;
            pCap->audwSupportedAlignList[2] = OSD_ALIFN_CHARACTER_RIGHT;
            pCap->audwSupportedAlignList[3] = OSD_ALIFN_ALL_LEFT;
            pCap->audwSupportedAlignList[4] = OSD_ALIFN_ALL_RIGHT;
            pCap->audwSupportedAlignList[5] = OSD_ALIFN_GB_MODE;
        }
    }
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_get_osd_cap_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    return cb_get_osd_cap(dwChannelID, (LPNET_TV_OSD_CAP_S)lpOutBuffer);
}

static NET_TV_COMMON_ECODE_E cb_get_device_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_DEVICE_BASICINFO_S pOut = (LPNET_TV_DEVICE_BASICINFO_S)lpOutBuffer;

    ::System::DeviceInfo_S stDeviceInfo;
    if (SystemManage::instance()->get_device_info(stDeviceInfo) != 0)
        return NET_TV_E_GET_CFG_FAILED;

    TvSdkConvert::FillDeviceBasicInfo(stDeviceInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_device_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_DEVICE_BASICINFO_S pIn = (LPNET_TV_DEVICE_BASICINFO_S)lpInBuffer;
    ::System::DeviceInfo_S stDeviceInfo;
    // 先读取当前信息，避免覆盖其他字段
    (void)SystemManage::instance()->get_device_info(stDeviceInfo);
    TvSdkConvert::ToDeviceInfo(*pIn, stDeviceInfo);

    int nRet = SystemManage::instance()->set_device_info(stDeviceInfo);
    return (nRet == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* 其余配置仍通过命令码 + JSON 透传，后续若有 SDK 结构体定义，可按上面的方式继续细化 */

static NET_TV_COMMON_ECODE_E get_cfg_by_action(INT32 dwChannelID, int actionCode, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_NULL_POINT;
    if (!s_taskManage)
        return NET_TV_E_GET_CFG_FAILED;

    std::string outJson;
    if (execute_get_result(actionCode, "{}", outJson) != 0)
        return NET_TV_E_GET_CFG_FAILED;

    size_t len = outJson.size() + 1;
    if (len > NET_TV_LEN_4096)
        return NET_TV_E_NOENOUGH_BUF;
    memcpy(lpOutBuffer, outJson.c_str(), len);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E set_cfg_by_action(INT32 dwChannelID, int actionCode, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_NULL_POINT;
    if (!s_taskManage)
        return NET_TV_E_SET_CFG_FAILED;

    Task::Info_S stInfo;
    stInfo.data = std::string(static_cast<const char *>(lpInBuffer));
    int nRet = s_taskManage->execute(actionCode, stInfo);
    return (nRet == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
static NET_TV_COMMON_ECODE_E cb_get_ntp_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    return get_cfg_by_action(dwChannelID, AC_GET_TIME_INFO, lpOutBuffer);
}
static NET_TV_COMMON_ECODE_E cb_set_ntp_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    return set_cfg_by_action(dwChannelID, AC_SET_TIME_INFO, lpInBuffer);
}

static bool parse_stream_cfg_json(const std::string &strJson, Video_NS::VideoConfig_S &stCfg)
{
    if (strJson.empty())
    {
        dlog_warn("[TVSDK][VideoCfg] parse failed: empty data json");
        return false;
    }

    Json::Object *pRoot = Json::init(strJson.c_str());
    if (!pRoot)
    {
        dlog_warn("[TVSDK][VideoCfg] parse failed: invalid data json, len=%u", (unsigned)strJson.size());
        return false;
    }

    const bool bIsConfigList = (Json::get(pRoot, "VideoConfig") != nullptr);
    Json::deinit(pRoot);
    dlog_info("[TVSDK][VideoCfg] data json len=%u, has VideoConfig list=%d",
              (unsigned)strJson.size(), bIsConfigList ? 1 : 0);

    if (bIsConfigList)
    {
        std::vector<Video_NS::VideoConfig_S> vecCfg;
        Convert::to_struct(strJson, vecCfg);
        if (vecCfg.empty())
        {
            dlog_warn("[TVSDK][VideoCfg] parse failed: VideoConfig list is empty");
            return false;
        }

        dlog_info("[TVSDK][VideoCfg] parsed VideoConfig count=%u", (unsigned)vecCfg.size());
        for (const auto &cfg : vecCfg)
        {
            dlog_info("[TVSDK][VideoCfg] candidate id=%d type=%d %dx%d fps=%d bitrateType=%d upper=%d avg=%d codec=%d smart=%d iframe=%d svc=%d smooth=%d",
                      cfg.nId,
                      (int)cfg.enVideoType,
                      cfg.stVideoResolution.nWidth,
                      cfg.stVideoResolution.nHeight,
                      cfg.getFrameRateAsInt(),
                      (int)cfg.enBitrateType,
                      cfg.nBitrateUpperLimit,
                      cfg.nAverageBitrate,
                      (int)cfg.enVideoCodec,
                      cfg.bSmartEnable ? 1 : 0,
                      cfg.nIFrameInterval,
                      (int)cfg.enSvcEnable,
                      cfg.nBitrateSmoothing);
        }

        for (const auto &cfg : vecCfg)
        {
            if (cfg.nId == NET_TV_LIVE_STREAM_INDEX_MAIN)
            {
                stCfg = cfg;
                dlog_info("[TVSDK][VideoCfg] selected main stream id=%d", stCfg.nId);
                return true;
            }
        }

        stCfg = vecCfg.front();
        dlog_warn("[TVSDK][VideoCfg] main stream id=%d not found, use first id=%d",
                  NET_TV_LIVE_STREAM_INDEX_MAIN, stCfg.nId);
        return true;
    }

    Convert::to_struct(strJson, stCfg);
    dlog_info("[TVSDK][VideoCfg] parsed single config id=%d type=%d %dx%d fps=%d bitrateType=%d upper=%d avg=%d codec=%d smart=%d iframe=%d svc=%d smooth=%d",
              stCfg.nId,
              (int)stCfg.enVideoType,
              stCfg.stVideoResolution.nWidth,
              stCfg.stVideoResolution.nHeight,
              stCfg.getFrameRateAsInt(),
              (int)stCfg.enBitrateType,
              stCfg.nBitrateUpperLimit,
              stCfg.nAverageBitrate,
              (int)stCfg.enVideoCodec,
              stCfg.bSmartEnable ? 1 : 0,
              stCfg.nIFrameInterval,
              (int)stCfg.enSvcEnable,
              stCfg.nBitrateSmoothing);
    return true;
}

static NET_TV_COMMON_ECODE_E cb_get_stream_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_VIDEO_ENCODE_OPTION_S pOut = (LPNET_TV_VIDEO_ENCODE_OPTION_S)lpOutBuffer;

    std::string outJson;
    int nExecResult = execute_get_result(AC_GET_VIDEO_CONFIG, "{}", outJson);
    dlog_info("[TVSDK][VideoCfg] AC_GET_VIDEO_CONFIG exec=%d, outJson.len=%u",
              nExecResult, (unsigned)outJson.size());
    if (nExecResult != 0 || outJson.empty())
    {
        dlog_warn("[TVSDK][VideoCfg] get video config failed: exec=%d, outJson.empty=%d",
                  nExecResult, outJson.empty() ? 1 : 0);
        return NET_TV_E_GET_CFG_FAILED;
    }

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
    {
        dlog_warn("[TVSDK][VideoCfg] get video config return failed: Return=%d, body=%s",
                  nRet, outJson.c_str());
        return NET_TV_E_GET_CFG_FAILED;
    }

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
    {
        dlog_warn("[TVSDK][VideoCfg] normalize Data failed, body=%s", outJson.c_str());
        return NET_TV_E_GET_CFG_FAILED;
    }
    dlog_info("[TVSDK][VideoCfg] normalized data=%s", strJson.c_str());

    Video_NS::VideoConfig_S stCfg;
    if (!parse_stream_cfg_json(strJson, stCfg))
        return NET_TV_E_GET_CFG_FAILED;
    dlog_info("[TVSDK][VideoCfg] selected ipc config id=%d type=%d %dx%d fps=%d bitrateType=%d upper=%d avg=%d codec=%d smart=%d iframe=%d svc=%d smooth=%d",
              stCfg.nId,
              (int)stCfg.enVideoType,
              stCfg.stVideoResolution.nWidth,
              stCfg.stVideoResolution.nHeight,
              stCfg.getFrameRateAsInt(),
              (int)stCfg.enBitrateType,
              stCfg.nBitrateUpperLimit,
              stCfg.nAverageBitrate,
              (int)stCfg.enVideoCodec,
              stCfg.bSmartEnable ? 1 : 0,
              stCfg.nIFrameInterval,
              (int)stCfg.enSvcEnable,
              stCfg.nBitrateSmoothing);
    TvSdkConvert::FillVideoEncodeOption(stCfg, *pOut);
    dlog_info("[TVSDK][VideoCfg] sdk output id=%d type=%d %dx%d fps=%d bitrateType=%d upper=%d avg=%d codec=%d smart=%d iframe=%d svc=%d smooth=%d",
              pOut->nId,
              pOut->enVideoType,
              pOut->stVideoResolution.dwWidth,
              pOut->stVideoResolution.dwHeight,
              pOut->enFrameRate,
              pOut->enBitrateType,
              pOut->nBitrateUpperLimit,
              pOut->nAverageBitrate,
              pOut->enVideoCodec,
              pOut->bSmartEnable,
              pOut->nIFrameInterval,
              pOut->enSvcEnable,
              pOut->nBitrateSmoothing);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_stream_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_VIDEO_ENCODE_OPTION_S *pIn = (const NET_TV_VIDEO_ENCODE_OPTION_S *)lpInBuffer;
    Video_NS::VideoConfig_S stCfg;
    TvSdkConvert::ToVideoConfig(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_VIDEO_CONFIG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_osd_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_NULL_POINT;

    LPNET_TV_VIDEO_OSD_CFG_S pOut = (LPNET_TV_VIDEO_OSD_CFG_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_OSD_CONFIG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Osd::OsdConfig_S stOsdConfig;
    Convert::to_struct(dataJson, stOsdConfig);
    TvSdkConvert::FillOsdConfig(stOsdConfig, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_osd_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_VIDEO_OSD_CFG_S *pIn = (const NET_TV_VIDEO_OSD_CFG_S *)lpInBuffer;
    Osd::OsdConfig_S stOsdConfig;
    TvSdkConvert::ToOsdConfig(*pIn, stOsdConfig);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stOsdConfig));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_OSD_CONFIG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
static NET_TV_COMMON_ECODE_E cb_get_image_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    return get_cfg_by_action(dwChannelID, AC_GET_VIDEO_EFFECT_INFO, lpOutBuffer);
}
static NET_TV_COMMON_ECODE_E cb_set_image_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    return set_cfg_by_action(dwChannelID, AC_SET_VIDEO_EFFECT_INFO, lpInBuffer);
}
static NET_TV_COMMON_ECODE_E cb_get_network_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_NETWORKCFG_S pOut = (LPNET_TV_NETWORKCFG_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_NETWORK_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Network::Info_S stNetInfo;
    Convert::to_struct(outJson, stNetInfo);
    TvSdkConvert::FillNetworkCfg(stNetInfo, *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_network_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_NETWORKCFG_S pIn = (LPNET_TV_NETWORKCFG_S)lpInBuffer;

    Network::Info_S stNetInfo;
    TvSdkConvert::ToNetworkInfo(*pIn, stNetInfo);

    std::string inJson = Convert::to_string(stNetInfo);
    Task::Info_S stInfo;
    stInfo.data = inJson;
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_NETWORK_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_set_config_wifi_sta(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_WIFI_STA_CFG_S *pIn = (const NET_TV_WIFI_STA_CFG_S *)lpInBuffer;
    Network::WifiStaInfo_S stCfg;
    TvSdkConvert::ToWifiStaInfo(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(wifi_sta_info_to_json(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CONFIG_WIFI_STA, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_connect_wifi_sta(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_WIFI_STA_CONNECT_S *pIn = (const NET_TV_WIFI_STA_CONNECT_S *)lpInBuffer;
    Network::WifiStaConncet_S stCfg;
    TvSdkConvert::ToWifiStaConnect(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(wifi_sta_connect_to_json(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_CONNECT_WIFI_STA, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_disconnect_wifi_sta(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    (void)lpInBuffer;

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json("{}");
    int nExec = s_taskManage ? s_taskManage->execute(AC_DISCONNECT_WIFI_STA, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_4g_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_4G_INFO_S pOut = (LPNET_TV_4G_INFO_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_4G_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Network::Network_4G_Config_t stCfg;
    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        dataJson = outJson;
    (void)parse_4g_from_json(dataJson, stCfg);

    TvSdkConvert::Fill4GInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_4g_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_4G_INFO_S *pIn = (const NET_TV_4G_INFO_S *)lpInBuffer;
    Network::Network_4G_Config_t stCfg;
    TvSdkConvert::To4GConfig(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(config_4g_to_json(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_4G_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_set_hotspot_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_HOTSPOT_INFO_S *pIn = (const NET_TV_HOTSPOT_INFO_S *)lpInBuffer;
    Network::HotspotConfig stCfg;
    TvSdkConvert::ToHotspotConfig(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(hotspot_to_json(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_HOTSPOT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_hotspot_conn(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_HOTSPOT_CONN_INFO_S pOut = (LPNET_TV_HOTSPOT_CONN_INFO_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_HOTSPOT_CONN, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    if (!TvSdkConvert::FillHotspotConnInfoFromJson(outJson, *pOut))
        return NET_TV_E_GET_CFG_FAILED;

    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_get_security_services_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_SECURITY_SERVICES_INFO_S pOut = (LPNET_TV_SECURITY_SERVICES_INFO_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_SECURITY_SERVICES_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    ::System::SecurityServices_S stInfo;
    Convert::to_struct(dataJson, stInfo);
    TvSdkConvert::FillSecurityServices(stInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_security_services_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_SECURITY_SERVICES_INFO_S *pIn = (const NET_TV_SECURITY_SERVICES_INFO_S *)lpInBuffer;
    ::System::SecurityServices_S stInfo;
    TvSdkConvert::ToSecurityServices(*pIn, stInfo);

    std::string inJson = wrap_data_json(Convert::to_string(stInfo));
    return (execute_action_expect_success(AC_SET_SECURITY_SERVICES_INFO, inJson) == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_ssh_countdown(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_SSH_COUNTDOWN_INFO_S pOut = (LPNET_TV_SSH_COUNTDOWN_INFO_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_SSH_COUNTDOWN, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    ::System::SshCountdown_S stInfo;
    Convert::to_struct(dataJson, stInfo);
    TvSdkConvert::FillSshCountdown(stInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_query_log_by_action(int actionCode, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_LOG_LIST_S pOut = (LPNET_TV_LOG_LIST_S)lpOutBuffer;

    Log::RetrievalCond_S stCond;
    Common::PageInfo_S stPage;
    TvSdkConvert::ToLogRetrievalCond(pOut->stCond, stCond);
    TvSdkConvert::ToPageInfo(pOut->stPage, stPage);
    if (stPage.nCurPage == 0)
        stPage.nCurPage = 1;
    if (stPage.nPageSize <= 0)
        stPage.nPageSize = NET_TV_LOG_QUERY_COND_NUM;

    std::string outJson;
    std::string inJson = wrap_data_json(Convert::to_string(stCond, stPage));
    if (execute_get_result(actionCode, inJson, outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    std::vector<Log::Info_S> vecLogInfo;
    Common::PageInfo_S stRespPage = stPage;
    Convert::to_struct(dataJson, vecLogInfo, stRespPage);
    TvSdkConvert::FillLogList(vecLogInfo, stRespPage, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_find_log(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    return cb_query_log_by_action(AC_FIND_LOG, lpOutBuffer);
}

static NET_TV_COMMON_ECODE_E cb_export_log(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    return cb_query_log_by_action(AC_EXPORT_LOG, lpOutBuffer);
}

static NET_TV_COMMON_ECODE_E cb_get_log_server(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_LOG_SERVER_INFO_S pOut = (LPNET_TV_LOG_SERVER_INFO_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_LOG_SERVER, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    ::System::LogServerInfo_S stInfo;
    Convert::to_struct(dataJson, stInfo);
    TvSdkConvert::FillLogServerInfo(stInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_log_server(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_LOG_SERVER_INFO_S *pIn = (const NET_TV_LOG_SERVER_INFO_S *)lpInBuffer;
    ::System::LogServerInfo_S stInfo;
    TvSdkConvert::ToLogServerInfo(*pIn, stInfo);

    std::string inJson = wrap_data_json(Convert::to_string(stInfo));
    return (execute_action_expect_success(AC_SET_LOG_SERVER, inJson) == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_test_log_server(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_LOG_SERVER_INFO_S *pIn = (const NET_TV_LOG_SERVER_INFO_S *)lpInBuffer;
    ::System::LogServerInfo_S stInfo;
    TvSdkConvert::ToLogServerInfo(*pIn, stInfo);

    std::string inJson = wrap_data_json(Convert::to_string(stInfo));
    return (execute_action_expect_success(AC_TEST_LOG_SERVER, inJson) == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_control_record_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_RECORD_INFO_S *pIn = (const NET_TV_RECORD_INFO_S *)lpInBuffer;
    Record_NS::Info_S stInfo;
    TvSdkConvert::ToRecordInfo(*pIn, stInfo);

    std::string inJson = wrap_data_json(Convert::to_string(stInfo));
    return (execute_action_expect_success(AC_SET_HUMAN_RECORD, inJson) == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_record_status(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_RECORD_STATUS_INFO_S pOut = (LPNET_TV_RECORD_STATUS_INFO_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_RECORD_STATUS, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Record_NS::RecordStatusInfo_S stInfo;
    Convert::to_struct(dataJson, stInfo);
    TvSdkConvert::FillRecordStatusInfo(stInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_get_record_schedule(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_RECORD_SCHEDULE_S pOut = (LPNET_TV_RECORD_SCHEDULE_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_RECORD_SCHEDULE, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Record_NS::Schedule_S stInfo;
    Convert::to_struct(dataJson, stInfo);
    TvSdkConvert::FillRecordSchedule(stInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_record_schedule(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_RECORD_SCHEDULE_S *pIn = (const NET_TV_RECORD_SCHEDULE_S *)lpInBuffer;
    Record_NS::Schedule_S stInfo;
    TvSdkConvert::ToRecordSchedule(*pIn, stInfo);

    std::string inJson = wrap_data_json(Convert::to_string(stInfo));
    return (execute_action_expect_success(AC_SET_RECORD_SCHEDULE, inJson) == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_record_advanced_param(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_RECORD_ADVANCED_PARAM_S pOut = (LPNET_TV_RECORD_ADVANCED_PARAM_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_RECORD_ADVANCED_PARAM, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Record_NS::AdvancedParam_S stInfo;
    Convert::to_struct(dataJson, stInfo);
    TvSdkConvert::FillRecordAdvancedParam(stInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_record_advanced_param(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_RECORD_ADVANCED_PARAM_S *pIn = (const NET_TV_RECORD_ADVANCED_PARAM_S *)lpInBuffer;
    Record_NS::AdvancedParam_S stInfo;
    TvSdkConvert::ToRecordAdvancedParam(*pIn, stInfo);

    std::string inJson = wrap_data_json(Convert::to_string(stInfo));
    return (execute_action_expect_success(AC_SET_RECORD_ADVANCED_PARAM, inJson) == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_find_record_file_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_RECORD_FILE_LIST_S pOut = (LPNET_TV_RECORD_FILE_LIST_S)lpOutBuffer;
    Record_NS::Find_S stFind;
    TvSdkConvert::ToRecordFind(pOut->stFind, stFind);

    std::string outJson;
    std::string inJson = wrap_data_json(Convert::to_string(stFind));
    if (execute_get_result(AC_FIND_RECORD_FILE_INFO, inJson, outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    std::vector<Record_NS::FindResult_S> vecInfo;
    Convert::to_struct(dataJson, vecInfo);
    TvSdkConvert::FillRecordFileList(vecInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_download_record_file(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_RECORD_DOWNLOAD_LIST_S pIn = (LPNET_TV_RECORD_DOWNLOAD_LIST_S)lpInBuffer;
    std::vector<Record_NS::DownloadInfo_S> vecInfo;
    TvSdkConvert::ToRecordDownloadList(*pIn, vecInfo);

    std::string outJson;
    std::string inJson = wrap_data_json(Convert::to_string(vecInfo));
    if (execute_action_expect_success(AC_DOWNLOAD_RECORD_FILE, inJson, &outJson) != 0)
        return NET_TV_E_SET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (!dataJson.empty())
    {
        std::vector<Record_NS::DownloadProgress_S> vecProgress;
        Convert::to_struct(dataJson, vecProgress);
        TvSdkConvert::FillRecordDownloadListProgress(vecProgress, *pIn);
    }
    return NET_TV_E_SUCCEED;
}

static int cb_notice_download_record_progress_publish(const void *pData, int nLen, int nActionCode, void *pHandle)
{
    (void)pHandle;
    if (nActionCode != AC_NOTICE_DOWNLOAD_RECORD_PROGRESS || !pData || nLen <= 0)
        return 0;

    std::string outJson(static_cast<const char *>(pData), static_cast<size_t>(nLen));
    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        dataJson = outJson;

    Record_NS::DownloadProgress_S stProgress;
    Convert::to_struct(dataJson, stProgress);

    NET_TV_RECORD_DOWNLOAD_PROGRESS_S stTvProgress;
    TvSdkConvert::FillRecordDownloadProgress(stProgress, stTvProgress);

    NET_TV_ALARMER_S stAlarmer;
    std::memset(&stAlarmer, 0, sizeof(stAlarmer));
    BOOL bRet = NET_TV_SERVER_PushAlarmInfo(&stAlarmer,
                                            NET_TV_NOTICE_DOWNLOAD_RECORD_PROGRESS,
                                            &stTvProgress,
                                            (INT32)sizeof(stTvProgress));
    return bRet ? 0 : -1;
}

static NET_TV_COMMON_ECODE_E cb_get_preview_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_PREVIEW_INFO_S pOut = (LPNET_TV_PREVIEW_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PREVIEW_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Preview::PreviewInfo_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPreviewInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_preview_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_PREVIEW_INFO_S *pIn = (const NET_TV_PREVIEW_INFO_S *)lpInBuffer;

    Preview::PreviewInfo_S stCfg;
    TvSdkConvert::ToPreviewInfo(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PREVIEW_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_privacy_mask_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_NULL_POINT;

    LPNET_TV_PRIVACY_MASK_CFG_S pOut = (LPNET_TV_PRIVACY_MASK_CFG_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_COVER_CONFIG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string dataJson = normalize_data_json(outJson);
    if (dataJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Osd::CoverConfig_S stCoverConfig;
    Convert::to_struct(dataJson, stCoverConfig);
    TvSdkConvert::FillPrivacyMaskCfg(stCoverConfig, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_privacy_mask_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_PRIVACY_MASK_CFG_S *pIn = (const NET_TV_PRIVACY_MASK_CFG_S *)lpInBuffer;
    Osd::CoverConfig_S stCoverConfig;
    TvSdkConvert::ToPrivacyMaskCfg(*pIn, stCoverConfig);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCoverConfig));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_COVER_CONFIG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_tamper_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_TAMPER_ALARM_INFO_S pOut = (LPNET_TV_TAMPER_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AG_GET_HIDE_ALARM_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::HideAlarm_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillTamperAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_tamper_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_TAMPER_ALARM_INFO_S *pIn = (const NET_TV_TAMPER_ALARM_INFO_S *)lpInBuffer;
    Alarm::HideAlarm_S stCfg;
    TvSdkConvert::ToHideAlarm(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AG_SET_HIDE_ALARM_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
static NET_TV_COMMON_ECODE_E cb_get_motion_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_MOTION_ALARM_INFO_S pOut = (LPNET_TV_MOTION_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_MOTION_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::MotionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);

    // 使用全局中间变量填充 TVSDK 结构体，再 memcpy 到 SDK 缓冲区
    std::memset(&g_tvMotionAlarmInfo, 0, sizeof(g_tvMotionAlarmInfo));
    TvSdkConvert::FillMotionAlarmInfo(stCfg, g_tvMotionAlarmInfo);
    std::memcpy(pOut, &g_tvMotionAlarmInfo, sizeof(g_tvMotionAlarmInfo));
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_motion_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_MOTION_ALARM_INFO_S *pIn = (const NET_TV_MOTION_ALARM_INFO_S *)lpInBuffer;

    Alarm::MotionDetection_S stCfg;
    TvSdkConvert::ToMotionDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    dlog_debug("\ncb_set_motion_alarm :stInfo.data result:%s\n", stInfo.data.c_str());
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_MOTION_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_cross_line_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_CROSS_LINE_ALARM_INFO_S pOut = (LPNET_TV_CROSS_LINE_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_LINE_CROSSING_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::BoundaryDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillCrossLineAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_cross_line_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_CROSS_LINE_ALARM_INFO_S *pIn = (const NET_TV_CROSS_LINE_ALARM_INFO_S *)lpInBuffer;
    Alarm::BoundaryDetection_S stCfg;
    TvSdkConvert::ToBoundaryDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
     stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_LINE_CROSSING_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
static NET_TV_COMMON_ECODE_E cb_get_intrusion_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_INTRUSION_ALARM_INFO_S pOut = (LPNET_TV_INTRUSION_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_REGIONAL_INTRUSION_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::FieldDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillIntrusionAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_intrusion_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_INTRUSION_ALARM_INFO_S *pIn = (const NET_TV_INTRUSION_ALARM_INFO_S *)lpInBuffer;
    Alarm::FieldDetection_S stCfg;
    TvSdkConvert::ToFieldDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
     stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_REGIONAL_INTRUSION_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
/*-----------------------------------获取/设置徘徊侦测-------------------------------------*/
static NET_TV_COMMON_ECODE_E cb_get_loitering_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_LOITERING_ALARM_INFO_S pOut = (LPNET_TV_LOITERING_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_LOITERING_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::LoiteringDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillLoiteringAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_loitering_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_LOITERING_ALARM_INFO_S *pIn = (const NET_TV_LOITERING_ALARM_INFO_S *)lpInBuffer;
    Alarm::LoiteringDetection_S stCfg;
    TvSdkConvert::ToLoiteringDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
     stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_LOITERING_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* ---------- Get/SetSceneChangeAlarm：AC_GET/SET_SCENE_CHANGE_DETECT_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_scene_change_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_SCENE_CHANGE_ALARM_INFO_S pOut = (LPNET_TV_SCENE_CHANGE_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SCENE_CHANGE_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::SceneChange_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSceneChangeAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_scene_change_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_SCENE_CHANGE_ALARM_INFO_S *pIn = (const NET_TV_SCENE_CHANGE_ALARM_INFO_S *)lpInBuffer;

    Alarm::SceneChange_S stCfg;
    TvSdkConvert::ToSceneChange(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SCENE_CHANGE_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* ---------- Get/SetCrowdGatheringAlarm：AC_GET/SET_CROWD_GATHERING_DETECT_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_crowd_gathering_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_CROWD_GATHERING_ALARM_INFO_S pOut = (LPNET_TV_CROWD_GATHERING_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    // 添加日志1：检查 execute_get_result 返回
    int nExecResult = execute_get_result(AC_GET_CROWD_GATHERING_DETECT_INFO, "{}", outJson);
    dlog_info("DEBUG: execute_get_result={%d}, outJson.size()={%d}", nExecResult, outJson.size());

    if (nExecResult != 0 || outJson.empty())
    {
        dlog_error("DEBUG: execute_get_result failed, outJson={%s}", outJson.c_str());
        return NET_TV_E_GET_CFG_FAILED;
    }

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

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
    dlog_info("DEBUG: pOut address=%p, dwRuleCount=%u", (void*)pOut, pOut->dwRuleCount);
    dlog_info("DEBUG: pOut->dwRuleCount={%d}", pOut->dwRuleCount);

    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_crowd_gathering_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_CROWD_GATHERING_ALARM_INFO_S *pIn = (const NET_TV_CROWD_GATHERING_ALARM_INFO_S *)lpInBuffer;

    Alarm::CrowdGathering_S stCfg;
    TvSdkConvert::ToCrowdGathering(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CROWD_GATHERING_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}


#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
/* ---------- Get/SetGarbageExposureCfg：AC_GET/SET_GARBAGE_EXPOSURE_DETECT_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_garbage_exposure_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_GARBAGE_EXPOSURE_CFG_S pOut = (LPNET_TV_GARBAGE_EXPOSURE_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_GARBAGE_EXPOSURE_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;


    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::GarbageExposureDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillGarbageExposureCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_garbage_exposure_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_GARBAGE_EXPOSURE_CFG_S *pIn = (const NET_TV_GARBAGE_EXPOSURE_CFG_S *)lpInBuffer;

    Alarm::GarbageExposureDetection_S stCfg;
    TvSdkConvert::ToGarbageExposure(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_GARBAGE_EXPOSURE_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
/* ---------- Get/SetGarbageOverflowCfg：AC_GET/SET_GARBAGE_OVERFLOW_DETECT_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_garbage_overflow_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_GARBAGE_OVERFLOW_CFG_S pOut = (LPNET_TV_GARBAGE_OVERFLOW_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_GARBAGE_OVERFLOW_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::GarbageOverflowDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillGarbageOverflowCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_garbage_overflow_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_GARBAGE_OVERFLOW_CFG_S *pIn = (const NET_TV_GARBAGE_OVERFLOW_CFG_S *)lpInBuffer;

    Alarm::GarbageOverflowDetection_S stCfg;
    TvSdkConvert::ToGarbageOverflow(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_GARBAGE_OVERFLOW_CFG, stInfo) : -1;
   return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
#endif

#ifdef SCENE_INTELLIGENCE
static NET_TV_COMMON_ECODE_E cb_get_manhole_cover_abnormal_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_MANHOLE_COVER_ABNORMAL_CFG_S pOut = (LPNET_TV_MANHOLE_COVER_ABNORMAL_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_MANHOLE_COVER_ABNORMAL_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::ManholeCoverAbnormalDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillManholeCoverAbnormalCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_manhole_cover_abnormal_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S *pIn = (const NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S *)lpInBuffer;

    Alarm::ManholeCoverAbnormalDetection_S stCfg;
    TvSdkConvert::ToManholeCoverAbnormal(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_MANHOLE_COVER_ABNORMAL_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_sleep_on_duty_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_SLEEP_ON_DUTY_CFG_S pOut = (LPNET_TV_SLEEP_ON_DUTY_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SLEEP_ON_DUTY_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::SleepOnDutyDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSleepOnDutyCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_sleep_on_duty_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_SLEEP_ON_DUTY_CFG_S *pIn = (const NET_TV_SLEEP_ON_DUTY_CFG_S *)lpInBuffer;

    Alarm::SleepOnDutyDetection_S stCfg;
    TvSdkConvert::ToSleepOnDuty(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SLEEP_ON_DUTY_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_electric_vehicle_in_elevator_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S pOut = (LPNET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::ElectricScooterDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillElectricVehicleInElevatorCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_electric_vehicle_in_elevator_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S *pIn = (const NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S *)lpInBuffer;

    Alarm::ElectricScooterDetection_S stCfg;
    TvSdkConvert::ToElectricVehicleInElevator(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_person_fall_down_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_PERSON_FALL_DOWN_CFG_S pOut = (LPNET_TV_PERSON_FALL_DOWN_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PERSON_FALL_DOWN_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::PersonFallDownDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPersonFallDownCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_person_fall_down_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_PERSON_FALL_DOWN_CFG_S *pIn = (const NET_TV_PERSON_FALL_DOWN_CFG_S *)lpInBuffer;

    Alarm::PersonFallDownDetection_S stCfg;
    TvSdkConvert::ToPersonFallDown(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PERSON_FALL_DOWN_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_construction_occupy_road_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S pOut = (LPNET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_CONSTRUCTION_OCCUPY_ROAD_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::ConstructionEncroachmentRoadDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillConstructionOccupyRoadCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_construction_occupy_road_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S *pIn = (const NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S *)lpInBuffer;

    Alarm::ConstructionEncroachmentRoadDetection_S stCfg;
    TvSdkConvert::ToConstructionOccupyRoad(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CONSTRUCTION_OCCUPY_ROAD_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_congestion_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_CONGESTION_CFG_S pOut = (LPNET_TV_CONGESTION_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_CONGESTION_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::CongestionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillCongestionCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_congestion_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_CONGESTION_CFG_S *pIn = (const NET_TV_CONGESTION_CFG_S *)lpInBuffer;

    Alarm::CongestionDetection_S stCfg;
    TvSdkConvert::ToCongestion(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CONGESTION_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_license_plate_recognition_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_LICENSE_PLATE_RECOGNITION_CFG_S pOut = (LPNET_TV_LICENSE_PLATE_RECOGNITION_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_LICENSE_PLATE_RECOGNITION_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::LicensePlateCognitionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillLicensePlateRecognitionCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_license_plate_recognition_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S *pIn = (const NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S *)lpInBuffer;

    Alarm::LicensePlateCognitionDetection_S stCfg;
    TvSdkConvert::ToLicensePlateRecognition(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_LICENSE_PLATE_RECOGNITION_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_high_altitude_seatbelt_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S pOut = (LPNET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_HIGH_ALTITUDE_SEATBELT_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::HighAltitudeSeatbeltDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillHighAltitudeSeatbeltCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_high_altitude_seatbelt_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S *pIn = (const NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S *)lpInBuffer;

    Alarm::HighAltitudeSeatbeltDetection_S stCfg;
    TvSdkConvert::ToHighAltitudeSeatbelt(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_HIGH_ALTITUDE_SEATBELT_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_safety_helmet_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_SAFETY_HELMET_CFG_S pOut = (LPNET_TV_SAFETY_HELMET_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SAFETY_HELMET_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::SafetyHelmetDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSafetyHelmetCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_safety_helmet_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_SAFETY_HELMET_CFG_S *pIn = (const NET_TV_SAFETY_HELMET_CFG_S *)lpInBuffer;

    Alarm::SafetyHelmetDection_S stCfg;
    TvSdkConvert::ToSafetyHelmet(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SAFETY_HELMET_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_person_fall_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_PERSON_FALL_CFG_S pOut = (LPNET_TV_PERSON_FALL_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PERSON_FALL_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::TripDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPersonFallCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_person_fall_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_PERSON_FALL_CFG_S *pIn = (const NET_TV_PERSON_FALL_CFG_S *)lpInBuffer;

    Alarm::TripDetection_S stCfg;
    TvSdkConvert::ToPersonFall(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PERSON_FALL_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_phone_usage_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_PHONE_USAGE_CFG_S pOut = (LPNET_TV_PHONE_USAGE_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PHONE_USAGE_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::PhoneUsageDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPhoneUsageCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_phone_usage_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_PHONE_USAGE_CFG_S *pIn = (const NET_TV_PHONE_USAGE_CFG_S *)lpInBuffer;

    Alarm::PhoneUsageDetection_S stCfg;
    TvSdkConvert::ToPhoneUsage(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PHONE_USAGE_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_smoking_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_SMOKING_CFG_S pOut = (LPNET_TV_SMOKING_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SMOKING_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::SmokingDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSmokingCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_smoking_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_SMOKING_CFG_S *pIn = (const NET_TV_SMOKING_CFG_S *)lpInBuffer;

    Alarm::SmokingDection_S stCfg;
    TvSdkConvert::ToSmoking(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SMOKING_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_open_flame_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_OPEN_FLAME_CFG_S pOut = (LPNET_TV_OPEN_FLAME_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_OPEN_FLAME_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::OpenFlameDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillOpenFlameCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_open_flame_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_OPEN_FLAME_CFG_S *pIn = (const NET_TV_OPEN_FLAME_CFG_S *)lpInBuffer;

    Alarm::OpenFlameDetection_S stCfg;
    TvSdkConvert::ToOpenFlame(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_OPEN_FLAME_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_bare_soil_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_BARE_SOIL_CFG_S pOut = (LPNET_TV_BARE_SOIL_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_BARE_SOIL_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::BareSoiletDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillBareSoilCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_bare_soil_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_BARE_SOIL_CFG_S *pIn = (const NET_TV_BARE_SOIL_CFG_S *)lpInBuffer;

    Alarm::BareSoiletDection_S stCfg;
    TvSdkConvert::ToBareSoil(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_BARE_SOIL_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_hole_protection_bar_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_HOLE_PROTECTION_BAR_CFG_S pOut = (LPNET_TV_HOLE_PROTECTION_BAR_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_HOLE_PROTECTION_BAR_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::HoleProtectionBarDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillHoleProtectionBarCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_hole_protection_bar_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_HOLE_PROTECTION_BAR_CFG_S *pIn = (const NET_TV_HOLE_PROTECTION_BAR_CFG_S *)lpInBuffer;

    Alarm::HoleProtectionBarDection_S stCfg;
    TvSdkConvert::ToHoleProtectionBar(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_HOLE_PROTECTION_BAR_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_reflective_clothing_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_REFLECTIVE_CLOTHING_CFG_S pOut = (LPNET_TV_REFLECTIVE_CLOTHING_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_REFLECTIVE_CLOTHING_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::ReflectiveClothingDection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillReflectiveClothingCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_reflective_clothing_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_REFLECTIVE_CLOTHING_CFG_S *pIn = (const NET_TV_REFLECTIVE_CLOTHING_CFG_S *)lpInBuffer;

    Alarm::ReflectiveClothingDection_S stCfg;
    TvSdkConvert::ToReflectiveClothing(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_REFLECTIVE_CLOTHING_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_pet_recognition_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_PET_RECOGNITION_INFO_S pOut = (LPNET_TV_PET_RECOGNITION_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PET_RECOGNITION_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::PetRecognition_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPetRecognitionInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_pet_recognition_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_PET_RECOGNITION_INFO_S *pIn = (const NET_TV_PET_RECOGNITION_INFO_S *)lpInBuffer;

    Alarm::PetRecognition_S stCfg;
    TvSdkConvert::ToPetRecognition(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PET_RECOGNITION_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_climb_fence_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_CLIMB_FENCE_INFO_S pOut = (LPNET_TV_CLIMB_FENCE_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_CLIMB_FENCE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::FenceClimbingDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillClimbFenceInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_climb_fence_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_CLIMB_FENCE_INFO_S *pIn = (const NET_TV_CLIMB_FENCE_INFO_S *)lpInBuffer;

    Alarm::FenceClimbingDetection_S stCfg;
    TvSdkConvert::ToClimbFence(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CLIMB_FENCE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_dimission_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_DIMISSION_INFO_S pOut = (LPNET_TV_DIMISSION_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_DIMISSION_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::LeavePostDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillDimissionInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_dimission_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_DIMISSION_INFO_S *pIn = (const NET_TV_DIMISSION_INFO_S *)lpInBuffer;

    Alarm::LeavePostDetection_S stCfg;
    TvSdkConvert::ToDimission(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_DIMISSION_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_illegal_lane_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_ILLEGAL_LANE_INFO_S pOut = (LPNET_TV_ILLEGAL_LANE_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_ILLEGAL_LANE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::IllegalLaneChangeDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillIllegalLaneInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_illegal_lane_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_ILLEGAL_LANE_INFO_S *pIn = (const NET_TV_ILLEGAL_LANE_INFO_S *)lpInBuffer;

    Alarm::IllegalLaneChangeDetection_S stCfg;
    TvSdkConvert::ToIllegalLane(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_ILLEGAL_LANE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_retrograde_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_RETROGRADE_INFO_S pOut = (LPNET_TV_RETROGRADE_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_RETROGRADE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::DrivingAgainstTrafficDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillRetrogradeInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_retrograde_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_RETROGRADE_INFO_S *pIn = (const NET_TV_RETROGRADE_INFO_S *)lpInBuffer;

    Alarm::DrivingAgainstTrafficDetection_S stCfg;
    TvSdkConvert::ToRetrograde(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_RETROGRADE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_nonmotor_vehicle_intrusion_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S pOut = (LPNET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_NONMOROT_VEHIINTRU_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::NonMotorVehicleIntrusionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillNonmotorVehicleIntrusionInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_nonmotor_vehicle_intrusion_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S *pIn = (const NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S *)lpInBuffer;

    Alarm::NonMotorVehicleIntrusionDetection_S stCfg;
    TvSdkConvert::ToNonmotorVehicleIntrusion(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_NONMOROT_VEHIINTRU_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_occupation_emergency_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_OCCUPATION_EMERGENCY_INFO_S pOut = (LPNET_TV_OCCUPATION_EMERGENCY_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_OCCUPATION_EMERGENCY_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::EmergencyLaneOccupancyDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillOccupationEmergencyInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_occupation_emergency_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_OCCUPATION_EMERGENCY_INFO_S *pIn = (const NET_TV_OCCUPATION_EMERGENCY_INFO_S *)lpInBuffer;

    Alarm::EmergencyLaneOccupancyDetection_S stCfg;
    TvSdkConvert::ToOccupationEmergency(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_OCCUPATION_EMERGENCY_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_pedestrian_intrusion_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_PEDESTRIAN_INTRUSION_INFO_S pOut = (LPNET_TV_PEDESTRIAN_INTRUSION_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PEDESTRAN_INTRUSION_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::PedestrianIntrusionDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPedestrianIntrusionInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_pedestrian_intrusion_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_PEDESTRIAN_INTRUSION_INFO_S *pIn = (const NET_TV_PEDESTRIAN_INTRUSION_INFO_S *)lpInBuffer;

    Alarm::PedestrianIntrusionDetection_S stCfg;
    TvSdkConvert::ToPedestrianIntrusion(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PEDESTRAN_INTRUSION_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_smoke_fire_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_SMOKE_FIRE_CFG_S pOut = (LPNET_TV_SMOKE_FIRE_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_SMOKE_FIRE_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::SmokeFireDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillSmokeFireCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_smoke_fire_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_SMOKE_FIRE_CFG_S *pIn = (const NET_TV_SMOKE_FIRE_CFG_S *)lpInBuffer;

    Alarm::SmokeFireDetection_S stCfg;
    TvSdkConvert::ToSmokeFire(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SMOKE_FIRE_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_road_ponding_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_ROAD_PONDING_CFG_S pOut = (LPNET_TV_ROAD_PONDING_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_ROAD_PONDING_CFG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::RoadPondingDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillRoadPondingCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_road_ponding_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_ROAD_PONDING_CFG_S *pIn = (const NET_TV_ROAD_PONDING_CFG_S *)lpInBuffer;

    Alarm::RoadPondingDetection_S stCfg;
    TvSdkConvert::ToRoadPonding(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_ROAD_PONDING_CFG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
#endif

#if CAP_AI_PEOPLE_STATISTICS
/* ---------- Get/SetPeopleFlowStatisticsCfg：AC_GET/SET_PEOPLE_FLOW_STATISTICS_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_people_flow_statistics_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_PEOPLE_FLOW_STATISTICS_CFG_S pOut = (LPNET_TV_PEOPLE_FLOW_STATISTICS_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PEOPLE_FLOW_STATISTICS_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::PeopleFlowStatistics_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPeopleFlowStatisticsCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_people_flow_statistics_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S *pIn = (const NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S *)lpInBuffer;

    Alarm::PeopleFlowStatistics_S stCfg;
    TvSdkConvert::ToPeopleFlowStatistics(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PEOPLE_FLOW_STATISTICS_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_reset_people_flow_statistics(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    (void)lpInBuffer;
    Task::Info_S stInfo;
    stInfo.data = "{}";
    int nExec = s_taskManage ? s_taskManage->execute(AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* ---------- Get/SetPeopleDensityDetectionCfg：AC_GET/SET_PEOPLE_DENSITY_DETECTION_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_people_density_detection_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_PEOPLE_DENSITY_DETECTION_CFG_S pOut = (LPNET_TV_PEOPLE_DENSITY_DETECTION_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PEOPLE_DENSITY_DETECTION_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::PeopleDensityDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPeopleDensityDetectionCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_people_density_detection_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S *pIn = (const NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S *)lpInBuffer;

    Alarm::PeopleDensityDetection_S stCfg;
    TvSdkConvert::ToPeopleDensityDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PEOPLE_DENSITY_DETECTION_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

#endif

/* ---------- Get/SetParkingDetectAlarm：AC_GET/SET_PARKING_DETECT_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_parking_detect_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_PARKING_ALARM_INFO_S pOut = (LPNET_TV_PARKING_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_PARKING_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::ParkingDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillParkingDetectAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_parking_detect_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_PARKING_ALARM_INFO_S *pIn = (const NET_TV_PARKING_ALARM_INFO_S *)lpInBuffer;

    Alarm::ParkingDetection_S stCfg;
    TvSdkConvert::ToParkingDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_PARKING_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* ---------- Get/SetUnattendedObjectAlarm：AC_GET/SET_UNATTENDED_OBJECT_DETECT_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_unattended_object_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_UNATTENDED_OBJECT_ALARM_INFO_S pOut = (LPNET_TV_UNATTENDED_OBJECT_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_UNATTENDED_OBJECT_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::UnattendedObject_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillUnattendedObjectAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_unattended_object_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S *pIn = (const NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S *)lpInBuffer;

    Alarm::UnattendedObject_S stCfg;
    TvSdkConvert::ToUnattendedObject(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_UNATTENDED_OBJECT_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* ---------- Get/SetObjectRemovalAlarm：AC_GET/SET_OBJECT_REMOVAL_DETECT_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_object_removal_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_OBJECT_REMOVAL_ALARM_INFO_S pOut = (LPNET_TV_OBJECT_REMOVAL_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_OBJECT_REMOVAL_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::ObjectRemoval_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillObjectRemovalAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_object_removal_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_OBJECT_REMOVAL_ALARM_INFO_S *pIn = (const NET_TV_OBJECT_REMOVAL_ALARM_INFO_S *)lpInBuffer;

    Alarm::ObjectRemoval_S stCfg;
    TvSdkConvert::ToObjectRemoval(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_OBJECT_REMOVAL_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* ---------- Get/SetAudioAnomalyAlarm：AC_GET/SET_AUDIO_ANOMALY_DETECT_INFO ---------- */
static NET_TV_COMMON_ECODE_E cb_get_audio_anomaly_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_AUDIO_ANOMALY_ALARM_INFO_S pOut = (LPNET_TV_AUDIO_ANOMALY_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_AUDIO_ANOMALY_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::AudioAnomaly_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);

    std::memset(&g_tvAudioAnomalyAlarmInfo, 0, sizeof(g_tvAudioAnomalyAlarmInfo));
    TvSdkConvert::FillAudioAnomalyAlarmInfo(stCfg, g_tvAudioAnomalyAlarmInfo);
    std::memcpy(pOut, &g_tvAudioAnomalyAlarmInfo, sizeof(g_tvAudioAnomalyAlarmInfo));
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_audio_anomaly_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_AUDIO_ANOMALY_ALARM_INFO_S *pIn = (const NET_TV_AUDIO_ANOMALY_ALARM_INFO_S *)lpInBuffer;
    Alarm::AudioAnomaly_S stCfg;
    TvSdkConvert::ToAudioAnomaly(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_AUDIO_ANOMALY_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* ---------- GetRtspUrl：获取主/子码流 RTSP URL ---------- */
static NET_TV_COMMON_ECODE_E cb_get_rtsp_url(INT32 dwChannelID, LPNET_TV_RTSP_URL_INFO_S pInfo)
{
    if (!pInfo)
        return NET_TV_E_INVALID_PARAM;

    const int streamIndex = pInfo->dwStreamIndex; // 调用方指定需要的码流
    std::memset(pInfo, 0, sizeof(*pInfo));
    pInfo->dwChannel = dwChannelID;
    pInfo->dwStreamIndex = streamIndex;

    int rtspChn = -1;
    switch (streamIndex)
    {
    case NET_TV_LIVE_STREAM_INDEX_MAIN: rtspChn = RTSP_CHN_MAIN; break;
    case NET_TV_LIVE_STREAM_INDEX_AUX:  rtspChn = RTSP_CHN_SUB;  break;
    default: return NET_TV_E_INVALID_PARAM;
    }

    const char *pUrl = CRtspServer::instance()->getRtspUrl(rtspChn, false);
    if (!pUrl || pUrl[0] == '\0')
        return NET_TV_E_GET_CFG_FAILED;

    std::strncpy(pInfo->szRtspUrl, pUrl, sizeof(pInfo->szRtspUrl) - 1);
    pInfo->szRtspUrl[sizeof(pInfo->szRtspUrl) - 1] = '\0';
    return NET_TV_E_SUCCEED;
}

/* --------------------------- 获取升级状态 --------------------------- */

static NET_TV_COMMON_ECODE_E cb_get_upgrade_status(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_UPGRADE_STATUS_S pOut = (LPNET_TV_UPGRADE_STATUS_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_UPGRADE_STATUS, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    ::System::UpgradeStatus_S stCfg;
    const std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillUpgradeStatus(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

/* --------------------------- 设置升级包路径后进行升级 --------------------------- */

static NET_TV_COMMON_ECODE_E cb_set_upgrade(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    if (!s_taskManage)
        return NET_TV_E_SET_CFG_FAILED;

    const NET_TV_UPGRADE_INFO_S *pIn = (const NET_TV_UPGRADE_INFO_S *)lpInBuffer;
    ::System::UpgradeInfo_S stCfg;
    TvSdkConvert::ToUpgradeInfo(*pIn, stCfg);
    const std::string reqPath = stCfg.strUpgradePath;
    stCfg.strUpgradePath = normalize_upgrade_local_path(stCfg.strUpgradePath);
    if (reqPath.empty())
        dlog_info("[TVSDK][Upgrade] request path empty, use default dir=%s", kDefaultUpgradeDir);
    if (!is_valid_upgrade_path(stCfg.strUpgradePath))
    {
        dlog_warn("[TVSDK][Upgrade] invalid path");
        return NET_TV_E_INVALID_PARAM;
    }

    std::string wrappedJson = wrap_data_json(Convert::to_string(stCfg));
    dlog_info("[TVSDK][Upgrade] recv local-path=%s", stCfg.strUpgradePath.c_str());

    if (execute_action_expect_success(AC_SET_UPGRADE, wrappedJson, nullptr) != 0)
        return NET_TV_E_SET_CFG_FAILED;

    std::string checkOutJson;
    if (execute_action_expect_success(AC_CHECK_UPGRADE, "{}", &checkOutJson) != 0)
        return NET_TV_E_SET_CFG_FAILED;

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));

    int nExec = s_taskManage->execute(AC_DO_UPGRADE, stInfo);
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* --------------------------- 获取升级包版本 --------------------------- */

static NET_TV_COMMON_ECODE_E cb_get_upgrade_version(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_UPGRADE_VERSION_S pOut = (LPNET_TV_UPGRADE_VERSION_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_CHECK_UPGRADE, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    ::System::UpgradeVersion_S stCfg;
    const std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillUpgradeVersion(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

/* --------------------------- 获取抓图计划信息 --------------------------- */

static NET_TV_COMMON_ECODE_E cb_get_capture_plan_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_CAPTURE_PLAN_INFO_S pOut = (LPNET_TV_CAPTURE_PLAN_INFO_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_CAPTURE_PLAN_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Capture_NS::CapturePlan_S stCfg;
    const std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillCapturePlan(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

/* --------------------------- 设置抓图计划信息 --------------------------- */

static NET_TV_COMMON_ECODE_E cb_set_capture_plan_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_CAPTURE_PLAN_INFO_S *pIn = (const NET_TV_CAPTURE_PLAN_INFO_S *)lpInBuffer;
    Capture_NS::CapturePlan_S stCfg;
    TvSdkConvert::ToCapturePlan(*pIn, stCfg);

    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CAPTURE_PLAN_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* --------------------------- 获取抓图参数信息 --------------------------- */

static NET_TV_COMMON_ECODE_E cb_get_capture_param_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_CAPTURE_PARAM_INFO_S pOut = (LPNET_TV_CAPTURE_PARAM_INFO_S)lpOutBuffer;
    std::string outJson;
    if (execute_get_result(AC_GET_CAPTURE_PARAM_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Capture_NS::CaptureParam_S stCfg;
    const std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillCaptureParam(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

/* --------------------------- 设置抓图参数信息 --------------------------- */

static NET_TV_COMMON_ECODE_E cb_set_capture_param_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_CAPTURE_PARAM_INFO_S *pIn = (const NET_TV_CAPTURE_PARAM_INFO_S *)lpInBuffer;
    Capture_NS::CaptureParam_S stCfg;
    TvSdkConvert::ToCaptureParam(*pIn, stCfg);

    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_CAPTURE_PARAM_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/* --------------------------- 设置抓图参数信息 --------------------------- */

typedef struct _IspPayload_S_
{
    bool bAllScene;
    ISP::AllSceneParams_S stAllScene;
    ISP::SceneParams_S stScene;
} IspPayload_S;

/* --------------------------- ISP相关信息 --------------------------- */

static NET_TV_COMMON_ECODE_E cb_get_exposure_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_EXPOSURE_INFO_S pOut = (LPNET_TV_EXPOSURE_INFO_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_EXPOSURE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;
    ISP::ExposureAttr_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillExposureInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_exposure_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    
    const NET_TV_EXPOSURE_INFO_S *pIn = (const NET_TV_EXPOSURE_INFO_S *)lpInBuffer;
    ISP::ExposureAttr_S stCfg;
    TvSdkConvert::ToExposureAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_EXPOSURE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_daynight_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_DAYNIGHT_INFO_S pOut = (LPNET_TV_DAYNIGHT_INFO_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_DAY_NIGHT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;
    ISP::DayNightAttr_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillDayNightInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_daynight_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    
    const NET_TV_DAYNIGHT_INFO_S *pIn = (const NET_TV_DAYNIGHT_INFO_S *)lpInBuffer;
    ISP::DayNightAttr_S stCfg;
    TvSdkConvert::ToDayNightAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_DAY_NIGHT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_backlight_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_BACKLIGHT_INFO_S pOut = (LPNET_TV_BACKLIGHT_INFO_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_BACK_LIGHT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;
    ISP::BackLightArrt_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillBackLightInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_backlight_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    
    const NET_TV_BACKLIGHT_INFO_S *pIn = (const NET_TV_BACKLIGHT_INFO_S *)lpInBuffer;
    ISP::BackLightArrt_S stCfg;
    TvSdkConvert::ToBackLightAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_BACK_LIGHT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_denoise_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_DENOISE_INFO_S pOut = (LPNET_TV_DENOISE_INFO_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_NOISE_REMOVE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;
    ISP::DnrAttr_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillDenoiseInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_denoise_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    
    const NET_TV_DENOISE_INFO_S *pIn = (const NET_TV_DENOISE_INFO_S *)lpInBuffer;
    ISP::DnrAttr_S stCfg;
    TvSdkConvert::ToDnrAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_NOISE_REMOVE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_whitebalance_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_WHITEBALANCE_INFO_S pOut = (LPNET_TV_WHITEBALANCE_INFO_S)lpOutBuffer;
    std::string strJson;
    std::string outJson;
    if (execute_get_result(AC_GET_WHITE_BALANCE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    
    Json::get(outJson.c_str(), "Return", nRet);
    if(nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;
    ISP::AwbAttr_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillWhiteBalanceInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_whitebalance_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    
    const NET_TV_WHITEBALANCE_INFO_S *pIn = (const NET_TV_WHITEBALANCE_INFO_S *)lpInBuffer;
    ISP::AwbAttr_S stCfg;
    TvSdkConvert::ToAwbAttr(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_WHITE_BALANCE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_set_talkback_state(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_TALKBACK_STATE_INFO_S *pIn = (const NET_TV_TALKBACK_STATE_INFO_S *)lpInBuffer;
    Preview::IntercomInfo_S stCfg;
    TvSdkConvert::ToIntercomInfo(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_STATE_TALKBACK, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_set_talkback_to_stream(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_TALKBACK_STREAM_INFO_S *pIn = (const NET_TV_TALKBACK_STREAM_INFO_S *)lpInBuffer;
    Replay::Stream::Info_S stCfg;
    TvSdkConvert::ToReplayStreamInfo(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_TO_STREAM_TALKBACK, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_talkback_from_stream(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_TALKBACK_STREAM_INFO_S pOut = (LPNET_TV_TALKBACK_STREAM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_FROM_STREAM_TALKBACK, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Replay::Stream::Info_S stCfg;
    strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillTalkbackStreamInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_replay_talkback(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_REPLAY_TALKBACK_INFO_S *pIn = (const NET_TV_REPLAY_TALKBACK_INFO_S *)lpInBuffer;
    Replay::Stream::ReplayRtpInfo_S stCfg;
    TvSdkConvert::ToReplayRtpInfo(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_REPLAY_TALKBACK, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/*-----------------------------------获取/设置音频配置-------------------------------------*/
static NET_TV_COMMON_ECODE_E cb_get_audio_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_AUDIO_CFG_S pOut = (LPNET_TV_AUDIO_CFG_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_AUDIO_CONFIG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Audio_NS::AudioConfig_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillAudioCfg(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_audio_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_AUDIO_CFG_S *pIn = (const NET_TV_AUDIO_CFG_S *)lpInBuffer;
    Audio_NS::AudioConfig_S stCfg;
    TvSdkConvert::ToAudioConfig(*pIn, stCfg);

    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_AUDIO_CONFIG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/*-----------------------------------获取/设置进入区域侦测-------------------------------------*/
static NET_TV_COMMON_ECODE_E cb_get_enter_region_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_ENTER_REGION_ALARM_INFO_S pOut = (LPNET_TV_ENTER_REGION_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_ENTER_REGION_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::EntranceDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillEnterRegionAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_enter_region_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_ENTER_REGION_ALARM_INFO_S *pIn = (const NET_TV_ENTER_REGION_ALARM_INFO_S *)lpInBuffer;
    Alarm::EntranceDetection_S stCfg;
    TvSdkConvert::ToEntranceDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_ENTER_REGION_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
static NET_TV_COMMON_ECODE_E cb_get_leave_region_alarm(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_LEAVE_REGION_ALARM_INFO_S pOut = (LPNET_TV_LEAVE_REGION_ALARM_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_LEAVE_REGION_DETECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::ExitingDetection_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillLeaveRegionAlarmInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}
/*-----------------------------------获取/设置离开区域侦测-------------------------------------*/
static NET_TV_COMMON_ECODE_E cb_set_leave_region_alarm(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_LEAVE_REGION_ALARM_INFO_S *pIn = (const NET_TV_LEAVE_REGION_ALARM_INFO_S *)lpInBuffer;
    Alarm::ExitingDetection_S stCfg;
    TvSdkConvert::ToExitingDetection(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_LEAVE_REGION_DETECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_face_capture_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_FACE_CAPTURE_INFO_S pOut = (LPNET_TV_FACE_CAPTURE_INFO_S)lpOutBuffer;

    std::string outJson;
    std::string strJson;
    if (execute_get_result(AC_GET_FACE_CAPTURE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    Alarm::FaceCapture_S stCfg;
    strJson = normalize_data_json(outJson);
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillFaceCaptureInfo(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_face_capture_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_FACE_CAPTURE_INFO_S *pIn = (const NET_TV_FACE_CAPTURE_INFO_S *)lpInBuffer;
    Alarm::FaceCapture_S stCfg;
    TvSdkConvert::ToFaceCapture(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_FACE_CAPTURE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_set_face_compare_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_FACE_COMPARE_INFO_S *pIn = (const NET_TV_FACE_COMPARE_INFO_S *)lpInBuffer;
    Alarm::FaceCompare_S stCfg;
    TvSdkConvert::ToFaceCompare(*pIn, stCfg);
    std::string inJson = Convert::to_string(stCfg);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_FACE_COMPARE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_add_target_lib(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_FACE_LIB_INFO_S *pIn = (const NET_TV_FACE_LIB_INFO_S *)lpInBuffer;
    Event::FaceLibInfo_S stInfoIn;
    TvSdkConvert::ToFaceLibInfo(*pIn, stInfoIn);
    std::string inJson = Convert::to_string(stInfoIn);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_ADD_TARGET_LIB, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_del_target_lib(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_FACE_LIB_INFO_S *pIn = (const NET_TV_FACE_LIB_INFO_S *)lpInBuffer;
    Event::FaceLibInfo_S stInfoIn;
    TvSdkConvert::ToFaceLibInfo(*pIn, stInfoIn);
    std::string inJson = Convert::to_string(stInfoIn);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_DEL_TARGET_LIB, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_set_target_lib(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_FACE_LIB_INFO_S *pIn = (const NET_TV_FACE_LIB_INFO_S *)lpInBuffer;
    Event::FaceLibInfo_S stInfoIn;
    TvSdkConvert::ToFaceLibInfo(*pIn, stInfoIn);
    std::string inJson = Convert::to_string(stInfoIn);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_TARGET_LIB, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_target_lib(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_FACE_LIB_LIST_S pOut = (LPNET_TV_FACE_LIB_LIST_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_TARGET_LIB, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = 0;
    if (Json::get(outJson.c_str(), "Return", nRet) && nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        strJson = outJson;
    std::vector<Event::FaceLibInfo_S> vecInfo;
    Convert::to_struct(strJson, vecInfo);
    TvSdkConvert::FillFaceLibList(vecInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_add_face_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_FACE_INFO_S *pIn = (const NET_TV_FACE_INFO_S *)lpInBuffer;
    Event::FaceInfo_S stInfoIn;
    TvSdkConvert::ToFaceInfo(*pIn, stInfoIn);
    std::string inJson = Convert::to_string(stInfoIn);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_ADD_FACE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_del_face_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_FACE_ID_INFO_S *pIn = (const NET_TV_FACE_ID_INFO_S *)lpInBuffer;
    Event::FaceIdInfo_S stInfoIn;
    TvSdkConvert::ToFaceIdInfo(*pIn, stInfoIn);
    std::string inJson = Convert::to_string(stInfoIn);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_DEL_FACE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_set_face_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;
    const NET_TV_FACE_INFO_S *pIn = (const NET_TV_FACE_INFO_S *)lpInBuffer;
    Event::FaceInfo_S stInfoIn;
    TvSdkConvert::ToFaceInfo(*pIn, stInfoIn);
    std::string inJson = Convert::to_string(stInfoIn);
    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(inJson);
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_FACE_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

static NET_TV_COMMON_ECODE_E cb_get_face_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;
    LPNET_TV_FACE_INFO_LIST_S pOut = (LPNET_TV_FACE_INFO_LIST_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_FACE_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;
    int nRet = 0;
    if (Json::get(outJson.c_str(), "Return", nRet) && nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        strJson = outJson;
    std::vector<Event::FaceInfo_S> vecInfo;
    Convert::to_struct(strJson, vecInfo);
    TvSdkConvert::FillFaceInfoList(vecInfo, *pOut);
    return NET_TV_E_SUCCEED;
}

void register_all()
{
    NET_TV_SERVER_RegisterCb_GetDeviceInfo(
        reinterpret_cast<NET_TV_COMMON_ECODE_E (*)(NET_TV_DEVICE_INFO_S)>(cb_get_device_info_impl));
    NET_TV_SERVER_RegisterCb_GetVideoEncodeCap(cb_get_video_encode_cap);
    NET_TV_SERVER_RegisterCb_GetAudioEncodeCap(cb_get_audio_encode_cap);
    NET_TV_SERVER_RegisterCb_GetOsdCap(cb_get_osd_cap);
    NET_TV_SERVER_RegisterCb_GetOsdCapCfg(cb_get_osd_cap_cfg);
    NET_TV_SERVER_RegisterCb_GetDeviceCfg(cb_get_device_cfg);
    NET_TV_SERVER_RegisterCb_SetDeviceCfg(cb_set_device_cfg);
    NET_TV_SERVER_RegisterCb_GetNtpCfg(cb_get_ntp_cfg);
    NET_TV_SERVER_RegisterCb_SetNtpCfg(cb_set_ntp_cfg);
    NET_TV_SERVER_RegisterCb_GetStreamCfg(cb_get_stream_cfg);
    NET_TV_SERVER_RegisterCb_SetStreamCfg(cb_set_stream_cfg);
    NET_TV_SERVER_RegisterCb_GetRtspUrl(cb_get_rtsp_url);
    NET_TV_SERVER_RegisterCb_GetOsdCfg(cb_get_osd_cfg);
    NET_TV_SERVER_RegisterCb_SetOsdCfg(cb_set_osd_cfg);
    
    NET_TV_SERVER_RegisterCb_GetImageCfg(cb_get_image_cfg);
    NET_TV_SERVER_RegisterCb_SetImageCfg(cb_set_image_cfg);
    NET_TV_SERVER_RegisterCb_GetNetworkCfg(cb_get_network_cfg);
    NET_TV_SERVER_RegisterCb_SetNetworkCfg(cb_set_network_cfg);
    NET_TV_SERVER_RegisterCb_GetPrivacyMaskCfg(cb_get_privacy_mask_cfg);
    NET_TV_SERVER_RegisterCb_SetPrivacyMaskCfg(cb_set_privacy_mask_cfg);
    NET_TV_SERVER_RegisterCb_GetPreviewInfo(cb_get_preview_info);
    NET_TV_SERVER_RegisterCb_SetPreviewInfo(cb_set_preview_info);
    NET_TV_SERVER_RegisterCb_GetTamperAlarm(cb_get_tamper_alarm);
    NET_TV_SERVER_RegisterCb_SetTamperAlarm(cb_set_tamper_alarm);
    NET_TV_SERVER_RegisterCb_GetMotionAlarm(cb_get_motion_alarm);
    NET_TV_SERVER_RegisterCb_SetMotionAlarm(cb_set_motion_alarm);
    NET_TV_SERVER_RegisterCb_GetSceneChangeAlarm(cb_get_scene_change_alarm);
    NET_TV_SERVER_RegisterCb_SetSceneChangeAlarm(cb_set_scene_change_alarm);    
    NET_TV_SERVER_RegisterCb_GetCrowGatheringAlarm(cb_get_crowd_gathering_alarm);
    NET_TV_SERVER_RegisterCb_SetCrowGatheringAlarm(cb_set_crowd_gathering_alarm);
    NET_TV_SERVER_RegisterCb_GetCrossLineAlarm(cb_get_cross_line_alarm);
    NET_TV_SERVER_RegisterCb_SetCrossLineAlarm(cb_set_cross_line_alarm);

    NET_TV_SERVER_RegisterCb_SetConfigWifiSta(cb_set_config_wifi_sta);
    NET_TV_SERVER_RegisterCb_ConnectWifiSta(cb_connect_wifi_sta);
    NET_TV_SERVER_RegisterCb_DisconnectWifiSta(cb_disconnect_wifi_sta);
    NET_TV_SERVER_RegisterCb_SetHotspotInfo(cb_set_hotspot_info);
    NET_TV_SERVER_RegisterCb_GetHotspotConn(cb_get_hotspot_conn);
    NET_TV_SERVER_RegisterCb_Get4GInfo(cb_get_4g_info);
    NET_TV_SERVER_RegisterCb_Set4GInfo(cb_set_4g_info);
    NET_TV_SERVER_RegisterCb_GetSecurityServicesInfo(cb_get_security_services_info);
    NET_TV_SERVER_RegisterCb_SetSecurityServicesInfo(cb_set_security_services_info);
    NET_TV_SERVER_RegisterCb_GetSshCountdown(cb_get_ssh_countdown);
    NET_TV_SERVER_RegisterCb_FindLog(cb_find_log);
    NET_TV_SERVER_RegisterCb_ExportLog(cb_export_log);
    NET_TV_SERVER_RegisterCb_GetLogServer(cb_get_log_server);
    NET_TV_SERVER_RegisterCb_SetLogServer(cb_set_log_server);
    NET_TV_SERVER_RegisterCb_TestLogServer(cb_test_log_server);
    NET_TV_SERVER_RegisterCb_ControlRecordInfo(cb_control_record_info);
    NET_TV_SERVER_RegisterCb_GetRecordStatus(cb_get_record_status);
    NET_TV_SERVER_RegisterCb_GetRecordSchedule(cb_get_record_schedule);
    NET_TV_SERVER_RegisterCb_SetRecordSchedule(cb_set_record_schedule);
    NET_TV_SERVER_RegisterCb_GetRecordAdvancedParam(cb_get_record_advanced_param);
    NET_TV_SERVER_RegisterCb_SetRecordAdvancedParam(cb_set_record_advanced_param);
    NET_TV_SERVER_RegisterCb_FindRecordFileInfo(cb_find_record_file_info);
    NET_TV_SERVER_RegisterCb_DownloadRecordFile(cb_download_record_file);
    if (s_taskManage)
    {
        s_taskManage->register_subscribe(AC_NOTICE_DOWNLOAD_RECORD_PROGRESS, cb_notice_download_record_progress_publish);
    }

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    NET_TV_SERVER_RegisterCb_GetGarbageExposureCfg(cb_get_garbage_exposure_cfg);
    NET_TV_SERVER_RegisterCb_SetGarbageExposureCfg(cb_set_garbage_exposure_cfg);
    NET_TV_SERVER_RegisterCb_GetGarbageOverflowCfg(cb_get_garbage_overflow_cfg);
    NET_TV_SERVER_RegisterCb_SetGarbageOverflowCfg(cb_set_garbage_overflow_cfg);
#endif

#ifdef SCENE_INTELLIGENCE
    NET_TV_SERVER_RegisterCb_GetManholeCoverAbnormalCfg(cb_get_manhole_cover_abnormal_cfg);
    NET_TV_SERVER_RegisterCb_SetManholeCoverAbnormalCfg(cb_set_manhole_cover_abnormal_cfg);
    NET_TV_SERVER_RegisterCb_GetSleepOnDutyCfg(cb_get_sleep_on_duty_cfg);
    NET_TV_SERVER_RegisterCb_SetSleepOnDutyCfg(cb_set_sleep_on_duty_cfg);
    NET_TV_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg(cb_get_electric_vehicle_in_elevator_cfg);
    NET_TV_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg(cb_set_electric_vehicle_in_elevator_cfg);
    NET_TV_SERVER_RegisterCb_GetPersonFallDownCfg(cb_get_person_fall_down_cfg);
    NET_TV_SERVER_RegisterCb_SetPersonFallDownCfg(cb_set_person_fall_down_cfg);
    NET_TV_SERVER_RegisterCb_GetConstructionOccupyRoadCfg(cb_get_construction_occupy_road_cfg);
    NET_TV_SERVER_RegisterCb_SetConstructionOccupyRoadCfg(cb_set_construction_occupy_road_cfg);
    NET_TV_SERVER_RegisterCb_GetCongestionCfg(cb_get_congestion_cfg);
    NET_TV_SERVER_RegisterCb_SetCongestionCfg(cb_set_congestion_cfg);
    NET_TV_SERVER_RegisterCb_GetLicensePlateRecognitionCfg(cb_get_license_plate_recognition_cfg);
    NET_TV_SERVER_RegisterCb_SetLicensePlateRecognitionCfg(cb_set_license_plate_recognition_cfg);
    NET_TV_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg(cb_get_high_altitude_seatbelt_cfg);
    NET_TV_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg(cb_set_high_altitude_seatbelt_cfg);
    NET_TV_SERVER_RegisterCb_GetSafetyHelmetCfg(cb_get_safety_helmet_cfg);
    NET_TV_SERVER_RegisterCb_SetSafetyHelmetCfg(cb_set_safety_helmet_cfg);
    NET_TV_SERVER_RegisterCb_GetPersonFallCfg(cb_get_person_fall_cfg);
    NET_TV_SERVER_RegisterCb_SetPersonFallCfg(cb_set_person_fall_cfg);
    NET_TV_SERVER_RegisterCb_GetPhoneUsageCfg(cb_get_phone_usage_cfg);
    NET_TV_SERVER_RegisterCb_SetPhoneUsageCfg(cb_set_phone_usage_cfg);
    NET_TV_SERVER_RegisterCb_GetSmokingCfg(cb_get_smoking_cfg);
    NET_TV_SERVER_RegisterCb_SetSmokingCfg(cb_set_smoking_cfg);
    NET_TV_SERVER_RegisterCb_GetOpenFlameCfg(cb_get_open_flame_cfg);
    NET_TV_SERVER_RegisterCb_SetOpenFlameCfg(cb_set_open_flame_cfg);
    NET_TV_SERVER_RegisterCb_GetBareSoilCfg(cb_get_bare_soil_cfg);
    NET_TV_SERVER_RegisterCb_SetBareSoilCfg(cb_set_bare_soil_cfg);
    NET_TV_SERVER_RegisterCb_GetHoleProtectionBarCfg(cb_get_hole_protection_bar_cfg);
    NET_TV_SERVER_RegisterCb_SetHoleProtectionBarCfg(cb_set_hole_protection_bar_cfg);
    NET_TV_SERVER_RegisterCb_GetReflectiveClothingCfg(cb_get_reflective_clothing_cfg);
    NET_TV_SERVER_RegisterCb_SetReflectiveClothingCfg(cb_set_reflective_clothing_cfg);
    NET_TV_SERVER_RegisterCb_GetPetRecognitionInfo(cb_get_pet_recognition_info);
    NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo(cb_set_pet_recognition_info);
    NET_TV_SERVER_RegisterCb_GetClimbFenceInfo(cb_get_climb_fence_info);
    NET_TV_SERVER_RegisterCb_SetClimbFenceInfo(cb_set_climb_fence_info);
    NET_TV_SERVER_RegisterCb_GetDimissionInfo(cb_get_dimission_info);
    NET_TV_SERVER_RegisterCb_SetDimissionInfo(cb_set_dimission_info);
    NET_TV_SERVER_RegisterCb_GetIllegalLaneInfo(cb_get_illegal_lane_info);
    NET_TV_SERVER_RegisterCb_SetIllegalLaneInfo(cb_set_illegal_lane_info);
    NET_TV_SERVER_RegisterCb_GetRetrogradeInfo(cb_get_retrograde_info);
    NET_TV_SERVER_RegisterCb_SetRetrogradeInfo(cb_set_retrograde_info);
    NET_TV_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo(cb_get_nonmotor_vehicle_intrusion_info);
    NET_TV_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo(cb_set_nonmotor_vehicle_intrusion_info);
    NET_TV_SERVER_RegisterCb_GetOccupationEmergencyInfo(cb_get_occupation_emergency_info);
    NET_TV_SERVER_RegisterCb_SetOccupationEmergencyInfo(cb_set_occupation_emergency_info);
    NET_TV_SERVER_RegisterCb_GetPedestrianIntrusionInfo(cb_get_pedestrian_intrusion_info);
    NET_TV_SERVER_RegisterCb_SetPedestrianIntrusionInfo(cb_set_pedestrian_intrusion_info);
    NET_TV_SERVER_RegisterCb_GetSmokeFireCfg(cb_get_smoke_fire_cfg);
    NET_TV_SERVER_RegisterCb_SetSmokeFireCfg(cb_set_smoke_fire_cfg);
    NET_TV_SERVER_RegisterCb_GetRoadPondingCfg(cb_get_road_ponding_cfg);
    NET_TV_SERVER_RegisterCb_SetRoadPondingCfg(cb_set_road_ponding_cfg);
#endif

#if CAP_AI_PEOPLE_STATISTICS
    NET_TV_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg(cb_get_people_flow_statistics_cfg);
    NET_TV_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg(cb_set_people_flow_statistics_cfg);
    NET_TV_SERVER_RegisterCb_ResetPeopleFlowStatistics(cb_reset_people_flow_statistics);
    NET_TV_SERVER_RegisterCb_GetPeopleDensityDetectionCfg(cb_get_people_density_detection_cfg);
    NET_TV_SERVER_RegisterCb_SetPeopleDensityDetectionCfg(cb_set_people_density_detection_cfg);
#endif

    NET_TV_SERVER_RegisterCb_GetIntrusionAlarm(cb_get_intrusion_alarm);
    NET_TV_SERVER_RegisterCb_SetIntrusionAlarm(cb_set_intrusion_alarm);
    NET_TV_SERVER_RegisterCb_GetLoiteringAlarm(cb_get_loitering_alarm);
    NET_TV_SERVER_RegisterCb_SetLoiteringAlarm(cb_set_loitering_alarm);
    NET_TV_SERVER_RegisterCb_GetAudioAnomalyAlarm(cb_get_audio_anomaly_alarm);
    NET_TV_SERVER_RegisterCb_SetAudioAnomalyAlarm(cb_set_audio_anomaly_alarm);
    NET_TV_SERVER_RegisterCb_GetParkingAlarm(cb_get_parking_detect_alarm);
    NET_TV_SERVER_RegisterCb_SetParkingAlarm(cb_set_parking_detect_alarm);
    NET_TV_SERVER_RegisterCb_GetUnattendedObjectAlarm(cb_get_unattended_object_alarm);
    NET_TV_SERVER_RegisterCb_SetUnattendedObjectAlarm(cb_set_unattended_object_alarm);
    NET_TV_SERVER_RegisterCb_GetObjectRemovalAlarm(cb_get_object_removal_alarm);
    NET_TV_SERVER_RegisterCb_SetObjectRemovalAlarm(cb_set_object_removal_alarm);

    NET_TV_SERVER_RegisterCb_GetUpgradeStatus(cb_get_upgrade_status);
    NET_TV_SERVER_RegisterCb_SetUpgrade(cb_set_upgrade);
    NET_TV_SERVER_RegisterCb_GetUpgradeVersion(cb_get_upgrade_version);

    NET_TV_SERVER_RegisterCb_GetCapturePlanInfo(cb_get_capture_plan_info);
    NET_TV_SERVER_RegisterCb_SetCapturePlanInfo(cb_set_capture_plan_info);
    NET_TV_SERVER_RegisterCb_GetCaptureParamInfo(cb_get_capture_param_info);
    NET_TV_SERVER_RegisterCb_SetCaptureParamInfo(cb_set_capture_param_info);

    NET_TV_SERVER_RegisterCb_GetExposureInfo(cb_get_exposure_info);
    NET_TV_SERVER_RegisterCb_SetExposureInfo(cb_set_exposure_info);
    NET_TV_SERVER_RegisterCb_GetDayNightInfo(cb_get_daynight_info);
    NET_TV_SERVER_RegisterCb_SetDayNightInfo(cb_set_daynight_info);
    NET_TV_SERVER_RegisterCb_GetBackLightInfo(cb_get_backlight_info);
    NET_TV_SERVER_RegisterCb_SetBackLightInfo(cb_set_backlight_info);
    NET_TV_SERVER_RegisterCb_GetDenoiseInfo(cb_get_denoise_info);
    NET_TV_SERVER_RegisterCb_SetDenoiseInfo(cb_set_denoise_info);
    NET_TV_SERVER_RegisterCb_GetWhiteBalanceInfo(cb_get_whitebalance_info);
    NET_TV_SERVER_RegisterCb_SetWhiteBalanceInfo(cb_set_whitebalance_info);
    
    NET_TV_SERVER_RegisterCb_SetTalkbackState(cb_set_talkback_state);
    NET_TV_SERVER_RegisterCb_SetTalkbackToStream(cb_set_talkback_to_stream);
    NET_TV_SERVER_RegisterCb_GetTalkbackFromStream(cb_get_talkback_from_stream);
    NET_TV_SERVER_RegisterCb_SetReplayTalkback(cb_set_replay_talkback);

    NET_TV_SERVER_RegisterCb_GetAudioCfg(cb_get_audio_cfg);
    NET_TV_SERVER_RegisterCb_SetAudioCfg(cb_set_audio_cfg);
    NET_TV_SERVER_RegisterCb_GetEnterRegionAlarm(cb_get_enter_region_alarm);
    NET_TV_SERVER_RegisterCb_SetEnterRegionAlarm(cb_set_enter_region_alarm);
    NET_TV_SERVER_RegisterCb_GetLeaveRegionAlarm(cb_get_leave_region_alarm);
    NET_TV_SERVER_RegisterCb_SetLeaveRegionAlarm(cb_set_leave_region_alarm);

    NET_TV_SERVER_RegisterCb_GetFaceCaptureInfo(cb_get_face_capture_info);
    NET_TV_SERVER_RegisterCb_SetFaceCaptureInfo(cb_set_face_capture_info);
    NET_TV_SERVER_RegisterCb_SetFaceCompareInfo(cb_set_face_compare_info);
    NET_TV_SERVER_RegisterCb_AddTargetLib(cb_add_target_lib);
    NET_TV_SERVER_RegisterCb_DelTargetLib(cb_del_target_lib);
    NET_TV_SERVER_RegisterCb_SetTargetLib(cb_set_target_lib);
    NET_TV_SERVER_RegisterCb_GetTargetLib(cb_get_target_lib);
    NET_TV_SERVER_RegisterCb_AddFaceInfo(cb_add_face_info);
    NET_TV_SERVER_RegisterCb_DelFaceInfo(cb_del_face_info);
    NET_TV_SERVER_RegisterCb_SetFaceInfo(cb_set_face_info);
    NET_TV_SERVER_RegisterCb_GetFaceInfo(cb_get_face_info);
}

} // namespace TvSdkCallbacks

