/*** 
 * @FilePath     : common_convert.cpp
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2024-10-05 11:10:15
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-23 15:19:59
 * @Description  : 公共定义数据的转换
 */

#include "common_convert.h"

#include "convert.h" /* 这个要放在common_convert.h的后面 */

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Common::PageInfo_S &stPageInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "CurPage", stPageInfo.nCurPage);
    convert.field(pRootJson, "PageSize", stPageInfo.nPageSize);
    convert.field(pRootJson, "DataTotal", stPageInfo.nDataTotal);
    convert.field(pRootJson, "PageTotal", stPageInfo.nPageTotal);
}

void Convert::deal(
    Json::Object *pRootJson,
    Common::Pos_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "X", stInfo.nX);
    convert.field(pRootJson, "Y", stInfo.nY);
}
void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Common::Pos_S> &vecInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, vecInfo);
}
void Convert::deal(
    Json::Object *pRootJson,
    Common::PosF_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "X", stInfo.fX);
    convert.field(pRootJson, "Y", stInfo.fY);
}
void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Common::PosF_S> &vecInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, vecInfo);
}
void Convert::deal(
    Json::Object *pRootJson,
    Common::Rect_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "X", stInfo.nX);
    convert.field(pRootJson, "Y", stInfo.nY);
    convert.field(pRootJson, "W", stInfo.nWidth);
    convert.field(pRootJson, "H", stInfo.nHeight);
}
void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Common::Rect_S> &vecInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, vecInfo);
}
void Convert::deal(
    Json::Object *pRootJson,
    Common::RectF_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "X", stInfo.fX);
    convert.field(pRootJson, "Y", stInfo.fY);
    convert.field(pRootJson, "W", stInfo.fWidth);
    convert.field(pRootJson, "H", stInfo.fHeight);
}
void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Common::RectF_S> &vecInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, vecInfo);
}
void Convert::deal(
    Json::Object *pRootJson,
    Common::Time_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Hour", stInfo.nHour);
    convert.field(pRootJson, "Min", stInfo.nMinute);
    convert.field(pRootJson, "Sec", stInfo.nSecond);
    convert.field(pRootJson, "MSec", stInfo.nMilliSec);
}
void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Common::Time_S> &vecInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, vecInfo);
}
void Convert::deal(
    Json::Object *pRootJson,
    Common::SchedTime_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "StartTime", stInfo.stStart);
    convert.structure(pRootJson, "StopTime", stInfo.stStop);
}
void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Common::SchedTime_S> &vecInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, vecInfo);
}

void Convert::deal(
    Json::Object *pRootJson,
    Common::Date_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Year", stInfo.nYear);
    convert.field(pRootJson, "Month", stInfo.nMonth);
    convert.field(pRootJson, "Day", stInfo.nDay);
}
void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Common::Date_S> &vecInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, vecInfo);
}