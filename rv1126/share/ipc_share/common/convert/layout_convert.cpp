/**
 * @FilePath     : layout_convert.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-16 17:00:39
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-16 17:05:13
 * @Description  : 布局定义装换
 */

#include "layout_convert.h"

#include "common_convert.h"
#include "convert.h" /* 这个要放在UserDefineConvert的后面 */

/* 转换函数 */
void Convert::deal(Json::Object* pRootJson, Layout::Rect_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "X", stInfo.nX);
    convert.field(pRootJson, "Y", stInfo.nY);
    convert.field(pRootJson, "Width", stInfo.nWidth);
    convert.field(pRootJson, "Height", stInfo.nHeight);
}

/* 转换函数 */
void Convert::deal(Json::Object* pRootJson, Layout::Item_S& stItem, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Pos", stItem.nPos);
    convert.field(pRootJson, "ChnId", stItem.nChnId);
}

/* 转换函数 */
void Convert::deal(Json::Object* pRootJson, std::vector<Layout::Item_S>& vstItem, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Items", vstItem);
}

/* 转换函数 */
void Convert::deal(Json::Object* pRootJson, Layout::ChnInfo_S& stChnInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stChnInfo.stItem);
    convert.structure(pRootJson, stChnInfo.stRect);
}

/* 转换函数 */
void Convert::deal(Json::Object* pRootJson, std::vector<Layout::ChnInfo_S>& vChnInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "ChnInfos", vChnInfos);
}