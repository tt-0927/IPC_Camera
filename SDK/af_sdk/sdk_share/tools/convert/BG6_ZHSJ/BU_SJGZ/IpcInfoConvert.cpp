/*
 * @FilePath     : sdk_new/sdk_share/tools/convert/BG6_ZHSJ/BU_SJGZ/IpcInfoConvert.cpp
 * @Author       : ITC
 * @Date         : 2026-08-21
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-21
 * @Description  : IPC 硬件专属配置转换
 *                 收口 SD卡/WiFi/4G/热点 等只有 IPC 才有的硬件能力结构体。
 */

#include "IpcInfoConvert.h"
#include "SDKConvert.h"
#include <algorithm>
#include <string>
#include <cstring>

namespace SDKConvert
{

void deal(Json::Object* pRootJson, NET_SdCardStatus_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Status", stInfo.nStatus);
    convert.field(pRootJson, "StatusText", stInfo.strStatusText);
    convert.field(pRootJson, "Ready", stInfo.bReady);
}


void deal(Json::Object* pRootJson, NET_WifiStaCfg_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "EnableWifi", stInfo.bEnableWifi);
    convert.field(pRootJson, "EnableBoost", stInfo.bEnableBoost);
}


void deal(Json::Object* pRootJson, NET_WifiWepKey_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Index", stInfo.nIndex);
    convert.field(pRootJson, "Value", stInfo.strValue);
}


void deal(Json::Object* pRootJson, NET_WifiStaConnect_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Ssid", stInfo.strSsid);
    convert.field(pRootJson, "SecurityMode", stInfo.nSecurityMode);
    convert.field(pRootJson, "IpAddress", stInfo.strIpAddress);
    convert.field(pRootJson, "Password", stInfo.strPassword);
    convert.field(pRootJson, "Pairwise", stInfo.strPairwise);
    convert.field(pRootJson, "WepKeyLen", stInfo.nWepKeyLen);
    convert.field(pRootJson, "WepIsHex", stInfo.bWepIsHex);
    convert.field(pRootJson, "AuthAlg", stInfo.strAuthAlg);
    convert.field(pRootJson, "WepKeyCount", stInfo.nWepKeyCount);

    convert.field(pRootJson, "EapIdentity", stInfo.strEapIdentity);
    convert.field(pRootJson, "EapPassword", stInfo.strEapPassword);
    convert.field(pRootJson, "PeapVersion", stInfo.strPeapVersion);
    convert.field(pRootJson, "Phase2", stInfo.strPhase2);
    convert.field(pRootJson, "AnonymousIdentity", stInfo.strAnonymousIdentity);
    convert.field(pRootJson, "CaCertPath", stInfo.strCaCertPath);
    convert.field(pRootJson, "PeapLabel", stInfo.strPeapLabel);

    convert.field(pRootJson, "TlsIdentity", stInfo.strTlsIdentity);
    convert.field(pRootJson, "PrivateKeyPasswd", stInfo.strPrivateKeyPasswd);
    convert.field(pRootJson, "EapolVersion", stInfo.strEapolVersion);
    convert.field(pRootJson, "ClientCertPath", stInfo.strClientCertPath);
    convert.field(pRootJson, "PrivateKeyPath", stInfo.strPrivateKeyPath);
    convert.field(pRootJson, "CtrlInterface", stInfo.strCtrlInterface);
    convert.field(pRootJson, "InterfaceName", stInfo.strInterfaceName);

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


void deal(Json::Object* pRootJson, NET_4GInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Apn", stInfo.strApn);
    convert.field(pRootJson, "UserName", stInfo.strUserName);
    convert.field(pRootJson, "Password", stInfo.strPassword);
    convert.field(pRootJson, "CallNumber", stInfo.strCallNumber);
    convert.field(pRootJson, "Mtu", stInfo.nMtu);
    convert.field(pRootJson, "AuthMode", stInfo.nAuthMode);
    convert.field(pRootJson, "NetworkMode", stInfo.nNetworkMode);
    convert.field(pRootJson, "DialMode", stInfo.nDialMode);
}


void deal(Json::Object* pRootJson, NET_HotspotInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Enabled", stInfo.bEnabled);
    convert.field(pRootJson, "Ssid", stInfo.strSsid);
    convert.field(pRootJson, "SecurityMode", stInfo.strSecurityMode);
    convert.field(pRootJson, "EncryptionType", stInfo.strEncryptionType);
    convert.field(pRootJson, "Password", stInfo.strPassword);
    convert.field(pRootJson, "ConfirmPassword", stInfo.strConfirmPassword);
}


void deal(Json::Object* pRootJson, NET_HotspotConnDevice_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Index", stInfo.nIndex);
    convert.field(pRootJson, "Mac", stInfo.strMac);
    convert.field(pRootJson, "Ip", stInfo.strIp);
    convert.field(pRootJson, "ConnTime", stInfo.strConnTime);
}


void deal(Json::Object* pRootJson, NET_HotspotConnInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Status", stInfo.strStatus);
    convert.field(pRootJson, "Total", stInfo.nTotal);

    if (!bOutStruct)
    {
        if (stInfo.nDeviceCount < 0)
        {
            stInfo.nDeviceCount = 0;
        }
        if (stInfo.nDeviceCount > NET_HOTSPOT_CONN_MAX_NUM)
        {
            stInfo.nDeviceCount = NET_HOTSPOT_CONN_MAX_NUM;
        }
    }

    convert.field(pRootJson, "DeviceCount", stInfo.nDeviceCount);

    if (bOutStruct)
    {
        Json::Object* pArray = Json::get(pRootJson, "Devices");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_HOTSPOT_CONN_MAX_NUM)
        {
            nCount = NET_HOTSPOT_CONN_MAX_NUM;
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

}
