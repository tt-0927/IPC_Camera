/**
 * @FilePath     : event_configure.cpp
* @Author       : zhangjc (zhangjc@kfb.cn)
* @Date         : 2024-12-14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-20 15:13:49
* @Descripttion : 事件配置
*/

#include "alarm_define.h"
#include "event_configure.h"
#include "path_define.h"

CEventConfigure::CEventConfigure():
      m_algorithmConfig(EVENT_ALGORITHM_CONFIG_FILE),
      m_eventSchedule(EVENT_SCHEDULE_CONFIG_FILE),
      m_smartEventEnableStatus(SMART_EVENT_ENABLE_STATUS_FILE),
      m_metadataConfig(METADATA_CONFIG_FILE),
      m_motionAlarm(EVENT_MOTION_DETECTION_CONFIG_FILE),
      m_HideAlarm(EVENT_HIDE_ALARM_CONFIG_FILE),
      m_AbnormalAlarm(EVENT_ABNORMAL_ALARM_CONFIG_FILE),
      m_SoundOutAlarm(EVENT_SOUND_ALARM_OUTPUT_CONFIG_FILE),
      m_IoInputAlarm(EVENT_ALARM_INPUT_CONFIG_FILE),
      m_IoOutputAlarm(EVENT_ALARM_OUTPUT_CONFIG_FILE),
      m_FlashAlarm(EVENT_FLASHING_LIGHT_ALARM_OUTPUT_CONFIG_FILE),
      m_ManualSoundLightAlarm(EVENT_MANUAL_SOUND_LIGHT_ALARM_CONFIG_FILE),
      m_PirAlarm(EVENT_PIR_ALARM_CONFIG_FILE),
      m_BoundaryDetection(EVENT_LINE_CROSSING_DETECTION_CONFIG_FILE),
      m_FieldDetection(EVENT_REGIONAL_INTRUSION_DETECTION_CONFIG_FILE),
      m_EntranceDetection(EVENT_ENTER_REGION_DETECTION_CONFIG_FILE),
      m_ExitDetection(EVENT_LEAVE_REGION_DETECTION_CONFIG_FILE),
      m_audioAnomaly(EVENT_AUDIO_ANOMALY_DETECTION_CONFIG_FILE),
      m_sceneChange(EVENT_SCENE_CHANGE_DETECTION_CONFIG_FILE),
      m_faceDetection(EVENT_FACE_DETECTION_CONFIG_FILE),
      m_loiteringDetection(EVENT_LOITERING_DETECTION_CONFIG_FILE),
      m_crowdGathering(EVENT_CROWD_GATHERING_DETECTION_CONFIG_FILE),
      m_parkingDetection(EVENT_PARKING_DETECTION_CONFIG_FILE),
      m_unattendedObject(EVENT_UNATTENDED_OBJECT_DETECTION_CONFIG_FILE),
      m_objectRemoval(EVENT_OBJECT_REMOVAL_DETECTION_CONFIG_FILE),
      m_petRecognition(EVENT_PET_RECOGNITION_CONFIG_FILE),
      m_faceCapture(EVENT_FACE_CAPTURE_CONFIG_FILE),
      m_faceCompare(EVENT_FACE_COMPARE_CONFIG_FILE),
      m_overlayInfo(EVENT_FACE_CAPTURE_OVERLAY_INFO_FILE)
#ifdef SCENE_INTELLIGENT_ANALYSIS
      ,
      m_AiSceneAnalysis(EVENT_AI_LLM_AI_SCENE_ANALYSIS_INFO_FILE),
      m_imageAnalysis(EVENT_AI_LLMIMAGE_ANALYSIS_INFO_FILE),
      m_imageAnalysisRecord(EVENT_AI_LLMIMAGE_ANALYSIS_RECORD_FILE),
      m_TextPresetAnalysis(EVENT_TEXT_PRESET_ANALYSIS_FILE),
      m_TextPresetTaskManager(EVENT_TEXT_PRESET_TASK_MANAGER_FILE),
      m_RealPushAlarmManager(EVENT_REAL_PUSH_ALARM_MANAGER_FILE)
#endif
#ifdef SCENE_INTELLIGENCE
      ,
      m_fenceClimbing(EVENT_FENCE_CLIMBING_INFO_FILE),
      m_leavePost(EVENT_LEAVE_POST_INFO_FILE),
      m_pedestrianIntrusion(EVENT_PEDESTRIAN_INTRUSION_INFO_FILE),
      m_smokeFire(EVENT_SMOKE_FIRE_INFO_FILE),
      m_openFlame(EVENT_OPEN_FLAME_INFO_FILE),
      m_roadPonding(EVENT_ROAD_PONDING_INFO_FILE),
      m_manholeCoverAbnormal(EVENT_MANHOLE_COVER_ABNORMAL_INFO_FILE),
      m_SleepOnDuty(EVENT_SLEEP_ON_DUTY_INFO_FILE),
      m_trip(EVENT_PERSON_TRIP_INFO_FILE),
      m_personFallDown(EVENT_PERSON_FALL_DOWN_INFO_FILE),
      m_phoneUsage(EVENT_PHONE_USAGE_INFO_FILE),
      m_highAltitudeSeatbelt(EVENT_HIGH_ALTITUDE_SEATBELT_INFO_FILE),
      m_bareSoilet(EVENT_BARE_SOIL_INFO_FILE),
      m_safetyHelmet(EVENT_SAFETY_HELMET_INFO_FILE),
      m_holeProtectionBar(EVENT_HOLE_PROTECTION_BAR_INFO_FILE),
      m_reflectiveClothing(EVENT_REFLECTIVE_CLOTHING_INFO_FILE),
      m_smoking(EVENT_SMOKINGE_INFO_FILE),
      m_constructionEncroachmentRoad(EVENT_CONSTRUCTION_ENCROACHMENT_ROAD_INFO_FILE),
      m_ElectricScooter(EVENT_ELECTRIC_SCOOTER_INFO_FILE),
      m_licensePlateCognition(EVENT_LICENSE_PLATE_COGNITION_INFO_FILE),
      m_drivingAgainstTrafficDetection(EVENT_DRIVING_AGAINST_TRAFFIC_INFO_FILE),
      m_illegalLaneChangeDetection(EVENT_ILLEGAL_LANE_CHANGE_INFO_FILE),
      m_congestionDetection(EVENT_CONGESTION_INFO_FILE),
      m_emergencyLaneOccupancyDetection(EVENT_EMERGENCY_LANE_OCCUPANCY_INFO_FILE),
      m_nonMotorVehicleIntrusionDetection(EVENT_NON_MOTOR_VEHICLE_INTRUSION_INFO_FILE),
      m_AttributeDetectSwitch(ATTRIBUTE_DETECT_INFO_FILE)
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
      ,
      m_garbageExposure(EVENT_GARBAGE_EXPOSURE_INFO_FILE),
      m_garbageOverflow(EVENT_GARBAGE_OVERFLOW_INFO_FILE)
#endif
#if CAP_AI_PEOPLE_STATISTICS
      ,
      m_peopleFlowStatistics(EVENT_PEOPLE_FLOW_STATISTICS_CONFIG_FILE),
      m_peopleDensityDetection(EVENT_PEOPLE_DENSITY_DETECTION_CONFIG_FILE)
#endif
{
}

CEventConfigure::~CEventConfigure() {}

int CEventConfigure::set_configure(const Event::AlgorithmConfig_S &alarm)
{
    return m_algorithmConfig.set(alarm);
}

int CEventConfigure::get_configure(Event::AlgorithmConfig_S &alarm) const
{
    return m_algorithmConfig.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::EventSchedule_S &alarm)
{
    return m_eventSchedule.set(alarm);
}

int CEventConfigure::get_configure(Alarm::EventSchedule_S &alarm) const
{
    return m_eventSchedule.get(alarm);
}

int CEventConfigure::get_configure(std::set<Alarm::EventSchedule_S> &alarm) const
{
    return m_eventSchedule.get(alarm);
}

int CEventConfigure::set_configure(const Event::SmartEventEnableStatus_S &alarm)
{
    return m_smartEventEnableStatus.set(alarm);
}

int CEventConfigure::get_configure(Event::SmartEventEnableStatus_S &alarm) const
{
    return m_smartEventEnableStatus.get(alarm);
}

int CEventConfigure::set_configure(const Event::MetadataConfig_S &alarm)
{
    return m_metadataConfig.set(alarm);
}

int CEventConfigure::get_configure(Event::MetadataConfig_S &alarm) const
{
    return m_metadataConfig.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::MotionDetection_S &stMotionAlarm)
{
    return m_motionAlarm.set(stMotionAlarm);
}

int CEventConfigure::get_configure(Alarm::MotionDetection_S &stMotionAlarm) const
{
    return m_motionAlarm.get(stMotionAlarm);
}

int CEventConfigure::set_configure(const Alarm::HideAlarm_S &stHideAlarm)
{
    return m_HideAlarm.set(stHideAlarm);
}

int CEventConfigure::get_configure(Alarm::HideAlarm_S &stHideAlarm) const
{
    return m_HideAlarm.get(stHideAlarm);
}

int CEventConfigure::set_configure(const Alarm::AbnormalDetection_S &stAbnormalAlarm)
{
    return m_AbnormalAlarm.set(stAbnormalAlarm);
}

int CEventConfigure::get_configure(Alarm::AbnormalDetection_S &stAbnormalAlarm) const
{
	return m_AbnormalAlarm.get(stAbnormalAlarm);
}

int CEventConfigure::get_configure(std::set<Alarm::AbnormalDetection_S> &stAbnormalAlarm) const
{
    return m_AbnormalAlarm.get(stAbnormalAlarm);
}

int CEventConfigure::set_configure(const Alarm::SoundOutputAlarm_S &stSoundOutAlarm)
{
    return m_SoundOutAlarm.set(stSoundOutAlarm);
}

int CEventConfigure::get_configure(Alarm::SoundOutputAlarm_S &stSoundOutAlarm) const
{
    return m_SoundOutAlarm.get(stSoundOutAlarm);
}

int CEventConfigure::set_configure(const Alarm::IoInputInfo_S &stIoInputAlarm)
{
    return m_IoInputAlarm.set(stIoInputAlarm);
}

int CEventConfigure::get_configure(Alarm::IoInputInfo_S &stIoInputAlarm) const
{
    return m_IoInputAlarm.get(stIoInputAlarm);
}
int CEventConfigure::get_configure(std::set<Alarm::IoInputInfo_S> &ioInputInfos) const
{
    return m_IoInputAlarm.get(ioInputInfos);
}
int CEventConfigure::set_configure(const Alarm::IoOutputInfo_S &stIoOutputAlarm)
{
    return m_IoOutputAlarm.set(stIoOutputAlarm);
}

int CEventConfigure::get_configure(Alarm::IoOutputInfo_S &stIoOutputAlarm) const
{
    return m_IoOutputAlarm.get(stIoOutputAlarm);
}
int CEventConfigure::get_configure(std::set<Alarm::IoOutputInfo_S> &ioInputInfos) const
{
    return m_IoOutputAlarm.get(ioInputInfos);
}

int CEventConfigure::set_configure(const Alarm::FlashInfo_S &stFlashAlarm)
{
    return m_FlashAlarm.set(stFlashAlarm);
}

int CEventConfigure::get_configure(Alarm::FlashInfo_S &stFlashAlarm) const
{
    return m_FlashAlarm.get(stFlashAlarm);
}

/**
 * @brief 保存手动声光报警联动配置。
 * @param [in] stManualSoundLightAlarm 本次手动触发使用的联动配置。
 * @return 保存成功返回 OK，失败返回错误码。
 */
int CEventConfigure::set_configure(const Alarm::LinkageList_S &stManualSoundLightAlarm)
{
    return m_ManualSoundLightAlarm.set(stManualSoundLightAlarm);
}

/**
 * @brief 获取手动声光报警联动配置。
 * @param [out] stManualSoundLightAlarm 已保存的手动触发联动配置。
 * @return 获取成功返回 OK，失败返回错误码。
 */
int CEventConfigure::get_configure(Alarm::LinkageList_S &stManualSoundLightAlarm) const
{
    return m_ManualSoundLightAlarm.get(stManualSoundLightAlarm);
}

int CEventConfigure::set_configure(const Alarm::PirAlarmInfo_S &stPirAlarm)
{
    return m_PirAlarm.set(stPirAlarm);
}

int CEventConfigure::get_configure(Alarm::PirAlarmInfo_S &stPirAlarm) const
{
    return m_PirAlarm.get(stPirAlarm);
}

int CEventConfigure::set_configure(const Alarm::BoundaryDetection_S &stInfo)
{
    return m_BoundaryDetection.set(stInfo);
}

int CEventConfigure::get_configure(Alarm::BoundaryDetection_S &stInfo) const
{
    return m_BoundaryDetection.get(stInfo);
}

int CEventConfigure::set_configure(const Alarm::FieldDetection_S &stInfo)
{
    return m_FieldDetection.set(stInfo);
}

int CEventConfigure::get_configure(Alarm::FieldDetection_S &stInfo) const
{
    return m_FieldDetection.get(stInfo);
}

int CEventConfigure::set_configure(const Alarm::EntranceDetection_S &stInfo)
{
    return m_EntranceDetection.set(stInfo);
}

int CEventConfigure::get_configure(Alarm::EntranceDetection_S &stInfo) const
{
    return m_EntranceDetection.get(stInfo);
}

int CEventConfigure::set_configure(const Alarm::ExitingDetection_S &stInfo)
{
    return m_ExitDetection.set(stInfo);
}

int CEventConfigure::get_configure(Alarm::ExitingDetection_S &stInfo) const
{
    return m_ExitDetection.get(stInfo);
}

int CEventConfigure::set_configure(const Alarm::AudioAnomaly_S &alarm)
{
    return m_audioAnomaly.set(alarm);
}

int CEventConfigure::get_configure(Alarm::AudioAnomaly_S &alarm) const
{
    return m_audioAnomaly.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::SceneChange_S &alarm)
{
    return m_sceneChange.set(alarm);
}

int CEventConfigure::get_configure(Alarm::SceneChange_S &alarm) const
{
    return m_sceneChange.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::FaceDetection_S &alarm)
{
    return m_faceDetection.set(alarm);
}

int CEventConfigure::get_configure(Alarm::FaceDetection_S &alarm) const
{
    return m_faceDetection.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::LoiteringDetection_S &alarm)
{
    return m_loiteringDetection.set(alarm);
}

int CEventConfigure::get_configure(Alarm::LoiteringDetection_S &alarm) const
{
    return m_loiteringDetection.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::CrowdGathering_S &alarm)
{
    return m_crowdGathering.set(alarm);
}

int CEventConfigure::get_configure(Alarm::CrowdGathering_S &alarm) const
{
    return m_crowdGathering.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::ParkingDetection_S &alarm)
{
    return m_parkingDetection.set(alarm);
}

int CEventConfigure::get_configure(Alarm::ParkingDetection_S &alarm) const
{
    return m_parkingDetection.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::UnattendedObject_S &alarm)
{
    return m_unattendedObject.set(alarm);
}

int CEventConfigure::get_configure(Alarm::UnattendedObject_S &alarm) const
{
    return m_unattendedObject.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::ObjectRemoval_S &alarm)
{
    return m_objectRemoval.set(alarm);
}

int CEventConfigure::get_configure(Alarm::ObjectRemoval_S &alarm) const
{
    return m_objectRemoval.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::PetRecognition_S &alarm)
{
    return m_petRecognition.set(alarm);
}

int CEventConfigure::get_configure(Alarm::PetRecognition_S &alarm) const
{
    return m_petRecognition.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::FaceCapture_S &alarm)
{
    return m_faceCapture.set(alarm);
}

int CEventConfigure::get_configure(Alarm::FaceCapture_S &alarm) const
{
    return m_faceCapture.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::FaceCompare_S &alarm)
{
    return m_faceCompare.set(alarm);
}

int CEventConfigure::get_configure(Alarm::FaceCompare_S &alarm) const
{
    return m_faceCompare.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::OverlayInfo_S &alarm)
{
    return m_overlayInfo.set(alarm);
}

int CEventConfigure::get_configure(Alarm::OverlayInfo_S &alarm) const
{
    return m_overlayInfo.get(alarm);
}

#ifdef SCENE_INTELLIGENT_ANALYSIS
int CEventConfigure::set_configure(const Alarm::LLMAISceneAnalysis_S &alarm)
{
    return m_AiSceneAnalysis.set(alarm);
}

int CEventConfigure::get_configure(Alarm::LLMAISceneAnalysis_S &alarm) const
{
    return m_AiSceneAnalysis.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::LLMImageAnalysis_S &alarm)
{
    return m_imageAnalysis.set(alarm);
}

int CEventConfigure::get_configure(Alarm::LLMImageAnalysis_S &alarm) const
{
    return m_imageAnalysis.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::AnalysisAllRecordIndexItem_S &alarm)
{
    return m_imageAnalysisRecord.set(alarm);
}

int CEventConfigure::get_configure(Alarm::AnalysisAllRecordIndexItem_S &alarm) const
{
    return m_imageAnalysisRecord.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::TextPreset_S &alarm)
{
    return m_TextPresetAnalysis.set(alarm);
}

int CEventConfigure::get_configure(Alarm::TextPreset_S &alarm) const
{
    return m_TextPresetAnalysis.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::TextPresetTaskManager_S &alarm)
{
    return m_TextPresetTaskManager.set(alarm);
}

int CEventConfigure::get_configure(Alarm::TextPresetTaskManager_S &alarm) const
{
    return m_TextPresetTaskManager.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::RealAlarmPushManager_S &alarm)
{
    return m_RealPushAlarmManager.set(alarm);
}

int CEventConfigure::get_configure(Alarm::RealAlarmPushManager_S &alarm) const
{
    return m_RealPushAlarmManager.get(alarm);
}
#endif

#ifdef SCENE_INTELLIGENCE
int CEventConfigure::set_configure(const Alarm::FenceClimbingDetection_S &alarm)
{
    return m_fenceClimbing.set(alarm);
}

int CEventConfigure::get_configure(Alarm::FenceClimbingDetection_S &alarm) const
{
    return m_fenceClimbing.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::LeavePostDetection_S &alarm)
{
    return m_leavePost.set(alarm);
}

int CEventConfigure::get_configure(Alarm::LeavePostDetection_S &alarm) const
{
    return m_leavePost.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::PedestrianIntrusionDetection_S &alarm)
{
    return m_pedestrianIntrusion.set(alarm);
}

int CEventConfigure::get_configure(Alarm::PedestrianIntrusionDetection_S &alarm) const
{
    return m_pedestrianIntrusion.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::SmokeFireDetection_S &alarm)
{
    return m_smokeFire.set(alarm);
}

int CEventConfigure::get_configure(Alarm::SmokeFireDetection_S &alarm) const
{
    return m_smokeFire.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::OpenFlameDetection_S &alarm)
{
    return m_openFlame.set(alarm);
}

int CEventConfigure::get_configure(Alarm::OpenFlameDetection_S &alarm) const
{
    return m_openFlame.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::RoadPondingDetection_S &alarm)
{
    return m_roadPonding.set(alarm);
}

int CEventConfigure::get_configure(Alarm::RoadPondingDetection_S &alarm) const
{
    return m_roadPonding.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::ManholeCoverAbnormalDetection_S &alarm)
{
    return m_manholeCoverAbnormal.set(alarm);
}

int CEventConfigure::get_configure(Alarm::ManholeCoverAbnormalDetection_S &alarm) const
{
    return m_manholeCoverAbnormal.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::SleepOnDutyDetection_S &alarm)
{
    return m_SleepOnDuty.set(alarm);
}

int CEventConfigure::get_configure(Alarm::SleepOnDutyDetection_S &alarm) const
{
    return m_SleepOnDuty.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::TripDetection_S &alarm)
{
    return m_trip.set(alarm);
}

int CEventConfigure::get_configure(Alarm::TripDetection_S &alarm) const
{
    return m_trip.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::PersonFallDownDetection_S &alarm)
{
    return m_personFallDown.set(alarm);
}

int CEventConfigure::get_configure(Alarm::PersonFallDownDetection_S &alarm) const
{
    return m_personFallDown.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::PhoneUsageDetection_S &alarm)
{
    return m_phoneUsage.set(alarm);
}

int CEventConfigure::get_configure(Alarm::PhoneUsageDetection_S &alarm) const
{
    return m_phoneUsage.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::HighAltitudeSeatbeltDetection_S &alarm)
{
    return m_highAltitudeSeatbelt.set(alarm);
}

int CEventConfigure::get_configure(Alarm::HighAltitudeSeatbeltDetection_S &alarm) const
{
    return m_highAltitudeSeatbelt.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::BareSoiletDection_S &alarm)
{
    return m_bareSoilet.set(alarm);
}

int CEventConfigure::get_configure(Alarm::BareSoiletDection_S &alarm) const
{
    return m_bareSoilet.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::SafetyHelmetDection_S &alarm)
{
    return m_safetyHelmet.set(alarm);
}

int CEventConfigure::get_configure(Alarm::SafetyHelmetDection_S &alarm) const
{
    return m_safetyHelmet.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::HoleProtectionBarDection_S &alarm)
{
    return m_holeProtectionBar.set(alarm);
}

int CEventConfigure::get_configure(Alarm::HoleProtectionBarDection_S &alarm) const
{
    return m_holeProtectionBar.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::ReflectiveClothingDection_S &alarm)
{
    return m_reflectiveClothing.set(alarm);
}

int CEventConfigure::get_configure(Alarm::ReflectiveClothingDection_S &alarm) const
{
    return m_reflectiveClothing.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::SmokingDection_S &alarm)
{
    return m_smoking.set(alarm);
}

int CEventConfigure::get_configure(Alarm::SmokingDection_S &alarm) const
{
    return m_smoking.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::ConstructionEncroachmentRoadDetection_S &alarm)
{
    return m_constructionEncroachmentRoad.set(alarm);
}

int CEventConfigure::get_configure(Alarm::ConstructionEncroachmentRoadDetection_S &alarm) const
{
    return m_constructionEncroachmentRoad.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::ElectricScooterDetection_S &alarm)
{
    return m_ElectricScooter.set(alarm);
}

int CEventConfigure::get_configure(Alarm::ElectricScooterDetection_S &alarm) const
{
    return m_ElectricScooter.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::LicensePlateCognitionDetection_S &alarm)
{
    return m_licensePlateCognition.set(alarm);
}

int CEventConfigure::get_configure(Alarm::LicensePlateCognitionDetection_S &alarm) const
{
    return m_licensePlateCognition.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::DrivingAgainstTrafficDetection_S &alarm)
{
    return m_drivingAgainstTrafficDetection.set(alarm);
}

int CEventConfigure::get_configure(Alarm::DrivingAgainstTrafficDetection_S &alarm) const
{
    return m_drivingAgainstTrafficDetection.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::IllegalLaneChangeDetection_S &alarm)
{
    return m_illegalLaneChangeDetection.set(alarm);
}

int CEventConfigure::get_configure(Alarm::IllegalLaneChangeDetection_S &alarm) const
{
    return m_illegalLaneChangeDetection.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::CongestionDetection_S &alarm)
{
    return m_congestionDetection.set(alarm);
}

int CEventConfigure::get_configure(Alarm::CongestionDetection_S &alarm) const
{
    return m_congestionDetection.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::EmergencyLaneOccupancyDetection_S &alarm)
{
    return m_emergencyLaneOccupancyDetection.set(alarm);
}

int CEventConfigure::get_configure(Alarm::EmergencyLaneOccupancyDetection_S &alarm) const
{
    return m_emergencyLaneOccupancyDetection.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::NonMotorVehicleIntrusionDetection_S &alarm)
{
    return m_nonMotorVehicleIntrusionDetection.set(alarm);
}

int CEventConfigure::get_configure(Alarm::NonMotorVehicleIntrusionDetection_S &alarm) const
{
    return m_nonMotorVehicleIntrusionDetection.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::AttributeDetectSwitch_S &alarm)
{
    return m_AttributeDetectSwitch.set(alarm);
}

int CEventConfigure::get_configure(Alarm::AttributeDetectSwitch_S &alarm) const
{
    return m_AttributeDetectSwitch.get(alarm);
}

#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
int CEventConfigure::set_configure(const Alarm::GarbageExposureDetection_S &alarm)
{
    return m_garbageExposure.set(alarm);
}

int CEventConfigure::get_configure(Alarm::GarbageExposureDetection_S &alarm) const
{
    return m_garbageExposure.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::GarbageOverflowDetection_S &alarm)
{
    return m_garbageOverflow.set(alarm);
}

int CEventConfigure::get_configure(Alarm::GarbageOverflowDetection_S &alarm) const
{
    return m_garbageOverflow.get(alarm);
}
#endif

#if CAP_AI_PEOPLE_STATISTICS
int CEventConfigure::set_configure(const Alarm::PeopleFlowStatistics_S &alarm)
{
    return m_peopleFlowStatistics.set(alarm);
}

int CEventConfigure::get_configure(Alarm::PeopleFlowStatistics_S &alarm) const
{
    return m_peopleFlowStatistics.get(alarm);
}

int CEventConfigure::set_configure(const Alarm::PeopleDensityDetection_S &alarm)
{
    return m_peopleDensityDetection.set(alarm);
}

int CEventConfigure::get_configure(Alarm::PeopleDensityDetection_S &alarm) const
{
    return m_peopleDensityDetection.get(alarm);
}
#endif
