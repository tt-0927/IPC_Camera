/**
 * @file NetTVDeviceCb.c
 * @author chenchl (chenchl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 *
 * @brief 通用设备回调实现（Common，跨设备通用）
 * 收口内容：
 * 1. 设备基本信息 Get/Set（NET_DeviceBasicInfo_S：型号/序列号/固件/MAC/名称/厂商）
 *    —— 通用身份属性，对应 NET_GET/SET_DEVICECFG，命令码不变，内部改走专用回调
 * 2. 设备控制（重启/恢复出厂等）
 * 3. 修改用户密码
 * 说明：NET_DeviceInfo_S（设备类型/报警端口数/通道数）为 NVR 规模信息，
 *       已迁至 BU_SJCL/NetTVNvrDeviceCb.c，不再混入通用回调表。
 */
#include<stdio.h>

#include "NetTVDeviceCbExecute.h"
#include "NetTVSDKServerInterface.h"

/**
 * @author chenchl (chenchl@kfb.cn)
 * @brief 通用设备回调枚举定义
 */
typedef enum
{
    NET_CB_TYPE_DEVICE_BASIC_INFO = 0,   /* 获取设备基本信息(通用身份:型号/序列号/固件/MAC等) */
    NET_CB_TYPE_SET_DEVICE_BASIC_INFO,  /* 设置设备基本信息(仅strDeviceName可写,其余只读) */
    NET_CB_TYPE_DEVICE_STORAGE_INFO,    /* 获取设备存储信息(NVR/录播等有硬盘的设备专用) */
    NET_CB_TYPE_DEVICE_CTRL,            /* 设备控制(重启/恢复出厂等) */
    NET_CB_TYPE_SET_USER_PASSWORD,      /* 修改用户密码 */

    NET_CB_TYPE_MAX
} Net_DeviceCb_E;

/**
 * @author chenchl (chenchl@kfb.cn)
 * @brief 通用设备回调函数联合体定义
 */
typedef union
{
    NET_CB_GetDeviceBasicInfo    DeviceBasicInfo;    /* 获取设备基本信息 */
    NET_CB_SetDeviceBasicInfo    SetDeviceBasicInfo; /* 设置设备基本信息 */
    NET_CB_GetDeviceStorageInfo  DeviceStorageInfo;  /* 获取设备存储信息 */
    NET_CB_DeviceControl         DeviceControl;
    NET_CB_SetUserPassword       SetUserPassword;

} Net_DeviceCb_Un;


typedef struct
{
    Net_DeviceCb_E 	enType;      			/* 回调类型 */
    Net_DeviceCb_Un 	unFunc;      			/* 回调函数指针（联合体） */
    int isRegistered;          					/* 注册标记：0=未注册，1=已注册 */
} NET_Device_CbItem_S;

/**
 * @author chenchl (chenchl@kfb.cn)
 * @brief 全局回调注册表
 */
static NET_Device_CbItem_S g_cbTable[NET_CB_TYPE_MAX] = {0};

/* ========================== 注册接口实现 ========================== */

/**
 * @brief 注册获取设备基本信息回调 (NET_GET_DEVICECFG)
 * @details 客户端获取设备基本信息（型号、序列号、固件、MAC等）时调用此回调
 * @param [in] pCb 设备基本信息获取回调函数指针
 * @return NET_TRUE 成功，NET_FALSE 失败（参数为NULL或已注册）
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceBasicInfoCb(NET_CB_GetDeviceBasicInfo pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_DEVICE_BASIC_INFO];
    if (pItem->isRegistered)
    {
        return NET_FALSE;
    }

    pItem->enType = NET_CB_TYPE_DEVICE_BASIC_INFO;
    pItem->unFunc.DeviceBasicInfo = pCb;
    pItem->isRegistered = 1;

    return NET_TRUE;
}

/**
 * @brief 注册设置设备基本信息回调 (NET_SET_DEVICECFG)
 * @details 客户端设置设备基本信息时调用此回调。仅设备名 strDeviceName 可写，
 *          其余身份字段(序列号/固件/MAC/型号/厂商)只读，宿主回调应仅应用 strDeviceName。
 * @param [in] pCb 设备基本信息设置回调函数指针
 * @return NET_TRUE 成功，NET_FALSE 失败（参数为NULL或已注册）
 */
NET_API BOOL STDCALL NET_serverRegisterSetDeviceBasicInfoCb(NET_CB_SetDeviceBasicInfo pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_SET_DEVICE_BASIC_INFO];
    if (pItem->isRegistered)
    {
        return NET_FALSE;
    }

    pItem->enType = NET_CB_TYPE_SET_DEVICE_BASIC_INFO;
    pItem->unFunc.SetDeviceBasicInfo = pCb;
    pItem->isRegistered = 1;

    return NET_TRUE;
}

/**
 * @brief 注册获取设备存储信息回调
 * @details 只有具备存储能力的设备（NVR/录播）才需要注册此回调。
 *          编码器/矩阵等无存储设备不注册即可，SDK 执行时返回 NET_E_NOT_SUPPORT。
 * @param [in] pCb 设备存储信息回调函数指针
 * @return NET_TRUE 成功，NET_FALSE 失败（参数为NULL或已注册）
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceStorageInfoCb(NET_CB_GetDeviceStorageInfo pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_DEVICE_STORAGE_INFO];
    if (pItem->isRegistered)
    {
        return NET_FALSE;
    }

    pItem->enType = NET_CB_TYPE_DEVICE_STORAGE_INFO;
    pItem->unFunc.DeviceStorageInfo = pCb;
    pItem->isRegistered = 1;

    return NET_TRUE;
}

/**
 * @brief 注册设备控制回调
 * @details 客户端执行设备控制操作（如重启、恢复出厂设置等）时调用此回调
 * @param [in] pCb 设备控制回调函数指针
 * @return NET_TRUE 成功，NET_FALSE 失败（参数为NULL或已注册）
 */
NET_API BOOL STDCALL NET_serverRegisterDeviceControlCb(NET_CB_DeviceControl pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_DEVICE_CTRL];
    if (pItem->isRegistered)
    {
        return NET_FALSE;
    }

    pItem->enType = NET_CB_TYPE_DEVICE_CTRL;
    pItem->unFunc.DeviceControl = pCb;
    pItem->isRegistered = 1;

    return NET_TRUE;
}

/**
 * @brief 注册修改用户密码回调
 * @details 客户端修改设备用户密码时调用此回调
 * @param [in] pCb 修改密码回调函数指针
 * @return NET_TRUE 成功，NET_FALSE 失败（参数为NULL或已注册）
 */
NET_API BOOL STDCALL NET_serverRegisterSetUserPasswordCb(NET_CB_SetUserPassword pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_SET_USER_PASSWORD];
    if (pItem->isRegistered)
    {
        return NET_FALSE;
    }

    pItem->enType = NET_CB_TYPE_SET_USER_PASSWORD;
    pItem->unFunc.SetUserPassword = pCb;
    pItem->isRegistered = 1;

    return NET_TRUE;
}

/* ========================== 执行接口实现 ========================== */

/**
 * @brief 执行获取设备基本信息回调
 * @details 查找并执行宿主注册的设备基本信息获取回调
 * @param [out] pInfo 设备基本信息结构体指针，由回调函数填充
 * @return NET_E_SUCCEED 成功，-1表示回调未注册，-2表示参数无效
 */
int executeGetDeviceBasicInfoCb(pNET_DeviceBasicInfo_S pInfo)
{
    if (pInfo == NULL) return -2;
    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_DEVICE_BASIC_INFO];
    if (!pItem->isRegistered) return -1;

    /* 执行对应回调（类型安全） */
    return pItem->unFunc.DeviceBasicInfo(pInfo);
}

/**
 * @brief 执行设置设备基本信息回调
 * @details 查找并执行宿主注册的设备基本信息设置回调
 * @param [in] pInfo 设备基本信息结构体指针，含待设置字段
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int executeSetDeviceBasicInfoCb(pNET_DeviceBasicInfo_S pInfo)
{
    if (pInfo == NULL) return NET_E_INVALID_PARAM;

    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_SET_DEVICE_BASIC_INFO];
    if (!pItem->isRegistered) return NET_E_NOT_SUPPORT;

    return pItem->unFunc.SetDeviceBasicInfo(pInfo);
}

/**
 * @brief 执行获取设备存储信息回调
 * @details 查找并执行宿主注册的设备存储信息获取回调。
 *          设备无存储能力（未注册回调）时返回 NET_E_NOT_SUPPORT。
 * @param [out] pInfo 设备存储信息结构体指针，由回调函数填充
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT表示回调未注册(设备无存储)，NET_E_INVALID_PARAM参数无效
 */
int executeGetDeviceStorageInfoCb(pNET_DeviceStorageInfo_S pInfo)
{
    if (pInfo == NULL) return NET_E_INVALID_PARAM;

    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_DEVICE_STORAGE_INFO];
    if (!pItem->isRegistered) return NET_E_NOT_SUPPORT;

    return pItem->unFunc.DeviceStorageInfo(pInfo);
}

/**
 * @brief 执行设备控制回调
 * @details 查找并执行宿主注册的设备控制回调
 * @param [in] pstCtrlInfo 设备控制信息结构体，包含控制命令和参数
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int executeDeviceControlCb(pNET_DeviceControlInfo_S pstCtrlInfo)
{
    if (pstCtrlInfo == NULL) return NET_E_INVALID_PARAM;

    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_DEVICE_CTRL];
    if (!pItem->isRegistered) return NET_E_NOT_SUPPORT;

    return pItem->unFunc.DeviceControl(pstCtrlInfo);
}

/**
 * @brief 执行修改用户密码回调
 * @details 查找并执行宿主注册的修改用户密码回调
 * @param [in] pPasswordInfo 修改密码信息结构体，包含用户名、旧密码、新密码
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT表示回调未注册，其他值表示失败
 */
int executeSetUserPasswordCb(pNET_UserPasswordInfo_S pPasswordInfo)
{
    if (pPasswordInfo == NULL) return NET_E_INVALID_PARAM;

    NET_Device_CbItem_S* pItem = &g_cbTable[NET_CB_TYPE_SET_USER_PASSWORD];
    if (!pItem->isRegistered) return NET_E_NOT_SUPPORT;

    return pItem->unFunc.SetUserPassword(pPasswordInfo);
}
