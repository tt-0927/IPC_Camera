#pragma once

#include "system_define.h"
#include "video_define.h"
#include "network_define.h"
#include "alarm_define.h"
#include "replay_define.h"

#include "NetTVSDKServer.h"

/**
 * @brief IPC <-> TVSDK 结构体转换
 *
 * 只做“结构到结构”的拷贝/映射，不关心命令码与 JSON。
 */
namespace TvSdkConvert
{
void FillDeviceInfo(const ::System::DeviceInfo_S &src, NET_TV_DEVICE_INFO_S &dst);

void FillDeviceBasicInfo(const ::System::DeviceInfo_S &src, NET_TV_DEVICE_BASICINFO_S &dst);
void ToDeviceInfo(const NET_TV_DEVICE_BASICINFO_S &src, ::System::DeviceInfo_S &dst);

void FillNetworkCfg(const Network::Info_S &src, NET_TV_NETWORKCFG_S &dst);
void ToNetworkInfo(const NET_TV_NETWORKCFG_S &src, Network::Info_S &dst);
void FillWifiStaCfg(const Network::WifiStaInfo_S &src, NET_TV_WIFI_STA_CFG_S &dst);
void ToWifiStaInfo(const NET_TV_WIFI_STA_CFG_S &src, Network::WifiStaInfo_S &dst);
void FillWifiStaConnect(const Network::WifiStaConncet_S &src, NET_TV_WIFI_STA_CONNECT_S &dst);
void ToWifiStaConnect(const NET_TV_WIFI_STA_CONNECT_S &src, Network::WifiStaConncet_S &dst);
void Fill4GInfo(const Network::Network_4G_Config_t &src, NET_TV_4G_INFO_S &dst);
void To4GConfig(const NET_TV_4G_INFO_S &src, Network::Network_4G_Config_t &dst);
void FillHotspotInfo(const Network::HotspotConfig &src, NET_TV_HOTSPOT_INFO_S &dst);
void ToHotspotConfig(const NET_TV_HOTSPOT_INFO_S &src, Network::HotspotConfig &dst);
void FillVideoEncodeOption(const Video_NS::VideoConfig_S &src, NET_TV_VIDEO_ENCODE_OPTION_S &dst);
void ToVideoConfig(const NET_TV_VIDEO_ENCODE_OPTION_S &src, Video_NS::VideoConfig_S &dst);

void ToUpgradeInfo(const NET_TV_UPGRADE_INFO_S &src, ::System::UpgradeInfo_S &dst);
void FillUpgradeStatus(const ::System::UpgradeStatus_S &src, NET_TV_UPGRADE_STATUS_S &dst);
void FillUpgradeVersion(const ::System::UpgradeVersion_S &src, NET_TV_UPGRADE_VERSION_S &dst);

void FillCapturePlan(const Capture_NS::CapturePlan_S &src, NET_TV_CAPTURE_PLAN_INFO_S &dst);
void ToCapturePlan(const NET_TV_CAPTURE_PLAN_INFO_S &src, Capture_NS::CapturePlan_S &dst);
void FillCaptureParam(const Capture_NS::CaptureParam_S &src, NET_TV_CAPTURE_PARAM_INFO_S &dst);
void ToCaptureParam(const NET_TV_CAPTURE_PARAM_INFO_S &src, Capture_NS::CaptureParam_S &dst);

void FillVideoEncodeCap(const Video_NS::VideoCapabilitySet_S &src, NET_TV_VIDEO_ENCODE_CAP_S &dst);
void FillAudioEncodeCap(const Audio_NS::AudioCapabilitySet_S &src, NET_TV_AUDIO_CAP_S &dst);
void FillVideoOSDCap(const Video_NS::VideoCapabilitySet_S &src, NET_TV_OSD_CAP_S &dst);

// --------- 智能告警配置（对应 NET_TV_GET/SET_*ALARM） ---------
void FillMotionAlarmInfo(const Alarm::MotionDetection_S &src, NET_TV_MOTION_ALARM_INFO_S &dst);
void ToMotionDetection(const NET_TV_MOTION_ALARM_INFO_S &src, Alarm::MotionDetection_S &dst);

void FillTamperAlarmInfo(const Alarm::HideAlarm_S &src, NET_TV_TAMPER_ALARM_INFO_S &dst);
void ToHideAlarm(const NET_TV_TAMPER_ALARM_INFO_S &src, Alarm::HideAlarm_S &dst);

void FillCrossLineAlarmInfo(const Alarm::BoundaryDetection_S &src, NET_TV_CROSS_LINE_ALARM_INFO_S &dst);
void ToBoundaryDetection(const NET_TV_CROSS_LINE_ALARM_INFO_S &src, Alarm::BoundaryDetection_S &dst);

void FillIntrusionAlarmInfo(const Alarm::FieldDetection_S &src, NET_TV_INTRUSION_ALARM_INFO_S &dst);
void ToFieldDetection(const NET_TV_INTRUSION_ALARM_INFO_S &src, Alarm::FieldDetection_S &dst);

void FillLoiteringAlarmInfo(const Alarm::LoiteringDetection_S &src, NET_TV_LOITERING_ALARM_INFO_S &dst);
void ToLoiteringDetection(const NET_TV_LOITERING_ALARM_INFO_S &src, Alarm::LoiteringDetection_S &dst);

void FillSceneChangeAlarmInfo(const Alarm::SceneChange_S &src, NET_TV_SCENE_CHANGE_ALARM_INFO_S &dst);
void ToSceneChange(const NET_TV_SCENE_CHANGE_ALARM_INFO_S &src, Alarm::SceneChange_S &dst);

void FillCrowdGatheringAlarmInfo(const Alarm::CrowdGathering_S &src, NET_TV_CROWD_GATHERING_ALARM_INFO_S &dst);
void ToCrowdGathering(const NET_TV_CROWD_GATHERING_ALARM_INFO_S &src, Alarm::CrowdGathering_S &dst);

void FillParkingDetectAlarmInfo(const Alarm::ParkingDetection_S &src, NET_TV_PARKING_ALARM_INFO_S &dst);
void ToParkingDetection(const NET_TV_PARKING_ALARM_INFO_S &src, Alarm::ParkingDetection_S &dst);

void FillUnattendedObjectAlarmInfo(const Alarm::UnattendedObject_S &src, NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S &dst);
void ToUnattendedObject(const NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S &src, Alarm::UnattendedObject_S &dst);

void FillObjectRemovalAlarmInfo(const Alarm::ObjectRemoval_S &src, NET_TV_OBJECT_REMOVAL_ALARM_INFO_S &dst);
void ToObjectRemoval(const NET_TV_OBJECT_REMOVAL_ALARM_INFO_S &src, Alarm::ObjectRemoval_S &dst);

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
void FillGarbageExposureCfg(const Alarm::GarbageExposureDetection_S &src, NET_TV_GARBAGE_EXPOSURE_CFG_S &dst);
void ToGarbageExposure(const NET_TV_GARBAGE_EXPOSURE_CFG_S &src, Alarm::GarbageExposureDetection_S &dst);

void FillGarbageOverflowCfg(const Alarm::GarbageOverflowDetection_S &src, NET_TV_GARBAGE_OVERFLOW_CFG_S &dst);
void ToGarbageOverflow(const NET_TV_GARBAGE_OVERFLOW_CFG_S &src, Alarm::GarbageOverflowDetection_S &dst);
#endif

#ifdef SCENE_INTELLIGENCE
void FillManholeCoverAbnormalCfg(const Alarm::ManholeCoverAbnormalDetection_S &src, NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S &dst);
void ToManholeCoverAbnormal(const NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S &src, Alarm::ManholeCoverAbnormalDetection_S &dst);
void FillSleepOnDutyCfg(const Alarm::SleepOnDutyDetection_S &src, NET_TV_SLEEP_ON_DUTY_CFG_S &dst);
void ToSleepOnDuty(const NET_TV_SLEEP_ON_DUTY_CFG_S &src, Alarm::SleepOnDutyDetection_S &dst);
void FillElectricVehicleInElevatorCfg(const Alarm::ElectricScooterDetection_S &src, NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S &dst);
void ToElectricVehicleInElevator(const NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S &src, Alarm::ElectricScooterDetection_S &dst);
void FillPersonFallDownCfg(const Alarm::PersonFallDownDetection_S &src, NET_TV_PERSON_FALL_DOWN_CFG_S &dst);
void ToPersonFallDown(const NET_TV_PERSON_FALL_DOWN_CFG_S &src, Alarm::PersonFallDownDetection_S &dst);
void FillConstructionOccupyRoadCfg(const Alarm::ConstructionEncroachmentRoadDetection_S &src, NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S &dst);
void ToConstructionOccupyRoad(const NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S &src, Alarm::ConstructionEncroachmentRoadDetection_S &dst);
void FillCongestionCfg(const Alarm::CongestionDetection_S &src, NET_TV_CONGESTION_CFG_S &dst);
void ToCongestion(const NET_TV_CONGESTION_CFG_S &src, Alarm::CongestionDetection_S &dst);
void FillLicensePlateRecognitionCfg(const Alarm::LicensePlateCognitionDetection_S &src, NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S &dst);
void ToLicensePlateRecognition(const NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S &src, Alarm::LicensePlateCognitionDetection_S &dst);
void FillHighAltitudeSeatbeltCfg(const Alarm::HighAltitudeSeatbeltDetection_S &src, NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S &dst);
void ToHighAltitudeSeatbelt(const NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S &src, Alarm::HighAltitudeSeatbeltDetection_S &dst);
void FillSafetyHelmetCfg(const Alarm::SafetyHelmetDection_S &src, NET_TV_SAFETY_HELMET_CFG_S &dst);
void ToSafetyHelmet(const NET_TV_SAFETY_HELMET_CFG_S &src, Alarm::SafetyHelmetDection_S &dst);
void FillPersonFallCfg(const Alarm::TripDetection_S &src, NET_TV_PERSON_FALL_CFG_S &dst);
void ToPersonFall(const NET_TV_PERSON_FALL_CFG_S &src, Alarm::TripDetection_S &dst);
void FillPhoneUsageCfg(const Alarm::PhoneUsageDetection_S &src, NET_TV_PHONE_USAGE_CFG_S &dst);
void ToPhoneUsage(const NET_TV_PHONE_USAGE_CFG_S &src, Alarm::PhoneUsageDetection_S &dst);
void FillSmokingCfg(const Alarm::SmokingDection_S &src, NET_TV_SMOKING_CFG_S &dst);
void ToSmoking(const NET_TV_SMOKING_CFG_S &src, Alarm::SmokingDection_S &dst);
void FillOpenFlameCfg(const Alarm::OpenFlameDetection_S &src, NET_TV_OPEN_FLAME_CFG_S &dst);
void ToOpenFlame(const NET_TV_OPEN_FLAME_CFG_S &src, Alarm::OpenFlameDetection_S &dst);
void FillBareSoilCfg(const Alarm::BareSoiletDection_S &src, NET_TV_BARE_SOIL_CFG_S &dst);
void ToBareSoil(const NET_TV_BARE_SOIL_CFG_S &src, Alarm::BareSoiletDection_S &dst);
void FillHoleProtectionBarCfg(const Alarm::HoleProtectionBarDection_S &src, NET_TV_HOLE_PROTECTION_BAR_CFG_S &dst);
void ToHoleProtectionBar(const NET_TV_HOLE_PROTECTION_BAR_CFG_S &src, Alarm::HoleProtectionBarDection_S &dst);
void FillReflectiveClothingCfg(const Alarm::ReflectiveClothingDection_S &src, NET_TV_REFLECTIVE_CLOTHING_CFG_S &dst);
void ToReflectiveClothing(const NET_TV_REFLECTIVE_CLOTHING_CFG_S &src, Alarm::ReflectiveClothingDection_S &dst);
void FillPetRecognitionInfo(const Alarm::PetRecognition_S &src, NET_TV_PET_RECOGNITION_INFO_S &dst);
void ToPetRecognition(const NET_TV_PET_RECOGNITION_INFO_S &src, Alarm::PetRecognition_S &dst);
void FillClimbFenceInfo(const Alarm::FenceClimbingDetection_S &src, NET_TV_CLIMB_FENCE_INFO_S &dst);
void ToClimbFence(const NET_TV_CLIMB_FENCE_INFO_S &src, Alarm::FenceClimbingDetection_S &dst);
void FillDimissionInfo(const Alarm::LeavePostDetection_S &src, NET_TV_DIMISSION_INFO_S &dst);
void ToDimission(const NET_TV_DIMISSION_INFO_S &src, Alarm::LeavePostDetection_S &dst);
void FillIllegalLaneInfo(const Alarm::IllegalLaneChangeDetection_S &src, NET_TV_ILLEGAL_LANE_INFO_S &dst);
void ToIllegalLane(const NET_TV_ILLEGAL_LANE_INFO_S &src, Alarm::IllegalLaneChangeDetection_S &dst);
void FillRetrogradeInfo(const Alarm::DrivingAgainstTrafficDetection_S &src, NET_TV_RETROGRADE_INFO_S &dst);
void ToRetrograde(const NET_TV_RETROGRADE_INFO_S &src, Alarm::DrivingAgainstTrafficDetection_S &dst);
void FillNonmotorVehicleIntrusionInfo(const Alarm::NonMotorVehicleIntrusionDetection_S &src, NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S &dst);
void ToNonmotorVehicleIntrusion(const NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S &src, Alarm::NonMotorVehicleIntrusionDetection_S &dst);
void FillOccupationEmergencyInfo(const Alarm::EmergencyLaneOccupancyDetection_S &src, NET_TV_OCCUPATION_EMERGENCY_INFO_S &dst);
void ToOccupationEmergency(const NET_TV_OCCUPATION_EMERGENCY_INFO_S &src, Alarm::EmergencyLaneOccupancyDetection_S &dst);
void FillPedestrianIntrusionInfo(const Alarm::PedestrianIntrusionDetection_S &src, NET_TV_PEDESTRIAN_INTRUSION_INFO_S &dst);
void ToPedestrianIntrusion(const NET_TV_PEDESTRIAN_INTRUSION_INFO_S &src, Alarm::PedestrianIntrusionDetection_S &dst);
void FillSmokeFireCfg(const Alarm::SmokeFireDetection_S &src, NET_TV_SMOKE_FIRE_CFG_S &dst);
void ToSmokeFire(const NET_TV_SMOKE_FIRE_CFG_S &src, Alarm::SmokeFireDetection_S &dst);
void FillRoadPondingCfg(const Alarm::RoadPondingDetection_S &src, NET_TV_ROAD_PONDING_CFG_S &dst);
void ToRoadPonding(const NET_TV_ROAD_PONDING_CFG_S &src, Alarm::RoadPondingDetection_S &dst);
#endif

#if CAP_AI_PEOPLE_STATISTICS
void FillPeopleFlowStatisticsCfg(const Alarm::PeopleFlowStatistics_S &src, NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S &dst);
void ToPeopleFlowStatistics(const NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S &src, Alarm::PeopleFlowStatistics_S &dst);

void FillPeopleDensityDetectionCfg(const Alarm::PeopleDensityDetection_S &src, NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S &dst);
void ToPeopleDensityDetection(const NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S &src, Alarm::PeopleDensityDetection_S &dst);
#endif
void FillAudioAnomalyAlarmInfo(const Alarm::AudioAnomaly_S &src, NET_TV_AUDIO_ANOMALY_ALARM_INFO_S &dst);
void ToAudioAnomaly(const NET_TV_AUDIO_ANOMALY_ALARM_INFO_S &src, Alarm::AudioAnomaly_S &dst);

void FillPreviewInfo(const Preview::PreviewInfo_S &src, NET_TV_PREVIEW_INFO_S &dst);
void ToPreviewInfo(const NET_TV_PREVIEW_INFO_S &src, Preview::PreviewInfo_S &dst);

void FillTalkbackStateInfo(const Preview::IntercomInfo_S &src, NET_TV_TALKBACK_STATE_INFO_S &dst);
void ToIntercomInfo(const NET_TV_TALKBACK_STATE_INFO_S &src, Preview::IntercomInfo_S &dst);

void FillTalkbackStreamInfo(const Replay::Stream::Info_S &src, NET_TV_TALKBACK_STREAM_INFO_S &dst);
void ToReplayStreamInfo(const NET_TV_TALKBACK_STREAM_INFO_S &src, Replay::Stream::Info_S &dst);

void FillReplayTalkbackInfo(const Replay::Stream::ReplayRtpInfo_S &src, NET_TV_REPLAY_TALKBACK_INFO_S &dst);
void ToReplayRtpInfo(const NET_TV_REPLAY_TALKBACK_INFO_S &src, Replay::Stream::ReplayRtpInfo_S &dst);

void FillExposureInfo(const ISP::ExposureAttr_S &src, NET_TV_EXPOSURE_INFO_S &dst);
void ToExposureAttr(const NET_TV_EXPOSURE_INFO_S &src, ISP::ExposureAttr_S &dst);
void FillDayNightInfo(const ISP::DayNightAttr_S &src, NET_TV_DAYNIGHT_INFO_S &dst);
void ToDayNightAttr(const NET_TV_DAYNIGHT_INFO_S &src, ISP::DayNightAttr_S &dst);
void FillBackLightInfo(const ISP::BackLightArrt_S &src, NET_TV_BACKLIGHT_INFO_S &dst);
void ToBackLightAttr(const NET_TV_BACKLIGHT_INFO_S &src, ISP::BackLightArrt_S &dst);
void FillDenoiseInfo(const ISP::DnrAttr_S &src, NET_TV_DENOISE_INFO_S &dst);
void ToDnrAttr(const NET_TV_DENOISE_INFO_S &src, ISP::DnrAttr_S &dst);
void FillWhiteBalanceInfo(const ISP::AwbAttr_S &src, NET_TV_WHITEBALANCE_INFO_S &dst);
void ToAwbAttr(const NET_TV_WHITEBALANCE_INFO_S &src, ISP::AwbAttr_S &dst);

void FillEnterRegionAlarmInfo(const Alarm::EntranceDetection_S &src, NET_TV_ENTER_REGION_ALARM_INFO_S &dst);
void ToEntranceDetection(const NET_TV_ENTER_REGION_ALARM_INFO_S &src, Alarm::EntranceDetection_S &dst);
void FillLeaveRegionAlarmInfo(const Alarm::ExitingDetection_S &src, NET_TV_LEAVE_REGION_ALARM_INFO_S &dst);
void ToExitingDetection(const NET_TV_LEAVE_REGION_ALARM_INFO_S &src, Alarm::ExitingDetection_S &dst);
void FillAudioCfg(const Audio_NS::AudioConfig_S &src, NET_TV_AUDIO_CFG_S &dst);
void ToAudioConfig(const NET_TV_AUDIO_CFG_S &src, Audio_NS::AudioConfig_S &dst);

void FillFaceCaptureInfo(const Alarm::FaceCapture_S &src, NET_TV_FACE_CAPTURE_INFO_S &dst);
void ToFaceCapture(const NET_TV_FACE_CAPTURE_INFO_S &src, Alarm::FaceCapture_S &dst);

} // namespace TvSdkConvert

