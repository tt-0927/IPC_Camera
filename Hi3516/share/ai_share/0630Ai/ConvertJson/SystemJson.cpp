#include "SystemJson.hpp"

#include "Convert.h"

using namespace Ai0630_NS;

void Ai0630_NS::dealJson(Json::Object* pRootJson, LoginInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "User", stInfo.strUserName);
    convert.field(pRootJson, "Password", stInfo.strPassword);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, AdjuestTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "DateTime", stInfo.strDateTime);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, SysNetwork_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "Ip", stInfo.strIp);
    convert.field(pRootJson, "Netmask", stInfo.strNetmask);
    convert.field(pRootJson, "Getway", stInfo.strGateway);
    convert.field(pRootJson, "Mac", stInfo.strMacAddr);
    convert.field(pRootJson, "DNS", stInfo.strDns);
    convert.field(pRootJson, "DHCP", stInfo.nDhcp);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, RegisterInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "DevID", stInfo.strDevID);
    convert.field(pRootJson, "MachinSn", stInfo.strMachinSn);
    convert.field(pRootJson, "RegisterCode", stInfo.strRegisterEg);
    convert.field(pRootJson, "StartTime", stInfo.strStartTime);
    convert.field(pRootJson, "PrevTime", stInfo.strPrevTime);
    convert.field(pRootJson, "UsableTimer", stInfo.nUsableTimer);
    convert.field(pRootJson, "ActionTime", (int&)stInfo.enActionTime);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, SystemInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);

    convert.field(pRootJson, "Version", stInfo.strVersion);
    convert.field(pRootJson, "MachinSn", stInfo.strMachinSn);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, CpuInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);

    convert.field(pRootJson, "HZ", stInfo.dHz);
    convert.field(pRootJson, "Temperature", stInfo.nTemp);
    convert.field(pRootJson, "Percent", stInfo.dPercent);
    convert.field(pRootJson, "Process", stInfo.nProcess);
    convert.field(pRootJson, "Speed", stInfo.dSpeed);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, MemoryInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);

    convert.field(pRootJson, "Used", stInfo.dUsed);
    convert.field(pRootJson, "Total", stInfo.dTotal);
    convert.field(pRootJson, "Usable", stInfo.dUsable);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, GpuInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);

    convert.field(pRootJson, "HZ", stInfo.dHz);
    convert.field(pRootJson, "Temperature", stInfo.nTemp);
    convert.field(pRootJson, "Percent", stInfo.dPercent);
    convert.field(pRootJson, "BitWidth", stInfo.nBitWidth);
    convert.field(pRootJson, "Computility", stInfo.dComputility);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, GrapMemoryInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);

    convert.field(pRootJson, "BandWidth", stInfo.dBandWidth);
    convert.field(pRootJson, "Percent", stInfo.dPercent);
    convert.field(pRootJson, "Used", stInfo.dUsed);
    convert.field(pRootJson, "Total", stInfo.dTotal);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, PerfInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);

    convert.structure(pRootJson, "CPU", stInfo.stCpuInfo);
    convert.structure(pRootJson, "Memory", stInfo.stMemoryInfo);
    convert.structure(pRootJson, "GPU", stInfo.stGpuInfo);
    convert.structure(pRootJson, "GrapMemory", stInfo.stGrapMemInfo);
}

void Ai0630_NS::dealJson(Json::Object* pRootJson, UpgradeInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert convert(bOutStruct);
    convert.field(pRootJson, "FileName", stInfo.strFileName);
    convert.field(pRootJson, "FilePath", stInfo.strFilePath);
    convert.field(pRootJson, "Status", (int&)stInfo.enStatus);
}