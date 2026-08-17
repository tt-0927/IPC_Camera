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
    void deal(Json::Object* pRootJson, NET_AlarmPlateInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmExceptionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmStatisticsTarget_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmStatisticsInfo_S& stInfo, bool bOutStruct);

    /** @brief 转换抓拍区域多边形。 */
    void deal(Json::Object* pRootJson, NET_CapturePolygon_S& stInfo, bool bOutStruct);
    /** @brief 转换人脸抓拍推送信息。 */
    void deal(Json::Object* pRootJson, NET_FaceCapturePushInfo_S& stInfo, bool bOutStruct);
    /** @brief 转换行人抓拍推送信息。 */
    void deal(Json::Object* pRootJson, NET_PersonCapturePushInfo_S& stInfo, bool bOutStruct);
    /** @brief 转换机动车抓拍推送信息。 */
    void deal(Json::Object* pRootJson, NET_MotorvehicleCapturePushInfo_S& stInfo, bool bOutStruct);
    /** @brief 转换非机动车抓拍推送信息。 */
    void deal(Json::Object* pRootJson, NET_NonMotorvehicleCapturePushInfo_S& stInfo, bool bOutStruct);

    /** @brief 转换基础告警 V2 信息。 */
    void deal(Json::Object* pRootJson, NET_AlarmBasicInfoV2_S& stInfo, bool bOutStruct);
    /** @brief 转换规则告警 V2 信息。 */
    void deal(Json::Object* pRootJson, NET_AlarmRuleInfoV2_S& stInfo, bool bOutStruct);
    /** @brief 转换 AI 对象告警 V2 信息。 */
    void deal(Json::Object* pRootJson, NET_AlarmAiObjectInfoV2_S& stInfo, bool bOutStruct);
    /** @brief 转换人脸比对告警 V2 信息。 */
    void deal(Json::Object* pRootJson, NET_AlarmFaceCompareInfoV2_S& stInfo, bool bOutStruct);
    /** @brief 转换车牌告警 V2 信息。 */
    void deal(Json::Object* pRootJson, NET_AlarmPlateInfoV2_S& stInfo, bool bOutStruct);
    /** @brief 转换统计告警目标 V2 信息。 */
    void deal(Json::Object* pRootJson, NET_AlarmStatisticsTargetV2_S& stInfo, bool bOutStruct);
    /** @brief 转换统计告警 V2 信息。 */
    void deal(Json::Object* pRootJson, NET_AlarmStatisticsInfoV2_S& stInfo, bool bOutStruct);
}

#endif
