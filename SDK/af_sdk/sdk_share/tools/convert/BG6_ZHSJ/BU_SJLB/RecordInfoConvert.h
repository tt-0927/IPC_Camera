/**
 * @file RecordInfoConvert.h
 * @author ITC
 * @date 2026-08-20
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-20
 *
 * @brief 录播部门（BU_SJLB）专用结构体 JSON 转换声明
 * 功能说明：
 * 1. 声明录播部门专有结构体的 SDKConvert::deal 重载
 * 2. 为各配置域模板提供编译期可见的转换接口
 */

#ifndef _RECORDINFOCONVERT_H
#define _RECORDINFOCONVERT_H

#include <string>

#include "Json.h"

// 库通用头文件
#ifdef NET_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif

namespace SDKConvert
{
    void deal(Json::Object* pRootJson, NET_RegisterInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordControlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LiveStatusInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFileItem_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RecordFileInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_DirectorModeInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_CameraControlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PresetBitItem_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PresetBitInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PresetBitCtrl_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_ExternalControlInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LayoutRect_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_LayoutSelfInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_PVW2PGMInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AppointmentItem_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_AppointmentInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_RebootInfo_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_OutVolume_S& stInfo, bool bOutStruct);
    void deal(Json::Object* pRootJson, NET_SshSafeInfo_S& stInfo, bool bOutStruct);
};

#endif
