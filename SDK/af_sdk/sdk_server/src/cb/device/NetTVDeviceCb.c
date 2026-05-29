/**
 * @file NetSDKDeviceCb.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-04
 * 
 * @brief 设备通用回调
 */

#include<stdio.h>

#include "NetTVDeviceCbExecute.h"
#include "NetTVSDKServerInterface.h"

/**
 * @brief 设备通用回调枚举定义
 */
typedef enum 
{
    NET_TV_CB_TYPE_DEVICE_INFO = 0,         /* 获取设备信息 */
    NET_TV_CB_TYPE_DEVICE_STATUS,     
    NET_TV_CB_TYPE_DEVICE_CTRL,       
    NET_TV_CB_TYPE_DEVICE_CONFIG,     

    NET_TV_CB_TYPE_MAX               
} Net_TV_DeviceCb_E;

/**
 * @brief 设备通用回调函数联合体定义
 */
typedef union 
{
    int (*DeviceInfo)(LPNET_TV_DEVICE_INFO_S pInfo);
  
} Net_TV_DeviceCb_Un;


typedef struct 
{
    Net_TV_DeviceCb_E 	enType;      			// 回调类型
    Net_TV_DeviceCb_Un 	unFunc;      			// 回调函数指针（联合体）
    int isRegistered;          					// 注册标记：0=未注册，1=已注册
} NET_TV_Device_CbItem;

/**
 * @brief 全局回调注册表
 */
static NET_TV_Device_CbItem g_cbTable[NET_TV_CB_TYPE_MAX] = {0};

// ========================== 注册接口实现 ==========================
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDeviceInfo(NET_TV_COMMON_ECODE_E (*CB)(LPNET_TV_DEVICE_INFO_S pInfo))
{
    if (CB == NULL)
    {
        return FALSE;
    } 

    NET_TV_Device_CbItem* pItem = &g_cbTable[NET_TV_CB_TYPE_DEVICE_INFO];
    if (pItem->isRegistered)
    {
        return FALSE; // 已注册
    }
    
    pItem->enType = NET_TV_CB_TYPE_DEVICE_INFO;
    pItem->unFunc.DeviceInfo = CB;
    pItem->isRegistered = 1;

    return TRUE;
}

// ========================== 执行接口实现 ==========================
int NetSDK_ExecuteCb_DeviceInfo(LPNET_TV_DEVICE_INFO_S pInfo) 
{
    if (pInfo == NULL) return -2;
    NET_TV_Device_CbItem* pItem = &g_cbTable[NET_TV_CB_TYPE_DEVICE_INFO];
    if (!pItem->isRegistered) return -1;
    
    // 执行对应回调（类型安全）
    return pItem->unFunc.DeviceInfo(pInfo);
}
