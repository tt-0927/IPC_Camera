/**
 * @file NetTVDeviceCbExecute.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVDeviceCbExecute 模块接口与类型定义
 * 功能说明：
 * 1. 声明 NetTVDeviceCbExecute 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_DEVICE_CALLBACK_EXECUTE_H
#define NETSDK_DEVICE_CALLBACK_EXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行获取设备信息回调
 * @details 调用宿主注册的设备信息获取回调，填充设备信息结构体
 * @param [out] pInfo 设备信息结构体指针，由回调函数填充
 * @return NET_TV_E_SUCCEED 成功，-1表示回调未注册，-2表示参数无效
 */
int NetSDK_ExecuteCb_DeviceInfo(pNET_DeviceInfo_S pInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行设备控制回调
 * @details 调用宿主注册的设备控制回调，执行设备控制操作（如重启、恢复出厂设置等）
 * @param [in] pstCtrlInfo 设备控制信息结构体，包含控制命令和参数
 * @return NET_TV_E_SUCCEED 成功，NET_TV_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int NetSDK_ExecuteCb_DeviceControl(pNET_DeviceControlInfo_S pstCtrlInfo);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行修改用户密码回调
 * @details 调用宿主注册的修改用户密码回调
 * @param [in] pPasswordInfo 修改密码信息结构体，包含用户名、旧密码、新密码
 * @return NET_TV_E_SUCCEED 成功，NET_TV_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int NetSDK_ExecuteCb_SetUserPassword(pNET_UserPasswordInfo_S pPasswordInfo);

#ifdef __cplusplus
}
#endif

#endif
