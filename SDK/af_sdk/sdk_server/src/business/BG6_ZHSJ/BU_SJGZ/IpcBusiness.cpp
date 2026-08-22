/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJGZ/IpcBusiness.cpp
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : IPC（摄像机）独有配置业务处理实现（BG6_ZHSJ/BU_SJGZ部门专用）
 */

#include "IpcBusiness.h"

/* ===================== SD卡状态（IPC独有） ===================== */

/**
 * @brief 获取SD卡物理状态
 * @details IPC独有：摄像机本地SD卡槽的插入/挂载/容量状态查询
 */
std::string CIpcBusiness::HandleGetSdCardStatus(INT32 nChannelId, INT32 nCommand)
{
    NET_SdCardStatus_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    NETSDK_LOG_MESSAGE_INFO("GetSdCardStatus callback START");
    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetSdCardStatus callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetSdCardStatus callback cmd=%d, ret=%d", nCommand, nRespCode);
    NETSDK_LOG_MESSAGE_INFO("GetSdCardStatus callback END");
    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}

/* ===================== WiFi无线网络（IPC独有） ===================== */

/**
 * @brief 设置Wifi STA参数配置
 * @details IPC独有：配置摄像机作为STA模式连接AP的SSID/密码等参数
 */
std::string CIpcBusiness::HandleSetWifiStaCfg(INT32 nChannelId, INT32 nCommand, const std::string& req_data)
{
    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    NET_WifiStaCfg_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    SDKConvert::deal(pRoot, stCfg, true);
    Json::deinit(pRoot);

    int nRespCode = executeSetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("SetWifiStaCfg callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }

    return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
}

/**
 * @brief 连接Wifi STA
 * @details IPC独有：触发摄像机发起Wifi连接请求
 */
std::string CIpcBusiness::HandleConnectWifiSta(INT32 nChannelId, INT32 nCommand, const std::string& req_data)
{
    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    NET_WifiStaConnect_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    SDKConvert::deal(pRoot, stCfg, true);
    Json::deinit(pRoot);

    int nRespCode = executeSetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("ConnectWifiSta callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }

    return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
}

/**
 * @brief 断开Wifi STA连接
 * @details IPC独有：触发摄像机主动断开当前Wifi连接
 */
std::string CIpcBusiness::HandleDisconnectWifiSta(INT32 nChannelId, INT32 nCommand, const std::string& req_data)
{
    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    NET_WifiStaConnect_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    SDKConvert::deal(pRoot, stCfg, true);
    Json::deinit(pRoot);

    int nRespCode = executeSetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("DisconnectWifiSta callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }

    return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
}

/* ===================== 4G蜂窝网络（IPC独有） ===================== */

/**
 * @brief 获取4G模块信息
 * @details IPC独有：查询4G模块的运营商/信号/IMEI/连接状态等信息
 */
std::string CIpcBusiness::HandleGet4GInfo(INT32 nChannelId, INT32 nCommand)
{
    NET_4GInfo_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    NETSDK_LOG_MESSAGE_INFO("Get4GInfo callback START");
    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("Get4GInfo callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("Get4GInfo callback cmd=%d, ret=%d", nCommand, nRespCode);
    NETSDK_LOG_MESSAGE_INFO("Get4GInfo callback END");
    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}

/**
 * @brief 设置4G模块参数
 * @details IPC独有：配置4G模块的APN/拨号参数等
 */
std::string CIpcBusiness::HandleSet4GInfo(INT32 nChannelId, INT32 nCommand, const std::string& req_data)
{
    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    NET_4GInfo_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    SDKConvert::deal(pRoot, stCfg, true);
    Json::deinit(pRoot);

    int nRespCode = executeSetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("Set4GInfo callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }

    return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
}

/* ===================== Hotspot热点（IPC独有） ===================== */

/**
 * @brief 设置Hotspot热点参数
 * @details IPC独有：配置摄像机作为AP热点的SSID/密码/信道等参数
 */
std::string CIpcBusiness::HandleSetHotspotInfo(INT32 nChannelId, INT32 nCommand, const std::string& req_data)
{
    if (req_data.empty())
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    NET_HotspotInfo_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    Json::Object* pRoot = Json::init(req_data);
    if (!pRoot)
    {
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    SDKConvert::deal(pRoot, stCfg, true);
    Json::deinit(pRoot);

    int nRespCode = executeSetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("SetHotspotInfo callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }

    return SDKConvert::to_respString((NET_COMMON_ECODE_E)nRespCode, nCommand);
}

/**
 * @brief 获取Hotspot热点连接状态
 * @details IPC独有：查询当前连接到摄像机热点的终端列表及状态
 */
std::string CIpcBusiness::HandleGetHotspotConn(INT32 nChannelId, INT32 nCommand)
{
    NET_HotspotConnInfo_S stCfg;
    memset(&stCfg, 0, sizeof(stCfg));

    NETSDK_LOG_MESSAGE_INFO("GetHotspotConn callback START");
    int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_WARN("GetHotspotConn callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
    }
    NETSDK_LOG_MESSAGE_INFO("GetHotspotConn callback cmd=%d, ret=%d", nCommand, nRespCode);
    NETSDK_LOG_MESSAGE_INFO("GetHotspotConn callback END");
    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
}

/* ===================== OSD能力集（IPC独有） ===================== */

/**
 * @brief 处理OSD参数能力集 (NET_CAP_OSD)
 * @details IPC独有：查询摄像机OSD（字符叠加）能力集
 */
std::string CIpcBusiness::HandleOsd(INT32 nChannelId, INT32 nCommand)
{
    if (nChannelId < 0)
    {
        NETSDK_LOG_MESSAGE_WARN("HandleOsd: invalid channel=%d", nChannelId);
        return SDKConvert::to_respString(NET_E_INVALID_PARAM, nCommand);
    }

    int nRespCode = NET_E_FAILED;
    NET_OsdCap_S stCap;
    memset(&stCap, 0, sizeof(NET_OsdCap_S));

    nRespCode = executeGetOsdCapCb(nChannelId, &stCap);
    if (nRespCode != NET_E_SUCCEED)
    {
        NETSDK_LOG_MESSAGE_DEBUG("OSD能力集回调执行失败! ret=%d", nRespCode);
    }

    return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCap);
}
