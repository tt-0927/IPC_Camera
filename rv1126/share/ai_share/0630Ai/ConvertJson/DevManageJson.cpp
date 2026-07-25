#include "DevManageJson.hpp"

#include "Convert.h"

using namespace Ai0630_NS;

/* 转换函数 */
void Ai0630_NS::dealJson(Json::Object* pRootJson, GetReqDevInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "CurPageNum", stInfo.nCurPageNum);
    convert.field(pRootJson, "PageSize", stInfo.nPageSize);
    convert.field(pRootJson, "MacKey", stInfo.strMacKey);
    convert.field(pRootJson, "DevNameKey", stInfo.strDevNameKey);
    convert.field(pRootJson, "DevModelKey", stInfo.strModelKey);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, AddDevInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "DevName", stInfo.strDevName);
    convert.field(pRootJson, "IP", stInfo.strIp);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, EditDevInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "ID", stInfo.nId);
    convert.field(pRootJson, "DevName", stInfo.strDevName);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, DelDevInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "DelIdArray", stInfo.vDelId);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, DevDataInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "ID", stInfo.nId);
    convert.field(pRootJson, "DevNameKey", stInfo.strDevName);
    convert.field(pRootJson, "DevModelKey", stInfo.strDevModel);
    convert.field(pRootJson, "MacKey", stInfo.strMac);
    convert.field(pRootJson, "IP", stInfo.strIP);
    convert.field(pRootJson, "State", stInfo.nState);
}