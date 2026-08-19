/*
 * @FilePath     : sdk_new/sdk_server/src/cb/Common/config/NetTVConfigCb.c
 * @Author        : tianl (tianl@kfb.cn)
 * @Date          : 2026-07-28
 * @LastEditors   : ITC
 * @LastEditTime  : 2026-08-18
 * @Description   : 通用设备配置回调注册与执行实现
 *                  本文件包含所有部门通用的设备配置回调及核心命令码分发函数。
 *                  通用配置范围：
 *                  - 设备基础（DeviceCfg/NtpCfg）
 *                  - 码流与OSD（StreamCfg/OsdCapCfg）
 *                  - 图像参数（ImageCfg/Exposure/DayNight/BackLight/Denoise/WhiteBalance）
 *                  - 音频/网络（AudioCfg/NetworkCfg）
 *                  - 安全/日志（SecurityServices/SshCountdown/Log相关）
 *                  - 录像/预览/升级/抓拍（RecordStatus/RecordSchedule/PreviewInfo/Upgrade/Capture）
 *                  - 对讲/通道（Talkback/ChannelInfo/ChannelList）
 *                  - 通用回调（GetDevConfig/SetDevConfig/RtspUrl/ReplayUrl/ControlReplay/ReplayRecordList）
 *
 *                  事件与报警类回调（基础报警、智能分析报警、人脸识别、人流统计、
 *                  垃圾检测、行业AI检测等）已迁移至：
 *                  sdk_server/src/cb/BG6_ZHSJ/BU_SJCL/NetTVEventConfigCb.c
 *
 *                  IPC摄像机独有配置回调（Wifi/4G/Hotspot/SD卡状态）已迁移至：
 *                  sdk_server/src/cb/BG6_ZHSJ/BU_SJGZ/NetTVIpcConfigCb.c
 *
 * @note 本文件实现配置回调的注册、查找和执行逻辑，采用两级回调机制：
 *       1. 按命令码注册的专用回调（优先级高）
 *       2. 通用回调（优先级低，当专用回调未注册时使用）
 *       新增配置项时调用Register函数即可，无需预声明命令码条目
 *       Net_RegisterGetCmdCb/Net_RegisterSetCmdCb 为非 static 函数，
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
    NET_CB_GetRtspUrl   cbGetRtspUrl;                /* RTSP流地址获取回调 */
    NET_CB_GetReplayUrl cbGetReplayUrl;              /* 回放URL获取回调 */
    NET_CB_ControlReplay cbControlReplay;            /* 回放控制回调 */
    NET_CB_GetReplayRecordList cbGetReplayRecordList;/* 回放录像列表获取回调 */
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
static NET_CONFIG_CMD_CB_ITEM_S* Net_FindCmdCbItemByGetCommand(INT32 nCommand)
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
static NET_CONFIG_CMD_CB_ITEM_S* Net_FindCmdCbItemBySetCommand(INT32 nCommand)
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
BOOL Net_RegisterGetCmdCb(INT32 nCommand, NET_CB_GetDevConfigByCommand pCb)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (pCb == NULL)
    {
        return FALSE;
    }

    pItem = Net_FindCmdCbItemByGetCommand(nCommand);
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
BOOL Net_RegisterSetCmdCb(INT32 nCommand, NET_CB_SetDevConfigByCommand pCb)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (pCb == NULL)
    {
        return FALSE;
    }

    pItem = Net_FindCmdCbItemBySetCommand(nCommand);
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
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDevConfig(NET_CB_GetDevConfig pCb)
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
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDevConfig(NET_CB_SetDevConfig pCb)
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
 * @brief 注册获取 RTSP 实时预览地址的回调函数
 * @param [in] pCb 用于填充 RTSP URL 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRtspUrl(NET_CB_GetRtspUrl pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    if (g_stConfigCbTable.cbGetRtspUrl != NULL)
    {
        return FALSE;
    }

    g_stConfigCbTable.cbGetRtspUrl = pCb;
    return TRUE;
}

/**
 * @brief 注册获取回放地址的回调函数
 * @param [in] pCb 用于填充回放 URL 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetReplayUrl(NET_CB_GetReplayUrl pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    if (g_stConfigCbTable.cbGetReplayUrl != NULL)
    {
        return FALSE;
    }

    g_stConfigCbTable.cbGetReplayUrl = pCb;
    return TRUE;
}

/**
 * @brief 注册回放控制回调函数（播放/暂停/快进/快退/定位）
 * @param [in] pCb 用于执行回放控制命令的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_ControlReplay(NET_CB_ControlReplay pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    if (g_stConfigCbTable.cbControlReplay != NULL)
    {
        return FALSE;
    }

    g_stConfigCbTable.cbControlReplay = pCb;
    return TRUE;
}

/**
 * @brief 注册获取回放录像文件列表的回调函数
 * @param [in] pCb 用于填充录像文件列表的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetReplayRecordList(NET_CB_GetReplayRecordList pCb)
{
    if (pCb == NULL)
    {
        return FALSE;
    }

    if (g_stConfigCbTable.cbGetReplayRecordList != NULL)
    {
        return FALSE;
    }

    g_stConfigCbTable.cbGetReplayRecordList = pCb;
    return TRUE;
}

/**
 * @brief 注册获取设备基础配置的回调函数
 * @param [in] pCb 用于填充 NET_DeviceCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDeviceCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_DEVICECFG, pCb);
}

/**
 * @brief 注册设置设备基础配置的回调函数
 * @param [in] pCb 用于读取 NET_DeviceCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDeviceCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_DEVICECFG, pCb);
}

/**
 * @brief 注册获取 NTP 时间同步配置的回调函数
 * @param [in] pCb 用于填充 NET_NtpCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetNtpCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_NTPCFG, pCb);
}

/**
 * @brief 注册设置 NTP 时间同步配置的回调函数
 * @param [in] pCb 用于读取 NET_NtpCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetNtpCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_NTPCFG, pCb);
}

/**
 * @brief 注册获取码流配置的回调函数
 * @param [in] pCb 用于填充 NET_StreamCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetStreamCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_STREAMCFG, pCb);
}

/**
 * @brief 注册设置码流配置的回调函数
 * @param [in] pCb 用于读取 NET_StreamCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetStreamCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_STREAMCFG, pCb);
}

/**
 * @brief 注册获取 OSD 叠加能力集的回调函数
 * @param [in] pCb 用于填充 NET_OsdCapCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetOsdCapCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_OSDCAPCFG, pCb);
}

/**
 * @brief 注册设置 OSD 叠加配置的回调函数
 * @param [in] pCb 用于读取 NET_OsdCapCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetOsdCapCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_OSDCAPCFG, pCb);
}

/**
 * @brief 注册获取图像参数配置的回调函数
 * @param [in] pCb 用于填充 NET_ImageCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetImageCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_IMAGECFG, pCb);
}

/**
 * @brief 注册设置图像参数配置的回调函数
 * @param [in] pCb 用于读取 NET_ImageCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetImageCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_IMAGECFG, pCb);
}

/**
 * @brief 注册获取音频参数配置的回调函数
 * @param [in] pCb 用于填充 NET_AudioCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetAudioCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_AUDIOCFG, pCb);
}

/**
 * @brief 注册设置音频参数配置的回调函数
 * @param [in] pCb 用于读取 NET_AudioCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetAudioCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_AUDIOCFG, pCb);
}

/**
 * @brief 注册获取网络参数配置的回调函数
 * @param [in] pCb 用于填充 NET_NetworkCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetNetworkCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_NETWORKCFG, pCb);
}

/**
 * @brief 注册设置网络参数配置的回调函数
 * @param [in] pCb 用于读取 NET_NetworkCfg_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetNetworkCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_NETWORKCFG, pCb);
}

/* IPC独有配置回调（Wifi/4G/Hotspot/SD卡状态）已迁移至
   sdk_server/src/cb/BG6_ZHSJ/BU_SJGZ/NetTVIpcConfigCb.c
   迁移范围：
   - SetConfigWifiSta/ConnectWifiSta/DisconnectWifiSta（Wifi STA）
   - Get4GInfo/Set4GInfo（4G蜂窝网络）
   - SetHotspotInfo/GetHotspotConn（Hotspot热点）
   - GetSdCardStatus（IPC物理SD卡槽状态） */

/**
 * @brief 注册获取安全服务信息的回调函数
 * @param [in] pCb 用于填充 NET_SecurityServicesInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSecurityServicesInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SECURITY_SERVICES_INFO, pCb);
}

/**
 * @brief 注册设置安全服务信息的回调函数
 * @param [in] pCb 用于读取 NET_SecurityServicesInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSecurityServicesInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_SECURITY_SERVICES_INFO, pCb);
}

/**
 * @brief 注册获取 SSH 倒计时信息的回调函数
 * @param [in] pCb 用于填充 NET_SshCountdown_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSshCountdown(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SSH_COUNTDOWN, pCb);
}

/**
 * @brief 注册查找日志的回调函数
 * @param [in] pCb 用于按条件查询日志的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_FindLog(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_FIND_LOG, pCb);
}

/**
 * @brief 注册导出日志的回调函数
 * @param [in] pCb 用于执行日志导出操作的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_ExportLog(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_EXPORT_LOG, pCb);
}

/**
 * @brief 注册获取日志服务器配置的回调函数
 * @param [in] pCb 用于填充 NET_LogServer_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetLogServer(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_LOG_SERVER, pCb);
}

/**
 * @brief 注册设置日志服务器配置的回调函数
 * @param [in] pCb 用于读取 NET_LogServer_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetLogServer(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_LOG_SERVER, pCb);
}

/**
 * @brief 注册测试日志服务器连通性的回调函数
 * @param [in] pCb 用于执行日志服务器连通性测试的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_TestLogServer(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_TEST_LOG_SERVER, pCb);
}

/**
 * @brief 注册录像控制回调函数（启停录像）
 * @param [in] pCb 用于执行录像启停控制的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_ControlRecordInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_CONTROL_RECORD_INFO, pCb);
}

/**
 * @brief 注册获取录像状态的回调函数
 * @param [in] pCb 用于填充 NET_RecordStatus_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRecordStatus(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_RECORD_STATUS, pCb);
}

/* SD卡状态查询（GetSdCardStatus）已迁移至
   sdk_server/src/cb/BG6_ZHSJ/BU_SJGZ/NetTVIpcConfigCb.c（IPC独有配置） */

/**
 * @brief 注册获取录像计划配置的回调函数
 * @param [in] pCb 用于填充 NET_RecordSchedule_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRecordSchedule(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_RECORD_SCHEDULE, pCb);
}

/**
 * @brief 注册设置录像计划配置的回调函数
 * @param [in] pCb 用于读取 NET_RecordSchedule_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRecordSchedule(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_RECORD_SCHEDULE, pCb);
}

/**
 * @brief 注册获取录像高级参数的回调函数
 * @param [in] pCb 用于填充 NET_RecordAdvancedParam_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRecordAdvancedParam(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_RECORD_ADVANCED_PARAM, pCb);
}

/**
 * @brief 注册设置录像高级参数的回调函数
 * @param [in] pCb 用于读取 NET_RecordAdvancedParam_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRecordAdvancedParam(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_RECORD_ADVANCED_PARAM, pCb);
}

/**
 * @brief 注册查找录像文件信息的回调函数
 * @param [in] pCb 用于按条件查询录像文件列表的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_FindRecordFileInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_FIND_RECORD_FILE_INFO, pCb);
}

/**
 * @brief 注册下载录像文件的回调函数
 * @param [in] pCb 用于执行录像文件下载的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_DownloadRecordFile(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_DOWNLOAD_RECORD_FILE, pCb);
}

/**
 * @brief 注册获取预览信息的回调函数
 * @param [in] pCb 用于填充 NET_PreviewInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPreviewInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PREVIEW_INFO, pCb);
}

/**
 * @brief 注册设置预览信息的回调函数
 * @param [in] pCb 用于读取 NET_PreviewInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPreviewInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PREVIEW_INFO, pCb);
}

/**
 * @brief 注册获取升级状态的回调函数
 * @param [in] pCb 用于填充升级进度/状态的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetUpgradeStatus(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_UPGRADESTATUS, pCb);
}

/**
 * @brief 注册获取升级版本信息的回调函数
 * @param [in] pCb 用于填充当前/可用版本信息的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetUpgradeVersion(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_UPGRADEVERSION, pCb);
}

/**
 * @brief 注册执行固件升级的回调函数
 * @param [in] pCb 用于触发固件升级流程的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetUpgrade(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_UPGRADE, pCb);
}

/**
 * @brief 注册获取抓拍计划配置的回调函数
 * @param [in] pCb 用于填充 NET_CapturePlanInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCapturePlanInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CAPTURE_PLAN_INFO, pCb);
}

/**
 * @brief 注册设置抓拍计划配置的回调函数
 * @param [in] pCb 用于读取 NET_CapturePlanInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCapturePlanInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CAPTURE_PLAN_INFO, pCb);
}

/**
 * @brief 注册获取抓拍参数的回调函数
 * @param [in] pCb 用于填充 NET_CaptureParamInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCaptureParamInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CAPTURE_PARAM_INFO, pCb);
}

/**
 * @brief 注册设置抓拍参数的回调函数
 * @param [in] pCb 用于读取 NET_CaptureParamInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCaptureParamInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CAPTURE_PARAM_INFO, pCb);
}

/**
 * @brief 注册获取曝光参数的回调函数
 * @param [in] pCb 用于填充 NET_ExposureInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetExposureInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_EXPOSURE_INFO, pCb);
}

/**
 * @brief 注册设置曝光参数的回调函数
 * @param [in] pCb 用于读取 NET_ExposureInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetExposureInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_EXPOSURE_INFO, pCb);
}

/**
 * @brief 注册获取日夜切换参数的回调函数
 * @param [in] pCb 用于填充 NET_DayNightInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDayNightInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_DAYNIGHT_INFO, pCb);
}

/**
 * @brief 注册设置日夜切换参数的回调函数
 * @param [in] pCb 用于读取 NET_DayNightInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDayNightInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_DAYNIGHT_INFO, pCb);
}

/**
 * @brief 注册获取背光补偿参数的回调函数
 * @param [in] pCb 用于填充 NET_BackLightInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetBackLightInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_BACKLIGHT_INFO, pCb);
}

/**
 * @brief 注册设置背光补偿参数的回调函数
 * @param [in] pCb 用于读取 NET_BackLightInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetBackLightInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_BACKLIGHT_INFO, pCb);
}

/**
 * @brief 注册获取降噪参数的回调函数
 * @param [in] pCb 用于填充 NET_DenoiseInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDenoiseInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_DENOISE_INFO, pCb);
}

/**
 * @brief 注册设置降噪参数的回调函数
 * @param [in] pCb 用于读取 NET_DenoiseInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDenoiseInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_DENOISE_INFO, pCb);
}

/**
 * @brief 注册获取白平衡参数的回调函数
 * @param [in] pCb 用于填充 NET_WhiteBalanceInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetWhiteBalanceInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_WHITEBALANCE_INFO, pCb);
}

/**
 * @brief 注册设置白平衡参数的回调函数
 * @param [in] pCb 用于读取 NET_WhiteBalanceInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetWhiteBalanceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_WHITEBALANCE_INFO, pCb);
}

/**
 * @brief 注册设置对讲状态的回调函数（开启/关闭对讲）
 * @param [in] pCb 用于执行对讲启停控制的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTalkbackState(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_STATE_TALKBACK, pCb);
}

/**
 * @brief 注册设置对讲音频流向（发送至码流）的回调函数
 * @param [in] pCb 用于将本地音频注入码流的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTalkbackToStream(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_TO_STREAM_TALKBACK, pCb);
}

/**
 * @brief 注册获取对讲音频流向（从码流读取）的回调函数
 * @param [in] pCb 用于从码流提取对讲音频的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetTalkbackFromStream(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_FROM_STREAM_TALKBACK, pCb);
}

/**
 * @brief 注册回放对讲控制的回调函数
 * @param [in] pCb 用于回放期间对讲控制的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetReplayTalkback(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_REPLAY_TALKBACK, pCb);
}

/**
 * @brief 注册获取通道信息的回调函数
 * @param [in] pCb 用于填充 NET_ChannelInfo_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetChannelInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CHANNEL_INFO, pCb);
}

/**
 * @brief 注册获取通道列表的回调函数
 * @param [in] pCb 用于填充 NET_ChannelList_S 的回调函数
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetChannelList(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CHANNEL_LIST, pCb);
}

/* 人脸识别/人流统计/垃圾检测/行业AI检测等事件配置回调已迁移至
   sdk_server/src/cb/BG6_ZHSJ/BU_SJCL/NetTVEventConfigCb.c
   迁移范围：
   - FaceCapture/FaceCompare/TargetLib/FaceInfo
   - GarbageExposure/GarbageOverflow
   - PeopleFlowStatistics/ResetPeopleFlowStatistics/PeopleDensityDetection
   - ManholeCoverAbnormal/SleepOnDuty/ElectricVehicleInElevator/PersonFallDown/PersonFall
   - ConstructionOccupyRoad/Congestion/LicensePlateRecognition/HighAltitudeSeatbelt/SafetyHelmet
   - PhoneUsage/Smoking/OpenFlame/BareSoil/HoleProtectionBar/ReflectiveClothing/PetRecognition
   - ClimbFence/Dimission/IllegalLane/Retrograde/NonmotorVehicleIntrusion/OccupationEmergency
   - PedestrianIntrusion/SmokeFire/RoadPonding */

/**
 * @brief 执行配置获取回调（核心分发函数）
 * @param [IN] dwChannelID 通道号
 * @param [IN] dwCommand 命令码（标识配置类型）
 * @param [OUT] lpOutBuffer 输出缓冲区，用于存放配置数据
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回调（RTSP/回放等）> 按命令码注册的回调 > 通用回调
 */
int NetSDK_ExecuteCb_GetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpOutBuffer)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (lpOutBuffer == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    /* RTSP URL 获取：优先走专用回调 */
    if (dwCommand == NET_GET_RTSPURLCFG && g_stConfigCbTable.cbGetRtspUrl != NULL)
    {
        return g_stConfigCbTable.cbGetRtspUrl(dwChannelID, (pNET_RtspUrlInfo_S)lpOutBuffer);
    }

    /* 回放URL获取：优先走专用回调 */
    if (dwCommand == NET_GET_REPLAY_URLCFG && g_stConfigCbTable.cbGetReplayUrl != NULL)
    {
        pNET_ReplayUrlInfo_S pInfo = (pNET_ReplayUrlInfo_S)lpOutBuffer;
        if (pInfo->uChannel == 0)
        {
            pInfo->uChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbGetReplayUrl(pInfo);
    }

    /* 回放录像列表获取：优先走专用回调 */
    if (dwCommand == NET_GET_REPLAY_RECORD_LIST && g_stConfigCbTable.cbGetReplayRecordList != NULL)
    {
        pNET_ReplayRecordList_S pInfo = (pNET_ReplayRecordList_S)lpOutBuffer;
        if (pInfo->uChannel == 0)
        {
            pInfo->uChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbGetReplayRecordList(pInfo);
    }

    /* 重复检查RTSP URL（兼容旧逻辑） */
    if (dwCommand == NET_GET_RTSPURLCFG && g_stConfigCbTable.cbGetRtspUrl != NULL)
    {
        return g_stConfigCbTable.cbGetRtspUrl(dwChannelID, (pNET_RtspUrlInfo_S)lpOutBuffer);
    }

    /* 重复检查回放URL（兼容旧逻辑） */
    if (dwCommand == NET_GET_REPLAY_URLCFG && g_stConfigCbTable.cbGetReplayUrl != NULL)
    {
        pNET_ReplayUrlInfo_S pInfo = (pNET_ReplayUrlInfo_S)lpOutBuffer;
        if (pInfo->uChannel == 0)
        {
            pInfo->uChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbGetReplayUrl(pInfo);
    }

    /* 查找按命令码注册的专用回调 */
    pItem = Net_FindCmdCbItemByGetCommand(dwCommand);
    if (pItem != NULL && pItem->cbGetByCmd != NULL)
    {
        return pItem->cbGetByCmd(dwChannelID, lpOutBuffer);
    }

    /* 降级到通用回调 */
    if (g_stConfigCbTable.cbGet != NULL)
    {
        return g_stConfigCbTable.cbGet(dwChannelID, dwCommand, lpOutBuffer);
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
 * @brief 执行回放URL获取回调
 * @param [INOUT] pInfo 回放查询条件和播放URL返回信息
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回放URL回调 > 通用配置回调
 */
int NetSDK_ExecuteCb_GetReplayUrl(pNET_ReplayUrlInfo_S pInfo)
{
    if (pInfo == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    /* 优先使用专用回放URL回调 */
    if (g_stConfigCbTable.cbGetReplayUrl != NULL)
    {
        return g_stConfigCbTable.cbGetReplayUrl(pInfo);
    }

    /* 降级到通用配置回调 */
    if (g_stConfigCbTable.cbGet != NULL)
    {
        return g_stConfigCbTable.cbGet(pInfo->uChannel, NET_GET_REPLAY_URLCFG, pInfo);
    }

    printf("[NetTVConfigCb] GetReplayUrl callback missing\n");
    return NET_E_NONSUPPORT;
}

/**
 * @brief 执行回放控制回调
 * @param [INOUT] pInfo 回放控制输入输出参数
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回放控制回调 > 通用配置设置回调
 */
int NetSDK_ExecuteCb_ControlReplay(pNET_ReplayCtrlInfo_S pInfo)
{
    if (pInfo == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    printf("[NetTVConfigCb] ControlReplay callback: channel=%d, ctrlType=%d, startTime=[%s], endTime=[%s], sessionId=[%s]\n",
           pInfo->uChannel, pInfo->uCtrlType, pInfo->szStartTime, pInfo->szEndTime, pInfo->szSessionId);

    /* 优先使用专用回放控制回调 */
    if (g_stConfigCbTable.cbControlReplay != NULL)
    {
        return g_stConfigCbTable.cbControlReplay(pInfo);
    }

    /* 降级到通用配置设置回调 */
    if (g_stConfigCbTable.cbSet != NULL)
    {
        return g_stConfigCbTable.cbSet(pInfo->uChannel, NET_SET_REPLAY_CTRL, pInfo);
    }

    printf("[NetTVConfigCb] ControlReplay callback missing\n");
    return NET_E_NONSUPPORT;
}

/**
 * @brief 执行回放录像列表获取回调
 * @param [INOUT] pInfo 查询条件及结果
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回放录像列表回调 > 通用配置回调
 */
int NetSDK_ExecuteCb_GetReplayRecordList(pNET_ReplayRecordList_S pInfo)
{
    if (pInfo == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    /* 优先使用专用回放录像列表回调 */
    if (g_stConfigCbTable.cbGetReplayRecordList != NULL)
    {
        return g_stConfigCbTable.cbGetReplayRecordList(pInfo);
    }

    /* 降级到通用配置回调 */
    if (g_stConfigCbTable.cbGet != NULL)
    {
        return g_stConfigCbTable.cbGet(pInfo->uChannel, NET_GET_REPLAY_RECORD_LIST, pInfo);
    }

    printf("[NetTVConfigCb] GetReplayRecordList callback missing\n");
    return NET_E_NONSUPPORT;
}

/**
 * @brief 执行配置设置回调（核心分发函数）
 * @param [IN] dwChannelID 通道号
 * @param [IN] dwCommand 命令码（标识配置类型）
 * @param [IN] lpInBuffer 输入缓冲区，包含要设置的配置数据
 * @return NET_E_SUCCEED 成功，其他值失败
 * @note 回调执行优先级：专用回调（回放控制等）> 按命令码注册的回调 > 通用回调
 */
int NetSDK_ExecuteCb_SetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpInBuffer)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (lpInBuffer == NULL)
    {
        return NET_E_INVALID_PARAM;
    }

    /* 回放控制：走专用回调 */
    if (dwCommand == NET_SET_REPLAY_CTRL && g_stConfigCbTable.cbControlReplay != NULL)
    {
        pNET_ReplayCtrlInfo_S pInfo = (pNET_ReplayCtrlInfo_S)lpInBuffer;
        if (pInfo->uChannel == 0)
        {
            pInfo->uChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbControlReplay(pInfo);
    }

    /* 查找按命令码注册的专用回调 */
    pItem = Net_FindCmdCbItemBySetCommand(dwCommand);
    if (pItem != NULL && pItem->cbSetByCmd != NULL)
    {
        return pItem->cbSetByCmd(dwChannelID, lpInBuffer);
    }

    /* 降级到通用回调 */
    if (g_stConfigCbTable.cbSet != NULL)
    {
        return g_stConfigCbTable.cbSet(dwChannelID, dwCommand, lpInBuffer);
    }

    return NET_E_NONSUPPORT;
}

