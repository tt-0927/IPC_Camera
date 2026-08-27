/*
 * @FilePath     : sdk_new/sdk_share/tools/convert/BG6_ZHSJ/BU_SJCL/RecordInfoConvert.cpp
 * @Author       : ITC
 * @Date         : 2026-08-21
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-21
 * @Description  : 录像/回放/通道 相关转换
 *                 收口 Record/Replay/Channel/Rtsp/RecordFrame 等 NVR/录播侧纯录像回放结构体。
 */

#include "RecordInfoConvert.h"
#include "SDKConvert.h"

#include <algorithm>
#include <vector>
#include <cstring>
#include <string>

namespace SDKConvert
{

void deal(Json::Object* pRootJson, NET_RecordInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "VideoStatus", stInfo.nVideoStatus);
    convert.field(pRootJson, "AudioStatus", stInfo.nAudioStatus);
    convert.field(pRootJson, "RecordStatus", stInfo.nRecordStatus);
    convert.field(pRootJson, "RecordFormat", stInfo.nRecordFormat);
    convert.field(pRootJson, "EventType", stInfo.nEventType);
    convert.field(pRootJson, "Path", stInfo.szPath);
    convert.field(pRootJson, "RedunPath", stInfo.szRedunPath);
    convert.field(pRootJson, "RecordName", stInfo.szRecordName);
    convert.field(pRootJson, "RecordTime", stInfo.szRecordTime);
    convert.field(pRootJson, "StreamType", stInfo.nStreamType);
}


void deal(Json::Object* pRootJson, NET_RecordStatusInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Status", stInfo.nStatus);
}


void deal(Json::Object* pRootJson, NET_RecordTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", stInfo.nType);
    convert.field(pRootJson, "StartTime", stInfo.nStartTime);
    convert.field(pRootJson, "EndTime", stInfo.nEndTime);
}


void deal(Json::Object* pRootJson, NET_RecordDaySchedule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "DayOfWeek", stInfo.nDayOfWeek);

    if (!bOutStruct)
    {
        if (stInfo.nRecordTimeCount < 0)
        {
            stInfo.nRecordTimeCount = 0;
        }
        if (stInfo.nRecordTimeCount > NET_TIME_DURATION_NUM)
        {
            stInfo.nRecordTimeCount = NET_TIME_DURATION_NUM;
        }
    }
    convert.field(pRootJson, "RecordTimeCount", stInfo.nRecordTimeCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "RecordTimes");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TIME_DURATION_NUM)
        {
            nCount = NET_TIME_DURATION_NUM;
        }
        if (nCount < 0)
        {
            nCount = 0;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astRecordTimes[i], bOutStruct);
            }
        }
        stInfo.nRecordTimeCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }
        for (int i = 0; i < stInfo.nRecordTimeCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astRecordTimes[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "RecordTimes", pArray);
    }
}


void deal(Json::Object* pRootJson, NET_RecordSchedule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "IsEnable", stInfo.bEnable);

    if (!bOutStruct)
    {
        if (stInfo.nDayScheduleCount < 0)
        {
            stInfo.nDayScheduleCount = 0;
        }
        if (stInfo.nDayScheduleCount > NET_PLAN_DAY_NUM_AWEEK)
        {
            stInfo.nDayScheduleCount = NET_PLAN_DAY_NUM_AWEEK;
        }
    }
    convert.field(pRootJson, "DayScheduleCount", stInfo.nDayScheduleCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "DaySchedules");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_PLAN_DAY_NUM_AWEEK)
        {
            nCount = NET_PLAN_DAY_NUM_AWEEK;
        }
        if (nCount < 0)
        {
            nCount = 0;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astDaySchedules[i], bOutStruct);
            }
        }
        stInfo.nDayScheduleCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }
        for (int i = 0; i < stInfo.nDayScheduleCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astDaySchedules[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "DaySchedules", pArray);
    }
}


void deal(Json::Object* pRootJson, NET_RecordAdvancedParam_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "LoopWrite", stInfo.bLoopWrite);
    convert.field(pRootJson, "PreTime", stInfo.nPreTime);
    convert.field(pRootJson, "DelayTime", stInfo.nDelayTime);
    convert.field(pRootJson, "StreamType", stInfo.nStreamType);
}


void deal(Json::Object* pRootJson, NET_RecordFindCond_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "Type", stInfo.nType);
    convert.field(pRootJson, "Year", stInfo.szYear);
    convert.field(pRootJson, "Month", stInfo.szMonth);
    convert.field(pRootJson, "Date", stInfo.szDate);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "Filename", stInfo.szFilename);
}


void deal(Json::Object* pRootJson, NET_RecordVideoTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartTime", stInfo.nStartTime);
    convert.field(pRootJson, "EndTime", stInfo.nEndTime);
}


void deal(Json::Object* pRootJson, NET_RecordFindResult_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);

    if (!bOutStruct)
    {
        if (stInfo.nDateCount < 0)
        {
            stInfo.nDateCount = 0;
        }
        if (stInfo.nDateCount > NET_RECORD_DATE_MAX_NUM)
        {
            stInfo.nDateCount = NET_RECORD_DATE_MAX_NUM;
        }
        if (stInfo.nVideoTimeCount < 0)
        {
            stInfo.nVideoTimeCount = 0;
        }
        if (stInfo.nVideoTimeCount > NET_TIME_DURATION_NUM)
        {
            stInfo.nVideoTimeCount = NET_TIME_DURATION_NUM;
        }
    }

    convert.field(pRootJson, "DateCount", stInfo.nDateCount);
    if (bOutStruct)
    {
        Json::Object* pDates = Json::get(pRootJson, "Dates");
        int nSize = pDates ? Json::Array::size(pDates) : 0;
        int nCount = nSize;
        if (nCount > NET_RECORD_DATE_MAX_NUM)
        {
            nCount = NET_RECORD_DATE_MAX_NUM;
        }
        for (int i = 0; pDates && i < nCount; ++i)
        {
            std::string strDate;
            Json::Object* pItem = Json::Array::get(pDates, i);
            if (pItem)
            {
                Json::Value::get(pItem, strDate);
                std::strncpy(stInfo.aszDates[i], strDate.c_str(), sizeof(stInfo.aszDates[i]) - 1);
            }
        }
        stInfo.nDateCount = nCount;
    }
    else
    {
        Json::Object* pDates = Json::Array::init();
        if (pDates)
        {
            for (int i = 0; i < stInfo.nDateCount; ++i)
            {
                Json::Array::add(pDates, stInfo.aszDates[i]);
            }
            Json::add(pRootJson, "Dates", pDates);
        }
    }

    convert.field(pRootJson, "Filename", stInfo.szFilename);
    convert.field(pRootJson, "VideoTimeCount", stInfo.nVideoTimeCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "VideoTimes");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TIME_DURATION_NUM)
        {
            nCount = NET_TIME_DURATION_NUM;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astVideoTimes[i], bOutStruct);
            }
        }
        stInfo.nVideoTimeCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }
        for (int i = 0; i < stInfo.nVideoTimeCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astVideoTimes[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "VideoTimes", pArray);
    }
}


void deal(Json::Object* pRootJson, NET_RecordFileList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.structure(pRootJson, "Find", stInfo.stFind);

    if (!bOutStruct)
    {
        if (stInfo.nResultCount < 0)
        {
            stInfo.nResultCount = 0;
        }
        if (stInfo.nResultCount > NET_RECORD_FILE_MAX_NUM)
        {
            stInfo.nResultCount = NET_RECORD_FILE_MAX_NUM;
        }
    }
    convert.field(pRootJson, "ResultCount", stInfo.nResultCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "Infos");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_RECORD_FILE_MAX_NUM)
        {
            nCount = NET_RECORD_FILE_MAX_NUM;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astResults[i], bOutStruct);
            }
        }
        stInfo.nResultCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }
        for (int i = 0; i < stInfo.nResultCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astResults[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "Infos", pArray);
    }
}


void deal(Json::Object* pRootJson, NET_RecordDownloadInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "Path", stInfo.szPath);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
}


void deal(Json::Object* pRootJson, NET_RecordDownloadProgress_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Filename", stInfo.szFilename);
    convert.field(pRootJson, "DownloadProgress", stInfo.nProgress);
}


void deal(Json::Object* pRootJson, NET_RecordDownloadList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    if (!bOutStruct)
    {
        if (stInfo.nDownloadCount < 0)
        {
            stInfo.nDownloadCount = 0;
        }
        if (stInfo.nDownloadCount > NET_RECORD_DOWNLOAD_MAX_NUM)
        {
            stInfo.nDownloadCount = NET_RECORD_DOWNLOAD_MAX_NUM;
        }
        if (stInfo.nProgressCount < 0)
        {
            stInfo.nProgressCount = 0;
        }
        if (stInfo.nProgressCount > NET_RECORD_DOWNLOAD_MAX_NUM)
        {
            stInfo.nProgressCount = NET_RECORD_DOWNLOAD_MAX_NUM;
        }
    }

    convert.field(pRootJson, "DownloadCount", stInfo.nDownloadCount);
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "DownloadInfos");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_RECORD_DOWNLOAD_MAX_NUM)
        {
            nCount = NET_RECORD_DOWNLOAD_MAX_NUM;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astDownloads[i], bOutStruct);
            }
        }
        stInfo.nDownloadCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (pArray)
        {
            for (int i = 0; i < stInfo.nDownloadCount; ++i)
            {
                Json::Object* pItem = Json::init();
                if (!pItem)
                {
                    continue;
                }
                deal(pItem, stInfo.astDownloads[i], bOutStruct);
                Json::Array::add(pArray, pItem);
            }
            Json::add(pRootJson, "DownloadInfos", pArray);
        }
    }

    convert.field(pRootJson, "ProgressCount", stInfo.nProgressCount);
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "DownloadProgressInfos");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_RECORD_DOWNLOAD_MAX_NUM)
        {
            nCount = NET_RECORD_DOWNLOAD_MAX_NUM;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astProgress[i], bOutStruct);
            }
        }
        stInfo.nProgressCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (pArray)
        {
            for (int i = 0; i < stInfo.nProgressCount; ++i)
            {
                Json::Object* pItem = Json::init();
                if (!pItem)
                {
                    continue;
                }
                deal(pItem, stInfo.astProgress[i], bOutStruct);
                Json::Array::add(pArray, pItem);
            }
            Json::add(pRootJson, "DownloadProgressInfos", pArray);
        }
    }
}


void deal(Json::Object* pRootJson, NET_ReplayUrlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.uChannel);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "Url", stInfo.szUrl);
}


void deal(Json::Object* pRootJson, NET_ReplayCtrlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.uChannel);
    convert.field(pRootJson, "CtrlType", stInfo.uCtrlType);
    convert.field(pRootJson, "Speed", stInfo.fSpeed);
    convert.field(pRootJson, "SeekTime", stInfo.nSeekTime);
    convert.field(pRootJson, "ReplayType", stInfo.nReplayType);
    convert.field(pRootJson, "SessionId", stInfo.szSessionId);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "Url", stInfo.szUrl);
}


void deal(Json::Object* pRootJson, NET_ReplayRecordTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartTime", stInfo.nStartTime);
    convert.field(pRootJson, "EndTime", stInfo.nEndTime);
}


void deal(Json::Object* pRootJson, NET_ReplayRecordList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.uChannel);
    convert.field(pRootJson, "FilterByEventType", stInfo.bFilterByEventType);
    convert.field(pRootJson, "EventType", stInfo.uEventType);
    convert.field(pRootJson, "Date", stInfo.szDate);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "VideoCount", stInfo.nVideoCount);
    convert.field(pRootJson, "PersonEventCount", stInfo.nPersonEventCount);
    convert.field(pRootJson, "VehicleEventCount", stInfo.nVehicleEventCount);
    convert.field(pRootJson, "OtherEventCount", stInfo.nOtherEventCount);

    if (bOutStruct)
    {
        std::vector<NET_ReplayRecordTime_S> videoTimes;
        std::vector<NET_ReplayRecordTime_S> personEventTimes;
        std::vector<NET_ReplayRecordTime_S> vehicleEventTimes;
        std::vector<NET_ReplayRecordTime_S> otherEventTimes;

        convert.structure(pRootJson, "VideoTimes", videoTimes);
        convert.structure(pRootJson, "PersonEventTimes", personEventTimes);
        convert.structure(pRootJson, "VehicleEventTimes", vehicleEventTimes);
        convert.structure(pRootJson, "OtherEventTimes", otherEventTimes);

        stInfo.nVideoCount = (INT32)std::min<size_t>(videoTimes.size(), NET_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nPersonEventCount = (INT32)std::min<size_t>(personEventTimes.size(), NET_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nVehicleEventCount = (INT32)std::min<size_t>(vehicleEventTimes.size(), NET_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nOtherEventCount = (INT32)std::min<size_t>(otherEventTimes.size(), NET_REPLAY_RECORD_SEGMENT_MAX);

        for (INT32 i = 0; i < stInfo.nVideoCount; ++i)
        {
            stInfo.astVideoTimes[i] = videoTimes[(size_t)i];
        }
        for (INT32 i = 0; i < stInfo.nPersonEventCount; ++i)
        {
            stInfo.astPersonEventTimes[i] = personEventTimes[(size_t)i];
        }
        for (INT32 i = 0; i < stInfo.nVehicleEventCount; ++i)
        {
            stInfo.astVehicleEventTimes[i] = vehicleEventTimes[(size_t)i];
        }
        for (INT32 i = 0; i < stInfo.nOtherEventCount; ++i)
        {
            stInfo.astOtherEventTimes[i] = otherEventTimes[(size_t)i];
        }
    }
    else
    {
        if (stInfo.nVideoCount < 0)
        {
            stInfo.nVideoCount = 0;
        }
        if (stInfo.nPersonEventCount < 0)
        {
            stInfo.nPersonEventCount = 0;
        }
        if (stInfo.nVehicleEventCount < 0)
        {
            stInfo.nVehicleEventCount = 0;
        }
        if (stInfo.nOtherEventCount < 0)
        {
            stInfo.nOtherEventCount = 0;
        }

        stInfo.nVideoCount = std::min<INT32>(stInfo.nVideoCount, NET_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nPersonEventCount = std::min<INT32>(stInfo.nPersonEventCount, NET_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nVehicleEventCount = std::min<INT32>(stInfo.nVehicleEventCount, NET_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nOtherEventCount = std::min<INT32>(stInfo.nOtherEventCount, NET_REPLAY_RECORD_SEGMENT_MAX);

        std::vector<NET_ReplayRecordTime_S> videoTimes;
        std::vector<NET_ReplayRecordTime_S> personEventTimes;
        std::vector<NET_ReplayRecordTime_S> vehicleEventTimes;
        std::vector<NET_ReplayRecordTime_S> otherEventTimes;

        for (INT32 i = 0; i < stInfo.nVideoCount; ++i)
        {
            videoTimes.push_back(stInfo.astVideoTimes[i]);
        }
        for (INT32 i = 0; i < stInfo.nPersonEventCount; ++i)
        {
            personEventTimes.push_back(stInfo.astPersonEventTimes[i]);
        }
        for (INT32 i = 0; i < stInfo.nVehicleEventCount; ++i)
        {
            vehicleEventTimes.push_back(stInfo.astVehicleEventTimes[i]);
        }
        for (INT32 i = 0; i < stInfo.nOtherEventCount; ++i)
        {
            otherEventTimes.push_back(stInfo.astOtherEventTimes[i]);
        }

        convert.structure(pRootJson, "VideoTimes", videoTimes);
        convert.structure(pRootJson, "PersonEventTimes", personEventTimes);
        convert.structure(pRootJson, "VehicleEventTimes", vehicleEventTimes);
        convert.structure(pRootJson, "OtherEventTimes", otherEventTimes);
    }
}


void deal(Json::Object* pRootJson, NET_RtspUrlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.uChannel);
    convert.field(pRootJson, "StreamIndex", stInfo.uStreamIndex);
    convert.field(pRootJson, "RtspUrl", stInfo.szRtspUrl);
}


void deal(Json::Object* pRootJson, NET_RecordFrameStreamCond_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "uSize", stInfo.uSize);
    convert.field(pRootJson, "channel", stInfo.uChannel);
    convert.field(pRootJson, "startTime", stInfo.szStartTime);
    convert.field(pRootJson, "endTime", stInfo.szEndTime);
    convert.field(pRootJson, "streamIndex", stInfo.uStreamIndex);
    convert.field(pRootJson, "mediaType", stInfo.uMediaType);
    convert.field(pRootJson, "codecType", stInfo.uCodecType);
    convert.field(pRootJson, "tcpPort", stInfo.uTcpPort);
}


void deal(Json::Object* pRootJson, NET_RecordFrameStreamInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "uSize", stInfo.uSize);
    convert.field(pRootJson, "streamId", stInfo.szStreamId);
    convert.field(pRootJson, "channel", stInfo.uChannel);
    convert.field(pRootJson, "tcpPort", stInfo.uTcpPort);
    convert.field(pRootJson, "mediaType", stInfo.uMediaType);
    convert.field(pRootJson, "codecType", stInfo.uCodecType);
    convert.field(pRootJson, "width", stInfo.uWidth);
    convert.field(pRootJson, "height", stInfo.uHeight);
}


void deal(Json::Object* pRootJson, NET_RecordFrameStopInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "uSize", stInfo.uSize);
    convert.field(pRootJson, "streamId", stInfo.szStreamId);
}


void deal(Json::Object* pRootJson, NET_ChannelInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    int nChannel = (int)stInfo.uChannel;
    convert.field(pRootJson, "Channel", nChannel);
    stInfo.uChannel = (UINT32)nChannel;

    int nEnable = (int)stInfo.byEnable;
    convert.field(pRootJson, "Enable", nEnable);
    stInfo.byEnable = (BYTE)nEnable;

    int nOnline = (int)stInfo.byOnline;
    convert.field(pRootJson, "Online", nOnline);
    stInfo.byOnline = (BYTE)nOnline;

    int nStreamType = (int)stInfo.byStreamType;
    convert.field(pRootJson, "StreamType", nStreamType);
    stInfo.byStreamType = (BYTE)nStreamType;

    int nHasRecord = (int)stInfo.byHasRecord;
    convert.field(pRootJson, "HasRecord", nHasRecord);
    stInfo.byHasRecord = (BYTE)nHasRecord;
    convert.field(pRootJson, "RecordStatus", stInfo.nRecordStatus);

    convert.field(pRootJson, "DevState", stInfo.nDevState);
    convert.field(pRootJson, "AppProto", stInfo.nAppProto);
    convert.field(pRootJson, "TransProto", stInfo.nTransProto);
    convert.field(pRootJson, "MfrsType", stInfo.nMfrsType);
    convert.field(pRootJson, "CtrlPort", stInfo.nCtrlPort);
    convert.field_array(pRootJson, "Reserved", stInfo.nReserved, 3, 3);

    convert.field(pRootJson, "ChannelName", stInfo.szChannelName);
    convert.field(pRootJson, "DevName", stInfo.szDevName);
    convert.field(pRootJson, "DevType", stInfo.szDevType);
    convert.field(pRootJson, "SerialNum", stInfo.szSerialNum);
    convert.field(pRootJson, "FirmwareVersion", stInfo.szFirmwareVersion);
    convert.field(pRootJson, "DeviceIP", stInfo.szDeviceIP);
    convert.field(pRootJson, "Mac", stInfo.szMac);
    convert.field(pRootJson, "SubnetMask", stInfo.szSubnetMask);
    convert.field(pRootJson, "MfrsName", stInfo.szMfrsName);
    convert.field(pRootJson, "AppProtoName", stInfo.szAppProtoName);
    convert.field(pRootJson, "OnvifDeviceUrl", stInfo.szOnvifDeviceUrl);
    convert.field(pRootJson, "PreviewMainUrl", stInfo.szPreviewMainUrl);
    convert.field(pRootJson, "PreviewSubUrl", stInfo.szPreviewSubUrl);
    convert.field(pRootJson, "RtspMainUrl", stInfo.szRtspMainUrl);
    convert.field(pRootJson, "RtspSubUrl", stInfo.szRtspSubUrl);

    if (bOutStruct)
    {
        if (stInfo.szPreviewMainUrl[0] == '\0' && stInfo.szRtspMainUrl[0] != '\0')
        {
            std::strncpy(stInfo.szPreviewMainUrl, stInfo.szRtspMainUrl, sizeof(stInfo.szPreviewMainUrl) - 1);
            stInfo.szPreviewMainUrl[sizeof(stInfo.szPreviewMainUrl) - 1] = '\0';
        }
        if (stInfo.szPreviewSubUrl[0] == '\0' && stInfo.szRtspSubUrl[0] != '\0')
        {
            std::strncpy(stInfo.szPreviewSubUrl, stInfo.szRtspSubUrl, sizeof(stInfo.szPreviewSubUrl) - 1);
            stInfo.szPreviewSubUrl[sizeof(stInfo.szPreviewSubUrl) - 1] = '\0';
        }
    }
}


void deal(Json::Object* pRootJson, NET_ChannelList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    int nChannelCount = (int)stInfo.uChannelCount;
    convert.field(pRootJson, "ChannelCount", nChannelCount);
    stInfo.uChannelCount = (UINT32)nChannelCount;

    if (bOutStruct)
    {
        std::vector<NET_ChannelInfo_S> channels;
        convert.structure(pRootJson, "Channels", channels);
        stInfo.uChannelCount = (UINT32)std::min<size_t>(channels.size(), NET_MAX_CHANNEL_NUM);
        for (UINT32 i = 0; i < stInfo.uChannelCount; ++i)
        {
            stInfo.stChannels[i] = channels[i];
        }
    }
    else
    {
        std::vector<NET_ChannelInfo_S> channels;
        const UINT32 count = std::min<UINT32>(stInfo.uChannelCount, NET_MAX_CHANNEL_NUM);
        stInfo.uChannelCount = count;
        for (UINT32 i = 0; i < count; ++i)
        {
            channels.push_back(stInfo.stChannels[i]);
        }
        convert.structure(pRootJson, "Channels", channels);
    }
}

}
