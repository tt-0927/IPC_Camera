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
    void deal(Json::Object* pRootJson, NET_Alarmer_S& stAlarmInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmBasicInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmRuleInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmAiObjectInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmFaceCompareInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmPlateInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmExceptionInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmStatisticsTarget_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AlarmStatisticsInfo_S& stInfo, bool bOutStruct);
}

#endif
