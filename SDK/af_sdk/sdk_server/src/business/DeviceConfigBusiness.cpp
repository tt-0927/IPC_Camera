/**
 * @file DeviceConfigBusiness.cpp
 * @brief Device config business implementation
 */

#include "DeviceConfigBusiness.h"
#include "NetTVSDKHttpUrl.h"

#include <cctype>
#include <cstdlib>
#include <memory>

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

/* 打印移动侦测配置 */
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
            return HandleGetConfig<NET_TV_ALARM_EXCEPTION_INFO_S>(channelId, command);

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
            return HandleGetConfig<NET_TV_OSD_CAP_S>(channelId, command);

        case NET_TV_GET_OSDCFG:
            return HandleGetConfig<NET_TV_VIDEO_OSD_CFG_S>(channelId, command);

        case NET_TV_GET_IMAGECFG:
            return HandleGetConfig<NET_TV_ALARM_EXCEPTION_INFO_S>(channelId, command);

        case NET_TV_GET_PRIVACYMASKCFG:
            return HandleGetConfig<NET_TV_ALARM_RULE_INFO_S>(channelId, command);

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

        case NET_TV_GET_FACE_COMPARE_INFO:
            return HandleGetConfig<NET_TV_FACE_COMPARE_INFO_S>(channelId, command);

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

std::string CDeviceConfigBusiness::SetDevConfig(const std::string& req_data, const std::string& url_param)
{
    INT32 channelId = ParseIntParam(url_param, TVAPI_PARAM_CHANNEL, 1);
    INT32 command = ParseIntParam(url_param, TVAPI_PARAM_COMMAND, NET_TV_CFG_INVALID);
    NSDK_LOG_INFO("SetDevConfig request: url[%s], channel=%d, command=%d",
                  url_param.c_str(),
                  channelId,
                  command);

    switch (command)
    {
        case NET_TV_SET_DEVICECFG:
            return HandleSetConfig<NET_TV_DEVICE_BASICINFO_S>(channelId, command, req_data);

        case NET_TV_SET_NTPCFG:
            return HandleSetConfig<NET_TV_ALARM_EXCEPTION_INFO_S>(channelId, command, req_data);

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

        case NET_TV_SET_OSDCFG:
            return HandleSetConfig<NET_TV_VIDEO_OSD_CFG_S>(channelId, command, req_data);
   
        case NET_TV_SET_IMAGECFG:
            return HandleSetConfig<NET_TV_ALARM_EXCEPTION_INFO_S>(channelId, command, req_data);

        case NET_TV_SET_PRIVACYMASKCFG:
            return HandleSetConfig<NET_TV_ALARM_RULE_INFO_S>(channelId, command, req_data);

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

        case NET_TV_SET_PREVIEW_INFO:
            return HandleSetConfig<NET_TV_PREVIEW_INFO_S>(channelId, command, req_data);
        
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
