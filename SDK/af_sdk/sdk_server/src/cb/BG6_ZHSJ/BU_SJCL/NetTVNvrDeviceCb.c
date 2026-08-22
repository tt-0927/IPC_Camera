/*
 * @FilePath     : sdk_new/sdk_server/src/cb/BG6_ZHSJ/BU_SJCL/NetTVNvrDeviceCb.c
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : NVR独有设备级回调注册与执行实现（BG6_ZHSJ/BU_SJCL部门专用）
 *                 收口 NVR 规模/能力数量信息（NET_DeviceInfo_S：设备类型/报警端口数/通道数）。
 *                 从 Common/NetTVDeviceCb.c 迁出，避免 NVR 专有回调混入通用设备回调表，
 *                 遵循「通用的放通用，独有的分好类」原则。
 *                 通用设备基本信息 NET_DeviceBasicInfo_S 仍收口于 Common/NetTVDeviceCb.c。
 */

#include <stdio.h>
#include <stddef.h>
#include "NetTVNvrDeviceCbExecute.h"
#include "NetTVSDKServerInterface.h"

/**
 * @brief NVR 设备信息回调表（独立于 Common 通用设备回调表）
 */
typedef struct tagNETTVNvrDeviceCbTable
{
    NET_COMMON_ECODE_E (*cbGetDeviceInfo)(pNET_DeviceInfo_S pInfo);  /* 获取设备规模信息回调 */
} NET_NVR_DEVICE_CB_TABLE_S;

static NET_NVR_DEVICE_CB_TABLE_S g_stNvrDeviceCbTable = {0};  /* NVR 设备信息回调表实例 */

/* ===================== 设备规模信息（NVR独有） ===================== */

/**
 * @brief 注册获取设备信息回调
 * @details 客户端获取设备规模信息（设备类型/报警输入端口数/报警输出端口数/通道数）时调用此回调。
 *          NET_DeviceInfo_S 为 NVR 规模信息，归 NVR 侧，
 *          区别于通用设备基本信息 NET_DeviceBasicInfo_S（后者收口于 Common 设备回调）。
 * @param [in] CB 设备信息获取回调函数指针
 * @return TRUE 注册成功；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceInfoCb(NET_COMMON_ECODE_E (*CB)(pNET_DeviceInfo_S pInfo))
{
    if (CB == NULL)
    {
        return FALSE;
    }

    if (g_stNvrDeviceCbTable.cbGetDeviceInfo != NULL)
    {
        return FALSE;
    }

    g_stNvrDeviceCbTable.cbGetDeviceInfo = CB;
    return TRUE;
}

/**
 * @brief 执行获取设备信息回调
 * @details 查找并执行宿主注册的设备信息获取回调
 * @param [out] pInfo 设备信息结构体指针，由回调函数填充
 * @return NET_E_SUCCEED 成功，-1表示回调未注册，-2表示参数无效
 */
int executeGetDeviceInfoCb(pNET_DeviceInfo_S pInfo)
{
    if (pInfo == NULL) return -2;
    if (g_stNvrDeviceCbTable.cbGetDeviceInfo == NULL) return -1;

    return g_stNvrDeviceCbTable.cbGetDeviceInfo(pInfo);
}
