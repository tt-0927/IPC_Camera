#ifndef _ALARMINFO_CONVERT_H
#define _ALARMINFO_CONVERT_H

#include <string>
#include "Json.h"

// 库通用头文件
#ifdef NET_TV_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_TV_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif

namespace SDKConvert
{
    void deal(Json::Object* pRootJson, NET_TV_ALARMER_S& stAlarmInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_BASIC_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_RULE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_AI_OBJECT_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_FACE_COMPARE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_PLATE_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_EXCEPTION_INFO_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_STATISTICS_TARGET_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_STATISTICS_INFO_S& stInfo, bool bOutStruct);
}

#endif
