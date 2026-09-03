#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include <algorithm>
#include <string>
#include <cstring>

void SDKConvert::deal(Json::Object* pRootJson, NET_DeviceInfo_S& stInfo, bool bOutStruct)
{
     if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "DevType", (int &)stInfo.uDevType);
    convert.field(pRootJson, "AlarmInPortNum", (int &)stInfo.uAlarmInPortNum);
    convert.field(pRootJson, "AlarmOutPortNum", (int &)stInfo.uAlarmOutPortNum);
    convert.field(pRootJson, "ChannelNum", (int &)stInfo.uChannelNum);

}


void SDKConvert::deal(Json::Object* pRootJson, NET_DeviceBasicInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "DevModel", stInfo.strDevModel);
    convert.field(pRootJson, "SerialNum", stInfo.strSerialNum);
    convert.field(pRootJson, "FirmwareVersion", stInfo.strFirmwareVersion);
    convert.field(pRootJson, "WebVersion", stInfo.strWebVersion);
    convert.field(pRootJson, "MacAddress", stInfo.strMacAddress);
    convert.field(pRootJson, "DeviceName", stInfo.strDeviceName);
    convert.field(pRootJson, "Manufacturer", stInfo.strManufacturer);
    convert.field(pRootJson, "DeviceTypeV2", stInfo.strDeviceTypeV2);
    convert.field(pRootJson, "CPULoadRatio", stInfo.fCPULoadRatio);
    convert.field(pRootJson, "MemoryUsage", stInfo.fMemoryUsage);
    convert.field(pRootJson, "BootTime", stInfo.nBootTime);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_DeviceStorageInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "HardDiskCount", stInfo.nHardDiskCount);
    convert.field(pRootJson, "HardDiskStatus", stInfo.nHardDiskStatus);
    convert.field(pRootJson, "DiskTotal", stInfo.strDiskTotal);
    convert.field(pRootJson, "DiskAvailable", stInfo.strDiskAvailable);
    convert.field(pRootJson, "DiskUsedSpace", stInfo.strDiskUsedSpace);
    convert.field(pRootJson, "DiskFileType", stInfo.strDiskFileType);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_SystemNtpInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "TimeZone", stInfo.enTimeZone);
    convert.field(pRootJson, "DateFormat", stInfo.enDateFormat);
    convert.field(pRootJson, "IsEnableNTPSync", stInfo.bEnableNTPSync);
    convert.field(pRootJson, "IsManualSync", stInfo.bManualSync);
    convert.field(pRootJson, "DateTime", stInfo.strDateTime);
    convert.field(pRootJson, "IsSyncWithComputer", stInfo.bIsSyncWithComputer);
    convert.field(pRootJson, "Address", stInfo.strAddress);
    convert.field(pRootJson, "Port", stInfo.nPort);
    convert.field(pRootJson, "SyncInterval", stInfo.nSyncInterval);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_SystemTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "DateTime", stInfo.strDateTime);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_NetworkCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "MTU", stInfo.uMTU);
    convert.field(pRootJson, "IPv4DHCP", stInfo.bIPv4DHCP);
    convert.field(pRootJson, "IPv4Address", stInfo.szIpv4Address);
    convert.field(pRootJson, "IPv4GateWay", stInfo.szIPv4GateWay);
    convert.field(pRootJson, "IPv4SubnetMask", stInfo.szIPv4SubnetMask);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_NetworkCfgList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    int nNetworkCount = (int)stInfo.uNetworkCount;
    convert.field(pRootJson, "NetworkCount", nNetworkCount);
    stInfo.uNetworkCount = (UINT32)nNetworkCount;

    if (bOutStruct)
    {
        std::vector<NET_NetworkCfg_S> nets;
        convert.structure(pRootJson, "Networks", nets);
        stInfo.uNetworkCount = (UINT32)std::min<size_t>(nets.size(), NET_MAX_NET_NUM);
        for (UINT32 i = 0; i < stInfo.uNetworkCount; ++i)
        {
            stInfo.stNets[i] = nets[i];
        }
    }
    else
    {
        std::vector<NET_NetworkCfg_S> nets;
        const UINT32 count = std::min<UINT32>(stInfo.uNetworkCount, NET_MAX_NET_NUM);
        stInfo.uNetworkCount = count;
        for (UINT32 i = 0; i < count; ++i)
        {
            nets.push_back(stInfo.stNets[i]);
        }
        convert.structure(pRootJson, "Networks", nets);
    }
}


void SDKConvert::deal(Json::Object* pRootJson, NET_PageInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "CurPage", stInfo.nCurPage);
    convert.field(pRootJson, "PageSize", stInfo.nPageSize);
    convert.field(pRootJson, "DataTotal", stInfo.nDataTotal);
    convert.field(pRootJson, "PageTotal", stInfo.nPageTotal);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_LoginLockInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "IllegalLoginEnable", stInfo.bIllegalLoginEnable);
    convert.field(pRootJson, "CheckInterval", stInfo.nCheckInterval);
    convert.field(pRootJson, "MaxErrorTimes", stInfo.nMaxErrorTimes);
    convert.field(pRootJson, "LockDuration", stInfo.nLockDuration);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_PwdPolicyInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "PwdSecurityLevelEnable", stInfo.bPwdSecurityLevelEnable);
    convert.field(pRootJson, "AllowLowLevelPwdLogin", stInfo.bAllowLowLevelPwdLogin);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_SshAdminInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "SshEnable", stInfo.bSshEnable);
    convert.field(pRootJson, "SshPort", stInfo.nSshPort);
    convert.field(pRootJson, "SshStartTime", stInfo.szSshStartTime);
    convert.field(pRootJson, "SshCountdown", stInfo.szSshCountdown);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_SecurityServicesInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.structure(pRootJson, stInfo.stLoginLock);
    convert.structure(pRootJson, stInfo.stPwdPolicy);
    convert.structure(pRootJson, stInfo.stSshAdmin);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_SshCountdownInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Countdown", stInfo.szCountdown);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_LogServerInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "EnSsl", stInfo.bEnSsl);
    convert.field(pRootJson, "ServerAddr", stInfo.szServerAddr);
    convert.field(pRootJson, "Port", stInfo.nPort);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_LogRetrievalCond_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", stInfo.nType);
    convert.field(pRootJson, "Action", stInfo.nAction);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_LogInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "Type", stInfo.nType);
    convert.field(pRootJson, "Action", stInfo.nAction);
    convert.field(pRootJson, "ChnName", stInfo.szChnName);
    convert.field(pRootJson, "User", stInfo.szUser);
    convert.field(pRootJson, "Host", stInfo.szHost);
    convert.field(pRootJson, "Context", stInfo.szContext);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_LogList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.structure(pRootJson, "Cond", stInfo.stCond);
    convert.structure(pRootJson, stInfo.stPage);

    if (!bOutStruct)
    {
        if (stInfo.nLogCount < 0)
        {
            stInfo.nLogCount = 0;
        }
        if (stInfo.nLogCount > NET_LOG_QUERY_COND_NUM)
        {
            stInfo.nLogCount = NET_LOG_QUERY_COND_NUM;
        }
    }

    convert.field(pRootJson, "LogCount", stInfo.nLogCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "LogInfos");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_LOG_QUERY_COND_NUM)
        {
            nCount = NET_LOG_QUERY_COND_NUM;
        }
        if (nCount < 0)
        {
            nCount = 0;
        }

        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astLogs[i], bOutStruct);
        }
        stInfo.nLogCount = nCount;
        if (stInfo.stPage.nDataTotal == 0)
        {
            stInfo.stPage.nDataTotal = nSize;
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }

        for (int i = 0; i < stInfo.nLogCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astLogs[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "LogInfos", pArray);
    }
}


void SDKConvert::deal(Json::Object* pRootJson, NET_DeviceControlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "uSize", stInfo.uSize);
    convert.field(pRootJson, "channel", stInfo.uChannelID);
    convert.field(pRootJson, "controlType", stInfo.uControlType);
    convert.field(pRootJson, "command", stInfo.uCommand);
    convert.field(pRootJson, "speed", stInfo.uSpeed);
    convert.field(pRootJson, "durationMs", stInfo.uDurationMs);
    convert.field(pRootJson, "param1", stInfo.uParam1);
    convert.field(pRootJson, "param2", stInfo.uParam2);
    convert.field(pRootJson, "ext", stInfo.szExt);
}



/* ===================== 系统升级相关 ============================== */

void SDKConvert::deal(Json::Object*& pRootJson, tagNET_UpgradeInfo* stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "UpgradePath",            stInfo->szUpgradePath);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_UpgradeInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "UpgradePath",            stInfo.szUpgradePath);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_UpgradeStatus_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "UpgradeStatus",            stInfo.nUpgradeStatus);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_UpgradeVersion_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Version",            stInfo.szVersion);
}


void SDKConvert::deal(Json::Object* pRootJson, NET_AudioCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AudioSwitch", stInfo.bAudioSwitch);
    convert.field(pRootJson, "InputType", stInfo.enInputType);
    convert.field(pRootJson, "Format", stInfo.enFormat);
    convert.field(pRootJson, "SampRate", stInfo.enSampRate);
    convert.field(pRootJson, "BitRate", stInfo.enBitRate);
    convert.field(pRootJson, "InputVolume", stInfo.u32InputVolume);
    convert.field(pRootJson, "Denoise", stInfo.bDenoise);
    convert.field(pRootJson, "OutputType", stInfo.enOutputType);
    convert.field(pRootJson, "OutputVolume", stInfo.u32OutputVolume);
}


/* ===================== 修改用户密码参数 ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_UserPasswordInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "UserName", stInfo.strUserName);
    convert.field(pRootJson, "OldPassword", stInfo.strOldPassword);
    convert.field(pRootJson, "NewPassword", stInfo.strNewPassword);
}
