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
    void deal(Json::Object* pRootJson, NET_TV_ALARM_BASIC_INFO_V2_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_RULE_INFO_V2_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_AI_OBJECT_INFO_V2_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_FACE_COMPARE_INFO_V2_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_PLATE_INFO_V2_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_STATISTICS_TARGET_V2_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_TV_ALARM_STATISTICS_INFO_V2_S& stInfo, bool bOutStruct);
    
    /**
     * @brief 转换抓拍区域多边形信息。
     * @author ITC
     * @param [in] pRootJson 源 JSON 对象。
     * @param [in,out] stInfo 抓拍区域多边形信息。
     * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_CapturePolygon_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换人脸抓拍推送信息。
     * @author ITC
     * @param [in] pRootJson 源 JSON 对象。
     * @param [in,out] stInfo 人脸抓拍推送信息。
     * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_FaceCapturePushInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换行人抓拍推送信息。
     * @author ITC
     * @param [in] pRootJson 源 JSON 对象。
     * @param [in,out] stInfo 行人抓拍推送信息。
     * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_PersonCapturePushInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换机动车抓拍推送信息。
     * @author ITC
     * @param [in] pRootJson 源 JSON 对象。
     * @param [in,out] stInfo 机动车抓拍推送信息。
     * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_MotorvehicleCapturePushInfo_S& stInfo, bool bOutStruct);

    /**
     * @brief 转换非机动车抓拍推送信息。
     * @author ITC
     * @param [in] pRootJson 源 JSON 对象。
     * @param [in,out] stInfo 非机动车抓拍推送信息。
     * @param [in] bOutStruct true 表示 JSON 转结构体，false 表示结构体转 JSON。
     * @return 无。
     */
    void deal(Json::Object* pRootJson, NET_NonMotorvehicleCapturePushInfo_S& stInfo, bool bOutStruct);

}

#endif
