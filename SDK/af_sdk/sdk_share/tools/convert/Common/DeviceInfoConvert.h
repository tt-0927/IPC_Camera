/**
 * @file DeviceInfoConvert.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DeviceInfoConvert 模块接口与类型定义
 * 功能说明：
 * 1. 声明 DeviceInfoConvert 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_DEVICE_INFO_CONVERT_H
#define NETSDK_DEVICE_INFO_CONVERT_H

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
    void deal(Json::Object* pRootJson, NET_AudioCfg_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_DeviceInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_DeviceBasicInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_DeviceStorageInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SystemNtpInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SystemTime_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UserPasswordInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_NetworkCfg_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_NetworkCfgList_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PageInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LoginLockInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PwdPolicyInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SshAdminInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SecurityServicesInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SshCountdownInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LogServerInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LogRetrievalCond_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LogInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LogList_S& stInfo, bool bOutStruct);

    void deal(Json::Object* pRootJson, NET_DeviceControlInfo_S& stInfo, bool bOutStruct);

    void deal(Json::Object*& pRootJson, tagNET_UpgradeInfo* stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UpgradeInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UpgradeStatus_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_UpgradeVersion_S& stInfo, bool bOutStruct);

};

#endif
