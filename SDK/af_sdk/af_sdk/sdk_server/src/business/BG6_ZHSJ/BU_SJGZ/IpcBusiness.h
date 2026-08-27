/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJGZ/IpcBusiness.h
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : IPC（摄像机）独有配置业务处理（BG6_ZHSJ/BU_SJGZ部门专用）
 *                 负责处理IPC特有功能的业务请求，独立于Common通用配置：
 *                 - SD卡状态查询（GetSdCardStatus）
 *                 - WiFi配置/连接/断开（SetWifiStaCfg/ConnectWifiSta/DisconnectWifiSta）
 *                 - 4G模块查询/设置（Get4GInfo/Set4GInfo）
 *                 - 热点配置/连接状态查询（SetHotspotInfo/GetHotspotConn）
 *                 - OSD能力集查询（HandleOsd, NET_CAP_OSD）
 *                 这些IPC独有接口统一收口于此类，避免Common/DeviceConfigBusiness
 *                 随IPC功能扩展而膨胀。
 */

#pragma once

#include <string>
#include <cstring>

#include "Singleton.h"
#include "NetTVSDKServerInterface.h"
#include "NetTVConfigCbExecute.h"
#include "NetTVCapabilityCbExecute.h"
#include "BG6_ZHSJ/BU_SJGZ/CapabilityInfoConvert.h"
#include "BG6_ZHSJ/BU_SJGZ/IpcInfoConvert.h"
#include "SDKConvert.h"
#include "NetSdkLog.h"

/**
 * IPC独有配置业务处理类
 * @details 单例模式，负责处理IPC（摄像机）特有功能的配置获取和设置请求。
 *          与Common/DeviceConfigBusiness解耦，IPC独有接口统一在此扩展。
 */
class CIpcBusiness : public CSingleton<CIpcBusiness>
{
    CIpcBusiness() {}
public:
    ~CIpcBusiness() {}
    friend class CSingleton<CIpcBusiness>;

public:
    /* ===================== SD卡状态（IPC独有） ===================== */
    std::string HandleGetSdCardStatus(INT32 channelId, INT32 nCommand);

    /* ===================== WiFi无线网络（IPC独有） ===================== */
    std::string HandleSetWifiStaCfg(INT32 channelId, INT32 nCommand, const std::string& req_data);
    std::string HandleConnectWifiSta(INT32 channelId, INT32 nCommand, const std::string& req_data);
    std::string HandleDisconnectWifiSta(INT32 channelId, INT32 nCommand, const std::string& req_data);

    /* ===================== 4G蜂窝网络（IPC独有） ===================== */
    std::string HandleGet4GInfo(INT32 channelId, INT32 nCommand);
    std::string HandleSet4GInfo(INT32 channelId, INT32 nCommand, const std::string& req_data);

    /* ===================== Hotspot热点（IPC独有） ===================== */
    std::string HandleSetHotspotInfo(INT32 channelId, INT32 nCommand, const std::string& req_data);
    std::string HandleGetHotspotConn(INT32 channelId, INT32 nCommand);

    /* ===================== OSD能力集（IPC独有） ===================== */
    /**
     * @brief 处理OSD参数能力集 (NET_CAP_OSD)
     * @details IPC独有：查询摄像机OSD（字符叠加）能力集
     */
    std::string HandleOsd(INT32 channelId, INT32 nCommand);
};
