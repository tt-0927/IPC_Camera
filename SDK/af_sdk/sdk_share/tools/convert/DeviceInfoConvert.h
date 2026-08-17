/**
 * @file DeviceInfoConvert.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceInfoConvert 模块接口与类型定义
 * 功能说明：
 * 1. 声明 DeviceInfoConvert 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_DEVICE_INFO_CONVERT_H
#define NETSDK_DEVICE_INFO_CONVERT_H

#include <string>
#include <vector>
#include <set>

#include "Json.h"

/* 库通用头文件 */
#ifdef NET_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_SDK_CLIENT_API)
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

    /**
     * @brief 在 JSON 与手动声光报警触发请求之间转换。
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标请求结构体。
     * @param [in] bOutStruct TRUE 表示 JSON 转结构体，FALSE 表示结构体转 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_SoundLightAlarmTrigger_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_MotionRegion_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionExpertMode_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionNormalMode_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionAlarmInfo_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_PrivacyMaskArea_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PrivacyMaskCfg_S& stInfo, bool bOutStruct);

    /* 遮挡报警相关 */
    void deal(Json::Object* pRootJson, NET_TamperAlarmInfo_S& stInfo, bool bOutStruct);

    /* 越界检测相关 */
    void deal(Json::Object* pRootJson, NET_BoundaryPlane_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_CrossLineAlarmInfo_S& stInfo, bool bOutStruct);

    /* 入侵检测相关 */
    void deal(Json::Object* pRootJson, NET_IntrusionRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_IntrusionAlarmInfo_S& stInfo, bool bOutStruct);

    /* 徘徊侦测相关 */
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

    /* 垃圾检测配置 */
    void deal(Json::Object* pRootJson, NET_GarbageExposureRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_GarbageExposureCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_GarbageOverflowRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_GarbageOverflowCfg_S& stInfo, bool bOutStruct);

    /* 单规则智能检测配置 */
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

    /* 抓图参数相关 */
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
    /**
     * @brief 在 JSON 与人脸抓拍图片叠加配置之间转换。
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标配置结构体。
     * @param [in] bOutStruct TRUE 表示 JSON 转结构体，FALSE 表示结构体转 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_FaceCaptureOverlayInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceCompareInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceLibInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceLibList_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceIdInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_FaceInfoList_S& stInfo, bool bOutStruct);

    /* 人流统计与人员密度检测相关 */
    void deal(Json::Object* pRootJson, NET_PeopleFlowRuleLine_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleAlarmRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleAlarmConfig_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_StatisticsResetConfig_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleFlowStatisticsCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PeopleDensityDetectionCfg_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK SD 卡状态结构体之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源 JSON 或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标 SD 卡状态结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到 stInfo；为 FALSE 时将 stInfo 序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_SdCardStatus_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 自定义声音告警音频信息之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标自定义音频结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AudibleAlarmCustomAudio_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 声音告警配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标声音告警配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AudibleAlarmInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 单路报警输入配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输入配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AlarmInputInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 报警输入配置集合之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输入配置集合结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AlarmInputInfoList_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 单路报警输出配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输出配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AlarmOutputInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 报警输出配置集合之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标报警输出配置集合结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_AlarmOutputInfoList_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK 闪光灯告警配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标闪光灯告警配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_FlashingLightAlarmInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 在 JSON 与 SDK PIR 告警配置之间转换。
     * @author ITC
     * @param [in,out] pRootJson 根据 bOutStruct 作为源或目标 JSON 对象。
     * @param [in,out] stInfo 根据 bOutStruct 作为源或目标 PIR 告警配置结构体。
     * @param [in] bOutStruct 为 TRUE 时将 JSON 解析到结构体；为 FALSE 时将结构体序列化到 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_PirAlarmInfo_S& stInfo, bool bOutStruct);

};

#endif
