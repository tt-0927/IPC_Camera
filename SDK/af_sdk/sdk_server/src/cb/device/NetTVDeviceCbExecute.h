/**
 * @file NetTVDeviceCbExecute.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-04
 * @brief 设备通用回调执行函数声明
 * @details 本文件声明设备通用回调的执行函数，供业务层调用，内部会查找并执行宿主注册的回调
 */
#ifndef _NETSDKDEVICECBEXECUTE_H
#define _NETSDKDEVICECBEXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 执行获取设备信息回调
 * @details 调用宿主注册的设备信息获取回调，填充设备信息结构体
 * @param [OUT] pInfo 设备信息结构体指针，由回调函数填充
 * @return NET_TV_E_SUCCEED 成功，-1表示回调未注册，-2表示参数无效
 */
int NetSDK_ExecuteCb_DeviceInfo(pNET_DeviceInfo_S pInfo);

/**
 * @brief 执行设备控制回调
 * @details 调用宿主注册的设备控制回调，执行设备控制操作（如重启、恢复出厂设置等）
 * @param [IN] pstCtrlInfo 设备控制信息结构体，包含控制命令和参数
 * @return NET_TV_E_SUCCEED 成功，NET_TV_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int NetSDK_ExecuteCb_DeviceControl(pNET_DeviceControlInfo_S pstCtrlInfo);

/**
 * @brief 执行修改用户密码回调
 * @details 调用宿主注册的修改用户密码回调
 * @param [IN] pPasswordInfo 修改密码信息结构体，包含用户名、旧密码、新密码
 * @return NET_TV_E_SUCCEED 成功，NET_TV_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int NetSDK_ExecuteCb_SetUserPassword(pNET_UserPasswordInfo_S pPasswordInfo);

#ifdef __cplusplus
}
#endif

#endif
