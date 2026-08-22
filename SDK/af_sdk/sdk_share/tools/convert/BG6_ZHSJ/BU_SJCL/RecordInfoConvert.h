/*
 * @FilePath     : sdk_new/sdk_share/tools/convert/BG6_ZHSJ/BU_SJCL/RecordInfoConvert.h
 * @Author       : ITC
 * @Date         : 2026-08-21
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-21
 * @Description  : 录像/回放/通道 相关转换
 *                 收口 Record/Replay/Channel/Rtsp/RecordFrame 等 NVR/录播侧纯录像回放结构体。
 */
#pragma once

#include <string>
#include <vector>
#include <set>

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
    void deal(Json::Object* pRootJson, NET_RecordInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordStatusInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordTime_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordDaySchedule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordSchedule_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordAdvancedParam_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFindCond_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordVideoTime_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFindResult_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFileList_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordDownloadInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordDownloadProgress_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordDownloadList_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_ReplayUrlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayCtrlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayRecordTime_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ReplayRecordList_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_RtspUrlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFrameStreamCond_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFrameStreamInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFrameStopInfo_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_ChannelInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ChannelList_S& stInfo, bool bOutStruct);
}
