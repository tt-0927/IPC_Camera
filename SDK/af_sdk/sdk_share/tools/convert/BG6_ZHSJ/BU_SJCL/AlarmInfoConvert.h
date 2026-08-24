/**
 * @file AlarmInfoConvert.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief AlarmInfoConvert 模块接口与类型定义
 * 功能说明：
 * 1. 声明 AlarmInfoConvert 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_ALARM_INFO_CONVERT_H
#define NETSDK_ALARM_INFO_CONVERT_H

#include <string>
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
    void deal(Json::Object* pRootJson, NET_Alarmer_S& stAlarmInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmBasicInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmRuleInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmAiObjectInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmFaceCompareInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmCaptureInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmPlateInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmExceptionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmStatisticsTarget_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmStatisticsInfo_S& stInfo, bool bOutStruct);

    /* 区域入侵报警相关 */
    void deal(Json::Object* pRootJson, NET_EnterRegionAlarmInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LeaveRegionAlarmInfo_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_LinkageList_S& stInfo, bool bOutStruct);

    /* 移动侦测相关 */
    void deal(Json::Object* pRootJson, NET_MotionRegion_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionExpertMode_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionNormalMode_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_MotionAlarmInfo_S& stInfo, bool bOutStruct);

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

    void deal(Json::Object* pRootJson, NET_AiSimpleRule_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_SmartRegion_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SmartRegionRule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SmartLineRule_S& stInfo, bool bOutStruct);

        
    void deal(Json::Object* pRootJson, NET_SchedTime_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmSchedule_S& stInfo, bool bOutStruct);

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


}

#endif
