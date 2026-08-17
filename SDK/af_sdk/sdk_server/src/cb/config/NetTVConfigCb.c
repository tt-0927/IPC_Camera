/**
 * @file NetTVConfigCb.c
 * @brief 设备配置回调注册与执行实现
 * @note 本文件实现配置回调的注册、查找和执行逻辑，采用两级回调机制：
 *       1. 按命令码注册的专用回调（优先级高）
 *       2. 通用回调（优先级低，当专用回调未注册时使用）
 *       新增配置项时调用Register函数即可，无需预声明命令码条目
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
static BOOL Net_RegisterGetCmdCb(INT32 nCommand, NET_CB_GetDevConfigByCommand pCb)
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
static BOOL Net_RegisterSetCmdCb(INT32 nCommand, NET_CB_SetDevConfigByCommand pCb)
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

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDeviceCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_DEVICECFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDeviceCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_DEVICECFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetNtpCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_NTPCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetNtpCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_NTPCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetStreamCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_STREAMCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetStreamCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_STREAMCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetOsdCapCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_OSDCAPCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetOsdCapCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_OSDCAPCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetImageCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_IMAGECFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetImageCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_IMAGECFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetAudioCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_AUDIOCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetAudioCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_AUDIOCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetNetworkCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_NETWORKCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetNetworkCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_NETWORKCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetConfigWifiSta(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CONFIG_WIFI_STA, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_ConnectWifiSta(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_CONNECT_WIFI_STA, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_DisconnectWifiSta(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_DISCONNECT_WIFI_STA, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_Get4GInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_4G_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_Set4GInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_4G_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetHotspotInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_HOTSPOT_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetHotspotConn(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_HOTSPOT_CONN, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSecurityServicesInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SECURITY_SERVICES_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSecurityServicesInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_SECURITY_SERVICES_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSshCountdown(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SSH_COUNTDOWN, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_FindLog(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_FIND_LOG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_ExportLog(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_EXPORT_LOG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetLogServer(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_LOG_SERVER, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetLogServer(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_LOG_SERVER, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_TestLogServer(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_TEST_LOG_SERVER, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_ControlRecordInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_CONTROL_RECORD_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRecordStatus(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_RECORD_STATUS, pCb);
}

/**
 * @brief 注册获取 SD 卡物理状态的回调函数。
 * @author ITC
 * @param [in] pCb 用于填充 NET_SdCardStatus_S 输出缓冲区的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSdCardStatus(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SD_CARD_STATUS, pCb);
}


/**
 * @brief 注册获取声音告警配置的回调函数。
 * @author ITC
 * @param [in] pCb 用于填充 NET_AudibleAlarmInfo_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetAudibleAlarmInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_AUDIBLE_ALARM_INFO, pCb);
}

/**
 * @brief 注册设置声音告警配置的回调函数。
 * @author ITC
 * @param [in] pCb 用于读取 NET_AudibleAlarmInfo_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetAudibleAlarmInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_AUDIBLE_ALARM_INFO, pCb);
}

/**
 * @brief 注册获取报警输入配置集合的回调函数。
 * @author ITC
 * @param [in] pCb 用于填充 NET_AlarmInputInfoList_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetAlarmInputInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_ALARM_INPUT_INFO, pCb);
}

/**
 * @brief 注册设置单路报警输入配置的回调函数。
 * @author ITC
 * @param [in] pCb 用于读取 NET_AlarmInputInfo_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetAlarmInputInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_ALARM_INPUT_INFO, pCb);
}

/**
 * @brief 注册获取报警输出配置集合的回调函数。
 * @author ITC
 * @param [in] pCb 用于填充 NET_AlarmOutputInfoList_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetAlarmOutputInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_ALARM_OUTPUT_INFO, pCb);
}

/**
 * @brief 注册设置单路报警输出配置的回调函数。
 * @author ITC
 * @param [in] pCb 用于读取 NET_AlarmOutputInfo_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetAlarmOutputInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_ALARM_OUTPUT_INFO, pCb);
}

/**
 * @brief 注册获取闪光灯告警配置的回调函数。
 * @author ITC
 * @param [in] pCb 用于填充 NET_FlashingLightAlarmInfo_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetFlashingLightAlarmInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_FLASHING_LIGHT_ALARM_INFO, pCb);
}

/**
 * @brief 注册设置闪光灯告警配置的回调函数。
 * @author ITC
 * @param [in] pCb 用于读取 NET_FlashingLightAlarmInfo_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetFlashingLightAlarmInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_FLASHING_LIGHT_ALARM_INFO, pCb);
}

/**
 * @brief 注册获取 PIR 告警配置的回调函数。
 * @author ITC
 * @param [in] pCb 用于填充 NET_PirAlarmInfo_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPirAlarmInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PIR_ALARM_INFO, pCb);
}

/**
 * @brief 注册设置 PIR 告警配置的回调函数。
 * @author ITC
 * @param [in] pCb 用于读取 NET_PirAlarmInfo_S 的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPirAlarmInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PIR_ALARM_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRecordSchedule(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_RECORD_SCHEDULE, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRecordSchedule(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_RECORD_SCHEDULE, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRecordAdvancedParam(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_RECORD_ADVANCED_PARAM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRecordAdvancedParam(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_RECORD_ADVANCED_PARAM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_FindRecordFileInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_FIND_RECORD_FILE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_DownloadRecordFile(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_DOWNLOAD_RECORD_FILE, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPrivacyMaskCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PRIVACYMASKCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPrivacyMaskCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PRIVACYMASKCFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetTamperAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_TAMPERALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTamperAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_TAMPERALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetMotionAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_MOTIONALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetMotionAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_MOTIONALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCrossLineAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CROSSLINEALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCrossLineAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CROSSLINEALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetIntrusionAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_INTRUSIONALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetIntrusionAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_INTRUSIONALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetEnterRegionAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_ENTERREGIONALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetEnterRegionAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_ENTERREGIONALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetLeaveRegionAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_LEAVEREGIONALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetLeaveRegionAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_LEAVEREGIONALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetLoiteringAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_LOITERINGALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetLoiteringAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_LOITERINGALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSceneChangeAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SCENECHANGEALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSceneChangeAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_SCENECHANGEALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCrowGatheringAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CROWDGATHERINGALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCrowGatheringAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CROWDGATHERINGALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetParkingAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PARKINGALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetParkingAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PARKINGALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetUnattendedObjectAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_UNATTENDEDOBJECTALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetUnattendedObjectAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_UNATTENDEDOBJECTALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetObjectRemovalAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_OBJECTREMOVALALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetObjectRemovalAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_OBJECTREMOVALALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetAudioAnomalyAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_AUDIOANOMALYALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetAudioAnomalyAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_AUDIOANOMALYALARM, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPreviewInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PREVIEW_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPreviewInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PREVIEW_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetUpgradeStatus(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_UPGRADESTATUS, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetUpgradeVersion(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_UPGRADEVERSION, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetUpgrade(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_UPGRADE, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCapturePlanInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CAPTURE_PLAN_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCapturePlanInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CAPTURE_PLAN_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCaptureParamInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CAPTURE_PARAM_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCaptureParamInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CAPTURE_PARAM_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetExposureInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_EXPOSURE_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetExposureInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_EXPOSURE_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDayNightInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_DAYNIGHT_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDayNightInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_DAYNIGHT_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetBackLightInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_BACKLIGHT_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetBackLightInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_BACKLIGHT_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDenoiseInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_DENOISE_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDenoiseInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_DENOISE_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetWhiteBalanceInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_WHITEBALANCE_INFO, pCb);
}
NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetWhiteBalanceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_WHITEBALANCE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTalkbackState(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_STATE_TALKBACK, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTalkbackToStream(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_TO_STREAM_TALKBACK, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetTalkbackFromStream(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_FROM_STREAM_TALKBACK, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetReplayTalkback(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_REPLAY_TALKBACK, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetChannelInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CHANNEL_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetChannelList(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CHANNEL_LIST, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetFaceCaptureInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_FACECAPTUREINFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetFaceCaptureInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_FACECAPTUREINFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetFaceCompareInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_FACE_COMPARE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_AddTargetLib(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_ADD_TARGET_LIB, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_DelTargetLib(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_DEL_TARGET_LIB, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetTargetLib(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_TARGET_LIB, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetTargetLib(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_TARGET_LIB, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_AddFaceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_ADD_FACE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_DelFaceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_DEL_FACE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetFaceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_FACE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetFaceInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_FACE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetGarbageExposureCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_GARBAGE_EXPOSURE_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetGarbageExposureCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_GARBAGE_EXPOSURE_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetGarbageOverflowCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_GARBAGE_OVERFLOW_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetGarbageOverflowCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_GARBAGE_OVERFLOW_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PEOPLE_FLOW_STATISTICS_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PEOPLE_FLOW_STATISTICS_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_ResetPeopleFlowStatistics(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_RESET_PEOPLE_FLOW_STATISTICS, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPeopleDensityDetectionCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PEOPLE_DENSITY_DETECTION_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPeopleDensityDetectionCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PEOPLE_DENSITY_DETECTION_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetManholeCoverAbnormalCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_MANHOLE_COVER_ABNORMAL_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetManholeCoverAbnormalCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_MANHOLE_COVER_ABNORMAL_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSleepOnDutyCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SLEEP_ON_DUTY_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSleepOnDutyCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_SLEEP_ON_DUTY_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPersonFallDownCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PERSON_FALL_DOWN_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPersonFallDownCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PERSON_FALL_DOWN_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetConstructionOccupyRoadCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetConstructionOccupyRoadCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetCongestionCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CONGESTION_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetCongestionCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CONGESTION_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetLicensePlateRecognitionCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_LICENSE_PLATE_RECOGNITION_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetLicensePlateRecognitionCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_LICENSE_PLATE_RECOGNITION_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_HIGH_ALTITUDE_SEATBELT_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_HIGH_ALTITUDE_SEATBELT_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSafetyHelmetCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SAFETY_HELMET_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSafetyHelmetCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_SAFETY_HELMET_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPersonFallCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PERSON_FALL_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPersonFallCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PERSON_FALL_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPhoneUsageCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PHONE_USAGE_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPhoneUsageCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PHONE_USAGE_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSmokingCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SMOKING_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSmokingCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_SMOKING_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetOpenFlameCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_OPEN_FLAME_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetOpenFlameCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_OPEN_FLAME_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetBareSoilCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_BARE_SOIL_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetBareSoilCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_BARE_SOIL_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetHoleProtectionBarCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_HOLE_PROTECTION_BAR_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetHoleProtectionBarCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_HOLE_PROTECTION_BAR_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetReflectiveClothingCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_REFLECTIVE_CLOTHING_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetReflectiveClothingCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_REFLECTIVE_CLOTHING_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPetRecognitionInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PET_RECOGNITION_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPetRecognitionInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PET_RECOGNITION_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetClimbFenceInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_CLIMB_FENCE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetClimbFenceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_CLIMB_FENCE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetDimissionInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_DIMISSION_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetDimissionInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_DIMISSION_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetIllegalLaneInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_ILLEGAL_LANE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetIllegalLaneInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_ILLEGAL_LANE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRetrogradeInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_RETROGRADE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRetrogradeInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_RETROGRADE_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetOccupationEmergencyInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_OCCUPATION_EMERGENCY_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetOccupationEmergencyInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_OCCUPATION_EMERGENCY_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetPedestrianIntrusionInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_PEDESTRIAN_INTRUSION_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetPedestrianIntrusionInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_PEDESTRIAN_INTRUSION_INFO, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetSmokeFireCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_SMOKE_FIRE_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetSmokeFireCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_SMOKE_FIRE_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_GetRoadPondingCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return Net_RegisterGetCmdCb(NET_GET_ROAD_PONDING_CFG, pCb);
}

NET_API BOOL STDCALL NET_SERVER_RegisterCb_SetRoadPondingCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return Net_RegisterSetCmdCb(NET_SET_ROAD_PONDING_CFG, pCb);
}

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

