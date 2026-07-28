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
    void deal(Json::Object* pRootJson, NET_ALARMER_S& stAlarmInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ALARM_BASIC_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ALARM_RULE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ALARM_AI_OBJECT_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ALARM_FACE_COMPARE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ALARM_PLATE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ALARM_EXCEPTION_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ALARM_STATISTICS_TARGET_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ALARM_STATISTICS_INFO_S& stInfo, bool bOutStruct);
}

#endif
