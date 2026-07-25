
#ifndef _USERCONVERT_H
#define _USERCONVERT_H

#include <string>
#include <vector>
#include <set>

#include "Json.h"

// 库通用头文件
#ifdef NET_TV_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_TV_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif

namespace SDKConvert
{
    void deal(Json::Object* pRootJson, NET_TV_AUDIO_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_EnterRegionAlarmInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LeaveRegionAlarmInfo_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_DeviceInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_DeviceBasicInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SystemNtpInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UserPasswordInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_NETWORKCFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_WIFI_STA_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_WIFI_WEP_KEY_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_WIFI_STA_CONNECT_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_4G_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_HOTSPOT_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_HOTSPOT_CONN_DEVICE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_HOTSPOT_CONN_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PAGE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOGIN_LOCK_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PWD_POLICY_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SSH_ADMIN_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SECURITY_SERVICES_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_SSH_COUNTDOWN_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOG_SERVER_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOG_RETRIEVAL_COND_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOG_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_LOG_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_STATUS_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_TIME_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_DAY_SCHEDULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_SCHEDULE_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_ADVANCED_PARAM_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_FIND_COND_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_VIDEO_TIME_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_FIND_RESULT_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_FILE_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_DOWNLOAD_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_DOWNLOAD_PROGRESS_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RECORD_DOWNLOAD_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_VIDEO_OSD_CFG_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_RTSP_URL_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_DeviceControlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFrameStreamCond_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFrameStreamInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFrameStopInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REPLAY_URL_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REPLAY_CTRL_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REPLAY_RECORD_TIME_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_REPLAY_RECORD_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CHANNEL_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_CHANNEL_LIST_S& stInfo, bool bOutStruct);


    void deal(Json::Object* pRootJson, NET_TV_PREVIEW_RTSP_URL_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PREVIEW_IMAGE_PARAM_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_IMAGE_SETTING_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PREVIEW_INFO_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_TalkbackStateInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TalkbackStreamInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayTalkbackInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_VoiceComAudioCfg_S& stInfo, bool bOutStruct);


    // 系统升级相关
    void deal(Json::Object*& pRootJson, tagNETTVUpgradeInfo*& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_UPGRADE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_UPGRADE_STATUS_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_UPGRADE_VERSION_S& stInfo, bool bOutStruct);
    
    // 布防时间和联动相关
    void deal(Json::Object* pRootJson, NET_TV_SCHED_TIME_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmSchedule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LinkageList_S& stInfo, bool bOutStruct);
    
    // 移动侦测相关
    void deal(Json::Object* pRootJson, NET_MotionRegion_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionExpertMode_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionNormalMode_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionAlarmInfo_S& stInfo, bool bOutStruct);
    
    // 隐私遮盖配置相关
    void deal(Json::Object* pRootJson, NET_TV_PRIVACY_MASK_AREA_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_PRIVACY_MASK_CFG_S& stInfo, bool bOutStruct);

    // 遮挡报警相关
    void deal(Json::Object* pRootJson, NET_TamperAlarmInfo_S& stInfo, bool bOutStruct);
    
    // 越界检测相关
    void deal(Json::Object* pRootJson, NET_BoundaryPlane_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_CrossLineAlarmInfo_S& stInfo, bool bOutStruct);
    
    // 入侵检测相关
    void deal(Json::Object* pRootJson, NET_IntrusionRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_IntrusionAlarmInfo_S& stInfo, bool bOutStruct);

    // 徘徊侦测相关
    void deal(Json::Object* pRootJson, NET_LoiteringRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LoiteringAlarmInfo_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_AudioAnomalyAlarmInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SceneChangeAlarmInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_CrowdGatheringRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_CrowdGatheringAlarmInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ParkingRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ParkingAlarmInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UnattendedObjectRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UnattendedObjectAlarmInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ObjectRemovalRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ObjectRemovalAlarmInfo_S& stInfo, bool bOutStruct);

    // 垃圾检测配置
    void deal(Json::Object* pRootJson, NET_GarbageExposureRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_GarbageExposureCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_GarbageOverflowRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_GarbageOverflowCfg_S& stInfo, bool bOutStruct);

    // 单规则智能检测配置
    void deal(Json::Object* pRootJson, NET_AiSimpleRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ManholeCoverAbnormalCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SleepOnDutyCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ElectricVehicleInElevatorCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PersonFallDownCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ConstructionOccupyRoadCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_CongestionCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LicensePlateRecognitionCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HighAltitudeSeatbeltCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SafetyHelmetCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PersonFallCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PhoneUsageCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SmokingCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_OpenFlameCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_BareSoilCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HoleProtectionBarCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReflectiveClothingCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SmartRegion_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SmartRegionRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SmartLineRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PetRecognitionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ClimbFenceInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_DimissionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_IllegalLaneInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RetrogradeInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_NonmotorVehicleIntrusionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_OccupationEmergencyInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PedestrianIntrusionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SmokeFireCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RoadPondingCfg_S& stInfo, bool bOutStruct);
    
    // 抓图计划相关
    void deal(Json::Object *pRootJson, NET_TV_CAPTURE_TIME_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_CAPTURE_DAY_SCHEDULE_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_CAPTURE_PLAN_INFO_S &stInfo, bool bOutStruct);

    // 抓图参数相关
    void deal(Json::Object *pRootJson, NET_CaptureConfig_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_CaptureParamInfo_S &stInfo, bool bOutStruct);

    // ISP params
    void deal(Json::Object *pRootJson, NET_TV_EXPOSURE_INFO_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_DAYNIGHT_INFO_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_BACKLIGHT_INFO_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_DENOISE_INFO_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_TV_WHITEBALANCE_INFO_S &stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_FaceCaptureRegion_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCaptureRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCaptureInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCompareInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_LIB_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_LIB_LIST_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_ID_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_FACE_INFO_LIST_S& stInfo, bool bOutStruct);

    // 人流统计与人员密度检测相关
    void deal(Json::Object* pRootJson, NET_PeopleFlowRuleLine_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleAlarmRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleAlarmConfig_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_StatisticsResetConfig_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleFlowStatisticsCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleDensityDetectionCfg_S& stInfo, bool bOutStruct);
};

#endif
