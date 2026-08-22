/*
 * @FilePath     : sdk_new/sdk_server/src/cb/BG6_ZHSJ/BU_SJGZ/NetTVIpcConfigCb.c
 * @Author       : ITC
 * @Date         : 2026-08-18
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-18
 * @Description  : IPC（摄像机）独有配置回调注册实现（BG6_ZHSJ/BU_SJGZ部门专用）
 *                 包含：
 *                 1. SD卡状态配置回调（IPC物理SD卡槽状态查询，NVR无此硬件）
 *                 2. Wifi无线配置回调（STA连接/断开/参数配置，IPC独有无线能力）
 *                 3. 4G蜂窝网络配置回调（4G模块信息查询与参数设置，IPC独有移动网络）
 *                 4. Hotspot热点配置回调（热点参数与连接状态，IPC独有AP能力）
 *                 5. 垃圾检测配置回调（垃圾暴露/满溢检测，IPC行业AI能力）
 *                 依赖：所有注册函数最终调用 registerGetCmdCb/registerSetCmdCb
 *                       （定义于Common/config/NetTVConfigCb.c，声明于NetTVConfigCbExecute.h）
 */

#include <stdio.h>
#include <stddef.h>
#include "NetTVIpcConfigCbExecute.h"
#include "NetTVConfigCbExecute.h"
#include "NetTVSDKServerInterface.h"

/* ===================== SD卡状态回调（IPC独有） ===================== */

/**
 * @brief 注册获取 SD 卡物理状态的回调函数
 * @param [in] pCb 用于填充 NET_SdCardStatus_S 输出缓冲区的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：摄像机本地SD卡槽的插入/挂载/容量状态查询
 */
NET_API BOOL STDCALL NET_serverRegisterGetSdCardStatusCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_SD_CARD_STATUS, pCb);
}

/* ===================== Wifi 无线网络回调（IPC独有） ===================== */

/**
 * @brief 注册设置 Wifi STA 参数配置的回调函数
 * @param [in] pCb 用于读取 NET_WifiStaCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：配置摄像机作为STA模式连接AP的SSID/密码等参数
 */
NET_API BOOL STDCALL NET_serverRegisterSetConfigWifiStaCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_CONFIG_WIFI_STA, pCb);
}

/**
 * @brief 注册连接 Wifi STA 的回调函数
 * @param [in] pCb 用于执行连接Wifi操作的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：触发摄像机发起Wifi连接请求
 */
NET_API BOOL STDCALL NET_serverRegisterConnectWifiStaCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_CONNECT_WIFI_STA, pCb);
}

/**
 * @brief 注册断开 Wifi STA 连接的回调函数
 * @param [in] pCb 用于执行断开Wifi连接的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：触发摄像机主动断开当前Wifi连接
 */
NET_API BOOL STDCALL NET_serverRegisterDisconnectWifiStaCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_DISCONNECT_WIFI_STA, pCb);
}

/* ===================== 4G 蜂窝网络回调（IPC独有） ===================== */

/**
 * @brief 注册获取 4G 模块信息的回调函数
 * @param [in] pCb 用于填充 NET_4GInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：查询4G模块的运营商/信号/IMEI/连接状态等信息
 */
NET_API BOOL STDCALL NET_serverRegisterGet4GInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_4G_INFO, pCb);
}

/**
 * @brief 注册设置 4G 模块参数的回调函数
 * @param [in] pCb 用于读取 NET_4GInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：配置4G模块的APN/拨号参数等
 */
NET_API BOOL STDCALL NET_serverRegisterSet4GInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_4G_INFO, pCb);
}

/* ===================== Hotspot 热点回调（IPC独有） ===================== */

/**
 * @brief 注册设置 Hotspot 热点参数的回调函数
 * @param [in] pCb 用于读取 NET_HotspotInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：配置摄像机作为AP热点的SSID/密码/信道等参数
 */
NET_API BOOL STDCALL NET_serverRegisterSetHotspotInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_HOTSPOT_INFO, pCb);
}

/**
 * @brief 注册获取 Hotspot 热点连接状态的回调函数
 * @param [in] pCb 用于填充 NET_HotspotConn_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：查询当前连接到摄像机热点的终端列表及状态
 */
NET_API BOOL STDCALL NET_serverRegisterGetHotspotConnCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_HOTSPOT_CONN, pCb);
}

/* ===================== 垃圾检测配置回调（IPC独有） ===================== */

/**
 * @brief 注册获取垃圾暴露检测配置的回调函数
 * @param [in] pCb 用于填充 NET_GarbageExposureCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：配置垃圾暴露检测的布防、检测区域和联动参数
 */
NET_API BOOL STDCALL NET_serverRegisterGetGarbageExposureConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_GARBAGE_EXPOSURE_CFG, pCb);
}

/**
 * @brief 注册设置垃圾暴露检测配置的回调函数
 * @param [in] pCb 用于接收 NET_GarbageExposureCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：配置垃圾暴露检测的布防、检测区域和联动参数
 */
NET_API BOOL STDCALL NET_serverRegisterSetGarbageExposureConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_GARBAGE_EXPOSURE_CFG, pCb);
}

/**
 * @brief 注册获取垃圾满溢检测配置的回调函数
 * @param [in] pCb 用于填充 NET_GarbageOverflowCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：配置垃圾满溢检测的布防、检测区域和联动参数
 */
NET_API BOOL STDCALL NET_serverRegisterGetGarbageOverflowConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_GARBAGE_OVERFLOW_CFG, pCb);
}

/**
 * @brief 注册设置垃圾满溢检测配置的回调函数
 * @param [in] pCb 用于接收 NET_GarbageOverflowCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 * @note IPC独有：配置垃圾满溢检测的布防、检测区域和联动参数
 */
NET_API BOOL STDCALL NET_serverRegisterSetGarbageOverflowConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_GARBAGE_OVERFLOW_CFG, pCb);
}
