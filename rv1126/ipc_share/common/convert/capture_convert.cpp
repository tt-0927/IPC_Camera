/**
 * @FilePath     : capture_convert.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-15 16:19:25
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-15 20:18:19
 * @Description  : 抓图配置转换处理
 */

#include "capture_convert.h"
#include "common_convert.h"
#include "convert.h" /* 这个要放在 capture_convert.h 的后面 */

using namespace Capture_NS;

void Convert::deal(Json::Object* pRootJson, Capture_NS::CaptureTime_S &stCaptureTime, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
    
	Convert::CConvert convert(bOutStruct);
	// convert.field(pRootJson, "Type", stCaptureTime.nType);
	convert.field(pRootJson, "StartTime", stCaptureTime.nStartTime);
	convert.field(pRootJson, "EndTime", stCaptureTime.nEndTime);
}


void Convert::deal(Json::Object* pRootJson, Capture_NS::DaySchedule_S &vstDaySchedules, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "DayOfWeek", (int &)vstDaySchedules.enDayOfWeek);
	convert.structure(pRootJson, "CaptureTimes", vstDaySchedules.captureTimes);
}

/* 抓图计划 */
void Convert::deal(Json::Object *pRootJson, Capture_NS::CapturePlan_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "CapturePlan", stInfo.vstDaySchedules);
}

/* 时间间隔结构体 */
void Convert::deal(Json::Object *pRootJson, Capture_NS::TimeInterval_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "TimeUnit", (int &)stInfo.enTimeUnit);
    convert.field(pRootJson, "Interval", stInfo.unInterval);

}

/* 抓图定时/事件参数配置 */
void Convert::deal(Json::Object *pRootJson, Capture_NS::CaptureConfig_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "PictureFormat", (int &)stInfo.enPictureFormat);
    convert.field(pRootJson, "Width", stInfo.stVideoResolution.nWidth);
	convert.field(pRootJson, "Height", stInfo.stVideoResolution.nHeight);
    convert.field(pRootJson, "ImageQuality", (int &)stInfo.enImageQuality);
    convert.structure(pRootJson, "TimeInterval", stInfo.stTimeInterval);
    convert.field(pRootJson, "Number", stInfo.unNumber);

}

/* 抓图参数 */
void Convert::deal(Json::Object *pRootJson, Capture_NS::CaptureParam_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Timing", stInfo.stCaptureTimingConfig);
    convert.structure(pRootJson, "Event", stInfo.stCaptureEventConfig);
}

/* 图片信息 */
void Convert::deal(Json::Object* pRootJson, Capture_NS::CaptureInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "Type", (int&)stInfo.enType);
    convert.field(pRootJson, "StartTime", stInfo.strStartTime);
    convert.field(pRootJson, "EndTime", stInfo.strEndTime);
    convert.field(pRootJson, "ImagePath", stInfo.strImagePath);
    convert.field(pRootJson, "ImageSize", stInfo.nImageSize);
}

void Convert::deal(Json::Object* pRootJson, std::vector<Capture_NS::CaptureInfo_S> &Infos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Infos", Infos);
}