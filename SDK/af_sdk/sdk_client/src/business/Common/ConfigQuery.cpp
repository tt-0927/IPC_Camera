/**
 * @file ConfigQuery.cpp
 * @brief 设备配置分发器实现 - GetDeviceCapability / GetDevConfig / SetDevConfig 路由逻辑
 */

#include "ConfigQuery.h"
#include "ConfigHelpers.h"
#include <cstdio>

/* 全局错误码定义（唯一实例） */
thread_local int CErrorManage::s_nLastErrorCode = NET_E_SDK_NOT_INIT;

/* ========================================================================== */
/*  GetDeviceCapability                                                        */
/* ========================================================================== */

BOOL CConfigQuery::GetDeviceCapability(LPVOID lpUserID,
                                             INT32 nChannelID, INT32 nCommand,
                                             LPVOID lpOutBuffer, INT32 nOutBufferSize,
                                             INT32 *pnBytesReturned)
{
    if (!lpOutBuffer || nOutBufferSize <= 0)
    {
        CErrorManage::instance()->SetLastError(NET_E_INVALID_PARAM);
        return FALSE;
    }

    std::string url = NET_API_URL_DEVICE_CAPABILITY(nChannelID, nCommand);

    switch (nCommand)
    {
        case NET_CAP_VIDEO_ENCODE:
        {
            if (nOutBufferSize < (INT32)sizeof(NET_VideoEncodeCap_S))
            {
                CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
                return FALSE;
            }
            return CCommandExecutor::instance()->ExecuteGet<NET_VideoEncodeCap_S>(lpUserID, url, lpOutBuffer, pnBytesReturned) ? TRUE : FALSE;
        }
        case NET_CAP_AUDIO:
        {
            if (nOutBufferSize < (INT32)sizeof(NET_AudioCap_S))
            {
                CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
                return FALSE;
            }
            return CCommandExecutor::instance()->ExecuteGet<NET_AudioCap_S>(lpUserID, url, lpOutBuffer, pnBytesReturned) ? TRUE : FALSE;
        }
        case NET_CAP_OSD:
        {
            if (nOutBufferSize < (INT32)sizeof(NET_OsdCap_S))
            {
                CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
                return FALSE;
            }
            return CCommandExecutor::instance()->ExecuteGet<NET_OsdCap_S>(lpUserID, url, lpOutBuffer, pnBytesReturned) ? TRUE : FALSE;
        }
        case NET_CAP_SYS:
        {
            if (nOutBufferSize < (INT32)sizeof(NET_SysCapability_S))
            {
                CErrorManage::instance()->SetLastError(NET_E_NOENOUGH_BUF);
                return FALSE;
            }
            return CCommandExecutor::instance()->ExecuteGet<NET_SysCapability_S>(lpUserID, url, lpOutBuffer, pnBytesReturned) ? TRUE : FALSE;
        }
        default:
            CErrorManage::instance()->SetLastError(NET_E_CMD_NOT_SUPPORT);
            return FALSE;
    }
}

/* ========================================================================== */
/*  GetDevConfig                                                               */
/* ========================================================================== */

BOOL CConfigQuery::GetDevConfig(LPVOID lpUserID, INT32 nChannelID, INT32 nCommand,
                                      LPVOID lpOutBuffer, INT32 nOutBufferSize,
                                      INT32 *pnBytesReturned)
{
    switch (nCommand)
    {
        case NET_GET_DEVICECFG:          return GetDevConfig_Impl<NET_DeviceBasicInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_STORAGE_INFO:        return GetDevConfig_Impl<NET_DeviceStorageInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_NTPCFG:             return GetDevConfig_Impl<NET_SystemNtpInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_AUDIOCFG:           return GetDevConfig_Impl<NET_AudioCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_STREAMCFG:          return GetDevConfig_Impl<NET_VideoEncodeOption_S, true>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_OSDCAPCFG:          return GetDevConfig_Impl<NET_VideoOsdCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_IMAGECFG:           return GetDevConfig_Impl<NET_ImageSetting_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_RTSPURLCFG:         return GetDevConfig_Impl<NET_RtspUrlInfo_S, true>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_NETWORKCFG:         return GetDevConfig_Impl<NET_NetworkCfgList_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_4G_INFO:            return GetDevConfig_Impl<NET_4GInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_HOTSPOT_CONN:       return GetDevConfig_Impl<NET_HotspotConnInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SECURITY_SERVICES_INFO: return GetDevConfig_Impl<NET_SecurityServicesInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SSH_COUNTDOWN:      return GetDevConfig_Impl<NET_SshCountdownInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_FIND_LOG:
        case NET_EXPORT_LOG:             return GetLogList(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_LOG_SERVER:         return GetDevConfig_Impl<NET_LogServerInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_RECORD_STATUS:      return GetDevConfig_Impl<NET_RecordStatusInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SD_CARD_STATUS:     return GetDevConfig_Impl<NET_SdCardStatus_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_AUDIO_ANOMALY_CURRENT_DB: return GetDevConfig_Impl<NET_AudioAnomalyCurrentDb_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_AUDIBLE_ALARM_INFO: return GetDevConfig_Impl<NET_AudibleAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_AUDIBLE_ALARM_CUSTOM_INFO: return GetDevConfig_Impl<NET_AudibleAlarmCustomAudioList_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_ALARM_INPUT_INFO:   return GetDevConfig_Impl<NET_AlarmInputInfoList_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_ALARM_OUTPUT_INFO:  return GetDevConfig_Impl<NET_AlarmOutputInfoList_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_FLASHING_LIGHT_ALARM_INFO: return GetDevConfig_Impl<NET_FlashingLightAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PIR_ALARM_INFO:     return GetDevConfig_Impl<NET_PirAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_RECORD_SCHEDULE:    return GetDevConfig_Impl<NET_RecordSchedule_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_RECORD_ADVANCED_PARAM: return GetDevConfig_Impl<NET_RecordAdvancedParam_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_FIND_RECORD_FILE_INFO:  return GetRecordFileList(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PRIVACYMASKCFG:     return GetDevConfig_Impl<NET_PrivacyMaskCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_TAMPERALARM:        return GetDevConfig_Impl<NET_TamperAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_MOTIONALARM:        return GetDevConfig_Impl<NET_MotionAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CROSSLINEALARM:     return GetDevConfig_Impl<NET_CrossLineAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_INTRUSIONALARM:     return GetDevConfig_Impl<NET_IntrusionAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_ENTERREGIONALARM:   return GetDevConfig_Impl<NET_EnterRegionAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_LEAVEREGIONALARM:   return GetDevConfig_Impl<NET_LeaveRegionAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_LOITERINGALARM:     return GetDevConfig_Impl<NET_LoiteringAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SCENECHANGEALARM:   return GetDevConfig_Impl<NET_SceneChangeAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CROWDGATHERINGALARM: return GetDevConfig_Impl<NET_CrowdGatheringAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_GARBAGE_EXPOSURE_CFG: return GetDevConfig_Impl<NET_GarbageExposureCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_GARBAGE_OVERFLOW_CFG: return GetDevConfig_Impl<NET_GarbageOverflowCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PEOPLE_FLOW_STATISTICS_CFG: return GetDevConfig_Impl<NET_PeopleFlowStatisticsCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PEOPLE_DENSITY_DETECTION_CFG: return GetDevConfig_Impl<NET_PeopleDensityDetectionCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_MANHOLE_COVER_ABNORMAL_CFG: return GetDevConfig_Impl<NET_ManholeCoverAbnormalCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SLEEP_ON_DUTY_CFG:  return GetDevConfig_Impl<NET_SleepOnDutyCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG: return GetDevConfig_Impl<NET_ElectricVehicleInElevatorCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PERSON_FALL_DOWN_CFG: return GetDevConfig_Impl<NET_PersonFallDownCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG: return GetDevConfig_Impl<NET_ConstructionOccupyRoadCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CONGESTION_CFG:     return GetDevConfig_Impl<NET_CongestionCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_LICENSE_PLATE_RECOGNITION_CFG: return GetDevConfig_Impl<NET_LicensePlateRecognitionCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_HIGH_ALTITUDE_SEATBELT_CFG: return GetDevConfig_Impl<NET_HighAltitudeSeatbeltCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SAFETY_HELMET_CFG:  return GetDevConfig_Impl<NET_SafetyHelmetCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PERSON_FALL_CFG:    return GetDevConfig_Impl<NET_PersonFallCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PHONE_USAGE_CFG:    return GetDevConfig_Impl<NET_PhoneUsageCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SMOKING_CFG:        return GetDevConfig_Impl<NET_SmokingCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_OPEN_FLAME_CFG:     return GetDevConfig_Impl<NET_OpenFlameCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_BARE_SOIL_CFG:      return GetDevConfig_Impl<NET_BareSoilCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_HOLE_PROTECTION_BAR_CFG: return GetDevConfig_Impl<NET_HoleProtectionBarCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_REFLECTIVE_CLOTHING_CFG: return GetDevConfig_Impl<NET_ReflectiveClothingCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PET_RECOGNITION_INFO: return GetDevConfig_Impl<NET_PetRecognitionInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CLIMB_FENCE_INFO:   return GetDevConfig_Impl<NET_ClimbFenceInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_DIMISSION_INFO:     return GetDevConfig_Impl<NET_DimissionInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_ILLEGAL_LANE_INFO:  return GetDevConfig_Impl<NET_IllegalLaneInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_RETROGRADE_INFO:    return GetDevConfig_Impl<NET_RetrogradeInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO: return GetDevConfig_Impl<NET_NonmotorVehicleIntrusionInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_OCCUPATION_EMERGENCY_INFO: return GetDevConfig_Impl<NET_OccupationEmergencyInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PEDESTRIAN_INTRUSION_INFO: return GetDevConfig_Impl<NET_PedestrianIntrusionInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SMOKE_FIRE_CFG:     return GetDevConfig_Impl<NET_SmokeFireCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_ROAD_PONDING_CFG:   return GetDevConfig_Impl<NET_RoadPondingCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PARKINGALARM:       return GetDevConfig_Impl<NET_ParkingAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_UNATTENDEDOBJECTALARM: return GetDevConfig_Impl<NET_UnattendedObjectAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_OBJECTREMOVALALARM: return GetDevConfig_Impl<NET_ObjectRemovalAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_AUDIOANOMALYALARM:  return GetDevConfig_Impl<NET_AudioAnomalyAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_PREVIEW_INFO:       return GetDevConfig_Impl<NET_PreviewInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CHANNEL_INFO:       return GetDevConfig_Impl<NET_ChannelList_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_UPGRADESTATUS:      return GetDevConfig_Impl<NET_UpgradeStatus_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_UPGRADEVERSION:     return GetDevConfig_Impl<NET_UpgradeVersion_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CAPTURE_PLAN_INFO:  return GetDevConfig_Impl<NET_CapturePlanInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CAPTURE_PARAM_INFO: return GetDevConfig_Impl<NET_CaptureParamInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_EXPOSURE_INFO:      return GetDevConfig_Impl<NET_ExposureInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_DAYNIGHT_INFO:      return GetDevConfig_Impl<NET_DayNightInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_BACKLIGHT_INFO:     return GetDevConfig_Impl<NET_BackLightInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_DENOISE_INFO:       return GetDevConfig_Impl<NET_DenoiseInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_WHITEBALANCE_INFO:  return GetDevConfig_Impl<NET_WhiteBalanceInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_FROM_STREAM_TALKBACK:   return GetDevConfig_Impl<NET_TalkbackStreamInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_FACECAPTUREINFO:    return GetDevConfig_Impl<NET_FaceCaptureInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_TARGET_LIB:         return GetDevConfig_Impl<NET_FaceLibList_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_FACE_INFO:          return GetDevConfig_Impl<NET_FaceInfoList_S, true>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_DEVICE_STATUS:      return GetDevConfig_Impl<NET_DeviceStatusInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CHANNEL_NAME:       return GetDevConfig_Impl<NET_ChannelNameInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SERIAL_PORT_PARAM:  return GetDevConfig_Impl<NET_SerialPortParam_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_VOICECOM_AUDIO_CFG: return GetDevConfig_Impl<NET_VoiceComAudioCfg_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_CONTROL_PRESET_BIT:     return GetDevConfig_Impl<NET_PresetBitInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        /* 580 设备端抓图 / 584 强制I帧：服务端 SJCL 域已实现（SjclDomain m_getTable + 设备侧
         * NvrSdkServer::GetCapturePicture / ForceKeyFrame），此前客户端漏登记这两个命令，
         * 请求还没出客户端就被这里判成 112(NET_E_CMD_NOT_SUPPORT)。补齐后 580/584 才能下发。 */
        case NET_GET_CAPTURE_PICTURE:    return GetDevConfig_Impl<NET_CapturePictureInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_FORCE_KEY_FRAME:        return GetDevConfig_Impl<NET_ForceKeyFrameInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        /* ---- 平安校园智能事件（600~613） ---- */
        case NET_GET_CHANNEL_OCCUPATION_INFO:       return GetDevConfig_Impl<NET_ChannelOccupationInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_CLIMB_DETECTION_INFO:          return GetDevConfig_Impl<NET_ClimbDetectionInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_DANGER_ZONE_APPROACH_INFO:     return GetDevConfig_Impl<NET_DangerZoneApproachInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_LONG_TIME_STAY_INFO:           return GetDevConfig_Impl<NET_LongTimeStayInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_DANGER_ZONE_STAY_INFO:         return GetDevConfig_Impl<NET_DangerZoneStayInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_KEY_AREA_ABNORMAL_STAY_INFO:   return GetDevConfig_Impl<NET_KeyAreaAbnormalStayInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_OPEN_FLAME_LEAVE_INFO:         return GetDevConfig_Impl<NET_OpenFlameLeaveInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_ROOFTOP_STAY_INFO:           return GetDevConfig_Impl<NET_RooftopStayInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_QUEUE_CROWDING_INFO:          return GetDevConfig_Impl<NET_QueueCrowdingInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_KITCHEN_UNRELATED_PERSON_INFO: return GetDevConfig_Impl<NET_KitchenUnrelatedPersonInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_ACCESS_CONTROL_ABNORMAL_INFO: return GetDevConfig_Impl<NET_AccessControlAbnormalInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_SOCIAL_RELATIONSHIP_INFO:     return GetDevConfig_Impl<NET_SocialRelationshipInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_KITCHEN_CLOTHING_INFO:        return GetDevConfig_Impl<NET_KitchenClothingInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_FIRE_EQUIPMENT_ABNORMAL_INFO: return GetDevConfig_Impl<NET_FireEquipmentAbnormalInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_FIGHT_DETECTION_INFO:         return GetDevConfig_Impl<NET_FightDetectionInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_MOUSE_DETECTION_INFO:         return GetDevConfig_Impl<NET_MouseDetectionInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        case NET_GET_FACECAPTUREOVERLAYINFO: return GetDevConfig_Impl<NET_FaceCaptureOverlayInfo_S>(lpUserID, nChannelID, nCommand, lpOutBuffer, nOutBufferSize, pnBytesReturned);
        default:
            CErrorManage::instance()->SetLastError(NET_E_CMD_NOT_SUPPORT);
            return FALSE;
    }
}

/* ========================================================================== */
/*  SetDevConfig                                                               */
/* ========================================================================== */

BOOL CConfigQuery::SetDevConfig(LPVOID lpUserID, INT32 nChannelID, INT32 nCommand,
                                      LPVOID lpInBuffer, INT32 nInBufferSize,
                                      INT32 *pnBytesReturned)
{
    switch (nCommand)
    {
        case NET_SET_DEVICECFG:          return SetDevConfig_Impl<NET_DeviceBasicInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_USERPASSWORD:
        {
            BOOL bResult = SetDevConfig_Impl<NET_UserPasswordInfo_S>(lpUserID, nChannelID, nCommand,
                                                                    lpInBuffer, nInBufferSize,
                                                                    pnBytesReturned);
            if (!bResult)
            {
                return FALSE;
            }

            auto pSession = CSessionManager::instance()->GetSession(lpUserID);
            auto pPasswordInfo = static_cast<pNET_UserPasswordInfo_S>(lpInBuffer);
            if (!pSession || !pSession->UpdateCredentials(pPasswordInfo->strUserName,
                                                          pPasswordInfo->strNewPassword))
            {
                CErrorManage::instance()->SetLastError(NET_E_INVALID_HANDLE);
                return FALSE;
            }
            return TRUE;
        }
        case NET_SET_NTPCFG:             return SetDevConfig_Impl<NET_SystemNtpInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_SYSTEM_TIME:        return SetDevConfig_Impl<NET_SystemTime_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_AUDIOCFG:           return SetDevConfig_Impl<NET_AudioCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_STREAMCFG:          return SetDevConfig_Impl<NET_VideoEncodeOption_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_OSDCAPCFG:          return SetDevConfig_Impl<NET_VideoOsdCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_IMAGECFG:           return SetDevConfig_Impl<NET_ImageSetting_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_NETWORKCFG:         return SetDevConfig_Impl<NET_NetworkCfgList_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_CONFIG_WIFI_STA:    return SetDevConfig_Impl<NET_WifiStaCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_CONNECT_WIFI_STA:       return SetDevConfig_Impl<NET_WifiStaConnect_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_DISCONNECT_WIFI_STA:    return SetDevConfig_Impl<NET_WifiStaConnect_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_4G_INFO:            return SetDevConfig_Impl<NET_4GInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_HOTSPOT_INFO:       return SetDevConfig_Impl<NET_HotspotInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_SECURITY_SERVICES_INFO: return SetDevConfig_Impl<NET_SecurityServicesInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_LOG_SERVER:         return SetDevConfig_Impl<NET_LogServerInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_TEST_LOG_SERVER:        return SetDevConfig_Impl<NET_LogServerInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_CONTROL_RECORD_INFO:    return SetDevConfig_Impl<NET_RecordInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_RECORD_SCHEDULE:    return SetDevConfig_Impl<NET_RecordSchedule_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_RECORD_ADVANCED_PARAM: return SetDevConfig_Impl<NET_RecordAdvancedParam_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_DOWNLOAD_RECORD_FILE:   return SetDevConfig_Impl<NET_RecordDownloadList_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PRIVACYMASKCFG:     return SetDevConfig_Impl<NET_PrivacyMaskCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_TAMPERALARM:        return SetDevConfig_Impl<NET_TamperAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_MOTIONALARM:        return SetDevConfig_Impl<NET_MotionAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_CROSSLINEALARM:     return SetDevConfig_Impl<NET_CrossLineAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_INTRUSIONALARM:     return SetDevConfig_Impl<NET_IntrusionAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_ENTERREGIONALARM:   return SetDevConfig_Impl<NET_EnterRegionAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_LEAVEREGIONALARM:   return SetDevConfig_Impl<NET_LeaveRegionAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_LOITERINGALARM:     return SetDevConfig_Impl<NET_LoiteringAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_SCENECHANGEALARM:   return SetDevConfig_Impl<NET_SceneChangeAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_CROWDGATHERINGALARM: return SetDevConfig_Impl<NET_CrowdGatheringAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_GARBAGE_EXPOSURE_CFG: return SetDevConfig_Impl<NET_GarbageExposureCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_GARBAGE_OVERFLOW_CFG: return SetDevConfig_Impl<NET_GarbageOverflowCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PEOPLE_FLOW_STATISTICS_CFG: return SetDevConfig_Impl<NET_PeopleFlowStatisticsCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_RESET_PEOPLE_FLOW_STATISTICS: return SetDevConfig_Impl<NET_PeopleFlowStatisticsCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PEOPLE_DENSITY_DETECTION_CFG: return SetDevConfig_Impl<NET_PeopleDensityDetectionCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_MANHOLE_COVER_ABNORMAL_CFG: return SetDevConfig_Impl<NET_ManholeCoverAbnormalCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_SLEEP_ON_DUTY_CFG:  return SetDevConfig_Impl<NET_SleepOnDutyCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG: return SetDevConfig_Impl<NET_ElectricVehicleInElevatorCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PERSON_FALL_DOWN_CFG: return SetDevConfig_Impl<NET_PersonFallDownCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG: return SetDevConfig_Impl<NET_ConstructionOccupyRoadCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_CONGESTION_CFG:     return SetDevConfig_Impl<NET_CongestionCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_LICENSE_PLATE_RECOGNITION_CFG: return SetDevConfig_Impl<NET_LicensePlateRecognitionCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_HIGH_ALTITUDE_SEATBELT_CFG: return SetDevConfig_Impl<NET_HighAltitudeSeatbeltCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_SAFETY_HELMET_CFG:  return SetDevConfig_Impl<NET_SafetyHelmetCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PERSON_FALL_CFG:    return SetDevConfig_Impl<NET_PersonFallCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PHONE_USAGE_CFG:    return SetDevConfig_Impl<NET_PhoneUsageCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_SMOKING_CFG:        return SetDevConfig_Impl<NET_SmokingCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_OPEN_FLAME_CFG:     return SetDevConfig_Impl<NET_OpenFlameCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_BARE_SOIL_CFG:      return SetDevConfig_Impl<NET_BareSoilCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_HOLE_PROTECTION_BAR_CFG: return SetDevConfig_Impl<NET_HoleProtectionBarCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_REFLECTIVE_CLOTHING_CFG: return SetDevConfig_Impl<NET_ReflectiveClothingCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PET_RECOGNITION_INFO: return SetDevConfig_Impl<NET_PetRecognitionInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_CLIMB_FENCE_INFO:   return SetDevConfig_Impl<NET_ClimbFenceInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_DIMISSION_INFO:     return SetDevConfig_Impl<NET_DimissionInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_ILLEGAL_LANE_INFO:  return SetDevConfig_Impl<NET_IllegalLaneInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_RETROGRADE_INFO:    return SetDevConfig_Impl<NET_RetrogradeInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO: return SetDevConfig_Impl<NET_NonmotorVehicleIntrusionInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_OCCUPATION_EMERGENCY_INFO: return SetDevConfig_Impl<NET_OccupationEmergencyInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PEDESTRIAN_INTRUSION_INFO: return SetDevConfig_Impl<NET_PedestrianIntrusionInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_SMOKE_FIRE_CFG:     return SetDevConfig_Impl<NET_SmokeFireCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_ROAD_PONDING_CFG:   return SetDevConfig_Impl<NET_RoadPondingCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PARKINGALARM:       return SetDevConfig_Impl<NET_ParkingAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_UNATTENDEDOBJECTALARM: return SetDevConfig_Impl<NET_UnattendedObjectAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_OBJECTREMOVALALARM: return SetDevConfig_Impl<NET_ObjectRemovalAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_AUDIBLE_ALARM_INFO: return SetDevConfig_Impl<NET_AudibleAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_EDIT_AUDIBLE_ALARM_CUSTOM_INFO: return SetDevConfig_Impl<NET_AudibleAlarmCustomOperation_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_ADD_AUDIBLE_ALARM_CUSTOM_INFO: return SetDevConfig_Impl<NET_AudibleAlarmCustomOperation_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_ALARM_INPUT_INFO:   return SetDevConfig_Impl<NET_AlarmInputInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_ALARM_OUTPUT_INFO:  return SetDevConfig_Impl<NET_AlarmOutputInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_FLASHING_LIGHT_ALARM_INFO: return SetDevConfig_Impl<NET_FlashingLightAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PIR_ALARM_INFO:     return SetDevConfig_Impl<NET_PirAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_AUDIOANOMALYALARM:  return SetDevConfig_Impl<NET_AudioAnomalyAlarmInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_PREVIEW_INFO:       return SetDevConfig_Impl<NET_PreviewInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_UPGRADE:            return SetDevConfig_Impl<NET_UpgradeInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_CAPTURE_PLAN_INFO:  return SetDevConfig_Impl<NET_CapturePlanInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_CAPTURE_PARAM_INFO: return SetDevConfig_Impl<NET_CaptureParamInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_VOICECOM_AUDIO_CFG: return SetDevConfig_Impl<NET_VoiceComAudioCfg_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_EXPOSURE_INFO:      return SetDevConfig_Impl<NET_ExposureInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_DAYNIGHT_INFO:      return SetDevConfig_Impl<NET_DayNightInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_BACKLIGHT_INFO:     return SetDevConfig_Impl<NET_BackLightInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_DENOISE_INFO:       return SetDevConfig_Impl<NET_DenoiseInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_WHITEBALANCE_INFO:  return SetDevConfig_Impl<NET_WhiteBalanceInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_STATE_TALKBACK:         return SetDevConfig_Impl<NET_TalkbackStateInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_TO_STREAM_TALKBACK:     return SetDevConfig_Impl<NET_TalkbackStreamInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_REPLAY_TALKBACK:        return SetDevConfig_Impl<NET_ReplayTalkbackInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_FACECAPTUREINFO:    return SetDevConfig_Impl<NET_FaceCaptureInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_FACE_COMPARE_INFO:  return SetDevConfig_Impl<NET_FaceCompareInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_ADD_TARGET_LIB:
        case NET_DEL_TARGET_LIB:
        case NET_SET_TARGET_LIB:         return SetDevConfig_Impl<NET_FaceLibInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_ADD_FACE_INFO:
        case NET_SET_FACE_INFO:          return SetDevConfig_Impl<NET_FaceInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_DEL_FACE_INFO:          return SetDevConfig_Impl<NET_FaceIdInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        /* -- 设备管理 -- */
        case NET_CONTROL_REBOOT:         return SetDevConfig_Impl<NET_RebootInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_CONTROL_SHUTDOWN:       return SetDevConfig_Impl<NET_ShutdownInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_CONTROL_FORMAT_DISK:    return SetDevConfig_Impl<NET_FormatDiskInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_CHANNEL_NAME:       return SetDevConfig_Impl<NET_ChannelNameInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_CONTROL_RESET:          return SetDevConfig_Impl<NET_ResetInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        /* -- 透明通道 -- */
        case NET_OPEN_TRANSPARENT_CHANNEL:  return SetDevConfig_Impl<NET_TransparentChannel_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_CLOSE_TRANSPARENT_CHANNEL: return SetDevConfig_Impl<NET_TransparentChannel_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SEND_TRANSPARENT_DATA:     return SetDevConfig_Impl<NET_TransparentData_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        /* -- 串口通信 -- */
        case NET_SEND_SERIAL_DATA:          return SetDevConfig_Impl<NET_SerialData_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SEND_RS232_DATA:           return SetDevConfig_Impl<NET_SerialData_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SEND_SERIAL_DIRECT_DATA:   return SetDevConfig_Impl<NET_SerialData_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_CONTROL_CAMERA:         return SetDevConfig_Impl<NET_CameraControlInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_CONTROL_PRESET_BIT:     return SetDevConfig_Impl<NET_PresetBitCtrl_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        /* ---- 平安校园智能事件（600~613） ---- */
        case NET_SET_CHANNEL_OCCUPATION_INFO:           return SetDevConfig_Impl<NET_ChannelOccupationInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_DANGER_ZONE_APPROACH_INFO:         return SetDevConfig_Impl<NET_DangerZoneApproachInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_LONG_TIME_STAY_INFO:               return SetDevConfig_Impl<NET_LongTimeStayInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_DANGER_ZONE_STAY_INFO:             return SetDevConfig_Impl<NET_DangerZoneStayInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_KEY_AREA_ABNORMAL_STAY_INFO:       return SetDevConfig_Impl<NET_KeyAreaAbnormalStayInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_OPEN_FLAME_LEAVE_INFO:             return SetDevConfig_Impl<NET_OpenFlameLeaveInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_QUEUE_CROWDING_INFO:          return SetDevConfig_Impl<NET_QueueCrowdingInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_ROOFTOP_STAY_INFO:           return SetDevConfig_Impl<NET_RooftopStayInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_KITCHEN_UNRELATED_PERSON_INFO: return SetDevConfig_Impl<NET_KitchenUnrelatedPersonInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_ACCESS_CONTROL_ABNORMAL_INFO: return SetDevConfig_Impl<NET_AccessControlAbnormalInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_SOCIAL_RELATIONSHIP_INFO:     return SetDevConfig_Impl<NET_SocialRelationshipInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_KITCHEN_CLOTHING_INFO:        return SetDevConfig_Impl<NET_KitchenClothingInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_FIRE_EQUIPMENT_ABNORMAL_INFO: return SetDevConfig_Impl<NET_FireEquipmentAbnormalInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_FIGHT_DETECTION_INFO:         return SetDevConfig_Impl<NET_FightDetectionInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_MOUSE_DETECTION_INFO:         return SetDevConfig_Impl<NET_MouseDetectionInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_SET_FACECAPTUREOVERLAYINFO: return SetDevConfig_Impl<NET_FaceCaptureOverlayInfo_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        case NET_TRIGGER_SOUND_LIGHT_ALARM: return SetDevConfig_Impl<NET_SoundLightAlarmTrigger_S>(lpUserID, nChannelID, nCommand, lpInBuffer, nInBufferSize, pnBytesReturned);
        default:
            CErrorManage::instance()->SetLastError(NET_E_CMD_NOT_SUPPORT);
            return FALSE;
    }
}
