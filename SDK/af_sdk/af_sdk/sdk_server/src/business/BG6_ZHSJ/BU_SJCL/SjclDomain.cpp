/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJCL/SjclDomain.cpp
 * @Author       : chenchl
 * @Date         : 2026-08-20
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-20
 * @Description  : SJCL 配置域实现
 *                 注册 NVR/录播命令码→处理函数映射。
 */
#ifndef BU_SJCL_EXCLUDE

#include "SjclDomain.h"
#include "NvrBusiness.h"

CSjclDomain::CSjclDomain()
{
    /* ==================================================================
     * Get 命令注册
     * ================================================================== */

    /* ===== 录像参数 ===== */
    m_getTable[NET_GET_RECORD_STATUS]        = &CSjclDomain::TemplatedGet<NET_RecordStatusInfo_S>;
    m_getTable[NET_GET_RECORD_SCHEDULE]      = &CSjclDomain::TemplatedGet<NET_RecordSchedule_S>;
    m_getTable[NET_GET_RECORD_ADVANCED_PARAM] = &CSjclDomain::TemplatedGet<NET_RecordAdvancedParam_S>;

    /* ===== 抓拍 ===== */
    m_getTable[NET_GET_CAPTURE_PLAN_INFO]   = &CSjclDomain::TemplatedGet<NET_CapturePlanInfo_S>;
    m_getTable[NET_GET_CAPTURE_PARAM_INFO] = &CSjclDomain::TemplatedGet<NET_CaptureParamInfo_S>;

    /* ===== 传统报警 ===== */
    m_getTable[NET_GET_TAMPERALARM]            = &CSjclDomain::TemplatedGet<NET_TamperAlarmInfo_S>;
    m_getTable[NET_GET_MOTIONALARM]             = &CSjclDomain::TemplatedGet<NET_MotionAlarmInfo_S>;
    m_getTable[NET_GET_CROSSLINEALARM]          = &CSjclDomain::TemplatedGet<NET_CrossLineAlarmInfo_S>;
    m_getTable[NET_GET_INTRUSIONALARM]           = &CSjclDomain::TemplatedGet<NET_IntrusionAlarmInfo_S>;
    m_getTable[NET_GET_ENTERREGIONALARM]        = &CSjclDomain::TemplatedGet<NET_EnterRegionAlarmInfo_S>;
    m_getTable[NET_GET_LEAVEREGIONALARM]         = &CSjclDomain::TemplatedGet<NET_LeaveRegionAlarmInfo_S>;
    m_getTable[NET_GET_LOITERINGALARM]           = &CSjclDomain::TemplatedGet<NET_LoiteringAlarmInfo_S>;
    m_getTable[NET_GET_SCENECHANGEALARM]         = &CSjclDomain::TemplatedGet<NET_SceneChangeAlarmInfo_S>;
    m_getTable[NET_GET_CROWDGATHERINGALARM]      = &CSjclDomain::TemplatedGet<NET_CrowdGatheringAlarmInfo_S>;
    m_getTable[NET_GET_GARBAGE_EXPOSURE_CFG]     = &CSjclDomain::TemplatedGet<NET_GarbageExposureCfg_S>;
    m_getTable[NET_GET_GARBAGE_OVERFLOW_CFG]     = &CSjclDomain::TemplatedGet<NET_GarbageOverflowCfg_S>;
    m_getTable[NET_GET_PARKINGALARM]             = &CSjclDomain::TemplatedGet<NET_ParkingAlarmInfo_S>;
    m_getTable[NET_GET_UNATTENDEDOBJECTALARM]    = &CSjclDomain::TemplatedGet<NET_UnattendedObjectAlarmInfo_S>;
    m_getTable[NET_GET_OBJECTREMOVALALARM]       = &CSjclDomain::TemplatedGet<NET_ObjectRemovalAlarmInfo_S>;
    m_getTable[NET_GET_AUDIOANOMALYALARM]        = &CSjclDomain::TemplatedGet<NET_AudioAnomalyAlarmInfo_S>;
    m_getTable[NET_GET_AUDIBLE_ALARM_INFO]      = &CSjclDomain::TemplatedGet<NET_AudibleAlarmInfo_S>;
    m_getTable[NET_GET_ALARM_INPUT_INFO]         = &CSjclDomain::TemplatedGet<NET_AlarmInputInfoList_S>;
    m_getTable[NET_GET_ALARM_OUTPUT_INFO]        = &CSjclDomain::TemplatedGet<NET_AlarmOutputInfoList_S>;
    m_getTable[NET_GET_FLASHING_LIGHT_ALARM_INFO]= &CSjclDomain::TemplatedGet<NET_FlashingLightAlarmInfo_S>;
    m_getTable[NET_GET_PIR_ALARM_INFO]          = &CSjclDomain::TemplatedGet<NET_PirAlarmInfo_S>;

    
    /* ===== 预览/对讲 ===== */
    m_getTable[NET_FROM_STREAM_TALKBACK]      = &CSjclDomain::TemplatedGet<NET_TalkbackStreamInfo_S>;
    m_getTable[NET_GET_VOICECOM_AUDIO_CFG]   = &CSjclDomain::TemplatedGet<NET_VoiceComAudioCfg_S>;

    /* ===== 人脸库/人脸抓拍 ===== */
    m_getTable[NET_GET_TARGET_LIB]        = &CSjclDomain::TemplatedGet<NET_FaceLibList_S>;
    m_getTable[NET_GET_FACECAPTUREINFO]   = &CSjclDomain::TemplatedGet<NET_FaceCaptureInfo_S>;
    m_getTable[NET_GET_FACE_INFO]         = &CSjclDomain::TemplatedGet<NET_FaceInfoList_S>;

    /* ===== AI 分析配置 ===== */
    m_getTable[NET_GET_PEOPLE_FLOW_STATISTICS_CFG]        = &CSjclDomain::TemplatedGet<NET_PeopleFlowStatisticsCfg_S>;
    m_getTable[NET_GET_PEOPLE_DENSITY_DETECTION_CFG]      = &CSjclDomain::TemplatedGet<NET_PeopleDensityDetectionCfg_S>;
    m_getTable[NET_GET_MANHOLE_COVER_ABNORMAL_CFG]        = &CSjclDomain::TemplatedGet<NET_ManholeCoverAbnormalCfg_S>;
    m_getTable[NET_GET_SLEEP_ON_DUTY_CFG]                  = &CSjclDomain::TemplatedGet<NET_SleepOnDutyCfg_S>;
    m_getTable[NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG]  = &CSjclDomain::TemplatedGet<NET_ElectricVehicleInElevatorCfg_S>;
    m_getTable[NET_GET_PERSON_FALL_DOWN_CFG]              = &CSjclDomain::TemplatedGet<NET_PersonFallDownCfg_S>;
    m_getTable[NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG]      = &CSjclDomain::TemplatedGet<NET_ConstructionOccupyRoadCfg_S>;
    m_getTable[NET_GET_CONGESTION_CFG]                    = &CSjclDomain::TemplatedGet<NET_CongestionCfg_S>;
    m_getTable[NET_GET_LICENSE_PLATE_RECOGNITION_CFG]    = &CSjclDomain::TemplatedGet<NET_LicensePlateRecognitionCfg_S>;
    m_getTable[NET_GET_HIGH_ALTITUDE_SEATBELT_CFG]       = &CSjclDomain::TemplatedGet<NET_HighAltitudeSeatbeltCfg_S>;
    m_getTable[NET_GET_SAFETY_HELMET_CFG]                 = &CSjclDomain::TemplatedGet<NET_SafetyHelmetCfg_S>;
    m_getTable[NET_GET_PERSON_FALL_CFG]                   = &CSjclDomain::TemplatedGet<NET_PersonFallCfg_S>;
    m_getTable[NET_GET_PHONE_USAGE_CFG]                   = &CSjclDomain::TemplatedGet<NET_PhoneUsageCfg_S>;
    m_getTable[NET_GET_SMOKING_CFG]                       = &CSjclDomain::TemplatedGet<NET_SmokingCfg_S>;
    m_getTable[NET_GET_OPEN_FLAME_CFG]                    = &CSjclDomain::TemplatedGet<NET_OpenFlameCfg_S>;
    m_getTable[NET_GET_BARE_SOIL_CFG]                     = &CSjclDomain::TemplatedGet<NET_BareSoilCfg_S>;
    m_getTable[NET_GET_HOLE_PROTECTION_BAR_CFG]           = &CSjclDomain::TemplatedGet<NET_HoleProtectionBarCfg_S>;
    m_getTable[NET_GET_REFLECTIVE_CLOTHING_CFG]           = &CSjclDomain::TemplatedGet<NET_ReflectiveClothingCfg_S>;
    m_getTable[NET_GET_PET_RECOGNITION_INFO]              = &CSjclDomain::TemplatedGet<NET_PetRecognitionInfo_S>;
    m_getTable[NET_GET_CLIMB_FENCE_INFO]                  = &CSjclDomain::TemplatedGet<NET_ClimbFenceInfo_S>;
    m_getTable[NET_GET_DIMISSION_INFO]                     = &CSjclDomain::TemplatedGet<NET_DimissionInfo_S>;
    m_getTable[NET_GET_ILLEGAL_LANE_INFO]                  = &CSjclDomain::TemplatedGet<NET_IllegalLaneInfo_S>;
    m_getTable[NET_GET_RETROGRADE_INFO]                    = &CSjclDomain::TemplatedGet<NET_RetrogradeInfo_S>;
    m_getTable[NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO]   = &CSjclDomain::TemplatedGet<NET_NonmotorVehicleIntrusionInfo_S>;
    m_getTable[NET_GET_OCCUPATION_EMERGENCY_INFO]         = &CSjclDomain::TemplatedGet<NET_OccupationEmergencyInfo_S>;
    m_getTable[NET_GET_PEDESTRIAN_INTRUSION_INFO]         = &CSjclDomain::TemplatedGet<NET_PedestrianIntrusionInfo_S>;
    m_getTable[NET_GET_SMOKE_FIRE_CFG]                    = &CSjclDomain::TemplatedGet<NET_SmokeFireCfg_S>;
    m_getTable[NET_GET_ROAD_PONDING_CFG]                  = &CSjclDomain::TemplatedGet<NET_RoadPondingCfg_S>;

    /* ===== NVR 专属（委托 CNvrBusiness） ===== */
    m_getTable[NET_FIND_RECORD_FILE_INFO] = [](INT32 ch, INT32 cmd, const std::string&, const std::string& url) -> std::string {
        return CNvrBusiness::instance()->HandleGetRecordFileList(ch, cmd, url);
    };
    m_getTable[NET_GET_CHANNEL_INFO] = [](INT32 ch, INT32 cmd, const std::string&, const std::string&) -> std::string {
        return CNvrBusiness::instance()->HandleGetChannelInfo(ch, cmd);
    };
    m_getTable[NET_GET_RTSPURLCFG] = [](INT32 ch, INT32 cmd, const std::string&, const std::string&) -> std::string {
        return CNvrBusiness::instance()->HandleGetRtspUrl(ch, cmd);
    };

    /* ==================================================================
     * Set 命令注册
     * ================================================================== */

    /* ===== 录像控制/计划/高级参数/下载 ===== */
    m_setTable[NET_CONTROL_RECORD_INFO]       = &CSjclDomain::TemplatedSet<NET_RecordInfo_S>;
    m_setTable[NET_SET_RECORD_SCHEDULE]       = &CSjclDomain::TemplatedSet<NET_RecordSchedule_S>;
    m_setTable[NET_SET_RECORD_ADVANCED_PARAM] = &CSjclDomain::TemplatedSet<NET_RecordAdvancedParam_S>;
    m_setTable[NET_DOWNLOAD_RECORD_FILE]      = &CSjclDomain::TemplatedSet<NET_RecordDownloadList_S>;

    /* ===== 传统报警 ===== */
    m_setTable[NET_SET_TAMPERALARM]              = &CSjclDomain::TemplatedSet<NET_TamperAlarmInfo_S>;
    m_setTable[NET_SET_MOTIONALARM]              = &CSjclDomain::TemplatedSet<NET_MotionAlarmInfo_S>;
    m_setTable[NET_SET_CROSSLINEALARM]           = &CSjclDomain::TemplatedSet<NET_CrossLineAlarmInfo_S>;
    m_setTable[NET_SET_INTRUSIONALARM]           = &CSjclDomain::TemplatedSet<NET_IntrusionAlarmInfo_S>;
    m_setTable[NET_SET_ENTERREGIONALARM]         = &CSjclDomain::TemplatedSet<NET_EnterRegionAlarmInfo_S>;
    m_setTable[NET_SET_LEAVEREGIONALARM]         = &CSjclDomain::TemplatedSet<NET_LeaveRegionAlarmInfo_S>;
    m_setTable[NET_SET_LOITERINGALARM]           = &CSjclDomain::TemplatedSet<NET_LoiteringAlarmInfo_S>;
    m_setTable[NET_SET_SCENECHANGEALARM]         = &CSjclDomain::TemplatedSet<NET_SceneChangeAlarmInfo_S>;
    m_setTable[NET_SET_CROWDGATHERINGALARM]      = &CSjclDomain::TemplatedSet<NET_CrowdGatheringAlarmInfo_S>;
    m_setTable[NET_SET_GARBAGE_EXPOSURE_CFG]     = &CSjclDomain::TemplatedSet<NET_GarbageExposureCfg_S>;
    m_setTable[NET_SET_GARBAGE_OVERFLOW_CFG]     = &CSjclDomain::TemplatedSet<NET_GarbageOverflowCfg_S>;
    m_setTable[NET_SET_PARKINGALARM]             = &CSjclDomain::TemplatedSet<NET_ParkingAlarmInfo_S>;
    m_setTable[NET_SET_UNATTENDEDOBJECTALARM]    = &CSjclDomain::TemplatedSet<NET_UnattendedObjectAlarmInfo_S>;
    m_setTable[NET_SET_OBJECTREMOVALALARM]       = &CSjclDomain::TemplatedSet<NET_ObjectRemovalAlarmInfo_S>;
    m_setTable[NET_SET_AUDIOANOMALYALARM]        = &CSjclDomain::TemplatedSet<NET_AudioAnomalyAlarmInfo_S>;
    m_setTable[NET_SET_AUDIBLE_ALARM_INFO]       = &CSjclDomain::TemplatedSet<NET_AudibleAlarmInfo_S>;
    m_setTable[NET_SET_ALARM_INPUT_INFO]         = &CSjclDomain::TemplatedSet<NET_AlarmInputInfo_S>;
    m_setTable[NET_SET_ALARM_OUTPUT_INFO]        = &CSjclDomain::TemplatedSet<NET_AlarmOutputInfo_S>;
    m_setTable[NET_SET_FLASHING_LIGHT_ALARM_INFO]= &CSjclDomain::TemplatedSet<NET_FlashingLightAlarmInfo_S>;
    m_setTable[NET_SET_PIR_ALARM_INFO]           = &CSjclDomain::TemplatedSet<NET_PirAlarmInfo_S>;

    /* ===== 预览/对讲 ===== */
    m_setTable[NET_SET_PREVIEW_INFO]         = &CSjclDomain::TemplatedSet<NET_PreviewInfo_S>;
    m_setTable[NET_SET_VOICECOM_AUDIO_CFG]   = &CSjclDomain::TemplatedSet<NET_VoiceComAudioCfg_S>;

    /* ===== 抓拍 ===== */
    m_setTable[NET_SET_CAPTURE_PLAN_INFO]   = &CSjclDomain::TemplatedSet<NET_CapturePlanInfo_S>;
    m_setTable[NET_SET_CAPTURE_PARAM_INFO]  = &CSjclDomain::TemplatedSet<NET_CaptureParamInfo_S>;

       /* ===== 对讲状态控制 ===== */
    m_setTable[NET_STATE_TALKBACK]      = &CSjclDomain::TemplatedSet<NET_TalkbackStateInfo_S>;
    m_setTable[NET_TO_STREAM_TALKBACK]  = &CSjclDomain::TemplatedSet<NET_TalkbackStreamInfo_S>;
    m_setTable[NET_REPLAY_TALKBACK]     = &CSjclDomain::TemplatedSet<NET_ReplayTalkbackInfo_S>;

    /* ===== 人脸库/人脸信息 ===== */
    m_setTable[NET_SET_FACECAPTUREINFO]   = &CSjclDomain::TemplatedSet<NET_FaceCaptureInfo_S>;
    m_setTable[NET_SET_FACE_COMPARE_INFO] = &CSjclDomain::TemplatedSet<NET_FaceCompareInfo_S>;
    m_setTable[NET_ADD_TARGET_LIB]        = &CSjclDomain::TemplatedSet<NET_FaceLibInfo_S>;
    m_setTable[NET_DEL_TARGET_LIB]        = &CSjclDomain::TemplatedSet<NET_FaceLibInfo_S>;
    m_setTable[NET_SET_TARGET_LIB]        = &CSjclDomain::TemplatedSet<NET_FaceLibInfo_S>;
    m_setTable[NET_ADD_FACE_INFO]         = &CSjclDomain::TemplatedSet<NET_FaceInfo_S>;
    m_setTable[NET_SET_FACE_INFO]         = &CSjclDomain::TemplatedSet<NET_FaceInfo_S>;
    m_setTable[NET_DEL_FACE_INFO]         = &CSjclDomain::TemplatedSet<NET_FaceIdInfo_S>;

    /* ===== AI 分析配置 ===== */
    m_setTable[NET_SET_PEOPLE_FLOW_STATISTICS_CFG]        = &CSjclDomain::TemplatedSet<NET_PeopleFlowStatisticsCfg_S>;
    m_setTable[NET_RESET_PEOPLE_FLOW_STATISTICS]          = &CSjclDomain::TemplatedSet<NET_PeopleFlowStatisticsCfg_S>;
    m_setTable[NET_SET_PEOPLE_DENSITY_DETECTION_CFG]      = &CSjclDomain::TemplatedSet<NET_PeopleDensityDetectionCfg_S>;
    m_setTable[NET_SET_MANHOLE_COVER_ABNORMAL_CFG]        = &CSjclDomain::TemplatedSet<NET_ManholeCoverAbnormalCfg_S>;
    m_setTable[NET_SET_SLEEP_ON_DUTY_CFG]                 = &CSjclDomain::TemplatedSet<NET_SleepOnDutyCfg_S>;
    m_setTable[NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG]  = &CSjclDomain::TemplatedSet<NET_ElectricVehicleInElevatorCfg_S>;
    m_setTable[NET_SET_PERSON_FALL_DOWN_CFG]              = &CSjclDomain::TemplatedSet<NET_PersonFallDownCfg_S>;
    m_setTable[NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG]      = &CSjclDomain::TemplatedSet<NET_ConstructionOccupyRoadCfg_S>;
    m_setTable[NET_SET_CONGESTION_CFG]                    = &CSjclDomain::TemplatedSet<NET_CongestionCfg_S>;
    m_setTable[NET_SET_LICENSE_PLATE_RECOGNITION_CFG]     = &CSjclDomain::TemplatedSet<NET_LicensePlateRecognitionCfg_S>;
    m_setTable[NET_SET_HIGH_ALTITUDE_SEATBELT_CFG]        = &CSjclDomain::TemplatedSet<NET_HighAltitudeSeatbeltCfg_S>;
    m_setTable[NET_SET_SAFETY_HELMET_CFG]                 = &CSjclDomain::TemplatedSet<NET_SafetyHelmetCfg_S>;
    m_setTable[NET_SET_PERSON_FALL_CFG]                   = &CSjclDomain::TemplatedSet<NET_PersonFallCfg_S>;
    m_setTable[NET_SET_PHONE_USAGE_CFG]                   = &CSjclDomain::TemplatedSet<NET_PhoneUsageCfg_S>;
    m_setTable[NET_SET_SMOKING_CFG]                       = &CSjclDomain::TemplatedSet<NET_SmokingCfg_S>;
    m_setTable[NET_SET_OPEN_FLAME_CFG]                    = &CSjclDomain::TemplatedSet<NET_OpenFlameCfg_S>;
    m_setTable[NET_SET_BARE_SOIL_CFG]                     = &CSjclDomain::TemplatedSet<NET_BareSoilCfg_S>;
    m_setTable[NET_SET_HOLE_PROTECTION_BAR_CFG]           = &CSjclDomain::TemplatedSet<NET_HoleProtectionBarCfg_S>;
    m_setTable[NET_SET_REFLECTIVE_CLOTHING_CFG]           = &CSjclDomain::TemplatedSet<NET_ReflectiveClothingCfg_S>;
    m_setTable[NET_SET_PET_RECOGNITION_INFO]              = &CSjclDomain::TemplatedSet<NET_PetRecognitionInfo_S>;
    m_setTable[NET_SET_CLIMB_FENCE_INFO]                  = &CSjclDomain::TemplatedSet<NET_ClimbFenceInfo_S>;
    m_setTable[NET_SET_DIMISSION_INFO]                    = &CSjclDomain::TemplatedSet<NET_DimissionInfo_S>;
    m_setTable[NET_SET_ILLEGAL_LANE_INFO]                 = &CSjclDomain::TemplatedSet<NET_IllegalLaneInfo_S>;
    m_setTable[NET_SET_RETROGRADE_INFO]                   = &CSjclDomain::TemplatedSet<NET_RetrogradeInfo_S>;
    m_setTable[NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO]   = &CSjclDomain::TemplatedSet<NET_NonmotorVehicleIntrusionInfo_S>;
    m_setTable[NET_SET_OCCUPATION_EMERGENCY_INFO]         = &CSjclDomain::TemplatedSet<NET_OccupationEmergencyInfo_S>;
    m_setTable[NET_SET_PEDESTRIAN_INTRUSION_INFO]         = &CSjclDomain::TemplatedSet<NET_PedestrianIntrusionInfo_S>;
    m_setTable[NET_SET_SMOKE_FIRE_CFG]                    = &CSjclDomain::TemplatedSet<NET_SmokeFireCfg_S>;
    m_setTable[NET_SET_ROAD_PONDING_CFG]                  = &CSjclDomain::TemplatedSet<NET_RoadPondingCfg_S>;
}

#endif /* BU_SJCL_EXCLUDE */
