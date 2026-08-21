/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : DeviceConfigBusiness.cpp
 * @Description  : 设备配置业务处理实现，负责处理设备配置的获取和设置请求
 */

#include "DeviceConfigBusiness.h"
#include "NetTVSDKHttpUrl.h"

#include <cctype>
#include <cstdlib>
#include <memory>

/**
 * 处理获取通道信息配置
 * @param channelId 通道号
 * @param command 命令码
 * @return JSON格式的响应数据
 */
static std::string HandleGetChannelInfo(INT32 channelId, INT32 command)
{
    NET_TV_CHANNEL_INFO_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    NSDK_LOG_INFO("GetChannelInfo callback START");
    int nRespCode = NetSDK_ExecuteCb_GetDevConfig(channelId, command, &stCfg);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NSDK_LOG_WARN("GetChannelInfo callback failed, cmd=%d, ret=%d", command, nRespCode);
    }
    NSDK_LOG_INFO("GetChannelInfo callback cmd=%d, ret=%d", command, nRespCode);
    NSDK_LOG_INFO("GetChannelInfo callback END");
    return SDKConvert::to_respString(nRespCode, stCfg);
}

/**
 * 处理获取通道列表配置
 * @param channelId 通道号
 * @param command 命令码
 * @return JSON格式的响应数据
 */
static std::string HandleGetChannelList(INT32 channelId, INT32 command)
{
    auto stCfg = std::make_unique<NET_TV_CHANNEL_LIST_S>();
    if (!stCfg)
    {
        NSDK_LOG_WARN("GetChannelList callback alloc failed");
        return SDKConvert::to_respString(NET_TV_E_FAILED);
    }

    memset(stCfg.get(), 0, sizeof(NET_TV_CHANNEL_LIST_S));

    NSDK_LOG_INFO("GetChannelList callback START");
    NSDK_LOG_INFO("[SDK] channel list cfg address=%p, sizeof=%zu",
                  (void*)stCfg.get(), sizeof(NET_TV_CHANNEL_LIST_S));

    int nRespCode = NetSDK_ExecuteCb_GetDevConfig(channelId, command, stCfg.get());
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NSDK_LOG_WARN("GetChannelList callback failed, cmd=%d, ret=%d", command, nRespCode);
    }
    NSDK_LOG_INFO("GetChannelList callback cmd=%d, ret=%d", command, nRespCode);
    NSDK_LOG_INFO("GetChannelList callback END");
    return SDKConvert::to_respString(nRespCode, *stCfg);
}

/**
 * 获取日志列表
 * @details 从URL参数中解析查询条件（类型、动作、时间范围、页码等），调用SDK获取日志列表
 * @param channelId 通道号
 * @param command 命令码
 * @param url_param URL参数
 * @return JSON格式的响应数据
 */
std::string CDeviceConfigBusiness::HandleGetLogList(INT32 channelId, INT32 command, const std::string& url_param)
{
    NET_TV_LOG_LIST_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    stCfg.stCond.nType = ParseIntParam(url_param, "Type", 0);
    stCfg.stCond.nAction = ParseIntParam(url_param, "Action", 0);

    std::string strStartTime = ParseStringParam(url_param, "StartTime");
    std::string strEndTime = ParseStringParam(url_param, "EndTime");
    strncpy(stCfg.stCond.szStartTime, strStartTime.c_str(), sizeof(stCfg.stCond.szStartTime) - 1);
    strncpy(stCfg.stCond.szEndTime, strEndTime.c_str(), sizeof(stCfg.stCond.szEndTime) - 1);

    stCfg.stPage.nCurPage = ParseIntParam(url_param, "CurPage", 1);
    stCfg.stPage.nPageSize = ParseIntParam(url_param, "PageSize", NET_TV_LOG_QUERY_COND_NUM);
    if (stCfg.stPage.nCurPage == 0)
    {
        stCfg.stPage.nCurPage = 1;
    }
    if (stCfg.stPage.nPageSize <= 0)
    {
        stCfg.stPage.nPageSize = NET_TV_LOG_QUERY_COND_NUM;
    }

    NSDK_LOG_INFO("GetLogList callback START");
    int nRespCode = NetSDK_ExecuteCb_GetDevConfig(channelId, command, &stCfg);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NSDK_LOG_WARN("GetLogList callback failed, cmd=%d, ret=%d", command, nRespCode);
    }
    NSDK_LOG_INFO("GetLogList callback cmd=%d, ret=%d", command, nRespCode);
    NSDK_LOG_INFO("GetLogList callback END");
    return SDKConvert::to_respString(nRespCode, stCfg);
}

/**
 * 获取录像文件列表
 * @details 从URL参数中解析查询条件（通道号、类型、日期、时间范围、文件名等），调用SDK获取录像文件列表
 * @param channelId 通道号
 * @param command 命令码
 * @param url_param URL参数
 * @return JSON格式的响应数据
 */
std::string CDeviceConfigBusiness::HandleGetRecordFileList(INT32 channelId, INT32 command, const std::string& url_param)
{
    NET_TV_RECORD_FILE_LIST_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    stCfg.stFind.nChnId = ParseIntParam(url_param, "ChnId", channelId);
    stCfg.stFind.nType = ParseIntParam(url_param, "Type", 0);

    std::string strYear = ParseStringParam(url_param, "Year");
    std::string strMonth = ParseStringParam(url_param, "Month");
    std::string strDate = ParseStringParam(url_param, "Date");
    std::string strStartTime = ParseStringParam(url_param, "StartTime");
    std::string strEndTime = ParseStringParam(url_param, "EndTime");
    std::string strFilename = ParseStringParam(url_param, "Filename");
    strncpy(stCfg.stFind.szYear, strYear.c_str(), sizeof(stCfg.stFind.szYear) - 1);
    strncpy(stCfg.stFind.szMonth, strMonth.c_str(), sizeof(stCfg.stFind.szMonth) - 1);
    strncpy(stCfg.stFind.szDate, strDate.c_str(), sizeof(stCfg.stFind.szDate) - 1);
    strncpy(stCfg.stFind.szStartTime, strStartTime.c_str(), sizeof(stCfg.stFind.szStartTime) - 1);
    strncpy(stCfg.stFind.szEndTime, strEndTime.c_str(), sizeof(stCfg.stFind.szEndTime) - 1);
    strncpy(stCfg.stFind.szFilename, strFilename.c_str(), sizeof(stCfg.stFind.szFilename) - 1);

    NSDK_LOG_INFO("GetRecordFileList callback START");
    int nRespCode = NetSDK_ExecuteCb_GetDevConfig(channelId, command, &stCfg);
    if (nRespCode != NET_TV_E_SUCCEED)
    {
        NSDK_LOG_WARN("GetRecordFileList callback failed, cmd=%d, ret=%d", command, nRespCode);
    }
    NSDK_LOG_INFO("GetRecordFileList callback cmd=%d, ret=%d", command, nRespCode);
    NSDK_LOG_INFO("GetRecordFileList callback END");
    return SDKConvert::to_respString(nRespCode, stCfg);
}

/**
 * 打印移动侦测配置信息到控制台
 * @details 输出移动侦测配置的详细信息，包括启用状态、动态分析、模式、灵敏度、区域类型和区域矩形等
 * @param pInfo 移动侦测配置信息指针
 */
static void PrintMotionAlarmInfo(const NET_TV_MOTION_ALARM_INFO_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    printf("\n[Client] ===== 移动侦测配置 =====\n");
    printf("  Enable              : %s\n", pInfo->bEnable ? "ON" : "OFF");
    printf("  DynamicAnalysis     : %s\n", pInfo->bDynamicAnalysisEnable ? "ON" : "OFF");
    printf("  Mode                : %s\n", pInfo->dwMode == NET_TV_MOTION_MODE_NORMAL ? "普通模式" : "专家模式");
    if (pInfo->dwMode == NET_TV_MOTION_MODE_NORMAL)
    {
        printf("  Sensitivity         : %d\n", pInfo->stNormalMode.nSensitivity);
        printf("  RegionType          : %s\n", pInfo->stNormalMode.nRegionType == 0 ? "筒型" : "网格");
        if (pInfo->stNormalMode.nRegionType == 0)
        {
            printf("  Rect               : [%d,%d,%d,%d]\n",
                   pInfo->stNormalMode.nRectLeft, pInfo->stNormalMode.nRectTop,
                   pInfo->stNormalMode.nRectRight, pInfo->stNormalMode.nRectBottom);
        }
    }
    printf("=================================\n");
}


/**
 * 获取设备配置
 * @details 解析URL参数中的通道号和命令码，根据命令码调用对应的获取配置处理函数
 * @param req_data 请求数据（未使用）
 * @param url_param URL参数（包含channel和command）
 * @return JSON格式的响应数据
 */
std::string CDeviceConfigBusiness::GetDevConfig(const std::string& req_data, const std::string& url_param)
{
    (void)req_data;
    INT32 channelId = ParseIntParam(url_param, TVAPI_PARAM_CHANNEL, 1);
    INT32 command = ParseIntParam(url_param, TVAPI_PARAM_COMMAND, NET_TV_CFG_INVALID);
    NSDK_LOG_INFO("GetDevConfig request: url[%s], channel=%d, command=%d",
                  url_param.c_str(),
                  channelId,
                  command);

    switch (command)
    {
        case NET_TV_GET_DEVICECFG:
            return HandleGetConfig<NET_TV_DEVICE_BASICINFO_S>(channelId, command);

        case NET_TV_GET_NTPCFG:
            return HandleGetConfig<NET_TV_SYSTEM_NTP_INFO_S>(channelId, command);

        case NET_TV_GET_AUDIOCFG:
            return HandleGetConfig<NET_TV_AUDIO_CFG_S>(channelId, command);

        case NET_TV_GET_NETWORKCFG:
            return HandleGetConfig<NET_TV_NETWORKCFG_S>(channelId, command);

        case NET_TV_GET_4G_INFO:
            return HandleGetConfig<NET_TV_4G_INFO_S>(channelId, command);
            
        case NET_TV_GET_HOTSPOT_CONN:
            return HandleGetConfig<NET_TV_HOTSPOT_CONN_INFO_S>(channelId, command);

        case NET_TV_GET_SECURITY_SERVICES_INFO:
            return HandleGetConfig<NET_TV_SECURITY_SERVICES_INFO_S>(channelId, command);

        case NET_TV_GET_SSH_COUNTDOWN:
            return HandleGetConfig<NET_TV_SSH_COUNTDOWN_INFO_S>(channelId, command);

        case NET_TV_FIND_LOG:
        case NET_TV_EXPORT_LOG:
            return HandleGetLogList(channelId, command, url_param);

        case NET_TV_GET_LOG_SERVER:
            return HandleGetConfig<NET_TV_LOG_SERVER_INFO_S>(channelId, command);

        case NET_TV_GET_RECORD_STATUS:
            return HandleGetConfig<NET_TV_RECORD_STATUS_INFO_S>(channelId, command);

        case NET_GET_SD_CARD_STATUS:
            return HandleGetConfig<NET_SdCardStatus_S>(channelId, command);

        case NET_GET_AUDIBLE_ALARM_INFO:
            return HandleGetConfig<NET_AudibleAlarmInfo_S>(channelId, command);

        case NET_GET_ALARM_INPUT_INFO:
            return HandleGetConfig<NET_AlarmInputInfoList_S>(channelId, command);

        case NET_GET_ALARM_OUTPUT_INFO:
            return HandleGetConfig<NET_AlarmOutputInfoList_S>(channelId, command);

        case NET_GET_FLASHING_LIGHT_ALARM_INFO:
            return HandleGetConfig<NET_FlashingLightAlarmInfo_S>(channelId, command);

        case NET_GET_PIR_ALARM_INFO:
            return HandleGetConfig<NET_PirAlarmInfo_S>(channelId, command);

        case NET_TV_GET_RECORD_SCHEDULE:
            return HandleGetConfig<NET_TV_RECORD_SCHEDULE_S>(channelId, command);

        case NET_TV_GET_RECORD_ADVANCED_PARAM:
            return HandleGetConfig<NET_TV_RECORD_ADVANCED_PARAM_S>(channelId, command);

        case NET_TV_FIND_RECORD_FILE_INFO:
            return HandleGetRecordFileList(channelId, command, url_param);
        
        case NET_TV_GET_STREAMCFG:
            return HandleGetConfig<NET_TV_VIDEO_ENCODE_OPTION_S>(channelId, command);
            
        case NET_TV_GET_RTSPURLCFG:
            return HandleGetConfig<NET_TV_RTSP_URL_INFO_S>(channelId, command);

        case NET_TV_GET_OSDCAPCFG:
            return HandleGetConfig<NET_TV_VIDEO_OSD_CFG_S>(channelId, command);

        case NET_TV_GET_IMAGECFG:
            return HandleGetConfig<NET_TV_IMAGE_SETTING_S>(channelId, command);

        case NET_TV_GET_PRIVACYMASKCFG:
            return HandleGetConfig<NET_TV_PRIVACY_MASK_CFG_S>(channelId, command);

        case NET_TV_GET_TAMPERALARM:
            return HandleGetConfig<NET_TV_TAMPER_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_MOTIONALARM:
            return HandleGetConfig<NET_TV_MOTION_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_CROSSLINEALARM:
            return HandleGetConfig<NET_TV_CROSS_LINE_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_INTRUSIONALARM:
            return HandleGetConfig<NET_TV_INTRUSION_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_ENTERREGIONALARM:
            return HandleGetConfig<NET_TV_ENTER_REGION_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_LEAVEREGIONALARM:
            return HandleGetConfig<NET_TV_LEAVE_REGION_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_LOITERINGALARM:
            return HandleGetConfig<NET_TV_LOITERING_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_SCENECHANGEALARM:
            return HandleGetConfig<NET_TV_SCENE_CHANGE_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_CROWDGATHERINGALARM:
            return HandleGetConfig<NET_TV_CROWD_GATHERING_ALARM_INFO_S>(channelId, command);
        
        case NET_TV_GET_GARBAGE_EXPOSURE_CFG:
            return HandleGetConfig<NET_TV_GARBAGE_EXPOSURE_CFG_S>(channelId, command);

        case NET_TV_GET_GARBAGE_OVERFLOW_CFG:
            return HandleGetConfig<NET_TV_GARBAGE_OVERFLOW_CFG_S>(channelId, command);

        case NET_TV_GET_PARKINGALARM:
            return HandleGetConfig<NET_TV_PARKING_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_UNATTENDEDOBJECTALARM:
            return HandleGetConfig<NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_OBJECTREMOVALALARM:
            return HandleGetConfig<NET_TV_OBJECT_REMOVAL_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_AUDIOANOMALYALARM:
            return HandleGetConfig<NET_TV_AUDIO_ANOMALY_ALARM_INFO_S>(channelId, command);

        case NET_TV_GET_PREVIEW_INFO:
            return HandleGetConfig<NET_TV_PREVIEW_INFO_S>(channelId, command);
            
        case NET_TV_GET_CHANNEL_INFO:
            return HandleGetChannelInfo(channelId, command);

        case NET_TV_GET_CHANNEL_LIST:
            return HandleGetChannelList(channelId, command);

        case NET_TV_FROM_STREAM_TALKBACK:
            return HandleGetConfig<NET_TV_TALKBACK_STREAM_INFO_S>(channelId, command);

        case NET_TV_GET_VOICECOM_AUDIO_CFG:
            return HandleGetConfig<NET_TV_VOICECOM_AUDIO_CFG_S>(channelId, command);

        case NET_TV_GET_UPGRADESTATUS:
            return HandleGetConfig<NET_TV_UPGRADE_STATUS_S>(channelId, command);

        case NET_TV_GET_UPGRADEVERSION:
            return HandleGetConfig<NET_TV_UPGRADE_VERSION_S>(channelId, command);

        case NET_TV_GET_CAPTURE_PLAN_INFO:
            return HandleGetConfig<NET_TV_CAPTURE_PLAN_INFO_S>(channelId, command);

        case NET_TV_GET_CAPTURE_PARAM_INFO:
            return HandleGetConfig<NET_TV_CAPTURE_PARAM_INFO_S>(channelId, command);

        case NET_TV_GET_EXPOSURE_INFO:
            return HandleGetConfig<NET_TV_EXPOSURE_INFO_S>(channelId, command);

        case NET_TV_GET_DAYNIGHT_INFO:
            return HandleGetConfig<NET_TV_DAYNIGHT_INFO_S>(channelId, command);

        case NET_TV_GET_BACKLIGHT_INFO:
            return HandleGetConfig<NET_TV_BACKLIGHT_INFO_S>(channelId, command);

        case NET_TV_GET_DENOISE_INFO:
            return HandleGetConfig<NET_TV_DENOISE_INFO_S>(channelId, command);

        case NET_TV_GET_WHITEBALANCE_INFO:
            return HandleGetConfig<NET_TV_WHITEBALANCE_INFO_S>(channelId, command);

        case NET_TV_GET_FACECAPTUREINFO:
            return HandleGetConfig<NET_TV_FACE_CAPTURE_INFO_S>(channelId, command);

        case NET_TV_GET_FACECAPTUREOVERLAYINFO:
            return HandleGetConfig<NET_TV_FACE_CAPTURE_OVERLAY_INFO_S>(channelId, command);

        case NET_TV_GET_TARGET_LIB:
            return HandleGetConfig<NET_TV_FACE_LIB_LIST_S>(channelId, command);

        case NET_TV_GET_FACE_INFO:
            return HandleGetConfig<NET_TV_FACE_INFO_LIST_S>(channelId, command);

        case NET_TV_GET_PEOPLE_FLOW_STATISTICS_CFG:
            return HandleGetConfig<NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S>(channelId, command);

        case NET_TV_GET_PEOPLE_DENSITY_DETECTION_CFG:
            return HandleGetConfig<NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S>(channelId, command);
        
        case NET_TV_GET_MANHOLE_COVER_ABNORMAL_CFG:
            return HandleGetConfig<NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S>(channelId, command);

        case NET_TV_GET_SLEEP_ON_DUTY_CFG:
            return HandleGetConfig<NET_TV_SLEEP_ON_DUTY_CFG_S>(channelId, command);

        case NET_TV_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG:
            return HandleGetConfig<NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S>(channelId, command);

        case NET_TV_GET_PERSON_FALL_DOWN_CFG:
            return HandleGetConfig<NET_TV_PERSON_FALL_DOWN_CFG_S>(channelId, command);

        case NET_TV_GET_CONSTRUCTION_OCCUPY_ROAD_CFG:
            return HandleGetConfig<NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S>(channelId, command);

        case NET_TV_GET_CONGESTION_CFG:
            return HandleGetConfig<NET_TV_CONGESTION_CFG_S>(channelId, command);

        case NET_TV_GET_LICENSE_PLATE_RECOGNITION_CFG:
            return HandleGetConfig<NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S>(channelId, command);

        case NET_TV_GET_HIGH_ALTITUDE_SEATBELT_CFG:
            return HandleGetConfig<NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S>(channelId, command);

        case NET_TV_GET_SAFETY_HELMET_CFG:
            return HandleGetConfig<NET_TV_SAFETY_HELMET_CFG_S>(channelId, command);

        case NET_TV_GET_PERSON_FALL_CFG:
            return HandleGetConfig<NET_TV_PERSON_FALL_CFG_S>(channelId, command);

        case NET_TV_GET_PHONE_USAGE_CFG:
            return HandleGetConfig<NET_TV_PHONE_USAGE_CFG_S>(channelId, command);

        case NET_TV_GET_SMOKING_CFG:
            return HandleGetConfig<NET_TV_SMOKING_CFG_S>(channelId, command);

        case NET_TV_GET_OPEN_FLAME_CFG:
            return HandleGetConfig<NET_TV_OPEN_FLAME_CFG_S>(channelId, command);

        case NET_TV_GET_BARE_SOIL_CFG:
            return HandleGetConfig<NET_TV_BARE_SOIL_CFG_S>(channelId, command);

        case NET_TV_GET_HOLE_PROTECTION_BAR_CFG:
            return HandleGetConfig<NET_TV_HOLE_PROTECTION_BAR_CFG_S>(channelId, command);

        case NET_TV_GET_REFLECTIVE_CLOTHING_CFG:
            return HandleGetConfig<NET_TV_REFLECTIVE_CLOTHING_CFG_S>(channelId, command);

        case NET_TV_GET_PET_RECOGNITION_INFO:
            return HandleGetConfig<NET_TV_PET_RECOGNITION_INFO_S>(channelId, command);

        case NET_TV_GET_CLIMB_FENCE_INFO:
            return HandleGetConfig<NET_TV_CLIMB_FENCE_INFO_S>(channelId, command);
        
        case NET_TV_GET_DIMISSION_INFO:
            return HandleGetConfig<NET_TV_DIMISSION_INFO_S>(channelId, command);

        case NET_TV_GET_ILLEGAL_LANE_INFO:
            return HandleGetConfig<NET_TV_ILLEGAL_LANE_INFO_S>(channelId, command);

        case NET_TV_GET_RETROGRADE_INFO:
            return HandleGetConfig<NET_TV_RETROGRADE_INFO_S>(channelId, command);

        case NET_TV_GET_NONMOTOR_VEHICLE_INTRUSION_INFO:
            return HandleGetConfig<NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S>(channelId, command);

        case NET_TV_GET_OCCUPATION_EMERGENCY_INFO:
            return HandleGetConfig<NET_TV_OCCUPATION_EMERGENCY_INFO_S>(channelId, command);

        case NET_TV_GET_PEDESTRIAN_INTRUSION_INFO:
            return HandleGetConfig<NET_TV_PEDESTRIAN_INTRUSION_INFO_S>(channelId, command);

        case NET_TV_GET_SMOKE_FIRE_CFG:
            return HandleGetConfig<NET_TV_SMOKE_FIRE_CFG_S>(channelId, command);

        case NET_TV_GET_ROAD_PONDING_CFG:
            return HandleGetConfig<NET_TV_ROAD_PONDING_CFG_S>(channelId, command);   
        default:
            NSDK_LOG_WARN("Unsupported GetDevConfig command: %d", command);
            return SDKConvert::to_respString(NET_TV_E_CMD_NOT_SUPPORT);
    }
}

/**
 * 设置设备配置
 * @details 解析URL参数中的通道号和命令码，解析请求数据中的配置信息，根据命令码调用对应的设置配置处理函数
 * @param req_data 请求数据（JSON格式，包含配置信息）
 * @param url_param URL参数（包含channel和command）
 * @return JSON格式的响应数据
 */
std::string CDeviceConfigBusiness::SetDevConfig(const std::string& req_data, const std::string& url_param)
{
    INT32 channelId = ParseIntParam(url_param, TVAPI_PARAM_CHANNEL, 1);
    INT32 command = ParseIntParam(url_param, TVAPI_PARAM_COMMAND, NET_TV_CFG_INVALID);
    NSDK_LOG_INFO("SetDevConfig request: url[%s], channel=%d, command=%d",
                  url_param.c_str(),
                  channelId,
                  command);
    
    if (command == NET_TV_TRIGGER_SOUND_LIGHT_ALARM)
    {
        NSDK_LOG_INFO("[TriggerSoundLight][Ingress] received command=508, channel=%d, payload_size=%zu",
                      channelId,
                      req_data.size());
    }
    
    switch (command)
    {
        case NET_TV_SET_DEVICECFG:
            return HandleSetConfig<NET_TV_DEVICE_BASICINFO_S>(channelId, command, req_data);

        case NET_TV_SET_NTPCFG:
            return HandleSetConfig<NET_TV_SYSTEM_NTP_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_AUDIOCFG:
            return HandleSetConfig<NET_TV_AUDIO_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_NETWORKCFG:
            return HandleSetConfig<NET_TV_NETWORKCFG_S>(channelId, command, req_data);
        
        case NET_TV_SET_CONFIG_WIFI_STA:
            return HandleSetConfig<NET_TV_WIFI_STA_CFG_S>(channelId, command, req_data);

        case NET_TV_CONNECT_WIFI_STA:
            return HandleSetConfig<NET_TV_WIFI_STA_CONNECT_S>(channelId, command, req_data);

        case NET_TV_DISCONNECT_WIFI_STA:
            return HandleSetConfig<NET_TV_WIFI_STA_CONNECT_S>(channelId, command, req_data);

        case NET_TV_SET_4G_INFO:
            return HandleSetConfig<NET_TV_4G_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_HOTSPOT_INFO:
            return HandleSetConfig<NET_TV_HOTSPOT_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_SECURITY_SERVICES_INFO:
            return HandleSetConfig<NET_TV_SECURITY_SERVICES_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_LOG_SERVER:
            return HandleSetConfig<NET_TV_LOG_SERVER_INFO_S>(channelId, command, req_data);

        case NET_TV_TEST_LOG_SERVER:
            return HandleSetConfig<NET_TV_LOG_SERVER_INFO_S>(channelId, command, req_data);

        case NET_TV_CONTROL_RECORD_INFO:
            return HandleSetConfig<NET_TV_RECORD_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_RECORD_SCHEDULE:
            return HandleSetConfig<NET_TV_RECORD_SCHEDULE_S>(channelId, command, req_data);

        case NET_TV_SET_RECORD_ADVANCED_PARAM:
            return HandleSetConfig<NET_TV_RECORD_ADVANCED_PARAM_S>(channelId, command, req_data);

        case NET_TV_DOWNLOAD_RECORD_FILE:
            return HandleSetConfig<NET_TV_RECORD_DOWNLOAD_LIST_S>(channelId, command, req_data);
        
        case NET_TV_SET_STREAMCFG:
            return HandleSetConfig<NET_TV_VIDEO_ENCODE_OPTION_S>(channelId, command, req_data);

        case NET_TV_SET_OSDCAPCFG:
            return HandleSetConfig<NET_TV_VIDEO_OSD_CFG_S>(channelId, command, req_data);
   
        case NET_TV_SET_IMAGECFG:
            return HandleSetConfig<NET_TV_IMAGE_SETTING_S>(channelId, command, req_data);

        case NET_TV_SET_PRIVACYMASKCFG:
            return HandleSetConfig<NET_TV_PRIVACY_MASK_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_TAMPERALARM:
            return HandleSetConfig<NET_TV_TAMPER_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_MOTIONALARM:
            return HandleSetConfig<NET_TV_MOTION_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_CROSSLINEALARM:
            return HandleSetConfig<NET_TV_CROSS_LINE_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_INTRUSIONALARM:
            return HandleSetConfig<NET_TV_INTRUSION_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_ENTERREGIONALARM:
            return HandleSetConfig<NET_TV_ENTER_REGION_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_LEAVEREGIONALARM:
            return HandleSetConfig<NET_TV_LEAVE_REGION_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_LOITERINGALARM:
            return HandleSetConfig<NET_TV_LOITERING_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_SCENECHANGEALARM:
            return HandleSetConfig<NET_TV_SCENE_CHANGE_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_CROWDGATHERINGALARM:
            return HandleSetConfig<NET_TV_CROWD_GATHERING_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_GARBAGE_EXPOSURE_CFG:
            return HandleSetConfig<NET_TV_GARBAGE_EXPOSURE_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_GARBAGE_OVERFLOW_CFG:
            return HandleSetConfig<NET_TV_GARBAGE_OVERFLOW_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_PARKINGALARM:
            return HandleSetConfig<NET_TV_PARKING_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_UNATTENDEDOBJECTALARM:
            return HandleSetConfig<NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_OBJECTREMOVALALARM:
            return HandleSetConfig<NET_TV_OBJECT_REMOVAL_ALARM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_AUDIOANOMALYALARM:
            return HandleSetConfig<NET_TV_AUDIO_ANOMALY_ALARM_INFO_S>(channelId, command, req_data);

        case NET_SET_AUDIBLE_ALARM_INFO:
            return HandleSetConfig<NET_AudibleAlarmInfo_S>(channelId, command, req_data);

        case NET_SET_ALARM_INPUT_INFO:
            return HandleSetConfig<NET_AlarmInputInfo_S>(channelId, command, req_data);

        case NET_SET_ALARM_OUTPUT_INFO:
            return HandleSetConfig<NET_AlarmOutputInfo_S>(channelId, command, req_data);

        case NET_SET_FLASHING_LIGHT_ALARM_INFO:
            return HandleSetConfig<NET_FlashingLightAlarmInfo_S>(channelId, command, req_data);

        case NET_SET_PIR_ALARM_INFO:
            return HandleSetConfig<NET_PirAlarmInfo_S>(channelId, command, req_data);

        case NET_TV_TRIGGER_SOUND_LIGHT_ALARM:
            return HandleSetConfig<NET_SoundLightAlarmTrigger_S>(channelId, command, req_data);

        case NET_TV_SET_PREVIEW_INFO:
            return HandleSetConfig<NET_TV_PREVIEW_INFO_S>(channelId, command, req_data);
        
        case NET_TV_SET_VOICECOM_AUDIO_CFG:
            return HandleSetConfig<NET_TV_VOICECOM_AUDIO_CFG_S>(channelId, command, req_data);
        
        case NET_TV_SET_UPGRADE:
            return HandleSetConfig<NET_TV_UPGRADE_INFO_S>(channelId, command, req_data);
        
        case NET_TV_SET_CAPTURE_PLAN_INFO:
            return HandleSetConfig<NET_TV_CAPTURE_PLAN_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_CAPTURE_PARAM_INFO:
            return HandleSetConfig<NET_TV_CAPTURE_PARAM_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_EXPOSURE_INFO:
            return HandleSetConfig<NET_TV_EXPOSURE_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_DAYNIGHT_INFO:
            return HandleSetConfig<NET_TV_DAYNIGHT_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_BACKLIGHT_INFO:
            return HandleSetConfig<NET_TV_BACKLIGHT_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_DENOISE_INFO:
            return HandleSetConfig<NET_TV_DENOISE_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_WHITEBALANCE_INFO:
            return HandleSetConfig<NET_TV_WHITEBALANCE_INFO_S>(channelId, command, req_data);
        
        case NET_TV_STATE_TALKBACK:
            return HandleSetConfig<NET_TV_TALKBACK_STATE_INFO_S>(channelId, command, req_data);

        case NET_TV_TO_STREAM_TALKBACK:
            return HandleSetConfig<NET_TV_TALKBACK_STREAM_INFO_S>(channelId, command, req_data);

        case NET_TV_REPLAY_TALKBACK:
            return HandleSetConfig<NET_TV_REPLAY_TALKBACK_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_FACECAPTUREINFO:
            return HandleSetConfig<NET_TV_FACE_CAPTURE_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_FACECAPTUREOVERLAYINFO:
            return HandleSetConfig<NET_TV_FACE_CAPTURE_OVERLAY_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_FACE_COMPARE_INFO:
            return HandleSetConfig<NET_TV_FACE_COMPARE_INFO_S>(channelId, command, req_data);

        case NET_TV_ADD_TARGET_LIB:
        case NET_TV_DEL_TARGET_LIB:
        case NET_TV_SET_TARGET_LIB:
            return HandleSetConfig<NET_TV_FACE_LIB_INFO_S>(channelId, command, req_data);

        case NET_TV_ADD_FACE_INFO:
        case NET_TV_SET_FACE_INFO:
            return HandleSetConfig<NET_TV_FACE_INFO_S>(channelId, command, req_data);

        case NET_TV_DEL_FACE_INFO:
            return HandleSetConfig<NET_TV_FACE_ID_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_PEOPLE_FLOW_STATISTICS_CFG:
            return HandleSetConfig<NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S>(channelId, command, req_data);

        case NET_TV_RESET_PEOPLE_FLOW_STATISTICS:
            return HandleSetConfig<NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_PEOPLE_DENSITY_DETECTION_CFG:
            return HandleSetConfig<NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_MANHOLE_COVER_ABNORMAL_CFG:
            return HandleSetConfig<NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_SLEEP_ON_DUTY_CFG:
            return HandleSetConfig<NET_TV_SLEEP_ON_DUTY_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG:
            return HandleSetConfig<NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_PERSON_FALL_DOWN_CFG:
            return HandleSetConfig<NET_TV_PERSON_FALL_DOWN_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_CONSTRUCTION_OCCUPY_ROAD_CFG:
            return HandleSetConfig<NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_CONGESTION_CFG:
            return HandleSetConfig<NET_TV_CONGESTION_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_LICENSE_PLATE_RECOGNITION_CFG:
            return HandleSetConfig<NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_HIGH_ALTITUDE_SEATBELT_CFG:
            return HandleSetConfig<NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_SAFETY_HELMET_CFG:
            return HandleSetConfig<NET_TV_SAFETY_HELMET_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_PERSON_FALL_CFG:
            return HandleSetConfig<NET_TV_PERSON_FALL_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_PHONE_USAGE_CFG:
            return HandleSetConfig<NET_TV_PHONE_USAGE_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_SMOKING_CFG:
            return HandleSetConfig<NET_TV_SMOKING_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_OPEN_FLAME_CFG:
            return HandleSetConfig<NET_TV_OPEN_FLAME_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_BARE_SOIL_CFG:
            return HandleSetConfig<NET_TV_BARE_SOIL_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_HOLE_PROTECTION_BAR_CFG:
            return HandleSetConfig<NET_TV_HOLE_PROTECTION_BAR_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_REFLECTIVE_CLOTHING_CFG:
            return HandleSetConfig<NET_TV_REFLECTIVE_CLOTHING_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_PET_RECOGNITION_INFO:
            return HandleSetConfig<NET_TV_PET_RECOGNITION_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_CLIMB_FENCE_INFO:
            return HandleSetConfig<NET_TV_CLIMB_FENCE_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_DIMISSION_INFO:
            return HandleSetConfig<NET_TV_DIMISSION_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_ILLEGAL_LANE_INFO:
            return HandleSetConfig<NET_TV_ILLEGAL_LANE_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_RETROGRADE_INFO:
            return HandleSetConfig<NET_TV_RETROGRADE_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_NONMOTOR_VEHICLE_INTRUSION_INFO:
            return HandleSetConfig<NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_OCCUPATION_EMERGENCY_INFO:
            return HandleSetConfig<NET_TV_OCCUPATION_EMERGENCY_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_PEDESTRIAN_INTRUSION_INFO:
            return HandleSetConfig<NET_TV_PEDESTRIAN_INTRUSION_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_SMOKE_FIRE_CFG:
            return HandleSetConfig<NET_TV_SMOKE_FIRE_CFG_S>(channelId, command, req_data);

        case NET_TV_SET_ROAD_PONDING_CFG:
            return HandleSetConfig<NET_TV_ROAD_PONDING_CFG_S>(channelId, command, req_data);

        default:
            NSDK_LOG_WARN("Unsupported SetDevConfig command: %d", command);
            return SDKConvert::to_respString(NET_TV_E_CMD_NOT_SUPPORT);
    }
}

/**
 * 解析URL参数中的字符串值
 * @param url_param URL参数字符串
 * @param key 参数名
 * @param defaultVal 默认值
 * @return 解析到的字符串值，未找到则返回默认值
 */
std::string CDeviceConfigBusiness::ParseStringParam(const std::string& url_param, const std::string& key, const std::string& defaultVal)
{
    std::string searchKey = key + "=";
    size_t pos = url_param.find(searchKey);
    if (pos == std::string::npos)
    {
        return defaultVal;
    }

    size_t valueStart = pos + searchKey.length();
    size_t valueEnd = url_param.find('&', valueStart);
    if (valueEnd == std::string::npos)
    {
        valueEnd = url_param.length();
    }

    return UrlDecode(url_param.substr(valueStart, valueEnd - valueStart));
}

/**
 * URL解码
 * @param value 待解码的URL编码字符串
 * @return 解码后的字符串
 */
std::string CDeviceConfigBusiness::UrlDecode(const std::string& value)
{
    std::string out;
    out.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i)
    {
        char ch = value[i];
        if (ch == '+')
        {
            out.push_back(' ');
            continue;
        }

        if (ch == '%' && i + 2 < value.size() &&
            std::isxdigit((unsigned char)value[i + 1]) &&
            std::isxdigit((unsigned char)value[i + 2]))
        {
            char hex[3] = {value[i + 1], value[i + 2], '\0'};
            out.push_back((char)std::strtol(hex, NULL, 16));
            i += 2;
            continue;
        }

        out.push_back(ch);
    }

    return out;
}

/**
 * 解析URL参数中的整数值
 * @param url_param URL参数字符串
 * @param key 参数名
 * @param defaultVal 默认值
 * @return 解析到的整数值，未找到或解析失败则返回默认值
 */
int CDeviceConfigBusiness::ParseIntParam(const std::string& url_param, const std::string& key, int defaultVal)
{
    std::string searchKey = key + "=";
    size_t pos = url_param.find(searchKey);
    if (pos == std::string::npos)
    {
        return defaultVal;
    }

    size_t valueStart = pos + searchKey.length();
    size_t valueEnd = url_param.find('&', valueStart);
    if (valueEnd == std::string::npos)
    {
        valueEnd = url_param.length();
    }

    std::string valueStr = url_param.substr(valueStart, valueEnd - valueStart);
    try
    {
        return std::stoi(valueStr);
    }
    catch (...)
    {
        return defaultVal;
    }
}
