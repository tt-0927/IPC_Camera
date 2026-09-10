/***
 * @FilePath     : osd_convert.cpp
 * @Author       : huangjunda
 * @Date         : 2025-05-27 11:12:52
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-05-27 11:31:31
 * @Description  : OSD配置转换处理
 */

#include "osd_convert.h"
#include "convert.h" /* 这个要放在video_convert.h的后面 */

void Convert::deal(Json::Object *pRootJson, Osd::CoordinateInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "X", stInfo.nX);
    convert.field(pRootJson, "Y", stInfo.nY);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Osd::CoordinateInfo_S> &vecInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Coordinate", vecInfo);
}

void Convert::deal(Json::Object *pRootJson, Osd::Info_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ID", stInfo.nID);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Name", stInfo.strName);
    convert.field(pRootJson, "ReferenceSize", (int &)stInfo.enRefSize);
}

void Convert::deal(Json::Object *pRootJson, Osd::Overplay_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "EnableFlicker", stInfo.bEnableFlicker);
    convert.field(pRootJson, "EnableReverseColor", stInfo.bEnableRevColor);
    convert.field(pRootJson, "Align", (int &)stInfo.enAlign);
    convert.field(pRootJson, "HorizontalMargin", stInfo.nHorMargin);
    convert.field(pRootJson, "VerticalMargin", stInfo.nVerMargin);
    convert.field(pRootJson, "Width", stInfo.nWidth);
    convert.field(pRootJson, "Height", stInfo.nHeight);
    convert.field(pRootJson, "LineSpace", stInfo.nLineSpace);
    convert.field(pRootJson, "FontSize", stInfo.nFontSize);
    convert.field(pRootJson, "FontColor", stInfo.strFontColor);
    convert.field(pRootJson, "BackColor", stInfo.strBackColor);
    convert.field(pRootJson, "FontAlpha", stInfo.nFontAlpha);
    convert.field(pRootJson, "BackAlpha", stInfo.nBackAlpha);
    convert.field(pRootJson, "ElementType", (int &)stInfo.enElementType);
    convert.field(pRootJson, "EnableTimeZone", stInfo.bEnableTimeZone);
    convert.field(pRootJson, "EnableWeek", stInfo.bEnableWeek);
    convert.field(pRootJson, "EnablePeriod", stInfo.bEnablePeriod);
    convert.field(pRootJson, "Customize", stInfo.strCustomize);
}

void Convert::deal(Json::Object *pRootJson, Osd::OverplayInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Info", stInfo.stuInfo);
    convert.structure(pRootJson, "Overplay", stInfo.stuOverplay);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Osd::OverplayInfo_S> &vecInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "OverplayInfo", vecInfo);
}

void Convert::deal(Json::Object *pRootJson, Osd::Cover_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "EnableRectangle", stInfo.bEnableRectangle);
    convert.field(pRootJson, "EnableSolid", stInfo.bEnableSolid);
    convert.field(pRootJson, "BackColor", stInfo.strBackColor);
    convert.structure(pRootJson, "Coordinate", stInfo.stuCoordinate);
}

void Convert::deal(Json::Object *pRootJson, Osd::CoverInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Info", stInfo.stuInfo);
    convert.structure(pRootJson, "Cover", stInfo.stuCover);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Osd::CoverInfo_S> &vecInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "CoverInfo", vecInfo);
}

void Convert::deal(Json::Object *pRootJson, Osd::OsdAttribute_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "X", stInfo.nX);
    convert.field(pRootJson, "Y", stInfo.nY);
    convert.field(pRootJson, "W", stInfo.nW);
    convert.field(pRootJson, "H", stInfo.nH);
    convert.field(pRootJson, "Attribute", (int &)stInfo.enAttribute);
    convert.field(pRootJson, "FontSize", (int &)stInfo.enFontSize);
    convert.field(pRootJson, "EnFontColor", (int &)stInfo.enFontColor);
    convert.field(pRootJson, "StrFontColor", stInfo.strFontColor);
    convert.field(pRootJson, "Token", stInfo.strToken);
}

void Convert::deal(Json::Object *pRootJson, Osd::OsdNameInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Name", stInfo.strName);
    convert.structure(pRootJson, "Attr", stInfo.stOsdAttr);
}

void Convert::deal(Json::Object *pRootJson, Osd::OsdTimeInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "EnableWeek", stInfo.bEnableWeek);
    convert.field(pRootJson, "TimeFormat", (int &)stInfo.enTimeFormat);
    convert.field(pRootJson, "DateFormat", (int &)stInfo.enDateFormat);
    convert.structure(pRootJson, "Attr", stInfo.stOsdAttr);
}

void Convert::deal(Json::Object *pRootJson, Osd::OsdInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ID", stInfo.nId);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Name", stInfo.strName);
    convert.structure(pRootJson, "Attr", stInfo.stOsdAttr);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Osd::OsdInfo_S> &vecInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Osd", vecInfo);
}

void Convert::deal(Json::Object *pRootJson, Osd::OsdConfig_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Align", (int &)stInfo.enAlign);
    convert.structure(pRootJson, "Name", stInfo.stOsdNameInfo);
    convert.structure(pRootJson, "Time", stInfo.stOsdTimeInfo);
    convert.structure(pRootJson, "Osd", stInfo.vecOsdInfo);
}

void Convert::deal(Json::Object *pRootJson, Osd::CoverAttribute_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ID", stInfo.nId);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Name", stInfo.strName);
    convert.field(pRootJson, "EnColor", (int &)stInfo.enColor);
    convert.field(pRootJson, "StrColor", stInfo.strColor);
    convert.field(pRootJson, "X", stInfo.nX);
    convert.field(pRootJson, "Y", stInfo.nY);
    convert.field(pRootJson, "Width", stInfo.nWidth);
    convert.field(pRootJson, "Height", stInfo.nHeight);
}

void Convert::deal(Json::Object *pRootJson, Osd::CoverConfig_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Attr", stInfo.vecCoverAttr);
}