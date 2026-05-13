/**
 * @FilePath     : replay_convert.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-16 17:00:09
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-16 17:03:18
 * @Description  : 预览定义数据的转换
 */

#include "replay_convert.h"

#include "common_convert.h"
#include "layout_convert.h"
#include "record_convert.h"
// #include "ipc_convert.h"

#include "convert.h"

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::Info_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Layout", (int &)stInfo.enLayout);

    convert.structure(pRootJson, stInfo.vChnInfos);
    convert.structure(pRootJson, stInfo.stPageInfo);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::Layout_S &stLayout, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Layout", (int &)stLayout.enLayout);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::Patrol_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Interval", stInfo.nInterval);
    convert.field(pRootJson, "EnablePatrol", stInfo.bEnablePatrol);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::Playback_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "State", (int &)stInfo.enState);
    convert.field(pRootJson, "CurTime", stInfo.nCurTime);
    convert.field(pRootJson, "TotalTime", stInfo.nTotalTime);

    convert.structure(pRootJson, stInfo.stItem);
}

void Convert::deal(Json::Object *pRootJson, Replay::SaveImage_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Path", stInfo.strPath);
    convert.field(pRootJson, "FileName", stInfo.strFileName);

    convert.structure(pRootJson, stInfo.stItem);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::DigitalZoom_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Type", (int &)stInfo.enType);
    convert.field(pRootJson, "MoveX", stInfo.nMoveX);
    convert.field(pRootJson, "MoveY", stInfo.nMoveY);
    convert.field(pRootJson, "WheelDelta", stInfo.nWheelDelta);

    convert.structure(pRootJson, stInfo.stItem);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::Voice_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Volume", stInfo.nVolume);
    convert.field(pRootJson, "Enable", stInfo.bEnable);

    convert.structure(pRootJson, stInfo.stItem);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::PlaybackInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Mode", (int &)stInfo.enMode);

    convert.structure(pRootJson, stInfo.stItem);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::StreamInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "MainStream", stInfo.bMainStream);

    convert.structure(pRootJson, stInfo.stItem);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::AiShowInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Show", stInfo.bShow);

    convert.structure(pRootJson, stInfo.stItem);
}

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::AdaptiveResInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);

    convert.structure(pRootJson, stInfo.stItem);
}
/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::IpcList_S &stIpcList, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", stIpcList.nType);
    // convert.structure(pRootJson, "IpcInfos", stIpcList.ipcInfos);
}

void Convert::deal(Json::Object *pRootJson, Replay::RecordTime_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    if (!bOutStruct)
    {
        if (!stInfo.videoTimes.empty())
        {
            convert.structure(pRootJson, "VideoTimes", stInfo.videoTimes);
        }
        if (!stInfo.EventTimes.empty())
        {
            convert.structure(pRootJson, "EventTimes", stInfo.EventTimes);
        }
        convert.field(pRootJson, "Type", stInfo.nEventType);
    }
    else
    {
        convert.structure(pRootJson, "VideoTimes", stInfo.videoTimes); 
        convert.structure(pRootJson, "EventTimes", stInfo.EventTimes);
        convert.field(pRootJson, "Type", stInfo.nEventType);
    }
}


/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::ChnInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Date", stInfo.date);
    convert.field(pRootJson, "Filename", stInfo.filename);
    convert.field(pRootJson, "Type", stInfo.nType);
    convert.structure(pRootJson, stInfo.stItem);
    convert.structure(pRootJson, "Rect", stInfo.stRect);
    convert.structure(pRootJson, stInfo.recordTime);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Replay::ChnInfo_S> &chnInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "ChnInfos", chnInfos);
}
/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Replay::LayoutInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Layout", (int &)stInfo.enLayout);
    convert.field(pRootJson, "Type", stInfo.nType);
    convert.structure(pRootJson, "ChnInfos", stInfo.chnInfos);
    convert.field(pRootJson, "Date", stInfo.date);
    convert.structure(pRootJson, stInfo.recordTime);
    convert.field(pRootJson, "SmartVideoSummary", stInfo.bSmartVideoSummary);
    convert.field(pRootJson, "StartTime", stInfo.strStartTime);
    convert.field(pRootJson, "EndTime", stInfo.strEndTime);
    convert.field(pRootJson, "SummaryType", (int &)stInfo.enSummaryType);
}

void Convert::deal(Json::Object *pRootJson, Replay::PlayInfo_S &stPlayInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stPlayInfo.stItem);
    convert.field(pRootJson, "IsPlay", stPlayInfo.bPlay);
}

void Convert::deal(Json::Object *pRootJson, Replay::SeekInfo_S &stSeekInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stSeekInfo.stItem);
    convert.field(pRootJson, "Seek", stSeekInfo.nSeek);
}
void Convert::deal(Json::Object *pRootJson, Replay::SpeedInfo_S &stSpeedInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stSpeedInfo.stItem);
    convert.field(pRootJson, "Speed", stSpeedInfo.nSpeed);
}
void Convert::deal(Json::Object *pRootJson, Replay::MediaInfo_S &stMediaInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stMediaInfo.nChnId);
    convert.field(pRootJson, "IsPlay", stMediaInfo.bPlay);
    convert.field(pRootJson, "PlayTime", stMediaInfo.nPlayTime);
    convert.field(pRootJson, "Speed", stMediaInfo.nSpeed);
    convert.field(pRootJson, "StartTime", stMediaInfo.nStartTime);
    convert.field(pRootJson, "TotalTime", stMediaInfo.nTotalTime);
}
void Convert::deal(Json::Object *pRootJson, std::vector<Replay::MediaInfo_S> &mediaInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "MediaInfos", mediaInfos);
}

void Convert::deal(Json::Object *pRootJson, Replay::LockInfo_S &stLockInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stLockInfo.nChnId);
    convert.field(pRootJson, "IsLock", stLockInfo.bLock);
    convert.field(pRootJson, "Date", stLockInfo.date);
    convert.field(pRootJson, "Timestamp", stLockInfo.nTimestamp);
    convert.field(pRootJson, "StartTime", stLockInfo.startTime);
    convert.field(pRootJson, "Filename", stLockInfo.filename);
}

void Convert::deal(Json::Object *pRootJson, Replay::FileInfo_S &stFileInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Filename", stFileInfo.filename);
    convert.structure(pRootJson, stFileInfo.stItem);
    convert.structure(pRootJson, stFileInfo.stRect);
}

void Convert::deal(Json::Object* pRootJson, Replay::Stream::Info_S &stStreamInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Host", stStreamInfo.host);
    convert.field(pRootJson, "Port", stStreamInfo.nPort);
    convert.field(pRootJson, "ChnId", stStreamInfo.nChnId);
    convert.field(pRootJson, "UserId", stStreamInfo.nUserId);
    convert.field(pRootJson, "IsMainStream", stStreamInfo.bMainStream);
    convert.field(pRootJson, "Protocol", stStreamInfo.protocol);
    convert.field(pRootJson, "StartTime", stStreamInfo.startTime);
    convert.field(pRootJson, "EndTime", stStreamInfo.endTime);
    convert.field(pRootJson, "Filename", stStreamInfo.filename);
    convert.field(pRootJson, "Filenames", stStreamInfo.filenames);
}

void Convert::deal(Json::Object *pRootJson, Replay::Stream::ReplayRtpInfo_S &stRtpInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "RemoteIp", stRtpInfo.remoteIp);
    convert.field(pRootJson, "NvrIp", stRtpInfo.nvrIp);
    convert.structure(pRootJson, "IpcInfo" ,stRtpInfo.ipcInfo);
}

void Convert::deal(Json::Object* pRootJson, Replay::Stream::Ctrl_S &stStreamCtrl, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stStreamCtrl.nChnId);
    convert.field(pRootJson, "Speed", stStreamCtrl.fSpeed);
    convert.field(pRootJson, "Seek", stStreamCtrl.nSeek);
    convert.field(pRootJson, "Play", stStreamCtrl.bPlay);
    convert.field(pRootJson, "Pause", stStreamCtrl.bPause);
    convert.field(pRootJson, "Stop", stStreamCtrl.bStop);
    
}
void Convert::deal(Json::Object* pRootJson, Replay::Stream::MediaInfo_S &stStreamMediaInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stStreamMediaInfo.nChnId);
    convert.field(pRootJson, "PlayStatus", stStreamMediaInfo.nPlayStatus);
    convert.field(pRootJson, "PlayTime", stStreamMediaInfo.nPlayTime);
    convert.field(pRootJson, "Speed", stStreamMediaInfo.fSpeed);
    convert.field(pRootJson, "StartTime", stStreamMediaInfo.nStartTime);
    convert.field(pRootJson, "TotalTime", stStreamMediaInfo.nTotalTime);
}