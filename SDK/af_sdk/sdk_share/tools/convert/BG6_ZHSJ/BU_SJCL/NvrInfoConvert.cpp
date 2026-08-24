/*
 * @FilePath     : sdk_new/sdk_share/tools/convert/BG6_ZHSJ/BU_SJCL/NvrInfoConvert.cpp
 * @Author       : ITC
 * @Date         : 2026-08-21
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-21
 * @Description  : NVR 侧杂项转换
 *                 收口 抓拍/人脸/对讲/人流/AI 分析配置等 NVR 侧非纯报警结构体。
 */

#include "NvrInfoConvert.h"
#include "AlarmInfoConvert.h"   
#include "SDKConvert.h"

#include <algorithm>
#include <vector>
#include <cstring>
#include <string>

namespace SDKConvert
{

static UINT32 clamp_time_count(UINT32 count)
{
    return (count > NET_PLAN_TIME_SECTION_NUM_ADAY) ? NET_PLAN_TIME_SECTION_NUM_ADAY : count;
}

static void JsonToFloatArray(Json::Object* pRootJson, const char* key, FLOAT* values, int maxCount)
{
    Json::Object* pArray = Json::get(pRootJson, key);
    if (!pArray)
    {
        return;
    }

    int nSize = Json::Array::size(pArray);
    for (int i = 0; i < nSize && i < maxCount; i++)
    {
        Json::Object* pItem = Json::Array::get(pArray, i);
        if (pItem)
        {
            double dVal = 0.0;
            Json::Value::get(pItem, dVal);
            values[i] = (FLOAT)dVal;
        }
    }
}

static void FloatArrayToJson(Json::Object* pRootJson, const char* key, const FLOAT* values, int count, int maxCount)
{
    Json::Object* pArray = Json::Array::init();
    if (count > maxCount)
    {
        count = maxCount;
    }
    for (int i = 0; i < count; i++)
    {
        Json::Array::add(pArray, static_cast<float>(values[i]));
    }
    Json::add(pRootJson, key, pArray);
}


/* ===================== 告警抓图相关 ============================== */

void deal(Json::Object *pRootJson, NET_CaptureTime_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartTime", stInfo.nStartTime);
    convert.field(pRootJson, "EndTime", stInfo.nEndTime);
}


void deal(Json::Object *pRootJson, NET_CaptureDaySchedule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "DayOfWeek", stInfo.nDayOfWeek);
    convert.field(pRootJson, "TimeCount", stInfo.udwTimeCount);

    if (bOutStruct)
    {
        UINT32 i = 0;
        UINT32 nTimeCount = clamp_time_count(stInfo.udwTimeCount);
        stInfo.udwTimeCount = nTimeCount;
        Json::Object *pTimes = Json::get(pRootJson, "Times");
        if (!pTimes)
            return;

        for (i = 0; i < nTimeCount; ++i)
        {
            std::string key = std::to_string(i);
            Json::Object *pTime = Json::get(pTimes, key.c_str());
            if (pTime)
                deal(pTime, stInfo.astTimes[i], true);
        }
    }
    else
    {
        UINT32 i = 0;
        UINT32 nTimeCount = clamp_time_count(stInfo.udwTimeCount);
        Json::Object *pTimes = Json::init();
        if (!pTimes)
            return;

        for (i = 0; i < nTimeCount; ++i)
        {
            std::string key = std::to_string(i);
            Json::Object *pTime = Json::init();
            if (!pTime)
                continue;
            deal(pTime, stInfo.astTimes[i], false);
            Json::add(pTimes, key.c_str(), pTime);
        }
        Json::add(pRootJson, "Times", pTimes);
    }
}


void deal(Json::Object *pRootJson, NET_CapturePlanInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    if (bOutStruct)
    {
        UINT32 i = 0;
        std::memset(&stInfo, 0, sizeof(stInfo));
        Json::Object *pDays = Json::get(pRootJson, "DaySchedules");
        if (!pDays)
            return;

        for (i = 0; i < NET_PLAN_DAY_NUM_AWEEK; ++i)
        {
            std::string key = std::to_string(i);
            Json::Object *pDay = Json::get(pDays, key.c_str());
            if (pDay)
                SDKConvert::deal(pDay, stInfo.astDaySchedules[i], true);
        }
    }
    else
    {
        UINT32 i = 0;
        Json::Object *pDays = Json::init();
        if (!pDays)
            return;

        for (i = 0; i < NET_PLAN_DAY_NUM_AWEEK; ++i)
        {
            std::string key = std::to_string(i);
            Json::Object *pDay = Json::init();
            if (!pDay)
                continue;
            deal(pDay, stInfo.astDaySchedules[i], false);
            Json::add(pDays, key.c_str(), pDay);
        }
        Json::add(pRootJson, "DaySchedules", pDays);
    }
}


void deal(Json::Object *pRootJson, NET_CaptureConfig_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
    std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "PictureFormat", stInfo.enPictureFormat);
    convert.field(pRootJson, "Width", stInfo.nWidth);
    convert.field(pRootJson, "Height", stInfo.nHeight);
    convert.field(pRootJson, "ImageQuality", stInfo.enImageQuality);
    convert.field(pRootJson, "Interval", stInfo.unInterval);
    convert.field(pRootJson, "TimeUnit", stInfo.enTimeUnit);
    convert.field(pRootJson, "Number", stInfo.unNumber);
}


void deal(Json::Object *pRootJson, NET_CaptureParamInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    if (bOutStruct)
    {
        std::memset(&stInfo, 0, sizeof(stInfo));
        Json::Object *pTimingCfg = Json::get(pRootJson, "CaptureTimingConfig");
        Json::Object *pEventCfg = Json::get(pRootJson, "CaptureEventConfig");
        if (pTimingCfg)
            deal(pTimingCfg, stInfo.stCaptureTimingConfig, true);
        if (pEventCfg)
            deal(pEventCfg, stInfo.stCaptureEventConfig, true);
    }
    else
    {
        Json::Object *pTimingCfg = Json::init();
        Json::Object *pEventCfg = Json::init();
        if (pTimingCfg)
        {
            deal(pTimingCfg, stInfo.stCaptureTimingConfig, false);
            Json::add(pRootJson, "CaptureTimingConfig", pTimingCfg);
        }
        if (pEventCfg)
        {
            deal(pEventCfg, stInfo.stCaptureEventConfig, false);
            Json::add(pRootJson, "CaptureEventConfig", pEventCfg);
        }
    }
}


/* ===================== 对讲相关 ============================== */

void deal(Json::Object* pRootJson, NET_TalkbackStateInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sdp", stInfo.szSdp);
    convert.field(pRootJson, "Url", stInfo.szUrl);
    convert.field(pRootJson, "LocalIp", stInfo.szLocalIP);
}


void deal(Json::Object* pRootJson, NET_TalkbackStreamInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Host", stInfo.szHost);
    convert.field(pRootJson, "Port", stInfo.nPort);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "UserId", stInfo.nUserID);
    convert.field(pRootJson, "IsMainStream", stInfo.bMainStream);
    convert.field(pRootJson, "Protocol", stInfo.szProtocol);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "Filename", stInfo.szFileName);
}


void deal(Json::Object* pRootJson, NET_ReplayTalkbackInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "NvrIp", stInfo.szNvrIp);
    convert.field(pRootJson, "RemoteIp", stInfo.szRemoteIp);
    convert.structure(pRootJson, "IpcInfo", stInfo.stIPCInfo);
}


void deal(Json::Object* pRootJson, NET_VoiceComAudioCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Format", stInfo.enFormat);
    convert.field(pRootJson, "SampleRate", stInfo.uSampleRate);
    convert.field(pRootJson, "BitDepth", stInfo.uBitDepth);
    convert.field(pRootJson, "Channels", stInfo.uChannels);
    convert.field(pRootJson, "FrameIntervalMs", stInfo.uFrameIntervalMs);
    convert.field(pRootJson, "FrameBytes", stInfo.uFrameBytes);
    convert.field(pRootJson, "BitRate", stInfo.uBitRate);
    convert.field(pRootJson, "LittleEndian", stInfo.bLittleEndian);
}


/* ===================== 人脸相关 ============================== */

void deal(Json::Object* pRootJson, NET_FaceCaptureRegion_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }
}


void deal(Json::Object* pRootJson, NET_FaceCaptureRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Region", stInfo.stRegion);

    convert.field(pRootJson, "ShieldRegionCount", stInfo.uShieldRegionCount);
    if (bOutStruct)
    {
        Json::Object* pShieldRegions = Json::get(pRootJson, "ShieldRegion");
        if (pShieldRegions)
        {
            int count = stInfo.uShieldRegionCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRegion = Json::get(pShieldRegions, key);
                if (pRegion)
                {
                    deal(pRegion, stInfo.astShieldRegion[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pShieldRegions = Json::init();
        int count = stInfo.uShieldRegionCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRegion = Json::init();
            deal(pRegion, stInfo.astShieldRegion[i], bOutStruct);
            Json::add(pShieldRegions, std::to_string(i).c_str(), pRegion);
        }
        Json::add(pRootJson, "ShieldRegion", pShieldRegions);
    }

    convert.field(pRootJson, "MinIpdRectLeft", stInfo.nMinIpdRectLeft);
    convert.field(pRootJson, "MinIpdRectTop", stInfo.nMinIpdRectTop);
    convert.field(pRootJson, "MinIpdRectRight", stInfo.nMinIpdRectRight);
    convert.field(pRootJson, "MinIpdRectBottom", stInfo.nMinIpdRectBottom);
    convert.field(pRootJson, "MinWidth", stInfo.nMinWidth);
    convert.field(pRootJson, "MinHeight", stInfo.nMinHeight);
    convert.field(pRootJson, "MaxWidth", stInfo.nMaxWidth);
    convert.field(pRootJson, "MaxHeight", stInfo.nMaxHeight);
    convert.field(pRootJson, "Interval", stInfo.nInterval);
}


void deal(Json::Object* pRootJson, NET_FaceCaptureInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}

/* 人脸抓拍图片叠加配置需支持 SDK JSON 与 IPC 回调之间的双向转换。 */
void deal(Json::Object* pRootJson, NET_FaceCaptureOverlayInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "DeviceID", stInfo.nDeviceID);
    convert.field(pRootJson, "MonitoryPointInfo", stInfo.strMonitoryPointInfo);
    convert.field(pRootJson, "OverlayDeviceID", stInfo.bOverlayDeviceID);
    convert.field(pRootJson, "OverlayCaptureTime", stInfo.bOverlayCaptureTime);
    convert.field(pRootJson, "OverlayMonitoryPointInfo", stInfo.bOverlayMonitoryPointInfo);

    INT32 nFontColor = static_cast<INT32>(stInfo.enFontColor);
    convert.field(pRootJson, "FontColor", nFontColor);
    if (bOutStruct)
    {
        stInfo.enFontColor = static_cast<NET_OSD_COLOR_E>(nFontColor);
    }
    convert.field(pRootJson, "CustomFontColor", stInfo.strFontColor);
}


void deal(Json::Object* pRootJson, NET_FaceCompareInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageSuccessMode", stInfo.stLinkageListSuccess);
    convert.structure(pRootJson, "LinkageFailMode", stInfo.stLinkageListFail);
}


void deal(Json::Object* pRootJson, NET_FaceLibInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "LibId", stInfo.szFaceLibName);
    convert.field(pRootJson, "TotalFace", stInfo.nTotalFace);
    convert.field(pRootJson, "NormalNum", stInfo.nNormalNum);
    convert.field(pRootJson, "AbnormalNum", stInfo.nAbnormalNum);
}


void deal(Json::Object* pRootJson, NET_FaceLibList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "TargetLibCount", stInfo.nTargetLibCount);
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "TargetLibInfos");
        int nSize = Json::Array::size(pArray);
        if (nSize > NET_FACE_LIB_MAX_NUM)
        {
            nSize = NET_FACE_LIB_MAX_NUM;
        }
        stInfo.nTargetLibCount = nSize;
        for (int i = 0; i < nSize; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astTargetLibInfos[i], bOutStruct);
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int nCount = stInfo.nTargetLibCount;
        if (nCount > NET_FACE_LIB_MAX_NUM)
        {
            nCount = NET_FACE_LIB_MAX_NUM;
        }
        for (int i = 0; i < nCount; ++i)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astTargetLibInfos[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "TargetLibInfos", pArray);
    }
}


void deal(Json::Object* pRootJson, NET_FaceIdInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "IdCount", stInfo.nIdCount);
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "Ids");
        int nSize = Json::Array::size(pArray);
        if (nSize > NET_FACE_ID_MAX_NUM)
        {
            nSize = NET_FACE_ID_MAX_NUM;
        }
        stInfo.nIdCount = nSize;
        for (int i = 0; i < nSize; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                Json::Value::get(pItem, stInfo.anIds[i]);
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int nCount = stInfo.nIdCount;
        if (nCount > NET_FACE_ID_MAX_NUM)
        {
            nCount = NET_FACE_ID_MAX_NUM;
        }
        if (nCount < 0)
        {
            nCount = 0;
        }
        for (int i = 0; i < nCount; ++i)
        {
            Json::Array::add(pArray, stInfo.anIds[i]);
        }
        Json::add(pRootJson, "Ids", pArray);
    }
}


void deal(Json::Object* pRootJson, NET_FaceInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Id", stInfo.nId);
    convert.field(pRootJson, "LibId", stInfo.szFaceLibName);
    convert.field(pRootJson, "Name", stInfo.szName);
    convert.field(pRootJson, "PhoneNum", stInfo.szPhoneNum);
    convert.field(pRootJson, "PicPath", stInfo.szPicPath);
    convert.field(pRootJson, "BinPath", stInfo.szBinPath);
    convert.field(pRootJson, "PicType", stInfo.szPicType);
    convert.field(pRootJson, "PicSize", stInfo.nPicSize);
    convert.field(pRootJson, "PicDate", stInfo.szPicDate);
    convert.field(pRootJson, "ModelState", stInfo.nModelState);
    convert.field(pRootJson, "RatingLevel", stInfo.nRatingLevel);
}


void deal(Json::Object* pRootJson, NET_FaceInfoList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "FaceInfoCount", stInfo.nFaceInfoCount);
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "FaceInfos");
        int nSize = Json::Array::size(pArray);
        if (nSize > NET_FACE_INFO_MAX_NUM)
        {
            nSize = NET_FACE_INFO_MAX_NUM;
        }
        stInfo.nFaceInfoCount = nSize;
        for (int i = 0; i < nSize; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astFaceInfos[i], bOutStruct);
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int nCount = stInfo.nFaceInfoCount;
        if (nCount > NET_FACE_INFO_MAX_NUM)
        {
            nCount = NET_FACE_INFO_MAX_NUM;
        }
        for (int i = 0; i < nCount; ++i)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astFaceInfos[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "FaceInfos", pArray);
    }
}


/* ===================== 人流统计规则线 ============================== */
void deal(Json::Object* pRootJson, NET_PeopleFlowRuleLine_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartPointX", stInfo.fStartPointX);
    convert.field(pRootJson, "StartPointY", stInfo.fStartPointY);
    convert.field(pRootJson, "EndPointX", stInfo.fEndPointX);
    convert.field(pRootJson, "EndPointY", stInfo.fEndPointY);
    convert.field(pRootJson, "Direction", stInfo.nDirection);
}


/* ===================== 单档人数报警配置 ============================== */
void deal(Json::Object* pRootJson, NET_PeopleAlarmRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Threshold", stInfo.nThreshold);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


/* ===================== 三级人数报警配置 ============================== */
void deal(Json::Object* pRootJson, NET_PeopleAlarmConfig_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.structure(pRootJson, "Normal", stInfo.stNormal);
    convert.structure(pRootJson, "Medium", stInfo.stMedium);
    convert.structure(pRootJson, "Severe", stInfo.stSevere);
}


/* ===================== 定时清零配置 ============================== */
void deal(Json::Object* pRootJson, NET_StatisticsResetConfig_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Hour", stInfo.nHour);
    convert.field(pRootJson, "Minute", stInfo.nMinute);
}


/* ===================== 人流统计配置 ============================== */
void deal(Json::Object* pRootJson, NET_PeopleFlowStatisticsCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "RuleLine", stInfo.stRuleLine);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }

    convert.field(pRootJson, "ReportInterval", stInfo.nReportInterval);
    convert.field(pRootJson, "StatisticsType", stInfo.enStatisticsType);
    convert.structure(pRootJson, "TimedReset", stInfo.stTimedReset);
    convert.structure(pRootJson, "StayAlarm", stInfo.stStayAlarm);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
}


/* ===================== 人员密度检测配置 ============================== */
void deal(Json::Object* pRootJson, NET_PeopleDensityDetectionCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    if (bOutStruct)
    {
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, 32);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, 32);
    }
    else
    {
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, 32);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, 32);
    }

    convert.field(pRootJson, "ReportInterval", stInfo.nReportInterval);
    convert.structure(pRootJson, "DensityAlarm", stInfo.stDensityAlarm);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
}


void deal(Json::Object* pRootJson, NET_ManholeCoverAbnormalCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_SleepOnDutyCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_ElectricVehicleInElevatorCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_PersonFallDownCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_ConstructionOccupyRoadCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_CongestionCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_LicensePlateRecognitionCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_HighAltitudeSeatbeltCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_SafetyHelmetCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_PersonFallCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_PhoneUsageCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_SmokingCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_OpenFlameCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_BareSoilCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_HoleProtectionBarCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_ReflectiveClothingCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_PetRecognitionInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "DynamicAnalysisEnable", stInfo.bDynamicAnalysisEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_ClimbFenceInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);
    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_DimissionInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);
    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_IllegalLaneInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);
    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_RetrogradeInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);
    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_NonmotorVehicleIntrusionInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);
    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_OccupationEmergencyInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);
    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_PedestrianIntrusionInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);
    if (bOutStruct)
    {
        Json::Object* pRules = Json::get(pRootJson, "Rules");
        if (pRules)
        {
            int count = stInfo.uRuleCount;
            if (count > 4) count = 4;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.stRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 4) count = 4;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.stRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_SmokeFireCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


void deal(Json::Object* pRootJson, NET_RoadPondingCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}

}
