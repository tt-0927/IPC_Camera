#pragma once

#include <cstddef>

#include "system_define.h"
#include "video_define.h"
#include "network_define.h"
#include "alarm_define.h"
#include "replay_define.h"
#include "osd_define.h"
#include "isp_define.h"

#include "NetTVSDKServer.h"

/**
 * @brief IPC <-> TVSDK 结构体转换
 *
 * 只做“结构到结构”的拷贝/映射，不关心命令码与 JSON。
 */
namespace TvSdkConvert
{
void FillDeviceInfo(const ::System::DeviceInfo_S &src, NET_DeviceInfo_S &dst);

void FillDeviceBasicInfo(const ::System::DeviceInfo_S &src, NET_DeviceBasicInfo_S &dst);
void ToDeviceInfo(const NET_DeviceBasicInfo_S &src, ::System::DeviceInfo_S &dst);
void FillSystemNtpInfo(const ::System::TimeInfo_S &src, NET_SystemNtpInfo_S &dst);
void ToTimeInfo(const NET_SystemNtpInfo_S &src, ::System::TimeInfo_S &dst);

void FillNetworkCfg(const Network::Info_S &src, NET_NetworkCfg_S &dst);
void ToNetworkInfo(const NET_NetworkCfg_S &src, Network::Info_S &dst);
void FillWifiStaCfg(const Network::WifiStaInfo_S &src, NET_WifiStaCfg_S &dst);
void ToWifiStaInfo(const NET_WifiStaCfg_S &src, Network::WifiStaInfo_S &dst);
void FillWifiStaConnect(const Network::WifiStaConncet_S &src, NET_WifiStaConnect_S &dst);
void ToWifiStaConnect(const NET_WifiStaConnect_S &src, Network::WifiStaConncet_S &dst);
void Fill4GInfo(const Network::Network_4G_Config_t &src, NET_4GInfo_S &dst);
void To4GConfig(const NET_4GInfo_S &src, Network::Network_4G_Config_t &dst);
void FillHotspotInfo(const Network::HotspotConfig &src, NET_HotspotInfo_S &dst);
void ToHotspotConfig(const NET_HotspotInfo_S &src, Network::HotspotConfig &dst);
void FillVideoEncodeOption(const Video_NS::VideoConfig_S &src, NET_VideoEncodeOption_S &dst);
void ToVideoConfig(const NET_VideoEncodeOption_S &src, Video_NS::VideoConfig_S &dst);
void FillOsdConfig(const Osd::OsdConfig_S &src, NET_VideoOsdCfg_S &dst);
void ToOsdConfig(const NET_VideoOsdCfg_S &src, Osd::OsdConfig_S &dst);
void FillPrivacyMaskCfg(const Osd::CoverConfig_S &src, std::size_t maxAreaCount, NET_PrivacyMaskCfg_S &dst);
bool ToPrivacyMaskCfg(const NET_PrivacyMaskCfg_S &src, std::size_t maxAreaCount, Osd::CoverConfig_S &dst);

void ToUpgradeInfo(const NET_UpgradeInfo_S &src, ::System::UpgradeInfo_S &dst);
void FillUpgradeStatus(const ::System::UpgradeStatus_S &src, NET_UpgradeStatus_S &dst);
void FillUpgradeVersion(const ::System::UpgradeVersion_S &src, NET_UpgradeVersion_S &dst);

void FillCapturePlan(const Capture_NS::CapturePlan_S &src, NET_CapturePlanInfo_S &dst);
void ToCapturePlan(const NET_CapturePlanInfo_S &src, Capture_NS::CapturePlan_S &dst);
void FillCaptureParam(const Capture_NS::CaptureParam_S &src, NET_CaptureParamInfo_S &dst);
void ToCaptureParam(const NET_CaptureParamInfo_S &src, Capture_NS::CaptureParam_S &dst);

void FillVideoEncodeCap(const Video_NS::VideoCapabilitySet_S &src, NET_VideoEncodeCap_S &dst);
void FillAudioEncodeCap(const Audio_NS::AudioCapabilitySet_S &src, NET_AudioCap_S &dst);
void FillVideoOSDCap(const Video_NS::VideoCapabilitySet_S &src, NET_OsdCap_S &dst);

// --------- 智能告警配置（对应 NET_GET/SET_*ALARM） ---------
void FillMotionAlarmInfo(const Alarm::MotionDetection_S &src, NET_MotionAlarmInfo_S &dst);
void ToMotionDetection(const NET_MotionAlarmInfo_S &src, Alarm::MotionDetection_S &dst);

void FillTamperAlarmInfo(const Alarm::HideAlarm_S &src, NET_TamperAlarmInfo_S &dst);
void ToHideAlarm(const NET_TamperAlarmInfo_S &src, Alarm::HideAlarm_S &dst);

void FillCrossLineAlarmInfo(const Alarm::BoundaryDetection_S &src, NET_CrossLineAlarmInfo_S &dst);
void ToBoundaryDetection(const NET_CrossLineAlarmInfo_S &src, Alarm::BoundaryDetection_S &dst);

void FillIntrusionAlarmInfo(const Alarm::FieldDetection_S &src, NET_IntrusionAlarmInfo_S &dst);
void ToFieldDetection(const NET_IntrusionAlarmInfo_S &src, Alarm::FieldDetection_S &dst);

void FillLoiteringAlarmInfo(const Alarm::LoiteringDetection_S &src, NET_LoiteringAlarmInfo_S &dst);
void ToLoiteringDetection(const NET_LoiteringAlarmInfo_S &src, Alarm::LoiteringDetection_S &dst);

void FillSceneChangeAlarmInfo(const Alarm::SceneChange_S &src, NET_SceneChangeAlarmInfo_S &dst);
void ToSceneChange(const NET_SceneChangeAlarmInfo_S &src, Alarm::SceneChange_S &dst);

void FillCrowdGatheringAlarmInfo(const Alarm::CrowdGathering_S &src, NET_CrowdGatheringAlarmInfo_S &dst);
void ToCrowdGathering(const NET_CrowdGatheringAlarmInfo_S &src, Alarm::CrowdGathering_S &dst);

void FillParkingDetectAlarmInfo(const Alarm::ParkingDetection_S &src, NET_ParkingAlarmInfo_S &dst);
void ToParkingDetection(const NET_ParkingAlarmInfo_S &src, Alarm::ParkingDetection_S &dst);

void FillUnattendedObjectAlarmInfo(const Alarm::UnattendedObject_S &src, NET_UnattendedObjectAlarmInfo_S &dst);
void ToUnattendedObject(const NET_UnattendedObjectAlarmInfo_S &src, Alarm::UnattendedObject_S &dst);

void FillObjectRemovalAlarmInfo(const Alarm::ObjectRemoval_S &src, NET_ObjectRemovalAlarmInfo_S &dst);
void ToObjectRemoval(const NET_ObjectRemovalAlarmInfo_S &src, Alarm::ObjectRemoval_S &dst);

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
void FillGarbageExposureCfg(const Alarm::GarbageExposureDetection_S &src, NET_GarbageExposureCfg_S &dst);
void ToGarbageExposure(const NET_GarbageExposureCfg_S &src, Alarm::GarbageExposureDetection_S &dst);

void FillGarbageOverflowCfg(const Alarm::GarbageOverflowDetection_S &src, NET_GarbageOverflowCfg_S &dst);
void ToGarbageOverflow(const NET_GarbageOverflowCfg_S &src, Alarm::GarbageOverflowDetection_S &dst);
#endif

#ifdef SCENE_INTELLIGENCE
void FillManholeCoverAbnormalCfg(const Alarm::ManholeCoverAbnormalDetection_S &src, NET_ManholeCoverAbnormalCfg_S &dst);
void ToManholeCoverAbnormal(const NET_ManholeCoverAbnormalCfg_S &src, Alarm::ManholeCoverAbnormalDetection_S &dst);
void FillSleepOnDutyCfg(const Alarm::SleepOnDutyDetection_S &src, NET_SleepOnDutyCfg_S &dst);
void ToSleepOnDuty(const NET_SleepOnDutyCfg_S &src, Alarm::SleepOnDutyDetection_S &dst);
void FillElectricVehicleInElevatorCfg(const Alarm::ElectricScooterDetection_S &src, NET_ElectricVehicleInElevatorCfg_S &dst);
void ToElectricVehicleInElevator(const NET_ElectricVehicleInElevatorCfg_S &src, Alarm::ElectricScooterDetection_S &dst);
void FillPersonFallDownCfg(const Alarm::PersonFallDownDetection_S &src, NET_PersonFallDownCfg_S &dst);
void ToPersonFallDown(const NET_PersonFallDownCfg_S &src, Alarm::PersonFallDownDetection_S &dst);
void FillConstructionOccupyRoadCfg(const Alarm::ConstructionEncroachmentRoadDetection_S &src, NET_ConstructionOccupyRoadCfg_S &dst);
void ToConstructionOccupyRoad(const NET_ConstructionOccupyRoadCfg_S &src, Alarm::ConstructionEncroachmentRoadDetection_S &dst);
void FillCongestionCfg(const Alarm::CongestionDetection_S &src, NET_CongestionCfg_S &dst);
void ToCongestion(const NET_CongestionCfg_S &src, Alarm::CongestionDetection_S &dst);
void FillLicensePlateRecognitionCfg(const Alarm::LicensePlateCognitionDetection_S &src, NET_LicensePlateRecognitionCfg_S &dst);
void ToLicensePlateRecognition(const NET_LicensePlateRecognitionCfg_S &src, Alarm::LicensePlateCognitionDetection_S &dst);
void FillHighAltitudeSeatbeltCfg(const Alarm::HighAltitudeSeatbeltDetection_S &src, NET_HighAltitudeSeatbeltCfg_S &dst);
void ToHighAltitudeSeatbelt(const NET_HighAltitudeSeatbeltCfg_S &src, Alarm::HighAltitudeSeatbeltDetection_S &dst);
void FillSafetyHelmetCfg(const Alarm::SafetyHelmetDection_S &src, NET_SafetyHelmetCfg_S &dst);
void ToSafetyHelmet(const NET_SafetyHelmetCfg_S &src, Alarm::SafetyHelmetDection_S &dst);
void FillPersonFallCfg(const Alarm::TripDetection_S &src, NET_PersonFallCfg_S &dst);
void ToPersonFall(const NET_PersonFallCfg_S &src, Alarm::TripDetection_S &dst);
void FillPhoneUsageCfg(const Alarm::PhoneUsageDetection_S &src, NET_PhoneUsageCfg_S &dst);
void ToPhoneUsage(const NET_PhoneUsageCfg_S &src, Alarm::PhoneUsageDetection_S &dst);
void FillSmokingCfg(const Alarm::SmokingDection_S &src, NET_SmokingCfg_S &dst);
void ToSmoking(const NET_SmokingCfg_S &src, Alarm::SmokingDection_S &dst);
void FillOpenFlameCfg(const Alarm::OpenFlameDetection_S &src, NET_OpenFlameCfg_S &dst);
void ToOpenFlame(const NET_OpenFlameCfg_S &src, Alarm::OpenFlameDetection_S &dst);
void FillBareSoilCfg(const Alarm::BareSoiletDection_S &src, NET_BareSoilCfg_S &dst);
void ToBareSoil(const NET_BareSoilCfg_S &src, Alarm::BareSoiletDection_S &dst);
void FillHoleProtectionBarCfg(const Alarm::HoleProtectionBarDection_S &src, NET_HoleProtectionBarCfg_S &dst);
void ToHoleProtectionBar(const NET_HoleProtectionBarCfg_S &src, Alarm::HoleProtectionBarDection_S &dst);
void FillReflectiveClothingCfg(const Alarm::ReflectiveClothingDection_S &src, NET_ReflectiveClothingCfg_S &dst);
void ToReflectiveClothing(const NET_ReflectiveClothingCfg_S &src, Alarm::ReflectiveClothingDection_S &dst);
#endif

void FillPetRecognitionInfo(const Alarm::PetRecognition_S &src, NET_PetRecognitionInfo_S &dst);
void ToPetRecognition(const NET_PetRecognitionInfo_S &src, Alarm::PetRecognition_S &dst);

#ifdef SCENE_INTELLIGENCE
void FillClimbFenceInfo(const Alarm::FenceClimbingDetection_S &src, NET_ClimbFenceInfo_S &dst);
void ToClimbFence(const NET_ClimbFenceInfo_S &src, Alarm::FenceClimbingDetection_S &dst);
void FillDimissionInfo(const Alarm::LeavePostDetection_S &src, NET_DimissionInfo_S &dst);
void ToDimission(const NET_DimissionInfo_S &src, Alarm::LeavePostDetection_S &dst);
void FillIllegalLaneInfo(const Alarm::IllegalLaneChangeDetection_S &src, NET_IllegalLaneInfo_S &dst);
void ToIllegalLane(const NET_IllegalLaneInfo_S &src, Alarm::IllegalLaneChangeDetection_S &dst);
void FillRetrogradeInfo(const Alarm::DrivingAgainstTrafficDetection_S &src, NET_RetrogradeInfo_S &dst);
void ToRetrograde(const NET_RetrogradeInfo_S &src, Alarm::DrivingAgainstTrafficDetection_S &dst);
void FillNonmotorVehicleIntrusionInfo(const Alarm::NonMotorVehicleIntrusionDetection_S &src, NET_NonmotorVehicleIntrusionInfo_S &dst);
void ToNonmotorVehicleIntrusion(const NET_NonmotorVehicleIntrusionInfo_S &src, Alarm::NonMotorVehicleIntrusionDetection_S &dst);
void FillOccupationEmergencyInfo(const Alarm::EmergencyLaneOccupancyDetection_S &src, NET_OccupationEmergencyInfo_S &dst);
void ToOccupationEmergency(const NET_OccupationEmergencyInfo_S &src, Alarm::EmergencyLaneOccupancyDetection_S &dst);
void FillPedestrianIntrusionInfo(const Alarm::PedestrianIntrusionDetection_S &src, NET_PedestrianIntrusionInfo_S &dst);
void ToPedestrianIntrusion(const NET_PedestrianIntrusionInfo_S &src, Alarm::PedestrianIntrusionDetection_S &dst);
void FillSmokeFireCfg(const Alarm::SmokeFireDetection_S &src, NET_SmokeFireCfg_S &dst);
void ToSmokeFire(const NET_SmokeFireCfg_S &src, Alarm::SmokeFireDetection_S &dst);
void FillRoadPondingCfg(const Alarm::RoadPondingDetection_S &src, NET_RoadPondingCfg_S &dst);
void ToRoadPonding(const NET_RoadPondingCfg_S &src, Alarm::RoadPondingDetection_S &dst);
#endif

#if CAP_AI_PEOPLE_STATISTICS
void FillPeopleFlowStatisticsCfg(const Alarm::PeopleFlowStatistics_S &src, NET_PeopleFlowStatisticsCfg_S &dst);
void ToPeopleFlowStatistics(const NET_PeopleFlowStatisticsCfg_S &src, Alarm::PeopleFlowStatistics_S &dst);

void FillPeopleDensityDetectionCfg(const Alarm::PeopleDensityDetection_S &src, NET_PeopleDensityDetectionCfg_S &dst);
void ToPeopleDensityDetection(const NET_PeopleDensityDetectionCfg_S &src, Alarm::PeopleDensityDetection_S &dst);
#endif
void FillAudioAnomalyAlarmInfo(const Alarm::AudioAnomaly_S &src, NET_AudioAnomalyAlarmInfo_S &dst);
void ToAudioAnomaly(const NET_AudioAnomalyAlarmInfo_S &src, Alarm::AudioAnomaly_S &dst);

void FillImageSetting(const ISP::ImageParam_S &src, NET_ImageSetting_S &dst);
void ToImageParam(const NET_ImageSetting_S &src, ISP::ImageParam_S &dst);

void FillPreviewInfo(const Preview::PreviewInfo_S &src, NET_PreviewInfo_S &dst);
void ToPreviewInfo(const NET_PreviewInfo_S &src, Preview::PreviewInfo_S &dst);

void FillTalkbackStateInfo(const Preview::IntercomInfo_S &src, NET_TalkbackStateInfo_S &dst);
void ToIntercomInfo(const NET_TalkbackStateInfo_S &src, Preview::IntercomInfo_S &dst);

void FillTalkbackStreamInfo(const Replay::Stream::Info_S &src, NET_TalkbackStreamInfo_S &dst);
void ToReplayStreamInfo(const NET_TalkbackStreamInfo_S &src, Replay::Stream::Info_S &dst);

void FillReplayTalkbackInfo(const Replay::Stream::ReplayRtpInfo_S &src, NET_ReplayTalkbackInfo_S &dst);
void ToReplayRtpInfo(const NET_ReplayTalkbackInfo_S &src, Replay::Stream::ReplayRtpInfo_S &dst);

void FillExposureInfo(const ISP::ExposureAttr_S &src, NET_ExposureInfo_S &dst);
void ToExposureAttr(const NET_ExposureInfo_S &src, ISP::ExposureAttr_S &dst);
void FillDayNightInfo(const ISP::DayNightAttr_S &src, NET_DayNightInfo_S &dst);
void ToDayNightAttr(const NET_DayNightInfo_S &src, ISP::DayNightAttr_S &dst);
void FillBackLightInfo(const ISP::BackLightArrt_S &src, NET_BackLightInfo_S &dst);
void ToBackLightAttr(const NET_BackLightInfo_S &src, ISP::BackLightArrt_S &dst);
void FillDenoiseInfo(const ISP::DnrAttr_S &src, NET_DenoiseInfo_S &dst);
void ToDnrAttr(const NET_DenoiseInfo_S &src, ISP::DnrAttr_S &dst);
void FillWhiteBalanceInfo(const ISP::AwbAttr_S &src, NET_WhiteBalanceInfo_S &dst);
void ToAwbAttr(const NET_WhiteBalanceInfo_S &src, ISP::AwbAttr_S &dst);

void FillEnterRegionAlarmInfo(const Alarm::EntranceDetection_S &src, NET_EnterRegionAlarmInfo_S &dst);
void ToEntranceDetection(const NET_EnterRegionAlarmInfo_S &src, Alarm::EntranceDetection_S &dst);
void FillLeaveRegionAlarmInfo(const Alarm::ExitingDetection_S &src, NET_LeaveRegionAlarmInfo_S &dst);
void ToExitingDetection(const NET_LeaveRegionAlarmInfo_S &src, Alarm::ExitingDetection_S &dst);
void FillAudioCfg(const Audio_NS::AudioConfig_S &src, NET_AudioCfg_S &dst);
void ToAudioConfig(const NET_AudioCfg_S &src, Audio_NS::AudioConfig_S &dst);

#ifdef SCENE_INTELLIGENCE
/**
 * @brief 将 IPC 人脸抓拍事件转换为 SDK 推送结构体。
 * @param [in] stSource IPC 人脸抓拍事件。
 * @param [out] stDestination SDK 人脸抓拍推送信息。
 * @return 无。
 */
void FillFaceCapturePushInfo(const Alarm::FaceAlarmInfo_S &stSource,
                             NET_FaceCapturePushInfo_S &stDestination);

/**
 * @brief 将 IPC 行人抓拍事件转换为 SDK 推送结构体。
 * @param [in] stSource IPC 行人抓拍事件。
 * @param [out] stDestination SDK 行人抓拍推送信息。
 * @return 无。
 */
void FillPersonCapturePushInfo(const Alarm::PersonAlarmInfo_S &stSource,
                               NET_PersonCapturePushInfo_S &stDestination);

/**
 * @brief 将 IPC 机动车抓拍事件转换为 SDK 推送结构体。
 * @param [in] stSource IPC 机动车抓拍事件。
 * @param [out] stDestination SDK 机动车抓拍推送信息。
 * @return 无。
 */
void FillMotorvehicleCapturePushInfo(const Alarm::MotorvehicleAlarmInfo_S &stSource,
                                     NET_MotorvehicleCapturePushInfo_S &stDestination);

/**
 * @brief 将 IPC 非机动车抓拍事件转换为 SDK 推送结构体。
 * @param [in] stSource IPC 非机动车抓拍事件。
 * @param [out] stDestination SDK 非机动车抓拍推送信息。
 * @return 无。
 */
void FillNonMotorvehicleCapturePushInfo(const Alarm::NonMotorvehicleAlarmInfo_S &stSource,
                                        NET_NonMotorvehicleCapturePushInfo_S &stDestination);
#endif

void FillFaceCaptureInfo(const Alarm::FaceCapture_S &src, NET_FaceCaptureInfo_S &dst);
void ToFaceCapture(const NET_FaceCaptureInfo_S &src, Alarm::FaceCapture_S &dst);

/**
 * @brief 将 TVSDK 联动配置转换为 IPC 联动配置。
 * @param [in] src TVSDK 联动配置。
 * @param [out] dst IPC 联动配置。
 * @return 无。
 */
void ToLinkageList(const NET_LinkageList_S &src, Alarm::LinkageList_S &dst);

/**
 * @brief 将 IPC 人脸抓拍图片叠加配置转换为 TVSDK 配置。
 * @param [in] src IPC 人脸抓拍图片叠加配置。
 * @param [out] dst TVSDK 人脸抓拍图片叠加配置。
 * @return 无。
 */
void FillFaceCaptureOverlayInfo(const Alarm::OverlayInfo_S &src,
                                NET_FaceCaptureOverlayInfo_S &dst);

/**
 * @brief 将 TVSDK 人脸抓拍图片叠加配置转换为 IPC 配置。
 * @param [in] src TVSDK 人脸抓拍图片叠加配置。
 * @param [out] dst IPC 人脸抓拍图片叠加配置。
 * @return 无。
 */
void ToFaceCaptureOverlayInfo(const NET_FaceCaptureOverlayInfo_S &src,
                              Alarm::OverlayInfo_S &dst);

} // namespace TvSdkConvert

