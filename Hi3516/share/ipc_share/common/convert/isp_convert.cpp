/*** 
 * @FilePath     : isp_convert.cpp
 * @Author       : cyc
 * @Date         : 2025-06-13 10:47:22
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-14 10:11:15
 * @Description  : 显示配置数据结构转换
 */

#include "isp_convert.h"
#include "common_convert.h"
#include "convert.h"
#include <iostream>

void Convert::deal(
    Json::Object *pRootJson,
    ISP::Light_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "LightLevel", stInfo.nLightLevel);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::FillLight_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "LightType", (int &)stInfo.enLightType);
    convert.structure(pRootJson, "WhiteAttr", stInfo.stWhiteAttr);
    convert.structure(pRootJson, "RedAttr", stInfo.stRedAttr);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::DayNightAttr_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", (int &)stInfo.enDayNightMode);
    convert.structure(pRootJson, "BeginTime", stInfo.stBeginTime);
    convert.structure(pRootJson, "EndTime", stInfo.stEndTime);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivityLevel);
    convert.field(pRootJson, "FilterTime", stInfo.nFilterTime);
    convert.field(pRootJson, "LightMode", (int &)stInfo.enLightMode);
    convert.field(pRootJson, "FillLightExp", stInfo.bFillLightExp);
    convert.structure(pRootJson, "FillLight", stInfo.stFillLight);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::ImageParam_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Brightness", stInfo.nBrightness);
    convert.field(pRootJson, "Contrast", stInfo.nContrast);
    convert.field(pRootJson, "Saturation", stInfo.nSaturation);
    convert.field(pRootJson, "Sharpness", stInfo.nSharpness);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::VideoAdjust_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", (int &)stInfo.enMirrorMode);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::AwbAttr_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", (int &)stInfo.enAwbMode);
    convert.field(pRootJson, "RGain", stInfo.nRGain);
    convert.field(pRootJson, "BGain", stInfo.nBGain);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::ExposureAttr_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ExpTime", (int &)stInfo.enExpTime);
    convert.field(pRootJson, "AntiBanding", stInfo.bAntiBanding);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::WdrAttr_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "WdrLevel", stInfo.nWdrLevel);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::HlsAttr_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "HlsLevel", stInfo.nHlsLevel);
}


void Convert::deal(
    Json::Object *pRootJson,
    ISP::BackLightArrt_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", (int &)stInfo.enBackLightArea);
    convert.structure(pRootJson, "Wdr", stInfo.stWdrAttr);
    convert.structure(pRootJson, "Hls", stInfo.stHlsAttr);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::DnrAttr_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", (int &)stInfo.enDnrMode);
    convert.field(pRootJson, "DnrLevel", stInfo.nDnrLevel);
    convert.field(pRootJson, "SnrLevel", stInfo.nSnrLevel);
    convert.field(pRootJson, "TnrLevel", stInfo.nTnrLevel);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::SceneParams_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "SceneType", (int &)stInfo.enSceneType);
    convert.structure(pRootJson, "Awb", stInfo.stAwbAttr);
    convert.structure(pRootJson, "BackLight", stInfo.stBackLightAttr);
    convert.structure(pRootJson, "DayNight", stInfo.stDayNightAttr);
    convert.structure(pRootJson, "Dnr", stInfo.stDnrAttr);
    convert.structure(pRootJson, "Exposure", stInfo.stExpAttr);
    convert.structure(pRootJson, "Image", stInfo.stImageParam);
}

void Convert::deal(
    Json::Object *pRootJson,
    std::vector<ISP::SceneParams_S> &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "SceneParam", stInfo);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::AllSceneParams_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "CurrentScene", (int&)stInfo.enCurrentScene);
    convert.structure(pRootJson,stInfo.aSceneParams);
}


void Convert::deal(Json::Object* pRootJson, ISP::SceneTime_S &stSceneTime, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Type", (int &)stSceneTime.enSceneType);
	convert.field(pRootJson, "StartTime", stSceneTime.nStartTime);
	convert.field(pRootJson, "EndTime", stSceneTime.nEndTime);
}


void Convert::deal(Json::Object* pRootJson, ISP::MonthSchedule_S &stMonthSchedule, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "DayOfMonth", (int &)stMonthSchedule.enMonthfYear);
	convert.structure(pRootJson, "SceneTimes", stMonthSchedule.aSceneTimes);
}
void Convert::deal(Json::Object* pRootJson, std::vector<ISP::MonthSchedule_S> &monthSchedules, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "Scenechedules", monthSchedules);
}


void Convert::deal(
    Json::Object *pRootJson,
    ISP::SceneSchedule_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, stInfo.aMonthSchedules);
}

void Convert::deal(
    Json::Object *pRootJson,
    ISP::TimeRange_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "StartTime", stInfo.stStartTime);
    convert.structure(pRootJson, "StopTime", stInfo.stEndTime);
}


void Convert::deal(
    Json::Object *pRootJson,
    ISP::SceneType_E &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", (int &)stInfo);
}



