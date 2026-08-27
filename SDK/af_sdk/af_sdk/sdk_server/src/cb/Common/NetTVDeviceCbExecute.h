/**
 * @file NetTVDeviceCbExecute.h
 * @author chenchl (chenchl@kfb.cn)
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
 * @author chenchl (chenchl@kfb.cn)
 * @brief 执行获取设备基本信息回调 (NET_GET_DEVICECFG)
 * @details 调用宿主注册的设备基本信息获取回调，填充通用设备身份信息结构体
 *         （型号/序列号/固件/MAC等）。NET_DeviceBasicInfo_S 为通用身份属性，
 *         收口于通用设备回调，NET_GET_DEVICECFG 内部改调本接口。
 * @param [out] pInfo 设备基本信息结构体指针，由回调函数填充
 * @return NET_E_SUCCEED 成功，-1表示回调未注册，-2表示参数无效
 */
int executeGetDeviceBasicInfoCb(pNET_DeviceBasicInfo_S pInfo);

/**
 * @author chenchl (chenchl@kfb.cn)
 * @brief 执行设置设备基本信息回调 (NET_SET_DEVICECFG)
 * @details 调用宿主注册的设备基本信息设置回调。仅设备名 strDeviceName 可写，
 *         其余身份字段(序列号/固件/MAC/型号/厂商)只读，宿主回调应仅应用 strDeviceName。
 * @param [in] pInfo 设备基本信息结构体指针，含待设置字段
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int executeSetDeviceBasicInfoCb(pNET_DeviceBasicInfo_S pInfo);

/**
 * @brief 执行获取设备存储信息回调 (NET_GET_STORAGE_INFO)
 * @details 调用宿主注册的设备存储信息回调，填充硬盘状态/容量信息。
 *          设备无存储能力（未注册回调）时返回 NET_E_NOT_SUPPORT。
 * @param [out] pInfo 设备存储信息结构体指针，由回调函数填充
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT表示回调未注册(设备无存储)，NET_E_INVALID_PARAM参数无效
 */
int executeGetDeviceStorageInfoCb(pNET_DeviceStorageInfo_S pInfo);

/**
 * @author chenchl (chenchl@kfb.cn)
 * @brief 执行设备控制回调
 * @details 调用宿主注册的设备控制回调，执行设备控制操作（如重启、恢复出厂设置等）
 * @param [in] pstCtrlInfo 设备控制信息结构体，包含控制命令和参数
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int executeDeviceControlCb(pNET_DeviceControlInfo_S pstCtrlInfo);

/**
 * @author chenchl (chenchl@kfb.cn)
 * @brief 执行修改用户密码回调
 * @details 调用宿主注册的修改用户密码回调
 * @param [in] pPasswordInfo 修改密码信息结构体，包含用户名、旧密码、新密码
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int executeSetUserPasswordCb(pNET_UserPasswordInfo_S pPasswordInfo);

#ifdef __cplusplus
}
#endif

#endif
