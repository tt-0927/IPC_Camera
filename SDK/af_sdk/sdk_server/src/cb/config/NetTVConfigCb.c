/**
 * @file NetTVConfigCb.c
 * @brief Device config callback registration/execute
 */
#include <stdio.h>
#include <stddef.h>
#include "NetTVConfigCbExecute.h"
#include "NetTVSDKServerInterface.h"

typedef struct tagNETTVConfigCbTable
{
    NET_TV_CB_GetDevConfig cbGet;
    NET_TV_CB_SetDevConfig cbSet;
    NET_TV_CB_GetRtspUrl   cbGetRtspUrl;
    NET_TV_CB_GetReplayUrl cbGetReplayUrl;
    NET_TV_CB_ControlReplay cbControlReplay;
    NET_TV_CB_GetReplayRecordList cbGetReplayRecordList;
} NET_TV_CONFIG_CB_TABLE_S;

typedef struct tagNETTVConfigCmdCbItem
{
    INT32 nGetCommand;
    INT32 nSetCommand;
    NET_TV_CB_GetDevConfigByCommand cbGetByCmd;
    NET_TV_CB_SetDevConfigByCommand cbSetByCmd;
} NET_TV_CONFIG_CMD_CB_ITEM_S;

static NET_TV_CONFIG_CB_TABLE_S g_stConfigCbTable = {0};

static NET_TV_CONFIG_CMD_CB_ITEM_S g_astConfigCmdCbTable[] =
{
    {NET_TV_GET_DEVICECFG,              NET_TV_SET_DEVICECFG,               NULL,   NULL},
    {NET_TV_GET_NTPCFG,                 NET_TV_SET_NTPCFG,                  NULL,   NULL},
    {NET_TV_GET_STREAMCFG,              NET_TV_SET_STREAMCFG,               NULL,   NULL},
    {NET_TV_GET_UPGRADESTATUS,          NULL,                               NULL,   NULL},
    {NULL,                              NET_TV_SET_UPGRADE,                 NULL,   NULL},
    {NET_TV_GET_UPGRADEVERSION,         NULL,                               NULL,   NULL},
    {NET_TV_GET_RTSPURLCFG,             NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_GET_REPLAY_URLCFG,          NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_GET_OSDCAPCFG,              NET_TV_SET_OSDCAPCFG,               NULL,   NULL},
    {NET_TV_GET_IMAGECFG,               NET_TV_SET_IMAGECFG,                NULL,   NULL},
    {NET_TV_GET_AUDIOCFG,               NET_TV_SET_AUDIOCFG,                NULL,   NULL},
    {NET_TV_GET_NETWORKCFG,             NET_TV_SET_NETWORKCFG,              NULL,   NULL},
    {NET_TV_GET_ENTERREGIONALARM,       NET_TV_SET_ENTERREGIONALARM,        NULL,   NULL},
    {NET_TV_GET_LEAVEREGIONALARM,       NET_TV_SET_LEAVEREGIONALARM,        NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_SET_CONFIG_WIFI_STA,         NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_CONNECT_WIFI_STA,            NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_DISCONNECT_WIFI_STA,         NULL,   NULL},
    {NET_TV_GET_4G_INFO,                NET_TV_SET_4G_INFO,                 NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_SET_HOTSPOT_INFO,            NULL,   NULL},
    {NET_TV_GET_HOTSPOT_CONN,           NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_GET_SECURITY_SERVICES_INFO, NET_TV_SET_SECURITY_SERVICES_INFO,  NULL,   NULL},
    {NET_TV_GET_SSH_COUNTDOWN,          NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_FIND_LOG,                   NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_EXPORT_LOG,                 NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_GET_LOG_SERVER,             NET_TV_SET_LOG_SERVER,              NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_TEST_LOG_SERVER,             NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_CONTROL_RECORD_INFO,         NULL,   NULL},
    {NET_TV_GET_RECORD_STATUS,          NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_GET_RECORD_SCHEDULE,        NET_TV_SET_RECORD_SCHEDULE,         NULL,   NULL},
    {NET_TV_GET_RECORD_ADVANCED_PARAM,  NET_TV_SET_RECORD_ADVANCED_PARAM,   NULL,   NULL},
    {NET_TV_FIND_RECORD_FILE_INFO,      NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_DOWNLOAD_RECORD_FILE,        NULL,   NULL},
    {NET_TV_GET_PRIVACYMASKCFG,         NET_TV_SET_PRIVACYMASKCFG,          NULL,   NULL},
    {NET_TV_GET_TAMPERALARM,            NET_TV_SET_TAMPERALARM,             NULL,   NULL},
    {NET_TV_GET_MOTIONALARM,            NET_TV_SET_MOTIONALARM,             NULL,   NULL},
    {NET_TV_GET_CROSSLINEALARM,         NET_TV_SET_CROSSLINEALARM,          NULL,   NULL},
    {NET_TV_GET_INTRUSIONALARM,         NET_TV_SET_INTRUSIONALARM,          NULL,   NULL},
    {NET_TV_GET_SCENECHANGEALARM,       NET_TV_SET_SCENECHANGEALARM,        NULL,   NULL},
    {NET_TV_GET_CROWDGATHERINGALARM,    NET_TV_SET_CROWDGATHERINGALARM,     NULL,   NULL},
    {NET_TV_GET_GARBAGE_EXPOSURE_CFG,   NET_TV_SET_GARBAGE_EXPOSURE_CFG,    NULL,   NULL},
    {NET_TV_GET_GARBAGE_OVERFLOW_CFG,   NET_TV_SET_GARBAGE_OVERFLOW_CFG,    NULL,   NULL},
    {NET_TV_GET_LOITERINGALARM,         NET_TV_SET_LOITERINGALARM,          NULL,   NULL},
    {NET_TV_GET_CAPTURE_PLAN_INFO,      NET_TV_SET_CAPTURE_PLAN_INFO,       NULL,   NULL},
    {NET_TV_GET_CAPTURE_PARAM_INFO,     NET_TV_SET_CAPTURE_PARAM_INFO,      NULL,   NULL},
    {NET_TV_GET_EXPOSURE_INFO,          NET_TV_SET_EXPOSURE_INFO,           NULL,   NULL},
    {NET_TV_GET_DAYNIGHT_INFO,          NET_TV_SET_DAYNIGHT_INFO,           NULL,   NULL},
    {NET_TV_GET_BACKLIGHT_INFO,         NET_TV_SET_BACKLIGHT_INFO,          NULL,   NULL},
    {NET_TV_GET_DENOISE_INFO,           NET_TV_SET_DENOISE_INFO,            NULL,   NULL},
    {NET_TV_GET_WHITEBALANCE_INFO,      NET_TV_SET_WHITEBALANCE_INFO,       NULL,   NULL},
    {NET_TV_GET_AUDIOANOMALYALARM,      NET_TV_SET_AUDIOANOMALYALARM,       NULL,   NULL},
    {NET_TV_GET_PREVIEW_INFO,           NET_TV_SET_PREVIEW_INFO,            NULL,   NULL},
    {NET_TV_GET_CHANNEL_INFO,           NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_GET_CHANNEL_LIST,           NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_STATE_TALKBACK,              NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_TO_STREAM_TALKBACK,          NULL,   NULL},
    {NET_TV_FROM_STREAM_TALKBACK,       NET_TV_CFG_INVALID,                 NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_REPLAY_TALKBACK,             NULL,   NULL},
    {NET_TV_GET_PARKINGALARM,           NET_TV_SET_PARKINGALARM,            NULL,   NULL},
    {NET_TV_GET_UNATTENDEDOBJECTALARM,  NET_TV_SET_UNATTENDEDOBJECTALARM,   NULL,   NULL},
    {NET_TV_GET_OBJECTREMOVALALARM,     NET_TV_SET_OBJECTREMOVALALARM,      NULL,   NULL},
    {NET_TV_GET_FACECAPTUREINFO,        NET_TV_SET_FACECAPTUREINFO,         NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_SET_FACE_COMPARE_INFO,       NULL,   NULL},
    {NET_TV_GET_TARGET_LIB,             NET_TV_ADD_TARGET_LIB,              NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_DEL_TARGET_LIB,              NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_SET_TARGET_LIB,              NULL,   NULL},
    {NET_TV_GET_FACE_INFO,              NET_TV_ADD_FACE_INFO,               NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_DEL_FACE_INFO,               NULL,   NULL},
    {NET_TV_CFG_INVALID,                NET_TV_SET_FACE_INFO,               NULL,   NULL},
    {NET_TV_GET_PEOPLE_FLOW_STATISTICS_CFG,    NET_TV_SET_PEOPLE_FLOW_STATISTICS_CFG,    NULL,   NULL},
    {NET_TV_CFG_INVALID,                       NET_TV_RESET_PEOPLE_FLOW_STATISTICS,       NULL,   NULL},
    {NET_TV_GET_PEOPLE_DENSITY_DETECTION_CFG,  NET_TV_SET_PEOPLE_DENSITY_DETECTION_CFG,  NULL,   NULL},
    {NET_TV_GET_MANHOLE_COVER_ABNORMAL_CFG,    NET_TV_SET_MANHOLE_COVER_ABNORMAL_CFG,    NULL,   NULL},
    {NET_TV_GET_SLEEP_ON_DUTY_CFG,             NET_TV_SET_SLEEP_ON_DUTY_CFG,              NULL,   NULL},
    {NET_TV_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, NET_TV_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, NULL, NULL},
    {NET_TV_GET_PERSON_FALL_DOWN_CFG,          NET_TV_SET_PERSON_FALL_DOWN_CFG,           NULL,   NULL},
    {NET_TV_GET_CONSTRUCTION_OCCUPY_ROAD_CFG,  NET_TV_SET_CONSTRUCTION_OCCUPY_ROAD_CFG,   NULL,   NULL},
    {NET_TV_GET_CONGESTION_CFG,                NET_TV_SET_CONGESTION_CFG,                 NULL,   NULL},
    {NET_TV_GET_LICENSE_PLATE_RECOGNITION_CFG, NET_TV_SET_LICENSE_PLATE_RECOGNITION_CFG,  NULL,   NULL},
    {NET_TV_GET_HIGH_ALTITUDE_SEATBELT_CFG,    NET_TV_SET_HIGH_ALTITUDE_SEATBELT_CFG,     NULL,   NULL},
    {NET_TV_GET_SAFETY_HELMET_CFG,             NET_TV_SET_SAFETY_HELMET_CFG,              NULL,   NULL},
    {NET_TV_GET_PERSON_FALL_CFG,               NET_TV_SET_PERSON_FALL_CFG,                NULL,   NULL},
    {NET_TV_GET_PHONE_USAGE_CFG,               NET_TV_SET_PHONE_USAGE_CFG,                NULL,   NULL},
    {NET_TV_GET_SMOKING_CFG,                   NET_TV_SET_SMOKING_CFG,                    NULL,   NULL},
    {NET_TV_GET_OPEN_FLAME_CFG,                NET_TV_SET_OPEN_FLAME_CFG,                 NULL,   NULL},
    {NET_TV_GET_BARE_SOIL_CFG,                 NET_TV_SET_BARE_SOIL_CFG,                  NULL,   NULL},
    {NET_TV_GET_HOLE_PROTECTION_BAR_CFG,       NET_TV_SET_HOLE_PROTECTION_BAR_CFG,        NULL,   NULL},
    {NET_TV_GET_REFLECTIVE_CLOTHING_CFG,       NET_TV_SET_REFLECTIVE_CLOTHING_CFG,        NULL,   NULL},
    {NET_TV_GET_PET_RECOGNITION_INFO,          NET_TV_SET_PET_RECOGNITION_INFO,           NULL,   NULL},
    {NET_TV_GET_CLIMB_FENCE_INFO,              NET_TV_SET_CLIMB_FENCE_INFO,               NULL,   NULL},
    {NET_TV_GET_DIMISSION_INFO,                NET_TV_SET_DIMISSION_INFO,                 NULL,   NULL},
    {NET_TV_GET_ILLEGAL_LANE_INFO,             NET_TV_SET_ILLEGAL_LANE_INFO,              NULL,   NULL},
    {NET_TV_GET_RETROGRADE_INFO,               NET_TV_SET_RETROGRADE_INFO,                NULL,   NULL},
    {NET_TV_GET_NONMOTOR_VEHICLE_INTRUSION_INFO, NET_TV_SET_NONMOTOR_VEHICLE_INTRUSION_INFO, NULL, NULL},
    {NET_TV_GET_OCCUPATION_EMERGENCY_INFO,     NET_TV_SET_OCCUPATION_EMERGENCY_INFO,      NULL,   NULL},
    {NET_TV_GET_PEDESTRIAN_INTRUSION_INFO,     NET_TV_SET_PEDESTRIAN_INTRUSION_INFO,      NULL,   NULL},
    {NET_TV_GET_SMOKE_FIRE_CFG,                NET_TV_SET_SMOKE_FIRE_CFG,                 NULL,   NULL},
    {NET_TV_GET_ROAD_PONDING_CFG,              NET_TV_SET_ROAD_PONDING_CFG,               NULL,   NULL},
};

static NET_TV_CONFIG_CMD_CB_ITEM_S* NetTV_FindCmdCbItemByGetCommand(INT32 nCommand)
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

static NET_TV_CONFIG_CMD_CB_ITEM_S* NetTV_FindCmdCbItemBySetCommand(INT32 nCommand)
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

static BOOL NetTV_RegisterGetCmdCb(INT32 nCommand, NET_TV_CB_GetDevConfigByCommand pCb)
{
    NET_TV_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (pCb == NULL)
    {
        return FALSE;
    }

    pItem = NetTV_FindCmdCbItemByGetCommand(nCommand);
    if (pItem == NULL || pItem->cbGetByCmd != NULL)
    {
        return FALSE;
    }

    pItem->cbGetByCmd = pCb;
    return TRUE;
}

static BOOL NetTV_RegisterSetCmdCb(INT32 nCommand, NET_TV_CB_SetDevConfigByCommand pCb)
{
    NET_TV_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (pCb == NULL)
    {
        return FALSE;
    }

    pItem = NetTV_FindCmdCbItemBySetCommand(nCommand);
    if (pItem == NULL || pItem->cbSetByCmd != NULL)
    {
        return FALSE;
    }

    pItem->cbSetByCmd = pCb;
    return TRUE;
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDevConfig(NET_TV_CB_GetDevConfig pCb)
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

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDevConfig(NET_TV_CB_SetDevConfig pCb)
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

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRtspUrl(NET_TV_CB_GetRtspUrl pCb)
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

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetReplayUrl(NET_TV_CB_GetReplayUrl pCb)
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

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ControlReplay(NET_TV_CB_ControlReplay pCb)
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

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetReplayRecordList(NET_TV_CB_GetReplayRecordList pCb)
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

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDeviceCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_DEVICECFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDeviceCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_DEVICECFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetNtpCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_NTPCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetNtpCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_NTPCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetStreamCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_STREAMCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetStreamCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_STREAMCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOsdCapCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_OSDCAPCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOsdCapCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_OSDCAPCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetImageCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_IMAGECFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetImageCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_IMAGECFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetAudioCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_AUDIOCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetAudioCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_AUDIOCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetNetworkCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_NETWORKCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetNetworkCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_NETWORKCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetConfigWifiSta(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_CONFIG_WIFI_STA, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ConnectWifiSta(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_CONNECT_WIFI_STA, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DisconnectWifiSta(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_DISCONNECT_WIFI_STA, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_Get4GInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_4G_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_Set4GInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_4G_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetHotspotInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_HOTSPOT_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetHotspotConn(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_HOTSPOT_CONN, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSecurityServicesInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_SECURITY_SERVICES_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSecurityServicesInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_SECURITY_SERVICES_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSshCountdown(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_SSH_COUNTDOWN, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_FindLog(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_FIND_LOG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ExportLog(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_EXPORT_LOG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLogServer(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_LOG_SERVER, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLogServer(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_LOG_SERVER, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_TestLogServer(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_TEST_LOG_SERVER, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ControlRecordInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_CONTROL_RECORD_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRecordStatus(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_RECORD_STATUS, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRecordSchedule(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_RECORD_SCHEDULE, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRecordSchedule(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_RECORD_SCHEDULE, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRecordAdvancedParam(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_RECORD_ADVANCED_PARAM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRecordAdvancedParam(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_RECORD_ADVANCED_PARAM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_FindRecordFileInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_FIND_RECORD_FILE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DownloadRecordFile(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_DOWNLOAD_RECORD_FILE, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPrivacyMaskCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PRIVACYMASKCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPrivacyMaskCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PRIVACYMASKCFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetTamperAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_TAMPERALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTamperAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_TAMPERALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetMotionAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_MOTIONALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetMotionAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_MOTIONALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCrossLineAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_CROSSLINEALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCrossLineAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_CROSSLINEALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetIntrusionAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_INTRUSIONALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetIntrusionAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_INTRUSIONALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetEnterRegionAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_ENTERREGIONALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetEnterRegionAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_ENTERREGIONALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLeaveRegionAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_LEAVEREGIONALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLeaveRegionAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_LEAVEREGIONALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLoiteringAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_LOITERINGALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLoiteringAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_LOITERINGALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSceneChangeAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_SCENECHANGEALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSceneChangeAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_SCENECHANGEALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCrowGatheringAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_CROWDGATHERINGALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCrowGatheringAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_CROWDGATHERINGALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetParkingAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PARKINGALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetParkingAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PARKINGALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetUnattendedObjectAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_UNATTENDEDOBJECTALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetUnattendedObjectAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_UNATTENDEDOBJECTALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetObjectRemovalAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_OBJECTREMOVALALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetObjectRemovalAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_OBJECTREMOVALALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetAudioAnomalyAlarm(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_AUDIOANOMALYALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetAudioAnomalyAlarm(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_AUDIOANOMALYALARM, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPreviewInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PREVIEW_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPreviewInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PREVIEW_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetUpgradeStatus(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_UPGRADESTATUS, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetUpgradeVersion(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_UPGRADEVERSION, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetUpgrade(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_UPGRADE, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCapturePlanInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_CAPTURE_PLAN_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCapturePlanInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_CAPTURE_PLAN_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCaptureParamInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_CAPTURE_PARAM_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCaptureParamInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_CAPTURE_PARAM_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetExposureInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_EXPOSURE_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetExposureInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_EXPOSURE_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDayNightInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_DAYNIGHT_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDayNightInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_DAYNIGHT_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetBackLightInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_BACKLIGHT_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetBackLightInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_BACKLIGHT_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDenoiseInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_DENOISE_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDenoiseInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_DENOISE_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetWhiteBalanceInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_WHITEBALANCE_INFO, pCb);
}
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetWhiteBalanceInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_WHITEBALANCE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTalkbackState(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_STATE_TALKBACK, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTalkbackToStream(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_TO_STREAM_TALKBACK, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetTalkbackFromStream(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_FROM_STREAM_TALKBACK, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetReplayTalkback(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_REPLAY_TALKBACK, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetChannelInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_CHANNEL_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetChannelList(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_CHANNEL_LIST, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetFaceCaptureInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_FACECAPTUREINFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetFaceCaptureInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_FACECAPTUREINFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetFaceCompareInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_FACE_COMPARE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_AddTargetLib(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_ADD_TARGET_LIB, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DelTargetLib(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_DEL_TARGET_LIB, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTargetLib(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_TARGET_LIB, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetTargetLib(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_TARGET_LIB, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_AddFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_ADD_FACE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DelFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_DEL_FACE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_FACE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetFaceInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_FACE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetGarbageExposureCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_GARBAGE_EXPOSURE_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetGarbageExposureCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_GARBAGE_EXPOSURE_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetGarbageOverflowCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_GARBAGE_OVERFLOW_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetGarbageOverflowCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_GARBAGE_OVERFLOW_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PEOPLE_FLOW_STATISTICS_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PEOPLE_FLOW_STATISTICS_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ResetPeopleFlowStatistics(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_RESET_PEOPLE_FLOW_STATISTICS, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPeopleDensityDetectionCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PEOPLE_DENSITY_DETECTION_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPeopleDensityDetectionCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PEOPLE_DENSITY_DETECTION_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetManholeCoverAbnormalCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_MANHOLE_COVER_ABNORMAL_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetManholeCoverAbnormalCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_MANHOLE_COVER_ABNORMAL_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSleepOnDutyCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_SLEEP_ON_DUTY_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSleepOnDutyCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_SLEEP_ON_DUTY_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPersonFallDownCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PERSON_FALL_DOWN_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPersonFallDownCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PERSON_FALL_DOWN_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetConstructionOccupyRoadCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_CONSTRUCTION_OCCUPY_ROAD_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetConstructionOccupyRoadCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_CONSTRUCTION_OCCUPY_ROAD_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCongestionCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_CONGESTION_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCongestionCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_CONGESTION_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLicensePlateRecognitionCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_LICENSE_PLATE_RECOGNITION_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLicensePlateRecognitionCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_LICENSE_PLATE_RECOGNITION_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_HIGH_ALTITUDE_SEATBELT_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_HIGH_ALTITUDE_SEATBELT_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSafetyHelmetCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_SAFETY_HELMET_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSafetyHelmetCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_SAFETY_HELMET_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPersonFallCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PERSON_FALL_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPersonFallCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PERSON_FALL_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPhoneUsageCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PHONE_USAGE_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPhoneUsageCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PHONE_USAGE_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSmokingCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_SMOKING_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSmokingCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_SMOKING_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOpenFlameCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_OPEN_FLAME_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOpenFlameCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_OPEN_FLAME_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetBareSoilCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_BARE_SOIL_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetBareSoilCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_BARE_SOIL_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetHoleProtectionBarCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_HOLE_PROTECTION_BAR_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetHoleProtectionBarCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_HOLE_PROTECTION_BAR_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetReflectiveClothingCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_REFLECTIVE_CLOTHING_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetReflectiveClothingCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_REFLECTIVE_CLOTHING_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPetRecognitionInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PET_RECOGNITION_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PET_RECOGNITION_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetClimbFenceInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_CLIMB_FENCE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetClimbFenceInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_CLIMB_FENCE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDimissionInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_DIMISSION_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDimissionInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_DIMISSION_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetIllegalLaneInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_ILLEGAL_LANE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetIllegalLaneInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_ILLEGAL_LANE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRetrogradeInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_RETROGRADE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRetrogradeInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_RETROGRADE_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_NONMOTOR_VEHICLE_INTRUSION_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_NONMOTOR_VEHICLE_INTRUSION_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOccupationEmergencyInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_OCCUPATION_EMERGENCY_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOccupationEmergencyInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_OCCUPATION_EMERGENCY_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPedestrianIntrusionInfo(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_PEDESTRIAN_INTRUSION_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPedestrianIntrusionInfo(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_PEDESTRIAN_INTRUSION_INFO, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSmokeFireCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_SMOKE_FIRE_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSmokeFireCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_SMOKE_FIRE_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRoadPondingCfg(NET_TV_CB_GetDevConfigByCommand pCb)
{
    return NetTV_RegisterGetCmdCb(NET_TV_GET_ROAD_PONDING_CFG, pCb);
}

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRoadPondingCfg(NET_TV_CB_SetDevConfigByCommand pCb)
{
    return NetTV_RegisterSetCmdCb(NET_TV_SET_ROAD_PONDING_CFG, pCb);
}

int NetSDK_ExecuteCb_GetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpOutBuffer)
{
    NET_TV_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (lpOutBuffer == NULL)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    /* RTSP URL 获取：优先走专用回调 */
    if (dwCommand == NET_TV_GET_RTSPURLCFG && g_stConfigCbTable.cbGetRtspUrl != NULL)
    {
        return g_stConfigCbTable.cbGetRtspUrl(dwChannelID, (LPNET_TV_RTSP_URL_INFO_S)lpOutBuffer);
    }

    if (dwCommand == NET_TV_GET_REPLAY_URLCFG && g_stConfigCbTable.cbGetReplayUrl != NULL)
    {
        LPNET_TV_REPLAY_URL_INFO_S pInfo = (LPNET_TV_REPLAY_URL_INFO_S)lpOutBuffer;
        if (pInfo->dwChannel == 0)
        {
            pInfo->dwChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbGetReplayUrl(pInfo);
    }

    if (dwCommand == NET_TV_GET_REPLAY_RECORD_LIST && g_stConfigCbTable.cbGetReplayRecordList != NULL)
    {
        LPNET_TV_REPLAY_RECORD_LIST_S pInfo = (LPNET_TV_REPLAY_RECORD_LIST_S)lpOutBuffer;
        if (pInfo->dwChannel == 0)
        {
            pInfo->dwChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbGetReplayRecordList(pInfo);
    }
    if (dwCommand == NET_TV_GET_RTSPURLCFG && g_stConfigCbTable.cbGetRtspUrl != NULL)
    {
        return g_stConfigCbTable.cbGetRtspUrl(dwChannelID, (LPNET_TV_RTSP_URL_INFO_S)lpOutBuffer);
    }

    if (dwCommand == NET_TV_GET_REPLAY_URLCFG && g_stConfigCbTable.cbGetReplayUrl != NULL)
    {
        LPNET_TV_REPLAY_URL_INFO_S pInfo = (LPNET_TV_REPLAY_URL_INFO_S)lpOutBuffer;
        if (pInfo->dwChannel == 0)
        {
            pInfo->dwChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbGetReplayUrl(pInfo);
    }

    pItem = NetTV_FindCmdCbItemByGetCommand(dwCommand);
    if (pItem != NULL && pItem->cbGetByCmd != NULL)
    {
        return pItem->cbGetByCmd(dwChannelID, lpOutBuffer);
    }

    if (g_stConfigCbTable.cbGet != NULL)
    {
        return g_stConfigCbTable.cbGet(dwChannelID, dwCommand, lpOutBuffer);
    }

    printf("[NetTVConfigCb] GetDevConfig callback missing, cmd=%d, item=%p, cbGetByCmd=%p, cbGet=%p\n",
           dwCommand,
           (void*)pItem,
           pItem ? (void*)pItem->cbGetByCmd : NULL,
           (void*)g_stConfigCbTable.cbGet);
    return NET_TV_E_NONSUPPORT;
}

int NetSDK_ExecuteCb_GetReplayUrl(LPNET_TV_REPLAY_URL_INFO_S pInfo)
{
    if (pInfo == NULL)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    if (g_stConfigCbTable.cbGetReplayUrl != NULL)
    {
        return g_stConfigCbTable.cbGetReplayUrl(pInfo);
    }

    if (g_stConfigCbTable.cbGet != NULL)
    {
        return g_stConfigCbTable.cbGet(pInfo->dwChannel, NET_TV_GET_REPLAY_URLCFG, pInfo);
    }

    printf("[NetTVConfigCb] GetReplayUrl callback missing\n");
    return NET_TV_E_NONSUPPORT;
}

int NetSDK_ExecuteCb_ControlReplay(LPNET_TV_REPLAY_CTRL_INFO_S pInfo)
{
    if (pInfo == NULL)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    printf("[NetTVConfigCb] ControlReplay callback: channel=%d, ctrlType=%d, startTime=[%s], endTime=[%s], sessionId=[%s]\n",
           pInfo->dwChannel, pInfo->dwCtrlType, pInfo->szStartTime, pInfo->szEndTime, pInfo->szSessionId);

    if (g_stConfigCbTable.cbControlReplay != NULL)
    {
        return g_stConfigCbTable.cbControlReplay(pInfo);
    }

    if (g_stConfigCbTable.cbSet != NULL)
    {
        return g_stConfigCbTable.cbSet(pInfo->dwChannel, NET_TV_SET_REPLAY_CTRL, pInfo);
    }

    printf("[NetTVConfigCb] ControlReplay callback missing\n");
    return NET_TV_E_NONSUPPORT;
}

int NetSDK_ExecuteCb_GetReplayRecordList(LPNET_TV_REPLAY_RECORD_LIST_S pInfo)
{
    if (pInfo == NULL)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    if (g_stConfigCbTable.cbGetReplayRecordList != NULL)
    {
        return g_stConfigCbTable.cbGetReplayRecordList(pInfo);
    }

    if (g_stConfigCbTable.cbGet != NULL)
    {
        return g_stConfigCbTable.cbGet(pInfo->dwChannel, NET_TV_GET_REPLAY_RECORD_LIST, pInfo);
    }

    printf("[NetTVConfigCb] GetReplayRecordList callback missing\n");
    return NET_TV_E_NONSUPPORT;
}

int NetSDK_ExecuteCb_SetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpInBuffer)
{
    NET_TV_CONFIG_CMD_CB_ITEM_S* pItem = NULL;
    if (lpInBuffer == NULL)
    {
        return NET_TV_E_INVALID_PARAM;
    }

    if (dwCommand == NET_TV_SET_REPLAY_CTRL && g_stConfigCbTable.cbControlReplay != NULL)
    {
        LPNET_TV_REPLAY_CTRL_INFO_S pInfo = (LPNET_TV_REPLAY_CTRL_INFO_S)lpInBuffer;
        if (pInfo->dwChannel == 0)
        {
            pInfo->dwChannel = dwChannelID;
        }
        return g_stConfigCbTable.cbControlReplay(pInfo);
    }

    pItem = NetTV_FindCmdCbItemBySetCommand(dwCommand);
    if (pItem != NULL && pItem->cbSetByCmd != NULL)
    {
        return pItem->cbSetByCmd(dwChannelID, lpInBuffer);
    }

    if (g_stConfigCbTable.cbSet != NULL)
    {
        return g_stConfigCbTable.cbSet(dwChannelID, dwCommand, lpInBuffer);
    }

    return NET_TV_E_NONSUPPORT;
}

