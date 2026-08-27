/*
 * @FilePath     : sdk_new/sdk_server/src/cb/BG6_ZHSJ/BU_SJCL/NetTVNvrDeviceCbExecute.h
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : NVR独有设备级回调执行接口（BG6_ZHSJ/BU_SJCL部门专用）
 *                 声明 NVR 规模/能力数量信息（NET_DeviceInfo_S）的回调执行函数。
 *                 NET_DeviceInfo_S 为 NVR 侧专有（设备类型/报警端口数/通道数），
 *                 区别于通用设备基本信息 NET_DeviceBasicInfo_S（后者收口于 Common 设备回调）。
 */
#ifndef _NETTVNVRDEVICECBEXECUTE_H
#define _NETTVNVRDEVICECBEXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 执行获取设备信息回调（NVR规模/能力数量）
 * @details 调用宿主注册的设备信息获取回调，填充 NET_DeviceInfo_S
 *         （设备类型/报警输入端口数/报警输出端口数/通道数）。
 *         该结构体为 NVR 规模信息，归 BU_SJCL/NVR 侧，
 *         区别于通用设备基本信息 NET_DeviceBasicInfo_S。
 * @param [out] pInfo 设备信息结构体指针，由回调函数填充
 * @return NET_E_SUCCEED 成功，-1表示回调未注册，-2表示参数无效
 */
int executeGetDeviceInfoCb(pNET_DeviceInfo_S pInfo);

#ifdef __cplusplus
}
#endif

#endif /* _NETTVNVRDEVICECBEXECUTE_H */
