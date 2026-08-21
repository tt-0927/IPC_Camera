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
#include <memory>
#include <mutex>

#include "task_manage.h"
#include "task.h"
#include "dlog.h"
#include "action_code.h"
#include "system_manage.h"
#include "system_define.h"
#include "network_define.h"
#include "alarm_define.h"
#include "preview_define.h"
#include "osd_manage.h"
#include "preview_manage.h"
#include "Json.h"
#include "convert_interface.h"
#include "video_define.h"
#include "path_define.h"

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
    return nId == NET_TV_LIVE_STREAM_INDEX_MAIN || nId == NET_TV_LIVE_STREAM_INDEX_AUX;
}

static bool is_valid_video_type(INT32 enVideoType)
{
    return enVideoType == static_cast<INT32>(Video_NS::VideoType_E::COMPOSITE_STREAM) ||
           enVideoType == static_cast<INT32>(Video_NS::VideoType_E::VIDEO_STREAM);
}

static bool is_valid_video_codec(INT32 enVideoCodec)
{
    return enVideoCodec >= NET_TV_VIDEO_CODE_H264 && enVideoCodec <= NET_TV_VIDEO_CODE_MPEG4;
}

static NET_TV_COMMON_ECODE_E validate_set_stream_cfg(const NET_TV_VIDEO_ENCODE_OPTION_S &cfg)
{
    if (!is_valid_live_stream_id(cfg.nId))
    {
        dlog_warn("TVSDK设置视频编码参数失败: 非法码流ID[%d]", cfg.nId);
        return NET_TV_E_INVALID_PARAM;
    }

    if (!is_valid_video_type(cfg.enVideoType))
    {
        dlog_warn("TVSDK设置视频编码参数失败: 非法视频类型[%d], 仅支持0-复合流/1-视频流", cfg.enVideoType);
        return NET_TV_E_INVALID_PARAM;
    }

    if (!is_valid_video_codec(cfg.enVideoCodec))
    {
        dlog_warn("TVSDK设置视频编码参数失败: 非法视频编码[%d]", cfg.enVideoCodec);
        return NET_TV_E_INVALID_PARAM;
    }

    return NET_TV_E_SUCCEED;
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

/* ---------- DeviceControl: TVSDK 声光控制 IPC 控制任务 ---------- */
static NET_TV_COMMON_ECODE_E cb_device_control(LPNET_TV_DEVICE_CONTROL_INFO_S pCtrlInfo)
{
    if (!pCtrlInfo)
    {
        return NET_TV_E_NULL_POINT;
    }

    if (pCtrlInfo->dwSize != sizeof(NET_TV_DEVICE_CONTROL_TYPE_E) || pCtrlInfo->dwChannelID <= 0 || pCtrlInfo->dwDurationMs < 0 || pCtrlInfo->dwDurationMs > 3000000)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    if (pCtrlInfo->dwControlType != NET_TV_DEVICE_CTRL_TYPE_ALARM_LIGHT)
    {
        return NET_TV_E_NOT_SUPPORT;
    }

    if (pCtrlInfo->dwCommand != NET_TV_ALARM_LIGHT_CTRL_START && pCtrlInfo->dwCommand != NET_TV_ALARM_LIGHT_CTRL_STOP && pCtrlInfo->dwCommand != NET_TV_ALARM_LIGHT_CTRL_SET_MODE)
    {
        return NET_TV_E_INVALID_PARAM;
    }

#if !CAP_ALARM_IO
    return NET_TV_E_NOT_SUPPORT;
#else
    ::Preview::DeviceControl_S stControl;
    stControl.nChannelId = pCtrlInfo->dwChannelID - 1;
    stControl.nControlType = pCtrlInfo->dwControlType;
    stControl.nCommand = pCtrlInfo->dwCommand;
    stControl.nDurationMs = pCtrlInfo->dwDurationMs;
    stControl.nParam1 = pCtrlInfo->dwParam1;
    stControl.nParam2 = pCtrlInfo->dwParam2;
    stControl.strExt.assign(pCtrlInfo->szExt, strnlen(pCtrlInfo->szExt, sizeof(pCtrlInfo->szExt)));

    std::string outJson;
    if(execute_get_result(AC_DEVICE_CONTROL, wrap_data_json(Convert::to_string(stControl)), outJson) != 0)
    {
        return NET_TV_E_SET_CFG_FAILED;
    }

    int nRet = ERR;
    if(outJson.empty() || !Json::get(outJson.c_str(), "Return", nRet) || nRet != OK)
    {
        dlog_warn("TVSDK声光控制任务失败： return[%d], body[%s]", nRet, outJson.c_str());
        return NET_TV_E_SET_CFG_FAILED;
    }

    return NET_TV_E_SUCCEED;
#endif
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

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Video_NS::VideoCapabilitySet_S stCapSet;
    Convert::to_struct(strJson, stCapSet);
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

    Audio_NS::AudioCapabilitySet_S stCapSet;
    Convert::to_struct(outJson, stCapSet);
    TvSdkConvert::FillAudioEncodeCap(stCapSet, *pCap);

    return NET_TV_E_SUCCEED;
}

/* ---------- GetOsdCap：OSD能力集 ---------- */
static NET_TV_COMMON_ECODE_E cb_get_osd_cap(INT32 dwChannelID, LPNET_TV_OSD_CAP_S pCap)
{
    (void)dwChannelID;
    if (!pCap)
        return NET_TV_E_NULL_POINT;
    memset(pCap, 0, sizeof(NET_TV_OSD_CAP_S));

    pCap->bSupportOsd = TRUE;
    pCap->bSupportName = TRUE;
    pCap->bSupportTime = TRUE;
    pCap->bSupportWeek = TRUE;
    pCap->bSupportCustomColor = TRUE;
    pCap->udwMaxOsdNum = 4;

    pCap->udwSupportedFontSizeNum = 4;
    pCap->audwSupportedFontSizeList[0] = NET_TV_OSD_FONT_SIZE_ADAPTIVE;
    pCap->audwSupportedFontSizeList[1] = NET_TV_OSD_FONT_SIZE_16;
    pCap->audwSupportedFontSizeList[2] = NET_TV_OSD_FONT_SIZE_32;
    pCap->audwSupportedFontSizeList[3] = NET_TV_OSD_FONT_SIZE_48;

    pCap->udwSupportedDateFormatNum = 9;
    pCap->audwSupportedDateFormatList[0] = NET_TV_OSD_DATE_YYYY_MM_DD;
    pCap->audwSupportedDateFormatList[1] = NET_TV_OSD_DATE_MM_DD_YYYY;
    pCap->audwSupportedDateFormatList[2] = NET_TV_OSD_DATE_DD_MM_YYYY;
    pCap->audwSupportedDateFormatList[3] = NET_TV_OSD_DATE_YYYY_MM_DD_CHN;
    pCap->audwSupportedDateFormatList[4] = NET_TV_OSD_DATE_MM_DD_YYYY_CHN;
    pCap->audwSupportedDateFormatList[5] = NET_TV_OSD_DATE_DD_MM_YYYY_CHN;
    pCap->audwSupportedDateFormatList[6] = NET_TV_OSD_DATE_YYYY_MM_DD_SLASH;
    pCap->audwSupportedDateFormatList[7] = NET_TV_OSD_DATE_MM_DD_YYYY_SLASH;
    pCap->audwSupportedDateFormatList[8] = NET_TV_OSD_DATE_DD_MM_YYYY_SLASH;

    pCap->udwSupportedTimeFormatNum = 2;
    pCap->audwSupportedTimeFormatList[0] = NET_TV_OSD_TIME_FORMAT_24;
    pCap->audwSupportedTimeFormatList[1] = NET_TV_OSD_TIME_FORMAT_12;

    pCap->udwSupportedAlignNum = 6;
    pCap->audwSupportedAlignList[0] = NET_TV_OSD_ALIGN_CUSTOMIZE;
    pCap->audwSupportedAlignList[1] = NET_TV_OSD_ALIGN_CHAR_LEFT;
    pCap->audwSupportedAlignList[2] = NET_TV_OSD_ALIGN_CHAR_RIGHT;
    pCap->audwSupportedAlignList[3] = NET_TV_OSD_ALIGN_ALL_LEFT;
    pCap->audwSupportedAlignList[4] = NET_TV_OSD_ALIGN_ALL_RIGHT;
    pCap->audwSupportedAlignList[5] = NET_TV_OSD_ALIGN_GB_MODE;

    return NET_TV_E_SUCCEED;
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
        const INT32 nSectionCount = stSchedule.dwTimeSectionCount[nDay];
        if (nSectionCount < 0 || nSectionCount > NET_TV_PLAN_SECTION_NUM)
        {
            return false;
        }

        for (INT32 nSection = 0; nSection < nSectionCount; ++nSection)
        {
            const NET_TV_SCHED_TIME_S& stTime = stSchedule.astTimeSection[nDay][nSection];
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
    if (stLinkageList.dwAlarmOutputCount < 0 ||
        stLinkageList.dwAlarmOutputCount > NET_TV_MAX_ALARM_OUT_NUM ||
        stLinkageList.dwRecordChannelCount < 0 ||
        stLinkageList.dwRecordChannelCount > NET_TV_CHANNEL_MAX ||
        stLinkageList.dwSnapshotChannelCount < 0 ||
        stLinkageList.dwSnapshotChannelCount > NET_TV_CHANNEL_MAX ||
        stLinkageList.dwTraditionalLinkageCount < 0 ||
        stLinkageList.dwTraditionalLinkageCount > NET_TV_TRADITIONAL_LINKAGE_MAX_NUM)
    {
        return false;
    }

    for (INT32 nIndex = 0; nIndex < stLinkageList.dwTraditionalLinkageCount; ++nIndex)
    {
        const INT32 nLinkageType = stLinkageList.adwTraditionalLinkage[nIndex];
        if (nLinkageType < NET_TV_TRADITIONAL_LINKAGE_SEND_EMAIL ||
            nLinkageType > NET_TV_TRADITIONAL_LINKAGE_UPLOAD_TARGET_IMAGE)
        {
            return false;
        }
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
    return stInfo.nAlarmNumber >= 0 && stInfo.nAlarmNumber < NET_TV_MAX_ALARM_IN_NUM &&
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
    return stInfo.nAlarmNumber >= 0 && stInfo.nAlarmNumber < NET_TV_MAX_ALARM_OUT_NUM &&
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
 * @return 成功返回 NET_TV_E_SUCCEED；否则返回配置获取失败码。
 */
static NET_TV_COMMON_ECODE_E cb_get_sd_card_status(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    pNET_SdCardStatus_S pSdCardStatus = static_cast<pNET_SdCardStatus_S>(pOutBuffer);
    *pSdCardStatus = {};

    std::string strOutJson;
    if (execute_get_result(AC_GET_SD_CARD_STATUS, "{}", strOutJson) != 0 || strOutJson.empty())
    {
        return NET_TV_E_GET_CFG_FAILED;
    }

    int nTaskResult = ERR;
    if (!Json::get(strOutJson.c_str(), "Return", nTaskResult) || nTaskResult != OK)
    {
        return NET_TV_E_GET_CFG_FAILED;
    }

    const std::string strDataJson = normalize_data_json(strOutJson);
    if (strDataJson.empty())
    {
        return NET_TV_E_GET_CFG_FAILED;
    }

    Json::Object *pDataJson = Json::init(strDataJson.c_str());
    if (!pDataJson)
    {
        return NET_TV_E_GET_CFG_FAILED;
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
        return NET_TV_E_GET_CFG_FAILED;
    }

    pSdCardStatus->nStatus = nStatus;
    pSdCardStatus->bReady = bReady ? TRUE : FALSE;
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
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_NULL_POINT;

    std::string outJson;
    if (execute_get_result(AC_GET_TIME_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    ::System::TimeInfo_S stTimeInfo;
    Convert::to_struct(strJson, stTimeInfo);
    TvSdkConvert::FillSystemNtpInfo(stTimeInfo, *(LPNET_TV_SYSTEM_NTP_INFO_S)lpOutBuffer);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_ntp_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_NULL_POINT;
    if (!s_taskManage)
        return NET_TV_E_SET_CFG_FAILED;

    const NET_TV_SYSTEM_NTP_INFO_S *pIn = (const NET_TV_SYSTEM_NTP_INFO_S *)lpInBuffer;
    ::System::TimeInfo_S stTimeInfo;
    TvSdkConvert::ToTimeInfo(*pIn, stTimeInfo);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stTimeInfo));
    int nRet = s_taskManage->execute(AC_SET_TIME_INFO, stInfo);
    return (nRet == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
static NET_TV_COMMON_ECODE_E cb_get_stream_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_VIDEO_ENCODE_OPTION_S pOut = (LPNET_TV_VIDEO_ENCODE_OPTION_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_VIDEO_CONFIG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    std::vector<Video_NS::VideoConfig_S> vecCfg;
    Convert::to_struct(strJson, vecCfg);

    const Video_NS::VideoConfig_S *pSelectedCfg = FindVideoConfigById(vecCfg, NET_TV_LIVE_STREAM_INDEX_MAIN);
    if (!pSelectedCfg)
        return NET_TV_E_GET_CFG_FAILED;

    TvSdkConvert::FillVideoEncodeOption(*pSelectedCfg, *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_stream_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_VIDEO_ENCODE_OPTION_S *pIn = (const NET_TV_VIDEO_ENCODE_OPTION_S *)lpInBuffer;
    NET_TV_COMMON_ECODE_E nValid = validate_set_stream_cfg(*pIn);
    if (nValid != NET_TV_E_SUCCEED)
        return nValid;

    Video_NS::VideoConfig_S stCfg;
    TvSdkConvert::ToVideoConfig(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_VIDEO_CONFIG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
static NET_TV_COMMON_ECODE_E cb_get_osd_cap_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_VIDEO_OSD_CFG_S pOut = (LPNET_TV_VIDEO_OSD_CFG_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_OSD_CONFIG, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Osd::OsdConfig_S stCfg;
    stCfg.clear();
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillOsdConfig(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}

static NET_TV_COMMON_ECODE_E cb_set_osd_cap_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_VIDEO_OSD_CFG_S *pIn = (const NET_TV_VIDEO_OSD_CFG_S *)lpInBuffer;
    Osd::OsdConfig_S stCfg;
    TvSdkConvert::ToOsdConfig(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_OSD_CONFIG, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}
static NET_TV_COMMON_ECODE_E cb_get_image_cfg(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;
    if (!lpOutBuffer)
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_IMAGE_SETTING_S pOut = (LPNET_TV_IMAGE_SETTING_S)lpOutBuffer;
    std::memset(pOut, 0, sizeof(*pOut));

    std::string outJson;
    if (execute_get_result(AC_GET_VIDEO_EFFECT_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    ISP::ImageParam_S stCfg;
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillImageSetting(stCfg, *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_image_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_IMAGE_SETTING_S *pIn = (const NET_TV_IMAGE_SETTING_S *)lpInBuffer;
    ISP::ImageParam_S stCfg;
    TvSdkConvert::ToImageParam(*pIn, stCfg);

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_VIDEO_EFFECT_INFO, stInfo) : -1;
    return (nExec == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
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
        return NET_TV_E_INVALID_PARAM;

    LPNET_TV_PRIVACY_MASK_CFG_S pOut = (LPNET_TV_PRIVACY_MASK_CFG_S)lpOutBuffer;

    std::string outJson;
    if (execute_get_result(AC_GET_SHELTER_INFO, "{}", outJson) != 0 || outJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    int nRet = -1;
    Json::get(outJson.c_str(), "Return", nRet);
    if (nRet != 0)
        return NET_TV_E_GET_CFG_FAILED;

    std::string strJson = normalize_data_json(outJson);
    if (strJson.empty())
        return NET_TV_E_GET_CFG_FAILED;

    Osd::CoverConfig_S stCfg;
    stCfg.clear();
    Convert::to_struct(strJson, stCfg);
    TvSdkConvert::FillPrivacyMaskCfg(stCfg, COsdManage::instance()->get_cover_max_area_count(), *pOut);
    return NET_TV_E_SUCCEED;
}
static NET_TV_COMMON_ECODE_E cb_set_privacy_mask_cfg(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;
    if (!lpInBuffer)
        return NET_TV_E_INVALID_PARAM;

    const NET_TV_PRIVACY_MASK_CFG_S *pIn = (const NET_TV_PRIVACY_MASK_CFG_S *)lpInBuffer;
    Osd::CoverConfig_S stCfg;
    const size_t maxAreaCount = COsdManage::instance()->get_cover_max_area_count();
    if (!TvSdkConvert::ToPrivacyMaskCfg(*pIn, maxAreaCount, stCfg))
    {
        dlog_warn("TVSDK隐私遮盖区域数非法, request:%d, max:%zu", pIn->uAreaCount, maxAreaCount);
        return NET_TV_E_INVALID_PARAM;
    }

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(Convert::to_string(stCfg));
    int nExec = s_taskManage ? s_taskManage->execute(AC_SET_SHELTER_INFO, stInfo) : -1;
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
#endif

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

#ifdef SCENE_INTELLIGENCE
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

/**
 * @brief 执行 IPC 获取命令并提取告警配置数据。
 * @author ITC
 * @param [in] nActionCode IPC 获取命令号。
 * @param [out] strDataJson 获取成功后返回的 Data JSON 字符串。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回获取配置失败码。
 */
static NET_TV_COMMON_ECODE_E get_alarm_config_data(INT32 nActionCode, std::string& strDataJson)
{
    std::string strResultJson;
    if (execute_get_result(nActionCode, "{}", strResultJson) != 0 || strResultJson.empty())
    {
        return NET_TV_E_GET_CFG_FAILED;
    }

    INT32 nTaskResult = ERR;
    Json::get(strResultJson.c_str(), "Return", nTaskResult);
    if (nTaskResult != OK)
    {
        return NET_TV_E_GET_CFG_FAILED;
    }

    strDataJson = normalize_data_json(strResultJson);
    return strDataJson.empty() ? NET_TV_E_GET_CFG_FAILED : NET_TV_E_SUCCEED;
}

/**
 * @brief 执行 IPC 设置命令以保存告警配置数据。
 * @author ITC
 * @param [in] nActionCode IPC 设置命令号。
 * @param [in] strDataJson 待设置的 Data JSON 字符串。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回设置配置失败码。
 */
static NET_TV_COMMON_ECODE_E set_alarm_config_data(INT32 nActionCode, const std::string& strDataJson)
{
    if (!s_taskManage || strDataJson.empty())
    {
        return NET_TV_E_SET_CFG_FAILED;
    }

    Task::Info_S stInfo;
    stInfo.data = wrap_data_json(strDataJson);
    const INT32 nExecuteResult = s_taskManage->execute(nActionCode, stInfo);
    return nExecuteResult == OK ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

/**
 * @brief 获取 IPC 的声音告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与查询。
 * @param [out] pOutBuffer 用于接收 NET_AudibleAlarmInfo_S 配置的缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_get_audible_alarm_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    pNET_AudibleAlarmInfo_S pOutput = static_cast<pNET_AudibleAlarmInfo_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_TV_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_AUDIBLE_ALARM_INFO, strDataJson);
    if (enResult != NET_TV_E_SUCCEED)
    {
        return enResult;
    }

    Alarm::SoundOutputAlarm_S stAlarmInfo;
    Convert::to_struct(strDataJson, stAlarmInfo);
    TvSdkConvert::FillAudibleAlarmInfo(stAlarmInfo, *pOutput);
    return NET_TV_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的声音告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_AudibleAlarmInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_set_audible_alarm_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    const pNET_AudibleAlarmInfo_S pInput = static_cast<pNET_AudibleAlarmInfo_S>(pInBuffer);
    if (!is_valid_audible_alarm_info(*pInput))
    {
        return NET_TV_E_INVALID_PARAM;
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
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_get_alarm_input_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    pNET_AlarmInputInfoList_S pOutput = static_cast<pNET_AlarmInputInfoList_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_TV_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_ALARM_INPUT_INFO, strDataJson);
    if (enResult != NET_TV_E_SUCCEED)
    {
        return enResult;
    }

    std::set<Alarm::IoInputInfo_S> stAlarmInputs;
    Convert::to_struct(strDataJson, stAlarmInputs);
    TvSdkConvert::FillAlarmInputInfoList(stAlarmInputs, *pOutput);
    return NET_TV_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的一路报警输入配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_AlarmInputInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_set_alarm_input_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    const pNET_AlarmInputInfo_S pInput = static_cast<pNET_AlarmInputInfo_S>(pInBuffer);
    if (!is_valid_alarm_input_info(*pInput))
    {
        return NET_TV_E_INVALID_PARAM;
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
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_get_alarm_output_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    pNET_AlarmOutputInfoList_S pOutput = static_cast<pNET_AlarmOutputInfoList_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_TV_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_ALARM_OUTPUT_INFO, strDataJson);
    if (enResult != NET_TV_E_SUCCEED)
    {
        return enResult;
    }

    std::set<Alarm::IoOutputInfo_S> stAlarmOutputs;
    Convert::to_struct(strDataJson, stAlarmOutputs);
    TvSdkConvert::FillAlarmOutputInfoList(stAlarmOutputs, *pOutput);
    return NET_TV_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的一路报警输出配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_AlarmOutputInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_set_alarm_output_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    const pNET_AlarmOutputInfo_S pInput = static_cast<pNET_AlarmOutputInfo_S>(pInBuffer);
    if (!is_valid_alarm_output_info(*pInput))
    {
        return NET_TV_E_INVALID_PARAM;
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
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_get_flashing_light_alarm_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    pNET_FlashingLightAlarmInfo_S pOutput = static_cast<pNET_FlashingLightAlarmInfo_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_TV_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_FLASHING_LIGHT_ALARM_INFO, strDataJson);
    if (enResult != NET_TV_E_SUCCEED)
    {
        return enResult;
    }

    Alarm::FlashInfo_S stAlarmInfo;
    Convert::to_struct(strDataJson, stAlarmInfo);
    TvSdkConvert::FillFlashingLightAlarmInfo(stAlarmInfo, *pOutput);
    return NET_TV_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的闪光灯告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_FlashingLightAlarmInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_set_flashing_light_alarm_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    const pNET_FlashingLightAlarmInfo_S pInput = static_cast<pNET_FlashingLightAlarmInfo_S>(pInBuffer);
    if (!is_valid_flashing_light_alarm_info(*pInput))
    {
        return NET_TV_E_INVALID_PARAM;
    }

    Alarm::FlashInfo_S stAlarmInfo;
    TvSdkConvert::ToFlashingLightAlarm(*pInput, stAlarmInfo);
    std::string strDataJson = Convert::to_string(stAlarmInfo);
    return set_alarm_config_data(AC_SET_FLASHING_LIGHT_ALARM_INFO, strDataJson);
}

/**
 * @brief 触发 IPC 的手动声光报警联动事件。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，当前 IPC 为单通道设备，不参与事件处理。
 * @param [in] pInBuffer 指向 NET_SoundLightAlarmTrigger_S 的输入缓冲区。
 * @param [out] 无。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_trigger_sound_light_alarm(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    const pNET_SoundLightAlarmTrigger_S pInput = static_cast<pNET_SoundLightAlarmTrigger_S>(pInBuffer);
    if (!is_valid_alarm_linkage(pInput->stLinkageList))
    {
        return NET_TV_E_INVALID_PARAM;
    }

    Alarm::LinkageList_S stLinkageList;
    TvSdkConvert::ToLinkageList(pInput->stLinkageList, stLinkageList);
    return set_alarm_config_data(AC_TRIGGER_SOUND_LIGHT_ALARM, Convert::to_string(stLinkageList));
}

/**
 * @brief 获取 IPC 的 PIR 告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与查询。
 * @param [out] pOutBuffer 用于接收 NET_PirAlarmInfo_S 配置的缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_get_pir_alarm_info(INT32 nChannelId, LPVOID pOutBuffer)
{
    (void)nChannelId;
    if (!pOutBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    pNET_PirAlarmInfo_S pOutput = static_cast<pNET_PirAlarmInfo_S>(pOutBuffer);
    std::memset(pOutput, 0, sizeof(*pOutput));
    std::string strDataJson;
    const NET_TV_COMMON_ECODE_E enResult = get_alarm_config_data(AC_GET_PIR_ALARM_INFO, strDataJson);
    if (enResult != NET_TV_E_SUCCEED)
    {
        return enResult;
    }

    Alarm::PirAlarmInfo_S stAlarmInfo;
    Convert::to_struct(strDataJson, stAlarmInfo);
    TvSdkConvert::FillPirAlarmInfo(stAlarmInfo, *pOutput);
    return NET_TV_E_SUCCEED;
}

/**
 * @brief 设置 IPC 的 PIR 告警配置。
 * @author ITC
 * @param [in] nChannelId 设备通道标识，本配置为设备级配置，不参与设置。
 * @param [in] pInBuffer 指向 NET_PirAlarmInfo_S 配置的输入缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED，否则返回相应错误码。
 */
static NET_TV_COMMON_ECODE_E cb_set_pir_alarm_info(INT32 nChannelId, LPVOID pInBuffer)
{
    (void)nChannelId;
    if (!pInBuffer)
    {
        return NET_TV_E_NULL_POINT;
    }

    const pNET_PirAlarmInfo_S pInput = static_cast<pNET_PirAlarmInfo_S>(pInBuffer);
    if (!is_valid_pir_alarm_info(*pInput))
    {
        return NET_TV_E_INVALID_PARAM;
    }

    Alarm::PirAlarmInfo_S stAlarmInfo;
    TvSdkConvert::ToPirAlarmInfo(*pInput, stAlarmInfo);
    std::string strDataJson = Convert::to_string(stAlarmInfo);
    return set_alarm_config_data(AC_SET_PIR_ALARM_INFO, strDataJson);
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


/**
 * @brief 获取 IPC 人脸抓拍图片叠加配置。
 * @param [in] dwChannelID 通道编号。
 * @param [out] lpOutBuffer SDK 人脸抓拍叠加配置输出缓冲区。
 * @return NET_TV_E_SUCCEED 表示成功，其他值表示获取失败或参数异常。
 */
static NET_TV_COMMON_ECODE_E cb_get_face_capture_overlay_info(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    std::string strJson;
    std::string strResultJson;
    Alarm::OverlayInfo_S stConfig;
    INT32 nReturn = -1;
    LPNET_TV_FACE_CAPTURE_OVERLAY_INFO_S pOut = nullptr;

    (void)dwChannelID;
    if (!lpOutBuffer)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    pOut = static_cast<LPNET_TV_FACE_CAPTURE_OVERLAY_INFO_S>(lpOutBuffer);
    if (execute_get_result(AC_GET_FACE_CAPTURE_OVERLAY_INFO_INFO, "{}", strResultJson) != 0 ||
        strResultJson.empty())
    {
        return NET_TV_E_GET_CFG_FAILED;
    }

    Json::get(strResultJson.c_str(), "Return", nReturn);
    if (nReturn != 0)
    {
        return NET_TV_E_GET_CFG_FAILED;
    }

    strJson = normalize_data_json(strResultJson);
    Convert::to_struct(strJson, stConfig);
    TvSdkConvert::FillFaceCaptureOverlayInfo(stConfig, *pOut);
    return NET_TV_E_SUCCEED;
}

/**
 * @brief 设置 IPC 人脸抓拍图片叠加配置。
 * @param [in] dwChannelID 通道编号。
 * @param [in] lpInBuffer SDK 人脸抓拍叠加配置输入缓冲区。
 * @return NET_TV_E_SUCCEED 表示成功，其他值表示设置失败或参数异常。
 */
static NET_TV_COMMON_ECODE_E cb_set_face_capture_overlay_info(INT32 dwChannelID, LPVOID lpInBuffer)
{
    const NET_TV_FACE_CAPTURE_OVERLAY_INFO_S* pIn = nullptr;
    Alarm::OverlayInfo_S stConfig;
    Task::Info_S stInfo = {};
    std::string strJson;
    INT32 nExecute = -1;

    (void)dwChannelID;
    if (!lpInBuffer)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    pIn = static_cast<const NET_TV_FACE_CAPTURE_OVERLAY_INFO_S*>(lpInBuffer);
    TvSdkConvert::ToFaceCaptureOverlayInfo(*pIn, stConfig);
    strJson = Convert::to_string(stConfig);
    stInfo.data = wrap_data_json(strJson);
    nExecute = s_taskManage ? s_taskManage->execute(AC_SET_FACE_CAPTURE_OVERLAY_INFO_INFO, stInfo) : -1;
    return (nExecute == 0) ? NET_TV_E_SUCCEED : NET_TV_E_SET_CFG_FAILED;
}

void register_all()
{
    NET_TV_SERVER_RegisterCb_GetDeviceInfo(
        reinterpret_cast<NET_TV_COMMON_ECODE_E (*)(NET_TV_DEVICE_INFO_S)>(cb_get_device_info_impl));
    NET_TV_SERVER_RegisterCb_DeviceControl(cb_device_control);
    NET_TV_SERVER_RegisterCb_GetVideoEncodeCap(cb_get_video_encode_cap);
    NET_TV_SERVER_RegisterCb_GetAudioEncodeCap(cb_get_audio_encode_cap);
    NET_TV_SERVER_RegisterCb_GetOsdCap(cb_get_osd_cap);
    NET_TV_SERVER_RegisterCb_GetDeviceCfg(cb_get_device_cfg);
    NET_TV_SERVER_RegisterCb_SetDeviceCfg(cb_set_device_cfg);
    NET_TV_SERVER_RegisterCb_GetNtpCfg(cb_get_ntp_cfg);
    NET_TV_SERVER_RegisterCb_SetNtpCfg(cb_set_ntp_cfg);
    NET_TV_SERVER_RegisterCb_GetSdCardStatus(cb_get_sd_card_status);
    NET_TV_SERVER_RegisterCb_GetStreamCfg(cb_get_stream_cfg);
    NET_TV_SERVER_RegisterCb_SetStreamCfg(cb_set_stream_cfg);
    NET_TV_SERVER_RegisterCb_GetRtspUrl(cb_get_rtsp_url);
    NET_TV_SERVER_RegisterCb_GetOsdCapCfg(cb_get_osd_cap_cfg);
    NET_TV_SERVER_RegisterCb_SetOsdCapCfg(cb_set_osd_cap_cfg);

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
#endif

    NET_TV_SERVER_RegisterCb_GetPetRecognitionInfo(cb_get_pet_recognition_info);
    NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo(cb_set_pet_recognition_info);
#ifdef SCENE_INTELLIGENCE
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

    NET_TV_SERVER_RegisterCb_GetAudibleAlarmInfo(cb_get_audible_alarm_info);
    NET_TV_SERVER_RegisterCb_SetAudibleAlarmInfo(cb_set_audible_alarm_info);
    NET_TV_SERVER_RegisterCb_GetAlarmInputInfo(cb_get_alarm_input_info);
    NET_TV_SERVER_RegisterCb_SetAlarmInputInfo(cb_set_alarm_input_info);
    NET_TV_SERVER_RegisterCb_GetAlarmOutputInfo(cb_get_alarm_output_info);
    NET_TV_SERVER_RegisterCb_SetAlarmOutputInfo(cb_set_alarm_output_info);
    NET_TV_SERVER_RegisterCb_TriggerSoundLightAlarm(cb_trigger_sound_light_alarm);
    NET_TV_SERVER_RegisterCb_GetFlashingLightAlarmInfo(cb_get_flashing_light_alarm_info);
    NET_TV_SERVER_RegisterCb_SetFlashingLightAlarmInfo(cb_set_flashing_light_alarm_info);
    NET_TV_SERVER_RegisterCb_GetPirAlarmInfo(cb_get_pir_alarm_info);
    NET_TV_SERVER_RegisterCb_SetPirAlarmInfo(cb_set_pir_alarm_info);

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
    NET_TV_SERVER_RegisterCb_GetFaceCaptureOverlayInfo(cb_get_face_capture_overlay_info);
    NET_TV_SERVER_RegisterCb_SetFaceCaptureOverlayInfo(cb_set_face_capture_overlay_info);
}

} // namespace TvSdkCallbacks

