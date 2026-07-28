/**
 * @file DeviceInfoConvert.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceInfoConvert 模块实现
 * 功能说明：
 * 1. 实现 DeviceInfoConvert 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "DeviceInfoConvert.h"
#include "SDKConvert.h"
#include <algorithm>
#include <string>
#include <cstring>

static constexpr int kOsdCustomSlotCount = NET_TV_OSD_CUSTOM_MAX_NUM;
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 clamp_time_count 定义的内部处理。
 * @param [in] count 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static UINT32 clamp_time_count(UINT32 count)
{
    return (count > NET_TV_PLAN_TIME_SECTION_NUM_ADAY) ? NET_TV_PLAN_TIME_SECTION_NUM_ADAY : count;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 JsonToFloatArray 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] values 函数处理参数。
 * @param [in] maxCount 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 FloatArrayToJson 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] values 函数处理参数。
 * @param [in] count 函数处理参数。
 * @param [in] maxCount 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
    convert.field(pRootJson, "MacAddress", stInfo.strMacAddress);
    convert.field(pRootJson, "DeviceName", stInfo.strDeviceName);
    convert.field(pRootJson, "Manufacturer", stInfo.strManufacturer);
    convert.field(pRootJson, "DeviceTypeV2", stInfo.strDeviceTypeV2);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_NetworkCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "MTU", stInfo.dwMTU);
    convert.field(pRootJson, "IPv4DHCP", stInfo.bIPv4DHCP);
    convert.field(pRootJson, "IPv4Address", stInfo.szIpv4Address);
    convert.field(pRootJson, "IPv4GateWay", stInfo.szIPv4GateWay);
    convert.field(pRootJson, "IPv4SubnetMask", stInfo.szIPv4SubnetMask);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_WifiStaCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "EnableWifi", stInfo.bEnableWifi);
    convert.field(pRootJson, "EnableBoost", stInfo.bEnableBoost);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_WifiWepKey_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Index", stInfo.nIndex);
    convert.field(pRootJson, "Value", stInfo.szValue);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_WifiStaConnect_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Ssid", stInfo.szSsid);
    convert.field(pRootJson, "SecurityMode", stInfo.nSecurityMode);
    convert.field(pRootJson, "IpAddress", stInfo.szIpAddress);
    convert.field(pRootJson, "Password", stInfo.szPassword);
    convert.field(pRootJson, "Pairwise", stInfo.szPairwise);
    convert.field(pRootJson, "WepKeyLen", stInfo.nWepKeyLen);
    convert.field(pRootJson, "WepIsHex", stInfo.bWepIsHex);
    convert.field(pRootJson, "AuthAlg", stInfo.szAuthAlg);
    convert.field(pRootJson, "WepKeyCount", stInfo.nWepKeyCount);

    convert.field(pRootJson, "EapIdentity", stInfo.szEapIdentity);
    convert.field(pRootJson, "EapPassword", stInfo.szEapPassword);
    convert.field(pRootJson, "PeapVersion", stInfo.szPeapVersion);
    convert.field(pRootJson, "Phase2", stInfo.szPhase2);
    convert.field(pRootJson, "AnonymousIdentity", stInfo.szAnonymousIdentity);
    convert.field(pRootJson, "CaCertPath", stInfo.szCaCertPath);
    convert.field(pRootJson, "PeapLabel", stInfo.szPeapLabel);

    convert.field(pRootJson, "TlsIdentity", stInfo.szTlsIdentity);
    convert.field(pRootJson, "PrivateKeyPasswd", stInfo.szPrivateKeyPasswd);
    convert.field(pRootJson, "EapolVersion", stInfo.szEapolVersion);
    convert.field(pRootJson, "ClientCertPath", stInfo.szClientCertPath);
    convert.field(pRootJson, "PrivateKeyPath", stInfo.szPrivateKeyPath);
    convert.field(pRootJson, "CtrlInterface", stInfo.szCtrlInterface);
    convert.field(pRootJson, "InterfaceName", stInfo.szInterfaceName);

    {
        int nKeyCount = stInfo.nWepKeyCount;
        if (nKeyCount < 0)
        {
            nKeyCount = 0;
        }
        if (nKeyCount > 4)
        {
            nKeyCount = 4;
        }

        if (bOutStruct)
        {
            Json::Object* pWepKeys = Json::get(pRootJson, "WepKeys");
            if (pWepKeys)
            {
                int i = 0;
                for (i = 0; i < nKeyCount; ++i)
                {
                    Json::Object* pKeyObj = Json::get(pWepKeys, std::to_string(i));
                    if (pKeyObj)
                    {
                        deal(pKeyObj, stInfo.astWepKeys[i], bOutStruct);
                    }
                }
            }
        }
        else
        {
            Json::Object* pWepKeys = Json::init();
            int i = 0;
            for (i = 0; i < nKeyCount; ++i)
            {
                Json::Object* pKeyObj = Json::init();
                deal(pKeyObj, stInfo.astWepKeys[i], bOutStruct);
                Json::add(pWepKeys, std::to_string(i), pKeyObj);
            }
            Json::add(pRootJson, "WepKeys", pWepKeys);
        }
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_4GInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Apn", stInfo.szApn);
    convert.field(pRootJson, "UserName", stInfo.szUserName);
    convert.field(pRootJson, "Password", stInfo.szPassword);
    convert.field(pRootJson, "CallNumber", stInfo.szCallNumber);
    convert.field(pRootJson, "Mtu", stInfo.nMtu);
    convert.field(pRootJson, "AuthMode", stInfo.nAuthMode);
    convert.field(pRootJson, "NetworkMode", stInfo.nNetworkMode);
    convert.field(pRootJson, "DialMode", stInfo.nDialMode);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_HotspotInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enabled", stInfo.bEnabled);
    convert.field(pRootJson, "Ssid", stInfo.szSsid);
    convert.field(pRootJson, "SecurityMode", stInfo.szSecurityMode);
    convert.field(pRootJson, "EncryptionType", stInfo.szEncryptionType);
    convert.field(pRootJson, "Password", stInfo.szPassword);
    convert.field(pRootJson, "ConfirmPassword", stInfo.szConfirmPassword);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_HotspotConnDevice_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Index", stInfo.nIndex);
    convert.field(pRootJson, "Mac", stInfo.szMac);
    convert.field(pRootJson, "Ip", stInfo.szIp);
    convert.field(pRootJson, "ConnTime", stInfo.szConnTime);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_HotspotConnInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Status", stInfo.szStatus);
    convert.field(pRootJson, "Total", stInfo.nTotal);

    if (!bOutStruct)
    {
        if (stInfo.nDeviceCount < 0)
        {
            stInfo.nDeviceCount = 0;
        }
        if (stInfo.nDeviceCount > NET_TV_HOTSPOT_CONN_MAX_NUM)
        {
            stInfo.nDeviceCount = NET_TV_HOTSPOT_CONN_MAX_NUM;
        }
    }

    convert.field(pRootJson, "DeviceCount", stInfo.nDeviceCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "Devices");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TV_HOTSPOT_CONN_MAX_NUM)
        {
            nCount = NET_TV_HOTSPOT_CONN_MAX_NUM;
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
            deal(pItem, stInfo.astDevices[i], bOutStruct);
        }
        stInfo.nDeviceCount = nCount;
        if (stInfo.nTotal == 0)
        {
            stInfo.nTotal = nSize;
        }
    }
    else
    {
        int nCount = stInfo.nDeviceCount;

        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }

        for (int i = 0; i < nCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astDevices[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "Devices", pArray);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_SshCountdownInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Countdown", stInfo.szCountdown);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
        if (stInfo.nLogCount > NET_TV_LOG_QUERY_COND_NUM)
        {
            stInfo.nLogCount = NET_TV_LOG_QUERY_COND_NUM;
        }
    }

    convert.field(pRootJson, "LogCount", stInfo.nLogCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "LogInfos");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TV_LOG_QUERY_COND_NUM)
        {
            nCount = NET_TV_LOG_QUERY_COND_NUM;
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "VideoStatus", stInfo.nVideoStatus);
    convert.field(pRootJson, "AudioStatus", stInfo.nAudioStatus);
    convert.field(pRootJson, "RecordStatus", stInfo.nRecordStatus);
    convert.field(pRootJson, "RecordFormat", stInfo.nRecordFormat);
    convert.field(pRootJson, "EventType", stInfo.nEventType);
    convert.field(pRootJson, "Path", stInfo.szPath);
    convert.field(pRootJson, "RedunPath", stInfo.szRedunPath);
    convert.field(pRootJson, "RecordName", stInfo.szRecordName);
    convert.field(pRootJson, "RecordTime", stInfo.szRecordTime);
    convert.field(pRootJson, "StreamType", stInfo.nStreamType);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordStatusInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Status", stInfo.nStatus);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Type", stInfo.nType);
    convert.field(pRootJson, "StartTime", stInfo.nStartTime);
    convert.field(pRootJson, "EndTime", stInfo.nEndTime);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordDaySchedule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "DayOfWeek", stInfo.nDayOfWeek);

    if (!bOutStruct)
    {
        if (stInfo.nRecordTimeCount < 0)
        {
            stInfo.nRecordTimeCount = 0;
        }
        if (stInfo.nRecordTimeCount > NET_TV_TIME_DURATION_NUM)
        {
            stInfo.nRecordTimeCount = NET_TV_TIME_DURATION_NUM;
        }
    }
    convert.field(pRootJson, "RecordTimeCount", stInfo.nRecordTimeCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "RecordTimes");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TV_TIME_DURATION_NUM)
        {
            nCount = NET_TV_TIME_DURATION_NUM;
        }
        if (nCount < 0)
        {
            nCount = 0;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astRecordTimes[i], bOutStruct);
            }
        }
        stInfo.nRecordTimeCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }
        for (int i = 0; i < stInfo.nRecordTimeCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astRecordTimes[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "RecordTimes", pArray);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordSchedule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "IsEnable", stInfo.bEnable);

    if (!bOutStruct)
    {
        if (stInfo.nDayScheduleCount < 0)
        {
            stInfo.nDayScheduleCount = 0;
        }
        if (stInfo.nDayScheduleCount > NET_TV_PLAN_DAY_NUM_AWEEK)
        {
            stInfo.nDayScheduleCount = NET_TV_PLAN_DAY_NUM_AWEEK;
        }
    }
    convert.field(pRootJson, "DayScheduleCount", stInfo.nDayScheduleCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "DaySchedules");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TV_PLAN_DAY_NUM_AWEEK)
        {
            nCount = NET_TV_PLAN_DAY_NUM_AWEEK;
        }
        if (nCount < 0)
        {
            nCount = 0;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astDaySchedules[i], bOutStruct);
            }
        }
        stInfo.nDayScheduleCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }
        for (int i = 0; i < stInfo.nDayScheduleCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astDaySchedules[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "DaySchedules", pArray);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordAdvancedParam_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "LoopWrite", stInfo.bLoopWrite);
    convert.field(pRootJson, "PreTime", stInfo.nPreTime);
    convert.field(pRootJson, "DelayTime", stInfo.nDelayTime);
    convert.field(pRootJson, "StreamType", stInfo.nStreamType);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordFindCond_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "Type", stInfo.nType);
    convert.field(pRootJson, "Year", stInfo.szYear);
    convert.field(pRootJson, "Month", stInfo.szMonth);
    convert.field(pRootJson, "Date", stInfo.szDate);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "Filename", stInfo.szFilename);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordVideoTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartTime", stInfo.nStartTime);
    convert.field(pRootJson, "EndTime", stInfo.nEndTime);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordFindResult_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);

    if (!bOutStruct)
    {
        if (stInfo.nDateCount < 0)
        {
            stInfo.nDateCount = 0;
        }
        if (stInfo.nDateCount > NET_TV_RECORD_DATE_MAX_NUM)
        {
            stInfo.nDateCount = NET_TV_RECORD_DATE_MAX_NUM;
        }
        if (stInfo.nVideoTimeCount < 0)
        {
            stInfo.nVideoTimeCount = 0;
        }
        if (stInfo.nVideoTimeCount > NET_TV_TIME_DURATION_NUM)
        {
            stInfo.nVideoTimeCount = NET_TV_TIME_DURATION_NUM;
        }
    }

    convert.field(pRootJson, "DateCount", stInfo.nDateCount);
    if (bOutStruct)
    {
        Json::Object* pDates = Json::get(pRootJson, "Dates");
        int nSize = pDates ? Json::Array::size(pDates) : 0;
        int nCount = nSize;
        if (nCount > NET_TV_RECORD_DATE_MAX_NUM)
        {
            nCount = NET_TV_RECORD_DATE_MAX_NUM;
        }
        for (int i = 0; pDates && i < nCount; ++i)
        {
            std::string strDate;
            Json::Object* pItem = Json::Array::get(pDates, i);
            if (pItem)
            {
                Json::Value::get(pItem, strDate);
                std::strncpy(stInfo.aszDates[i], strDate.c_str(), sizeof(stInfo.aszDates[i]) - 1);
            }
        }
        stInfo.nDateCount = nCount;
    }
    else
    {
        Json::Object* pDates = Json::Array::init();
        if (pDates)
        {
            for (int i = 0; i < stInfo.nDateCount; ++i)
            {
                Json::Array::add(pDates, stInfo.aszDates[i]);
            }
            Json::add(pRootJson, "Dates", pDates);
        }
    }

    convert.field(pRootJson, "Filename", stInfo.szFilename);
    convert.field(pRootJson, "VideoTimeCount", stInfo.nVideoTimeCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "VideoTimes");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TV_TIME_DURATION_NUM)
        {
            nCount = NET_TV_TIME_DURATION_NUM;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astVideoTimes[i], bOutStruct);
            }
        }
        stInfo.nVideoTimeCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }
        for (int i = 0; i < stInfo.nVideoTimeCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astVideoTimes[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "VideoTimes", pArray);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordFileList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.structure(pRootJson, "Find", stInfo.stFind);

    if (!bOutStruct)
    {
        if (stInfo.nResultCount < 0)
        {
            stInfo.nResultCount = 0;
        }
        if (stInfo.nResultCount > NET_TV_RECORD_FILE_MAX_NUM)
        {
            stInfo.nResultCount = NET_TV_RECORD_FILE_MAX_NUM;
        }
    }
    convert.field(pRootJson, "ResultCount", stInfo.nResultCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "Infos");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TV_RECORD_FILE_MAX_NUM)
        {
            nCount = NET_TV_RECORD_FILE_MAX_NUM;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astResults[i], bOutStruct);
            }
        }
        stInfo.nResultCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (!pArray)
        {
            return;
        }
        for (int i = 0; i < stInfo.nResultCount; ++i)
        {
            Json::Object* pItem = Json::init();
            if (!pItem)
            {
                continue;
            }
            deal(pItem, stInfo.astResults[i], bOutStruct);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "Infos", pArray);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordDownloadInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "Path", stInfo.szPath);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordDownloadProgress_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Filename", stInfo.szFilename);
    convert.field(pRootJson, "DownloadProgress", stInfo.nProgress);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordDownloadList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    if (!bOutStruct)
    {
        if (stInfo.nDownloadCount < 0)
        {
            stInfo.nDownloadCount = 0;
        }
        if (stInfo.nDownloadCount > NET_TV_RECORD_DOWNLOAD_MAX_NUM)
        {
            stInfo.nDownloadCount = NET_TV_RECORD_DOWNLOAD_MAX_NUM;
        }
        if (stInfo.nProgressCount < 0)
        {
            stInfo.nProgressCount = 0;
        }
        if (stInfo.nProgressCount > NET_TV_RECORD_DOWNLOAD_MAX_NUM)
        {
            stInfo.nProgressCount = NET_TV_RECORD_DOWNLOAD_MAX_NUM;
        }
    }

    convert.field(pRootJson, "DownloadCount", stInfo.nDownloadCount);
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "DownloadInfos");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TV_RECORD_DOWNLOAD_MAX_NUM)
        {
            nCount = NET_TV_RECORD_DOWNLOAD_MAX_NUM;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astDownloads[i], bOutStruct);
            }
        }
        stInfo.nDownloadCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (pArray)
        {
            for (int i = 0; i < stInfo.nDownloadCount; ++i)
            {
                Json::Object* pItem = Json::init();
                if (!pItem)
                {
                    continue;
                }
                deal(pItem, stInfo.astDownloads[i], bOutStruct);
                Json::Array::add(pArray, pItem);
            }
            Json::add(pRootJson, "DownloadInfos", pArray);
        }
    }

    convert.field(pRootJson, "ProgressCount", stInfo.nProgressCount);
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "DownloadProgressInfos");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_TV_RECORD_DOWNLOAD_MAX_NUM)
        {
            nCount = NET_TV_RECORD_DOWNLOAD_MAX_NUM;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astProgress[i], bOutStruct);
            }
        }
        stInfo.nProgressCount = nCount;
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        if (pArray)
        {
            for (int i = 0; i < stInfo.nProgressCount; ++i)
            {
                Json::Object* pItem = Json::init();
                if (!pItem)
                {
                    continue;
                }
                deal(pItem, stInfo.astProgress[i], bOutStruct);
                Json::Array::add(pArray, pItem);
            }
            Json::add(pRootJson, "DownloadProgressInfos", pArray);
        }
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_VideoOsdCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
    {
        std::memset(stInfo.OsdInfo, 0, sizeof(stInfo.OsdInfo));
    }

#define NETSDK_CONVERT_ENUM_FIELD(jsonKey, enumField, enumType)      \
    do                                                        \
    {                                                         \
        int nTmpValue = (int)(enumField);                     \
        convert.field(pRootJson, jsonKey, nTmpValue);         \
        if (bOutStruct)                                      \
        {                                                     \
            (enumField) = (enumType)nTmpValue;                \
        }                                                     \
    } while (0)

    /* 全局对齐方式 */
    NETSDK_CONVERT_ENUM_FIELD("Align", stInfo.enAlign, OSD_ALIGN_E);

    /* Name OSD */
    convert.field(pRootJson, "NameEnable",      stInfo.stOsdNameInfo.bEnable);
    convert.field(pRootJson, "Name",            stInfo.stOsdNameInfo.strName);
    convert.field(pRootJson, "NameX",           stInfo.stOsdNameInfo.stOsdAttr.nX);
    convert.field(pRootJson, "NameY",           stInfo.stOsdNameInfo.stOsdAttr.nY);
    convert.field(pRootJson, "NameW",           stInfo.stOsdNameInfo.stOsdAttr.nW);
    convert.field(pRootJson, "NameH",           stInfo.stOsdNameInfo.stOsdAttr.nH);
    NETSDK_CONVERT_ENUM_FIELD("NameAttribute", stInfo.stOsdNameInfo.stOsdAttr.enAttribute, OSD_ATTRIBUTE_E);
    NETSDK_CONVERT_ENUM_FIELD("NameFontSize",  stInfo.stOsdNameInfo.stOsdAttr.enFontSize,  OSD_FONT_SIZE_E);
    NETSDK_CONVERT_ENUM_FIELD("NameFontColor", stInfo.stOsdNameInfo.stOsdAttr.enFontColor, OSD_COLOR_E);
    convert.field(pRootJson, "NameCustomColor", stInfo.stOsdNameInfo.stOsdAttr.strFontColor);
    convert.field(pRootJson, "NameToken",       stInfo.stOsdNameInfo.stOsdAttr.strToken);

    /* Time OSD */
    convert.field(pRootJson, "TimeEnable",      stInfo.stOsdTimeInfo.bEnable);
    convert.field(pRootJson, "TimeEnableWeek",  stInfo.stOsdTimeInfo.bEnableWeek);
    NETSDK_CONVERT_ENUM_FIELD("TimeFormat", stInfo.stOsdTimeInfo.enTimeFormat, OSD_TIME_FORMAT_E);
    NETSDK_CONVERT_ENUM_FIELD("DateFormat", stInfo.stOsdTimeInfo.enDateFormat, OSD_DATE_FORMAT_E);
    convert.field(pRootJson, "TimeX",           stInfo.stOsdTimeInfo.stOsdAttr.nX);
    convert.field(pRootJson, "TimeY",           stInfo.stOsdTimeInfo.stOsdAttr.nY);
    convert.field(pRootJson, "TimeW",           stInfo.stOsdTimeInfo.stOsdAttr.nW);
    convert.field(pRootJson, "TimeH",           stInfo.stOsdTimeInfo.stOsdAttr.nH);
    NETSDK_CONVERT_ENUM_FIELD("TimeAttribute", stInfo.stOsdTimeInfo.stOsdAttr.enAttribute, OSD_ATTRIBUTE_E);
    NETSDK_CONVERT_ENUM_FIELD("TimeFontSize",  stInfo.stOsdTimeInfo.stOsdAttr.enFontSize,  OSD_FONT_SIZE_E);
    NETSDK_CONVERT_ENUM_FIELD("TimeFontColor", stInfo.stOsdTimeInfo.stOsdAttr.enFontColor, OSD_COLOR_E);
    convert.field(pRootJson, "TimeCustomColor", stInfo.stOsdTimeInfo.stOsdAttr.strFontColor);
    convert.field(pRootJson, "TimeToken",       stInfo.stOsdTimeInfo.stOsdAttr.strToken);

    /* 当前IPC能力只开放4个自定义字符叠加槽位，结构体保留32槽位用于兼容。 */
    for (int i = 0; i < kOsdCustomSlotCount; ++i)
    {
        char szKey[64] = {0};

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Id", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].nId);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Enable", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].bEnable);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Name", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].strName);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_X", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.nX);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Y", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.nY);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_W", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.nW);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_H", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.nH);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Attribute", i);
        {
            int nTmpValue = (int)stInfo.OsdInfo[i].stOsdAttr.enAttribute;
            convert.field(pRootJson, szKey, nTmpValue);
            if (bOutStruct)
            {
                stInfo.OsdInfo[i].stOsdAttr.enAttribute = (OSD_ATTRIBUTE_E)nTmpValue;
            }
        }

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_FontSize", i);
        {
            int nTmpValue = (int)stInfo.OsdInfo[i].stOsdAttr.enFontSize;
            convert.field(pRootJson, szKey, nTmpValue);
            if (bOutStruct)
            {
                stInfo.OsdInfo[i].stOsdAttr.enFontSize = (OSD_FONT_SIZE_E)nTmpValue;
            }
        }

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_FontColor", i);
        {
            int nTmpValue = (int)stInfo.OsdInfo[i].stOsdAttr.enFontColor;
            convert.field(pRootJson, szKey, nTmpValue);
            if (bOutStruct)
            {
                stInfo.OsdInfo[i].stOsdAttr.enFontColor = (OSD_COLOR_E)nTmpValue;
            }
        }

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_CustomColor", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.strFontColor);

        snprintf(szKey, sizeof(szKey), "OsdInfo%02d_Token", i);
        convert.field(pRootJson, szKey, stInfo.OsdInfo[i].stOsdAttr.strToken);
    }

    /* byRes 为保留字段，不做转换 */

#undef NETSDK_CONVERT_ENUM_FIELD
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RtspUrlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.dwChannel);
    convert.field(pRootJson, "StreamIndex", stInfo.dwStreamIndex);
    convert.field(pRootJson, "RtspUrl", stInfo.szRtspUrl);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordFrameStreamCond_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "uSize", stInfo.uSize);
    convert.field(pRootJson, "channel", stInfo.uChannel);
    convert.field(pRootJson, "startTime", stInfo.szStartTime);
    convert.field(pRootJson, "endTime", stInfo.szEndTime);
    convert.field(pRootJson, "streamIndex", stInfo.uStreamIndex);
    convert.field(pRootJson, "mediaType", stInfo.uMediaType);
    convert.field(pRootJson, "codecType", stInfo.uCodecType);
    convert.field(pRootJson, "tcpPort", stInfo.uTcpPort);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordFrameStreamInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "uSize", stInfo.uSize);
    convert.field(pRootJson, "streamId", stInfo.szStreamId);
    convert.field(pRootJson, "channel", stInfo.uChannel);
    convert.field(pRootJson, "tcpPort", stInfo.uTcpPort);
    convert.field(pRootJson, "mediaType", stInfo.uMediaType);
    convert.field(pRootJson, "codecType", stInfo.uCodecType);
    convert.field(pRootJson, "width", stInfo.uWidth);
    convert.field(pRootJson, "height", stInfo.uHeight);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordFrameStopInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "uSize", stInfo.uSize);
    convert.field(pRootJson, "streamId", stInfo.szStreamId);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ReplayUrlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.dwChannel);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "Url", stInfo.szUrl);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ReplayCtrlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.dwChannel);
    convert.field(pRootJson, "CtrlType", stInfo.dwCtrlType);
    convert.field(pRootJson, "Speed", stInfo.fSpeed);
    convert.field(pRootJson, "SeekTime", stInfo.nSeekTime);
    convert.field(pRootJson, "ReplayType", stInfo.nReplayType);
    convert.field(pRootJson, "SessionId", stInfo.szSessionId);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "Url", stInfo.szUrl);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ReplayRecordTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartTime", stInfo.nStartTime);
    convert.field(pRootJson, "EndTime", stInfo.nEndTime);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ReplayRecordList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Channel", stInfo.dwChannel);
    convert.field(pRootJson, "FilterByEventType", stInfo.bFilterByEventType);
    convert.field(pRootJson, "EventType", stInfo.dwEventType);
    convert.field(pRootJson, "Date", stInfo.szDate);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "EndTime", stInfo.szEndTime);
    convert.field(pRootJson, "VideoCount", stInfo.nVideoCount);
    convert.field(pRootJson, "PersonEventCount", stInfo.nPersonEventCount);
    convert.field(pRootJson, "VehicleEventCount", stInfo.nVehicleEventCount);
    convert.field(pRootJson, "OtherEventCount", stInfo.nOtherEventCount);

    if (bOutStruct)
    {
        std::vector<NET_ReplayRecordTime_S> videoTimes;
        std::vector<NET_ReplayRecordTime_S> personEventTimes;
        std::vector<NET_ReplayRecordTime_S> vehicleEventTimes;
        std::vector<NET_ReplayRecordTime_S> otherEventTimes;

        convert.structure(pRootJson, "VideoTimes", videoTimes);
        convert.structure(pRootJson, "PersonEventTimes", personEventTimes);
        convert.structure(pRootJson, "VehicleEventTimes", vehicleEventTimes);
        convert.structure(pRootJson, "OtherEventTimes", otherEventTimes);

        stInfo.nVideoCount = (INT32)std::min<size_t>(videoTimes.size(), NET_TV_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nPersonEventCount = (INT32)std::min<size_t>(personEventTimes.size(), NET_TV_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nVehicleEventCount = (INT32)std::min<size_t>(vehicleEventTimes.size(), NET_TV_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nOtherEventCount = (INT32)std::min<size_t>(otherEventTimes.size(), NET_TV_REPLAY_RECORD_SEGMENT_MAX);

        for (INT32 i = 0; i < stInfo.nVideoCount; ++i)
        {
            stInfo.astVideoTimes[i] = videoTimes[(size_t)i];
        }
        for (INT32 i = 0; i < stInfo.nPersonEventCount; ++i)
        {
            stInfo.astPersonEventTimes[i] = personEventTimes[(size_t)i];
        }
        for (INT32 i = 0; i < stInfo.nVehicleEventCount; ++i)
        {
            stInfo.astVehicleEventTimes[i] = vehicleEventTimes[(size_t)i];
        }
        for (INT32 i = 0; i < stInfo.nOtherEventCount; ++i)
        {
            stInfo.astOtherEventTimes[i] = otherEventTimes[(size_t)i];
        }
    }
    else
    {
        if (stInfo.nVideoCount < 0)
        {
            stInfo.nVideoCount = 0;
        }
        if (stInfo.nPersonEventCount < 0)
        {
            stInfo.nPersonEventCount = 0;
        }
        if (stInfo.nVehicleEventCount < 0)
        {
            stInfo.nVehicleEventCount = 0;
        }
        if (stInfo.nOtherEventCount < 0)
        {
            stInfo.nOtherEventCount = 0;
        }

        stInfo.nVideoCount = std::min<INT32>(stInfo.nVideoCount, NET_TV_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nPersonEventCount = std::min<INT32>(stInfo.nPersonEventCount, NET_TV_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nVehicleEventCount = std::min<INT32>(stInfo.nVehicleEventCount, NET_TV_REPLAY_RECORD_SEGMENT_MAX);
        stInfo.nOtherEventCount = std::min<INT32>(stInfo.nOtherEventCount, NET_TV_REPLAY_RECORD_SEGMENT_MAX);

        std::vector<NET_ReplayRecordTime_S> videoTimes;
        std::vector<NET_ReplayRecordTime_S> personEventTimes;
        std::vector<NET_ReplayRecordTime_S> vehicleEventTimes;
        std::vector<NET_ReplayRecordTime_S> otherEventTimes;

        for (INT32 i = 0; i < stInfo.nVideoCount; ++i)
        {
            videoTimes.push_back(stInfo.astVideoTimes[i]);
        }
        for (INT32 i = 0; i < stInfo.nPersonEventCount; ++i)
        {
            personEventTimes.push_back(stInfo.astPersonEventTimes[i]);
        }
        for (INT32 i = 0; i < stInfo.nVehicleEventCount; ++i)
        {
            vehicleEventTimes.push_back(stInfo.astVehicleEventTimes[i]);
        }
        for (INT32 i = 0; i < stInfo.nOtherEventCount; ++i)
        {
            otherEventTimes.push_back(stInfo.astOtherEventTimes[i]);
        }

        convert.structure(pRootJson, "VideoTimes", videoTimes);
        convert.structure(pRootJson, "PersonEventTimes", personEventTimes);
        convert.structure(pRootJson, "VehicleEventTimes", vehicleEventTimes);
        convert.structure(pRootJson, "OtherEventTimes", otherEventTimes);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ChannelInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    int nSize = (int)stInfo.dwSize;
    convert.field(pRootJson, "Size", nSize);
    stInfo.dwSize = (UINT32)nSize;

    int nChannel = (int)stInfo.dwChannel;
    convert.field(pRootJson, "Channel", nChannel);
    stInfo.dwChannel = (UINT32)nChannel;

    int nEnable = (int)stInfo.byEnable;
    convert.field(pRootJson, "Enable", nEnable);
    stInfo.byEnable = (BYTE)nEnable;

    int nOnline = (int)stInfo.byOnline;
    convert.field(pRootJson, "Online", nOnline);
    stInfo.byOnline = (BYTE)nOnline;

    int nStreamType = (int)stInfo.byStreamType;
    convert.field(pRootJson, "StreamType", nStreamType);
    stInfo.byStreamType = (BYTE)nStreamType;

    int nHasRecord = (int)stInfo.byHasRecord;
    convert.field(pRootJson, "HasRecord", nHasRecord);
    stInfo.byHasRecord = (BYTE)nHasRecord;
    convert.field(pRootJson, "RecordStatus", stInfo.nRecordStatus);

    convert.field(pRootJson, "DevState", stInfo.nDevState);
    convert.field(pRootJson, "AppProto", stInfo.nAppProto);
    convert.field(pRootJson, "TransProto", stInfo.nTransProto);
    convert.field(pRootJson, "MfrsType", stInfo.nMfrsType);
    convert.field(pRootJson, "CtrlPort", stInfo.nCtrlPort);
    convert.field_array(pRootJson, "Reserved", stInfo.nReserved, 3, 3);

    convert.field(pRootJson, "ChannelName", stInfo.szChannelName);
    convert.field(pRootJson, "DevName", stInfo.szDevName);
    convert.field(pRootJson, "DevType", stInfo.szDevType);
    convert.field(pRootJson, "SerialNum", stInfo.szSerialNum);
    convert.field(pRootJson, "FirmwareVersion", stInfo.szFirmwareVersion);
    convert.field(pRootJson, "DeviceIP", stInfo.szDeviceIP);
    convert.field(pRootJson, "Mac", stInfo.szMac);
    convert.field(pRootJson, "SubnetMask", stInfo.szSubnetMask);
    convert.field(pRootJson, "MfrsName", stInfo.szMfrsName);
    convert.field(pRootJson, "AppProtoName", stInfo.szAppProtoName);
    convert.field(pRootJson, "OnvifDeviceUrl", stInfo.szOnvifDeviceUrl);
    convert.field(pRootJson, "PreviewMainUrl", stInfo.szPreviewMainUrl);
    convert.field(pRootJson, "PreviewSubUrl", stInfo.szPreviewSubUrl);
    convert.field(pRootJson, "RtspMainUrl", stInfo.szRtspMainUrl);
    convert.field(pRootJson, "RtspSubUrl", stInfo.szRtspSubUrl);

    if (bOutStruct)
    {
        if (stInfo.dwSize == 0)
        {
            stInfo.dwSize = sizeof(stInfo);
        }
        if (stInfo.szPreviewMainUrl[0] == '\0' && stInfo.szRtspMainUrl[0] != '\0')
        {
            std::strncpy(stInfo.szPreviewMainUrl, stInfo.szRtspMainUrl, sizeof(stInfo.szPreviewMainUrl) - 1);
            stInfo.szPreviewMainUrl[sizeof(stInfo.szPreviewMainUrl) - 1] = '\0';
        }
        if (stInfo.szPreviewSubUrl[0] == '\0' && stInfo.szRtspSubUrl[0] != '\0')
        {
            std::strncpy(stInfo.szPreviewSubUrl, stInfo.szRtspSubUrl, sizeof(stInfo.szPreviewSubUrl) - 1);
            stInfo.szPreviewSubUrl[sizeof(stInfo.szPreviewSubUrl) - 1] = '\0';
        }
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ChannelList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    int nSize = (int)stInfo.dwSize;
    convert.field(pRootJson, "Size", nSize);
    stInfo.dwSize = (UINT32)nSize;

    int nChannelCount = (int)stInfo.dwChannelCount;
    convert.field(pRootJson, "ChannelCount", nChannelCount);
    stInfo.dwChannelCount = (UINT32)nChannelCount;

    if (bOutStruct)
    {
        std::vector<NET_ChannelInfo_S> channels;
        convert.structure(pRootJson, "Channels", channels);
        stInfo.dwChannelCount = (UINT32)std::min<size_t>(channels.size(), NET_TV_MAX_CHANNEL_NUM);
        for (UINT32 i = 0; i < stInfo.dwChannelCount; ++i)
        {
            stInfo.stChannels[i] = channels[i];
        }
        if (stInfo.dwSize == 0)
        {
            stInfo.dwSize = sizeof(stInfo);
        }
    }
    else
    {
        std::vector<NET_ChannelInfo_S> channels;
        const UINT32 count = std::min<UINT32>(stInfo.dwChannelCount, NET_TV_MAX_CHANNEL_NUM);
        stInfo.dwChannelCount = count;
        for (UINT32 i = 0; i < count; ++i)
        {
            channels.push_back(stInfo.stChannels[i]);
        }
        convert.structure(pRootJson, "Channels", channels);
    }
}

/* ==================== Preview info ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_PreviewRtspUrl_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "RtspMainUrl", stInfo.szRtspMainUrl);
    convert.field(pRootJson, "RtspSubUrl", stInfo.szRtspSubUrl);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_PreviewImageParam_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Brightness", stInfo.nBrightness);
    convert.field(pRootJson, "Contrast", stInfo.nContrast);
    convert.field(pRootJson, "Saturation", stInfo.nSaturation);
    convert.field(pRootJson, "Sharpness", stInfo.nSharpness);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ImageSetting_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Brightness", stInfo.nBrightness);
    convert.field(pRootJson, "Contrast", stInfo.nContrast);
    convert.field(pRootJson, "Saturation", stInfo.nSaturation);
    convert.field(pRootJson, "Sharpness", stInfo.nSharpness);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_PreviewInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.structure(pRootJson, "RtspUrl", stInfo.stRtspUrl);
    convert.structure(pRootJson, "ImageParam", stInfo.stImageParam);
}
/* ==================== 布防时间和联动相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_SchedTime_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartHour", stInfo.nStartHour);
    convert.field(pRootJson, "StartMinute", stInfo.nStartMinute);
    convert.field(pRootJson, "EndHour", stInfo.nEndHour);
    convert.field(pRootJson, "EndMinute", stInfo.nEndMinute);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_AlarmSchedule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    /* 处理每天的时间段数量 */
    if (bOutStruct)
    {
        /* JSON -> 结构体 */
        Json::Object* pTimeSectionCount = Json::get(pRootJson, "TimeSectionCount");
        if (pTimeSectionCount)
        {
            for (int day = 0; day < 7; day++)
            {
                std::string key = std::to_string(day);
                int count = 0;
                Json::get(pTimeSectionCount, key, count);
                stInfo.uTimeSectionCount[day] = count;
            }
        }

        /* 处理每天的时间段数组 */
        Json::Object* pTimeSections = Json::get(pRootJson, "TimeSections");
        if (pTimeSections)
        {
            for (int day = 0; day < 7; day++)
            {
                std::string dayKey = std::to_string(day);
                Json::Object* pDaySections = Json::get(pTimeSections, dayKey);
                if (pDaySections)
                {
                    int count = stInfo.uTimeSectionCount[day];
                    if (count > NET_TV_PLAN_SECTION_NUM) count = NET_TV_PLAN_SECTION_NUM;

                    for (int i = 0; i < count; i++)
                    {
                        std::string idxKey = std::to_string(i);
                        Json::Object* pTimeItem = Json::get(pDaySections, idxKey);
                        if (pTimeItem)
                        {
                            deal(pTimeItem, stInfo.astTimeSection[day][i], bOutStruct);
                        }
                    }
                }
            }
        }
    }
    else
    {
        /* 结构体 -> JSON */
        Json::Object* pTimeSectionCount = Json::init();
        for (int day = 0; day < 7; day++)
        {
            Json::add(pTimeSectionCount, std::to_string(day).c_str(), stInfo.uTimeSectionCount[day]);
        }
        Json::add(pRootJson, "TimeSectionCount", pTimeSectionCount);

        Json::Object* pTimeSections = Json::init();
        for (int day = 0; day < 7; day++)
        {
            int count = stInfo.uTimeSectionCount[day];
            if (count > NET_TV_PLAN_SECTION_NUM) count = NET_TV_PLAN_SECTION_NUM;

            Json::Object* pDaySections = Json::init();
            for (int i = 0; i < count; i++)
            {
                Json::Object* pTimeItem = Json::init();
                deal(pTimeItem, stInfo.astTimeSection[day][i], bOutStruct);
                Json::add(pDaySections, std::to_string(i).c_str(), pTimeItem);
            }
            Json::add(pTimeSections, std::to_string(day).c_str(), pDaySections);
        }
        Json::add(pRootJson, "TimeSections", pTimeSections);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_LinkageList_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    if (bOutStruct)
    {
        /* JSON -> 结构体 */
        convert.field(pRootJson, "AlarmOutputCount", stInfo.uAlarmOutputCount);
        convert.field_array(pRootJson, "AlarmOutput", stInfo.auAlarmOutput,
                           stInfo.uAlarmOutputCount, NET_TV_MAX_ALARM_OUT_NUM);
        convert.field(pRootJson, "RecordChannelCount", stInfo.uRecordChannelCount);
        convert.field_array(pRootJson, "RecordChannel", stInfo.auRecordChannel,
                           stInfo.uRecordChannelCount, NET_TV_CHANNEL_MAX);
        convert.field(pRootJson, "SnapshotChannelCount", stInfo.uSnapshotChannelCount);
        convert.field_array(pRootJson, "SnapshotChannel", stInfo.auSnapshotChannel,
                           stInfo.uSnapshotChannelCount, NET_TV_CHANNEL_MAX);
    }
    else
    {
        /* 结构体 -> JSON */
        convert.field(pRootJson, "AlarmOutputCount", stInfo.uAlarmOutputCount);
        convert.field_array(pRootJson, "AlarmOutput", stInfo.auAlarmOutput,
                           stInfo.uAlarmOutputCount, NET_TV_MAX_ALARM_OUT_NUM);
        convert.field(pRootJson, "RecordChannelCount", stInfo.uRecordChannelCount);
        convert.field_array(pRootJson, "RecordChannel", stInfo.auRecordChannel,
                           stInfo.uRecordChannelCount, NET_TV_CHANNEL_MAX);
        convert.field(pRootJson, "SnapshotChannelCount", stInfo.uSnapshotChannelCount);
        convert.field_array(pRootJson, "SnapshotChannel", stInfo.auSnapshotChannel,
                           stInfo.uSnapshotChannelCount, NET_TV_CHANNEL_MAX);
    }
}

/* ==================== 移动侦测相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_MotionRegion_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AreaNo", stInfo.nAreaNo);
    convert.field(pRootJson, "RectLeft", stInfo.nRectLeft);
    convert.field(pRootJson, "RectTop", stInfo.nRectTop);
    convert.field(pRootJson, "RectRight", stInfo.nRectRight);
    convert.field(pRootJson, "RectBottom", stInfo.nRectBottom);
    convert.field(pRootJson, "CloseSensitivity", stInfo.nCloseSensitivity);
    convert.field(pRootJson, "DaytimeSensitivity", stInfo.nDaytimeSensitivity);
    convert.field(pRootJson, "NightSensitivity", stInfo.nNightSensitivity);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_MotionExpertMode_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ExpertDayNightCtrl", stInfo.nExpertDayNightCtrl);
    convert.structure(pRootJson, "DayTime", stInfo.stDayTime);
    convert.field(pRootJson, "RegionCount", stInfo.uRegionCount);

    /* 处理区域数组 */
    if (bOutStruct)
    {
        Json::Object* pRegions = Json::get(pRootJson, "Regions");
        if (pRegions)
        {
            int count = stInfo.uRegionCount;
            if (count > 16) count = 16;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRegion = Json::get(pRegions, key);
                if (pRegion)
                {
                    deal(pRegion, stInfo.astRegion[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRegions = Json::init();
        int count = stInfo.uRegionCount;
        if (count > 16) count = 16;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRegion = Json::init();
            deal(pRegion, stInfo.astRegion[i], bOutStruct);
            Json::add(pRegions, std::to_string(i).c_str(), pRegion);
        }
        Json::add(pRootJson, "Regions", pRegions);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_MotionNormalMode_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "RegionType", stInfo.nRegionType);
    convert.field(pRootJson, "RectLeft", stInfo.nRectLeft);
    convert.field(pRootJson, "RectTop", stInfo.nRectTop);
    convert.field(pRootJson, "RectRight", stInfo.nRectRight);
    convert.field(pRootJson, "RectBottom", stInfo.nRectBottom);
    convert.field(pRootJson, "GridWidth", stInfo.uGridWidth);
    convert.field(pRootJson, "GridHeight", stInfo.uGridHeight);

    /* 处理网格区域数组 */
    if (bOutStruct)
    {
        Json::Object* pGridArea = Json::get(pRootJson, "GridArea");
        if (pGridArea)
        {
            for (int i = 0; i < 18; i++)
            {
                std::string rowKey = std::to_string(i);
                Json::Object* pRow = Json::get(pGridArea, rowKey);
                if (pRow)
                {
                    for (int j = 0; j < 22; j++)
                    {
                        std::string colKey = std::to_string(j);
                        int val = 0;
                        Json::get(pRow, colKey, val);
                        stInfo.abyGridArea[i][j] = (BYTE)val;
                    }
                }
            }
        }
    }
    else
    {
        Json::Object* pGridArea = Json::init();
        for (int i = 0; i < 18; i++)
        {
            Json::Object* pRow = Json::init();
            for (int j = 0; j < 22; j++)
            {
                Json::add(pRow, std::to_string(j).c_str(), (int)stInfo.abyGridArea[i][j]);
            }
            Json::add(pGridArea, std::to_string(i).c_str(), pRow);
        }
        Json::add(pRootJson, "GridArea", pGridArea);
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_MotionAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "DynamicAnalysisEnable", stInfo.bDynamicAnalysisEnable);
    convert.field(pRootJson, "Mode", stInfo.uMode);
    convert.structure(pRootJson, "NormalMode", stInfo.stNormalMode);
    convert.structure(pRootJson, "ExpertMode", stInfo.stExpertMode);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}

/* ==================== 隐私遮盖配置相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_PrivacyMaskArea_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "AreaID", stInfo.nAreaID);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RectLeft", stInfo.nRectLeft);
    convert.field(pRootJson, "RectTop", stInfo.nRectTop);
    convert.field(pRootJson, "RectRight", stInfo.nRectRight);
    convert.field(pRootJson, "RectBottom", stInfo.nRectBottom);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_PrivacyMaskCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "AreaCount", stInfo.dwAreaCount);

    auto clamp_area_count = [](int nCount) -> int
    {
        if (nCount < 0)
        {
            return 0;
        }
        if (nCount > NET_TV_MAX_PRIVACY_MASK_AREA_NUM)
        {
            return NET_TV_MAX_PRIVACY_MASK_AREA_NUM;
        }
        return nCount;
    };

    /* 逐个转换遮盖区域 */
    if (bOutStruct)
    {
        /* Json -> Struct */
        Json::Object* pAreas = Json::get(pRootJson, "Areas");
        int nSize = pAreas ? Json::Array::size(pAreas) : 0;
        nSize = clamp_area_count(nSize);

        int nCount = clamp_area_count(stInfo.dwAreaCount);
        if (nCount == 0 || nCount > nSize)
        {
            nCount = nSize;
        }
        stInfo.dwAreaCount = nCount;

        for (int i = 0; i < nCount; i++)
        {
            Json::Object* pItem = Json::Array::get(pAreas, i);
            if (pItem)
            {
                deal(pItem, stInfo.astArea[i], bOutStruct);
            }
        }
    }
    else
    {
        /* Struct -> Json */
        Json::Object* pAreas = Json::Array::init();
        int nCount = clamp_area_count(stInfo.dwAreaCount);
        stInfo.dwAreaCount = nCount;
        for (int i = 0; i < nCount; i++)
        {
            Json::Object* pItem = Json::init();
            if (pItem)
            {
                deal(pItem, stInfo.astArea[i], bOutStruct);
                Json::Array::add(pAreas, pItem);
            }
        }
        Json::add(pRootJson, "Areas", pAreas);
    }
}

/* ==================== 遮挡报警相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_TamperAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.uSensitivity);
    convert.field(pRootJson, "RectLeft", stInfo.nRectLeft);
    convert.field(pRootJson, "RectTop", stInfo.nRectTop);
    convert.field(pRootJson, "RectRight", stInfo.nRectRight);
    convert.field(pRootJson, "RectBottom", stInfo.nRectBottom);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}

/* ==================== 越界检测相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_BoundaryPlane_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);

    /* 处理FLOAT字段 */
    if (bOutStruct)
    {
        double dVal = 0.0;
        if (Json::get(pRootJson, "StartPosX", dVal))
        {
            stInfo.fStartPosX = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "StartPosY", dVal))
        {
            stInfo.fStartPosY = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "EndPosX", dVal))
        {
            stInfo.fEndPosX = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "EndPosY", dVal))
        {
            stInfo.fEndPosY = (FLOAT)dVal;
        }
    }
    else
    {
        Json::add(pRootJson, "StartPosX", (double)stInfo.fStartPosX);
        Json::add(pRootJson, "StartPosY", (double)stInfo.fStartPosY);
        Json::add(pRootJson, "EndPosX", (double)stInfo.fEndPosX);
        Json::add(pRootJson, "EndPosY", (double)stInfo.fEndPosY);
    }

    convert.field(pRootJson, "CrossDirection", stInfo.enCrossDirection);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTargetCount", stInfo.uDetectionTargetCount);
    convert.field_array(pRootJson, "DetectionTarget", stInfo.auDetectionTarget,
                       stInfo.uDetectionTargetCount, 8);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_CrossLineAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    /* 处理规则数组 */
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

/* ==================== 入侵检测相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_IntrusionRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    /* 处理FLOAT数组 PointX */
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "PointX");
        if (pArray)
        {
            int nSize = Json::Array::size(pArray);
            for (int i = 0; i < nSize && i < 32; i++)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                if (pItem)
                {
                    double dVal = 0.0;
                    Json::Value::get(pItem, dVal);
                    stInfo.afPointX[i] = (FLOAT)dVal;
                }
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int count = stInfo.uPointCount;
        if (count > 32) count = 32;
        for (int i = 0; i < count; i++)
        {
            Json::Array::add(pArray, static_cast<float>(stInfo.afPointX[i]));
        }
        Json::add(pRootJson, "PointX", pArray);
    }

    /* 处理FLOAT数组 PointY */
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "PointY");
        if (pArray)
        {
            int nSize = Json::Array::size(pArray);
            for (int i = 0; i < nSize && i < 32; i++)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                if (pItem)
                {
                    double dVal = 0.0;
                    Json::Value::get(pItem, dVal);
                    stInfo.afPointY[i] = (FLOAT)dVal;
                }
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int count = stInfo.uPointCount;
        if (count > 32) count = 32;
        for (int i = 0; i < count; i++)
        {
            Json::Array::add(pArray, static_cast<float>(stInfo.afPointY[i]));
        }
        Json::add(pRootJson, "PointY", pArray);
    }

    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTargetCount", stInfo.uDetectionTargetCount);
    convert.field_array(pRootJson, "DetectionTarget", stInfo.auDetectionTarget,
                       stInfo.uDetectionTargetCount, 8);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_IntrusionAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    /* 处理规则数组 */
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

/* ==================== 徘徊侦测相关转换函数 ==================== */

void SDKConvert::deal(Json::Object* pRootJson, NET_LoiteringRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "PointCount", stInfo.uPointCount);

    /* 处理FLOAT数组 PointX */
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "PointX");
        if (pArray)
        {
            int nSize = Json::Array::size(pArray);
            for (int i = 0; i < nSize && i < 32; i++)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                if (pItem)
                {
                    double dVal = 0.0;
                    Json::Value::get(pItem, dVal);
                    stInfo.afPointX[i] = (FLOAT)dVal;
                }
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int count = stInfo.uPointCount;
        if (count > 32) count = 32;
        for (int i = 0; i < count; i++)
        {
            Json::Array::add(pArray, static_cast<float>(stInfo.afPointX[i]));
        }
        Json::add(pRootJson, "PointX", pArray);
    }

    /* 处理FLOAT数组 PointY */
    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "PointY");
        if (pArray)
        {
            int nSize = Json::Array::size(pArray);
            for (int i = 0; i < nSize && i < 32; i++)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                if (pItem)
                {
                    double dVal = 0.0;
                    Json::Value::get(pItem, dVal);
                    stInfo.afPointY[i] = (FLOAT)dVal;
                }
            }
        }
    }
    else
    {
        Json::Object* pArray = Json::Array::init();
        int count = stInfo.uPointCount;
        if (count > 32) count = 32;
        for (int i = 0; i < count; i++)
        {
            Json::Array::add(pArray, static_cast<float>(stInfo.afPointY[i]));
        }
        Json::add(pRootJson, "PointY", pArray);
    }

    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTargetCount", stInfo.uDetectionTargetCount);
    convert.field_array(pRootJson, "DetectionTarget", stInfo.auDetectionTarget,
                       stInfo.uDetectionTargetCount, 8);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_LoiteringAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "RuleCount", stInfo.uRuleCount);

    /* 处理规则数组 */
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_SceneChangeAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_CrowdGatheringRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
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

    convert.field(pRootJson, "ObjectOccup", stInfo.nObjectOccup);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_CrowdGatheringAlarmInfo_S& stInfo, bool bOutStruct)
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
                    deal(pRule, stInfo.astRule[i], bOutStruct);
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
            deal(pRule, stInfo.astRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ParkingRule_S& stInfo, bool bOutStruct)
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

    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ParkingAlarmInfo_S& stInfo, bool bOutStruct)
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
            if (count > 8) count = 8;
            for (int i = 0; i < count; i++)
            {
                std::string key = std::to_string(i);
                Json::Object* pRule = Json::get(pRules, key);
                if (pRule)
                {
                    deal(pRule, stInfo.astRule[i], bOutStruct);
                }
            }
        }
    }
    else
    {
        Json::Object* pRules = Json::init();
        int count = stInfo.uRuleCount;
        if (count > 8) count = 8;
        for (int i = 0; i < count; i++)
        {
            Json::Object* pRule = Json::init();
            deal(pRule, stInfo.astRule[i], bOutStruct);
            Json::add(pRules, std::to_string(i).c_str(), pRule);
        }
        Json::add(pRootJson, "Rules", pRules);
    }

    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_UnattendedObjectRule_S& stInfo, bool bOutStruct)
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

    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_UnattendedObjectAlarmInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ObjectRemovalRule_S& stInfo, bool bOutStruct)
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

    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ObjectRemovalAlarmInfo_S& stInfo, bool bOutStruct)
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

/* ===================== 垃圾暴露检测配置 ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_GarbageExposureRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
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
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_GarbageExposureCfg_S& stInfo, bool bOutStruct)
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

/* ===================== 垃圾满溢检测配置 ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_GarbageOverflowRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
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
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_GarbageOverflowCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
}

/* ===================== 单规则智能检测配置 ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_AiSimpleRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ManholeCoverAbnormalCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_SleepOnDutyCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ElectricVehicleInElevatorCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_PersonFallDownCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ConstructionOccupyRoadCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_CongestionCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_LicensePlateRecognitionCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_HighAltitudeSeatbeltCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_SafetyHelmetCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_PersonFallCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_PhoneUsageCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_SmokingCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_OpenFlameCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_BareSoilCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_HoleProtectionBarCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ReflectiveClothingCfg_S& stInfo, bool bOutStruct)
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
/* ===================== 智能事件配置 ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_SmartRegion_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_SmartRegionRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
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
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTargetCount", stInfo.uDetectionTargetCount);
    convert.field_array(pRootJson, "DetectionTarget", stInfo.auDetectionTarget, stInfo.uDetectionTargetCount, 8);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_SmartLineRule_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    if (bOutStruct)
    {
        double dVal = 0.0;
        if (Json::get(pRootJson, "StartPosX", dVal))
        {
            stInfo.fStartPosX = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "StartPosY", dVal))
        {
            stInfo.fStartPosY = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "EndPosX", dVal))
        {
            stInfo.fEndPosX = (FLOAT)dVal;
        }
        if (Json::get(pRootJson, "EndPosY", dVal))
        {
            stInfo.fEndPosY = (FLOAT)dVal;
        }
    }
    else
    {
        Json::add(pRootJson, "StartPosX", (double)stInfo.fStartPosX);
        Json::add(pRootJson, "StartPosY", (double)stInfo.fStartPosY);
        Json::add(pRootJson, "EndPosX", (double)stInfo.fEndPosX);
        Json::add(pRootJson, "EndPosY", (double)stInfo.fEndPosY);
    }
    convert.field(pRootJson, "CrossDirection", stInfo.enCrossDirection);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_PetRecognitionInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ClimbFenceInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_DimissionInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_IllegalLaneInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RetrogradeInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_NonmotorVehicleIntrusionInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_OccupationEmergencyInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_PedestrianIntrusionInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_SmokeFireCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_RoadPondingCfg_S& stInfo, bool bOutStruct)
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

/* ===================== Audio anomaly alarm config ============================== */
void SDKConvert::deal(Json::Object* pRootJson, NET_AudioAnomalyAlarmInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "AudioInputAnomaly", stInfo.bAudioInputAnomaly);
    convert.field(pRootJson, "UpEnable", stInfo.bUpEnable);
    convert.field(pRootJson, "UpSensitivity", stInfo.nUpSensitivity);
    convert.field(pRootJson, "UpThreshold", stInfo.nUpThreshold);
    convert.field(pRootJson, "DownEnable", stInfo.bDownEnable);
    convert.field(pRootJson, "DownSensitivity", stInfo.nDownSensitivity);
    convert.structure(pRootJson, "AlarmSchedule", stInfo.stAlarmSchedule);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_UpgradeInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "UpgradePath",            stInfo.szUpgradePath);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_UpgradeStatus_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "UpgradeStatus",            stInfo.nUpgradeStatus);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_UpgradeVersion_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Version",            stInfo.szVersion);
}

/* ===================== 告警抓图相关 ============================== */

void SDKConvert::deal(Json::Object *pRootJson, NET_CaptureTime_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "StartTime", stInfo.nStartTime);
    convert.field(pRootJson, "EndTime", stInfo.nEndTime);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object *pRootJson, NET_CaptureDaySchedule_S &stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object *pRootJson, NET_CapturePlanInfo_S &stInfo, bool bOutStruct)
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

        for (i = 0; i < NET_TV_PLAN_DAY_NUM_AWEEK; ++i)
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

        for (i = 0; i < NET_TV_PLAN_DAY_NUM_AWEEK; ++i)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object *pRootJson, NET_CaptureConfig_S &stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object *pRootJson, NET_CaptureParamInfo_S &stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object *pRootJson, NET_ExposureInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "ExpTime", stInfo.enExpTime);
    convert.field(pRootJson, "AntiBanding", stInfo.bAntiBanding);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object *pRootJson, NET_DayNightInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "DayNightMode", stInfo.enDayNightMode);
    convert.field(pRootJson, "BeginHour", stInfo.nBeginHour);
    convert.field(pRootJson, "BeginMinute", stInfo.nBeginMinute);
    convert.field(pRootJson, "BeginSecond", stInfo.nBeginSecond);
    convert.field(pRootJson, "BeginMilliSec", stInfo.nBeginMilliSec);
    convert.field(pRootJson, "EndHour", stInfo.nEndHour);
    convert.field(pRootJson, "EndMinute", stInfo.nEndMinute);
    convert.field(pRootJson, "EndSecond", stInfo.nEndSecond);
    convert.field(pRootJson, "EndMilliSec", stInfo.nEndMilliSec);
    convert.field(pRootJson, "SensitivityLevel", stInfo.nSensitivityLevel);
    convert.field(pRootJson, "FilterTime", stInfo.nFilterTime);
    convert.field(pRootJson, "FillLightExp", stInfo.bFillLightExp);
    convert.field(pRootJson, "LightMode", stInfo.enLightMode);
    convert.field(pRootJson, "LightType", stInfo.enLightType);
    convert.field(pRootJson, "WhiteLightEnable", stInfo.bWhiteLightEnable);
    convert.field(pRootJson, "WhiteLightLevel", stInfo.nWhiteLightLevel);
    convert.field(pRootJson, "RedLightEnable", stInfo.bRedLightEnable);
    convert.field(pRootJson, "RedLightLevel", stInfo.nRedLightLevel);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object *pRootJson, NET_BackLightInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "BackLightArea", stInfo.enBackLightArea);
    convert.field(pRootJson, "WdrEnable", stInfo.bWdrEnable);
    convert.field(pRootJson, "WdrLevel", stInfo.nWdrLevel);
    convert.field(pRootJson, "HlsEnable", stInfo.bHlsEnable);
    convert.field(pRootJson, "HlsLevel", stInfo.nHlsLevel);
    printf("\n CTest \n");
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object *pRootJson, NET_DenoiseInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "DnrMode", stInfo.enDnrMode);
    convert.field(pRootJson, "DnrLevel", stInfo.nDnrLevel);
    convert.field(pRootJson, "SnrLevel", stInfo.nSnrLevel);
    convert.field(pRootJson, "TnrLevel", stInfo.nTnrLevel);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object *pRootJson, NET_WhiteBalanceInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
        return;

    SDKConvert::CSDKConvert convert(bOutStruct);
    if (bOutStruct)
        std::memset(&stInfo, 0, sizeof(stInfo));
    convert.field(pRootJson, "AwbMode", stInfo.enAwbMode);
    convert.field(pRootJson, "RGain", stInfo.nRGain);
    convert.field(pRootJson, "BGain", stInfo.nBGain);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_TalkbackStateInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_TalkbackStreamInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_ReplayTalkbackInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_VoiceComAudioCfg_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_EnterRegionAlarmInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_LeaveRegionAlarmInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceCaptureRegion_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceCaptureRule_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceCaptureInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceCompareInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceLibInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceLibList_S& stInfo, bool bOutStruct)
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
        if (nSize > NET_TV_FACE_LIB_MAX_NUM)
        {
            nSize = NET_TV_FACE_LIB_MAX_NUM;
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
        if (nCount > NET_TV_FACE_LIB_MAX_NUM)
        {
            nCount = NET_TV_FACE_LIB_MAX_NUM;
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceIdInfo_S& stInfo, bool bOutStruct)
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
        if (nSize > NET_TV_FACE_ID_MAX_NUM)
        {
            nSize = NET_TV_FACE_ID_MAX_NUM;
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
        if (nCount > NET_TV_FACE_ID_MAX_NUM)
        {
            nCount = NET_TV_FACE_ID_MAX_NUM;
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceInfo_S& stInfo, bool bOutStruct)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内部处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

void SDKConvert::deal(Json::Object* pRootJson, NET_FaceInfoList_S& stInfo, bool bOutStruct)
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
        if (nSize > NET_TV_FACE_INFO_MAX_NUM)
        {
            nSize = NET_TV_FACE_INFO_MAX_NUM;
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
        if (nCount > NET_TV_FACE_INFO_MAX_NUM)
        {
            nCount = NET_TV_FACE_INFO_MAX_NUM;
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
void SDKConvert::deal(Json::Object* pRootJson, NET_PeopleFlowRuleLine_S& stInfo, bool bOutStruct)
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
void SDKConvert::deal(Json::Object* pRootJson, NET_PeopleAlarmRule_S& stInfo, bool bOutStruct)
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
void SDKConvert::deal(Json::Object* pRootJson, NET_PeopleAlarmConfig_S& stInfo, bool bOutStruct)
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
void SDKConvert::deal(Json::Object* pRootJson, NET_StatisticsResetConfig_S& stInfo, bool bOutStruct)
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
void SDKConvert::deal(Json::Object* pRootJson, NET_PeopleFlowStatisticsCfg_S& stInfo, bool bOutStruct)
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
void SDKConvert::deal(Json::Object* pRootJson, NET_PeopleDensityDetectionCfg_S& stInfo, bool bOutStruct)
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
