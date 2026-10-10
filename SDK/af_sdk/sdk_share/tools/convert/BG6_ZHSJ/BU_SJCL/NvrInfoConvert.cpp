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
#include "Base64Util.h"
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "LibId", stInfo.szFaceLibName);          /* 原名(添加/删除/修改共用, Add/Del 使用) */
    convert.field(pRootJson, "LibId_new", stInfo.szFaceLibNameNew);   /* 修改目标库(重命名)新名 */
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
    convert.field(pRootJson, "LibId", stInfo.szFaceLibName);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    Json::Object *pRule = Json::get(pRootJson, "Rule");
    if (bOutStruct)
    {
        if (pRule)
        {
            convert.field(pRule, "TimeThreshold", stInfo.nTimeThreshold);
            convert.field(pRule, "PointCount", stInfo.uPointCount);
            JsonToFloatArray(pRule, "PointX", stInfo.afPointX, NET_AI_SIMPLE_REGION_POINT_MAX_NUM);
            JsonToFloatArray(pRule, "PointY", stInfo.afPointY, NET_AI_SIMPLE_REGION_POINT_MAX_NUM);
            convert.structure(pRule, stInfo.stRule);
        }
    }
    else
    {
        pRule = Json::init();
        convert.field(pRule, "TimeThreshold", stInfo.nTimeThreshold);
        convert.field(pRule, "PointCount", stInfo.uPointCount);
        FloatArrayToJson(pRule, "PointX", stInfo.afPointX, stInfo.uPointCount, NET_AI_SIMPLE_REGION_POINT_MAX_NUM);
        FloatArrayToJson(pRule, "PointY", stInfo.afPointY, stInfo.uPointCount, NET_AI_SIMPLE_REGION_POINT_MAX_NUM);
        convert.structure(pRule, stInfo.stRule);
        Json::add(pRootJson, "Rule", pRule);
    }
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
    
    if (bOutStruct)
    {
        convert.field(pRootJson, "PointCount", stInfo.uPointCount);
        JsonToFloatArray(pRootJson, "PointX", stInfo.afPointX, NET_AI_SIMPLE_REGION_POINT_MAX_NUM);
        JsonToFloatArray(pRootJson, "PointY", stInfo.afPointY, NET_AI_SIMPLE_REGION_POINT_MAX_NUM);
    }
    else
    {
        convert.field(pRootJson, "PointCount", stInfo.uPointCount);
        FloatArrayToJson(pRootJson, "PointX", stInfo.afPointX, stInfo.uPointCount, NET_AI_SIMPLE_REGION_POINT_MAX_NUM);
        FloatArrayToJson(pRootJson, "PointY", stInfo.afPointY, stInfo.uPointCount, NET_AI_SIMPLE_REGION_POINT_MAX_NUM);
    }
}


void deal(Json::Object* pRootJson, NET_ConstructionOccupyRoadCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
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
    convert.field(pRootJson, "Channel", stInfo.uChannel);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}


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
    convert.field(pRootJson, "FontColor", stInfo.enFontColor);
    convert.field(pRootJson, "FontColorStr", stInfo.strFontColor);
}


/* ===== 设备状态/通道名/透明通道/串口 ===== */

void deal(Json::Object* pRootJson, NET_DeviceStatusInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "nCpuUsage", stInfo.nCpuUsage);
    convert.field(pRootJson, "nMemoryUsage", stInfo.nMemoryUsage);
    convert.field(pRootJson, "nDiskCount", stInfo.nDiskCount);
    convert.field(pRootJson, "nAbnormalDiskCount", stInfo.nAbnormalDiskCount);
    convert.field(pRootJson, "nChannelCount", stInfo.nChannelCount);
    convert.field(pRootJson, "nOnlineChannelCount", stInfo.nOnlineChannelCount);
    convert.field(pRootJson, "nNetworkCount", stInfo.nNetworkCount);
    
    /* nNetworkStatus 数组序列化 */
    if (bOutStruct)
    {
        std::vector<int> vec;
        Json::Object* pArr = Json::get(pRootJson, "nNetworkStatus");
        if (pArr) Json::Array::get(pArr, vec);
        for (int i = 0; i < 4; ++i)
            stInfo.nNetworkStatus[i] = (i < (int)vec.size()) ? vec[i] : 0;
    }
    else
    {
        Json::Object* pArr = Json::Array::init();
        for (int i = 0; i < stInfo.nNetworkCount && i < 4; ++i)
            Json::Array::add(pArr, stInfo.nNetworkStatus[i]);
        Json::add(pRootJson, "nNetworkStatus", pArr);
    }
    
    convert.field(pRootJson, "nRecordStatus", stInfo.nRecordStatus);
}

void deal(Json::Object* pRootJson, NET_ChannelNameInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "uChannel", stInfo.uChannel);
    convert.field(pRootJson, "szChannelName", stInfo.szChannelName);
}

void deal(Json::Object* pRootJson, NET_TransparentChannel_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "nChannelIndex", stInfo.nChannelIndex);
    convert.field(pRootJson, "nSerialPortIndex", stInfo.nSerialPortIndex);
    convert.field(pRootJson, "nBaudRate", stInfo.nBaudRate);
}

void deal(Json::Object* pRootJson, NET_TransparentData_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "nChannelIndex", stInfo.nChannelIndex);
    convert.field(pRootJson, "uDataLen", stInfo.uDataLen);
    /* byData 按字符串往返 */
    if (bOutStruct)
    {
        std::string strData;
        if (Json::get(pRootJson, "byData", strData))
        {
            size_t copyLen = (std::min)(strData.size(), sizeof(stInfo.byData));
            std::memcpy(stInfo.byData, strData.c_str(), copyLen);
            stInfo.uDataLen = static_cast<UINT32>(copyLen);
        }
    }
    else
    {
        UINT32 dataLen = (std::min)(stInfo.uDataLen, static_cast<UINT32>(sizeof(stInfo.byData)));
        if (dataLen > 0)
        {
            std::string strData(reinterpret_cast<const char*>(stInfo.byData), dataLen);
            Json::add(pRootJson, "byData", strData);
        }
    }
}

void deal(Json::Object* pRootJson, NET_SerialPortParam_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "nPortIndex", stInfo.nPortIndex);
    convert.field(pRootJson, "nBaudRate", stInfo.nBaudRate);
    convert.field(pRootJson, "nDataBits", stInfo.nDataBits);
    convert.field(pRootJson, "nStopBits", stInfo.nStopBits);
    convert.field(pRootJson, "nParity", stInfo.nParity);
    convert.field(pRootJson, "nFlowControl", stInfo.nFlowControl);
}

void deal(Json::Object* pRootJson, NET_SerialData_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "nPortIndex", stInfo.nPortIndex);
    convert.field(pRootJson, "uDataLen", stInfo.uDataLen);
    /* byData 按字符串往返 */
    if (bOutStruct)
    {
        std::string strData;
        if (Json::get(pRootJson, "byData", strData))
        {
            size_t copyLen = (std::min)(strData.size(), sizeof(stInfo.byData));
            std::memcpy(stInfo.byData, strData.c_str(), copyLen);
            stInfo.uDataLen = static_cast<UINT32>(copyLen);
        }
    }
    else
    {
        UINT32 dataLen = (std::min)(stInfo.uDataLen, static_cast<UINT32>(sizeof(stInfo.byData)));
        if (dataLen > 0)
        {
            std::string strData(reinterpret_cast<const char*>(stInfo.byData), dataLen);
            Json::add(pRootJson, "byData", strData);
        }
    }
}

/* ===================== 抓图 / 录像锁定 / 强制I帧（580~584） ============================== */

/* 抓图图片数据按 Base64 编码往返：固定 1MB 数组 + 实际长度字段。
 * 输出（结构→JSON）只编码实际长度范围内的字节，避免传输 1MB 空数据。
 * 输入（JSON→结构）按 Base64 解码后拷贝，长度截断到数组容量。 */
static void CapturePicBase64Field(Json::Object* pRootJson,
                                  const std::string& key,
                                  BYTE (&arr)[NET_PIC_DATA_MAX_LEN],
                                  UINT32& len,
                                  bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    if (bOutStruct)
    {
        len = 0;
        std::string b64;
        if (Json::get(pRootJson, key, b64) && !b64.empty())
        {
            std::vector<unsigned char> decoded;
            if (SDKConvert::Base64Decode(b64, decoded))
            {
                size_t copyLen = (std::min)(decoded.size(),
                                            static_cast<size_t>(NET_PIC_DATA_MAX_LEN));
                if (copyLen > 0)
                {
                    std::memcpy(arr, decoded.data(), copyLen);
                }
                len = static_cast<UINT32>(copyLen);
            }
        }
        return;
    }

    UINT32 imageLen = len;
    if (imageLen > static_cast<UINT32>(NET_PIC_DATA_MAX_LEN))
    {
        imageLen = static_cast<UINT32>(NET_PIC_DATA_MAX_LEN);
    }
    if (imageLen > 0)
    {
        std::string b64 = SDKConvert::Base64Encode(
            reinterpret_cast<const unsigned char*>(arr),
            static_cast<size_t>(imageLen));
        Json::add(pRootJson, key, b64);
    }
}

void deal(Json::Object* pRootJson, NET_CapturePictureInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.nChannel);
    convert.field(pRootJson, "StreamType", stInfo.nStreamType);
    convert.field(pRootJson, "PicFormat", stInfo.nPicFormat);
    convert.field(pRootJson, "OutputType", stInfo.nOutputType);
    convert.field(pRootJson, "ImageQuality", stInfo.nImageQuality);
    convert.field(pRootJson, "FilePath", stInfo.szFilePath);
    /* 图片数据最后处理：依赖 uPicLen，且只在内存输出模式下有效 */
    if (!bOutStruct && stInfo.nOutputType == NET_CAPTURE_OUTPUT_MEMORY)
    {
        CapturePicBase64Field(pRootJson, "PicDataBase64",
                              stInfo.abyPicData, stInfo.uPicLen, bOutStruct);
    }
    else if (bOutStruct)
    {
        CapturePicBase64Field(pRootJson, "PicDataBase64",
                              stInfo.abyPicData, stInfo.uPicLen, bOutStruct);
    }
    convert.field(pRootJson, "PicLen", stInfo.uPicLen);
}

void deal(Json::Object* pRootJson, NET_RecordLockInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.nChannel);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "FileName", stInfo.szFileName);
    convert.field(pRootJson, "LockStatus", stInfo.nLockStatus);
}

void deal(Json::Object* pRootJson, NET_ForceKeyFrameInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.nChannel);
    convert.field(pRootJson, "StreamType", stInfo.nStreamType);
    convert.field(pRootJson, "StreamId", stInfo.szStreamId);
    convert.field(pRootJson, "Result", stInfo.nResult);
    convert.field(pRootJson, "StreamUrl", stInfo.szStreamUrl);
}

}
