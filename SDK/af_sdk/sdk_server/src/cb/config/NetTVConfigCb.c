/**
 * @file NetTVConfigCb.c
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetTVConfigCb 模块实现
 * 功能说明：
 * 1. 实现 NetTVConfigCb 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */

#include <stdio.h>
#include <stddef.h>
#include "NetTVConfigCbExecute.h"
#include "NetTVSDKServerInterface.h"

/**
 * @author tianl (tianl@kfb.cn)
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
 * @author tianl (tianl@kfb.cn)
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

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 命令码与回调映射表（新增配置项时需在此添加条目）
 * @note 格式：{Get命令码, Set命令码, Get回调指针(初始为NULL), Set回调指针(初始为NULL)}
 *       当某个命令码不存在时填 NET_CFG_INVALID
 */
static NET_CONFIG_CMD_CB_ITEM_S g_astConfigCmdCbTable[] =
{
    {NET_GET_DEVICECFG,              NET_SET_DEVICECFG,               NULL,   NULL},
    {NET_GET_NTPCFG,                 NET_SET_NTPCFG,                  NULL,   NULL},
    {NET_GET_STREAMCFG,              NET_SET_STREAMCFG,               NULL,   NULL},
    {NET_GET_UPGRADESTATUS,          NULL,                               NULL,   NULL},
    {NULL,                              NET_SET_UPGRADE,                 NULL,   NULL},
    {NET_GET_UPGRADEVERSION,         NULL,                               NULL,   NULL},
    {NET_GET_RTSPURLCFG,             NET_CFG_INVALID,                 NULL,   NULL},
    {NET_GET_REPLAY_URLCFG,          NET_CFG_INVALID,                 NULL,   NULL},
    {NET_GET_OSDCAPCFG,              NET_SET_OSDCAPCFG,               NULL,   NULL},
    {NET_GET_IMAGECFG,               NET_SET_IMAGECFG,                NULL,   NULL},
    {NET_GET_AUDIOCFG,               NET_SET_AUDIOCFG,                NULL,   NULL},
    {NET_GET_NETWORKCFG,             NET_SET_NETWORKCFG,              NULL,   NULL},
    {NET_GET_ENTERREGIONALARM,       NET_SET_ENTERREGIONALARM,        NULL,   NULL},
    {NET_GET_LEAVEREGIONALARM,       NET_SET_LEAVEREGIONALARM,        NULL,   NULL},
    {NET_CFG_INVALID,                NET_SET_CONFIG_WIFI_STA,         NULL,   NULL},
    {NET_CFG_INVALID,                NET_CONNECT_WIFI_STA,            NULL,   NULL},
    {NET_CFG_INVALID,                NET_DISCONNECT_WIFI_STA,         NULL,   NULL},
    {NET_GET_4G_INFO,                NET_SET_4G_INFO,                 NULL,   NULL},
    {NET_CFG_INVALID,                NET_SET_HOTSPOT_INFO,            NULL,   NULL},
    {NET_GET_HOTSPOT_CONN,           NET_CFG_INVALID,                 NULL,   NULL},
    {NET_GET_SECURITY_SERVICES_INFO, NET_SET_SECURITY_SERVICES_INFO,  NULL,   NULL},
    {NET_GET_SSH_COUNTDOWN,          NET_CFG_INVALID,                 NULL,   NULL},
    {NET_FIND_LOG,                   NET_CFG_INVALID,                 NULL,   NULL},
    {NET_EXPORT_LOG,                 NET_CFG_INVALID,                 NULL,   NULL},
    {NET_GET_LOG_SERVER,             NET_SET_LOG_SERVER,              NULL,   NULL},
    {NET_CFG_INVALID,                NET_TEST_LOG_SERVER,             NULL,   NULL},
    {NET_CFG_INVALID,                NET_CONTROL_RECORD_INFO,         NULL,   NULL},
    {NET_GET_RECORD_STATUS,          NET_CFG_INVALID,                 NULL,   NULL},
    {NET_GET_RECORD_SCHEDULE,        NET_SET_RECORD_SCHEDULE,         NULL,   NULL},
    {NET_GET_RECORD_ADVANCED_PARAM,  NET_SET_RECORD_ADVANCED_PARAM,   NULL,   NULL},
    {NET_FIND_RECORD_FILE_INFO,      NET_CFG_INVALID,                 NULL,   NULL},
    {NET_CFG_INVALID,                NET_DOWNLOAD_RECORD_FILE,        NULL,   NULL},
    {NET_GET_PRIVACYMASKCFG,         NET_SET_PRIVACYMASKCFG,          NULL,   NULL},
    {NET_GET_TAMPERALARM,            NET_SET_TAMPERALARM,             NULL,   NULL},
    {NET_GET_MOTIONALARM,            NET_SET_MOTIONALARM,             NULL,   NULL},
    {NET_GET_CROSSLINEALARM,         NET_SET_CROSSLINEALARM,          NULL,   NULL},
    {NET_GET_INTRUSIONALARM,         NET_SET_INTRUSIONALARM,          NULL,   NULL},
    {NET_GET_SCENECHANGEALARM,       NET_SET_SCENECHANGEALARM,        NULL,   NULL},
    {NET_GET_CROWDGATHERINGALARM,    NET_SET_CROWDGATHERINGALARM,     NULL,   NULL},
    {NET_GET_GARBAGE_EXPOSURE_CFG,   NET_SET_GARBAGE_EXPOSURE_CFG,    NULL,   NULL},
    {NET_GET_GARBAGE_OVERFLOW_CFG,   NET_SET_GARBAGE_OVERFLOW_CFG,    NULL,   NULL},
    {NET_GET_LOITERINGALARM,         NET_SET_LOITERINGALARM,          NULL,   NULL},
    {NET_GET_CAPTURE_PLAN_INFO,      NET_SET_CAPTURE_PLAN_INFO,       NULL,   NULL},
    {NET_GET_CAPTURE_PARAM_INFO,     NET_SET_CAPTURE_PARAM_INFO,      NULL,   NULL},
    {NET_GET_EXPOSURE_INFO,          NET_SET_EXPOSURE_INFO,           NULL,   NULL},
    {NET_GET_DAYNIGHT_INFO,          NET_SET_DAYNIGHT_INFO,           NULL,   NULL},
    {NET_GET_BACKLIGHT_INFO,         NET_SET_BACKLIGHT_INFO,          NULL,   NULL},
    {NET_GET_DENOISE_INFO,           NET_SET_DENOISE_INFO,            NULL,   NULL},
    {NET_GET_WHITEBALANCE_INFO,      NET_SET_WHITEBALANCE_INFO,       NULL,   NULL},
    {NET_GET_AUDIOANOMALYALARM,      NET_SET_AUDIOANOMALYALARM,       NULL,   NULL},
    {NET_GET_PREVIEW_INFO,           NET_SET_PREVIEW_INFO,            NULL,   NULL},
    {NET_GET_CHANNEL_INFO,           NET_CFG_INVALID,                 NULL,   NULL},
    {NET_GET_CHANNEL_LIST,           NET_CFG_INVALID,                 NULL,   NULL},
    {NET_CFG_INVALID,                NET_STATE_TALKBACK,              NULL,   NULL},
    {NET_CFG_INVALID,                NET_TO_STREAM_TALKBACK,          NULL,   NULL},
    {NET_FROM_STREAM_TALKBACK,       NET_CFG_INVALID,                 NULL,   NULL},
    {NET_CFG_INVALID,                NET_REPLAY_TALKBACK,             NULL,   NULL},
    {NET_GET_VOICECOM_AUDIO_CFG,     NET_SET_VOICECOM_AUDIO_CFG,      NULL,   NULL},
    {NET_GET_PARKINGALARM,           NET_SET_PARKINGALARM,            NULL,   NULL},
    {NET_GET_UNATTENDEDOBJECTALARM,  NET_SET_UNATTENDEDOBJECTALARM,   NULL,   NULL},
    {NET_GET_OBJECTREMOVALALARM,     NET_SET_OBJECTREMOVALALARM,      NULL,   NULL},
    {NET_GET_FACECAPTUREINFO,        NET_SET_FACECAPTUREINFO,         NULL,   NULL},
    {NET_CFG_INVALID,                NET_SET_FACE_COMPARE_INFO,       NULL,   NULL},
    {NET_GET_TARGET_LIB,             NET_ADD_TARGET_LIB,              NULL,   NULL},
    {NET_CFG_INVALID,                NET_DEL_TARGET_LIB,              NULL,   NULL},
    {NET_CFG_INVALID,                NET_SET_TARGET_LIB,              NULL,   NULL},
    {NET_GET_FACE_INFO,              NET_ADD_FACE_INFO,               NULL,   NULL},
    {NET_CFG_INVALID,                NET_DEL_FACE_INFO,               NULL,   NULL},
    {NET_CFG_INVALID,                NET_SET_FACE_INFO,               NULL,   NULL},
    {NET_GET_PEOPLE_FLOW_STATISTICS_CFG,    NET_SET_PEOPLE_FLOW_STATISTICS_CFG,    NULL,   NULL},
    {NET_CFG_INVALID,                       NET_RESET_PEOPLE_FLOW_STATISTICS,       NULL,   NULL},
    {NET_GET_PEOPLE_DENSITY_DETECTION_CFG,  NET_SET_PEOPLE_DENSITY_DETECTION_CFG,  NULL,   NULL},
    {NET_GET_MANHOLE_COVER_ABNORMAL_CFG,    NET_SET_MANHOLE_COVER_ABNORMAL_CFG,    NULL,   NULL},
    {NET_GET_SLEEP_ON_DUTY_CFG,             NET_SET_SLEEP_ON_DUTY_CFG,              NULL,   NULL},
    {NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, NULL, NULL},
    {NET_GET_PERSON_FALL_DOWN_CFG,          NET_SET_PERSON_FALL_DOWN_CFG,           NULL,   NULL},
    {NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG,  NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG,   NULL,   NULL},
    {NET_GET_CONGESTION_CFG,                NET_SET_CONGESTION_CFG,                 NULL,   NULL},
    {NET_GET_LICENSE_PLATE_RECOGNITION_CFG, NET_SET_LICENSE_PLATE_RECOGNITION_CFG,  NULL,   NULL},
    {NET_GET_HIGH_ALTITUDE_SEATBELT_CFG,    NET_SET_HIGH_ALTITUDE_SEATBELT_CFG,     NULL,   NULL},
    {NET_GET_SAFETY_HELMET_CFG,             NET_SET_SAFETY_HELMET_CFG,              NULL,   NULL},
    {NET_GET_PERSON_FALL_CFG,               NET_SET_PERSON_FALL_CFG,                NULL,   NULL},
    {NET_GET_PHONE_USAGE_CFG,               NET_SET_PHONE_USAGE_CFG,                NULL,   NULL},
    {NET_GET_SMOKING_CFG,                   NET_SET_SMOKING_CFG,                    NULL,   NULL},
    {NET_GET_OPEN_FLAME_CFG,                NET_SET_OPEN_FLAME_CFG,                 NULL,   NULL},
    {NET_GET_BARE_SOIL_CFG,                 NET_SET_BARE_SOIL_CFG,                  NULL,   NULL},
    {NET_GET_HOLE_PROTECTION_BAR_CFG,       NET_SET_HOLE_PROTECTION_BAR_CFG,        NULL,   NULL},
    {NET_GET_REFLECTIVE_CLOTHING_CFG,       NET_SET_REFLECTIVE_CLOTHING_CFG,        NULL,   NULL},
    {NET_GET_PET_RECOGNITION_INFO,          NET_SET_PET_RECOGNITION_INFO,           NULL,   NULL},
    {NET_GET_CLIMB_FENCE_INFO,              NET_SET_CLIMB_FENCE_INFO,               NULL,   NULL},
    {NET_GET_DIMISSION_INFO,                NET_SET_DIMISSION_INFO,                 NULL,   NULL},
    {NET_GET_ILLEGAL_LANE_INFO,             NET_SET_ILLEGAL_LANE_INFO,              NULL,   NULL},
    {NET_GET_RETROGRADE_INFO,               NET_SET_RETROGRADE_INFO,                NULL,   NULL},
    {NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO, NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO, NULL, NULL},
    {NET_GET_OCCUPATION_EMERGENCY_INFO,     NET_SET_OCCUPATION_EMERGENCY_INFO,      NULL,   NULL},
    {NET_GET_PEDESTRIAN_INTRUSION_INFO,     NET_SET_PEDESTRIAN_INTRUSION_INFO,      NULL,   NULL},
    {NET_GET_SMOKE_FIRE_CFG,                NET_SET_SMOKE_FIRE_CFG,                 NULL,   NULL},
    {NET_GET_ROAD_PONDING_CFG,              NET_SET_ROAD_PONDING_CFG,               NULL,   NULL},
};

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 根据Get命令码查找回调条目
 * @param [in] nCommand Get命令码
 * @return 找到返回条目指针，未找到返回NULL
 */
static NET_CONFIG_CMD_CB_ITEM_S* NetTV_FindCmdCbItemByGetCommand(INT32 nCommand)
{
    UINT32 i = 0;
    UINT32 nCount = (UINT32)(sizeof(g_astConfigCmdCbTable) / sizeof(g_astConfigCmdCbTable[0]));
    for (i = 0; i < nCount; ++i)
    {
        if (g_astConfigCmdCbTable[i].nGetCommand == nCommand)
        {
            return &g_astConfigCmdCbTable[i];
        }
    }
    return NULL;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 根据Set命令码查找回调条目
 * @param [in] nCommand Set命令码
 * @return 找到返回条目指针，未找到返回NULL
 */
static NET_CONFIG_CMD_CB_ITEM_S* NetTV_FindCmdCbItemBySetCommand(INT32 nCommand)
{
    UINT32 i = 0;
    UINT32 nCount = (UINT32)(sizeof(g_astConfigCmdCbTable) / sizeof(g_astConfigCmdCbTable[0]));
    for (i = 0; i < nCount; ++i)
    {
        if (g_astConfigCmdCbTable[i].nSetCommand == nCommand)
        {
            return &g_astConfigCmdCbTable[i];
        }
    }
    return NULL;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册按命令码分发的Get回调
 * @param [in] nCommand Get命令码
 * @param [in] pCb 回调函数指针
 * @return 注册成功返回TRUE，失败返回FALSE（命令码不存在或已注册）
 * @note 必须先在 g_astConfigCmdCbTable 数组中添加对应命令码条目
 */
static BOOL NetTV_RegisterGetCmdCb(INT32 nCommand, NET_CB_GetDevConfigByCommand pCb)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    pItem = NetTV_FindCmdCbItemByGetCommand(nCommand);
    if (pItem == NULL || pItem->cbGetByCmd != NULL)
    {
        return NET_FALSE;
    }

    pItem->cbGetByCmd = pCb;
    return NET_TRUE;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 注册按命令码分发的Set回调
 * @param [in] nCommand Set命令码
 * @param [in] pCb 回调函数指针
 * @return 注册成功返回TRUE，失败返回FALSE（命令码不存在或已注册）
 * @note 必须先在 g_astConfigCmdCbTable 数组中添加对应命令码条目
 */
static BOOL NetTV_RegisterSetCmdCb(INT32 nCommand, NET_CB_SetDevConfigByCommand pCb)
{
    NET_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    pItem = NetTV_FindCmdCbItemBySetCommand(nCommand);
    if (pItem == NULL || pItem->cbSetByCmd != NULL)
    {
        return NET_FALSE;
    }

    pItem->cbSetByCmd = pCb;
    return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetDevConfig 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetDevConfig(NET_CB_GetDevConfig pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    if (g_stConfigCbTable.cbGet != NULL)
    {
        return NET_FALSE;
    }

    g_stConfigCbTable.cbGet = pCb;
    return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetDevConfig 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetDevConfig(NET_CB_SetDevConfig pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    if (g_stConfigCbTable.cbSet != NULL)
    {
        return NET_FALSE;
    }

    g_stConfigCbTable.cbSet = pCb;
    return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetRtspUrl 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetRtspUrl(NET_CB_GetRtspUrl pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    if (g_stConfigCbTable.cbGetRtspUrl != NULL)
    {
        return NET_FALSE;
    }

    g_stConfigCbTable.cbGetRtspUrl = pCb;
    return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetReplayUrl 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetReplayUrl(NET_CB_GetReplayUrl pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    if (g_stConfigCbTable.cbGetReplayUrl != NULL)
    {
        return NET_FALSE;
    }

    g_stConfigCbTable.cbGetReplayUrl = pCb;
    return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_ControlReplay 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_ControlReplay(NET_CB_ControlReplay pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    if (g_stConfigCbTable.cbControlReplay != NULL)
    {
        return NET_FALSE;
    }

    g_stConfigCbTable.cbControlReplay = pCb;
    return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetReplayRecordList 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetReplayRecordList(NET_CB_GetReplayRecordList pCb)
{
    if (pCb == NULL)
    {
        return NET_FALSE;
    }

    if (g_stConfigCbTable.cbGetReplayRecordList != NULL)
    {
        return NET_FALSE;
    }

    g_stConfigCbTable.cbGetReplayRecordList = pCb;
    return NET_TRUE;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetDeviceCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetDeviceCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_DEVICECFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetDeviceCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetDeviceCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_DEVICECFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetNtpCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetNtpCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_NTPCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetNtpCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetNtpCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_NTPCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetStreamCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetStreamCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_STREAMCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetStreamCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetStreamCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_STREAMCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetOsdCapCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetOsdCapCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_OSDCAPCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetOsdCapCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetOsdCapCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_OSDCAPCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetImageCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetImageCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_IMAGECFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetImageCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetImageCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_IMAGECFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetAudioCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetAudioCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_AUDIOCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetAudioCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetAudioCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_AUDIOCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetNetworkCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetNetworkCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_NETWORKCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetNetworkCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetNetworkCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_NETWORKCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetConfigWifiSta 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetConfigWifiSta(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_CONFIG_WIFI_STA, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_ConnectWifiSta 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_ConnectWifiSta(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_CONNECT_WIFI_STA, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_DisconnectWifiSta 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_DisconnectWifiSta(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_DISCONNECT_WIFI_STA, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_Get4GInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_Get4GInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_4G_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_Set4GInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_Set4GInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_4G_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetHotspotInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetHotspotInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_HOTSPOT_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetHotspotConn 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetHotspotConn(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_HOTSPOT_CONN, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetSecurityServicesInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetSecurityServicesInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_SECURITY_SERVICES_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetSecurityServicesInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetSecurityServicesInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_SECURITY_SERVICES_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetSshCountdown 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetSshCountdown(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_SSH_COUNTDOWN, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_FindLog 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_FindLog(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_FIND_LOG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_ExportLog 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_ExportLog(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_EXPORT_LOG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetLogServer 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetLogServer(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_LOG_SERVER, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetLogServer 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetLogServer(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_LOG_SERVER, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_TestLogServer 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_TestLogServer(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TEST_LOG_SERVER, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_ControlRecordInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_ControlRecordInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_CONTROL_RECORD_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetRecordStatus 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetRecordStatus(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_RECORD_STATUS, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetRecordSchedule 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetRecordSchedule(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_RECORD_SCHEDULE, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetRecordSchedule 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetRecordSchedule(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_RECORD_SCHEDULE, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetRecordAdvancedParam 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetRecordAdvancedParam(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_RECORD_ADVANCED_PARAM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetRecordAdvancedParam 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetRecordAdvancedParam(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_RECORD_ADVANCED_PARAM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_FindRecordFileInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_FindRecordFileInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_FIND_RECORD_FILE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_DownloadRecordFile 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_DownloadRecordFile(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_DOWNLOAD_RECORD_FILE, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetPrivacyMaskCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetPrivacyMaskCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PRIVACYMASKCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetPrivacyMaskCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetPrivacyMaskCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PRIVACYMASKCFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetTamperAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetTamperAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_TAMPERALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetTamperAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetTamperAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_TAMPERALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetMotionAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetMotionAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_MOTIONALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetMotionAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetMotionAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_MOTIONALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetCrossLineAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetCrossLineAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_CROSSLINEALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetCrossLineAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetCrossLineAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_CROSSLINEALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetIntrusionAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetIntrusionAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_INTRUSIONALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetIntrusionAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetIntrusionAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_INTRUSIONALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetEnterRegionAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetEnterRegionAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_ENTERREGIONALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetEnterRegionAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetEnterRegionAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_ENTERREGIONALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetLeaveRegionAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetLeaveRegionAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_LEAVEREGIONALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetLeaveRegionAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetLeaveRegionAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_LEAVEREGIONALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetLoiteringAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetLoiteringAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_LOITERINGALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetLoiteringAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetLoiteringAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_LOITERINGALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetSceneChangeAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetSceneChangeAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_SCENECHANGEALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetSceneChangeAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetSceneChangeAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_SCENECHANGEALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetCrowGatheringAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetCrowGatheringAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_CROWDGATHERINGALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetCrowGatheringAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetCrowGatheringAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_CROWDGATHERINGALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetParkingAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetParkingAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PARKINGALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetParkingAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetParkingAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PARKINGALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetUnattendedObjectAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetUnattendedObjectAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_UNATTENDEDOBJECTALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetUnattendedObjectAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetUnattendedObjectAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_UNATTENDEDOBJECTALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetObjectRemovalAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetObjectRemovalAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_OBJECTREMOVALALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetObjectRemovalAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetObjectRemovalAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_OBJECTREMOVALALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetAudioAnomalyAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetAudioAnomalyAlarm(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_AUDIOANOMALYALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetAudioAnomalyAlarm 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetAudioAnomalyAlarm(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_AUDIOANOMALYALARM, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetPreviewInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetPreviewInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PREVIEW_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetPreviewInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetPreviewInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PREVIEW_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetUpgradeStatus 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetUpgradeStatus(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_UPGRADESTATUS, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetUpgradeVersion 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetUpgradeVersion(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_UPGRADEVERSION, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetUpgrade 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetUpgrade(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_UPGRADE, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetCapturePlanInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetCapturePlanInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_CAPTURE_PLAN_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetCapturePlanInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetCapturePlanInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_CAPTURE_PLAN_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetCaptureParamInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetCaptureParamInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_CAPTURE_PARAM_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetCaptureParamInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetCaptureParamInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_CAPTURE_PARAM_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetExposureInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetExposureInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_EXPOSURE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetExposureInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetExposureInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_EXPOSURE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetDayNightInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetDayNightInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_DAYNIGHT_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetDayNightInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetDayNightInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_DAYNIGHT_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetBackLightInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetBackLightInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_BACKLIGHT_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetBackLightInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetBackLightInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_BACKLIGHT_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetDenoiseInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetDenoiseInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_DENOISE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetDenoiseInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetDenoiseInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_DENOISE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetWhiteBalanceInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetWhiteBalanceInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_WHITEBALANCE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetWhiteBalanceInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetWhiteBalanceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_WHITEBALANCE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetTalkbackState 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetTalkbackState(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_STATE_TALKBACK, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetTalkbackToStream 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetTalkbackToStream(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TO_STREAM_TALKBACK, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetTalkbackFromStream 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetTalkbackFromStream(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_FROM_STREAM_TALKBACK, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetReplayTalkback 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetReplayTalkback(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_REPLAY_TALKBACK, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetChannelInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetChannelInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_CHANNEL_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetChannelList 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetChannelList(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_CHANNEL_LIST, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetFaceCaptureInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetFaceCaptureInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_FACECAPTUREINFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetFaceCaptureInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetFaceCaptureInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_FACECAPTUREINFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetFaceCompareInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetFaceCompareInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_FACE_COMPARE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_AddTargetLib 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_AddTargetLib(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_ADD_TARGET_LIB, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_DelTargetLib 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_DelTargetLib(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_DEL_TARGET_LIB, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetTargetLib 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetTargetLib(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_TARGET_LIB, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetTargetLib 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetTargetLib(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_TARGET_LIB, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_AddFaceInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_AddFaceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_ADD_FACE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_DelFaceInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_DelFaceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_DEL_FACE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetFaceInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetFaceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_FACE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetFaceInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetFaceInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_FACE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetGarbageExposureCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetGarbageExposureCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_GARBAGE_EXPOSURE_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetGarbageExposureCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetGarbageExposureCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_GARBAGE_EXPOSURE_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetGarbageOverflowCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetGarbageOverflowCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_GARBAGE_OVERFLOW_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetGarbageOverflowCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetGarbageOverflowCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_GARBAGE_OVERFLOW_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PEOPLE_FLOW_STATISTICS_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PEOPLE_FLOW_STATISTICS_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_ResetPeopleFlowStatistics 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_ResetPeopleFlowStatistics(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_RESET_PEOPLE_FLOW_STATISTICS, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetPeopleDensityDetectionCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetPeopleDensityDetectionCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PEOPLE_DENSITY_DETECTION_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetPeopleDensityDetectionCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetPeopleDensityDetectionCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PEOPLE_DENSITY_DETECTION_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetManholeCoverAbnormalCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetManholeCoverAbnormalCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_MANHOLE_COVER_ABNORMAL_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetManholeCoverAbnormalCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetManholeCoverAbnormalCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_MANHOLE_COVER_ABNORMAL_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetSleepOnDutyCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetSleepOnDutyCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_SLEEP_ON_DUTY_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetSleepOnDutyCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetSleepOnDutyCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_SLEEP_ON_DUTY_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetPersonFallDownCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetPersonFallDownCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PERSON_FALL_DOWN_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetPersonFallDownCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetPersonFallDownCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PERSON_FALL_DOWN_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetConstructionOccupyRoadCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetConstructionOccupyRoadCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetConstructionOccupyRoadCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetConstructionOccupyRoadCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetCongestionCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetCongestionCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_CONGESTION_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetCongestionCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetCongestionCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_CONGESTION_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetLicensePlateRecognitionCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetLicensePlateRecognitionCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_LICENSE_PLATE_RECOGNITION_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetLicensePlateRecognitionCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetLicensePlateRecognitionCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_LICENSE_PLATE_RECOGNITION_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_HIGH_ALTITUDE_SEATBELT_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_HIGH_ALTITUDE_SEATBELT_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetSafetyHelmetCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetSafetyHelmetCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_SAFETY_HELMET_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetSafetyHelmetCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetSafetyHelmetCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_SAFETY_HELMET_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetPersonFallCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetPersonFallCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PERSON_FALL_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetPersonFallCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetPersonFallCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PERSON_FALL_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetPhoneUsageCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetPhoneUsageCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PHONE_USAGE_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetPhoneUsageCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetPhoneUsageCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PHONE_USAGE_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetSmokingCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetSmokingCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_SMOKING_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetSmokingCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetSmokingCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_SMOKING_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetOpenFlameCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetOpenFlameCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_OPEN_FLAME_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetOpenFlameCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetOpenFlameCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_OPEN_FLAME_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetBareSoilCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetBareSoilCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_BARE_SOIL_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetBareSoilCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetBareSoilCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_BARE_SOIL_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetHoleProtectionBarCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetHoleProtectionBarCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_HOLE_PROTECTION_BAR_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetHoleProtectionBarCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetHoleProtectionBarCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_HOLE_PROTECTION_BAR_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetReflectiveClothingCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetReflectiveClothingCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_REFLECTIVE_CLOTHING_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetReflectiveClothingCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetReflectiveClothingCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_REFLECTIVE_CLOTHING_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetPetRecognitionInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetPetRecognitionInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PET_RECOGNITION_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetPetRecognitionInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetPetRecognitionInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PET_RECOGNITION_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetClimbFenceInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetClimbFenceInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_CLIMB_FENCE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetClimbFenceInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetClimbFenceInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_CLIMB_FENCE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetDimissionInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetDimissionInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_DIMISSION_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetDimissionInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetDimissionInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_DIMISSION_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetIllegalLaneInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetIllegalLaneInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_ILLEGAL_LANE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetIllegalLaneInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetIllegalLaneInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_ILLEGAL_LANE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetRetrogradeInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetRetrogradeInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_RETROGRADE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetRetrogradeInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetRetrogradeInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_RETROGRADE_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetOccupationEmergencyInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetOccupationEmergencyInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_OCCUPATION_EMERGENCY_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetOccupationEmergencyInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetOccupationEmergencyInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_OCCUPATION_EMERGENCY_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetPedestrianIntrusionInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetPedestrianIntrusionInfo(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_PEDESTRIAN_INTRUSION_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetPedestrianIntrusionInfo 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetPedestrianIntrusionInfo(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_PEDESTRIAN_INTRUSION_INFO, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetSmokeFireCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetSmokeFireCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_SMOKE_FIRE_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetSmokeFireCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetSmokeFireCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_SMOKE_FIRE_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_GetRoadPondingCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_GetRoadPondingCfg(NET_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_GET_ROAD_PONDING_CFG, pCb);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 NET_SERVER_RegisterCb_SetRoadPondingCfg 定义的内部处理。
 * @param [in] pCb 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

NET_API BOOL NET_STDCALL NET_SERVER_RegisterCb_SetRoadPondingCfg(NET_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_SET_ROAD_PONDING_CFG, pCb);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行配置获取回调（核心分发函数）
 * @param [in] dwChannelID 通道号
 * @param [in] dwCommand 命令码（标识配置类型）
 * @param [out] lpOutBuffer 输出缓冲区，用于存放配置数据
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
        if (pInfo->dwChannel == 0)
        {
            pInfo->dwChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbGetReplayUrl(pInfo);
    }

    /* 回放录像列表获取：优先走专用回调 */
    if (dwCommand == NET_GET_REPLAY_RECORD_LIST && g_stConfigCbTable.cbGetReplayRecordList != NULL)
    {
        pNET_ReplayRecordList_S pInfo = (pNET_ReplayRecordList_S)lpOutBuffer;
        if (pInfo->dwChannel == 0)
        {
            pInfo->dwChannel = dwChannelID;
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
        if (pInfo->dwChannel == 0)
        {
            pInfo->dwChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbGetReplayUrl(pInfo);
    }

    /* 查找按命令码注册的专用回调 */
    pItem = NetTV_FindCmdCbItemByGetCommand(dwCommand);
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
 * @author tianl (tianl@kfb.cn)
 * @brief 执行回放URL获取回调
 * @param [in,out] pInfo 回放查询条件和播放URL返回信息
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
        return g_stConfigCbTable.cbGet(pInfo->dwChannel, NET_GET_REPLAY_URLCFG, pInfo);
    }

    printf("[NetTVConfigCb] GetReplayUrl callback missing\n");
    return NET_E_NONSUPPORT;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行回放控制回调
 * @param [in,out] pInfo 回放控制输入输出参数
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
           pInfo->dwChannel, pInfo->dwCtrlType, pInfo->szStartTime, pInfo->szEndTime, pInfo->szSessionId);

    /* 优先使用专用回放控制回调 */
    if (g_stConfigCbTable.cbControlReplay != NULL)
    {
        return g_stConfigCbTable.cbControlReplay(pInfo);
    }

    /* 降级到通用配置设置回调 */
    if (g_stConfigCbTable.cbSet != NULL)
    {
        return g_stConfigCbTable.cbSet(pInfo->dwChannel, NET_SET_REPLAY_CTRL, pInfo);
    }

    printf("[NetTVConfigCb] ControlReplay callback missing\n");
    return NET_E_NONSUPPORT;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行回放录像列表获取回调
 * @param [in,out] pInfo 查询条件及结果
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
        return g_stConfigCbTable.cbGet(pInfo->dwChannel, NET_GET_REPLAY_RECORD_LIST, pInfo);
    }

    printf("[NetTVConfigCb] GetReplayRecordList callback missing\n");
    return NET_E_NONSUPPORT;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行配置设置回调（核心分发函数）
 * @param [in] dwChannelID 通道号
 * @param [in] dwCommand 命令码（标识配置类型）
 * @param [in] lpInBuffer 输入缓冲区，包含要设置的配置数据
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
        if (pInfo->dwChannel == 0)
        {
            pInfo->dwChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbControlReplay(pInfo);
    }

    /* 查找按命令码注册的专用回调 */
    pItem = NetTV_FindCmdCbItemBySetCommand(dwCommand);
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

