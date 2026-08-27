/*
 * @FilePath     : sdk_new/sdk_share/tools/convert/BG6_ZHSJ/BU_SJGZ/IpcInfoConvert.h
 * @Author       : ITC
 * @Date         : 2026-08-21
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-21
 * @Description  : IPC 硬件专属配置转换
 *                 收口 SD卡/WiFi/4G/热点 等只有 IPC 才有的硬件能力结构体。
 */
#pragma once

#include <string>
#include <vector>
#include <set>

#include "Json.h"

/* 库通用头文件 */
#ifdef NET_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif

namespace SDKConvert
{
    void deal(Json::Object* pRootJson, NET_SdCardStatus_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_WifiStaCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_WifiWepKey_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_WifiStaConnect_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_4GInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HotspotInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HotspotConnDevice_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_HotspotConnInfo_S& stInfo, bool bOutStruct);
}
