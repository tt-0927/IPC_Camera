/*
 * @FilePath     : sdk_new/sdk_server/src/cb/BG6_ZHSJ/BU_SJCL/NetTVEventConfigCb.c
 * @Author       : ITC
 * @Date         : 2026-08-18
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : 事件与报警配置回调注册实现（BG6_ZHSJ/BU_SJCL部门专用）
 *                 包含：
 *                 1. 基础报警配置回调（PrivacyMask/Tamper/Motion/Audible/AlarmInput/AlarmOutput/FlashingLight/Pir）
 *                 2. 智能分析报警回调（CrossLine/Intrusion/EnterRegion/LeaveRegion/Loitering/SceneChange/CrowGathering/Parking/UnattendedObject/ObjectRemoval/AudioAnomaly）
 *                 3. 人脸识别相关回调（FaceCapture/FaceCompare/TargetLib/FaceInfo）
 *                 4. 人流统计回调（PeopleFlowStatistics/PeopleDensityDetection/Reset）
 *                 5. 垃圾检测回调（GarbageExposure/GarbageOverflow）
 *                 6. 行业AI检测回调（SleepOnDuty/PersonFall/Smoking/SafetyHelmet/SmokeFire等）
 *                 依赖：所有注册函数最终调用 registerGetCmdCb/registerSetCmdCb（定义于Common/NetTVConfigCb.c）
 *                 注：NVR独有配置（回放/RTSP）已迁移至 NetTVNvrConfigCb.c
 */

#include <stdio.h>
#include <stddef.h>
#include "NetTVEventConfigCbExecute.h"
#include "NetTVConfigCbExecute.h"
#include "NetTVSDKServerInterface.h"

/* ===================== 基础报警配置回调 ===================== */

/**
 * @brief 注册获取声音告警配置的回调函数
 * @param [in] pCb 用于填充 NET_AudibleAlarmInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetAudibleAlarmInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_AUDIBLE_ALARM_INFO, pCb);
}

/**
 * @brief 注册设置声音告警配置的回调函数
 * @param [in] pCb 用于读取 NET_AudibleAlarmInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetAudibleAlarmInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_AUDIBLE_ALARM_INFO, pCb);
}

/**
 * @brief 注册获取报警输入配置集合的回调函数
 * @param [in] pCb 用于填充 NET_AlarmInputInfoList_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetAlarmInputInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_ALARM_INPUT_INFO, pCb);
}

/**
 * @brief 注册设置单路报警输入配置的回调函数
 * @param [in] pCb 用于读取 NET_AlarmInputInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetAlarmInputInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_ALARM_INPUT_INFO, pCb);
}

/**
 * @brief 注册获取报警输出配置集合的回调函数
 * @param [in] pCb 用于填充 NET_AlarmOutputInfoList_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetAlarmOutputInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_ALARM_OUTPUT_INFO, pCb);
}

/**
 * @brief 注册设置单路报警输出配置的回调函数
 * @param [in] pCb 用于读取 NET_AlarmOutputInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetAlarmOutputInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_ALARM_OUTPUT_INFO, pCb);
}

/**
 * @brief 注册获取闪光灯告警配置的回调函数
 * @param [in] pCb 用于填充 NET_FlashingLightAlarmInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetFlashingLightAlarmInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_FLASHING_LIGHT_ALARM_INFO, pCb);
}

/**
 * @brief 注册设置闪光灯告警配置的回调函数
 * @param [in] pCb 用于读取 NET_FlashingLightAlarmInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetFlashingLightAlarmInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_FLASHING_LIGHT_ALARM_INFO, pCb);
}

/**
 * @brief 注册获取 PIR 告警配置的回调函数
 * @param [in] pCb 用于填充 NET_PirAlarmInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetPirAlarmInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PIR_ALARM_INFO, pCb);
}

/**
 * @brief 注册设置 PIR 告警配置的回调函数
 * @param [in] pCb 用于读取 NET_PirAlarmInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetPirAlarmInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PIR_ALARM_INFO, pCb);
}

/**
 * @brief 注册获取隐私遮挡配置的回调函数
 * @param [in] pCb 用于填充 NET_PrivacyMaskCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetPrivacyMaskConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PRIVACYMASKCFG, pCb);
}

/**
 * @brief 注册设置隐私遮挡配置的回调函数
 * @param [in] pCb 用于读取 NET_PrivacyMaskCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetPrivacyMaskConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PRIVACYMASKCFG, pCb);
}

/**
 * @brief 注册获取防拆报警配置的回调函数
 * @param [in] pCb 用于填充 NET_TamperAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetTamperAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_TAMPERALARM, pCb);
}

/**
 * @brief 注册设置防拆报警配置的回调函数
 * @param [in] pCb 用于读取 NET_TamperAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetTamperAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_TAMPERALARM, pCb);
}

/**
 * @brief 注册获取移动侦测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_MotionAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetMotionAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_MOTIONALARM, pCb);
}

/**
 * @brief 注册设置移动侦测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_MotionAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetMotionAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_MOTIONALARM, pCb);
}

/* ===================== 智能分析报警回调 ===================== */

/**
 * @brief 注册获取越界检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_CrossLineAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetCrossLineAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_CROSSLINEALARM, pCb);
}

/**
 * @brief 注册设置越界检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_CrossLineAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetCrossLineAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_CROSSLINEALARM, pCb);
}

/**
 * @brief 注册获取入侵检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_IntrusionAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetIntrusionAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_INTRUSIONALARM, pCb);
}

/**
 * @brief 注册设置入侵检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_IntrusionAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetIntrusionAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_INTRUSIONALARM, pCb);
}

/**
 * @brief 注册获取进入区域检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_EnterRegionAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetEnterRegionAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_ENTERREGIONALARM, pCb);
}

/**
 * @brief 注册设置进入区域检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_EnterRegionAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetEnterRegionAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_ENTERREGIONALARM, pCb);
}

/**
 * @brief 注册获取离开区域检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_LeaveRegionAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetLeaveRegionAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_LEAVEREGIONALARM, pCb);
}

/**
 * @brief 注册设置离开区域检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_LeaveRegionAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetLeaveRegionAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_LEAVEREGIONALARM, pCb);
}

/**
 * @brief 注册获取徘徊检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_LoiteringAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetLoiteringAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_LOITERINGALARM, pCb);
}

/**
 * @brief 注册设置徘徊检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_LoiteringAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetLoiteringAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_LOITERINGALARM, pCb);
}

/**
 * @brief 注册获取场景变更检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_SceneChangeAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetSceneChangeAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_SCENECHANGEALARM, pCb);
}

/**
 * @brief 注册设置场景变更检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_SceneChangeAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetSceneChangeAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_SCENECHANGEALARM, pCb);
}

/**
 * @brief 注册获取人群聚集检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_CrowGatheringAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetCrowGatheringAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_CROWDGATHERINGALARM, pCb);
}

/**
 * @brief 注册设置人群聚集检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_CrowGatheringAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetCrowGatheringAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_CROWDGATHERINGALARM, pCb);
}

/**
 * @brief 注册获取停车检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_ParkingAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetParkingAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PARKINGALARM, pCb);
}

/**
 * @brief 注册设置停车检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_ParkingAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetParkingAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PARKINGALARM, pCb);
}

/**
 * @brief 注册获取遗留物检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_UnattendedObjectAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetUnattendedObjectAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_UNATTENDEDOBJECTALARM, pCb);
}

/**
 * @brief 注册设置遗留物检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_UnattendedObjectAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetUnattendedObjectAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_UNATTENDEDOBJECTALARM, pCb);
}

/**
 * @brief 注册获取物品搬移检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_ObjectRemovalAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetObjectRemovalAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_OBJECTREMOVALALARM, pCb);
}

/**
 * @brief 注册设置物品搬移检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_ObjectRemovalAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetObjectRemovalAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_OBJECTREMOVALALARM, pCb);
}

/**
 * @brief 注册获取音频异常检测报警配置的回调函数
 * @param [in] pCb 用于填充 NET_AudioAnomalyAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetAudioAnomalyAlarmCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_AUDIOANOMALYALARM, pCb);
}

/**
 * @brief 注册设置音频异常检测报警配置的回调函数
 * @param [in] pCb 用于读取 NET_AudioAnomalyAlarm_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetAudioAnomalyAlarmCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_AUDIOANOMALYALARM, pCb);
}

/* ===================== 人脸识别相关回调 ===================== */

/**
 * @brief 注册获取人脸抓拍配置的回调函数
 * @param [in] pCb 用于填充 NET_FaceCaptureInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetFaceCaptureInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_FACECAPTUREINFO, pCb);
}

/**
 * @brief 注册设置人脸抓拍配置的回调函数
 * @param [in] pCb 用于读取 NET_FaceCaptureInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetFaceCaptureInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_FACECAPTUREINFO, pCb);
}

/**
 * @brief 注册设置人脸比对配置的回调函数
 * @param [in] pCb 用于读取 NET_FaceCompareInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetFaceCompareInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_FACE_COMPARE_INFO, pCb);
}

/**
 * @brief 注册添加目标库（人脸库）的回调函数
 * @param [in] pCb 用于执行添加目标库操作的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterAddTargetLibCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_ADD_TARGET_LIB, pCb);
}

/**
 * @brief 注册删除目标库（人脸库）的回调函数
 * @param [in] pCb 用于执行删除目标库操作的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterDelTargetLibCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_DEL_TARGET_LIB, pCb);
}

/**
 * @brief 注册设置目标库（人脸库）的回调函数
 * @param [in] pCb 用于修改目标库信息的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetTargetLibCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_TARGET_LIB, pCb);
}

/**
 * @brief 注册获取目标库（人脸库）的回调函数
 * @param [in] pCb 用于填充 NET_TargetLibList_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetTargetLibCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_TARGET_LIB, pCb);
}

/**
 * @brief 注册添加人脸信息的回调函数
 * @param [in] pCb 用于执行添加单条人脸信息操作的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterAddFaceInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_ADD_FACE_INFO, pCb);
}

/**
 * @brief 注册删除人脸信息的回调函数
 * @param [in] pCb 用于执行删除单条人脸信息操作的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterDelFaceInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_DEL_FACE_INFO, pCb);
}

/**
 * @brief 注册设置人脸信息的回调函数
 * @param [in] pCb 用于修改单条人脸信息的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetFaceInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_FACE_INFO, pCb);
}

/**
 * @brief 注册获取人脸信息的回调函数
 * @param [in] pCb 用于填充 NET_FaceInfoList_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetFaceInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_FACE_INFO, pCb);
}

/* ===================== 人流统计回调 ===================== */

/**
 * @brief 注册获取人流统计配置的回调函数
 * @param [in] pCb 用于填充 NET_PeopleFlowStatisticsCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetPeopleFlowStatisticsConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PEOPLE_FLOW_STATISTICS_CFG, pCb);
}

/**
 * @brief 注册设置人流统计配置的回调函数
 * @param [in] pCb 用于读取 NET_PeopleFlowStatisticsCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetPeopleFlowStatisticsConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PEOPLE_FLOW_STATISTICS_CFG, pCb);
}

/**
 * @brief 注册重置人流统计数据的回调函数
 * @param [in] pCb 用于执行人流统计数据清零操作的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterResetPeopleFlowStatisticsCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_RESET_PEOPLE_FLOW_STATISTICS, pCb);
}

/**
 * @brief 注册获取人群密度检测配置的回调函数
 * @param [in] pCb 用于填充 NET_PeopleDensityDetectionCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetPeopleDensityDetectionConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PEOPLE_DENSITY_DETECTION_CFG, pCb);
}

/**
 * @brief 注册设置人群密度检测配置的回调函数
 * @param [in] pCb 用于读取 NET_PeopleDensityDetectionCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetPeopleDensityDetectionConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PEOPLE_DENSITY_DETECTION_CFG, pCb);
}

/* ===================== 行业AI检测回调 ===================== */

/**
 * @brief 注册获取井盖异常检测配置的回调函数
 * @param [in] pCb 用于填充 NET_ManholeCoverAbnormalCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetManholeCoverAbnormalConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_MANHOLE_COVER_ABNORMAL_CFG, pCb);
}

/**
 * @brief 注册设置井盖异常检测配置的回调函数
 * @param [in] pCb 用于读取 NET_ManholeCoverAbnormalCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetManholeCoverAbnormalConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_MANHOLE_COVER_ABNORMAL_CFG, pCb);
}

/**
 * @brief 注册获取睡岗检测配置的回调函数
 * @param [in] pCb 用于填充 NET_SleepOnDutyCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetSleepOnDutyConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_SLEEP_ON_DUTY_CFG, pCb);
}

/**
 * @brief 注册设置睡岗检测配置的回调函数
 * @param [in] pCb 用于读取 NET_SleepOnDutyCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetSleepOnDutyConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_SLEEP_ON_DUTY_CFG, pCb);
}

/**
 * @brief 注册获取电瓶车入电梯检测配置的回调函数
 * @param [in] pCb 用于填充 NET_ElectricVehicleInElevatorCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetElectricVehicleInElevatorConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, pCb);
}

/**
 * @brief 注册设置电瓶车入电梯检测配置的回调函数
 * @param [in] pCb 用于读取 NET_ElectricVehicleInElevatorCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetElectricVehicleInElevatorConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, pCb);
}

/**
 * @brief 注册获取人员跌倒检测配置的回调函数（旧版本，建议使用 PersonFallCfg）
 * @param [in] pCb 用于填充 NET_PersonFallDownCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetPersonFallDownConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PERSON_FALL_DOWN_CFG, pCb);
}

/**
 * @brief 注册设置人员跌倒检测配置的回调函数（旧版本，建议使用 PersonFallCfg）
 * @param [in] pCb 用于读取 NET_PersonFallDownCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetPersonFallDownConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PERSON_FALL_DOWN_CFG, pCb);
}

/**
 * @brief 注册获取施工占道检测配置的回调函数
 * @param [in] pCb 用于填充 NET_ConstructionOccupyRoadCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetConstructionOccupyRoadConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG, pCb);
}

/**
 * @brief 注册设置施工占道检测配置的回调函数
 * @param [in] pCb 用于读取 NET_ConstructionOccupyRoadCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetConstructionOccupyRoadConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG, pCb);
}

/**
 * @brief 注册获取交通拥堵检测配置的回调函数
 * @param [in] pCb 用于填充 NET_CongestionCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetCongestionConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_CONGESTION_CFG, pCb);
}

/**
 * @brief 注册设置交通拥堵检测配置的回调函数
 * @param [in] pCb 用于读取 NET_CongestionCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetCongestionConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_CONGESTION_CFG, pCb);
}

/**
 * @brief 注册获取车牌识别配置的回调函数
 * @param [in] pCb 用于填充 NET_LicensePlateRecognitionCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetLicensePlateRecognitionConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_LICENSE_PLATE_RECOGNITION_CFG, pCb);
}

/**
 * @brief 注册设置车牌识别配置的回调函数
 * @param [in] pCb 用于读取 NET_LicensePlateRecognitionCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetLicensePlateRecognitionConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_LICENSE_PLATE_RECOGNITION_CFG, pCb);
}

/**
 * @brief 注册获取高空未系安全带检测配置的回调函数
 * @param [in] pCb 用于填充 NET_HighAltitudeSeatbeltCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetHighAltitudeSeatbeltConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_HIGH_ALTITUDE_SEATBELT_CFG, pCb);
}

/**
 * @brief 注册设置高空未系安全带检测配置的回调函数
 * @param [in] pCb 用于读取 NET_HighAltitudeSeatbeltCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetHighAltitudeSeatbeltConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_HIGH_ALTITUDE_SEATBELT_CFG, pCb);
}

/**
 * @brief 注册获取安全帽检测配置的回调函数
 * @param [in] pCb 用于填充 NET_SafetyHelmetCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetSafetyHelmetConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_SAFETY_HELMET_CFG, pCb);
}

/**
 * @brief 注册设置安全帽检测配置的回调函数
 * @param [in] pCb 用于读取 NET_SafetyHelmetCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetSafetyHelmetConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_SAFETY_HELMET_CFG, pCb);
}

/**
 * @brief 注册获取人员跌倒检测配置的回调函数（新版本）
 * @param [in] pCb 用于填充 NET_PersonFallCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetPersonFallConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PERSON_FALL_CFG, pCb);
}

/**
 * @brief 注册设置人员跌倒检测配置的回调函数（新版本）
 * @param [in] pCb 用于读取 NET_PersonFallCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetPersonFallConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PERSON_FALL_CFG, pCb);
}

/**
 * @brief 注册获取玩手机检测配置的回调函数
 * @param [in] pCb 用于填充 NET_PhoneUsageCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetPhoneUsageConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PHONE_USAGE_CFG, pCb);
}

/**
 * @brief 注册设置玩手机检测配置的回调函数
 * @param [in] pCb 用于读取 NET_PhoneUsageCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetPhoneUsageConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PHONE_USAGE_CFG, pCb);
}

/**
 * @brief 注册获取吸烟检测配置的回调函数
 * @param [in] pCb 用于填充 NET_SmokingCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetSmokingConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_SMOKING_CFG, pCb);
}

/**
 * @brief 注册设置吸烟检测配置的回调函数
 * @param [in] pCb 用于读取 NET_SmokingCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetSmokingConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_SMOKING_CFG, pCb);
}

/**
 * @brief 注册获取明火检测配置的回调函数
 * @param [in] pCb 用于填充 NET_OpenFlameCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetOpenFlameConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_OPEN_FLAME_CFG, pCb);
}

/**
 * @brief 注册设置明火检测配置的回调函数
 * @param [in] pCb 用于读取 NET_OpenFlameCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetOpenFlameConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_OPEN_FLAME_CFG, pCb);
}

/**
 * @brief 注册获取裸土检测配置的回调函数
 * @param [in] pCb 用于填充 NET_BareSoilCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetBareSoilConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_BARE_SOIL_CFG, pCb);
}

/**
 * @brief 注册设置裸土检测配置的回调函数
 * @param [in] pCb 用于读取 NET_BareSoilCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetBareSoilConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_BARE_SOIL_CFG, pCb);
}

/**
 * @brief 注册获取洞口防护栏检测配置的回调函数
 * @param [in] pCb 用于填充 NET_HoleProtectionBarCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetHoleProtectionBarConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_HOLE_PROTECTION_BAR_CFG, pCb);
}

/**
 * @brief 注册设置洞口防护栏检测配置的回调函数
 * @param [in] pCb 用于读取 NET_HoleProtectionBarCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetHoleProtectionBarConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_HOLE_PROTECTION_BAR_CFG, pCb);
}

/**
 * @brief 注册获取反光衣检测配置的回调函数
 * @param [in] pCb 用于填充 NET_ReflectiveClothingCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetReflectiveClothingConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_REFLECTIVE_CLOTHING_CFG, pCb);
}

/**
 * @brief 注册设置反光衣检测配置的回调函数
 * @param [in] pCb 用于读取 NET_ReflectiveClothingCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetReflectiveClothingConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_REFLECTIVE_CLOTHING_CFG, pCb);
}

/**
 * @brief 注册获取宠物识别配置的回调函数
 * @param [in] pCb 用于填充 NET_PetRecognitionInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetPetRecognitionInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PET_RECOGNITION_INFO, pCb);
}

/**
 * @brief 注册设置宠物识别配置的回调函数
 * @param [in] pCb 用于读取 NET_PetRecognitionInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetPetRecognitionInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PET_RECOGNITION_INFO, pCb);
}

/**
 * @brief 注册获取攀爬围栏检测配置的回调函数
 * @param [in] pCb 用于填充 NET_ClimbFenceInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetClimbFenceInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_CLIMB_FENCE_INFO, pCb);
}

/**
 * @brief 注册设置攀爬围栏检测配置的回调函数
 * @param [in] pCb 用于读取 NET_ClimbFenceInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetClimbFenceInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_CLIMB_FENCE_INFO, pCb);
}

/**
 * @brief 注册获取离职检测配置的回调函数
 * @param [in] pCb 用于填充 NET_DimissionInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetDimissionInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_DIMISSION_INFO, pCb);
}

/**
 * @brief 注册设置离职检测配置的回调函数
 * @param [in] pCb 用于读取 NET_DimissionInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetDimissionInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_DIMISSION_INFO, pCb);
}

/**
 * @brief 注册获取占用应急车道检测配置的回调函数
 * @param [in] pCb 用于填充 NET_IllegalLaneInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetIllegalLaneInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_ILLEGAL_LANE_INFO, pCb);
}

/**
 * @brief 注册设置占用应急车道检测配置的回调函数
 * @param [in] pCb 用于读取 NET_IllegalLaneInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetIllegalLaneInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_ILLEGAL_LANE_INFO, pCb);
}

/**
 * @brief 注册获取逆行检测配置的回调函数
 * @param [in] pCb 用于填充 NET_RetrogradeInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetRetrogradeInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_RETROGRADE_INFO, pCb);
}

/**
 * @brief 注册设置逆行检测配置的回调函数
 * @param [in] pCb 用于读取 NET_RetrogradeInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetRetrogradeInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_RETROGRADE_INFO, pCb);
}

/**
 * @brief 注册获取非机动车入侵检测配置的回调函数
 * @param [in] pCb 用于填充 NET_NonmotorVehicleIntrusionInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetNonmotorVehicleIntrusionInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO, pCb);
}

/**
 * @brief 注册设置非机动车入侵检测配置的回调函数
 * @param [in] pCb 用于读取 NET_NonmotorVehicleIntrusionInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetNonmotorVehicleIntrusionInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO, pCb);
}

/**
 * @brief 注册获取占道经营检测配置的回调函数
 * @param [in] pCb 用于填充 NET_OccupationEmergencyInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetOccupationEmergencyInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_OCCUPATION_EMERGENCY_INFO, pCb);
}

/**
 * @brief 注册设置占道经营检测配置的回调函数
 * @param [in] pCb 用于读取 NET_OccupationEmergencyInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetOccupationEmergencyInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_OCCUPATION_EMERGENCY_INFO, pCb);
}

/**
 * @brief 注册获取行人入侵检测配置的回调函数
 * @param [in] pCb 用于填充 NET_PedestrianIntrusionInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetPedestrianIntrusionInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_PEDESTRIAN_INTRUSION_INFO, pCb);
}

/**
 * @brief 注册设置行人入侵检测配置的回调函数
 * @param [in] pCb 用于读取 NET_PedestrianIntrusionInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetPedestrianIntrusionInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_PEDESTRIAN_INTRUSION_INFO, pCb);
}

/**
 * @brief 注册获取烟雾火焰检测配置的回调函数
 * @param [in] pCb 用于填充 NET_SmokeFireCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetSmokeFireConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_SMOKE_FIRE_CFG, pCb);
}

/**
 * @brief 注册设置烟雾火焰检测配置的回调函数
 * @param [in] pCb 用于读取 NET_SmokeFireCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetSmokeFireConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_SMOKE_FIRE_CFG, pCb);
}

/**
 * @brief 注册获取道路积水检测配置的回调函数
 * @param [in] pCb 用于填充 NET_RoadPondingCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetRoadPondingConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_ROAD_PONDING_CFG, pCb);
}

/**
 * @brief 注册设置道路积水检测配置的回调函数
 * @param [in] pCb 用于读取 NET_RoadPondingCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetRoadPondingConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_ROAD_PONDING_CFG, pCb);
}
