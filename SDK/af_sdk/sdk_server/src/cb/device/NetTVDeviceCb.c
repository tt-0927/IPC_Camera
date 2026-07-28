/**
 * @file NetTVDeviceCb.c
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVDeviceCb 模块实现
 * 功能说明：
 * 1. 实现 NetTVDeviceCb 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include<stdio.h>

#include "NetTVDeviceCbExecute.h"
#include "NetTVSDKServerInterface.h"

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设备通用回调枚举定义
 */
typedef enum
{
    NET_TV_CB_TYPE_DEVICE_INFO = 0,         /* 获取设备信息 */
    NET_TV_CB_TYPE_DEVICE_STATUS,
    NET_TV_CB_TYPE_DEVICE_CTRL,
    NET_TV_CB_TYPE_DEVICE_CONFIG,
    NET_TV_CB_TYPE_SET_USER_PASSWORD, /* 修改用户密码 */

    NET_TV_CB_TYPE_MAX
} Net_TV_DeviceCb_E;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设备通用回调函数联合体定义
 */
typedef union
{
    int (*DeviceInfo)(pNET_DeviceInfo_S pInfo);
    NET_TV_CB_DeviceControl DeviceControl;
    NET_TV_CB_SetUserPassword SetUserPassword;

} Net_TV_DeviceCb_Un;


typedef struct
{
    Net_TV_DeviceCb_E 	enType;      			/* 回调类型 */
    Net_TV_DeviceCb_Un 	unFunc;      			/* 回调函数指针（联合体） */
    int isRegistered;          					/* 注册标记：0=未注册，1=已注册 */
} NET_TV_Device_CbItem_S;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 全局回调注册表
 */
static NET_TV_Device_CbItem_S g_cbTable[NET_TV_CB_TYPE_MAX] = {0};

/* ========================== 注册接口实现 ========================== */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册获取设备信息回调
 * @details 客户端获取设备信息（型号、版本、序列号等）时调用此回调
 * @param [in] CB 设备信息获取回调函数指针
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败（参数为NULL或已注册）
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_GetDeviceInfo(NET_TV_COMMON_ECODE_E (*CB)(pNET_DeviceInfo_S pInfo))
{
    if (CB == NULL)
    {
        return NET_TV_FALSE;
    }

    NET_TV_Device_CbItem_S* pItem = &g_cbTable[NET_TV_CB_TYPE_DEVICE_INFO];
    if (pItem->isRegistered)
    {
        return NET_TV_FALSE;
    }

    pItem->enType = NET_TV_CB_TYPE_DEVICE_INFO;
    pItem->unFunc.DeviceInfo = CB;
    pItem->isRegistered = 1;

    return NET_TV_TRUE;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册设备控制回调
 * @details 客户端执行设备控制操作（如重启、恢复出厂设置等）时调用此回调
 * @param [in] pCb 设备控制回调函数指针
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败（参数为NULL或已注册）
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_DeviceControl(NET_TV_CB_DeviceControl pCb)
{
    if (pCb == NULL)
    {
        return NET_TV_FALSE;
    }

    NET_TV_Device_CbItem_S* pItem = &g_cbTable[NET_TV_CB_TYPE_DEVICE_CTRL];
    if (pItem->isRegistered)
    {
        return NET_TV_FALSE;
    }

    pItem->enType = NET_TV_CB_TYPE_DEVICE_CTRL;
    pItem->unFunc.DeviceControl = pCb;
    pItem->isRegistered = 1;

    return NET_TV_TRUE;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册修改用户密码回调
 * @details 客户端修改设备用户密码时调用此回调
 * @param [in] pCb 修改密码回调函数指针
 * @return NET_TV_TRUE 成功，NET_TV_FALSE 失败（参数为NULL或已注册）
 */
NET_TV_API BOOL NET_TV_STDCALL NET_TV_SERVER_RegisterCb_SetUserPassword(NET_TV_CB_SetUserPassword pCb)
{
    if (pCb == NULL)
    {
        return NET_TV_FALSE;
    }

    NET_TV_Device_CbItem_S* pItem = &g_cbTable[NET_TV_CB_TYPE_SET_USER_PASSWORD];
    if (pItem->isRegistered)
    {
        return NET_TV_FALSE;
    }

    pItem->enType = NET_TV_CB_TYPE_SET_USER_PASSWORD;
    pItem->unFunc.SetUserPassword = pCb;
    pItem->isRegistered = 1;

    return NET_TV_TRUE;
}

/* ========================== 执行接口实现 ========================== */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行获取设备信息回调
 * @details 查找并执行宿主注册的设备信息获取回调
 * @param [out] pInfo 设备信息结构体指针，由回调函数填充
 * @return NET_TV_E_SUCCEED 成功，-1表示回调未注册，-2表示参数无效
 */
int NetSDK_ExecuteCb_DeviceInfo(pNET_DeviceInfo_S pInfo)
{
    if (pInfo == NULL) return -2;
    NET_TV_Device_CbItem_S* pItem = &g_cbTable[NET_TV_CB_TYPE_DEVICE_INFO];
    if (!pItem->isRegistered) return -1;

    /* 执行对应回调（类型安全） */
    return pItem->unFunc.DeviceInfo(pInfo);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行设备控制回调
 * @details 查找并执行宿主注册的设备控制回调
 * @param [in] pstCtrlInfo 设备控制信息结构体，包含控制命令和参数
 * @return NET_TV_E_SUCCEED 成功，NET_TV_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int NetSDK_ExecuteCb_DeviceControl(pNET_DeviceControlInfo_S pstCtrlInfo)
{
    if (pstCtrlInfo == NULL) return NET_TV_E_INVALID_PARAM;

    NET_TV_Device_CbItem_S* pItem = &g_cbTable[NET_TV_CB_TYPE_DEVICE_CTRL];
    if (!pItem->isRegistered) return NET_TV_E_NOT_SUPPORT;

    return pItem->unFunc.DeviceControl(pstCtrlInfo);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行修改用户密码回调
 * @details 查找并执行宿主注册的修改用户密码回调
 * @param [in] pPasswordInfo 修改密码信息结构体，包含用户名、旧密码、新密码
 * @return NET_TV_E_SUCCEED 成功，NET_TV_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int NetSDK_ExecuteCb_SetUserPassword(pNET_UserPasswordInfo_S pPasswordInfo)
{
    if (pPasswordInfo == NULL) return NET_TV_E_INVALID_PARAM;

    NET_TV_Device_CbItem_S* pItem = &g_cbTable[NET_TV_CB_TYPE_SET_USER_PASSWORD];
    if (!pItem->isRegistered) return NET_TV_E_NOT_SUPPORT;

    return pItem->unFunc.SetUserPassword(pPasswordInfo);
}
