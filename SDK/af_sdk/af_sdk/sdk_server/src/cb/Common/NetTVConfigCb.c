/*
 * @FilePath     : sdk_new/sdk_server/src/cb/Common/config/NetTVConfigCb.c
 * @Author        : chenchl (chenchl@kfb.cn)
 * @Date          : 2026-07-28
 * @LastEditors   : ITC
 * @LastEditTime  : 2026-08-19
 * @Description   : 通用设备配置回调注册与执行实现
 *                  本文件包含所有部门通用的设备配置回调及核心命令码分发函数。
 *                  部门独有配置回调已分离至对应目录：
 *                  - BG6_ZHSJ/BU_SJCL/NetTVEventConfigCb.c（事件报警类）
 *                  - BG6_ZHSJ/BU_SJCL/NetTVNvrConfigCb.c（NVR独有：RTSP/回放/录像/预览/对讲/通道）
 *                  - BG6_ZHSJ/BU_SJGZ/NetTVIpcConfigCb.c（IPC外设类：SD/WiFi/4G/热点）
 *                  - BG6_ZHSJ/BU_SJGZ/NetTVIpcImageConfigCb.c（IPC图像类：曝光/日夜/背光/降噪/白平衡/抓拍）
 *                  通用配置范围：
 *                  - 设备基础（DeviceCfg/NtpCfg）
 *                  - 码流与OSD（StreamCfg/OsdCapCfg）
 *                  - 图像参数（ImageCfg）
 *                  - 音频/网络（AudioCfg/NetworkCfg）
 *                  - 安全/日志（SecurityServices/SshCountdown/Log相关）
 *                  - 录像状态/升级（RecordStatus/Upgrade）
 *                  - 通用回调（GetDevConfig/SetDevConfig）
 *
 *                  事件与报警类回调（基础报警、智能分析报警、人脸识别、人流统计、
 *                  垃圾检测、行业AI检测等）
 *                  sdk_server/src/cb/BG6_ZHSJ/BU_SJCL/NetTVEventConfigCb.c
 *
 *                  NVR独有配置回调
 *                  sdk_server/src/cb/BG6_ZHSJ/BU_SJCL/NetTVNvrConfigCb.c
 *
 *                  IPC摄像机独有配置回调（Wifi/4G/Hotspot/SD卡状态）
 *                  sdk_server/src/cb/BG6_ZHSJ/BU_SJGZ/NetTVIpcConfigCb.c
 *
 *                  IPC摄像机图像类配置回调
 *                  sdk_server/src/cb/BG6_ZHSJ/BU_SJGZ/NetTVIpcImageConfigCb.c
 *
 * @note 本文件实现配置回调的注册、查找和执行逻辑，采用两级回调机制：
 *       1. 按命令码注册的专用回调（优先级高）
 *       2. 通用回调（优先级低，当专用回调未注册时使用）
 *       新增配置项时调用Register函数即可，无需预声明命令码条目
 *       registerGetCmdCb/registerSetCmdCb 为非 static 函数，
 *       供部门子模块（BG6_ZHSJ/BU_SJCL、BG6_ZHSJ/BU_SJGZ 等）通过
 *       NetTVConfigCbExecute.h 调用。
 */
#include <stdio.h>
#include <stddef.h>
#include "NetTVConfigCbExecute.h"
#include "NetTVSDKServerInterface.h"

/**
 * @brief 通用配置回调表结构体（存储全局注册的通用回调）
 */
typedef struct tagNETTVConfigCbTable
{
    NET_CB_GetDevConfig cbGet;                       /* 通用配置获取回调 */
    NET_CB_SetDevConfig cbSet;                       /* 通用配置设置回调 */
} NET_CONFIG_CB_TABLE_S;

/**
 * @brief 按命令码注册的配置回调条目结构体（命令码与回调的映射）
 */
typedef struct tagNETTVConfigCmdCbItem
{
    INT32 nGetCommand;                                  /* Get命令码，NET_CFG_INVALID表示无Get */
    INT32 nSetCommand;                                  /* Set命令码，NET_CFG_INVALID表示无Set */
    NET_CB_GetDevConfigByCommand cbGetByCmd;         /* 按命令码注册的Get回调 */
    NET_CB_SetDevConfigByCommand cbSetByCmd;         /* 按命令码注册的Set回调 */
} NET_CONFIG_CMD_CB_ITEM_S;

static NET_CONFIG_CB_TABLE_S g_stConfigCbTable = {0};   /* 通用配置回调表实例 */

#define MAX_CMD_CB_ENTRIES 256  /* 动态注册表最大条目数 */

/**
 * @brief 动态配置回调表（运行时注册，无需预声明命令码）
 * @note 注册回调时自动追加条目，新增配置项只需调用Register函数即可
 */
static NET_CONFIG_CMD_CB_ITEM_S g_astConfigCmdCbTable[MAX_CMD_CB_ENTRIES];
static UINT32 g_dwCmdCbCount = 0;

/**
 * @brief 根据Get命令码查找回调条目
 * @param [IN] nCommand Get命令码
 * @return 找到返回条目指针，未找到返回NULL
 */
static NET_CONFIG_CMD_CB_ITEM_S* findCmdCbItemByGetCmd(INT32 nCommand)
{
    UINT32 i = 0;
    for (i = 0; i < g_dwCmdCbCount; ++i)
    {
        if (g_astConfigCmdCbTable[i].nGetCommand == nCommand)
        {
            return &g_astConfigCmdCbTable[i];
        }
    }
    return NULL;
}

/**
 * @brief 根据Set命令码查找回调条目
 * @param [IN] nCommand Set命令码
 * @return 找到返回条目指针，未找到返回NULL
 */
static NET_CONFIG_CMD_CB_ITEM_S* findCmdCbItemBySetCmd(INT32 nCommand)
{
    UINT32 i = 0;
    for (i = 0; i < g_dwCmdCbCount; ++i)
    {
        if (g_astConfigCmdCbTable[i].nSetCommand == nCommand)
        {
            return &g_astConfigCmdCbTable[i];
        }
    }
    return NULL;
}

/**
 * @brief 注册按命令码分发的Get回调
 * @param [IN] nCommand Get命令码
 * @param [IN] pCb 回调函数指针
 * @return 注册成功返回TRUE，失败返回FALSE（已注册或表满）
 * @note 回调在运行时动态注册，无需预声明命令码条目
 */
BOOL registerGetCmdCb(INT32 nCommand, NET_CB_GetDevConfigByCommand pCb)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (pCb == NULL)
    {
        return FALSE;
    }

    pItem = findCmdCbItemByGetCmd(nCommand);
    if (pItem != NULL)
    {
        if (pItem->cbGetByCmd != NULL)
        {
            return FALSE;
        }
        pItem->cbGetByCmd = pCb;
        return TRUE;
    }

    /* 未找到：追加新条目 */
    if (g_dwCmdCbCount >= MAX_CMD_CB_ENTRIES)
    {
        return FALSE;
    }
    pItem = &g_astConfigCmdCbTable[g_dwCmdCbCount];
    pItem->nGetCommand = nCommand;
    pItem->nSetCommand = NET_CFG_INVALID;
    pItem->cbGetByCmd = pCb;
    pItem->cbSetByCmd = NULL;
    g_dwCmdCbCount++;
    return TRUE;
}

/**
 * @brief 注册按命令码分发的Set回调
 * @param [IN] nCommand Set命令码
 * @param [IN] pCb 回调函数指针
 * @return 注册成功返回TRUE，失败返回FALSE（已注册或表满）
 * @note 回调在运行时动态注册，无需预声明命令码条目
 */
BOOL registerSetCmdCb(INT32 nCommand, NET_CB_SetDevConfigByCommand pCb)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (pCb == NULL)
    {
        return FALSE;
    }

    pItem = findCmdCbItemBySetCmd(nCommand);
    if (pItem != NULL)
    {
        if (pItem->cbSetByCmd != NULL)
        {
            return FALSE;
        }
        pItem->cbSetByCmd = pCb;
        return TRUE;
    }

    /* 未找到：追加新条目 */
    if (g_dwCmdCbCount >= MAX_CMD_CB_ENTRIES)
    {
        return FALSE;
    }
    pItem = &g_astConfigCmdCbTable[g_dwCmdCbCount];
    pItem->nGetCommand = NET_CFG_INVALID;
    pItem->nSetCommand = nCommand;
    pItem->cbGetByCmd = NULL;
    pItem->cbSetByCmd = pCb;
    g_dwCmdCbCount++;
    return TRUE;
}

/**
 * @brief 注册通用获取配置回调（不带命令码，用于兜底处理）
 * @param [in] pCb 通用获取配置回调函数指针
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetDevConfigCb(NET_CB_GetDevConfig pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    if (g_stConfigCbTable.cbGet != NULL)
    {
        return FALSE;
    }

    g_stConfigCbTable.cbGet = pCb;
    return TRUE;
}

/**
 * @brief 注册通用设置配置回调（不带命令码，用于兜底处理）
 * @param [in] pCb 通用设置配置回调函数指针
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetDevConfigCb(NET_CB_SetDevConfig pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    if (g_stConfigCbTable.cbSet != NULL)
    {
        return FALSE;
    }

    g_stConfigCbTable.cbSet = pCb;
    return TRUE;
}

/**
 * @brief 注册获取设备基础配置的回调函数
 * @param [in] pCb 用于填充 NET_DeviceCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_DEVICECFG, pCb);
}

/**
 * @brief 注册设置设备基础配置的回调函数
 * @param [in] pCb 用于读取 NET_DeviceCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetDeviceConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_DEVICECFG, pCb);
}

/**
 * @brief 注册获取 NTP 时间同步配置的回调函数
 * @param [in] pCb 用于填充 NET_NtpCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetNtpConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_NTPCFG, pCb);
}

/**
 * @brief 注册设置 NTP 时间同步配置的回调函数
 * @param [in] pCb 用于读取 NET_NtpCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetNtpConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_NTPCFG, pCb);
}

/**
 * @brief 注册获取码流配置的回调函数
 * @param [in] pCb 用于填充 NET_StreamCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetStreamConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_STREAMCFG, pCb);
}

/**
 * @brief 注册设置码流配置的回调函数
 * @param [in] pCb 用于读取 NET_StreamCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetStreamConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_STREAMCFG, pCb);
}


/**
 * @brief 注册获取网络参数配置的回调函数
 * @param [in] pCb 用于填充 NET_NetworkCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetNetworkConfigCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_NETWORKCFG, pCb);
}

/**
 * @brief 注册设置网络参数配置的回调函数
 * @param [in] pCb 用于读取 NET_NetworkCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetNetworkConfigCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_NETWORKCFG, pCb);
}


/**
 * @brief 注册获取安全服务信息的回调函数
 * @param [in] pCb 用于填充 NET_SecurityServicesInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetSecurityServicesInfoCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_SECURITY_SERVICES_INFO, pCb);
}

/**
 * @brief 注册设置安全服务信息的回调函数
 * @param [in] pCb 用于读取 NET_SecurityServicesInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetSecurityServicesInfoCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_SECURITY_SERVICES_INFO, pCb);
}

/**
 * @brief 注册获取 SSH 倒计时信息的回调函数
 * @param [in] pCb 用于填充 NET_SshCountdown_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetSshCountdownCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_SSH_COUNTDOWN, pCb);
}

/**
 * @brief 注册查找日志的回调函数
 * @param [in] pCb 用于按条件查询日志的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterFindLogCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_FIND_LOG, pCb);
}

/**
 * @brief 注册导出日志的回调函数
 * @param [in] pCb 用于执行日志导出操作的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterExportLogCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_EXPORT_LOG, pCb);
}

/**
 * @brief 注册获取日志服务器配置的回调函数
 * @param [in] pCb 用于填充 NET_LogServer_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetLogServerCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_LOG_SERVER, pCb);
}

/**
 * @brief 注册设置日志服务器配置的回调函数
 * @param [in] pCb 用于读取 NET_LogServer_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetLogServerCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_LOG_SERVER, pCb);
}

/**
 * @brief 注册测试日志服务器连通性的回调函数
 * @param [in] pCb 用于执行日志服务器连通性测试的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterTestLogServerCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_TEST_LOG_SERVER, pCb);
}


/**
 * @brief 注册获取升级状态的回调函数
 * @param [in] pCb 用于填充升级进度/状态的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetUpgradeStatusCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_UPGRADESTATUS, pCb);
}

/**
 * @brief 注册获取升级版本信息的回调函数
 * @param [in] pCb 用于填充当前/可用版本信息的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterGetUpgradeVersionCb(NET_CB_GetDevConfigByCommand pCb)
{
    return registerGetCmdCb(NET_GET_UPGRADEVERSION, pCb);
}

/**
 * @brief 注册执行固件升级的回调函数
 * @param [in] pCb 用于触发固件升级流程的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_serverRegisterSetUpgradeCb(NET_CB_SetDevConfigByCommand pCb)
{
    return registerSetCmdCb(NET_SET_UPGRADE, pCb);
}


/**
 * @brief 执行配置获取回调（核心分发函数）
 * @param [IN] nChannelId 通道号
 * @param [IN] dwCommand 命令码（标识配置类型）
 * @param [OUT] lpOutBuffer 输出缓冲区，用于存放配置数据
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回调（RTSP/回放等）> 按命令码注册的回调 > 通用回调
 */
int executeGetDevConfigCb(INT32 nChannelId, INT32 dwCommand, LPVOID lpOutBuffer)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (lpOutBuffer == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    /* 查找按命令码注册的专用回调 */
    pItem = findCmdCbItemByGetCmd(dwCommand);
    if (pItem != NULL && pItem->cbGetByCmd != NULL)
    {
        return pItem->cbGetByCmd(nChannelId, lpOutBuffer);
    }

    /* 降级到通用回调 */
    if (g_stConfigCbTable.cbGet != NULL)
    {
        return g_stConfigCbTable.cbGet(nChannelId, dwCommand, lpOutBuffer);
    }

    /* 无任何回调注册 */
    printf("[NetTVConfigCb] GetDevConfig callback missing, cmd=%d, item=%p, cbGetByCmd=%p, cbGet=%p\n",
           dwCommand,
           (void*)pItem,
           pItem ? (void*)pItem->cbGetByCmd : NULL,
           (void*)g_stConfigCbTable.cbGet);
    return NET_E_NONSUPPORT;
}

/**
 * @brief 执行配置设置回调（核心分发函数）
 * @param [IN] nChannelId 通道号
 * @param [IN] dwCommand 命令码（标识配置类型）
 * @param [IN] lpInBuffer 输入缓冲区，包含要设置的配置数据
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回调（回放控制等）> 按命令码注册的回调 > 通用回调
 */
int executeSetDevConfigCb(INT32 nChannelId, INT32 dwCommand, LPVOID lpInBuffer)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (lpInBuffer == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    /* 查找按命令码注册的专用回调 */
    pItem = findCmdCbItemBySetCmd(dwCommand);
    if (pItem != NULL && pItem->cbSetByCmd != NULL)
    {
        return pItem->cbSetByCmd(nChannelId, lpInBuffer);
    }

    /* 降级到通用回调 */
    if (g_stConfigCbTable.cbSet != NULL)
    {
        return g_stConfigCbTable.cbSet(nChannelId, dwCommand, lpInBuffer);
    }

    return NET_E_NONSUPPORT;
}

