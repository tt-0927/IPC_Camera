
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
    void deal(Json::Object* pRootJson, NET_AudioCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_EnterRegionAlarmInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LeaveRegionAlarmInfo_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_DeviceInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_DeviceBasicInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SystemNtpInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UserPasswordInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_NetworkCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_WifiStaCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_WifiWepKey_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_WifiStaConnect_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_4GInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HotspotInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HotspotConnDevice_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HotspotConnInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PageInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LoginLockInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PwdPolicyInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SshAdminInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SecurityServicesInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SshCountdownInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LogServerInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LogRetrievalCond_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LogInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LogList_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordStatusInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordTime_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordDaySchedule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordSchedule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordAdvancedParam_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFindCond_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordVideoTime_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFindResult_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFileList_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordDownloadInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordDownloadProgress_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordDownloadList_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_VideoOsdCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RtspUrlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_DeviceControlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFrameStreamCond_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFrameStreamInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFrameStopInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayUrlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayCtrlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayRecordTime_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayRecordList_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ChannelInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ChannelList_S& stInfo, bool bOutStruct);


    void deal(Json::Object* pRootJson, NET_PreviewRtspUrl_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PreviewImageParam_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ImageSetting_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PreviewInfo_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_TalkbackStateInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TalkbackStreamInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayTalkbackInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_VoiceComAudioCfg_S& stInfo, bool bOutStruct);


    void deal(Json::Object*& pRootJson, tagNET_UpgradeInfo* stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UpgradeInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UpgradeStatus_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UpgradeVersion_S& stInfo, bool bOutStruct);
    
    void deal(Json::Object* pRootJson, NET_SchedTime_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmSchedule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LinkageList_S& stInfo, bool bOutStruct);
    
    void deal(Json::Object* pRootJson, NET_MotionRegion_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionExpertMode_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionNormalMode_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionAlarmInfo_S& stInfo, bool bOutStruct);
    
    void deal(Json::Object* pRootJson, NET_PrivacyMaskArea_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PrivacyMaskCfg_S& stInfo, bool bOutStruct);

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
    
    void deal(Json::Object *pRootJson, NET_CaptureTime_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_CaptureDaySchedule_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_CapturePlanInfo_S &stInfo, bool bOutStruct);

    // 抓图参数相关
    void deal(Json::Object *pRootJson, NET_CaptureConfig_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_CaptureParamInfo_S &stInfo, bool bOutStruct);

    void deal(Json::Object *pRootJson, NET_ExposureInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_DayNightInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_BackLightInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_DenoiseInfo_S &stInfo, bool bOutStruct);
    void deal(Json::Object *pRootJson, NET_WhiteBalanceInfo_S &stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_FaceCaptureRegion_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCaptureRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCaptureInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCompareInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceLibInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceLibList_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceIdInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceInfoList_S& stInfo, bool bOutStruct);

    // 人流统计与人员密度检测相关
    void deal(Json::Object* pRootJson, NET_PeopleFlowRuleLine_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleAlarmRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleAlarmConfig_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_StatisticsResetConfig_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleFlowStatisticsCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleDensityDetectionCfg_S& stInfo, bool bOutStruct);
};

#endif
