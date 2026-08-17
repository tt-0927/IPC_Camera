/**
 * @file main.c
 * @brief SDK服务端 配置(Get/Set) Demo
 *
 * 演示内容：
 * 1. 注册设备信息回调（用于客户端登录）
 * 2. 注册设备基本信息/网络配置的 Get/Set 配置回调
 * 3. 启动 SDK 服务端，等待客户端通过 NET_GetDevConfig/NET_SetDevConfig 访问配置
 *
 * 说明：
 * - 本 Demo 仅在内存中保存配置，不做持久化存储
 * - 主要用于演示配置获取与设置的调用流程
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "NetSdkLog.h"
#include "NetTVSDKServerInterface.h"

/* 日志配置 */
#define MAX_LOG_SIZE  (20 * 1024 * 1024)
#define MAX_LOG_FILES (10)

/* 服务端口与默认账号 */
#define SDKSERVER_PORT 9888
#define SDKSERVER_USERNAME "admin"
#define SDKSERVER_PASSWORD "sj2@2025"
#define DEMO_VOICECOM_PORT 9006
#define DEMO_VOICECOM_SERVER_RECV_DUMP "/tmp/VoiceComServerRecv.audio"
/* 当前IPC能力只开放前4个自定义OSD槽位，结构体数组长度仍按SDK ABI保留。 */
#define DEMO_OSD_CUSTOM_MAX_NUM NET_OSD_CUSTOM_MAX_NUM
#define DEMO_OSD_STRUCT_SLOT_NUM NET_OSD_TYPE_MAX_NUM
#define DEMO_ALARM_CHANNEL_INDEX 0
#define DEMO_ALARM_OUTPUT_DELAY_SECONDS 5

static INT32 g_serverPort = SDKSERVER_PORT;
static CHAR g_serverUsername[NET_LEN_132] = SDKSERVER_USERNAME;
static CHAR g_serverPassword[NET_LEN_132] = SDKSERVER_PASSWORD;

/* 运行标志，用于优雅退出 */
static volatile sig_atomic_t g_running = 1;
static unsigned long long g_voiceComFrameCount = 0;
static unsigned long long g_voiceComBytes = 0;
static FILE* g_voiceComDumpFp = NULL;

static unsigned char VoiceComSilenceByte(INT32 enFormat)
{
    if (enFormat == NET_AUDIO_FORMAT_G711A)
    {
        return 0xD5;
    }
    if (enFormat == NET_AUDIO_FORMAT_G711U)
    {
        return 0xFF;
    }
    return 0x00;
}

static void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        printf("\n[ConfigServerDemo] Received signal %d, exiting...\n", signum);
        g_running = 0;
    }
}

static void CopyString(char* pDst, size_t dstSize, const char* pSrc)
{
    if (!pDst || dstSize == 0)
    {
        return;
    }

    pDst[0] = '\0';
    if (!pSrc)
    {
        return;
    }

    strncpy(pDst, pSrc, dstSize - 1);
    pDst[dstSize - 1] = '\0';
}

static void PrintUsage(const char* pProgram)
{
    printf("Usage: %s [port] [username] [password]\n",
           pProgram ? pProgram : "ConfigServerDemo");
    printf("Example: %s 9888 admin sj2@2025\n",
           pProgram ? pProgram : "ConfigServerDemo");
}

static void ConfigureByArgs(int argc, char* argv[])
{
    if (argc > 4)
    {
        PrintUsage(argv[0]);
    }

    if (argc > 1)
    {
        int port = atoi(argv[1]);
        if (port > 0)
        {
            g_serverPort = port;
        }
    }

    if (argc > 2)
    {
        CopyString(g_serverUsername, sizeof(g_serverUsername), argv[2]);
    }

    if (argc > 3)
    {
        CopyString(g_serverPassword, sizeof(g_serverPassword), argv[3]);
    }
}

/* ====================== 全局配置数据（仅示例用，存放在内存） ====================== */

static NET_DeviceBasicInfo_S g_stDeviceBasicInfo;
static NET_NetworkCfg_S   g_stNetworkCfg;
static NET_SystemNtpInfo_S   g_stSystemNtpCfg;
static NET_VideoEncodeOption_S g_stStreamCfg;
static NET_AudioCfg_S        g_stAudioCfg;
static NET_WifiStaCfg_S     g_stWifiStaCfg;
static NET_WifiStaConnect_S g_stWifiStaConnect;
static NET_4GInfo_S          g_st4GInfo;
static NET_HotspotInfo_S     g_stHotspotInfo;
static NET_HotspotConnInfo_S g_stHotspotConnInfo;
static NET_VideoOsdCfg_S    g_stOsdCfg;
static NET_MotionAlarmInfo_S g_stMotionAlarmInfo;
static NET_PrivacyMaskCfg_S g_stPrivacyMaskCfg;
static NET_TamperAlarmInfo_S g_stTamperAlarmInfo;
static NET_CrossLineAlarmInfo_S g_stCrossLineAlarmInfo;
static NET_IntrusionAlarmInfo_S g_stIntrusionAlarmInfo;
static NET_LoiteringAlarmInfo_S g_stLoiteringAlarmInfo;
static NET_SceneChangeAlarmInfo_S g_stSceneChangeAlarmInfo;
static NET_CrowdGatheringAlarmInfo_S g_stCrowdGatheringAlarmInfo;
static NET_ParkingAlarmInfo_S g_stParkingAlarmInfo;
static NET_UnattendedObjectAlarmInfo_S g_stUnattendedObjectAlarmInfo;
static NET_ObjectRemovalAlarmInfo_S g_stObjectRemovalAlarmInfo;
static NET_AudioAnomalyAlarmInfo_S g_stAudioAnomalyAlarmInfo;
static NET_AudibleAlarmInfo_S gs_stAudibleAlarmInfo;
static NET_AlarmInputInfoList_S gs_stAlarmInputInfoList;
static NET_AlarmOutputInfoList_S gs_stAlarmOutputInfoList;
static NET_FlashingLightAlarmInfo_S gs_stFlashingLightAlarmInfo;
static NET_PirAlarmInfo_S gs_stPirAlarmInfo;
static NET_ImageSetting_S g_stImageCfg;
static NET_PreviewInfo_S g_stPreviewInfo;
static NET_ChannelInfo_S g_stChannelInfo;

static NET_UpgradeInfo_S g_stUpgradeInfo;
static NET_UpgradeStatus_S g_stUpgradeStatuts;
static NET_UpgradeVersion_S g_stUpgradeVersions;

static NET_CapturePlanInfo_S g_stCapturePlanInfo;
static NET_CaptureParamInfo_S g_stCaptureParamInfo;

static NET_ExposureInfo_S g_stExposureInfo;
static NET_DayNightInfo_S g_stDayNightInfo;
static NET_BackLightInfo_S g_stBackLightInfo;
static NET_DenoiseInfo_S g_stDenoiseInfo;
static NET_WhiteBalanceInfo_S g_stWhiteBalanceInfo;

static NET_TalkbackStateInfo_S g_stTalkbackStateInfo;
static NET_TalkbackStreamInfo_S g_stTalkbackToStreamInfo;
static NET_TalkbackStreamInfo_S g_stTalkbackFromStreamInfo;
static NET_ReplayTalkbackInfo_S g_stReplayTalkbackInfo;
static NET_ReplayCtrlInfo_S g_stReplayCtrlInfo;

static NET_EnterRegionAlarmInfo_S g_stEnterRegionAlarmInfo;
static NET_LeaveRegionAlarmInfo_S g_stLeaveRegionAlarmInfo;

static NET_FaceCaptureInfo_S g_stFaceCaptureInfo;
static NET_FaceCompareInfo_S g_stFaceCompareInfo;
static NET_FaceLibList_S g_stFaceLibList;
static NET_FaceInfoList_S g_stFaceInfoList;

static NET_GarbageExposureCfg_S g_stGarbageExposureCfg;
static NET_GarbageOverflowCfg_S g_stGarbageOverflowCfg;

static NET_PeopleFlowStatisticsCfg_S g_stPeopleFlowStatisticsCfg;
static NET_PeopleDensityDetectionCfg_S g_stPeopleDensityDetectionCfg;

static NET_ManholeCoverAbnormalCfg_S g_stManholeCoverAbnormalCfg;
static NET_SleepOnDutyCfg_S g_stSleepOnDutyCfg;
static NET_ElectricVehicleInElevatorCfg_S g_stElectricVehicleInElevatorCfg;
static NET_PersonFallDownCfg_S g_stPersonFallDownCfg;
static NET_ConstructionOccupyRoadCfg_S g_stConstructionOccupyRoadCfg;
static NET_CongestionCfg_S g_stCongestionCfg;
static NET_LicensePlateRecognitionCfg_S g_stLicensePlateRecognitionCfg;
static NET_HighAltitudeSeatbeltCfg_S g_stHighAltitudeSeatbeltCfg;
static NET_SafetyHelmetCfg_S g_stSafetyHelmetCfg;
static NET_PersonFallCfg_S g_stPersonFallCfg;
static NET_PhoneUsageCfg_S g_stPhoneUsageCfg;
static NET_SmokingCfg_S g_stSmokingCfg;
static NET_OpenFlameCfg_S g_stOpenFlameCfg;
static NET_BareSoilCfg_S g_stBareSoilCfg;
static NET_HoleProtectionBarCfg_S g_stHoleProtectionBarCfg;
static NET_ReflectiveClothingCfg_S g_stReflectiveClothingCfg;
static NET_PetRecognitionInfo_S g_stPetRecognitionInfo;
static NET_ClimbFenceInfo_S g_stClimbFenceInfo;
static NET_DimissionInfo_S g_stDimissionInfo;
static NET_IllegalLaneInfo_S g_stIllegalLaneInfo;
static NET_RetrogradeInfo_S g_stRetrogradeInfo;
static NET_NonmotorVehicleIntrusionInfo_S g_stNonmotorVehicleIntrusionInfo;
static NET_OccupationEmergencyInfo_S g_stOccupationEmergencyInfo;
static NET_PedestrianIntrusionInfo_S g_stPedestrianIntrusionInfo;
static NET_SmokeFireCfg_S g_stSmokeFireCfg;
static NET_RoadPondingCfg_S g_stRoadPondingCfg;
static NET_SecurityServicesInfo_S g_stSecurityServicesInfo;
static NET_SshCountdownInfo_S g_stSshCountdownInfo;
static NET_LogServerInfo_S g_stLogServerInfo;
static NET_LogList_S g_stLogList;
static NET_RecordInfo_S g_stRecordInfo;
static NET_RecordStatusInfo_S g_stRecordStatusInfo;
static NET_RecordSchedule_S g_stRecordSchedule;
static NET_RecordAdvancedParam_S g_stRecordAdvancedParam;
static NET_RecordFileList_S g_stRecordFileList;
static NET_RecordDownloadList_S g_stRecordDownloadList;

static void InitSimpleAiAlarmSchedule(NET_AlarmSchedule_S* pSchedule)
{
    if (!pSchedule)
    {
        return;
    }

    for (int day = 0; day < 7; day++)
    {
        pSchedule->uTimeSectionCount[day] = 1;
        pSchedule->astTimeSection[day][0].nStartHour = 0;
        pSchedule->astTimeSection[day][0].nStartMinute = 0;
        pSchedule->astTimeSection[day][0].nEndHour = 23;
        pSchedule->astTimeSection[day][0].nEndMinute = 59;
    }
}

static void InitDemoSmartRegion(NET_SmartRegion_S* pRegion)
{
    if (!pRegion)
    {
        return;
    }

    pRegion->uPointCount = 4;
    pRegion->afPointX[0] = 0.2f; pRegion->afPointY[0] = 0.2f;
    pRegion->afPointX[1] = 0.8f; pRegion->afPointY[1] = 0.2f;
    pRegion->afPointX[2] = 0.8f; pRegion->afPointY[2] = 0.8f;
    pRegion->afPointX[3] = 0.2f; pRegion->afPointY[3] = 0.8f;
}

static void InitDemoSmartRegionRule(NET_SmartRegionRule_S* pRule)
{
    if (!pRule)
    {
        return;
    }

    pRule->bEnable = TRUE;
    pRule->uPointCount = 4;
    pRule->afPointX[0] = 0.2f; pRule->afPointY[0] = 0.2f;
    pRule->afPointX[1] = 0.8f; pRule->afPointY[1] = 0.2f;
    pRule->afPointX[2] = 0.8f; pRule->afPointY[2] = 0.8f;
    pRule->afPointX[3] = 0.2f; pRule->afPointY[3] = 0.8f;
    pRule->nTimeThreshold = 10;
    pRule->nSensitivity = 50;
    pRule->uDetectionTargetCount = 1;
    pRule->auDetectionTarget[0] = NET_TARGET_HUMAN;
}

static void InitDemoSmartLineRule(NET_SmartLineRule_S* pRule)
{
    if (!pRule)
    {
        return;
    }

    pRule->bEnable = TRUE;
    pRule->fStartPosX = 0.2f;
    pRule->fStartPosY = 0.5f;
    pRule->fEndPosX = 0.8f;
    pRule->fEndPosY = 0.5f;
    pRule->enCrossDirection = NET_CROSS_A_TO_B;
    pRule->nSensitivity = 50;
}

static void FillOsdAttr(OsdAttribute_S* pAttr,
                        INT32 x,
                        INT32 y,
                        INT32 width,
                        INT32 height,
                        OSD_FONT_SIZE_E fontSize,
                        OSD_COLOR_E fontColor,
                        const char* pColor,
                        const char* pToken)
{
    if (!pAttr)
    {
        return;
    }

    memset(pAttr, 0, sizeof(*pAttr));
    pAttr->nX = x;
    pAttr->nY = y;
    pAttr->nW = width;
    pAttr->nH = height;
    pAttr->enAttribute = OSD_ATTR_ALPHA_N_FLASH_N;
    pAttr->enFontSize = fontSize;
    pAttr->enFontColor = fontColor;
    if (pColor)
    {
        strncpy(pAttr->strFontColor, pColor, sizeof(pAttr->strFontColor) - 1);
    }
    if (pToken)
    {
        strncpy(pAttr->strToken, pToken, sizeof(pAttr->strToken) - 1);
    }
}

static void InitDemoOsdConfig(NET_VideoOsdCfg_S* pCfg)
{
    int i = 0;

    if (!pCfg)
    {
        return;
    }

    memset(pCfg, 0, sizeof(*pCfg));
    pCfg->enAlign = OSD_ALIFN_CUSTOMIZE;

    pCfg->stOsdNameInfo.bEnable = TRUE;
    strncpy(pCfg->stOsdNameInfo.strName,
            "Demo-Camera",
            sizeof(pCfg->stOsdNameInfo.strName) - 1);
    FillOsdAttr(&pCfg->stOsdNameInfo.stOsdAttr,
                32,
                32,
                -1,
                32,
                OSD_FONT_SIZE_32,
                OSD_COLOR_WHITE,
                "#FFFFFF",
                "name_token_0");

    pCfg->stOsdTimeInfo.bEnable = TRUE;
    pCfg->stOsdTimeInfo.bEnableWeek = TRUE;
    pCfg->stOsdTimeInfo.enTimeFormat = OSD_TIME_FORMAT_24;
    pCfg->stOsdTimeInfo.enDateFormat = ENGLISH_YYYY_MM_DD;
    FillOsdAttr(&pCfg->stOsdTimeInfo.stOsdAttr,
                32,
                80,
                -1,
                32,
                OSD_FONT_SIZE_32,
                OSD_COLOR_WHITE,
                "#FFFFFF",
                "time_token_0");

    for (i = 0; i < DEMO_OSD_CUSTOM_MAX_NUM; ++i)
    {
        char name[NET_LEN_128] = {0};
        char color[NET_LEN_16] = {0};
        char token[NET_LEN_512] = {0};

        snprintf(name, sizeof(name), "SDK Demo OSD %d", i + 1);
        snprintf(color, sizeof(color), "#%02X%02X%02X", 0, 255 - i * 32, i * 48);
        snprintf(token, sizeof(token), "custom_token_%d", i);

        pCfg->OsdInfo[i].nId = i + 1;
        pCfg->OsdInfo[i].bEnable = (i < 2) ? TRUE : FALSE;
        strncpy(pCfg->OsdInfo[i].strName, name, sizeof(pCfg->OsdInfo[i].strName) - 1);
        FillOsdAttr(&pCfg->OsdInfo[i].stOsdAttr,
                    32,
                    128 + i * 40,
                    -1,
                    32,
                    OSD_FONT_SIZE_32,
                    OSD_COLOR_CUSTOMIZE,
                    color,
                    token);
    }

    for (i = DEMO_OSD_CUSTOM_MAX_NUM; i < DEMO_OSD_STRUCT_SLOT_NUM; ++i)
    {
        memset(&pCfg->OsdInfo[i], 0, sizeof(pCfg->OsdInfo[i]));
    }
}

static void PrintOsdAttr(const char* pPrefix, const OsdAttribute_S* pAttr)
{
    if (!pAttr)
    {
        return;
    }

    printf("%sPos=(%d,%d,%d,%d), Attr=%d, FontSize=%d, FontColor=%d, CustomColor=%s, Token=%s\n",
           pPrefix ? pPrefix : "",
           pAttr->nX,
           pAttr->nY,
           pAttr->nW,
           pAttr->nH,
           pAttr->enAttribute,
           pAttr->enFontSize,
           pAttr->enFontColor,
           pAttr->strFontColor,
           pAttr->strToken);
}

static void PrintOsdConfigSummary(const char* pTitle, const NET_VideoOsdCfg_S* pCfg)
{
    int i = 0;
    int enabledCustomCount = 0;

    if (!pCfg)
    {
        return;
    }

    printf("[ConfigServerDemo] %s\n", pTitle ? pTitle : "OSD config");
    printf("  Align=%d\n", pCfg->enAlign);
    printf("  [NameOSD] Enable=%d, Name=%s\n",
           pCfg->stOsdNameInfo.bEnable,
           pCfg->stOsdNameInfo.strName);
    PrintOsdAttr("    ", &pCfg->stOsdNameInfo.stOsdAttr);
    printf("  [TimeOSD] Enable=%d, EnableWeek=%d, TimeFormat=%d, DateFormat=%d\n",
           pCfg->stOsdTimeInfo.bEnable,
           pCfg->stOsdTimeInfo.bEnableWeek,
           pCfg->stOsdTimeInfo.enTimeFormat,
           pCfg->stOsdTimeInfo.enDateFormat);
    PrintOsdAttr("    ", &pCfg->stOsdTimeInfo.stOsdAttr);

    for (i = 0; i < DEMO_OSD_CUSTOM_MAX_NUM; ++i)
    {
        if (pCfg->OsdInfo[i].bEnable)
        {
            ++enabledCustomCount;
        }

        printf("  [CustomOSD-%d] Id=%d, Enable=%d, Name=%s\n",
               i,
               pCfg->OsdInfo[i].nId,
               pCfg->OsdInfo[i].bEnable,
               pCfg->OsdInfo[i].strName);
        PrintOsdAttr("    ", &pCfg->OsdInfo[i].stOsdAttr);
    }

    printf("  Enabled custom OSD count in first %d slots=%d\n",
           DEMO_OSD_CUSTOM_MAX_NUM,
           enabledCustomCount);
}

static void NormalizeDemoOsdConfig(NET_VideoOsdCfg_S* pCfg)
{
    int i = 0;

    if (!pCfg)
    {
        return;
    }

    for (i = 0; i < DEMO_OSD_CUSTOM_MAX_NUM; ++i)
    {
        if (pCfg->OsdInfo[i].nId <= 0)
        {
            pCfg->OsdInfo[i].nId = i + 1;
        }
    }

    for (i = DEMO_OSD_CUSTOM_MAX_NUM; i < DEMO_OSD_STRUCT_SLOT_NUM; ++i)
    {
        memset(&pCfg->OsdInfo[i], 0, sizeof(pCfg->OsdInfo[i]));
    }
}

/**
 * @brief 初始化告警配置示例使用的全天布防时间表。
 * @author ITC
 * @param [out] pSchedule 待初始化的告警时间表。
 * @return 无。
 */
static void ConfigDemoInitAlarmSchedule(NET_AlarmSchedule_S* pSchedule)
{
    INT32 nDay = 0;

    if (!pSchedule)
    {
        return;
    }

    memset(pSchedule, 0, sizeof(*pSchedule));
    for (nDay = 0; nDay < NET_ALARM_SCHEDULE_DAY_COUNT; ++nDay)
    {
        pSchedule->uTimeSectionCount[nDay] = 1;
        pSchedule->astTimeSection[nDay][0].nStartHour = NET_ALARM_SCHEDULE_HOUR_MIN;
        pSchedule->astTimeSection[nDay][0].nStartMinute = NET_ALARM_SCHEDULE_MINUTE_MIN;
        pSchedule->astTimeSection[nDay][0].nEndHour = NET_ALARM_SCHEDULE_HOUR_MAX;
        pSchedule->astTimeSection[nDay][0].nEndMinute = NET_ALARM_SCHEDULE_MINUTE_MAX;
    }
}

static void InitDefaultConfig(void)
{
    memset(&g_stDeviceBasicInfo, 0, sizeof(g_stDeviceBasicInfo));
    memset(&g_stNetworkCfg, 0, sizeof(g_stNetworkCfg));
    memset(&g_stSystemNtpCfg, 0, sizeof(g_stSystemNtpCfg));
    memset(&g_stStreamCfg, 0, sizeof(g_stStreamCfg));
    memset(&g_stAudioCfg, 0, sizeof(g_stAudioCfg));
    memset(&g_stWifiStaCfg, 0, sizeof(g_stWifiStaCfg));
    memset(&g_stWifiStaConnect, 0, sizeof(g_stWifiStaConnect));
    memset(&g_st4GInfo, 0, sizeof(g_st4GInfo));
    memset(&g_stHotspotInfo, 0, sizeof(g_stHotspotInfo));
    memset(&g_stHotspotConnInfo, 0, sizeof(g_stHotspotConnInfo));
    memset(&g_stOsdCfg, 0, sizeof(g_stOsdCfg));
    memset(&g_stMotionAlarmInfo, 0, sizeof(g_stMotionAlarmInfo));
    memset(&g_stPrivacyMaskCfg, 0, sizeof(g_stPrivacyMaskCfg));
    memset(&g_stTamperAlarmInfo, 0, sizeof(g_stTamperAlarmInfo));
    memset(&g_stCrossLineAlarmInfo, 0, sizeof(g_stCrossLineAlarmInfo));
    memset(&g_stIntrusionAlarmInfo, 0, sizeof(g_stIntrusionAlarmInfo));
    memset(&g_stLoiteringAlarmInfo, 0, sizeof(g_stLoiteringAlarmInfo));
    memset(&g_stSceneChangeAlarmInfo, 0, sizeof(g_stSceneChangeAlarmInfo));
    memset(&g_stCrowdGatheringAlarmInfo, 0, sizeof(g_stCrowdGatheringAlarmInfo));
    memset(&g_stParkingAlarmInfo, 0, sizeof(g_stParkingAlarmInfo));
    memset(&g_stUnattendedObjectAlarmInfo, 0, sizeof(g_stUnattendedObjectAlarmInfo));
    memset(&g_stObjectRemovalAlarmInfo, 0, sizeof(g_stObjectRemovalAlarmInfo));
    memset(&g_stAudioAnomalyAlarmInfo, 0, sizeof(g_stAudioAnomalyAlarmInfo));
    memset(&gs_stAudibleAlarmInfo, 0, sizeof(gs_stAudibleAlarmInfo));
    memset(&gs_stAlarmInputInfoList, 0, sizeof(gs_stAlarmInputInfoList));
    memset(&gs_stAlarmOutputInfoList, 0, sizeof(gs_stAlarmOutputInfoList));
    memset(&gs_stFlashingLightAlarmInfo, 0, sizeof(gs_stFlashingLightAlarmInfo));
    memset(&gs_stPirAlarmInfo, 0, sizeof(gs_stPirAlarmInfo));

    memset(&g_stImageCfg, 0, sizeof(g_stImageCfg));
    memset(&g_stPreviewInfo, 0, sizeof(g_stPreviewInfo));

    memset(&g_stUpgradeInfo, 0, sizeof(g_stUpgradeInfo));
    memset(&g_stUpgradeStatuts, 0, sizeof(g_stUpgradeStatuts));
    memset(&g_stUpgradeVersions, 0, sizeof(g_stUpgradeVersions));

    memset(&g_stExposureInfo, 0, sizeof(g_stExposureInfo));
    memset(&g_stDayNightInfo, 0, sizeof(g_stDayNightInfo));
    memset(&g_stBackLightInfo, 0, sizeof(g_stBackLightInfo));
    memset(&g_stDenoiseInfo, 0, sizeof(g_stDenoiseInfo));
    memset(&g_stWhiteBalanceInfo, 0, sizeof(g_stWhiteBalanceInfo));

    memset(&g_stTalkbackStateInfo, 0, sizeof(g_stTalkbackStateInfo));
    memset(&g_stTalkbackToStreamInfo, 0, sizeof(g_stTalkbackToStreamInfo));
    memset(&g_stTalkbackFromStreamInfo, 0, sizeof(g_stTalkbackFromStreamInfo));
    memset(&g_stReplayTalkbackInfo, 0, sizeof(g_stReplayTalkbackInfo));

    memset(&g_stEnterRegionAlarmInfo, 0, sizeof(g_stEnterRegionAlarmInfo));
    memset(&g_stLeaveRegionAlarmInfo, 0, sizeof(g_stLeaveRegionAlarmInfo));

    memset(&g_stFaceCaptureInfo, 0, sizeof(g_stFaceCaptureInfo));
    memset(&g_stFaceCompareInfo, 0, sizeof(g_stFaceCompareInfo));
    memset(&g_stFaceLibList, 0, sizeof(g_stFaceLibList));
    memset(&g_stFaceInfoList, 0, sizeof(g_stFaceInfoList));

    memset(&g_stGarbageExposureCfg, 0, sizeof(g_stGarbageExposureCfg));
    memset(&g_stGarbageOverflowCfg, 0, sizeof(g_stGarbageOverflowCfg));

    memset(&g_stPeopleFlowStatisticsCfg, 0, sizeof(g_stPeopleFlowStatisticsCfg));
    memset(&g_stPeopleDensityDetectionCfg, 0, sizeof(g_stPeopleDensityDetectionCfg));
    memset(&g_stManholeCoverAbnormalCfg, 0, sizeof(g_stManholeCoverAbnormalCfg));
    memset(&g_stSleepOnDutyCfg, 0, sizeof(g_stSleepOnDutyCfg));
    memset(&g_stElectricVehicleInElevatorCfg, 0, sizeof(g_stElectricVehicleInElevatorCfg));
    memset(&g_stPersonFallDownCfg, 0, sizeof(g_stPersonFallDownCfg));
    memset(&g_stConstructionOccupyRoadCfg, 0, sizeof(g_stConstructionOccupyRoadCfg));
    memset(&g_stCongestionCfg, 0, sizeof(g_stCongestionCfg));
    memset(&g_stLicensePlateRecognitionCfg, 0, sizeof(g_stLicensePlateRecognitionCfg));
    memset(&g_stHighAltitudeSeatbeltCfg, 0, sizeof(g_stHighAltitudeSeatbeltCfg));
    memset(&g_stSafetyHelmetCfg, 0, sizeof(g_stSafetyHelmetCfg));
    memset(&g_stPersonFallCfg, 0, sizeof(g_stPersonFallCfg));
    memset(&g_stPhoneUsageCfg, 0, sizeof(g_stPhoneUsageCfg));
    memset(&g_stSmokingCfg, 0, sizeof(g_stSmokingCfg));
    memset(&g_stOpenFlameCfg, 0, sizeof(g_stOpenFlameCfg));
    memset(&g_stBareSoilCfg, 0, sizeof(g_stBareSoilCfg));
    memset(&g_stHoleProtectionBarCfg, 0, sizeof(g_stHoleProtectionBarCfg));
    memset(&g_stReflectiveClothingCfg, 0, sizeof(g_stReflectiveClothingCfg));
    memset(&g_stPetRecognitionInfo, 0, sizeof(g_stPetRecognitionInfo));
    memset(&g_stClimbFenceInfo, 0, sizeof(g_stClimbFenceInfo));
    memset(&g_stDimissionInfo, 0, sizeof(g_stDimissionInfo));
    memset(&g_stIllegalLaneInfo, 0, sizeof(g_stIllegalLaneInfo));
    memset(&g_stRetrogradeInfo, 0, sizeof(g_stRetrogradeInfo));
    memset(&g_stNonmotorVehicleIntrusionInfo, 0, sizeof(g_stNonmotorVehicleIntrusionInfo));
    memset(&g_stOccupationEmergencyInfo, 0, sizeof(g_stOccupationEmergencyInfo));
    memset(&g_stPedestrianIntrusionInfo, 0, sizeof(g_stPedestrianIntrusionInfo));
    memset(&g_stSmokeFireCfg, 0, sizeof(g_stSmokeFireCfg));
    memset(&g_stRoadPondingCfg, 0, sizeof(g_stRoadPondingCfg));
    memset(&g_stSecurityServicesInfo, 0, sizeof(g_stSecurityServicesInfo));
    memset(&g_stSshCountdownInfo, 0, sizeof(g_stSshCountdownInfo));
    memset(&g_stLogServerInfo, 0, sizeof(g_stLogServerInfo));
    memset(&g_stLogList, 0, sizeof(g_stLogList));
    memset(&g_stRecordInfo, 0, sizeof(g_stRecordInfo));
    memset(&g_stRecordStatusInfo, 0, sizeof(g_stRecordStatusInfo));
    memset(&g_stRecordSchedule, 0, sizeof(g_stRecordSchedule));
    memset(&g_stRecordAdvancedParam, 0, sizeof(g_stRecordAdvancedParam));
    memset(&g_stRecordFileList, 0, sizeof(g_stRecordFileList));
    memset(&g_stRecordDownloadList, 0, sizeof(g_stRecordDownloadList));

    /* 人流统计配置默认值 */
    g_stPeopleFlowStatisticsCfg.bEnable = FALSE;
    g_stPeopleFlowStatisticsCfg.nSensitivity = 50;
    g_stPeopleFlowStatisticsCfg.nReportInterval = 60;
    g_stPeopleFlowStatisticsCfg.enStatisticsType = NET_PEOPLE_FLOW_STAT_TOTAL;
    g_stPeopleFlowStatisticsCfg.stTimedReset.bEnable = FALSE;
    g_stPeopleFlowStatisticsCfg.stTimedReset.nHour = 0;
    g_stPeopleFlowStatisticsCfg.stTimedReset.nMinute = 0;

    /* 人员密度检测配置默认值 */
    g_stPeopleDensityDetectionCfg.bEnable = FALSE;
    g_stPeopleDensityDetectionCfg.nSensitivity = 50;
    g_stPeopleDensityDetectionCfg.nReportInterval = 60;

    /* 系统升级信息默认值*/
    strncpy(g_stUpgradeInfo.szUpgradePath,        "/opt/course/",                    sizeof(g_stUpgradeInfo.szUpgradePath) - 1);
    g_stUpgradeStatuts.nUpgradeStatus = -1;
    strncpy(g_stUpgradeVersions.szVersion,        "Ver1.00.00",                    sizeof(g_stUpgradeVersions.szVersion) - 1);

    /* 设备基本信息默认值 */
    strncpy(g_stDeviceBasicInfo.strDevModel,       "Demo-IPCamera-Model", sizeof(g_stDeviceBasicInfo.strDevModel) - 1);
    strncpy(g_stDeviceBasicInfo.strSerialNum,      "SN-DEMO-000001",      sizeof(g_stDeviceBasicInfo.strSerialNum) - 1);
    strncpy(g_stDeviceBasicInfo.strFirmwareVersion,"V1.0.0",              sizeof(g_stDeviceBasicInfo.strFirmwareVersion) - 1);
    strncpy(g_stDeviceBasicInfo.strMacAddress,     "00:11:22:33:44:55",   sizeof(g_stDeviceBasicInfo.strMacAddress) - 1);
    strncpy(g_stDeviceBasicInfo.strDeviceName,     "Demo-Device",         sizeof(g_stDeviceBasicInfo.strDeviceName) - 1);
    strncpy(g_stDeviceBasicInfo.strManufacturer,   "DemoManufacturer",    sizeof(g_stDeviceBasicInfo.strManufacturer) - 1);
    strncpy(g_stDeviceBasicInfo.strDeviceTypeV2,   "IPC",                 sizeof(g_stDeviceBasicInfo.strDeviceTypeV2) - 1);

    /* 网络配置默认值 */
    g_stNetworkCfg.uMTU     = 1500;
    g_stNetworkCfg.bIPv4DHCP = 0; /* 0-手动配置，1-DHCP */
    strncpy(g_stNetworkCfg.szIpv4Address,   "192.168.1.100", sizeof(g_stNetworkCfg.szIpv4Address) - 1);
    strncpy(g_stNetworkCfg.szIPv4GateWay,   "192.168.1.1",   sizeof(g_stNetworkCfg.szIPv4GateWay) - 1);
    strncpy(g_stNetworkCfg.szIPv4SubnetMask,"255.255.255.0", sizeof(g_stNetworkCfg.szIPv4SubnetMask) - 1);

    /* 系统校时配置默认值 */
    g_stSystemNtpCfg.enTimeZone = 8;
    g_stSystemNtpCfg.enDateFormat = 0;
    g_stSystemNtpCfg.bEnableNTPSync = FALSE;
    g_stSystemNtpCfg.bManualSync = TRUE;
    strncpy(g_stSystemNtpCfg.strDateTime, "2026-06-23 10:00:00", sizeof(g_stSystemNtpCfg.strDateTime) - 1);
    g_stSystemNtpCfg.bIsSyncWithComputer = FALSE;
    strncpy(g_stSystemNtpCfg.strAddress, "time.windows.com", sizeof(g_stSystemNtpCfg.strAddress) - 1);
    g_stSystemNtpCfg.nPort = 123;
    g_stSystemNtpCfg.nSyncInterval = 60;

    /* 视频码流配置默认值 */
    g_stStreamCfg.nId = NET_LIVE_STREAM_INDEX_MAIN;
    g_stStreamCfg.enVideoType = 0;
    g_stStreamCfg.stVideoResolution.uWidth = 1920;
    g_stStreamCfg.stVideoResolution.uHeight = 1080;
    g_stStreamCfg.enBitrateType = 0;
    g_stStreamCfg.enImageQuality = 60;
    g_stStreamCfg.enFrameRate = 25;
    g_stStreamCfg.nBitrateUpperLimit = 4096;
    g_stStreamCfg.nAverageBitrate = 2048;
    g_stStreamCfg.enVideoCodec = NET_VIDEO_CODE_H264;
    g_stStreamCfg.bSmartEnable = FALSE;
    g_stStreamCfg.enEncodingComplexity = 1;
    g_stStreamCfg.nIFrameInterval = 50;
    g_stStreamCfg.enSvcEnable = 0;
    g_stStreamCfg.nBitrateSmoothing = 50;

    /* 音频配置默认值 */
    g_stAudioCfg.bAudioSwitch = TRUE;
    g_stAudioCfg.enInputType = NET_AUDIO_INPUT_MICIN;
    g_stAudioCfg.enFormat = NET_AUDIO_FORMAT_AAC;
    g_stAudioCfg.enSampRate = NET_AUDIO_SAMPRATE_16000;
    g_stAudioCfg.enBitRate = NET_AUDIO_BITRATE_64K;
    g_stAudioCfg.u32InputVolume = 60;
    g_stAudioCfg.bDenoise = TRUE;
    g_stAudioCfg.enOutputType = NET_AUDIO_OUTPUT_SPEAKER;
    g_stAudioCfg.u32OutputVolume = 50;

    /* WiFi配置默认值 */
    g_stWifiStaCfg.bEnableWifi = TRUE;
    g_stWifiStaCfg.bEnableBoost = FALSE;

    strncpy(g_stWifiStaConnect.szSsid, "DemoWifi", sizeof(g_stWifiStaConnect.szSsid) - 1);
    g_stWifiStaConnect.nSecurityMode = NET_WIFI_SECURITY_WPA_PERSONAL;
    strncpy(g_stWifiStaConnect.szIpAddress, "192.168.1.120", sizeof(g_stWifiStaConnect.szIpAddress) - 1);
    strncpy(g_stWifiStaConnect.szPassword, "12345678", sizeof(g_stWifiStaConnect.szPassword) - 1);
    strncpy(g_stWifiStaConnect.szPairwise, "CCMP", sizeof(g_stWifiStaConnect.szPairwise) - 1);
    g_stWifiStaConnect.nWepKeyLen = 128;
    g_stWifiStaConnect.bWepIsHex = FALSE;
    strncpy(g_stWifiStaConnect.szAuthAlg, "OPEN", sizeof(g_stWifiStaConnect.szAuthAlg) - 1);
    g_stWifiStaConnect.nWepKeyCount = 1;
    g_stWifiStaConnect.astWepKeys[0].nIndex = 1;
    strncpy(g_stWifiStaConnect.astWepKeys[0].szValue, "1234567890", sizeof(g_stWifiStaConnect.astWepKeys[0].szValue) - 1);
    strncpy(g_stWifiStaConnect.szCtrlInterface, "/var/run/wpa_supplicant", sizeof(g_stWifiStaConnect.szCtrlInterface) - 1);
    strncpy(g_stWifiStaConnect.szInterfaceName, "wlan0", sizeof(g_stWifiStaConnect.szInterfaceName) - 1);

    strncpy(g_st4GInfo.szApn, "ctnet", sizeof(g_st4GInfo.szApn) - 1);
    strncpy(g_st4GInfo.szCallNumber, "*99#", sizeof(g_st4GInfo.szCallNumber) - 1);
    g_st4GInfo.nMtu = 1500;
    g_st4GInfo.nAuthMode = 0;
    g_st4GInfo.nNetworkMode = 0;
    g_st4GInfo.nDialMode = 0;

    g_stHotspotInfo.bEnabled = FALSE;
    strncpy(g_stHotspotInfo.szSsid, "DemoHotspot", sizeof(g_stHotspotInfo.szSsid) - 1);
    strncpy(g_stHotspotInfo.szSecurityMode, "WPA2-personal", sizeof(g_stHotspotInfo.szSecurityMode) - 1);
    strncpy(g_stHotspotInfo.szEncryptionType, "TKIP", sizeof(g_stHotspotInfo.szEncryptionType) - 1);
    strncpy(g_stHotspotInfo.szPassword, "12345678", sizeof(g_stHotspotInfo.szPassword) - 1);
    strncpy(g_stHotspotInfo.szConfirmPassword, "12345678", sizeof(g_stHotspotInfo.szConfirmPassword) - 1);

    strncpy(g_stHotspotConnInfo.szStatus, "success", sizeof(g_stHotspotConnInfo.szStatus) - 1);
    g_stHotspotConnInfo.nTotal = 2;
    g_stHotspotConnInfo.nDeviceCount = 2;
    g_stHotspotConnInfo.astDevices[0].nIndex = 1;
    strncpy(g_stHotspotConnInfo.astDevices[0].szMac, "00:11:22:33:44:66", sizeof(g_stHotspotConnInfo.astDevices[0].szMac) - 1);
    strncpy(g_stHotspotConnInfo.astDevices[0].szIp, "192.168.43.10", sizeof(g_stHotspotConnInfo.astDevices[0].szIp) - 1);
    strncpy(g_stHotspotConnInfo.astDevices[0].szConnTime, "2026-04-30 10:00:00", sizeof(g_stHotspotConnInfo.astDevices[0].szConnTime) - 1);
    g_stHotspotConnInfo.astDevices[1].nIndex = 2;
    strncpy(g_stHotspotConnInfo.astDevices[1].szMac, "00:11:22:33:44:77", sizeof(g_stHotspotConnInfo.astDevices[1].szMac) - 1);
    strncpy(g_stHotspotConnInfo.astDevices[1].szIp, "192.168.43.11", sizeof(g_stHotspotConnInfo.astDevices[1].szIp) - 1);
    strncpy(g_stHotspotConnInfo.astDevices[1].szConnTime, "2026-04-30 10:05:00", sizeof(g_stHotspotConnInfo.astDevices[1].szConnTime) - 1);

    /* 安全服务、日志与日志服务器默认值 */
    g_stSecurityServicesInfo.stLoginLock.bIllegalLoginEnable = TRUE;
    g_stSecurityServicesInfo.stLoginLock.nCheckInterval = 30;
    g_stSecurityServicesInfo.stLoginLock.nMaxErrorTimes = 5;
    g_stSecurityServicesInfo.stLoginLock.nLockDuration = 0;
    g_stSecurityServicesInfo.stPwdPolicy.bPwdSecurityLevelEnable = FALSE;
    g_stSecurityServicesInfo.stPwdPolicy.bAllowLowLevelPwdLogin = FALSE;
    g_stSecurityServicesInfo.stSshAdmin.bSshEnable = FALSE;
    g_stSecurityServicesInfo.stSshAdmin.nSshPort = 22;
    strncpy(g_stSecurityServicesInfo.stSshAdmin.szSshStartTime, "2026-05-06 10:00:00", sizeof(g_stSecurityServicesInfo.stSshAdmin.szSshStartTime) - 1);
    strncpy(g_stSecurityServicesInfo.stSshAdmin.szSshCountdown, "08:00:00", sizeof(g_stSecurityServicesInfo.stSshAdmin.szSshCountdown) - 1);
    strncpy(g_stSshCountdownInfo.szCountdown, "08:00:00", sizeof(g_stSshCountdownInfo.szCountdown) - 1);

    g_stLogServerInfo.bEnable = TRUE;
    g_stLogServerInfo.bEnSsl = FALSE;
    strncpy(g_stLogServerInfo.szServerAddr, "oam.itc-pa.cn", sizeof(g_stLogServerInfo.szServerAddr) - 1);
    g_stLogServerInfo.nPort = 1883;

    g_stLogList.stPage.nCurPage = 1;
    g_stLogList.stPage.nPageSize = 20;
    g_stLogList.stPage.nDataTotal = 2;
    g_stLogList.stPage.nPageTotal = 1;
    g_stLogList.nLogCount = 2;
    strncpy(g_stLogList.astLogs[0].szStartTime, "2026-05-06 10:00:00", sizeof(g_stLogList.astLogs[0].szStartTime) - 1);
    g_stLogList.astLogs[0].nType = 3;
    g_stLogList.astLogs[0].nAction = 0;
    strncpy(g_stLogList.astLogs[0].szChnName, "IPC-1", sizeof(g_stLogList.astLogs[0].szChnName) - 1);
    strncpy(g_stLogList.astLogs[0].szUser, "admin", sizeof(g_stLogList.astLogs[0].szUser) - 1);
    strncpy(g_stLogList.astLogs[0].szHost, "192.168.1.10", sizeof(g_stLogList.astLogs[0].szHost) - 1);
    strncpy(g_stLogList.astLogs[0].szContext, "Demo login success", sizeof(g_stLogList.astLogs[0].szContext) - 1);
    strncpy(g_stLogList.astLogs[1].szStartTime, "2026-05-06 10:05:00", sizeof(g_stLogList.astLogs[1].szStartTime) - 1);
    g_stLogList.astLogs[1].nType = 4;
    g_stLogList.astLogs[1].nAction = 0;
    strncpy(g_stLogList.astLogs[1].szChnName, "IPC-1", sizeof(g_stLogList.astLogs[1].szChnName) - 1);
    strncpy(g_stLogList.astLogs[1].szUser, "system", sizeof(g_stLogList.astLogs[1].szUser) - 1);
    strncpy(g_stLogList.astLogs[1].szHost, "localhost", sizeof(g_stLogList.astLogs[1].szHost) - 1);
    strncpy(g_stLogList.astLogs[1].szContext, "Demo log server heartbeat", sizeof(g_stLogList.astLogs[1].szContext) - 1);

    /* 手动录像、录像计划、录像文件与下载默认值 */
    g_stRecordInfo.nChnId = 0;
    g_stRecordInfo.nVideoStatus = 1;
    g_stRecordInfo.nAudioStatus = 1;
    g_stRecordInfo.nRecordStatus = NET_RECORD_STATUS_RECORDING;
    g_stRecordInfo.nRecordFormat = 0;
    g_stRecordInfo.nEventType = 0;
    strncpy(g_stRecordInfo.szPath, "/tmp/", sizeof(g_stRecordInfo.szPath) - 1);
    strncpy(g_stRecordInfo.szRedunPath, "/tmp/", sizeof(g_stRecordInfo.szRedunPath) - 1);
    strncpy(g_stRecordInfo.szRecordName, "manual_record_demo.ts", sizeof(g_stRecordInfo.szRecordName) - 1);
    strncpy(g_stRecordInfo.szRecordTime, "2026-05-06 10:00:00", sizeof(g_stRecordInfo.szRecordTime) - 1);
    g_stRecordInfo.nStreamType = 0;

    g_stRecordStatusInfo.nStatus = NET_RECORD_STATUS_STOP;

    g_stRecordSchedule.bEnable = TRUE;
    g_stRecordSchedule.nDayScheduleCount = NET_PLAN_DAY_NUM_AWEEK;
    for (int i = 0; i < NET_PLAN_DAY_NUM_AWEEK; ++i)
    {
        g_stRecordSchedule.astDaySchedules[i].nDayOfWeek = i + 1;
        g_stRecordSchedule.astDaySchedules[i].nRecordTimeCount = 1;
        g_stRecordSchedule.astDaySchedules[i].astRecordTimes[0].nType = 1;
        g_stRecordSchedule.astDaySchedules[i].astRecordTimes[0].nStartTime = 0;
        g_stRecordSchedule.astDaySchedules[i].astRecordTimes[0].nEndTime = 86400;
    }

    g_stRecordAdvancedParam.bLoopWrite = TRUE;
    g_stRecordAdvancedParam.nPreTime = 0;
    g_stRecordAdvancedParam.nDelayTime = 5;
    g_stRecordAdvancedParam.nStreamType = 0;

    g_stRecordFileList.stFind.nChnId = 0;
    g_stRecordFileList.stFind.nType = 0;
    strncpy(g_stRecordFileList.stFind.szYear, "2026", sizeof(g_stRecordFileList.stFind.szYear) - 1);
    strncpy(g_stRecordFileList.stFind.szMonth, "05", sizeof(g_stRecordFileList.stFind.szMonth) - 1);
    strncpy(g_stRecordFileList.stFind.szDate, "2026-05-06", sizeof(g_stRecordFileList.stFind.szDate) - 1);
    strncpy(g_stRecordFileList.stFind.szStartTime, "2026-05-06 00:00:00", sizeof(g_stRecordFileList.stFind.szStartTime) - 1);
    strncpy(g_stRecordFileList.stFind.szEndTime, "2026-05-06 23:59:59", sizeof(g_stRecordFileList.stFind.szEndTime) - 1);
    g_stRecordFileList.nResultCount = 1;
    g_stRecordFileList.astResults[0].nChnId = 0;
    g_stRecordFileList.astResults[0].nDateCount = 1;
    strncpy(g_stRecordFileList.astResults[0].aszDates[0], "2026-05-06", sizeof(g_stRecordFileList.astResults[0].aszDates[0]) - 1);
    strncpy(g_stRecordFileList.astResults[0].szFilename, "manual_record_demo.ts", sizeof(g_stRecordFileList.astResults[0].szFilename) - 1);
    g_stRecordFileList.astResults[0].nVideoTimeCount = 1;
    g_stRecordFileList.astResults[0].astVideoTimes[0].nStartTime = 3600;
    g_stRecordFileList.astResults[0].astVideoTimes[0].nEndTime = 7200;

    g_stRecordDownloadList.nDownloadCount = 1;
    g_stRecordDownloadList.astDownloads[0].nChnId = 0;
    strncpy(g_stRecordDownloadList.astDownloads[0].szPath, "/tmp/manual_record_demo.ts", sizeof(g_stRecordDownloadList.astDownloads[0].szPath) - 1);
    strncpy(g_stRecordDownloadList.astDownloads[0].szStartTime, "2026-05-06 10:00:00", sizeof(g_stRecordDownloadList.astDownloads[0].szStartTime) - 1);
    strncpy(g_stRecordDownloadList.astDownloads[0].szEndTime, "2026-05-06 10:30:00", sizeof(g_stRecordDownloadList.astDownloads[0].szEndTime) - 1);
    g_stRecordDownloadList.nProgressCount = 1;
    strncpy(g_stRecordDownloadList.astProgress[0].szFilename, "manual_record_demo.ts", sizeof(g_stRecordDownloadList.astProgress[0].szFilename) - 1);
    g_stRecordDownloadList.astProgress[0].nProgress = 0;

    /* OSD配置默认值 */
    InitDemoOsdConfig(&g_stOsdCfg);

    /* 移动侦测配置默认值 */
    g_stMotionAlarmInfo.bEnable = TRUE;
    g_stMotionAlarmInfo.bDynamicAnalysisEnable = FALSE;
    g_stMotionAlarmInfo.uMode = NET_MOTION_MODE_NORMAL;
    g_stMotionAlarmInfo.stNormalMode.nSensitivity = 50;
    g_stMotionAlarmInfo.stNormalMode.nRegionType = 0; /* 筒型 */
    g_stMotionAlarmInfo.stNormalMode.nRectLeft = 100;
    g_stMotionAlarmInfo.stNormalMode.nRectTop = 100;
    g_stMotionAlarmInfo.stNormalMode.nRectRight = 800;
    g_stMotionAlarmInfo.stNormalMode.nRectBottom = 600;
    /* 初始化布防时间：每天一个时间段 00:00-23:59 */
    for (int day = 0; day < 7; day++)
    {
        g_stMotionAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stMotionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stMotionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stMotionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stMotionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 隐私遮盖配置默认值 */
    g_stPrivacyMaskCfg.bEnable = TRUE;
    g_stPrivacyMaskCfg.uAreaCount = 1;
    g_stPrivacyMaskCfg.astArea[0].nAreaID = 0;
    g_stPrivacyMaskCfg.astArea[0].bEnable = TRUE;
    g_stPrivacyMaskCfg.astArea[0].nRectLeft = 100;
    g_stPrivacyMaskCfg.astArea[0].nRectTop = 100;
    g_stPrivacyMaskCfg.astArea[0].nRectRight = 400;
    g_stPrivacyMaskCfg.astArea[0].nRectBottom = 300;

    /* 遮挡报警配置默认值 */
    g_stTamperAlarmInfo.bEnable = FALSE;
    g_stTamperAlarmInfo.uSensitivity = 2;
    g_stTamperAlarmInfo.nRectLeft = 200;
    g_stTamperAlarmInfo.nRectTop = 200;
    g_stTamperAlarmInfo.nRectRight = 700;
    g_stTamperAlarmInfo.nRectBottom = 500;
    for (int day = 0; day < 7; day++)
    {
        g_stTamperAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stTamperAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stTamperAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stTamperAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stTamperAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 越界检测配置默认值 */
    g_stCrossLineAlarmInfo.bEnable = FALSE;
    g_stCrossLineAlarmInfo.uRuleCount = 1;
    g_stCrossLineAlarmInfo.stRule[0].bEnable = TRUE;
    g_stCrossLineAlarmInfo.stRule[0].fStartPosX = 0.1f;
    g_stCrossLineAlarmInfo.stRule[0].fStartPosY = 0.5f;
    g_stCrossLineAlarmInfo.stRule[0].fEndPosX = 0.9f;
    g_stCrossLineAlarmInfo.stRule[0].fEndPosY = 0.5f;
    g_stCrossLineAlarmInfo.stRule[0].enCrossDirection = NET_CROSS_BOTH_WAYS;
    g_stCrossLineAlarmInfo.stRule[0].nSensitivity = 50;
    g_stCrossLineAlarmInfo.stRule[0].uDetectionTargetCount = 1;
    g_stCrossLineAlarmInfo.stRule[0].auDetectionTarget[0] = NET_TARGET_HUMAN;
    for (int day = 0; day < 7; day++)
    {
        g_stCrossLineAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stCrossLineAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stCrossLineAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stCrossLineAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stCrossLineAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 入侵检测配置默认值 */
    g_stIntrusionAlarmInfo.bEnable = FALSE;
    g_stIntrusionAlarmInfo.uRuleCount = 1;
    g_stIntrusionAlarmInfo.stRule[0].bEnable = TRUE;
    g_stIntrusionAlarmInfo.stRule[0].uPointCount = 4;
    /* 矩形区域：左上、右上、右下、左下 */
    g_stIntrusionAlarmInfo.stRule[0].afPointX[0] = 0.2f; g_stIntrusionAlarmInfo.stRule[0].afPointY[0] = 0.2f;
    g_stIntrusionAlarmInfo.stRule[0].afPointX[1] = 0.8f; g_stIntrusionAlarmInfo.stRule[0].afPointY[1] = 0.2f;
    g_stIntrusionAlarmInfo.stRule[0].afPointX[2] = 0.8f; g_stIntrusionAlarmInfo.stRule[0].afPointY[2] = 0.8f;
    g_stIntrusionAlarmInfo.stRule[0].afPointX[3] = 0.2f; g_stIntrusionAlarmInfo.stRule[0].afPointY[3] = 0.8f;
    g_stIntrusionAlarmInfo.stRule[0].nTimeThreshold = 10;
    g_stIntrusionAlarmInfo.stRule[0].nSensitivity = 50;
    g_stIntrusionAlarmInfo.stRule[0].uDetectionTargetCount = 1;
    g_stIntrusionAlarmInfo.stRule[0].auDetectionTarget[0] = NET_TARGET_HUMAN;
    for (int day = 0; day < 7; day++)
    {
        g_stIntrusionAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stIntrusionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stIntrusionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stIntrusionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stIntrusionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 进入区域侦测配置默认值 */
    g_stEnterRegionAlarmInfo.bEnable = FALSE;
    g_stEnterRegionAlarmInfo.uRuleCount = 1;
    g_stEnterRegionAlarmInfo.stRule[0].bEnable = TRUE;
    g_stEnterRegionAlarmInfo.stRule[0].uPointCount = 4;
    g_stEnterRegionAlarmInfo.stRule[0].afPointX[0] = 0.2f; g_stEnterRegionAlarmInfo.stRule[0].afPointY[0] = 0.2f;
    g_stEnterRegionAlarmInfo.stRule[0].afPointX[1] = 0.8f; g_stEnterRegionAlarmInfo.stRule[0].afPointY[1] = 0.2f;
    g_stEnterRegionAlarmInfo.stRule[0].afPointX[2] = 0.8f; g_stEnterRegionAlarmInfo.stRule[0].afPointY[2] = 0.8f;
    g_stEnterRegionAlarmInfo.stRule[0].afPointX[3] = 0.2f; g_stEnterRegionAlarmInfo.stRule[0].afPointY[3] = 0.8f;
    g_stEnterRegionAlarmInfo.stRule[0].nTimeThreshold = 10;
    g_stEnterRegionAlarmInfo.stRule[0].nSensitivity = 50;
    g_stEnterRegionAlarmInfo.stRule[0].uDetectionTargetCount = 1;
    g_stEnterRegionAlarmInfo.stRule[0].auDetectionTarget[0] = NET_TARGET_HUMAN;
    for (int day = 0; day < 7; day++)
    {
        g_stEnterRegionAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stEnterRegionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stEnterRegionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stEnterRegionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stEnterRegionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 离开区域侦测配置默认值 */
    g_stLeaveRegionAlarmInfo.bEnable = g_stEnterRegionAlarmInfo.bEnable;
    g_stLeaveRegionAlarmInfo.uRuleCount = g_stEnterRegionAlarmInfo.uRuleCount;
    g_stLeaveRegionAlarmInfo.stRule[0] = g_stEnterRegionAlarmInfo.stRule[0];
    for (int day = 0; day < 7; day++)
    {
        g_stLeaveRegionAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stLeaveRegionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stLeaveRegionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stLeaveRegionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stLeaveRegionAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 徘徊侦测配置默认值 */
    g_stLoiteringAlarmInfo.bEnable = FALSE;
    g_stLoiteringAlarmInfo.uRuleCount = 1;
    g_stLoiteringAlarmInfo.stRule[0].bEnable = TRUE;
    g_stLoiteringAlarmInfo.stRule[0].uPointCount = 4;
    /* 矩形区域：左上、右上、右下、左下 */
    g_stLoiteringAlarmInfo.stRule[0].afPointX[0] = 0.2f; g_stLoiteringAlarmInfo.stRule[0].afPointY[0] = 0.2f;
    g_stLoiteringAlarmInfo.stRule[0].afPointX[1] = 0.8f; g_stLoiteringAlarmInfo.stRule[0].afPointY[1] = 0.2f;
    g_stLoiteringAlarmInfo.stRule[0].afPointX[2] = 0.8f; g_stLoiteringAlarmInfo.stRule[0].afPointY[2] = 0.8f;
    g_stLoiteringAlarmInfo.stRule[0].afPointX[3] = 0.2f; g_stLoiteringAlarmInfo.stRule[0].afPointY[3] = 0.8f;
    g_stLoiteringAlarmInfo.stRule[0].nTimeThreshold = 10;
    g_stLoiteringAlarmInfo.stRule[0].nSensitivity = 50;
    g_stLoiteringAlarmInfo.stRule[0].uDetectionTargetCount = 1;
    g_stLoiteringAlarmInfo.stRule[0].auDetectionTarget[0] = NET_TARGET_HUMAN;
    for (int day = 0; day < 7; day++)
    {
        g_stLoiteringAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stLoiteringAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stLoiteringAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stLoiteringAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stLoiteringAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 场景变更侦测配置默认值 */
    g_stSceneChangeAlarmInfo.bEnable = FALSE;
    g_stSceneChangeAlarmInfo.nSensitivity = 50;
    for (int day = 0; day < 7; day++)
    {
        g_stSceneChangeAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stSceneChangeAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stSceneChangeAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stSceneChangeAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stSceneChangeAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 人员聚集侦测配置默认值 */
    g_stCrowdGatheringAlarmInfo.bEnable = FALSE;
    g_stCrowdGatheringAlarmInfo.uRuleCount = 1;
    g_stCrowdGatheringAlarmInfo.astRule[0].uPointCount = 4;
    g_stCrowdGatheringAlarmInfo.astRule[0].afPointX[0] = 0.2f; g_stCrowdGatheringAlarmInfo.astRule[0].afPointY[0] = 0.2f;
    g_stCrowdGatheringAlarmInfo.astRule[0].afPointX[1] = 0.8f; g_stCrowdGatheringAlarmInfo.astRule[0].afPointY[1] = 0.2f;
    g_stCrowdGatheringAlarmInfo.astRule[0].afPointX[2] = 0.8f; g_stCrowdGatheringAlarmInfo.astRule[0].afPointY[2] = 0.8f;
    g_stCrowdGatheringAlarmInfo.astRule[0].afPointX[3] = 0.2f; g_stCrowdGatheringAlarmInfo.astRule[0].afPointY[3] = 0.8f;
    g_stCrowdGatheringAlarmInfo.astRule[0].nObjectOccup = 50;
    for (int day = 0; day < 7; day++)
    {
        g_stCrowdGatheringAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stCrowdGatheringAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stCrowdGatheringAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stCrowdGatheringAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stCrowdGatheringAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 停车侦测配置默认值 */
    g_stParkingAlarmInfo.bEnable = FALSE;
    g_stParkingAlarmInfo.uRuleCount = 1;
    g_stParkingAlarmInfo.astRule[0].uPointCount = 4;
    g_stParkingAlarmInfo.astRule[0].afPointX[0] = 0.2f; g_stParkingAlarmInfo.astRule[0].afPointY[0] = 0.2f;
    g_stParkingAlarmInfo.astRule[0].afPointX[1] = 0.8f; g_stParkingAlarmInfo.astRule[0].afPointY[1] = 0.2f;
    g_stParkingAlarmInfo.astRule[0].afPointX[2] = 0.8f; g_stParkingAlarmInfo.astRule[0].afPointY[2] = 0.8f;
    g_stParkingAlarmInfo.astRule[0].afPointX[3] = 0.2f; g_stParkingAlarmInfo.astRule[0].afPointY[3] = 0.8f;
    g_stParkingAlarmInfo.astRule[0].nSensitivity = 50;
    g_stParkingAlarmInfo.astRule[0].nTimeThreshold = 10;
    for (int day = 0; day < 7; day++)
    {
        g_stParkingAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stParkingAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stParkingAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stParkingAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stParkingAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 物品遗留侦测配置默认值 */
    g_stUnattendedObjectAlarmInfo.bEnable = FALSE;
    g_stUnattendedObjectAlarmInfo.uRuleCount = 1;
    g_stUnattendedObjectAlarmInfo.stRule[0].uPointCount = 4;
    g_stUnattendedObjectAlarmInfo.stRule[0].afPointX[0] = 0.2f; g_stUnattendedObjectAlarmInfo.stRule[0].afPointY[0] = 0.2f;
    g_stUnattendedObjectAlarmInfo.stRule[0].afPointX[1] = 0.8f; g_stUnattendedObjectAlarmInfo.stRule[0].afPointY[1] = 0.2f;
    g_stUnattendedObjectAlarmInfo.stRule[0].afPointX[2] = 0.8f; g_stUnattendedObjectAlarmInfo.stRule[0].afPointY[2] = 0.8f;
    g_stUnattendedObjectAlarmInfo.stRule[0].afPointX[3] = 0.2f; g_stUnattendedObjectAlarmInfo.stRule[0].afPointY[3] = 0.8f;
    g_stUnattendedObjectAlarmInfo.stRule[0].nSensitivity = 50;
    g_stUnattendedObjectAlarmInfo.stRule[0].nTimeThreshold = 30;
    for (int day = 0; day < 7; day++)
    {
        g_stUnattendedObjectAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stUnattendedObjectAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stUnattendedObjectAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stUnattendedObjectAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stUnattendedObjectAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 物品拿取侦测配置默认值 */
    g_stObjectRemovalAlarmInfo.bEnable = FALSE;
    g_stObjectRemovalAlarmInfo.uRuleCount = 1;
    g_stObjectRemovalAlarmInfo.stRule[0].uPointCount = 4;
    g_stObjectRemovalAlarmInfo.stRule[0].afPointX[0] = 0.2f; g_stObjectRemovalAlarmInfo.stRule[0].afPointY[0] = 0.2f;
    g_stObjectRemovalAlarmInfo.stRule[0].afPointX[1] = 0.8f; g_stObjectRemovalAlarmInfo.stRule[0].afPointY[1] = 0.2f;
    g_stObjectRemovalAlarmInfo.stRule[0].afPointX[2] = 0.8f; g_stObjectRemovalAlarmInfo.stRule[0].afPointY[2] = 0.8f;
    g_stObjectRemovalAlarmInfo.stRule[0].afPointX[3] = 0.2f; g_stObjectRemovalAlarmInfo.stRule[0].afPointY[3] = 0.8f;
    g_stObjectRemovalAlarmInfo.stRule[0].nSensitivity = 50;
    g_stObjectRemovalAlarmInfo.stRule[0].nTimeThreshold = 30;
    for (int day = 0; day < 7; day++)
    {
        g_stObjectRemovalAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stObjectRemovalAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stObjectRemovalAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stObjectRemovalAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stObjectRemovalAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 音频异常侦测配置默认值 */
    g_stAudioAnomalyAlarmInfo.bEnable = FALSE;
    g_stAudioAnomalyAlarmInfo.bAudioInputAnomaly = TRUE;
    g_stAudioAnomalyAlarmInfo.bUpEnable = TRUE;
    g_stAudioAnomalyAlarmInfo.nUpSensitivity = 50;
    g_stAudioAnomalyAlarmInfo.nUpThreshold = 50;
    g_stAudioAnomalyAlarmInfo.bDownEnable = FALSE;
    g_stAudioAnomalyAlarmInfo.nDownSensitivity = 50;
    for (int day = 0; day < 7; day++)
    {
        g_stAudioAnomalyAlarmInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stAudioAnomalyAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stAudioAnomalyAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stAudioAnomalyAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stAudioAnomalyAlarmInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 声音报警配置默认值。 */
    gs_stAudibleAlarmInfo.enSoundType = NET_AUDIBLE_ALARM_SOUND_TYPE_ALERT;
    gs_stAudibleAlarmInfo.enAlertSound = NET_AUDIBLE_ALARM_ALERT_SOUND_GENERAL_WARNING_TONE;
    gs_stAudibleAlarmInfo.nTimes = NET_AUDIBLE_ALARM_PLAY_TIMES_MIN;
    ConfigDemoInitAlarmSchedule(&gs_stAudibleAlarmInfo.stAlarmSchedule);

    /* 报警输入配置默认值。 */
    gs_stAlarmInputInfoList.nAlarmInputCount = 1;
    gs_stAlarmInputInfoList.astAlarmInputs[DEMO_ALARM_CHANNEL_INDEX].nAlarmNumber = DEMO_ALARM_CHANNEL_INDEX;
    CopyString(gs_stAlarmInputInfoList.astAlarmInputs[DEMO_ALARM_CHANNEL_INDEX].strAlarmAddress,
               sizeof(gs_stAlarmInputInfoList.astAlarmInputs[DEMO_ALARM_CHANNEL_INDEX].strAlarmAddress),
               "AlarmInput0");
    CopyString(gs_stAlarmInputInfoList.astAlarmInputs[DEMO_ALARM_CHANNEL_INDEX].strAlarmName,
               sizeof(gs_stAlarmInputInfoList.astAlarmInputs[DEMO_ALARM_CHANNEL_INDEX].strAlarmName),
               "Demo Alarm Input");
    gs_stAlarmInputInfoList.astAlarmInputs[DEMO_ALARM_CHANNEL_INDEX].bNormallyOpen = TRUE;
    gs_stAlarmInputInfoList.astAlarmInputs[DEMO_ALARM_CHANNEL_INDEX].nDealType = NET_ALARM_INPUT_DEAL_TYPE_ENABLED;
    ConfigDemoInitAlarmSchedule(&gs_stAlarmInputInfoList.astAlarmInputs[DEMO_ALARM_CHANNEL_INDEX].stAlarmSchedule);

    /* 报警输出配置默认值。 */
    gs_stAlarmOutputInfoList.nAlarmOutputCount = 1;
    gs_stAlarmOutputInfoList.astAlarmOutputs[DEMO_ALARM_CHANNEL_INDEX].nAlarmNumber = DEMO_ALARM_CHANNEL_INDEX;
    CopyString(gs_stAlarmOutputInfoList.astAlarmOutputs[DEMO_ALARM_CHANNEL_INDEX].strAlarmAddress,
               sizeof(gs_stAlarmOutputInfoList.astAlarmOutputs[DEMO_ALARM_CHANNEL_INDEX].strAlarmAddress),
               "AlarmOutput0");
    CopyString(gs_stAlarmOutputInfoList.astAlarmOutputs[DEMO_ALARM_CHANNEL_INDEX].strAlarmName,
               sizeof(gs_stAlarmOutputInfoList.astAlarmOutputs[DEMO_ALARM_CHANNEL_INDEX].strAlarmName),
               "Demo Alarm Output");
    gs_stAlarmOutputInfoList.astAlarmOutputs[DEMO_ALARM_CHANNEL_INDEX].nDelayTime = DEMO_ALARM_OUTPUT_DELAY_SECONDS;
    gs_stAlarmOutputInfoList.astAlarmOutputs[DEMO_ALARM_CHANNEL_INDEX].enState = NET_ALARM_OUTPUT_STATE_OFF;
    ConfigDemoInitAlarmSchedule(&gs_stAlarmOutputInfoList.astAlarmOutputs[DEMO_ALARM_CHANNEL_INDEX].stAlarmSchedule);

    /* 闪光报警灯配置默认值。 */
    gs_stFlashingLightAlarmInfo.nFlashTime = NET_FLASHING_LIGHT_ALARM_TIME_MIN;
    gs_stFlashingLightAlarmInfo.enFlashFrequency = NET_FLASHING_LIGHT_FREQUENCY_MIDDLE;
    ConfigDemoInitAlarmSchedule(&gs_stFlashingLightAlarmInfo.stAlarmSchedule);

    /* PIR 报警配置默认值。 */
    gs_stPirAlarmInfo.bEnable = TRUE;
    CopyString(gs_stPirAlarmInfo.strAlarmName,
               sizeof(gs_stPirAlarmInfo.strAlarmName),
               "Demo PIR Alarm");
    ConfigDemoInitAlarmSchedule(&gs_stPirAlarmInfo.stAlarmSchedule);

    /* 图像配置默认值，对应 ISP::ImageParam_S */
    g_stImageCfg.nBrightness = 50;
    g_stImageCfg.nContrast = 50;
    g_stImageCfg.nSaturation = 50;
    g_stImageCfg.nSharpness = 50;

    /* 预览信息默认值 */
    strncpy(g_stPreviewInfo.stRtspUrl.szRtspMainUrl,
            "rtsp://127.0.0.1:554/live/main",
            sizeof(g_stPreviewInfo.stRtspUrl.szRtspMainUrl) - 1);
    g_stPreviewInfo.stRtspUrl.szRtspMainUrl[sizeof(g_stPreviewInfo.stRtspUrl.szRtspMainUrl) - 1] = '\0';
    strncpy(g_stPreviewInfo.stRtspUrl.szRtspSubUrl,
            "rtsp://127.0.0.1:554/live/sub",
            sizeof(g_stPreviewInfo.stRtspUrl.szRtspSubUrl) - 1);
    g_stPreviewInfo.stRtspUrl.szRtspSubUrl[sizeof(g_stPreviewInfo.stRtspUrl.szRtspSubUrl) - 1] = '\0';
    g_stPreviewInfo.stImageParam.nBrightness = 50;
    g_stPreviewInfo.stImageParam.nContrast = 50;
    g_stPreviewInfo.stImageParam.nSaturation = 50;
    g_stPreviewInfo.stImageParam.nSharpness = 50;

      /* 通道信息默认值 */
    g_stChannelInfo.byEnable = TRUE;
    g_stChannelInfo.byOnline = TRUE;
    g_stChannelInfo.byStreamType = 0;
    g_stChannelInfo.uChannel = 1;

    strncpy(g_stChannelInfo.szChannelName,
            "Demo-Channel-1",
            sizeof(g_stChannelInfo.szChannelName) - 1);
    strncpy(g_stChannelInfo.szRtspMainUrl,
            g_stPreviewInfo.stRtspUrl.szRtspMainUrl,
            sizeof(g_stChannelInfo.szRtspMainUrl) - 1);
    strncpy(g_stChannelInfo.szRtspSubUrl,
            g_stPreviewInfo.stRtspUrl.szRtspSubUrl,
            sizeof(g_stChannelInfo.szRtspSubUrl) - 1);
    strncpy(g_stChannelInfo.szPreviewMainUrl,
            g_stPreviewInfo.stRtspUrl.szRtspMainUrl,
            sizeof(g_stChannelInfo.szPreviewMainUrl) - 1);
    strncpy(g_stChannelInfo.szPreviewSubUrl,
            g_stPreviewInfo.stRtspUrl.szRtspSubUrl,
            sizeof(g_stChannelInfo.szPreviewSubUrl) - 1);
    strncpy(g_stChannelInfo.szDeviceIP,
            "127.0.0.1",
            sizeof(g_stChannelInfo.szDeviceIP) - 1);

    /* 对讲相关默认值 */
    g_stTalkbackStateInfo.bEnable = FALSE;
    strncpy(g_stTalkbackStateInfo.szSdp,
            "v=0\\r\\no=- 0 0 IN IP4 127.0.0.1\\r\\ns=Talkback\\r\\n",
            sizeof(g_stTalkbackStateInfo.szSdp) - 1);
    strncpy(g_stTalkbackStateInfo.szUrl,
            "rtsp://127.0.0.1:554/talkback",
            sizeof(g_stTalkbackStateInfo.szUrl) - 1);
    strncpy(g_stTalkbackStateInfo.szLocalIP,
            "127.0.0.1",
            sizeof(g_stTalkbackStateInfo.szLocalIP) - 1);

    strncpy(g_stTalkbackToStreamInfo.szHost, "239.0.0.1", sizeof(g_stTalkbackToStreamInfo.szHost) - 1);
    g_stTalkbackToStreamInfo.nPort = 5004;
    g_stTalkbackToStreamInfo.nChnId = 1;
    g_stTalkbackToStreamInfo.nUserID = 1001;
    g_stTalkbackToStreamInfo.bMainStream = TRUE;
    strncpy(g_stTalkbackToStreamInfo.szProtocol, "rtp", sizeof(g_stTalkbackToStreamInfo.szProtocol) - 1);
    strncpy(g_stTalkbackToStreamInfo.szStartTime, "2026-01-01 08:00:00", sizeof(g_stTalkbackToStreamInfo.szStartTime) - 1);
    strncpy(g_stTalkbackToStreamInfo.szEndTime, "2026-01-01 08:30:00", sizeof(g_stTalkbackToStreamInfo.szEndTime) - 1);
    strncpy(g_stTalkbackToStreamInfo.szFileName, "talkback_live.aac", sizeof(g_stTalkbackToStreamInfo.szFileName) - 1);

    g_stTalkbackFromStreamInfo = g_stTalkbackToStreamInfo;
    strncpy(g_stTalkbackFromStreamInfo.szHost, "239.0.0.2", sizeof(g_stTalkbackFromStreamInfo.szHost) - 1);

    strncpy(g_stReplayTalkbackInfo.szNvrIp, "192.168.1.200", sizeof(g_stReplayTalkbackInfo.szNvrIp) - 1);
    strncpy(g_stReplayTalkbackInfo.szRemoteIp, "192.168.1.10", sizeof(g_stReplayTalkbackInfo.szRemoteIp) - 1);
    g_stReplayTalkbackInfo.stIPCInfo = g_stTalkbackToStreamInfo;
    strncpy(g_stReplayTalkbackInfo.stIPCInfo.szFileName, "talkback_replay.aac", sizeof(g_stReplayTalkbackInfo.stIPCInfo.szFileName) - 1);

    /* 抓图计划配置默认值 */
    UINT32 i = 0;
    memset(&g_stCapturePlanInfo, 0, sizeof(g_stCapturePlanInfo));
    for (i = 0; i < NET_PLAN_DAY_NUM_AWEEK; ++i)
    {
        g_stCapturePlanInfo.astDaySchedules[i].nDayOfWeek = (INT32)(i + 1);
        g_stCapturePlanInfo.astDaySchedules[i].udwTimeCount = 1;
        g_stCapturePlanInfo.astDaySchedules[i].astTimes[0].nStartTime = 0;
        g_stCapturePlanInfo.astDaySchedules[i].astTimes[0].nEndTime = 24 * 60 * 60;
    }

    /* 抓图参数默认值 */
    memset(&g_stCaptureParamInfo, 0, sizeof(g_stCaptureParamInfo));

    g_stCaptureParamInfo.stCaptureTimingConfig.bEnable = TRUE;
    g_stCaptureParamInfo.stCaptureTimingConfig.enPictureFormat = NET_CAPTURE_PICTURE_FORMAT_JPEG;
    g_stCaptureParamInfo.stCaptureTimingConfig.nWidth = 1920;
    g_stCaptureParamInfo.stCaptureTimingConfig.nHeight = 1080;
    g_stCaptureParamInfo.stCaptureTimingConfig.enImageQuality = NET_CAPTURE_IMAGE_QUALITY_MEDIUM;
    g_stCaptureParamInfo.stCaptureTimingConfig.unInterval = 2000;
    g_stCaptureParamInfo.stCaptureTimingConfig.enTimeUnit = NET_CAPTURE_TIME_UNIT_MILLISECONDS;
    g_stCaptureParamInfo.stCaptureTimingConfig.unNumber = 20;

    g_stCaptureParamInfo.stCaptureEventConfig.bEnable = TRUE;
    g_stCaptureParamInfo.stCaptureEventConfig.enPictureFormat = NET_CAPTURE_PICTURE_FORMAT_JPEG;
    g_stCaptureParamInfo.stCaptureEventConfig.nWidth = 1920;
    g_stCaptureParamInfo.stCaptureEventConfig.nHeight = 1080;
    g_stCaptureParamInfo.stCaptureEventConfig.enImageQuality = NET_CAPTURE_IMAGE_QUALITY_HIGH;
    g_stCaptureParamInfo.stCaptureEventConfig.unInterval = 1;
    g_stCaptureParamInfo.stCaptureEventConfig.enTimeUnit = NET_CAPTURE_TIME_UNIT_SECONDS;
    g_stCaptureParamInfo.stCaptureEventConfig.unNumber = 1;

    /* 曝光信息默认值 */
    g_stExposureInfo.enExpTime = 0;
    g_stExposureInfo.bAntiBanding = TRUE;

    /* 日夜转换信息默认值 */
    g_stDayNightInfo.enDayNightMode = 0;
    g_stDayNightInfo.nBeginHour = 18;
    g_stDayNightInfo.nBeginMinute = 0;
    g_stDayNightInfo.nBeginSecond = 0;
    g_stDayNightInfo.nBeginMilliSec = 0;
    g_stDayNightInfo.nEndHour = 6;
    g_stDayNightInfo.nEndMinute = 0;
    g_stDayNightInfo.nEndSecond = 0;
    g_stDayNightInfo.nEndMilliSec = 0;
    g_stDayNightInfo.nSensitivityLevel = 50;
    g_stDayNightInfo.nFilterTime = 5;
    g_stDayNightInfo.bFillLightExp = TRUE;
    g_stDayNightInfo.enLightMode = 0;
    g_stDayNightInfo.enLightType = 0;
    g_stDayNightInfo.bWhiteLightEnable = TRUE;
    g_stDayNightInfo.nWhiteLightLevel = 50;
    g_stDayNightInfo.bRedLightEnable = FALSE;
    g_stDayNightInfo.nRedLightLevel = 0;

    /* 背光信息默认值 */
    g_stBackLightInfo.enBackLightArea = 0;
    g_stBackLightInfo.bWdrEnable = TRUE;
    g_stBackLightInfo.nWdrLevel = 50;
    g_stBackLightInfo.bHlsEnable = FALSE;
    g_stBackLightInfo.nHlsLevel = 0;

    /* 降噪信息默认值 */
    g_stDenoiseInfo.enDnrMode = 0;
    g_stDenoiseInfo.nDnrLevel = 50;
    g_stDenoiseInfo.nSnrLevel = 50;
    g_stDenoiseInfo.nTnrLevel = 50;

    /* 白平衡信息默认值 */
    g_stWhiteBalanceInfo.enAwbMode = 0;
    g_stWhiteBalanceInfo.nRGain = 100;
    g_stWhiteBalanceInfo.nBGain = 100;

    /* 人脸抓拍配置默认值 */
    g_stFaceCaptureInfo.bEnable = FALSE;
    g_stFaceCaptureInfo.stRule.nSensitivity = 55;
    g_stFaceCaptureInfo.stRule.stRegion.uPointCount = 4;
    g_stFaceCaptureInfo.stRule.stRegion.afPointX[0] = 0.20f; g_stFaceCaptureInfo.stRule.stRegion.afPointY[0] = 0.20f;
    g_stFaceCaptureInfo.stRule.stRegion.afPointX[1] = 0.80f; g_stFaceCaptureInfo.stRule.stRegion.afPointY[1] = 0.20f;
    g_stFaceCaptureInfo.stRule.stRegion.afPointX[2] = 0.80f; g_stFaceCaptureInfo.stRule.stRegion.afPointY[2] = 0.80f;
    g_stFaceCaptureInfo.stRule.stRegion.afPointX[3] = 0.20f; g_stFaceCaptureInfo.stRule.stRegion.afPointY[3] = 0.80f;
    g_stFaceCaptureInfo.stRule.uShieldRegionCount = 1;
    g_stFaceCaptureInfo.stRule.astShieldRegion[0].uPointCount = 4;
    g_stFaceCaptureInfo.stRule.astShieldRegion[0].afPointX[0] = 0.45f; g_stFaceCaptureInfo.stRule.astShieldRegion[0].afPointY[0] = 0.45f;
    g_stFaceCaptureInfo.stRule.astShieldRegion[0].afPointX[1] = 0.55f; g_stFaceCaptureInfo.stRule.astShieldRegion[0].afPointY[1] = 0.45f;
    g_stFaceCaptureInfo.stRule.astShieldRegion[0].afPointX[2] = 0.55f; g_stFaceCaptureInfo.stRule.astShieldRegion[0].afPointY[2] = 0.55f;
    g_stFaceCaptureInfo.stRule.astShieldRegion[0].afPointX[3] = 0.45f; g_stFaceCaptureInfo.stRule.astShieldRegion[0].afPointY[3] = 0.55f;
    g_stFaceCaptureInfo.stRule.nMinIpdRectLeft = 10;
    g_stFaceCaptureInfo.stRule.nMinIpdRectTop = 10;
    g_stFaceCaptureInfo.stRule.nMinIpdRectRight = 90;
    g_stFaceCaptureInfo.stRule.nMinIpdRectBottom = 90;
    g_stFaceCaptureInfo.stRule.nMinWidth = 20;
    g_stFaceCaptureInfo.stRule.nMinHeight = 20;
    g_stFaceCaptureInfo.stRule.nMaxWidth = 300;
    g_stFaceCaptureInfo.stRule.nMaxHeight = 300;
    g_stFaceCaptureInfo.stRule.nInterval = 2;
    for (int day = 0; day < 7; day++)
    {
        g_stFaceCaptureInfo.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stFaceCaptureInfo.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stFaceCaptureInfo.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stFaceCaptureInfo.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stFaceCaptureInfo.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 人脸比对配置默认值 */
    g_stFaceCompareInfo.bEnable = FALSE;
    InitSimpleAiAlarmSchedule(&g_stFaceCompareInfo.stAlarmSchedule);
    g_stFaceCompareInfo.stLinkageListSuccess.uSnapshotChannelCount = 1;
    g_stFaceCompareInfo.stLinkageListSuccess.auSnapshotChannel[0] = 1;
    g_stFaceCompareInfo.stLinkageListFail.uSnapshotChannelCount = 1;
    g_stFaceCompareInfo.stLinkageListFail.auSnapshotChannel[0] = 1;

    /* 目标库默认值 */
    g_stFaceLibList.nTargetLibCount = 1;
    strncpy(g_stFaceLibList.astTargetLibInfos[0].szFaceLibName,
            "default_library",
            sizeof(g_stFaceLibList.astTargetLibInfos[0].szFaceLibName) - 1);
    g_stFaceLibList.astTargetLibInfos[0].nTotalFace = 1;
    g_stFaceLibList.astTargetLibInfos[0].nNormalNum = 1;
    g_stFaceLibList.astTargetLibInfos[0].nAbnormalNum = 0;

    /* 人脸信息默认值 */
    g_stFaceInfoList.nFaceInfoCount = 1;
    g_stFaceInfoList.astFaceInfos[0].nId = 1;
    strncpy(g_stFaceInfoList.astFaceInfos[0].szFaceLibName,
            "default_library",
            sizeof(g_stFaceInfoList.astFaceInfos[0].szFaceLibName) - 1);
    strncpy(g_stFaceInfoList.astFaceInfos[0].szName,
            "demo_person",
            sizeof(g_stFaceInfoList.astFaceInfos[0].szName) - 1);
    strncpy(g_stFaceInfoList.astFaceInfos[0].szPhoneNum,
            "13800000000",
            sizeof(g_stFaceInfoList.astFaceInfos[0].szPhoneNum) - 1);
    strncpy(g_stFaceInfoList.astFaceInfos[0].szPicPath,
            "/tmp/demo_person.jpg",
            sizeof(g_stFaceInfoList.astFaceInfos[0].szPicPath) - 1);
    strncpy(g_stFaceInfoList.astFaceInfos[0].szBinPath,
            "/tmp/demo_person.bin",
            sizeof(g_stFaceInfoList.astFaceInfos[0].szBinPath) - 1);
    strncpy(g_stFaceInfoList.astFaceInfos[0].szPicType,
            "jpg",
            sizeof(g_stFaceInfoList.astFaceInfos[0].szPicType) - 1);
    g_stFaceInfoList.astFaceInfos[0].nPicSize = 1024;
    strncpy(g_stFaceInfoList.astFaceInfos[0].szPicDate,
            "2026-05-07 10:00:00",
            sizeof(g_stFaceInfoList.astFaceInfos[0].szPicDate) - 1);
    g_stFaceInfoList.astFaceInfos[0].nModelState = 1;
    g_stFaceInfoList.astFaceInfos[0].nRatingLevel = 3;

    /* 垃圾暴露配置默认值 */
    g_stGarbageExposureCfg.bEnable = FALSE;
    g_stGarbageExposureCfg.stRule.nSensitivity = 50;
    g_stGarbageExposureCfg.stRule.uPointCount = 4;
    g_stGarbageExposureCfg.stRule.afPointX[0] = 0.2f; g_stGarbageExposureCfg.stRule.afPointY[0] = 0.2f;
    g_stGarbageExposureCfg.stRule.afPointX[1] = 0.8f; g_stGarbageExposureCfg.stRule.afPointY[1] = 0.2f;
    g_stGarbageExposureCfg.stRule.afPointX[2] = 0.8f; g_stGarbageExposureCfg.stRule.afPointY[2] = 0.8f;
    g_stGarbageExposureCfg.stRule.afPointX[3] = 0.2f; g_stGarbageExposureCfg.stRule.afPointY[3] = 0.8f;
    for (int day = 0; day < 7; day++)
    {
        g_stGarbageExposureCfg.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stGarbageExposureCfg.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stGarbageExposureCfg.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stGarbageExposureCfg.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stGarbageExposureCfg.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    /* 垃圾满溢配置默认值 */
    g_stGarbageOverflowCfg.bEnable = FALSE;
    g_stGarbageOverflowCfg.stRule.nSensitivity = 50;
    g_stGarbageOverflowCfg.stRule.uPointCount = 4;
    g_stGarbageOverflowCfg.stRule.afPointX[0] = 0.2f; g_stGarbageOverflowCfg.stRule.afPointY[0] = 0.2f;
    g_stGarbageOverflowCfg.stRule.afPointX[1] = 0.8f; g_stGarbageOverflowCfg.stRule.afPointY[1] = 0.2f;
    g_stGarbageOverflowCfg.stRule.afPointX[2] = 0.8f; g_stGarbageOverflowCfg.stRule.afPointY[2] = 0.8f;
    g_stGarbageOverflowCfg.stRule.afPointX[3] = 0.2f; g_stGarbageOverflowCfg.stRule.afPointY[3] = 0.8f;
    g_stGarbageOverflowCfg.nTimeThreshold = 60;
    for (int day = 0; day < 7; day++)
    {
        g_stGarbageOverflowCfg.stAlarmSchedule.uTimeSectionCount[day] = 1;
        g_stGarbageOverflowCfg.stAlarmSchedule.astTimeSection[day][0].nStartHour = 0;
        g_stGarbageOverflowCfg.stAlarmSchedule.astTimeSection[day][0].nStartMinute = 0;
        g_stGarbageOverflowCfg.stAlarmSchedule.astTimeSection[day][0].nEndHour = 23;
        g_stGarbageOverflowCfg.stAlarmSchedule.astTimeSection[day][0].nEndMinute = 59;
    }

    g_stManholeCoverAbnormalCfg.bEnable = FALSE;
    g_stManholeCoverAbnormalCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stManholeCoverAbnormalCfg.stAlarmSchedule);

    g_stSleepOnDutyCfg.bEnable = FALSE;
    g_stSleepOnDutyCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stSleepOnDutyCfg.stAlarmSchedule);

    g_stElectricVehicleInElevatorCfg.bEnable = FALSE;
    g_stElectricVehicleInElevatorCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stElectricVehicleInElevatorCfg.stAlarmSchedule);

    g_stPersonFallDownCfg.bEnable = FALSE;
    g_stPersonFallDownCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stPersonFallDownCfg.stAlarmSchedule);

    g_stConstructionOccupyRoadCfg.bEnable = FALSE;
    g_stConstructionOccupyRoadCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stConstructionOccupyRoadCfg.stAlarmSchedule);

    g_stCongestionCfg.bEnable = FALSE;
    g_stCongestionCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stCongestionCfg.stAlarmSchedule);

    g_stLicensePlateRecognitionCfg.bEnable = FALSE;
    g_stLicensePlateRecognitionCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stLicensePlateRecognitionCfg.stAlarmSchedule);

    g_stHighAltitudeSeatbeltCfg.bEnable = FALSE;
    g_stHighAltitudeSeatbeltCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stHighAltitudeSeatbeltCfg.stAlarmSchedule);

    g_stSafetyHelmetCfg.bEnable = FALSE;
    g_stSafetyHelmetCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stSafetyHelmetCfg.stAlarmSchedule);

    g_stPersonFallCfg.bEnable = FALSE;
    g_stPersonFallCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stPersonFallCfg.stAlarmSchedule);

    g_stPhoneUsageCfg.bEnable = FALSE;
    g_stPhoneUsageCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stPhoneUsageCfg.stAlarmSchedule);

    g_stSmokingCfg.bEnable = FALSE;
    g_stSmokingCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stSmokingCfg.stAlarmSchedule);

    g_stOpenFlameCfg.bEnable = FALSE;
    g_stOpenFlameCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stOpenFlameCfg.stAlarmSchedule);

    g_stBareSoilCfg.bEnable = FALSE;
    g_stBareSoilCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stBareSoilCfg.stAlarmSchedule);

    g_stHoleProtectionBarCfg.bEnable = FALSE;
    g_stHoleProtectionBarCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stHoleProtectionBarCfg.stAlarmSchedule);

    g_stReflectiveClothingCfg.bEnable = FALSE;
    g_stReflectiveClothingCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stReflectiveClothingCfg.stAlarmSchedule);

    g_stPetRecognitionInfo.bEnable = FALSE;
    g_stPetRecognitionInfo.bDynamicAnalysisEnable = FALSE;
    g_stPetRecognitionInfo.nSensitivity = 50;
    InitDemoSmartRegion(&g_stPetRecognitionInfo.stRegion);
    InitSimpleAiAlarmSchedule(&g_stPetRecognitionInfo.stAlarmSchedule);

    g_stClimbFenceInfo.bEnable = FALSE;
    g_stClimbFenceInfo.uRuleCount = 1;
    InitDemoSmartRegionRule(&g_stClimbFenceInfo.stRule[0]);
    InitSimpleAiAlarmSchedule(&g_stClimbFenceInfo.stAlarmSchedule);

    g_stDimissionInfo.bEnable = FALSE;
    g_stDimissionInfo.uRuleCount = 1;
    InitDemoSmartRegionRule(&g_stDimissionInfo.stRule[0]);
    InitSimpleAiAlarmSchedule(&g_stDimissionInfo.stAlarmSchedule);

    g_stIllegalLaneInfo.bEnable = FALSE;
    g_stIllegalLaneInfo.uRuleCount = 1;
    InitDemoSmartLineRule(&g_stIllegalLaneInfo.stRule[0]);
    InitSimpleAiAlarmSchedule(&g_stIllegalLaneInfo.stAlarmSchedule);

    g_stRetrogradeInfo.bEnable = FALSE;
    g_stRetrogradeInfo.uRuleCount = 1;
    InitDemoSmartLineRule(&g_stRetrogradeInfo.stRule[0]);
    InitSimpleAiAlarmSchedule(&g_stRetrogradeInfo.stAlarmSchedule);

    g_stNonmotorVehicleIntrusionInfo.bEnable = FALSE;
    g_stNonmotorVehicleIntrusionInfo.uRuleCount = 1;
    InitDemoSmartRegionRule(&g_stNonmotorVehicleIntrusionInfo.stRule[0]);
    InitSimpleAiAlarmSchedule(&g_stNonmotorVehicleIntrusionInfo.stAlarmSchedule);

    g_stOccupationEmergencyInfo.bEnable = FALSE;
    g_stOccupationEmergencyInfo.uRuleCount = 1;
    InitDemoSmartRegionRule(&g_stOccupationEmergencyInfo.stRule[0]);
    InitSimpleAiAlarmSchedule(&g_stOccupationEmergencyInfo.stAlarmSchedule);

    g_stPedestrianIntrusionInfo.bEnable = FALSE;
    g_stPedestrianIntrusionInfo.uRuleCount = 1;
    InitDemoSmartRegionRule(&g_stPedestrianIntrusionInfo.stRule[0]);
    InitSimpleAiAlarmSchedule(&g_stPedestrianIntrusionInfo.stAlarmSchedule);

    g_stSmokeFireCfg.bEnable = FALSE;
    g_stSmokeFireCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stSmokeFireCfg.stAlarmSchedule);

    g_stRoadPondingCfg.bEnable = FALSE;
    g_stRoadPondingCfg.stRule.nSensitivity = 50;
    InitSimpleAiAlarmSchedule(&g_stRoadPondingCfg.stAlarmSchedule);
}


/* ====================== 设备信息回调（用于登录获取基本能力） ====================== */

static NET_COMMON_ECODE_E MyDeviceInfoCb(pNET_DeviceInfo_S pInfo)
{
    if (!pInfo)
    {
        return NET_E_INVALID_PARAM;
    }

    printf("[ConfigServerDemo] GetDeviceInfo callback\n");

    memset(pInfo, 0, sizeof(NET_DeviceInfo_S));
    pInfo->uDevType        = 0;
    pInfo->uAlarmInPortNum  = 0;
    pInfo->uAlarmOutPortNum = 0;
    pInfo->uChannelNum     = 1; /* 单通道示例 */

    return NET_E_SUCCEED;
}

/* ====================== 配置 Get/Set 回调实现 ====================== */

/* 设备基本信息 Get 回调，对应命令 NET_GET_DEVICECFG */
static NET_COMMON_ECODE_E MyGetDeviceCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_DeviceBasicInfo_S pOut = (pNET_DeviceBasicInfo_S)lpOutBuffer;
    *pOut = g_stDeviceBasicInfo;

    printf("[ConfigServerDemo] GetDeviceCfg callback, Channel=%d\n", dwChannelID);
    printf("  DevModel=%s\n", g_stDeviceBasicInfo.strDevModel);
    printf("  SerialNum=%s\n", g_stDeviceBasicInfo.strSerialNum);
    printf("  DeviceName=%s\n", g_stDeviceBasicInfo.strDeviceName);

    return NET_E_SUCCEED;
}

/* 设备基本信息 Set 回调，对应命令 NET_SET_DEVICECFG */
static NET_COMMON_ECODE_E MySetDeviceCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_DeviceBasicInfo_S pIn = (pNET_DeviceBasicInfo_S)lpInBuffer;
    g_stDeviceBasicInfo = *pIn;

    printf("[ConfigServerDemo] SetDeviceCfg callback, Channel=%d\n", dwChannelID);
    printf("  New DevModel=%s\n", g_stDeviceBasicInfo.strDevModel);
    printf("  New SerialNum=%s\n", g_stDeviceBasicInfo.strSerialNum);
    printf("  New DeviceName=%s\n", g_stDeviceBasicInfo.strDeviceName);

    return NET_E_SUCCEED;
}

/* 音频配置 Get 回调，对应命令 NET_GET_AUDIOCFG */
static NET_COMMON_ECODE_E MyGetAudioCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_AudioCfg_S pOut = (pNET_AudioCfg_S)lpOutBuffer;
    *pOut = g_stAudioCfg;

    printf("[ConfigServerDemo] GetAudioCfg callback, Channel=%d\n", dwChannelID);
    printf("  AudioSwitch=%d, InputType=%d, Format=%d, SampRate=%d, BitRate=%d, InputVolume=%u, Denoise=%d, OutputType=%d, OutputVolume=%u\n",
           g_stAudioCfg.bAudioSwitch,
           g_stAudioCfg.enInputType,
           g_stAudioCfg.enFormat,
           g_stAudioCfg.enSampRate,
           g_stAudioCfg.enBitRate,
           g_stAudioCfg.u32InputVolume,
           g_stAudioCfg.bDenoise,
           g_stAudioCfg.enOutputType,
           g_stAudioCfg.u32OutputVolume);

    return NET_E_SUCCEED;
}

/* 音频配置 Set 回调，对应命令 NET_SET_AUDIOCFG */
static NET_COMMON_ECODE_E MySetAudioCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_AudioCfg_S pIn = (pNET_AudioCfg_S)lpInBuffer;
    g_stAudioCfg = *pIn;

    printf("[ConfigServerDemo] SetAudioCfg callback, Channel=%d\n", dwChannelID);
    printf("  New AudioSwitch=%d, InputType=%d, Format=%d, SampRate=%d, BitRate=%d, InputVolume=%u, Denoise=%d, OutputType=%d, OutputVolume=%u\n",
           g_stAudioCfg.bAudioSwitch, g_stAudioCfg.enInputType, g_stAudioCfg.enFormat, g_stAudioCfg.enSampRate,
           g_stAudioCfg.enBitRate, g_stAudioCfg.u32InputVolume, g_stAudioCfg.bDenoise, g_stAudioCfg.enOutputType, g_stAudioCfg.u32OutputVolume);
    return NET_E_SUCCEED;
}

/* 视频码流配置 Get 回调，对应命令 NET_GET_STREAMCFG */
static NET_COMMON_ECODE_E MyGetStreamCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_VideoEncodeOption_S pOut = (pNET_VideoEncodeOption_S)lpOutBuffer;
    *pOut = g_stStreamCfg;

    printf("[ConfigServerDemo] GetStreamCfg callback, Channel=%d\n", dwChannelID);
    printf("  Id=%d, Codec=%d, Resolution=%dx%d, FrameRate=%d, AverageBitrate=%d, BitrateUpperLimit=%d\n",
           g_stStreamCfg.nId,
           g_stStreamCfg.enVideoCodec,
           g_stStreamCfg.stVideoResolution.uWidth,
           g_stStreamCfg.stVideoResolution.uHeight,
           g_stStreamCfg.enFrameRate,
           g_stStreamCfg.nAverageBitrate,
           g_stStreamCfg.nBitrateUpperLimit);

    return NET_E_SUCCEED;
}

/* 视频码流配置 Set 回调，对应命令 NET_SET_STREAMCFG */
static NET_COMMON_ECODE_E MySetStreamCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_VideoEncodeOption_S pIn = (pNET_VideoEncodeOption_S)lpInBuffer;
    g_stStreamCfg = *pIn;

    printf("[ConfigServerDemo] SetStreamCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Id=%d, Codec=%d, Resolution=%dx%d, FrameRate=%d, AverageBitrate=%d, BitrateUpperLimit=%d\n",
           g_stStreamCfg.nId,
           g_stStreamCfg.enVideoCodec,
           g_stStreamCfg.stVideoResolution.uWidth,
           g_stStreamCfg.stVideoResolution.uHeight,
           g_stStreamCfg.enFrameRate,
           g_stStreamCfg.nAverageBitrate,
           g_stStreamCfg.nBitrateUpperLimit);

    return NET_E_SUCCEED;
}

/* 网络配置 Get 回调，对应命令 NET_GET_NETWORKCFG */
static NET_COMMON_ECODE_E MyGetNetworkCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_NetworkCfg_S pOut = (pNET_NetworkCfg_S)lpOutBuffer;
    *pOut = g_stNetworkCfg;

    printf("[ConfigServerDemo] GetNetworkCfg callback, Channel=%d\n", dwChannelID);
    printf("  IPv4Address=%s\n", g_stNetworkCfg.szIpv4Address);
    printf("  IPv4Gateway=%s\n", g_stNetworkCfg.szIPv4GateWay);
    printf("  IPv4SubnetMask=%s\n", g_stNetworkCfg.szIPv4SubnetMask);

    return NET_E_SUCCEED;
}

/* 网络配置 Set 回调，对应命令 NET_SET_NETWORKCFG */
static NET_COMMON_ECODE_E MySetNetworkCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_NetworkCfg_S pIn = (pNET_NetworkCfg_S)lpInBuffer;
    g_stNetworkCfg = *pIn;

    printf("[ConfigServerDemo] SetNetworkCfg callback, Channel=%d\n", dwChannelID);
    printf("  New IPv4Address=%s\n", g_stNetworkCfg.szIpv4Address);
    printf("  New IPv4Gateway=%s\n", g_stNetworkCfg.szIPv4GateWay);
    printf("  New IPv4SubnetMask=%s\n", g_stNetworkCfg.szIPv4SubnetMask);

    return NET_E_SUCCEED;
}

/* 系统校时配置 Get 回调，对应命令 NET_GET_NTPCFG */
static NET_COMMON_ECODE_E MyGetNtpCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SystemNtpInfo_S pOut = (pNET_SystemNtpInfo_S)lpOutBuffer;
    *pOut = g_stSystemNtpCfg;

    printf("[ConfigServerDemo] GetNtpCfg callback, Channel=%d\n", dwChannelID);
    printf("  TimeZone=UTC%+d, DateFormat=%d, EnableNtp=%d, ManualSync=%d, DateTime=%s\n",
           g_stSystemNtpCfg.enTimeZone,
           g_stSystemNtpCfg.enDateFormat,
           g_stSystemNtpCfg.bEnableNTPSync,
           g_stSystemNtpCfg.bManualSync,
           g_stSystemNtpCfg.strDateTime);
    printf("  SyncWithComputer=%d, Address=%s, Port=%d, SyncInterval=%d\n",
           g_stSystemNtpCfg.bIsSyncWithComputer,
           g_stSystemNtpCfg.strAddress,
           g_stSystemNtpCfg.nPort,
           g_stSystemNtpCfg.nSyncInterval);

    return NET_E_SUCCEED;
}

/* 系统校时配置 Set 回调，对应命令 NET_SET_NTPCFG */
static NET_COMMON_ECODE_E MySetNtpCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SystemNtpInfo_S pIn = (pNET_SystemNtpInfo_S)lpInBuffer;
    g_stSystemNtpCfg = *pIn;
    g_stSystemNtpCfg.strDateTime[sizeof(g_stSystemNtpCfg.strDateTime) - 1] = '\0';
    g_stSystemNtpCfg.strAddress[sizeof(g_stSystemNtpCfg.strAddress) - 1] = '\0';

    printf("[ConfigServerDemo] SetNtpCfg callback, Channel=%d\n", dwChannelID);
    printf("  New TimeZone=UTC%+d, DateFormat=%d, EnableNtp=%d, ManualSync=%d, DateTime=%s\n",
           g_stSystemNtpCfg.enTimeZone,
           g_stSystemNtpCfg.enDateFormat,
           g_stSystemNtpCfg.bEnableNTPSync,
           g_stSystemNtpCfg.bManualSync,
           g_stSystemNtpCfg.strDateTime);
    printf("  New SyncWithComputer=%d, Address=%s, Port=%d, SyncInterval=%d\n",
           g_stSystemNtpCfg.bIsSyncWithComputer,
           g_stSystemNtpCfg.strAddress,
           g_stSystemNtpCfg.nPort,
           g_stSystemNtpCfg.nSyncInterval);

    return NET_E_SUCCEED;
}


/* WIFI STA配置 Set 回调，对应命令 NET_SET_CONFIG_WIFI_STA */
static NET_COMMON_ECODE_E MySetConfigWifiStaCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stWifiStaCfg = *(pNET_WifiStaCfg_S)lpInBuffer;

    printf("[ConfigServerDemo] SetConfigWifiSta callback, Channel=%d\n", dwChannelID);
    printf("  EnableWifi=%d, EnableBoost=%d\n", g_stWifiStaCfg.bEnableWifi, g_stWifiStaCfg.bEnableBoost);
    return NET_E_SUCCEED;
}

/* WIFI STA连接 Set 回调，对应命令 NET_CONNECT_WIFI_STA */
static NET_COMMON_ECODE_E MyConnectWifiStaCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stWifiStaConnect = *(pNET_WifiStaConnect_S)lpInBuffer;

    printf("[ConfigServerDemo] ConnectWifiSta callback, Channel=%d\n", dwChannelID);
    printf("  Ssid=%s, SecurityMode=%d, Ip=%s, Interface=%s\n",
           g_stWifiStaConnect.szSsid,
           g_stWifiStaConnect.nSecurityMode,
           g_stWifiStaConnect.szIpAddress,
           g_stWifiStaConnect.szInterfaceName);
    return NET_E_SUCCEED;
}

/* WIFI STA断开 Set 回调，对应命令 NET_DISCONNECT_WIFI_STA */
static NET_COMMON_ECODE_E MyDisconnectWifiStaCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stWifiStaConnect = *(pNET_WifiStaConnect_S)lpInBuffer;
    memset(g_stWifiStaConnect.szIpAddress, 0, sizeof(g_stWifiStaConnect.szIpAddress));

    printf("[ConfigServerDemo] DisconnectWifiSta callback, Channel=%d\n", dwChannelID);
    printf("  Ssid=%s disconnected\n", g_stWifiStaConnect.szSsid);
    return NET_E_SUCCEED;
}

/* 4G配置 Get 回调，对应命令 NET_GET_4G_INFO */
static NET_COMMON_ECODE_E MyGet4GInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    *(pNET_4GInfo_S)lpOutBuffer = g_st4GInfo;

    printf("[ConfigServerDemo] Get4GInfo callback, Channel=%d\n", dwChannelID);
    printf("  APN=%s, CallNumber=%s, MTU=%d, AuthMode=%d, NetworkMode=%d, DialMode=%d\n",
           g_st4GInfo.szApn,
           g_st4GInfo.szCallNumber,
           g_st4GInfo.nMtu,
           g_st4GInfo.nAuthMode,
           g_st4GInfo.nNetworkMode,
           g_st4GInfo.nDialMode);
    return NET_E_SUCCEED;
}

/* 4G配置 Set 回调，对应命令 NET_SET_4G_INFO */
static NET_COMMON_ECODE_E MySet4GInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_st4GInfo = *(pNET_4GInfo_S)lpInBuffer;

    printf("[ConfigServerDemo] Set4GInfo callback, Channel=%d\n", dwChannelID);
    printf("  APN=%s, CallNumber=%s, MTU=%d, AuthMode=%d, NetworkMode=%d, DialMode=%d\n",
           g_st4GInfo.szApn,
           g_st4GInfo.szCallNumber,
           g_st4GInfo.nMtu,
           g_st4GInfo.nAuthMode,
           g_st4GInfo.nNetworkMode,
           g_st4GInfo.nDialMode);
    return NET_E_SUCCEED;
}

/* 热点配置 Set 回调，对应命令 NET_SET_HOTSPOT_INFO */
static NET_COMMON_ECODE_E MySetHotspotInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stHotspotInfo = *(pNET_HotspotInfo_S)lpInBuffer;

    printf("[ConfigServerDemo] SetHotspotInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enabled=%d, Ssid=%s, SecurityMode=%s, EncryptionType=%s\n",
           g_stHotspotInfo.bEnabled,
           g_stHotspotInfo.szSsid,
           g_stHotspotInfo.szSecurityMode,
           g_stHotspotInfo.szEncryptionType);
    return NET_E_SUCCEED;
}

/* 热点连接设备 Get 回调，对应命令 NET_GET_HOTSPOT_CONN */
static NET_COMMON_ECODE_E MyGetHotspotConnCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    int i = 0;
    int nCount = 0;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    *(pNET_HotspotConnInfo_S)lpOutBuffer = g_stHotspotConnInfo;

    nCount = g_stHotspotConnInfo.nDeviceCount;
    if (nCount > NET_HOTSPOT_CONN_MAX_NUM)
    {
        nCount = NET_HOTSPOT_CONN_MAX_NUM;
    }

    printf("[ConfigServerDemo] GetHotspotConn callback, Channel=%d\n", dwChannelID);
    printf("  Status=%s, Total=%d, DeviceCount=%d\n",
           g_stHotspotConnInfo.szStatus,
           g_stHotspotConnInfo.nTotal,
           g_stHotspotConnInfo.nDeviceCount);
    for (i = 0; i < nCount; ++i)
    {
        printf("  Device[%d]: Index=%d, Mac=%s, Ip=%s, ConnTime=%s\n",
               i,
               g_stHotspotConnInfo.astDevices[i].nIndex,
               g_stHotspotConnInfo.astDevices[i].szMac,
               g_stHotspotConnInfo.astDevices[i].szIp,
               g_stHotspotConnInfo.astDevices[i].szConnTime);
    }

    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyGetSecurityServicesInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    *(pNET_SecurityServicesInfo_S)lpOutBuffer = g_stSecurityServicesInfo;
    printf("[ConfigServerDemo] GetSecurityServicesInfo callback, Channel=%d, SshEnable=%d, SshPort=%d\n",
           dwChannelID,
           g_stSecurityServicesInfo.stSshAdmin.bSshEnable,
           g_stSecurityServicesInfo.stSshAdmin.nSshPort);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MySetSecurityServicesInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stSecurityServicesInfo = *(pNET_SecurityServicesInfo_S)lpInBuffer;
    strncpy(g_stSshCountdownInfo.szCountdown,
            g_stSecurityServicesInfo.stSshAdmin.szSshCountdown,
            sizeof(g_stSshCountdownInfo.szCountdown) - 1);
    g_stSshCountdownInfo.szCountdown[sizeof(g_stSshCountdownInfo.szCountdown) - 1] = '\0';
    printf("[ConfigServerDemo] SetSecurityServicesInfo callback, Channel=%d, SshEnable=%d, SshPort=%d\n",
           dwChannelID,
           g_stSecurityServicesInfo.stSshAdmin.bSshEnable,
           g_stSecurityServicesInfo.stSshAdmin.nSshPort);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyGetSshCountdownCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    *(pNET_SshCountdownInfo_S)lpOutBuffer = g_stSshCountdownInfo;
    printf("[ConfigServerDemo] GetSshCountdown callback, Channel=%d, Countdown=%s\n",
           dwChannelID, g_stSshCountdownInfo.szCountdown);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyFindLogCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    NET_LogList_S stReq;
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    stReq = *(pNET_LogList_S)lpOutBuffer;
    *(pNET_LogList_S)lpOutBuffer = g_stLogList;
    ((pNET_LogList_S)lpOutBuffer)->stCond = stReq.stCond;
    ((pNET_LogList_S)lpOutBuffer)->stPage.nCurPage = stReq.stPage.nCurPage > 0 ? stReq.stPage.nCurPage : g_stLogList.stPage.nCurPage;
    ((pNET_LogList_S)lpOutBuffer)->stPage.nPageSize = stReq.stPage.nPageSize > 0 ? stReq.stPage.nPageSize : g_stLogList.stPage.nPageSize;

    printf("[ConfigServerDemo] FindLog callback, Channel=%d, Type=%d, Action=%d, Count=%d\n",
           dwChannelID,
           stReq.stCond.nType,
           stReq.stCond.nAction,
           g_stLogList.nLogCount);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyExportLogCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    printf("[ConfigServerDemo] ExportLog callback, Channel=%d\n", dwChannelID);
    return MyFindLogCb(dwChannelID, lpOutBuffer);
}

static NET_COMMON_ECODE_E MyGetLogServerCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    *(pNET_LogServerInfo_S)lpOutBuffer = g_stLogServerInfo;
    printf("[ConfigServerDemo] GetLogServer callback, Channel=%d, Enable=%d, Ssl=%d, Addr=%s, Port=%d\n",
           dwChannelID,
           g_stLogServerInfo.bEnable,
           g_stLogServerInfo.bEnSsl,
           g_stLogServerInfo.szServerAddr,
           g_stLogServerInfo.nPort);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MySetLogServerCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stLogServerInfo = *(pNET_LogServerInfo_S)lpInBuffer;
    printf("[ConfigServerDemo] SetLogServer callback, Channel=%d, Enable=%d, Ssl=%d, Addr=%s, Port=%d\n",
           dwChannelID,
           g_stLogServerInfo.bEnable,
           g_stLogServerInfo.bEnSsl,
           g_stLogServerInfo.szServerAddr,
           g_stLogServerInfo.nPort);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyTestLogServerCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_LogServerInfo_S pInfo = (pNET_LogServerInfo_S)lpInBuffer;
    printf("[ConfigServerDemo] TestLogServer callback, Channel=%d, Addr=%s, Port=%d\n",
           dwChannelID,
           pInfo->szServerAddr,
           pInfo->nPort);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyControlRecordInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stRecordInfo = *(pNET_RecordInfo_S)lpInBuffer;
    g_stRecordStatusInfo.nStatus = g_stRecordInfo.nRecordStatus;
    printf("[ConfigServerDemo] ControlRecordInfo callback, Channel=%d, ChnId=%d, Status=%d, Name=%s\n",
           dwChannelID,
           g_stRecordInfo.nChnId,
           g_stRecordInfo.nRecordStatus,
           g_stRecordInfo.szRecordName);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyGetRecordStatusCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    *(pNET_RecordStatusInfo_S)lpOutBuffer = g_stRecordStatusInfo;
    printf("[ConfigServerDemo] GetRecordStatus callback, Channel=%d, Status=%d\n",
           dwChannelID,
           g_stRecordStatusInfo.nStatus);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyGetRecordScheduleCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    *(pNET_RecordSchedule_S)lpOutBuffer = g_stRecordSchedule;
    printf("[ConfigServerDemo] GetRecordSchedule callback, Channel=%d, Enable=%d, DayCount=%d\n",
           dwChannelID,
           g_stRecordSchedule.bEnable,
           g_stRecordSchedule.nDayScheduleCount);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MySetRecordScheduleCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stRecordSchedule = *(pNET_RecordSchedule_S)lpInBuffer;
    printf("[ConfigServerDemo] SetRecordSchedule callback, Channel=%d, Enable=%d, DayCount=%d\n",
           dwChannelID,
           g_stRecordSchedule.bEnable,
           g_stRecordSchedule.nDayScheduleCount);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyGetRecordAdvancedParamCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    *(pNET_RecordAdvancedParam_S)lpOutBuffer = g_stRecordAdvancedParam;
    printf("[ConfigServerDemo] GetRecordAdvancedParam callback, Channel=%d, LoopWrite=%d, PreTime=%d, DelayTime=%d\n",
           dwChannelID,
           g_stRecordAdvancedParam.bLoopWrite,
           g_stRecordAdvancedParam.nPreTime,
           g_stRecordAdvancedParam.nDelayTime);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MySetRecordAdvancedParamCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stRecordAdvancedParam = *(pNET_RecordAdvancedParam_S)lpInBuffer;
    printf("[ConfigServerDemo] SetRecordAdvancedParam callback, Channel=%d, LoopWrite=%d, PreTime=%d, DelayTime=%d\n",
           dwChannelID,
           g_stRecordAdvancedParam.bLoopWrite,
           g_stRecordAdvancedParam.nPreTime,
           g_stRecordAdvancedParam.nDelayTime);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyFindRecordFileInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    NET_RecordFindCond_S stFind;
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    stFind = ((pNET_RecordFileList_S)lpOutBuffer)->stFind;
    *(pNET_RecordFileList_S)lpOutBuffer = g_stRecordFileList;
    ((pNET_RecordFileList_S)lpOutBuffer)->stFind = stFind;
    printf("[ConfigServerDemo] FindRecordFileInfo callback, Channel=%d, ChnId=%d, Date=%s, Count=%d\n",
           dwChannelID,
           stFind.nChnId,
           stFind.szDate,
           g_stRecordFileList.nResultCount);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyDownloadRecordFileCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    NET_Alarmer_S stAlarmer;
    NET_RecordDownloadProgress_S stProgress;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stRecordDownloadList = *(pNET_RecordDownloadList_S)lpInBuffer;
    if (g_stRecordDownloadList.nProgressCount <= 0)
    {
        g_stRecordDownloadList.nProgressCount = 1;
        strncpy(g_stRecordDownloadList.astProgress[0].szFilename,
                "manual_record_demo.ts",
                sizeof(g_stRecordDownloadList.astProgress[0].szFilename) - 1);
    }
    g_stRecordDownloadList.astProgress[0].nProgress = 35;

    printf("[ConfigServerDemo] DownloadRecordFile callback, Channel=%d, DownloadCount=%d, Path=%s\n",
           dwChannelID,
           g_stRecordDownloadList.nDownloadCount,
           g_stRecordDownloadList.astDownloads[0].szPath);

    memset(&stAlarmer, 0, sizeof(stAlarmer));
    strncpy(stAlarmer.strDeviceIP, "127.0.0.1", sizeof(stAlarmer.strDeviceIP) - 1);
    stProgress = g_stRecordDownloadList.astProgress[0];
    NET_SERVER_PushAlarmInfo(&stAlarmer,
                                NET_NOTICE_DOWNLOAD_RECORD_PROGRESS,
                                &stProgress,
                                sizeof(stProgress));
    return NET_E_SUCCEED;
}

/* OSD能力集配置 Get 回调，对应命令 NET_GET_OSDCAPCFG */
static NET_COMMON_ECODE_E MyGetOSDCapCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }


    pNET_VideoOsdCfg_S pOut = (pNET_VideoOsdCfg_S)lpOutBuffer;
    *pOut = g_stOsdCfg;

    printf("[ConfigServerDemo] GetOSDCapCfg callback, Channel=%d\n", dwChannelID);
    PrintOsdConfigSummary("Return OSD config", &g_stOsdCfg);

    return NET_E_SUCCEED;
}

/* OSD能力集配置 Set 回调，对应命令 NET_SET_OSDCAPCFG */
static NET_COMMON_ECODE_E MySetOSDCapCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_VideoOsdCfg_S pIn = (pNET_VideoOsdCfg_S)lpInBuffer;
    g_stOsdCfg = *pIn;
    NormalizeDemoOsdConfig(&g_stOsdCfg);

    printf("[ConfigServerDemo] SetOSDCapCfg callback, Channel=%d\n", dwChannelID);
    PrintOsdConfigSummary("Saved OSD config", &g_stOsdCfg);

    return NET_E_SUCCEED;
}

/* 移动侦测配置 Get 回调，对应命令 NET_GET_MOTIONALARM */
static NET_COMMON_ECODE_E MyGetMotionAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_MotionAlarmInfo_S pOut = (pNET_MotionAlarmInfo_S)lpOutBuffer;
    *pOut = g_stMotionAlarmInfo;

    printf("[ConfigServerDemo] GetMotionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Mode=%d, Sensitivity=%d\n",
           g_stMotionAlarmInfo.bEnable, g_stMotionAlarmInfo.uMode,
           g_stMotionAlarmInfo.stNormalMode.nSensitivity);

    return NET_E_SUCCEED;
}

/* 移动侦测配置 Set 回调，对应命令 NET_SET_MOTIONALARM */
static NET_COMMON_ECODE_E MySetMotionAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_MotionAlarmInfo_S pIn = (pNET_MotionAlarmInfo_S)lpInBuffer;
    g_stMotionAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetMotionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Mode=%d, Sensitivity=%d\n",
           g_stMotionAlarmInfo.bEnable, g_stMotionAlarmInfo.uMode,
           g_stMotionAlarmInfo.stNormalMode.nSensitivity);

    return NET_E_SUCCEED;
}

/* 隐私遮盖配置 Get 回调，对应命令 NET_GET_PRIVACYMASKCFG */
static NET_COMMON_ECODE_E MyGetPrivacyMaskCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PrivacyMaskCfg_S pOut = (pNET_PrivacyMaskCfg_S)lpOutBuffer;
    *pOut = g_stPrivacyMaskCfg;

    printf("[ConfigServerDemo] GetPrivacyMaskCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, AreaCount=%d\n",
           g_stPrivacyMaskCfg.bEnable, g_stPrivacyMaskCfg.uAreaCount);
    for (int i = 0; i < g_stPrivacyMaskCfg.uAreaCount && i < NET_MAX_PRIVACY_MASK_AREA_NUM; i++)
    {
        const NET_PrivacyMaskArea_S* pArea = &g_stPrivacyMaskCfg.astArea[i];
        printf("  Area[%d]: ID=%d, Enable=%d, Rect=[%d,%d,%d,%d]\n",
               i, pArea->nAreaID, pArea->bEnable,
               pArea->nRectLeft, pArea->nRectTop,
               pArea->nRectRight, pArea->nRectBottom);
    }

    return NET_E_SUCCEED;
}

/* 隐私遮盖配置 Set 回调，对应命令 NET_SET_PRIVACYMASKCFG */
static NET_COMMON_ECODE_E MySetPrivacyMaskCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PrivacyMaskCfg_S pIn = (pNET_PrivacyMaskCfg_S)lpInBuffer;
    g_stPrivacyMaskCfg = *pIn;
    if (g_stPrivacyMaskCfg.uAreaCount < 0)
    {
        g_stPrivacyMaskCfg.uAreaCount = 0;
    }
    if (g_stPrivacyMaskCfg.uAreaCount > NET_MAX_PRIVACY_MASK_AREA_NUM)
    {
        g_stPrivacyMaskCfg.uAreaCount = NET_MAX_PRIVACY_MASK_AREA_NUM;
    }

    printf("[ConfigServerDemo] SetPrivacyMaskCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, AreaCount=%d\n",
           g_stPrivacyMaskCfg.bEnable, g_stPrivacyMaskCfg.uAreaCount);

    return NET_E_SUCCEED;
}

/* 遮挡报警配置 Get 回调，对应命令 NET_GET_TAMPERALARM */
static NET_COMMON_ECODE_E MyGetTamperAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_TamperAlarmInfo_S pOut = (pNET_TamperAlarmInfo_S)lpOutBuffer;
    *pOut = g_stTamperAlarmInfo;

    printf("[ConfigServerDemo] GetTamperAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d, Rect=[%d,%d,%d,%d]\n",
           g_stTamperAlarmInfo.bEnable, g_stTamperAlarmInfo.uSensitivity,
           g_stTamperAlarmInfo.nRectLeft, g_stTamperAlarmInfo.nRectTop,
           g_stTamperAlarmInfo.nRectRight, g_stTamperAlarmInfo.nRectBottom);

    return NET_E_SUCCEED;
}

/* 遮挡报警配置 Set 回调，对应命令 NET_SET_TAMPERALARM */
static NET_COMMON_ECODE_E MySetTamperAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_TamperAlarmInfo_S pIn = (pNET_TamperAlarmInfo_S)lpInBuffer;
    g_stTamperAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetTamperAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stTamperAlarmInfo.bEnable, g_stTamperAlarmInfo.uSensitivity);

    return NET_E_SUCCEED;
}

/* 越界检测配置 Get 回调，对应命令 NET_GET_CROSSLINEALARM */
static NET_COMMON_ECODE_E MyGetCrossLineAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_CrossLineAlarmInfo_S pOut = (pNET_CrossLineAlarmInfo_S)lpOutBuffer;
    *pOut = g_stCrossLineAlarmInfo;

    printf("[ConfigServerDemo] GetCrossLineAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d\n",
           g_stCrossLineAlarmInfo.bEnable, g_stCrossLineAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 越界检测配置 Set 回调，对应命令 NET_SET_CROSSLINEALARM */
static NET_COMMON_ECODE_E MySetCrossLineAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_CrossLineAlarmInfo_S pIn = (pNET_CrossLineAlarmInfo_S)lpInBuffer;
    g_stCrossLineAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetCrossLineAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d\n",
           g_stCrossLineAlarmInfo.bEnable, g_stCrossLineAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 入侵检测配置 Get 回调，对应命令 NET_GET_INTRUSIONALARM */
static NET_COMMON_ECODE_E MyGetIntrusionAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_IntrusionAlarmInfo_S pOut = (pNET_IntrusionAlarmInfo_S)lpOutBuffer;
    *pOut = g_stIntrusionAlarmInfo;

    printf("[ConfigServerDemo] GetIntrusionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d\n",
           g_stIntrusionAlarmInfo.bEnable, g_stIntrusionAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 入侵检测配置 Set 回调，对应命令 NET_SET_INTRUSIONALARM */
static NET_COMMON_ECODE_E MySetIntrusionAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_IntrusionAlarmInfo_S pIn = (pNET_IntrusionAlarmInfo_S)lpInBuffer;
    g_stIntrusionAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetIntrusionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d\n",
           g_stIntrusionAlarmInfo.bEnable, g_stIntrusionAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 进入区域侦测配置 Get 回调，对应命令 NET_GET_ENTERREGIONALARM */
static NET_COMMON_ECODE_E MyGetEnterRegionAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_EnterRegionAlarmInfo_S pOut = (pNET_EnterRegionAlarmInfo_S)lpOutBuffer;
    *pOut = g_stEnterRegionAlarmInfo;

    printf("[ConfigServerDemo] GetEnterRegionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d\n",
           g_stEnterRegionAlarmInfo.bEnable,
           g_stEnterRegionAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 进入区域侦测配置 Set 回调，对应命令 NET_SET_ENTERREGIONALARM */
static NET_COMMON_ECODE_E MySetEnterRegionAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_EnterRegionAlarmInfo_S pIn = (pNET_EnterRegionAlarmInfo_S)lpInBuffer;
    g_stEnterRegionAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetEnterRegionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d\n",
           g_stEnterRegionAlarmInfo.bEnable,
           g_stEnterRegionAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 离开区域侦测配置 Get 回调，对应命令 NET_GET_LEAVEREGIONALARM */
static NET_COMMON_ECODE_E MyGetLeaveRegionAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_LeaveRegionAlarmInfo_S pOut = (pNET_LeaveRegionAlarmInfo_S)lpOutBuffer;
    *pOut = g_stLeaveRegionAlarmInfo;

    printf("[ConfigServerDemo] GetLeaveRegionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d\n",
           g_stLeaveRegionAlarmInfo.bEnable,
           g_stLeaveRegionAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 离开区域侦测配置 Set 回调，对应命令 NET_SET_LEAVEREGIONALARM */
static NET_COMMON_ECODE_E MySetLeaveRegionAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_LeaveRegionAlarmInfo_S pIn = (pNET_LeaveRegionAlarmInfo_S)lpInBuffer;
    g_stLeaveRegionAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetLeaveRegionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d\n",
           g_stLeaveRegionAlarmInfo.bEnable,
           g_stLeaveRegionAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 徘徊侦测配置 Get 回调，对应命令 NET_GET_LOITERINGALARM */
static NET_COMMON_ECODE_E MyGetLoiteringAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_LoiteringAlarmInfo_S pOut = (pNET_LoiteringAlarmInfo_S)lpOutBuffer;
    *pOut = g_stLoiteringAlarmInfo;

    printf("[ConfigServerDemo] GetIntrusionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d\n",
           g_stLoiteringAlarmInfo.bEnable, g_stLoiteringAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 徘徊侦测配置 Set 回调，对应命令 NET_SET_LOITERINGALARM */
static NET_COMMON_ECODE_E MySetLoiteringAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_LoiteringAlarmInfo_S pIn = (pNET_LoiteringAlarmInfo_S)lpInBuffer;
    g_stLoiteringAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetIntrusionAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d\n",
           g_stLoiteringAlarmInfo.bEnable, g_stLoiteringAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}


/* 场景变更侦测配置 Get 回调，对应命令 NET_GET_SCENECHANGEALARM */
static NET_COMMON_ECODE_E MyGetSceneChangeAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SceneChangeAlarmInfo_S pOut = (pNET_SceneChangeAlarmInfo_S)lpOutBuffer;
    *pOut = g_stSceneChangeAlarmInfo;

    printf("[ConfigServerDemo] GetSceneChangeAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stSceneChangeAlarmInfo.bEnable,
           g_stSceneChangeAlarmInfo.nSensitivity);

    return NET_E_SUCCEED;
}

/* 场景变更侦测配置 Set 回调，对应命令 NET_SET_SCENECHANGEALARM */
static NET_COMMON_ECODE_E MySetSceneChangeAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SceneChangeAlarmInfo_S pIn = (pNET_SceneChangeAlarmInfo_S)lpInBuffer;
    g_stSceneChangeAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetSceneChangeAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stSceneChangeAlarmInfo.bEnable,
           g_stSceneChangeAlarmInfo.nSensitivity);

    return NET_E_SUCCEED;
}

/* 人员聚集侦测配置 Get 回调，对应命令 NET_GET_CROWDGATHERINGALARM */
static NET_COMMON_ECODE_E MyGetCrowdGatheringAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_CrowdGatheringAlarmInfo_S pOut = (pNET_CrowdGatheringAlarmInfo_S)lpOutBuffer;
    *pOut = g_stCrowdGatheringAlarmInfo;

    printf("[ConfigServerDemo] GetCrowdGatheringAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d\n",
           g_stCrowdGatheringAlarmInfo.bEnable,
           g_stCrowdGatheringAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 人员聚集侦测配置 Set 回调，对应命令 NET_SET_CROWDGATHERINGALARM */
static NET_COMMON_ECODE_E MySetCrowdGatheringAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_CrowdGatheringAlarmInfo_S pIn = (pNET_CrowdGatheringAlarmInfo_S)lpInBuffer;
    g_stCrowdGatheringAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetCrowdGatheringAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d\n",
           g_stCrowdGatheringAlarmInfo.bEnable,
           g_stCrowdGatheringAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 垃圾暴露配置 Get 回调，对应命令 NET_GET_GARBAGE_EXPOSURE_CFG */
static NET_COMMON_ECODE_E MyGetGarbageExposureCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_GarbageExposureCfg_S pOut = (pNET_GarbageExposureCfg_S)lpOutBuffer;
    *pOut = g_stGarbageExposureCfg;

    printf("[ConfigServerDemo] GetGarbageExposureCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d, PointCount=%d\n",
           g_stGarbageExposureCfg.bEnable,
           g_stGarbageExposureCfg.stRule.nSensitivity,
           g_stGarbageExposureCfg.stRule.uPointCount);

    return NET_E_SUCCEED;
}

/* 垃圾暴露配置 Set 回调，对应命令 NET_SET_GARBAGE_EXPOSURE_CFG */
static NET_COMMON_ECODE_E MySetGarbageExposureCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_GarbageExposureCfg_S pIn = (pNET_GarbageExposureCfg_S)lpInBuffer;
    g_stGarbageExposureCfg = *pIn;

    printf("[ConfigServerDemo] SetGarbageExposureCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d, PointCount=%d\n",
           g_stGarbageExposureCfg.bEnable,
           g_stGarbageExposureCfg.stRule.nSensitivity,
           g_stGarbageExposureCfg.stRule.uPointCount);

    return NET_E_SUCCEED;
}

/* 垃圾满溢配置 Get 回调，对应命令 NET_GET_GARBAGE_OVERFLOW_CFG */
static NET_COMMON_ECODE_E MyGetGarbageOverflowCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_GarbageOverflowCfg_S pOut = (pNET_GarbageOverflowCfg_S)lpOutBuffer;
    *pOut = g_stGarbageOverflowCfg;

    printf("[ConfigServerDemo] GetGarbageOverflowCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d, PointCount=%d\n",
           g_stGarbageOverflowCfg.bEnable,
           g_stGarbageOverflowCfg.stRule.nSensitivity,
           g_stGarbageOverflowCfg.stRule.uPointCount);

    return NET_E_SUCCEED;
}

/* 垃圾满溢配置 Set 回调，对应命令 NET_SET_GARBAGE_OVERFLOW_CFG */
static NET_COMMON_ECODE_E MySetGarbageOverflowCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_GarbageOverflowCfg_S pIn = (pNET_GarbageOverflowCfg_S)lpInBuffer;
    g_stGarbageOverflowCfg = *pIn;

    printf("[ConfigServerDemo] SetGarbageOverflowCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d, PointCount=%d\n",
           g_stGarbageOverflowCfg.bEnable,
           g_stGarbageOverflowCfg.stRule.nSensitivity,
           g_stGarbageOverflowCfg.stRule.uPointCount);

    return NET_E_SUCCEED;
}

/* 人流统计配置 Get 回调，对应命令 NET_GET_PEOPLE_FLOW_STATISTICS_CFG */
static NET_COMMON_ECODE_E MyGetPeopleFlowStatisticsCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PeopleFlowStatisticsCfg_S pOut = (pNET_PeopleFlowStatisticsCfg_S)lpOutBuffer;
    *pOut = g_stPeopleFlowStatisticsCfg;

    printf("[ConfigServerDemo] GetPeopleFlowStatisticsCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d, StatisticsType=%d\n",
           g_stPeopleFlowStatisticsCfg.bEnable,
           g_stPeopleFlowStatisticsCfg.nSensitivity,
           g_stPeopleFlowStatisticsCfg.enStatisticsType);

    return NET_E_SUCCEED;
}

/* 人流统计配置 Set 回调，对应命令 NET_SET_PEOPLE_FLOW_STATISTICS_CFG */
static NET_COMMON_ECODE_E MySetPeopleFlowStatisticsCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PeopleFlowStatisticsCfg_S pIn = (pNET_PeopleFlowStatisticsCfg_S)lpInBuffer;
    g_stPeopleFlowStatisticsCfg = *pIn;

    printf("[ConfigServerDemo] SetPeopleFlowStatisticsCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d, StatisticsType=%d\n",
           g_stPeopleFlowStatisticsCfg.bEnable,
           g_stPeopleFlowStatisticsCfg.nSensitivity,
           g_stPeopleFlowStatisticsCfg.enStatisticsType);

    return NET_E_SUCCEED;
}

/* 人流统计清零回调，对应命令 NET_RESET_PEOPLE_FLOW_STATISTICS */
static NET_COMMON_ECODE_E MyResetPeopleFlowStatisticsCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    printf("[ConfigServerDemo] ResetPeopleFlowStatistics callback, Channel=%d\n", dwChannelID);

    /* 清零操作：将配置中的统计值置零 */
    g_stPeopleFlowStatisticsCfg.nReportInterval = 0;

    return NET_E_SUCCEED;
}

/* 人员密度检测配置 Get 回调，对应命令 NET_GET_PEOPLE_DENSITY_DETECTION_CFG */
static NET_COMMON_ECODE_E MyGetPeopleDensityDetectionCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PeopleDensityDetectionCfg_S pOut = (pNET_PeopleDensityDetectionCfg_S)lpOutBuffer;
    *pOut = g_stPeopleDensityDetectionCfg;

    printf("[ConfigServerDemo] GetPeopleDensityDetectionCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stPeopleDensityDetectionCfg.bEnable,
           g_stPeopleDensityDetectionCfg.nSensitivity);

    return NET_E_SUCCEED;
}

/* 人员密度检测配置 Set 回调，对应命令 NET_SET_PEOPLE_DENSITY_DETECTION_CFG */
static NET_COMMON_ECODE_E MySetPeopleDensityDetectionCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PeopleDensityDetectionCfg_S pIn = (pNET_PeopleDensityDetectionCfg_S)lpInBuffer;
    g_stPeopleDensityDetectionCfg = *pIn;

    printf("[ConfigServerDemo] SetPeopleDensityDetectionCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stPeopleDensityDetectionCfg.bEnable,
           g_stPeopleDensityDetectionCfg.nSensitivity);

    return NET_E_SUCCEED;
}

/* 井盖异常检测配置 Get 回调，对应命令 NET_GET_MANHOLE_COVER_ABNORMAL_CFG */
static NET_COMMON_ECODE_E MyGetManholeCoverAbnormalCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ManholeCoverAbnormalCfg_S pOut = (pNET_ManholeCoverAbnormalCfg_S)lpOutBuffer;
    *pOut = g_stManholeCoverAbnormalCfg;

    printf("[ConfigServerDemo] GetManholeCoverAbnormalCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stManholeCoverAbnormalCfg.bEnable,
           g_stManholeCoverAbnormalCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 井盖异常检测配置 Set 回调，对应命令 NET_SET_MANHOLE_COVER_ABNORMAL_CFG */
static NET_COMMON_ECODE_E MySetManholeCoverAbnormalCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ManholeCoverAbnormalCfg_S pIn = (pNET_ManholeCoverAbnormalCfg_S)lpInBuffer;
    g_stManholeCoverAbnormalCfg = *pIn;

    printf("[ConfigServerDemo] SetManholeCoverAbnormalCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stManholeCoverAbnormalCfg.bEnable,
           g_stManholeCoverAbnormalCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 睡岗识别配置 Get 回调，对应命令 NET_GET_SLEEP_ON_DUTY_CFG */
static NET_COMMON_ECODE_E MyGetSleepOnDutyCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SleepOnDutyCfg_S pOut = (pNET_SleepOnDutyCfg_S)lpOutBuffer;
    *pOut = g_stSleepOnDutyCfg;

    printf("[ConfigServerDemo] GetSleepOnDutyCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stSleepOnDutyCfg.bEnable,
           g_stSleepOnDutyCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 睡岗识别配置 Set 回调，对应命令 NET_SET_SLEEP_ON_DUTY_CFG */
static NET_COMMON_ECODE_E MySetSleepOnDutyCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SleepOnDutyCfg_S pIn = (pNET_SleepOnDutyCfg_S)lpInBuffer;
    g_stSleepOnDutyCfg = *pIn;

    printf("[ConfigServerDemo] SetSleepOnDutyCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stSleepOnDutyCfg.bEnable,
           g_stSleepOnDutyCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 电瓶车进电梯识别配置 Get 回调，对应命令 NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG */
static NET_COMMON_ECODE_E MyGetElectricVehicleInElevatorCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ElectricVehicleInElevatorCfg_S pOut = (pNET_ElectricVehicleInElevatorCfg_S)lpOutBuffer;
    *pOut = g_stElectricVehicleInElevatorCfg;

    printf("[ConfigServerDemo] GetElectricVehicleInElevatorCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stElectricVehicleInElevatorCfg.bEnable,
           g_stElectricVehicleInElevatorCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 电瓶车进电梯识别配置 Set 回调，对应命令 NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG */
static NET_COMMON_ECODE_E MySetElectricVehicleInElevatorCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ElectricVehicleInElevatorCfg_S pIn = (pNET_ElectricVehicleInElevatorCfg_S)lpInBuffer;
    g_stElectricVehicleInElevatorCfg = *pIn;

    printf("[ConfigServerDemo] SetElectricVehicleInElevatorCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stElectricVehicleInElevatorCfg.bEnable,
           g_stElectricVehicleInElevatorCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 人员倒地识别配置 Get 回调，对应命令 NET_GET_PERSON_FALL_DOWN_CFG */
static NET_COMMON_ECODE_E MyGetPersonFallDownCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PersonFallDownCfg_S pOut = (pNET_PersonFallDownCfg_S)lpOutBuffer;
    *pOut = g_stPersonFallDownCfg;

    printf("[ConfigServerDemo] GetPersonFallDownCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stPersonFallDownCfg.bEnable,
           g_stPersonFallDownCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 人员倒地识别配置 Set 回调，对应命令 NET_SET_PERSON_FALL_DOWN_CFG */
static NET_COMMON_ECODE_E MySetPersonFallDownCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PersonFallDownCfg_S pIn = (pNET_PersonFallDownCfg_S)lpInBuffer;
    g_stPersonFallDownCfg = *pIn;

    printf("[ConfigServerDemo] SetPersonFallDownCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stPersonFallDownCfg.bEnable,
           g_stPersonFallDownCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 施工占道识别配置 Get 回调，对应命令 NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG */
static NET_COMMON_ECODE_E MyGetConstructionOccupyRoadCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ConstructionOccupyRoadCfg_S pOut = (pNET_ConstructionOccupyRoadCfg_S)lpOutBuffer;
    *pOut = g_stConstructionOccupyRoadCfg;

    printf("[ConfigServerDemo] GetConstructionOccupyRoadCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stConstructionOccupyRoadCfg.bEnable,
           g_stConstructionOccupyRoadCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 施工占道识别配置 Set 回调，对应命令 NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG */
static NET_COMMON_ECODE_E MySetConstructionOccupyRoadCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ConstructionOccupyRoadCfg_S pIn = (pNET_ConstructionOccupyRoadCfg_S)lpInBuffer;
    g_stConstructionOccupyRoadCfg = *pIn;

    printf("[ConfigServerDemo] SetConstructionOccupyRoadCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stConstructionOccupyRoadCfg.bEnable,
           g_stConstructionOccupyRoadCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 拥堵识别配置 Get 回调，对应命令 NET_GET_CONGESTION_CFG */
static NET_COMMON_ECODE_E MyGetCongestionCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_CongestionCfg_S pOut = (pNET_CongestionCfg_S)lpOutBuffer;
    *pOut = g_stCongestionCfg;

    printf("[ConfigServerDemo] GetCongestionCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stCongestionCfg.bEnable,
           g_stCongestionCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyGetChannelListCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ChannelList_S pOut = (pNET_ChannelList_S)lpOutBuffer;
    memset(pOut, 0, sizeof(*pOut));
    pOut->uSize = sizeof(*pOut);
    pOut->uChannelCount = 1;
    pOut->stChannels[0] = g_stChannelInfo;

    printf("[ConfigServerDemo] GetChannelList callback\n");
    printf("  ChannelCount=%u\n", pOut->uChannelCount);
    printf("  Channel[0].Name=%s\n", pOut->stChannels[0].szChannelName);
    printf("  Channel[0].PreviewMainUrl=%s\n", pOut->stChannels[0].szPreviewMainUrl);
    printf("  Channel[0].PreviewSubUrl=%s\n", pOut->stChannels[0].szPreviewSubUrl);

    return NET_E_SUCCEED;
}

/* 拥堵识别配置 Set 回调，对应命令 NET_SET_CONGESTION_CFG */
static NET_COMMON_ECODE_E MySetCongestionCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_CongestionCfg_S pIn = (pNET_CongestionCfg_S)lpInBuffer;
    g_stCongestionCfg = *pIn;

    printf("[ConfigServerDemo] SetCongestionCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stCongestionCfg.bEnable,
           g_stCongestionCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 车牌识别配置 Get 回调，对应命令 NET_GET_LICENSE_PLATE_RECOGNITION_CFG */
static NET_COMMON_ECODE_E MyGetLicensePlateRecognitionCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_LicensePlateRecognitionCfg_S pOut = (pNET_LicensePlateRecognitionCfg_S)lpOutBuffer;
    *pOut = g_stLicensePlateRecognitionCfg;

    printf("[ConfigServerDemo] GetLicensePlateRecognitionCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stLicensePlateRecognitionCfg.bEnable,
           g_stLicensePlateRecognitionCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 车牌识别配置 Set 回调，对应命令 NET_SET_LICENSE_PLATE_RECOGNITION_CFG */
static NET_COMMON_ECODE_E MySetLicensePlateRecognitionCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_LicensePlateRecognitionCfg_S pIn = (pNET_LicensePlateRecognitionCfg_S)lpInBuffer;
    g_stLicensePlateRecognitionCfg = *pIn;

    printf("[ConfigServerDemo] SetLicensePlateRecognitionCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stLicensePlateRecognitionCfg.bEnable,
           g_stLicensePlateRecognitionCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 高空安全带识别配置 Get 回调，对应命令 NET_GET_HIGH_ALTITUDE_SEATBELT_CFG */
static NET_COMMON_ECODE_E MyGetHighAltitudeSeatbeltCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_HighAltitudeSeatbeltCfg_S pOut = (pNET_HighAltitudeSeatbeltCfg_S)lpOutBuffer;
    *pOut = g_stHighAltitudeSeatbeltCfg;

    printf("[ConfigServerDemo] GetHighAltitudeSeatbeltCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stHighAltitudeSeatbeltCfg.bEnable,
           g_stHighAltitudeSeatbeltCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 高空安全带识别配置 Set 回调，对应命令 NET_SET_HIGH_ALTITUDE_SEATBELT_CFG */
static NET_COMMON_ECODE_E MySetHighAltitudeSeatbeltCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_HighAltitudeSeatbeltCfg_S pIn = (pNET_HighAltitudeSeatbeltCfg_S)lpInBuffer;
    g_stHighAltitudeSeatbeltCfg = *pIn;

    printf("[ConfigServerDemo] SetHighAltitudeSeatbeltCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stHighAltitudeSeatbeltCfg.bEnable,
           g_stHighAltitudeSeatbeltCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 安全帽识别配置 Get 回调，对应命令 NET_GET_SAFETY_HELMET_CFG */
static NET_COMMON_ECODE_E MyGetSafetyHelmetCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SafetyHelmetCfg_S pOut = (pNET_SafetyHelmetCfg_S)lpOutBuffer;
    *pOut = g_stSafetyHelmetCfg;

    printf("[ConfigServerDemo] GetSafetyHelmetCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stSafetyHelmetCfg.bEnable,
           g_stSafetyHelmetCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 安全帽识别配置 Set 回调，对应命令 NET_SET_SAFETY_HELMET_CFG */
static NET_COMMON_ECODE_E MySetSafetyHelmetCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SafetyHelmetCfg_S pIn = (pNET_SafetyHelmetCfg_S)lpInBuffer;
    g_stSafetyHelmetCfg = *pIn;

    printf("[ConfigServerDemo] SetSafetyHelmetCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stSafetyHelmetCfg.bEnable,
           g_stSafetyHelmetCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 摔倒识别配置 Get 回调，对应命令 NET_GET_PERSON_FALL_CFG */
static NET_COMMON_ECODE_E MyGetPersonFallCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PersonFallCfg_S pOut = (pNET_PersonFallCfg_S)lpOutBuffer;
    *pOut = g_stPersonFallCfg;

    printf("[ConfigServerDemo] GetPersonFallCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stPersonFallCfg.bEnable,
           g_stPersonFallCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 摔倒识别配置 Set 回调，对应命令 NET_SET_PERSON_FALL_CFG */
static NET_COMMON_ECODE_E MySetPersonFallCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PersonFallCfg_S pIn = (pNET_PersonFallCfg_S)lpInBuffer;
    g_stPersonFallCfg = *pIn;

    printf("[ConfigServerDemo] SetPersonFallCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stPersonFallCfg.bEnable,
           g_stPersonFallCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 玩手机识别配置 Get 回调，对应命令 NET_GET_PHONE_USAGE_CFG */
static NET_COMMON_ECODE_E MyGetPhoneUsageCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PhoneUsageCfg_S pOut = (pNET_PhoneUsageCfg_S)lpOutBuffer;
    *pOut = g_stPhoneUsageCfg;

    printf("[ConfigServerDemo] GetPhoneUsageCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stPhoneUsageCfg.bEnable,
           g_stPhoneUsageCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 玩手机识别配置 Set 回调，对应命令 NET_SET_PHONE_USAGE_CFG */
static NET_COMMON_ECODE_E MySetPhoneUsageCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PhoneUsageCfg_S pIn = (pNET_PhoneUsageCfg_S)lpInBuffer;
    g_stPhoneUsageCfg = *pIn;

    printf("[ConfigServerDemo] SetPhoneUsageCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stPhoneUsageCfg.bEnable,
           g_stPhoneUsageCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 抽烟识别配置 Get 回调，对应命令 NET_GET_SMOKING_CFG */
static NET_COMMON_ECODE_E MyGetSmokingCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SmokingCfg_S pOut = (pNET_SmokingCfg_S)lpOutBuffer;
    *pOut = g_stSmokingCfg;

    printf("[ConfigServerDemo] GetSmokingCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stSmokingCfg.bEnable,
           g_stSmokingCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 抽烟识别配置 Set 回调，对应命令 NET_SET_SMOKING_CFG */
static NET_COMMON_ECODE_E MySetSmokingCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SmokingCfg_S pIn = (pNET_SmokingCfg_S)lpInBuffer;
    g_stSmokingCfg = *pIn;

    printf("[ConfigServerDemo] SetSmokingCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stSmokingCfg.bEnable,
           g_stSmokingCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 明火识别配置 Get 回调，对应命令 NET_GET_OPEN_FLAME_CFG */
static NET_COMMON_ECODE_E MyGetOpenFlameCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_OpenFlameCfg_S pOut = (pNET_OpenFlameCfg_S)lpOutBuffer;
    *pOut = g_stOpenFlameCfg;

    printf("[ConfigServerDemo] GetOpenFlameCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stOpenFlameCfg.bEnable,
           g_stOpenFlameCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 明火识别配置 Set 回调，对应命令 NET_SET_OPEN_FLAME_CFG */
static NET_COMMON_ECODE_E MySetOpenFlameCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_OpenFlameCfg_S pIn = (pNET_OpenFlameCfg_S)lpInBuffer;
    g_stOpenFlameCfg = *pIn;

    printf("[ConfigServerDemo] SetOpenFlameCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stOpenFlameCfg.bEnable,
           g_stOpenFlameCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 黄土裸露识别配置 Get 回调，对应命令 NET_GET_BARE_SOIL_CFG */
static NET_COMMON_ECODE_E MyGetBareSoilCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_BareSoilCfg_S pOut = (pNET_BareSoilCfg_S)lpOutBuffer;
    *pOut = g_stBareSoilCfg;

    printf("[ConfigServerDemo] GetBareSoilCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stBareSoilCfg.bEnable,
           g_stBareSoilCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 黄土裸露识别配置 Set 回调，对应命令 NET_SET_BARE_SOIL_CFG */
static NET_COMMON_ECODE_E MySetBareSoilCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_BareSoilCfg_S pIn = (pNET_BareSoilCfg_S)lpInBuffer;
    g_stBareSoilCfg = *pIn;

    printf("[ConfigServerDemo] SetBareSoilCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stBareSoilCfg.bEnable,
           g_stBareSoilCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 洞口防护栏识别配置 Get 回调，对应命令 NET_GET_HOLE_PROTECTION_BAR_CFG */
static NET_COMMON_ECODE_E MyGetHoleProtectionBarCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_HoleProtectionBarCfg_S pOut = (pNET_HoleProtectionBarCfg_S)lpOutBuffer;
    *pOut = g_stHoleProtectionBarCfg;

    printf("[ConfigServerDemo] GetHoleProtectionBarCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stHoleProtectionBarCfg.bEnable,
           g_stHoleProtectionBarCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 洞口防护栏识别配置 Set 回调，对应命令 NET_SET_HOLE_PROTECTION_BAR_CFG */
static NET_COMMON_ECODE_E MySetHoleProtectionBarCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_HoleProtectionBarCfg_S pIn = (pNET_HoleProtectionBarCfg_S)lpInBuffer;
    g_stHoleProtectionBarCfg = *pIn;

    printf("[ConfigServerDemo] SetHoleProtectionBarCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stHoleProtectionBarCfg.bEnable,
           g_stHoleProtectionBarCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 反光衣识别配置 Get 回调，对应命令 NET_GET_REFLECTIVE_CLOTHING_CFG */
static NET_COMMON_ECODE_E MyGetReflectiveClothingCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ReflectiveClothingCfg_S pOut = (pNET_ReflectiveClothingCfg_S)lpOutBuffer;
    *pOut = g_stReflectiveClothingCfg;

    printf("[ConfigServerDemo] GetReflectiveClothingCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stReflectiveClothingCfg.bEnable,
           g_stReflectiveClothingCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 反光衣识别配置 Set 回调，对应命令 NET_SET_REFLECTIVE_CLOTHING_CFG */
static NET_COMMON_ECODE_E MySetReflectiveClothingCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ReflectiveClothingCfg_S pIn = (pNET_ReflectiveClothingCfg_S)lpInBuffer;
    g_stReflectiveClothingCfg = *pIn;

    printf("[ConfigServerDemo] SetReflectiveClothingCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stReflectiveClothingCfg.bEnable,
           g_stReflectiveClothingCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 宠物识别配置 Get 回调，对应命令 NET_GET_PET_RECOGNITION_INFO */
static NET_COMMON_ECODE_E MyGetPetRecognitionInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PetRecognitionInfo_S pOut = (pNET_PetRecognitionInfo_S)lpOutBuffer;
    *pOut = g_stPetRecognitionInfo;

    printf("[ConfigServerDemo] GetPetRecognitionInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, DynamicAnalysisEnable=%d, Sensitivity=%d, PointCount=%d\n",
           g_stPetRecognitionInfo.bEnable,
           g_stPetRecognitionInfo.bDynamicAnalysisEnable,
           g_stPetRecognitionInfo.nSensitivity,
           g_stPetRecognitionInfo.stRegion.uPointCount);

    return NET_E_SUCCEED;
}

/* 宠物识别配置 Set 回调，对应命令 NET_SET_PET_RECOGNITION_INFO */
static NET_COMMON_ECODE_E MySetPetRecognitionInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PetRecognitionInfo_S pIn = (pNET_PetRecognitionInfo_S)lpInBuffer;
    g_stPetRecognitionInfo = *pIn;

    printf("[ConfigServerDemo] SetPetRecognitionInfo callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, DynamicAnalysisEnable=%d, Sensitivity=%d, PointCount=%d\n",
           g_stPetRecognitionInfo.bEnable,
           g_stPetRecognitionInfo.bDynamicAnalysisEnable,
           g_stPetRecognitionInfo.nSensitivity,
           g_stPetRecognitionInfo.stRegion.uPointCount);

    return NET_E_SUCCEED;
}

/* 翻越围栏配置 Get 回调，对应命令 NET_GET_CLIMB_FENCE_INFO */
static NET_COMMON_ECODE_E MyGetClimbFenceInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ClimbFenceInfo_S pOut = (pNET_ClimbFenceInfo_S)lpOutBuffer;
    *pOut = g_stClimbFenceInfo;

    printf("[ConfigServerDemo] GetClimbFenceInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stClimbFenceInfo.bEnable,
           g_stClimbFenceInfo.uRuleCount,
           g_stClimbFenceInfo.stRule[0].nSensitivity,
           g_stClimbFenceInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 翻越围栏配置 Set 回调，对应命令 NET_SET_CLIMB_FENCE_INFO */
static NET_COMMON_ECODE_E MySetClimbFenceInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ClimbFenceInfo_S pIn = (pNET_ClimbFenceInfo_S)lpInBuffer;
    g_stClimbFenceInfo = *pIn;

    printf("[ConfigServerDemo] SetClimbFenceInfo callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stClimbFenceInfo.bEnable,
           g_stClimbFenceInfo.uRuleCount,
           g_stClimbFenceInfo.stRule[0].nSensitivity,
           g_stClimbFenceInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 离岗配置 Get 回调，对应命令 NET_GET_DIMISSION_INFO */
static NET_COMMON_ECODE_E MyGetDimissionInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_DimissionInfo_S pOut = (pNET_DimissionInfo_S)lpOutBuffer;
    *pOut = g_stDimissionInfo;

    printf("[ConfigServerDemo] GetDimissionInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stDimissionInfo.bEnable,
           g_stDimissionInfo.uRuleCount,
           g_stDimissionInfo.stRule[0].nSensitivity,
           g_stDimissionInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 离岗配置 Set 回调，对应命令 NET_SET_DIMISSION_INFO */
static NET_COMMON_ECODE_E MySetDimissionInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_DimissionInfo_S pIn = (pNET_DimissionInfo_S)lpInBuffer;
    g_stDimissionInfo = *pIn;

    printf("[ConfigServerDemo] SetDimissionInfo callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stDimissionInfo.bEnable,
           g_stDimissionInfo.uRuleCount,
           g_stDimissionInfo.stRule[0].nSensitivity,
           g_stDimissionInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 违规变道配置 Get 回调，对应命令 NET_GET_ILLEGAL_LANE_INFO */
static NET_COMMON_ECODE_E MyGetIllegalLaneInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_IllegalLaneInfo_S pOut = (pNET_IllegalLaneInfo_S)lpOutBuffer;
    *pOut = g_stIllegalLaneInfo;

    printf("[ConfigServerDemo] GetIllegalLaneInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d, FirstDirection=%d, FirstSensitivity=%d\n",
           g_stIllegalLaneInfo.bEnable,
           g_stIllegalLaneInfo.uRuleCount,
           g_stIllegalLaneInfo.stRule[0].enCrossDirection,
           g_stIllegalLaneInfo.stRule[0].nSensitivity);

    return NET_E_SUCCEED;
}

/* 违规变道配置 Set 回调，对应命令 NET_SET_ILLEGAL_LANE_INFO */
static NET_COMMON_ECODE_E MySetIllegalLaneInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_IllegalLaneInfo_S pIn = (pNET_IllegalLaneInfo_S)lpInBuffer;
    g_stIllegalLaneInfo = *pIn;

    printf("[ConfigServerDemo] SetIllegalLaneInfo callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d, FirstDirection=%d, FirstSensitivity=%d\n",
           g_stIllegalLaneInfo.bEnable,
           g_stIllegalLaneInfo.uRuleCount,
           g_stIllegalLaneInfo.stRule[0].enCrossDirection,
           g_stIllegalLaneInfo.stRule[0].nSensitivity);

    return NET_E_SUCCEED;
}

/* 逆行配置 Get 回调，对应命令 NET_GET_RETROGRADE_INFO */
static NET_COMMON_ECODE_E MyGetRetrogradeInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_RetrogradeInfo_S pOut = (pNET_RetrogradeInfo_S)lpOutBuffer;
    *pOut = g_stRetrogradeInfo;

    printf("[ConfigServerDemo] GetRetrogradeInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d, FirstDirection=%d, FirstSensitivity=%d\n",
           g_stRetrogradeInfo.bEnable,
           g_stRetrogradeInfo.uRuleCount,
           g_stRetrogradeInfo.stRule[0].enCrossDirection,
           g_stRetrogradeInfo.stRule[0].nSensitivity);

    return NET_E_SUCCEED;
}

/* 逆行配置 Set 回调，对应命令 NET_SET_RETROGRADE_INFO */
static NET_COMMON_ECODE_E MySetRetrogradeInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_RetrogradeInfo_S pIn = (pNET_RetrogradeInfo_S)lpInBuffer;
    g_stRetrogradeInfo = *pIn;

    printf("[ConfigServerDemo] SetRetrogradeInfo callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d, FirstDirection=%d, FirstSensitivity=%d\n",
           g_stRetrogradeInfo.bEnable,
           g_stRetrogradeInfo.uRuleCount,
           g_stRetrogradeInfo.stRule[0].enCrossDirection,
           g_stRetrogradeInfo.stRule[0].nSensitivity);

    return NET_E_SUCCEED;
}

/* 非机动车闯入配置 Get 回调，对应命令 NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO */
static NET_COMMON_ECODE_E MyGetNonmotorVehicleIntrusionInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_NonmotorVehicleIntrusionInfo_S pOut = (pNET_NonmotorVehicleIntrusionInfo_S)lpOutBuffer;
    *pOut = g_stNonmotorVehicleIntrusionInfo;

    printf("[ConfigServerDemo] GetNonmotorVehicleIntrusionInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stNonmotorVehicleIntrusionInfo.bEnable,
           g_stNonmotorVehicleIntrusionInfo.uRuleCount,
           g_stNonmotorVehicleIntrusionInfo.stRule[0].nSensitivity,
           g_stNonmotorVehicleIntrusionInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 非机动车闯入配置 Set 回调，对应命令 NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO */
static NET_COMMON_ECODE_E MySetNonmotorVehicleIntrusionInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_NonmotorVehicleIntrusionInfo_S pIn = (pNET_NonmotorVehicleIntrusionInfo_S)lpInBuffer;
    g_stNonmotorVehicleIntrusionInfo = *pIn;

    printf("[ConfigServerDemo] SetNonmotorVehicleIntrusionInfo callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stNonmotorVehicleIntrusionInfo.bEnable,
           g_stNonmotorVehicleIntrusionInfo.uRuleCount,
           g_stNonmotorVehicleIntrusionInfo.stRule[0].nSensitivity,
           g_stNonmotorVehicleIntrusionInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 应急车道占用识别配置 Get 回调，对应命令 NET_GET_OCCUPATION_EMERGENCY_INFO */
static NET_COMMON_ECODE_E MyGetOccupationEmergencyInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_OccupationEmergencyInfo_S pOut = (pNET_OccupationEmergencyInfo_S)lpOutBuffer;
    *pOut = g_stOccupationEmergencyInfo;

    printf("[ConfigServerDemo] GetOccupationEmergencyInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stOccupationEmergencyInfo.bEnable,
           g_stOccupationEmergencyInfo.uRuleCount,
           g_stOccupationEmergencyInfo.stRule[0].nSensitivity,
           g_stOccupationEmergencyInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 应急车道占用识别配置 Set 回调，对应命令 NET_SET_OCCUPATION_EMERGENCY_INFO */
static NET_COMMON_ECODE_E MySetOccupationEmergencyInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_OccupationEmergencyInfo_S pIn = (pNET_OccupationEmergencyInfo_S)lpInBuffer;
    g_stOccupationEmergencyInfo = *pIn;

    printf("[ConfigServerDemo] SetOccupationEmergencyInfo callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stOccupationEmergencyInfo.bEnable,
           g_stOccupationEmergencyInfo.uRuleCount,
           g_stOccupationEmergencyInfo.stRule[0].nSensitivity,
           g_stOccupationEmergencyInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 行人闯入配置 Get 回调，对应命令 NET_GET_PEDESTRIAN_INTRUSION_INFO */
static NET_COMMON_ECODE_E MyGetPedestrianIntrusionInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PedestrianIntrusionInfo_S pOut = (pNET_PedestrianIntrusionInfo_S)lpOutBuffer;
    *pOut = g_stPedestrianIntrusionInfo;

    printf("[ConfigServerDemo] GetPedestrianIntrusionInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stPedestrianIntrusionInfo.bEnable,
           g_stPedestrianIntrusionInfo.uRuleCount,
           g_stPedestrianIntrusionInfo.stRule[0].nSensitivity,
           g_stPedestrianIntrusionInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 行人闯入配置 Set 回调，对应命令 NET_SET_PEDESTRIAN_INTRUSION_INFO */
static NET_COMMON_ECODE_E MySetPedestrianIntrusionInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PedestrianIntrusionInfo_S pIn = (pNET_PedestrianIntrusionInfo_S)lpInBuffer;
    g_stPedestrianIntrusionInfo = *pIn;

    printf("[ConfigServerDemo] SetPedestrianIntrusionInfo callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d, FirstSensitivity=%d, FirstTimeThreshold=%d\n",
           g_stPedestrianIntrusionInfo.bEnable,
           g_stPedestrianIntrusionInfo.uRuleCount,
           g_stPedestrianIntrusionInfo.stRule[0].nSensitivity,
           g_stPedestrianIntrusionInfo.stRule[0].nTimeThreshold);

    return NET_E_SUCCEED;
}

/* 烟火识别配置 Get 回调，对应命令 NET_GET_SMOKE_FIRE_CFG */
static NET_COMMON_ECODE_E MyGetSmokeFireCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SmokeFireCfg_S pOut = (pNET_SmokeFireCfg_S)lpOutBuffer;
    *pOut = g_stSmokeFireCfg;

    printf("[ConfigServerDemo] GetSmokeFireCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stSmokeFireCfg.bEnable,
           g_stSmokeFireCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 烟火识别配置 Set 回调，对应命令 NET_SET_SMOKE_FIRE_CFG */
static NET_COMMON_ECODE_E MySetSmokeFireCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_SmokeFireCfg_S pIn = (pNET_SmokeFireCfg_S)lpInBuffer;
    g_stSmokeFireCfg = *pIn;

    printf("[ConfigServerDemo] SetSmokeFireCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stSmokeFireCfg.bEnable,
           g_stSmokeFireCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 道路积水检测配置 Get 回调，对应命令 NET_GET_ROAD_PONDING_CFG */
static NET_COMMON_ECODE_E MyGetRoadPondingCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_RoadPondingCfg_S pOut = (pNET_RoadPondingCfg_S)lpOutBuffer;
    *pOut = g_stRoadPondingCfg;

    printf("[ConfigServerDemo] GetRoadPondingCfg callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d\n",
           g_stRoadPondingCfg.bEnable,
           g_stRoadPondingCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 道路积水检测配置 Set 回调，对应命令 NET_SET_ROAD_PONDING_CFG */
static NET_COMMON_ECODE_E MySetRoadPondingCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_RoadPondingCfg_S pIn = (pNET_RoadPondingCfg_S)lpInBuffer;
    g_stRoadPondingCfg = *pIn;

    printf("[ConfigServerDemo] SetRoadPondingCfg callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d\n",
           g_stRoadPondingCfg.bEnable,
           g_stRoadPondingCfg.stRule.nSensitivity);

    return NET_E_SUCCEED;
}

/* 停车侦测配置 Get 回调，对应命令 NET_GET_PARKINGALARM */
static NET_COMMON_ECODE_E MyGetParkingAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ParkingAlarmInfo_S pOut = (pNET_ParkingAlarmInfo_S)lpOutBuffer;
    *pOut = g_stParkingAlarmInfo;

    printf("[ConfigServerDemo] GetParkingAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d\n",
           g_stParkingAlarmInfo.bEnable,
           g_stParkingAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 停车侦测配置 Set 回调，对应命令 NET_SET_PARKINGALARM */
static NET_COMMON_ECODE_E MySetParkingAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ParkingAlarmInfo_S pIn = (pNET_ParkingAlarmInfo_S)lpInBuffer;
    g_stParkingAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetParkingAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d\n",
           g_stParkingAlarmInfo.bEnable,
           g_stParkingAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 物品遗留侦测配置 Get 回调，对应命令 NET_GET_UNATTENDEDOBJECTALARM */
static NET_COMMON_ECODE_E MyGetUnattendedObjectAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_UnattendedObjectAlarmInfo_S pOut = (pNET_UnattendedObjectAlarmInfo_S)lpOutBuffer;
    *pOut = g_stUnattendedObjectAlarmInfo;

    printf("[ConfigServerDemo] GetUnattendedObjectAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d\n",
           g_stUnattendedObjectAlarmInfo.bEnable,
           g_stUnattendedObjectAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 物品遗留侦测配置 Set 回调，对应命令 NET_SET_UNATTENDEDOBJECTALARM */
static NET_COMMON_ECODE_E MySetUnattendedObjectAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_UnattendedObjectAlarmInfo_S pIn = (pNET_UnattendedObjectAlarmInfo_S)lpInBuffer;
    g_stUnattendedObjectAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetUnattendedObjectAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d\n",
           g_stUnattendedObjectAlarmInfo.bEnable,
           g_stUnattendedObjectAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 物品拿取侦测配置 Get 回调，对应命令 NET_GET_OBJECTREMOVALALARM */
static NET_COMMON_ECODE_E MyGetObjectRemovalAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ObjectRemovalAlarmInfo_S pOut = (pNET_ObjectRemovalAlarmInfo_S)lpOutBuffer;
    *pOut = g_stObjectRemovalAlarmInfo;

    printf("[ConfigServerDemo] GetObjectRemovalAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, RuleCount=%d\n",
           g_stObjectRemovalAlarmInfo.bEnable,
           g_stObjectRemovalAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 物品拿取侦测配置 Set 回调，对应命令 NET_SET_OBJECTREMOVALALARM */
static NET_COMMON_ECODE_E MySetObjectRemovalAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ObjectRemovalAlarmInfo_S pIn = (pNET_ObjectRemovalAlarmInfo_S)lpInBuffer;
    g_stObjectRemovalAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetObjectRemovalAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, RuleCount=%d\n",
           g_stObjectRemovalAlarmInfo.bEnable,
           g_stObjectRemovalAlarmInfo.uRuleCount);

    return NET_E_SUCCEED;
}

/* 音频异常侦测配置 Get 回调，对应命令 NET_GET_AUDIOANOMALYALARM */
static NET_COMMON_ECODE_E MyGetAudioAnomalyAlarmCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_AudioAnomalyAlarmInfo_S pOut = (pNET_AudioAnomalyAlarmInfo_S)lpOutBuffer;
    *pOut = g_stAudioAnomalyAlarmInfo;

    printf("[ConfigServerDemo] GetAudioAnomalyAlarm callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, AudioInputAnomaly=%d, UpEnable=%d, UpSensitivity=%d, UpThreshold=%d, DownEnable=%d, DownSensitivity=%d\n",
           g_stAudioAnomalyAlarmInfo.bEnable,
           g_stAudioAnomalyAlarmInfo.bAudioInputAnomaly,
           g_stAudioAnomalyAlarmInfo.bUpEnable,
           g_stAudioAnomalyAlarmInfo.nUpSensitivity,
           g_stAudioAnomalyAlarmInfo.nUpThreshold,
           g_stAudioAnomalyAlarmInfo.bDownEnable,
           g_stAudioAnomalyAlarmInfo.nDownSensitivity);

    return NET_E_SUCCEED;
}

/* 音频异常侦测配置 Set 回调，对应命令 NET_SET_AUDIOANOMALYALARM */
static NET_COMMON_ECODE_E MySetAudioAnomalyAlarmCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_AudioAnomalyAlarmInfo_S pIn = (pNET_AudioAnomalyAlarmInfo_S)lpInBuffer;
    g_stAudioAnomalyAlarmInfo = *pIn;

    printf("[ConfigServerDemo] SetAudioAnomalyAlarm callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, AudioInputAnomaly=%d, UpEnable=%d, UpSensitivity=%d, UpThreshold=%d, DownEnable=%d, DownSensitivity=%d\n",
           g_stAudioAnomalyAlarmInfo.bEnable,
           g_stAudioAnomalyAlarmInfo.bAudioInputAnomaly,
           g_stAudioAnomalyAlarmInfo.bUpEnable,
           g_stAudioAnomalyAlarmInfo.nUpSensitivity,
           g_stAudioAnomalyAlarmInfo.nUpThreshold,
           g_stAudioAnomalyAlarmInfo.bDownEnable,
           g_stAudioAnomalyAlarmInfo.nDownSensitivity);

    return NET_E_SUCCEED;
}

/**
 * @brief 获取内存中的声音报警配置。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [out] pOutBuffer 指向 NET_AudibleAlarmInfo_S 的输出缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED；参数为空返回 NET_TV_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoGetAudibleAlarmInfo(INT32 nChannelId, LPVOID pOutBuffer)
{
    pNET_AudibleAlarmInfo_S pAlarmInfo = NULL;

    if (!pOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmInfo = (pNET_AudibleAlarmInfo_S)pOutBuffer;
    *pAlarmInfo = gs_stAudibleAlarmInfo;
    printf("[ConfigServerDemo] GetAudibleAlarmInfo callback, Channel=%d\n", nChannelId);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置内存中的声音报警配置。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [in] pInBuffer 指向 NET_AudibleAlarmInfo_S 的输入缓冲区。
 * @return 成功返回 NET_E_SUCCEED；参数为空返回 NET_TV_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoSetAudibleAlarmInfo(INT32 nChannelId, LPVOID pInBuffer)
{
    pNET_AudibleAlarmInfo_S pAlarmInfo = NULL;

    if (!pInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmInfo = (pNET_AudibleAlarmInfo_S)pInBuffer;
    gs_stAudibleAlarmInfo = *pAlarmInfo;
    printf("[ConfigServerDemo] SetAudibleAlarmInfo callback, Channel=%d\n", nChannelId);
    return NET_E_SUCCEED;
}

/**
 * @brief 获取内存中的报警输入配置集合。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [out] pOutBuffer 指向 NET_AlarmInputInfoList_S 的输出缓冲区。
 * @return 成功返回 NET_E_SUCCEED；参数为空返回 NET_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoGetAlarmInputInfo(INT32 nChannelId, LPVOID pOutBuffer)
{
    pNET_AlarmInputInfoList_S pAlarmInputList = NULL;

    if (!pOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmInputList = (pNET_AlarmInputInfoList_S)pOutBuffer;
    *pAlarmInputList = gs_stAlarmInputInfoList;
    printf("[ConfigServerDemo] GetAlarmInputInfo callback, Channel=%d, Count=%d\n",
           nChannelId,
           gs_stAlarmInputInfoList.nAlarmInputCount);
    return NET_E_SUCCEED;
}

/**
 * @brief 按报警输入通道号更新或追加配置。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [in] pInBuffer 指向 NET_AlarmInputInfo_S 的输入缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED；参数或通道数量异常返回 NET_TV_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoSetAlarmInputInfo(INT32 nChannelId, LPVOID pInBuffer)
{
    pNET_AlarmInputInfo_S pAlarmInput = NULL;
    INT32 nIndex = 0;
    INT32 nInputCount = 0;

    if (!pInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmInput = (pNET_AlarmInputInfo_S)pInBuffer;
    nInputCount = gs_stAlarmInputInfoList.nAlarmInputCount;
    if (nInputCount < 0 || nInputCount > NET_MAX_ALARM_IN_NUM)
    {
        return NET_E_INVALID_PARAM;
    }

    for (nIndex = 0; nIndex < nInputCount; ++nIndex)
    {
        if (gs_stAlarmInputInfoList.astAlarmInputs[nIndex].nAlarmNumber == pAlarmInput->nAlarmNumber)
        {
            gs_stAlarmInputInfoList.astAlarmInputs[nIndex] = *pAlarmInput;
            printf("[ConfigServerDemo] SetAlarmInputInfo updated, Channel=%d, AlarmNumber=%d\n",
                   nChannelId,
                   pAlarmInput->nAlarmNumber);
            return NET_E_SUCCEED;
        }
    }

    if (nInputCount >= NET_MAX_ALARM_IN_NUM)
    {
        return NET_E_INVALID_PARAM;
    }

    gs_stAlarmInputInfoList.astAlarmInputs[nInputCount] = *pAlarmInput;
    gs_stAlarmInputInfoList.nAlarmInputCount = nInputCount + 1;
    printf("[ConfigServerDemo] SetAlarmInputInfo appended, Channel=%d, AlarmNumber=%d\n",
           nChannelId,
           pAlarmInput->nAlarmNumber);
    return NET_E_SUCCEED;
}

/**
 * @brief 获取内存中的报警输出配置集合。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [out] pOutBuffer 指向 NET_AlarmOutputInfoList_S 的输出缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED；参数为空返回 NET_TV_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoGetAlarmOutputInfo(INT32 nChannelId, LPVOID pOutBuffer)
{
    pNET_AlarmOutputInfoList_S pAlarmOutputList = NULL;

    if (!pOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmOutputList = (pNET_AlarmOutputInfoList_S)pOutBuffer;
    *pAlarmOutputList = gs_stAlarmOutputInfoList;
    printf("[ConfigServerDemo] GetAlarmOutputInfo callback, Channel=%d, Count=%d\n",
           nChannelId,
           gs_stAlarmOutputInfoList.nAlarmOutputCount);
    return NET_E_SUCCEED;
}

/**
 * @brief 按报警输出通道号更新或追加配置。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [in] pInBuffer 指向 NET_AlarmOutputInfo_S 的输入缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED；参数或通道数量异常返回 NET_TV_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoSetAlarmOutputInfo(INT32 nChannelId, LPVOID pInBuffer)
{
    pNET_AlarmOutputInfo_S pAlarmOutput = NULL;
    INT32 nIndex = 0;
    INT32 nOutputCount = 0;

    if (!pInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmOutput = (pNET_AlarmOutputInfo_S)pInBuffer;
    nOutputCount = gs_stAlarmOutputInfoList.nAlarmOutputCount;
    if (nOutputCount < 0 || nOutputCount > NET_MAX_ALARM_OUT_NUM)
    {
        return NET_E_INVALID_PARAM;
    }

    for (nIndex = 0; nIndex < nOutputCount; ++nIndex)
    {
        if (gs_stAlarmOutputInfoList.astAlarmOutputs[nIndex].nAlarmNumber == pAlarmOutput->nAlarmNumber)
        {
            gs_stAlarmOutputInfoList.astAlarmOutputs[nIndex] = *pAlarmOutput;
            printf("[ConfigServerDemo] SetAlarmOutputInfo updated, Channel=%d, AlarmNumber=%d\n",
                   nChannelId,
                   pAlarmOutput->nAlarmNumber);
            return NET_E_SUCCEED;
        }
    }

    if (nOutputCount >= NET_MAX_ALARM_OUT_NUM)
    {
        return NET_E_INVALID_PARAM;
    }

    gs_stAlarmOutputInfoList.astAlarmOutputs[nOutputCount] = *pAlarmOutput;
    gs_stAlarmOutputInfoList.nAlarmOutputCount = nOutputCount + 1;
    printf("[ConfigServerDemo] SetAlarmOutputInfo appended, Channel=%d, AlarmNumber=%d\n",
           nChannelId,
           pAlarmOutput->nAlarmNumber);
    return NET_E_SUCCEED;
}

/**
 * @brief 获取内存中的闪光报警灯配置。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [out] pOutBuffer 指向 NET_FlashingLightAlarmInfo_S 的输出缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED；参数为空返回 NET_TV_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoGetFlashingLightAlarmInfo(INT32 nChannelId, LPVOID pOutBuffer)
{
    pNET_FlashingLightAlarmInfo_S pAlarmInfo = NULL;

    if (!pOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmInfo = (pNET_FlashingLightAlarmInfo_S)pOutBuffer;
    *pAlarmInfo = gs_stFlashingLightAlarmInfo;
    printf("[ConfigServerDemo] GetFlashingLightAlarmInfo callback, Channel=%d\n", nChannelId);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置内存中的闪光报警灯配置。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [in] pInBuffer 指向 NET_FlashingLightAlarmInfo_S 的输入缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED；参数为空返回 NET_TV_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoSetFlashingLightAlarmInfo(INT32 nChannelId, LPVOID pInBuffer)
{
    pNET_FlashingLightAlarmInfo_S pAlarmInfo = NULL;

    if (!pInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmInfo = (pNET_FlashingLightAlarmInfo_S)pInBuffer;
    gs_stFlashingLightAlarmInfo = *pAlarmInfo;
    printf("[ConfigServerDemo] SetFlashingLightAlarmInfo callback, Channel=%d\n", nChannelId);
    return NET_E_SUCCEED;
}

/**
 * @brief 获取内存中的 PIR 报警配置。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [out] pOutBuffer 指向 NET_PirAlarmInfo_S 的输出缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED；参数为空返回 NET_TV_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoGetPirAlarmInfo(INT32 nChannelId, LPVOID pOutBuffer)
{
    pNET_PirAlarmInfo_S pAlarmInfo = NULL;

    if (!pOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmInfo = (pNET_PirAlarmInfo_S)pOutBuffer;
    *pAlarmInfo = gs_stPirAlarmInfo;
    printf("[ConfigServerDemo] GetPirAlarmInfo callback, Channel=%d\n", nChannelId);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置内存中的 PIR 报警配置。
 * @author ITC
 * @param [in] nChannelId 请求通道标识，本示例不区分通道。
 * @param [in] pInBuffer 指向 NET_PirAlarmInfo_S 的输入缓冲区。
 * @return 成功返回 NET_TV_E_SUCCEED；参数为空返回 NET_TV_E_INVALID_PARAM。
 */
static NET_COMMON_ECODE_E ConfigDemoSetPirAlarmInfo(INT32 nChannelId, LPVOID pInBuffer)
{
    pNET_PirAlarmInfo_S pAlarmInfo = NULL;

    if (!pInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pAlarmInfo = (pNET_PirAlarmInfo_S)pInBuffer;
    gs_stPirAlarmInfo = *pAlarmInfo;
    printf("[ConfigServerDemo] SetPirAlarmInfo callback, Channel=%d\n", nChannelId);
    return NET_E_SUCCEED;
}

/* 图像配置 Get 回调，对应命令 NET_GET_IMAGECFG */
static NET_COMMON_ECODE_E MyGetImageCfgCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ImageSetting_S pOut = (pNET_ImageSetting_S)lpOutBuffer;
    *pOut = g_stImageCfg;

    printf("[ConfigServerDemo] GetImageCfg callback, Channel=%d\n", dwChannelID);
    printf("  Brightness=%u, Contrast=%u, Saturation=%u, Sharpness=%u\n",
           g_stImageCfg.nBrightness,
           g_stImageCfg.nContrast,
           g_stImageCfg.nSaturation,
           g_stImageCfg.nSharpness);

    return NET_E_SUCCEED;
}

/* 图像配置 Set 回调，对应命令 NET_SET_IMAGECFG */
static NET_COMMON_ECODE_E MySetImageCfgCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ImageSetting_S pIn = (pNET_ImageSetting_S)lpInBuffer;
    g_stImageCfg = *pIn;

    printf("[ConfigServerDemo] SetImageCfg callback, Channel=%d\n", dwChannelID);
    printf("  Brightness=%u, Contrast=%u, Saturation=%u, Sharpness=%u\n",
           g_stImageCfg.nBrightness,
           g_stImageCfg.nContrast,
           g_stImageCfg.nSaturation,
           g_stImageCfg.nSharpness);

    return NET_E_SUCCEED;
}

/* 预览信息配置 Get 回调，对应命令 NET_GET_PREVIEW_INFO */
static NET_COMMON_ECODE_E MyGetPreviewInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PreviewInfo_S pOut = (pNET_PreviewInfo_S)lpOutBuffer;
    *pOut = g_stPreviewInfo;

    printf("[ConfigServerDemo] GetPreviewInfo callback, Channel=%d\n", dwChannelID);
    printf("  MainUrl=%s\n", g_stPreviewInfo.stRtspUrl.szRtspMainUrl);
    printf("  SubUrl=%s\n", g_stPreviewInfo.stRtspUrl.szRtspSubUrl);
    printf("  Brightness=%d, Contrast=%d, Saturation=%d, Sharpness=%d\n",
           g_stPreviewInfo.stImageParam.nBrightness,
           g_stPreviewInfo.stImageParam.nContrast,
           g_stPreviewInfo.stImageParam.nSaturation,
           g_stPreviewInfo.stImageParam.nSharpness);

    return NET_E_SUCCEED;
}

/* 预览信息配置 Set 回调，对应命令 NET_SET_PREVIEW_INFO */
static NET_COMMON_ECODE_E MySetPreviewInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_PreviewInfo_S pIn = (pNET_PreviewInfo_S)lpInBuffer;
    g_stPreviewInfo = *pIn;

    printf("[ConfigServerDemo] SetPreviewInfo callback, Channel=%d\n", dwChannelID);
    printf("  MainUrl=%s\n", g_stPreviewInfo.stRtspUrl.szRtspMainUrl);
    printf("  SubUrl=%s\n", g_stPreviewInfo.stRtspUrl.szRtspSubUrl);
    printf("  Brightness=%d, Contrast=%d, Saturation=%d, Sharpness=%d\n",
           g_stPreviewInfo.stImageParam.nBrightness,
           g_stPreviewInfo.stImageParam.nContrast,
           g_stPreviewInfo.stImageParam.nSaturation,
           g_stPreviewInfo.stImageParam.nSharpness);

    return NET_E_SUCCEED;
}

/* 通道信息 Get 回调，对应命令 NET_GET_CHANNEL_INFO */
static NET_COMMON_ECODE_E MyGetChannelInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_ChannelInfo_S pOut = (pNET_ChannelInfo_S)lpOutBuffer;
    *pOut = g_stChannelInfo;
    pOut->uChannel = (UINT32)dwChannelID;

    printf("[ConfigServerDemo] GetChannelInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%u, Online=%u, Name=%s\n",
           pOut->byEnable,
           pOut->byOnline,
           pOut->szChannelName);
    printf("  MainUrl=%s\n", pOut->szRtspMainUrl);
    printf("  SubUrl=%s\n", pOut->szRtspSubUrl);
    printf("  PreviewMainUrl=%s\n", pOut->szPreviewMainUrl);
    printf("  PreviewSubUrl=%s\n", pOut->szPreviewSubUrl);

    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MySetTalkbackStateCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stTalkbackStateInfo = *(pNET_TalkbackStateInfo_S)lpInBuffer;
    printf("[ConfigServerDemo] SetTalkbackState callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Url=%s, LocalIp=%s\n",
           g_stTalkbackStateInfo.bEnable,
           g_stTalkbackStateInfo.szUrl,
           g_stTalkbackStateInfo.szLocalIP);
    return NET_E_SUCCEED;
}

static void STDCALL MyVoiceComPlayCb(const char* data, unsigned int size)
{
    if (!data || size == 0)
    {
        printf("[ConfigServerDemo][VoiceCom] invalid audio frame\n");
        return;
    }

    ++g_voiceComFrameCount;
    g_voiceComBytes += size;

    if (g_voiceComDumpFp)
    {
        fwrite(data, 1, size, g_voiceComDumpFp);
        fflush(g_voiceComDumpFp);
    }

    if (g_voiceComFrameCount <= 5 || (g_voiceComFrameCount % 50) == 0)
    {
        printf("[ConfigServerDemo][VoiceCom] recv frame=%llu size=%u totalBytes=%llu\n",
               g_voiceComFrameCount,
               size,
               g_voiceComBytes);
    }

}

static INT32 STDCALL MyVoiceComCaptureCb(const NET_VoiceComAudioParam_S* pstAudioParam,
                                         CHAR* pBuffer,
                                         UINT32 dwBufferSize,
                                         LPVOID lpUserData)
{
    (void)lpUserData;

    if (!pstAudioParam || !pBuffer || dwBufferSize == 0 ||
        pstAudioParam->uFrameBytes <= 0 ||
        pstAudioParam->uFrameBytes > (INT32)dwBufferSize)
    {
        return 0;
    }

    /*
     * 企业规范：Demo仅返回静音帧用于验证VoiceCom双向链路。
     * 真实设备应在此回调中读取MIC/LineIn采集数据，并保证音频格式与pstAudioParam一致。
     */
    memset(pBuffer, VoiceComSilenceByte(pstAudioParam->enFormat), (size_t)pstAudioParam->uFrameBytes);
    return pstAudioParam->uFrameBytes;
}

static NET_COMMON_ECODE_E MySetTalkbackToStreamCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stTalkbackToStreamInfo = *(pNET_TalkbackStreamInfo_S)lpInBuffer;
    /* demo: mirror to from-stream cache so client can read back */
    g_stTalkbackFromStreamInfo = g_stTalkbackToStreamInfo;

    printf("[ConfigServerDemo] SetTalkbackToStream callback, Channel=%d\n", dwChannelID);
    printf("  Host=%s, Port=%d, ChnId=%d, UserId=%d, Protocol=%s\n",
           g_stTalkbackToStreamInfo.szHost,
           g_stTalkbackToStreamInfo.nPort,
           g_stTalkbackToStreamInfo.nChnId,
           g_stTalkbackToStreamInfo.nUserID,
           g_stTalkbackToStreamInfo.szProtocol);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyGetTalkbackFromStreamCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    *(pNET_TalkbackStreamInfo_S)lpOutBuffer = g_stTalkbackFromStreamInfo;
    printf("[ConfigServerDemo] GetTalkbackFromStream callback, Channel=%d\n", dwChannelID);
    printf("  Host=%s, Port=%d, ChnId=%d, UserId=%d, Protocol=%s\n",
           g_stTalkbackFromStreamInfo.szHost,
           g_stTalkbackFromStreamInfo.nPort,
           g_stTalkbackFromStreamInfo.nChnId,
           g_stTalkbackFromStreamInfo.nUserID,
           g_stTalkbackFromStreamInfo.szProtocol);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MyGetReplayUrlCb(pNET_ReplayUrlInfo_S pInfo)
{
    if (!pInfo)
    {
        return NET_E_INVALID_PARAM;
    }

    snprintf(pInfo->szUrl,
             sizeof(pInfo->szUrl),
             "rtsp://127.0.0.1:554/replay/channel%d?start=%s&end=%s",
             pInfo->uChannel,
             pInfo->szStartTime,
             pInfo->szEndTime);

    printf("[ConfigServerDemo] GetReplayUrl callback\n");
    printf("  Channel=%d, Start=%s, End=%s\n",
           pInfo->uChannel,
           pInfo->szStartTime,
           pInfo->szEndTime);
    printf("  Url=%s\n", pInfo->szUrl);
    return NET_E_SUCCEED;
}

static int HasTextValue(const CHAR* text)
{
    return (text != NULL && text[0] != '\0');
}

static void CopyTextIfPresent(char* dst, size_t dstSize, const char* src)
{
    if (!dst || dstSize == 0 || !HasTextValue(src))
    {
        return;
    }

    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

static int ParseDateTimeText(const char* text, struct tm* outTm)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (!HasTextValue(text) || !outTm)
    {
        return 0;
    }

    memset(outTm, 0, sizeof(*outTm));
    if (sscanf(text, "%d-%d-%d %d:%d:%d",
               &year, &month, &day, &hour, &minute, &second) != 6)
    {
        return 0;
    }

    outTm->tm_year = year - 1900;
    outTm->tm_mon = month - 1;
    outTm->tm_mday = day;
    outTm->tm_hour = hour;
    outTm->tm_min = minute;
    outTm->tm_sec = second;
    outTm->tm_isdst = -1;
    return 1;
}

static int ShiftDateTimeText(const char* src, int deltaSeconds, char* dst, size_t dstSize)
{
    struct tm timeInfo;
    time_t rawTime = 0;
    struct tm* shiftedTm = NULL;

    if (!dst || dstSize == 0 || !ParseDateTimeText(src, &timeInfo))
    {
        return 0;
    }

    rawTime = mktime(&timeInfo);
    if (rawTime == (time_t)-1)
    {
        return 0;
    }

    rawTime += deltaSeconds;
    shiftedTm = localtime(&rawTime);
    if (!shiftedTm)
    {
        return 0;
    }

    if (strftime(dst, dstSize, "%Y-%m-%d %H:%M:%S", shiftedTm) == 0)
    {
        return 0;
    }

    return 1;
}

static void BuildReplayUrl(NET_ReplayCtrlInfo_S* pInfo)
{
    if (!pInfo)
    {
        return;
    }

    snprintf(pInfo->szUrl,
             sizeof(pInfo->szUrl),
             "rtp://127.0.0.1:6000/playback/%s?channel=%d&start=%s&end=%s&speed=%.2f&replayType=%d&seek=%d",
             pInfo->szSessionId,
             pInfo->uChannel,
             pInfo->szStartTime,
             pInfo->szEndTime,
             pInfo->fSpeed,
             pInfo->nReplayType,
             pInfo->nSeekTime);
}

static void MergeReplayCtrlState(NET_ReplayCtrlInfo_S* pDst, const NET_ReplayCtrlInfo_S* pSrc)
{
    if (!pDst || !pSrc)
    {
        return;
    }

    if (pSrc->uChannel > 0)
    {
        pDst->uChannel = pSrc->uChannel;
    }
    pDst->uCtrlType = pSrc->uCtrlType;
    if (pSrc->fSpeed > 0.0f)
    {
        pDst->fSpeed = pSrc->fSpeed;
    }
    if (pSrc->nSeekTime != 0)
    {
        pDst->nSeekTime = pSrc->nSeekTime;
    }
    if (pSrc->nReplayType != NET_REPLAY_PLATFORM_CTRL_NONE)
    {
        pDst->nReplayType = pSrc->nReplayType;
    }
    CopyTextIfPresent(pDst->szSessionId, sizeof(pDst->szSessionId), pSrc->szSessionId);
    CopyTextIfPresent(pDst->szStartTime, sizeof(pDst->szStartTime), pSrc->szStartTime);
    CopyTextIfPresent(pDst->szEndTime, sizeof(pDst->szEndTime), pSrc->szEndTime);
    CopyTextIfPresent(pDst->szUrl, sizeof(pDst->szUrl), pSrc->szUrl);
}

static NET_COMMON_ECODE_E MyControlReplayCb(pNET_ReplayCtrlInfo_S pInfo)
{
    NET_ReplayCtrlInfo_S stNewState;
    int nDeltaSeconds = 0;

    if (!pInfo)
    {
        return NET_E_INVALID_PARAM;
    }

    memset(&stNewState, 0, sizeof(stNewState));
    MergeReplayCtrlState(&stNewState, &g_stReplayCtrlInfo);
    MergeReplayCtrlState(&stNewState, pInfo);

    if (stNewState.uCtrlType == NET_REPLAY_CTRL_START)
    {
        if (!HasTextValue(stNewState.szSessionId))
        {
            snprintf(stNewState.szSessionId,
                     sizeof(stNewState.szSessionId),
                     "demo_replay_%d",
                     stNewState.uChannel);
        }

        if (stNewState.fSpeed <= 0.0f)
        {
            stNewState.fSpeed = 1.0f;
        }
        stNewState.nReplayType = NET_REPLAY_PLATFORM_CTRL_NONE;
        stNewState.nSeekTime = 0;
        BuildReplayUrl(&stNewState);
    }
    else
    {
        if (!HasTextValue(stNewState.szSessionId))
        {
            return NET_E_INVALID_PARAM;
        }

        if (stNewState.uCtrlType == NET_REPLAY_CTRL_STOP)
        {
            stNewState.szUrl[0] = '\0';
        }
        else if (stNewState.uCtrlType == NET_REPLAY_CTRL_PAUSE)
        {
            BuildReplayUrl(&stNewState);
        }
        else if (stNewState.uCtrlType == NET_REPLAY_CTRL_RESUME)
        {
            BuildReplayUrl(&stNewState);
        }
        else if (stNewState.uCtrlType == NET_REPLAY_CTRL_SET_SPEED)
        {
            if (stNewState.nReplayType == NET_REPLAY_PLATFORM_CTRL_NONE)
            {
                stNewState.nReplayType = NET_REPLAY_PLATFORM_CTRL_SPEED;
            }
            if (stNewState.nReplayType != NET_REPLAY_PLATFORM_CTRL_SPEED || stNewState.fSpeed <= 0.0f)
            {
                return NET_E_INVALID_PARAM;
            }
            BuildReplayUrl(&stNewState);
        }
        else if (stNewState.uCtrlType == NET_REPLAY_CTRL_SET_SEEK)
        {
            if (stNewState.nReplayType == NET_REPLAY_PLATFORM_CTRL_NONE &&
                HasTextValue(stNewState.szStartTime) &&
                HasTextValue(stNewState.szEndTime))
            {
                stNewState.nReplayType = NET_REPLAY_PLATFORM_CTRL_JUMP_TIME;
            }

            switch (stNewState.nReplayType)
            {
                case NET_REPLAY_PLATFORM_CTRL_JUMP_TIME:
                    if (!HasTextValue(stNewState.szStartTime) || !HasTextValue(stNewState.szEndTime))
                    {
                        return NET_E_INVALID_PARAM;
                    }
                    break;
                case NET_REPLAY_PLATFORM_CTRL_BACKWARD_30S:
                    nDeltaSeconds = -(stNewState.nSeekTime > 0 ? stNewState.nSeekTime : 30);
                    if (!ShiftDateTimeText(g_stReplayCtrlInfo.szStartTime, nDeltaSeconds,
                                           stNewState.szStartTime, sizeof(stNewState.szStartTime)) ||
                        !ShiftDateTimeText(g_stReplayCtrlInfo.szEndTime, nDeltaSeconds,
                                           stNewState.szEndTime, sizeof(stNewState.szEndTime)))
                    {
                        return NET_E_INVALID_PARAM;
                    }
                    stNewState.nSeekTime = -nDeltaSeconds;
                    break;
                case NET_REPLAY_PLATFORM_CTRL_FORWARD_30S:
                    nDeltaSeconds = (stNewState.nSeekTime > 0 ? stNewState.nSeekTime : 30);
                    if (!ShiftDateTimeText(g_stReplayCtrlInfo.szStartTime, nDeltaSeconds,
                                           stNewState.szStartTime, sizeof(stNewState.szStartTime)) ||
                        !ShiftDateTimeText(g_stReplayCtrlInfo.szEndTime, nDeltaSeconds,
                                           stNewState.szEndTime, sizeof(stNewState.szEndTime)))
                    {
                        return NET_E_INVALID_PARAM;
                    }
                    stNewState.nSeekTime = nDeltaSeconds;
                    break;
                case NET_REPLAY_PLATFORM_CTRL_PERSON_EVENT:
                case NET_REPLAY_PLATFORM_CTRL_VEHICLE_EVENT:
                case NET_REPLAY_PLATFORM_CTRL_PERSON_VEHICLE_EVENT:
                case NET_REPLAY_PLATFORM_CTRL_CANCEL_EVENT:
                case NET_REPLAY_PLATFORM_CTRL_NONE:
                    break;
                default:
                    return NET_E_INVALID_PARAM;
            }
            BuildReplayUrl(&stNewState);
        }
        else
        {
            return NET_E_INVALID_PARAM;
        }
    }

    g_stReplayCtrlInfo = stNewState;
    *pInfo = g_stReplayCtrlInfo;

    printf("[ConfigServerDemo] ControlReplay callback\n");
    printf("  Channel=%d, CtrlType=%d, ReplayType=%d, Session=%s, Speed=%.2f, SeekTime=%d\n",
           pInfo->uChannel,
           pInfo->uCtrlType,
           pInfo->nReplayType,
           pInfo->szSessionId,
           pInfo->fSpeed,
           pInfo->nSeekTime);
    printf("  Start=%s, End=%s, Url=%s\n",
           pInfo->szStartTime,
           pInfo->szEndTime,
           pInfo->szUrl);
    return NET_E_SUCCEED;
}

static void FillReplayRecordSegment(NET_ReplayRecordTime_S* pSegment, INT32 nStartTime, INT32 nEndTime)
{
    if (!pSegment)
    {
        return;
    }

    memset(pSegment, 0, sizeof(*pSegment));
    pSegment->nStartTime = nStartTime;
    pSegment->nEndTime = nEndTime;
}

static NET_COMMON_ECODE_E MyGetReplayRecordListCb(pNET_ReplayRecordList_S pInfo)
{
    if (!pInfo)
    {
        return NET_E_INVALID_PARAM;
    }

    if (pInfo->szDate[0] == '\0')
    {
        strncpy(pInfo->szDate, "2026-05-06", sizeof(pInfo->szDate) - 1);
    }
    if (pInfo->szStartTime[0] == '\0')
    {
        strncpy(pInfo->szStartTime, "2026-05-06 00:00:00", sizeof(pInfo->szStartTime) - 1);
    }
    if (pInfo->szEndTime[0] == '\0')
    {
        strncpy(pInfo->szEndTime, "2026-05-06 23:59:59", sizeof(pInfo->szEndTime) - 1);
    }

    pInfo->nVideoCount = 2;
    FillReplayRecordSegment(&pInfo->astVideoTimes[0], 8 * 60 * 60, 8 * 60 * 60 + 30 * 60);
    FillReplayRecordSegment(&pInfo->astVideoTimes[1], 14 * 60 * 60, 14 * 60 * 60 + 45 * 60);

    pInfo->nPersonEventCount = 2;
    FillReplayRecordSegment(&pInfo->astPersonEventTimes[0], 8 * 60 * 60 + 5 * 60, 8 * 60 * 60 + 8 * 60);
    FillReplayRecordSegment(&pInfo->astPersonEventTimes[1], 14 * 60 * 60 + 10 * 60, 14 * 60 * 60 + 12 * 60);

    pInfo->nVehicleEventCount = 1;
    FillReplayRecordSegment(&pInfo->astVehicleEventTimes[0], 9 * 60 * 60 + 30 * 60, 9 * 60 * 60 + 33 * 60);

    pInfo->nOtherEventCount = 1;
    FillReplayRecordSegment(&pInfo->astOtherEventTimes[0], 16 * 60 * 60 + 20 * 60, 16 * 60 * 60 + 24 * 60);

    printf("[ConfigServerDemo] GetReplayRecordList callback\n");
    printf("  Channel=%d, FilterByEventType=%d, EventType=%d, Date=%s, Start=%s, End=%s, Video=%d, Person=%d, Vehicle=%d, Other=%d\n",
           pInfo->uChannel,
           pInfo->bFilterByEventType,
           pInfo->uEventType,
           pInfo->szDate,
           pInfo->szStartTime,
           pInfo->szEndTime,
           pInfo->nVideoCount,
           pInfo->nPersonEventCount,
           pInfo->nVehicleEventCount,
           pInfo->nOtherEventCount);
    return NET_E_SUCCEED;
}

static NET_COMMON_ECODE_E MySetReplayTalkbackCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    g_stReplayTalkbackInfo = *(pNET_ReplayTalkbackInfo_S)lpInBuffer;
    printf("[ConfigServerDemo] SetReplayTalkback callback, Channel=%d\n", dwChannelID);
    printf("  NvrIp=%s, RemoteIp=%s, File=%s\n",
           g_stReplayTalkbackInfo.szNvrIp,
           g_stReplayTalkbackInfo.szRemoteIp,
           g_stReplayTalkbackInfo.stIPCInfo.szFileName);
    return NET_E_SUCCEED;
}

/* 升级状态 Get 回调，对应命令 NET_GET_UPGRADESTATUS */
static NET_COMMON_ECODE_E MyGetUpgradeStatus(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_UpgradeStatus_S pOut = (pNET_UpgradeStatus_S)lpOutBuffer;
    *pOut = g_stUpgradeStatuts;

    printf("[ConfigServerDemo] MyGetUpgradeStatus callback, Channel=%d\n", dwChannelID);
    printf("    nUpgradeStatus=%d\n", pOut->nUpgradeStatus);
    return NET_E_SUCCEED;
}

/* 升级文件 Set 回调，对应命令 NET_SET_UPGRADE */
static NET_COMMON_ECODE_E MySetUpgrade(INT32 dwChannelID, LPVOID lpInBuffer)
{
    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    pNET_UpgradeInfo_S pIn = (pNET_UpgradeInfo_S)lpInBuffer;

    memset(&g_stUpgradeInfo, 0, sizeof(g_stUpgradeInfo));
    strncpy(g_stUpgradeInfo.szUpgradePath, pIn->szUpgradePath, sizeof(g_stUpgradeInfo.szUpgradePath) - 1);
    g_stUpgradeInfo.szUpgradePath[sizeof(g_stUpgradeInfo.szUpgradePath) - 1] = '\0';

    printf("[ConfigServerDemo] MySetUpgrade callback, Channel=%d\n", dwChannelID);
    printf("    szUpgradePath=%s\n", pIn->szUpgradePath);
    printf("    Demo only: skip AC_SET_UPGRADE / AC_CHECK_UPGRADE.\n");
    return NET_E_SUCCEED;
}

/* 升级版本 Get 回调，对应命令 NET_GET_UPGRADEVERSION */
static NET_COMMON_ECODE_E MyGetUpgradeVersion(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pNET_UpgradeVersion_S pOut = (pNET_UpgradeVersion_S)lpOutBuffer;
    *pOut = g_stUpgradeVersions;

    printf("[ConfigServerDemo] MyGetUpgradeVersion callback, Channel=%d\n", dwChannelID);
    printf("    szVersion=%s\n", pOut->szVersion);
    return NET_E_SUCCEED;
}

/* 打印抓图计划信息 */
static void PrintCapturePlanInfo(const NET_CapturePlanInfo_S *pstCfg)
{
    UINT32 i = 0;
    if (!pstCfg)
        return;

    for (i = 0; i < NET_PLAN_DAY_NUM_AWEEK; ++i)
    {
        UINT32 j = 0;
        UINT32 nTimeCount = pstCfg->astDaySchedules[i].udwTimeCount;
        if (nTimeCount > NET_PLAN_TIME_SECTION_NUM_ADAY)
            nTimeCount = NET_PLAN_TIME_SECTION_NUM_ADAY;

        printf("    Day[%u] week=%d timeCount=%u\n",
               i,
               pstCfg->astDaySchedules[i].nDayOfWeek,
               nTimeCount);

        for (j = 0; j < nTimeCount; ++j)
        {
            printf("      Time[%u] start=%d end=%d\n",
                   j,
                   pstCfg->astDaySchedules[i].astTimes[j].nStartTime,
                   pstCfg->astDaySchedules[i].astTimes[j].nEndTime);
        }
    }
}

/* 打印单张抓图参数*/
static void PrintOneCaptureConfig(const char *prefix, const NET_CaptureConfig_S *pstCfg)
{
    if (!pstCfg)
        return;

    printf("    %s enable=%d format=%d resolution=%dx%d quality=%d interval=%u unit=%d number=%u\n",
           prefix ? prefix : "CaptureCfg",
           pstCfg->bEnable,
           pstCfg->enPictureFormat,
           pstCfg->nWidth,
           pstCfg->nHeight,
           pstCfg->enImageQuality,
           pstCfg->unInterval,
           pstCfg->enTimeUnit,
           pstCfg->unNumber);
}

/* 抓图计划信息 Get 回调，对应命令 NET_GET_CAPTURE_PLAN_INFO */
static NET_COMMON_ECODE_E MyGetCapturePlanInfo(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    pNET_CapturePlanInfo_S pOut = NULL;

    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pOut = (pNET_CapturePlanInfo_S)lpOutBuffer;
    *pOut = g_stCapturePlanInfo;

    printf("[ConfigServerDemo] MyGetCapturePlanInfo callback, Channel=%d\n", dwChannelID);
    PrintCapturePlanInfo(pOut);
    return NET_E_SUCCEED;
}

/* 抓图计划信息 Set 回调，对应命令 NET_SET_CAPTURE_PLAN_INFO */
static NET_COMMON_ECODE_E MySetCapturePlanInfo(INT32 dwChannelID, LPVOID lpInBuffer)
{
    pNET_CapturePlanInfo_S pIn = NULL;

    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    pIn = (pNET_CapturePlanInfo_S)lpInBuffer;
    g_stCapturePlanInfo = *pIn;

    printf("[ConfigServerDemo] MySetCapturePlanInfo callback, Channel=%d\n", dwChannelID);
    PrintCapturePlanInfo(&g_stCapturePlanInfo);
    return NET_E_SUCCEED;
}

/* 抓图参数 Get 回调，对应命令 NET_GET_CAPTURE_PARAM_INFO */
static NET_COMMON_ECODE_E MyGetCaptureParamInfo(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    pNET_CaptureParamInfo_S pOut = NULL;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pOut = (pNET_CaptureParamInfo_S)lpOutBuffer;
    *pOut = g_stCaptureParamInfo;

    printf("[ConfigServerDemo] MyGetCaptureParamInfo callback, Channel=%d\n", dwChannelID);
    PrintOneCaptureConfig("TimingCfg", &pOut->stCaptureTimingConfig);
    PrintOneCaptureConfig("EventCfg", &pOut->stCaptureEventConfig);
    return NET_E_SUCCEED;
}

/* 抓图参数 Set 回调，对应命令 NET_SET_CAPTURE_PARAM_INFO */
static NET_COMMON_ECODE_E MySetCaptureParamInfo(INT32 dwChannelID, LPVOID lpInBuffer)
{
    pNET_CaptureParamInfo_S pIn = NULL;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pIn = (pNET_CaptureParamInfo_S)lpInBuffer;
    g_stCaptureParamInfo = *pIn;

    printf("[ConfigServerDemo] MySetCaptureParamInfo callback, Channel=%d\n", dwChannelID);
    PrintOneCaptureConfig("TimingCfg", &g_stCaptureParamInfo.stCaptureTimingConfig);
    PrintOneCaptureConfig("EventCfg", &g_stCaptureParamInfo.stCaptureEventConfig);
    return NET_E_SUCCEED;
}

/**
 * @brief 打印曝光信息
 * @param pstCfg 曝光信息结构体指针
 */
static void PrintExposureInfo(const NET_ExposureInfo_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    enExpTime=%d antiBanding=%d\n", pstCfg->enExpTime, pstCfg->bAntiBanding);
}

/**
 * @brief 打印日夜信息
 * @param pstCfg 日夜信息结构体指针
 */
static void PrintDayNightInfo(const NET_DayNightInfo_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    mode=%d begin=%02d:%02d:%02d.%03d end=%02d:%02d:%02d.%03d\n",
           pstCfg->enDayNightMode,
           pstCfg->nBeginHour,
           pstCfg->nBeginMinute,
           pstCfg->nBeginSecond,
           pstCfg->nBeginMilliSec,
           pstCfg->nEndHour,
           pstCfg->nEndMinute,
           pstCfg->nEndSecond,
           pstCfg->nEndMilliSec);
    printf("    sensitivity=%u filterTime=%u fillLightExp=%d lightMode=%d lightType=%d\n",
           pstCfg->nSensitivityLevel,
           pstCfg->nFilterTime,
           pstCfg->bFillLightExp,
           pstCfg->enLightMode,
           pstCfg->enLightType);
    printf("    whiteEnable=%d whiteLevel=%d redEnable=%d redLevel=%d\n",
           pstCfg->bWhiteLightEnable,
           pstCfg->nWhiteLightLevel,
           pstCfg->bRedLightEnable,
           pstCfg->nRedLightLevel);
}

/**
 * @brief 打印背光信息
 * @param pstCfg 背光信息结构体指针
 */
static void PrintBackLightInfo(const NET_BackLightInfo_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    area=%d wdrEnable=%d wdrLevel=%d hlsEnable=%d hlsLevel=%d\n",
           pstCfg->enBackLightArea,
           pstCfg->bWdrEnable,
           pstCfg->nWdrLevel,
           pstCfg->bHlsEnable,
           pstCfg->nHlsLevel);
}

/**
 * @brief 打印降噪信息
 * @param pstCfg 降噪信息结构体指针
 */

static void PrintDenoiseInfo(const NET_DenoiseInfo_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    mode=%d dnrLevel=%u snrLevel=%u tnrLevel=%u\n",
           pstCfg->enDnrMode,
           pstCfg->nDnrLevel,
           pstCfg->nSnrLevel,
           pstCfg->nTnrLevel);
}
/**
 * @brief 打印白平衡信息
 * @param pstCfg 白平衡信息结构体指针
 */
static void PrintWhiteBalanceInfo(const NET_WhiteBalanceInfo_S *pstCfg)
{
    if (!pstCfg)
        return;
    printf("    mode=%d rGain=%u bGain=%u\n",
           pstCfg->enAwbMode,
           pstCfg->nRGain,
           pstCfg->nBGain);
}

/**
 * @brief 获取曝光信息的回调函数
 */
static NET_COMMON_ECODE_E MyGetExposureInfo(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    pNET_ExposureInfo_S pOut = NULL;

    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pOut = (pNET_ExposureInfo_S)lpOutBuffer;
    *pOut = g_stExposureInfo;

    printf("[ConfigServerDemo] MyGetExposureInfo callback, Channel=%d\n", dwChannelID);
    PrintExposureInfo(pOut);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置曝光信息的回调函数
 */
static NET_COMMON_ECODE_E MySetExposureInfo(INT32 dwChannelID, LPVOID lpInBuffer)
{
    pNET_ExposureInfo_S pIn = NULL;

    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    pIn = (pNET_ExposureInfo_S)lpInBuffer;
    g_stExposureInfo = *pIn;

    printf("[ConfigServerDemo] MySetExposureInfo callback, Channel=%d\n", dwChannelID);
    PrintExposureInfo(&g_stExposureInfo);
    return NET_E_SUCCEED;
}

/**
 * @brief 获取日夜信息的回调函数
 */
static NET_COMMON_ECODE_E MyGetDayNightInfo(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    pNET_DayNightInfo_S pOut = NULL;

    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pOut = (pNET_DayNightInfo_S)lpOutBuffer;
    *pOut = g_stDayNightInfo;

    printf("[ConfigServerDemo] MyGetDayNightInfo callback, Channel=%d\n", dwChannelID);
    PrintDayNightInfo(pOut);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置日夜信息的回调函数
 */
static NET_COMMON_ECODE_E MySetDayNightInfo(INT32 dwChannelID, LPVOID lpInBuffer)
{
    pNET_DayNightInfo_S pIn = NULL;

    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    pIn = (pNET_DayNightInfo_S)lpInBuffer;
    g_stDayNightInfo = *pIn;

    printf("[ConfigServerDemo] MySetDayNightInfo callback, Channel=%d\n", dwChannelID);
    PrintDayNightInfo(&g_stDayNightInfo);
    return NET_E_SUCCEED;
}

/**
 * @brief 获取背光信息的回调函数
 */
static NET_COMMON_ECODE_E MyGetBackLightInfo(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    pNET_BackLightInfo_S pOut = NULL;

    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pOut = (pNET_BackLightInfo_S)lpOutBuffer;
    *pOut = g_stBackLightInfo;

    printf("[ConfigServerDemo] MyGetBackLightInfo callback, Channel=%d\n", dwChannelID);
    PrintBackLightInfo(pOut);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置背光信息的回调函数
 */
static NET_COMMON_ECODE_E MySetBackLightInfo(INT32 dwChannelID, LPVOID lpInBuffer)
{
    pNET_BackLightInfo_S pIn = NULL;

    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    pIn = (pNET_BackLightInfo_S)lpInBuffer;
    g_stBackLightInfo = *pIn;

    printf("[ConfigServerDemo] MySetBackLightInfo callback, Channel=%d\n", dwChannelID);
    PrintBackLightInfo(&g_stBackLightInfo);
    return NET_E_SUCCEED;
}

/**
 * @brief 获取降噪信息的回调函数
 */
static NET_COMMON_ECODE_E MyGetDenoiseInfo(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    pNET_DenoiseInfo_S pOut = NULL;

    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pOut = (pNET_DenoiseInfo_S)lpOutBuffer;
    *pOut = g_stDenoiseInfo;

    printf("[ConfigServerDemo] MyGetDenoiseInfo callback, Channel=%d\n", dwChannelID);
    PrintDenoiseInfo(pOut);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置降噪信息的回调函数
 */
static NET_COMMON_ECODE_E MySetDenoiseInfo(INT32 dwChannelID, LPVOID lpInBuffer)
{
    pNET_DenoiseInfo_S pIn = NULL;

    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    pIn = (pNET_DenoiseInfo_S)lpInBuffer;
    g_stDenoiseInfo = *pIn;

    printf("[ConfigServerDemo] MySetDenoiseInfo callback, Channel=%d\n", dwChannelID);
    PrintDenoiseInfo(&g_stDenoiseInfo);
    return NET_E_SUCCEED;
}

/**
 * @brief 获取白平衡信息的回调函数
 */
static NET_COMMON_ECODE_E MyGetWhiteBalanceInfo(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    pNET_WhiteBalanceInfo_S pOut = NULL;

    if (!lpOutBuffer)
        return NET_E_INVALID_PARAM;

    pOut = (pNET_WhiteBalanceInfo_S)lpOutBuffer;
    *pOut = g_stWhiteBalanceInfo;

    printf("[ConfigServerDemo] MyGetWhiteBalanceInfo callback, Channel=%d\n", dwChannelID);
    PrintWhiteBalanceInfo(pOut);
    return NET_E_SUCCEED;
}

/**
 * @brief 设置白平衡信息的回调函数
 */
static NET_COMMON_ECODE_E MySetWhiteBalanceInfo(INT32 dwChannelID, LPVOID lpInBuffer)
{
    pNET_WhiteBalanceInfo_S pIn = NULL;

    if (!lpInBuffer)
        return NET_E_INVALID_PARAM;

    pIn = (pNET_WhiteBalanceInfo_S)lpInBuffer;
    g_stWhiteBalanceInfo = *pIn;

    printf("[ConfigServerDemo] MySetWhiteBalanceInfo callback, Channel=%d\n", dwChannelID);
    PrintWhiteBalanceInfo(&g_stWhiteBalanceInfo);
    return NET_E_SUCCEED;
}

/* 人脸抓拍配置 Get 回调，对应命令 NET_GET_FACECAPTUREINFO */
static NET_COMMON_ECODE_E MyGetFaceCaptureInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceCaptureInfo_S pOut = (pNET_FaceCaptureInfo_S)lpOutBuffer;
    *pOut = g_stFaceCaptureInfo;

    printf("[ConfigServerDemo] GetFaceCaptureInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, Sensitivity=%d, RegionPointCount=%d, ShieldRegionCount=%d\n",
           g_stFaceCaptureInfo.bEnable,
           g_stFaceCaptureInfo.stRule.nSensitivity,
           g_stFaceCaptureInfo.stRule.stRegion.uPointCount,
           g_stFaceCaptureInfo.stRule.uShieldRegionCount);

    return NET_E_SUCCEED;
}

/* 人脸抓拍配置 Set 回调，对应命令 NET_SET_FACECAPTUREINFO */
static NET_COMMON_ECODE_E MySetFaceCaptureInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceCaptureInfo_S pIn = (pNET_FaceCaptureInfo_S)lpInBuffer;
    g_stFaceCaptureInfo = *pIn;

    printf("[ConfigServerDemo] SetFaceCaptureInfo callback, Channel=%d\n", dwChannelID);
    printf("  New Enable=%d, Sensitivity=%d, RegionPointCount=%d, ShieldRegionCount=%d\n",
           g_stFaceCaptureInfo.bEnable,
           g_stFaceCaptureInfo.stRule.nSensitivity,
           g_stFaceCaptureInfo.stRule.stRegion.uPointCount,
           g_stFaceCaptureInfo.stRule.uShieldRegionCount);

    return NET_E_SUCCEED;
}

static INT32 FindTargetLibIndex(const CHAR* szFaceLibName)
{
    if (!szFaceLibName || szFaceLibName[0] == '\0')
    {
        return -1;
    }

    for (INT32 i = 0; i < g_stFaceLibList.nTargetLibCount && i < NET_FACE_LIB_MAX_NUM; ++i)
    {
        if (strcmp(g_stFaceLibList.astTargetLibInfos[i].szFaceLibName, szFaceLibName) == 0)
        {
            return i;
        }
    }

    return -1;
}

static INT32 FindFaceInfoIndex(INT32 nId)
{
    for (INT32 i = 0; i < g_stFaceInfoList.nFaceInfoCount && i < NET_FACE_INFO_MAX_NUM; ++i)
    {
        if (g_stFaceInfoList.astFaceInfos[i].nId == nId)
        {
            return i;
        }
    }

    return -1;
}

/* 人脸比对配置 Set 回调，对应命令 NET_SET_FACE_COMPARE_INFO */
static NET_COMMON_ECODE_E MySetFaceCompareInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceCompareInfo_S pIn = (pNET_FaceCompareInfo_S)lpInBuffer;
    g_stFaceCompareInfo = *pIn;

    printf("[ConfigServerDemo] SetFaceCompareInfo callback, Channel=%d\n", dwChannelID);
    printf("  Enable=%d, SuccessSnapshotCount=%d, FailSnapshotCount=%d\n",
           g_stFaceCompareInfo.bEnable,
           g_stFaceCompareInfo.stLinkageListSuccess.uSnapshotChannelCount,
           g_stFaceCompareInfo.stLinkageListFail.uSnapshotChannelCount);

    return NET_E_SUCCEED;
}

/* 添加目标库 Set 回调，对应命令 NET_ADD_TARGET_LIB */
static NET_COMMON_ECODE_E MyAddTargetLibCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceLibInfo_S pIn = (pNET_FaceLibInfo_S)lpInBuffer;
    INT32 nIndex = FindTargetLibIndex(pIn->szFaceLibName);
    if (nIndex < 0)
    {
        if (g_stFaceLibList.nTargetLibCount >= NET_FACE_LIB_MAX_NUM)
        {
            return NET_E_NOENOUGH_BUF;
        }
        nIndex = g_stFaceLibList.nTargetLibCount++;
    }

    g_stFaceLibList.astTargetLibInfos[nIndex] = *pIn;
    printf("[ConfigServerDemo] AddTargetLib callback, LibId=%s, Count=%d\n",
           pIn->szFaceLibName, g_stFaceLibList.nTargetLibCount);
    return NET_E_SUCCEED;
}

/* 删除目标库 Set 回调，对应命令 NET_DEL_TARGET_LIB */
static NET_COMMON_ECODE_E MyDelTargetLibCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceLibInfo_S pIn = (pNET_FaceLibInfo_S)lpInBuffer;
    INT32 nIndex = FindTargetLibIndex(pIn->szFaceLibName);
    if (nIndex >= 0)
    {
        for (INT32 i = nIndex; i < g_stFaceLibList.nTargetLibCount - 1; ++i)
        {
            g_stFaceLibList.astTargetLibInfos[i] = g_stFaceLibList.astTargetLibInfos[i + 1];
        }
        memset(&g_stFaceLibList.astTargetLibInfos[g_stFaceLibList.nTargetLibCount - 1],
               0,
               sizeof(g_stFaceLibList.astTargetLibInfos[0]));
        g_stFaceLibList.nTargetLibCount--;
    }

    printf("[ConfigServerDemo] DelTargetLib callback, LibId=%s, Count=%d\n",
           pIn->szFaceLibName, g_stFaceLibList.nTargetLibCount);
    return NET_E_SUCCEED;
}

/* 修改目标库 Set 回调，对应命令 NET_SET_TARGET_LIB */
static NET_COMMON_ECODE_E MySetTargetLibCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceLibInfo_S pIn = (pNET_FaceLibInfo_S)lpInBuffer;
    INT32 nIndex = FindTargetLibIndex(pIn->szFaceLibName);
    if (nIndex < 0)
    {
        if (g_stFaceLibList.nTargetLibCount >= NET_FACE_LIB_MAX_NUM)
        {
            return NET_E_NOENOUGH_BUF;
        }
        nIndex = g_stFaceLibList.nTargetLibCount++;
    }

    g_stFaceLibList.astTargetLibInfos[nIndex] = *pIn;
    printf("[ConfigServerDemo] SetTargetLib callback, LibId=%s, Count=%d\n",
           pIn->szFaceLibName, g_stFaceLibList.nTargetLibCount);
    return NET_E_SUCCEED;
}

/* 获取目标库 Get 回调，对应命令 NET_GET_TARGET_LIB */
static NET_COMMON_ECODE_E MyGetTargetLibCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceLibList_S pOut = (pNET_FaceLibList_S)lpOutBuffer;
    *pOut = g_stFaceLibList;

    printf("[ConfigServerDemo] GetTargetLib callback, Count=%d\n", pOut->nTargetLibCount);
    return NET_E_SUCCEED;
}

/* 添加人脸 Set 回调，对应命令 NET_ADD_FACE_INFO */
static NET_COMMON_ECODE_E MyAddFaceInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceInfo_S pIn = (pNET_FaceInfo_S)lpInBuffer;
    INT32 nIndex = FindFaceInfoIndex(pIn->nId);
    if (nIndex < 0)
    {
        if (g_stFaceInfoList.nFaceInfoCount >= NET_FACE_INFO_MAX_NUM)
        {
            return NET_E_NOENOUGH_BUF;
        }
        nIndex = g_stFaceInfoList.nFaceInfoCount++;
    }

    g_stFaceInfoList.astFaceInfos[nIndex] = *pIn;
    printf("[ConfigServerDemo] AddFaceInfo callback, Id=%d, Name=%s, Count=%d\n",
           pIn->nId, pIn->szName, g_stFaceInfoList.nFaceInfoCount);
    return NET_E_SUCCEED;
}

/* 删除人脸 Set 回调，对应命令 NET_DEL_FACE_INFO */
static NET_COMMON_ECODE_E MyDelFaceInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceIdInfo_S pIn = (pNET_FaceIdInfo_S)lpInBuffer;
    for (INT32 idIndex = 0; idIndex < pIn->nIdCount && idIndex < NET_FACE_ID_MAX_NUM; ++idIndex)
    {
        INT32 nIndex = FindFaceInfoIndex(pIn->anIds[idIndex]);
        if (nIndex < 0)
        {
            continue;
        }

        for (INT32 i = nIndex; i < g_stFaceInfoList.nFaceInfoCount - 1; ++i)
        {
            g_stFaceInfoList.astFaceInfos[i] = g_stFaceInfoList.astFaceInfos[i + 1];
        }
        memset(&g_stFaceInfoList.astFaceInfos[g_stFaceInfoList.nFaceInfoCount - 1],
               0,
               sizeof(g_stFaceInfoList.astFaceInfos[0]));
        g_stFaceInfoList.nFaceInfoCount--;
    }

    printf("[ConfigServerDemo] DelFaceInfo callback, IdCount=%d, Count=%d\n",
           pIn->nIdCount, g_stFaceInfoList.nFaceInfoCount);
    return NET_E_SUCCEED;
}

/* 修改人脸 Set 回调，对应命令 NET_SET_FACE_INFO */
static NET_COMMON_ECODE_E MySetFaceInfoCb(INT32 dwChannelID, LPVOID lpInBuffer)
{
    (void)dwChannelID;

    if (!lpInBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceInfo_S pIn = (pNET_FaceInfo_S)lpInBuffer;
    INT32 nIndex = FindFaceInfoIndex(pIn->nId);
    if (nIndex < 0)
    {
        if (g_stFaceInfoList.nFaceInfoCount >= NET_FACE_INFO_MAX_NUM)
        {
            return NET_E_NOENOUGH_BUF;
        }
        nIndex = g_stFaceInfoList.nFaceInfoCount++;
    }

    g_stFaceInfoList.astFaceInfos[nIndex] = *pIn;
    printf("[ConfigServerDemo] SetFaceInfo callback, Id=%d, Name=%s, Count=%d\n",
           pIn->nId, pIn->szName, g_stFaceInfoList.nFaceInfoCount);
    return NET_E_SUCCEED;
}

/* 获取人脸 Get 回调，对应命令 NET_GET_FACE_INFO */
static NET_COMMON_ECODE_E MyGetFaceInfoCb(INT32 dwChannelID, LPVOID lpOutBuffer)
{
    (void)dwChannelID;

    if (!lpOutBuffer)
    {
        return NET_E_INVALID_PARAM;
    }

    pNET_FaceInfoList_S pOut = (pNET_FaceInfoList_S)lpOutBuffer;
    *pOut = g_stFaceInfoList;

    printf("[ConfigServerDemo] GetFaceInfo callback, Count=%d\n", pOut->nFaceInfoCount);
    return NET_E_SUCCEED;
}

/* 注册所有需要的回调 */
static void RegisterCallbacks(void)
{
    /* 设备信息回调（登录时会用到） */
    if (NET_SERVER_RegisterCb_GetDeviceInfo(MyDeviceInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetDeviceInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetDeviceInfo FAILED\n");
    }

    /* 设备基本信息配置回调 */
    if (NET_SERVER_RegisterCb_GetDeviceCfg(MyGetDeviceCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetDeviceCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetDeviceCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetDeviceCfg(MySetDeviceCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetDeviceCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetDeviceCfg FAILED\n");
    }

    /* 网络配置回调 */
    if (NET_SERVER_RegisterCb_GetNetworkCfg(MyGetNetworkCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetNetworkCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetNetworkCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetNetworkCfg(MySetNetworkCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetNetworkCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetNetworkCfg FAILED\n");
    }

    /* 系统校时配置回调 */
    if (NET_SERVER_RegisterCb_GetNtpCfg(MyGetNtpCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetNtpCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetNtpCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetNtpCfg(MySetNtpCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetNtpCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetNtpCfg FAILED\n");
    }

    /* 音频配置回调 */
    if (NET_SERVER_RegisterCb_GetAudioCfg(MyGetAudioCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetAudioCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetAudioCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetAudioCfg(MySetAudioCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetAudioCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetAudioCfg FAILED\n");
    }

    /* 视频码流配置回调 */
    if (NET_SERVER_RegisterCb_GetStreamCfg(MyGetStreamCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetStreamCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetStreamCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetStreamCfg(MySetStreamCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetStreamCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetStreamCfg FAILED\n");
    }

    /* WIFI/4G/热点配置回调 */
    if (NET_SERVER_RegisterCb_SetConfigWifiSta(MySetConfigWifiStaCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetConfigWifiSta SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetConfigWifiSta FAILED\n");
    }

    if (NET_SERVER_RegisterCb_ConnectWifiSta(MyConnectWifiStaCb))
    {
        printf("[ConfigServerDemo] RegisterCb_ConnectWifiSta SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_ConnectWifiSta FAILED\n");
    }

    if (NET_SERVER_RegisterCb_DisconnectWifiSta(MyDisconnectWifiStaCb))
    {
        printf("[ConfigServerDemo] RegisterCb_DisconnectWifiSta SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_DisconnectWifiSta FAILED\n");
    }

    if (NET_SERVER_RegisterCb_Get4GInfo(MyGet4GInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_Get4GInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_Get4GInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_Set4GInfo(MySet4GInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_Set4GInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_Set4GInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetHotspotInfo(MySetHotspotInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetHotspotInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetHotspotInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetHotspotConn(MyGetHotspotConnCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetHotspotConn SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetHotspotConn FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetSecurityServicesInfo(MyGetSecurityServicesInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetSecurityServicesInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetSecurityServicesInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetSecurityServicesInfo(MySetSecurityServicesInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetSecurityServicesInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetSecurityServicesInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetSshCountdown(MyGetSshCountdownCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetSshCountdown SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetSshCountdown FAILED\n");
    }

    if (NET_SERVER_RegisterCb_FindLog(MyFindLogCb))
    {
        printf("[ConfigServerDemo] RegisterCb_FindLog SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_FindLog FAILED\n");
    }

    if (NET_SERVER_RegisterCb_ExportLog(MyExportLogCb))
    {
        printf("[ConfigServerDemo] RegisterCb_ExportLog SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_ExportLog FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetLogServer(MyGetLogServerCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetLogServer SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetLogServer FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetLogServer(MySetLogServerCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetLogServer SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetLogServer FAILED\n");
    }

    if (NET_SERVER_RegisterCb_TestLogServer(MyTestLogServerCb))
    {
        printf("[ConfigServerDemo] RegisterCb_TestLogServer SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_TestLogServer FAILED\n");
    }

    if (NET_SERVER_RegisterCb_ControlRecordInfo(MyControlRecordInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_ControlRecordInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_ControlRecordInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetRecordStatus(MyGetRecordStatusCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetRecordStatus SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetRecordStatus FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetRecordSchedule(MyGetRecordScheduleCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetRecordSchedule SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetRecordSchedule FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetRecordSchedule(MySetRecordScheduleCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetRecordSchedule SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetRecordSchedule FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetRecordAdvancedParam(MyGetRecordAdvancedParamCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetRecordAdvancedParam SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetRecordAdvancedParam FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetRecordAdvancedParam(MySetRecordAdvancedParamCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetRecordAdvancedParam SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetRecordAdvancedParam FAILED\n");
    }

    if (NET_SERVER_RegisterCb_FindRecordFileInfo(MyFindRecordFileInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_FindRecordFileInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_FindRecordFileInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_DownloadRecordFile(MyDownloadRecordFileCb))
    {
        printf("[ConfigServerDemo] RegisterCb_DownloadRecordFile SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_DownloadRecordFile FAILED\n");
    }

    /* OSD能力集配置回调 */
    if (NET_SERVER_RegisterCb_GetOsdCapCfg(MyGetOSDCapCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetOSDCapCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetOSDCapCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetOsdCapCfg(MySetOSDCapCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetOSDCapCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetOSDCapCfg FAILED\n");
    }

    /* 移动侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetMotionAlarm(MyGetMotionAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetMotionAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetMotionAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetMotionAlarm(MySetMotionAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetMotionAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetMotionAlarm FAILED\n");
    }

    /* 隐私遮盖配置回调 */
    if (NET_SERVER_RegisterCb_GetPrivacyMaskCfg(MyGetPrivacyMaskCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPrivacyMaskCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPrivacyMaskCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetPrivacyMaskCfg(MySetPrivacyMaskCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPrivacyMaskCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPrivacyMaskCfg FAILED\n");
    }

    /* 遮挡报警配置回调 */
    if (NET_SERVER_RegisterCb_GetTamperAlarm(MyGetTamperAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetTamperAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetTamperAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetTamperAlarm(MySetTamperAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetTamperAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetTamperAlarm FAILED\n");
    }

    /* 越界检测配置回调 */
    if (NET_SERVER_RegisterCb_GetCrossLineAlarm(MyGetCrossLineAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetCrossLineAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetCrossLineAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetCrossLineAlarm(MySetCrossLineAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetCrossLineAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetCrossLineAlarm FAILED\n");
    }

    /* 入侵检测配置回调 */
    if (NET_SERVER_RegisterCb_GetIntrusionAlarm(MyGetIntrusionAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetIntrusionAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetIntrusionAlarm FAILED\n");
    }

    /* 进入区域侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetEnterRegionAlarm(MyGetEnterRegionAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetEnterRegionAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetEnterRegionAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetEnterRegionAlarm(MySetEnterRegionAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetEnterRegionAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetEnterRegionAlarm FAILED\n");
    }

    /* 离开区域侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetLeaveRegionAlarm(MyGetLeaveRegionAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetLeaveRegionAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetLeaveRegionAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetLeaveRegionAlarm(MySetLeaveRegionAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetLeaveRegionAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetLeaveRegionAlarm FAILED\n");
    }

    /* 徘徊侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetLoiteringAlarm(MyGetLoiteringAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetLoiteringAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetLoiteringAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetLoiteringAlarm(MySetLoiteringAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetLoiteringAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetLoiteringAlarm FAILED\n");
    }

    /* 场景变更侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetSceneChangeAlarm(MyGetSceneChangeAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetSceneChangeAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetSceneChangeAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetSceneChangeAlarm(MySetSceneChangeAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetSceneChangeAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetSceneChangeAlarm FAILED\n");
    }

    /* 人员聚集侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetCrowGatheringAlarm(MyGetCrowdGatheringAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetCrowdGatheringAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetCrowdGatheringAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetCrowGatheringAlarm(MySetCrowdGatheringAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetCrowdGatheringAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetCrowdGatheringAlarm FAILED\n");
    }

    /* 垃圾暴露配置回调 */
    if (NET_SERVER_RegisterCb_GetGarbageExposureCfg(MyGetGarbageExposureCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetGarbageExposureCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetGarbageExposureCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetGarbageExposureCfg(MySetGarbageExposureCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetGarbageExposureCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetGarbageExposureCfg FAILED\n");
    }

    /* 垃圾满溢配置回调 */
    if (NET_SERVER_RegisterCb_GetGarbageOverflowCfg(MyGetGarbageOverflowCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetGarbageOverflowCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetGarbageOverflowCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetGarbageOverflowCfg(MySetGarbageOverflowCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetGarbageOverflowCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetGarbageOverflowCfg FAILED\n");
    }

    /* 人流统计配置回调 */
    if (NET_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg(MyGetPeopleFlowStatisticsCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPeopleFlowStatisticsCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPeopleFlowStatisticsCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg(MySetPeopleFlowStatisticsCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPeopleFlowStatisticsCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPeopleFlowStatisticsCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_ResetPeopleFlowStatistics(MyResetPeopleFlowStatisticsCb))
    {
        printf("[ConfigServerDemo] RegisterCb_ResetPeopleFlowStatistics SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_ResetPeopleFlowStatistics FAILED\n");
    }

    /* 人员密度检测配置回调 */
    if (NET_SERVER_RegisterCb_GetPeopleDensityDetectionCfg(MyGetPeopleDensityDetectionCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPeopleDensityDetectionCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPeopleDensityDetectionCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetPeopleDensityDetectionCfg(MySetPeopleDensityDetectionCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPeopleDensityDetectionCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPeopleDensityDetectionCfg FAILED\n");
    }

    /* 井盖异常检测配置回调 */
    if (NET_SERVER_RegisterCb_GetManholeCoverAbnormalCfg(MyGetManholeCoverAbnormalCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetManholeCoverAbnormalCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetManholeCoverAbnormalCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetManholeCoverAbnormalCfg(MySetManholeCoverAbnormalCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetManholeCoverAbnormalCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetManholeCoverAbnormalCfg FAILED\n");
    }

    /* 睡岗识别配置回调 */
    if (NET_SERVER_RegisterCb_GetSleepOnDutyCfg(MyGetSleepOnDutyCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetSleepOnDutyCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetSleepOnDutyCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetSleepOnDutyCfg(MySetSleepOnDutyCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetSleepOnDutyCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetSleepOnDutyCfg FAILED\n");
    }

    /* 电瓶车进电梯识别配置回调 */
    if (NET_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg(MyGetElectricVehicleInElevatorCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetElectricVehicleInElevatorCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetElectricVehicleInElevatorCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg(MySetElectricVehicleInElevatorCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetElectricVehicleInElevatorCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetElectricVehicleInElevatorCfg FAILED\n");
    }

    /* 人员倒地识别配置回调 */
    if (NET_SERVER_RegisterCb_GetPersonFallDownCfg(MyGetPersonFallDownCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPersonFallDownCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPersonFallDownCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetPersonFallDownCfg(MySetPersonFallDownCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPersonFallDownCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPersonFallDownCfg FAILED\n");
    }

    /* 施工占道识别配置回调 */
    if (NET_SERVER_RegisterCb_GetConstructionOccupyRoadCfg(MyGetConstructionOccupyRoadCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetConstructionOccupyRoadCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetConstructionOccupyRoadCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetConstructionOccupyRoadCfg(MySetConstructionOccupyRoadCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetConstructionOccupyRoadCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetConstructionOccupyRoadCfg FAILED\n");
    }

    /* 拥堵识别配置回调 */
    if (NET_SERVER_RegisterCb_GetCongestionCfg(MyGetCongestionCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetCongestionCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetCongestionCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetCongestionCfg(MySetCongestionCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetCongestionCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetCongestionCfg FAILED\n");
    }

    /* 车牌识别配置回调 */
    if (NET_SERVER_RegisterCb_GetLicensePlateRecognitionCfg(MyGetLicensePlateRecognitionCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetLicensePlateRecognitionCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetLicensePlateRecognitionCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetLicensePlateRecognitionCfg(MySetLicensePlateRecognitionCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetLicensePlateRecognitionCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetLicensePlateRecognitionCfg FAILED\n");
    }

    /* 高空安全带识别配置回调 */
    if (NET_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg(MyGetHighAltitudeSeatbeltCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetHighAltitudeSeatbeltCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetHighAltitudeSeatbeltCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg(MySetHighAltitudeSeatbeltCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetHighAltitudeSeatbeltCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetHighAltitudeSeatbeltCfg FAILED\n");
    }

    /* 安全帽识别配置回调 */
    if (NET_SERVER_RegisterCb_GetSafetyHelmetCfg(MyGetSafetyHelmetCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetSafetyHelmetCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetSafetyHelmetCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetSafetyHelmetCfg(MySetSafetyHelmetCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetSafetyHelmetCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetSafetyHelmetCfg FAILED\n");
    }

    /* 摔倒识别配置回调 */
    if (NET_SERVER_RegisterCb_GetPersonFallCfg(MyGetPersonFallCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPersonFallCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPersonFallCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetPersonFallCfg(MySetPersonFallCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPersonFallCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPersonFallCfg FAILED\n");
    }

    /* 玩手机识别配置回调 */
    if (NET_SERVER_RegisterCb_GetPhoneUsageCfg(MyGetPhoneUsageCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPhoneUsageCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPhoneUsageCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetPhoneUsageCfg(MySetPhoneUsageCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPhoneUsageCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPhoneUsageCfg FAILED\n");
    }

    /* 抽烟识别配置回调 */
    if (NET_SERVER_RegisterCb_GetSmokingCfg(MyGetSmokingCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetSmokingCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetSmokingCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetSmokingCfg(MySetSmokingCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetSmokingCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetSmokingCfg FAILED\n");
    }

    /* 明火识别配置回调 */
    if (NET_SERVER_RegisterCb_GetOpenFlameCfg(MyGetOpenFlameCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetOpenFlameCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetOpenFlameCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetOpenFlameCfg(MySetOpenFlameCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetOpenFlameCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetOpenFlameCfg FAILED\n");
    }

    /* 黄土裸露识别配置回调 */
    if (NET_SERVER_RegisterCb_GetBareSoilCfg(MyGetBareSoilCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetBareSoilCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetBareSoilCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetBareSoilCfg(MySetBareSoilCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetBareSoilCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetBareSoilCfg FAILED\n");
    }

    /* 洞口防护栏识别配置回调 */
    if (NET_SERVER_RegisterCb_GetHoleProtectionBarCfg(MyGetHoleProtectionBarCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetHoleProtectionBarCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetHoleProtectionBarCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetHoleProtectionBarCfg(MySetHoleProtectionBarCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetHoleProtectionBarCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetHoleProtectionBarCfg FAILED\n");
    }

    /* 反光衣识别配置回调 */
    if (NET_SERVER_RegisterCb_GetReflectiveClothingCfg(MyGetReflectiveClothingCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetReflectiveClothingCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetReflectiveClothingCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetReflectiveClothingCfg(MySetReflectiveClothingCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetReflectiveClothingCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetReflectiveClothingCfg FAILED\n");
    }

    /* 宠物识别配置回调 */
    if (NET_SERVER_RegisterCb_GetPetRecognitionInfo(MyGetPetRecognitionInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPetRecognitionInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPetRecognitionInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetPetRecognitionInfo(MySetPetRecognitionInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPetRecognitionInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPetRecognitionInfo FAILED\n");
    }

    /* 翻越围栏配置回调 */
    if (NET_SERVER_RegisterCb_GetClimbFenceInfo(MyGetClimbFenceInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetClimbFenceInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetClimbFenceInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetClimbFenceInfo(MySetClimbFenceInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetClimbFenceInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetClimbFenceInfo FAILED\n");
    }

    /* 离岗配置回调 */
    if (NET_SERVER_RegisterCb_GetDimissionInfo(MyGetDimissionInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetDimissionInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetDimissionInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetDimissionInfo(MySetDimissionInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetDimissionInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetDimissionInfo FAILED\n");
    }

    /* 违规变道配置回调 */
    if (NET_SERVER_RegisterCb_GetIllegalLaneInfo(MyGetIllegalLaneInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetIllegalLaneInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetIllegalLaneInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetIllegalLaneInfo(MySetIllegalLaneInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetIllegalLaneInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetIllegalLaneInfo FAILED\n");
    }

    /* 逆行配置回调 */
    if (NET_SERVER_RegisterCb_GetRetrogradeInfo(MyGetRetrogradeInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetRetrogradeInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetRetrogradeInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetRetrogradeInfo(MySetRetrogradeInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetRetrogradeInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetRetrogradeInfo FAILED\n");
    }

    /* 非机动车闯入配置回调 */
    if (NET_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo(MyGetNonmotorVehicleIntrusionInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetNonmotorVehicleIntrusionInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetNonmotorVehicleIntrusionInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo(MySetNonmotorVehicleIntrusionInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetNonmotorVehicleIntrusionInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetNonmotorVehicleIntrusionInfo FAILED\n");
    }

    /* 应急车道占用识别配置回调 */
    if (NET_SERVER_RegisterCb_GetOccupationEmergencyInfo(MyGetOccupationEmergencyInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetOccupationEmergencyInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetOccupationEmergencyInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetOccupationEmergencyInfo(MySetOccupationEmergencyInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetOccupationEmergencyInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetOccupationEmergencyInfo FAILED\n");
    }

    /* 行人闯入配置回调 */
    if (NET_SERVER_RegisterCb_GetPedestrianIntrusionInfo(MyGetPedestrianIntrusionInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPedestrianIntrusionInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPedestrianIntrusionInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetPedestrianIntrusionInfo(MySetPedestrianIntrusionInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPedestrianIntrusionInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPedestrianIntrusionInfo FAILED\n");
    }

    /* 烟火识别配置回调 */
    if (NET_SERVER_RegisterCb_GetSmokeFireCfg(MyGetSmokeFireCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetSmokeFireCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetSmokeFireCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetSmokeFireCfg(MySetSmokeFireCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetSmokeFireCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetSmokeFireCfg FAILED\n");
    }

    /* 道路积水检测配置回调 */
    if (NET_SERVER_RegisterCb_GetRoadPondingCfg(MyGetRoadPondingCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetRoadPondingCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetRoadPondingCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetRoadPondingCfg(MySetRoadPondingCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetRoadPondingCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetRoadPondingCfg FAILED\n");
    }

    /* 停车侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetParkingAlarm(MyGetParkingAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetParkingAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetParkingAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetParkingAlarm(MySetParkingAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetParkingAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetParkingAlarm FAILED\n");
    }

    /* 物品遗留侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetUnattendedObjectAlarm(MyGetUnattendedObjectAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetUnattendedObjectAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetUnattendedObjectAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetUnattendedObjectAlarm(MySetUnattendedObjectAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetUnattendedObjectAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetUnattendedObjectAlarm FAILED\n");
    }

    /* 物品拿取侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetObjectRemovalAlarm(MyGetObjectRemovalAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetObjectRemovalAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetObjectRemovalAlarm FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetObjectRemovalAlarm(MySetObjectRemovalAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetObjectRemovalAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetObjectRemovalAlarm FAILED\n");
    }

    /* 音频异常侦测配置回调 */
    if (NET_SERVER_RegisterCb_GetAudioAnomalyAlarm(MyGetAudioAnomalyAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetAudioAnomalyAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetAudioAnomalyAlarm FAILED\n");
    }
    if (NET_SERVER_RegisterCb_SetAudioAnomalyAlarm(MySetAudioAnomalyAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetAudioAnomalyAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetAudioAnomalyAlarm FAILED\n");
    }

    /* 声音报警、报警输入输出、闪光报警灯和 PIR 报警配置回调。 */
    if (NET_SERVER_RegisterCb_GetAudibleAlarmInfo(ConfigDemoGetAudibleAlarmInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetAudibleAlarmInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetAudibleAlarmInfo FAILED\n");
    }
    if (NET_SERVER_RegisterCb_SetAudibleAlarmInfo(ConfigDemoSetAudibleAlarmInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetAudibleAlarmInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetAudibleAlarmInfo FAILED\n");
    }
    if (NET_SERVER_RegisterCb_GetAlarmInputInfo(ConfigDemoGetAlarmInputInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetAlarmInputInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetAlarmInputInfo FAILED\n");
    }
    if (NET_SERVER_RegisterCb_SetAlarmInputInfo(ConfigDemoSetAlarmInputInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetAlarmInputInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetAlarmInputInfo FAILED\n");
    }
    if (NET_SERVER_RegisterCb_GetAlarmOutputInfo(ConfigDemoGetAlarmOutputInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetAlarmOutputInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetAlarmOutputInfo FAILED\n");
    }
    if (NET_SERVER_RegisterCb_SetAlarmOutputInfo(ConfigDemoSetAlarmOutputInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetAlarmOutputInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetAlarmOutputInfo FAILED\n");
    }
    if (NET_SERVER_RegisterCb_GetFlashingLightAlarmInfo(ConfigDemoGetFlashingLightAlarmInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetFlashingLightAlarmInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetFlashingLightAlarmInfo FAILED\n");
    }
    if (NET_SERVER_RegisterCb_SetFlashingLightAlarmInfo(ConfigDemoSetFlashingLightAlarmInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetFlashingLightAlarmInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetFlashingLightAlarmInfo FAILED\n");
    }
    if (NET_SERVER_RegisterCb_GetPirAlarmInfo(ConfigDemoGetPirAlarmInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPirAlarmInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPirAlarmInfo FAILED\n");
    }
    if (NET_SERVER_RegisterCb_SetPirAlarmInfo(ConfigDemoSetPirAlarmInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPirAlarmInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPirAlarmInfo FAILED\n");
    }

    /* 图像配置回调 */
    if (NET_SERVER_RegisterCb_GetImageCfg(MyGetImageCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetImageCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetImageCfg FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetImageCfg(MySetImageCfgCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetImageCfg SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetImageCfg FAILED\n");
    }

    /* 预览信息配置回调 */
    if (NET_SERVER_RegisterCb_GetPreviewInfo(MyGetPreviewInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetPreviewInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetPreviewInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetPreviewInfo(MySetPreviewInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetPreviewInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetPreviewInfo FAILED\n");
    }

    /*通道信息*/
    if (NET_SERVER_RegisterCb_GetChannelInfo(MyGetChannelInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetChannelInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetChannelInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetChannelList(MyGetChannelListCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetChannelList SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetChannelList FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetAudioAnomalyAlarm(MySetAudioAnomalyAlarmCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetAudioAnomalyAlarm SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetAudioAnomalyAlarm FAILED\n");
    }

    /* 系统升级相关回调*/
    if(NET_SERVER_RegisterCb_GetUpgradeStatus(MyGetUpgradeStatus))
    {
        printf("[ConfigServerDemo] RegisterCb_GetUpgradeStatus SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetUpgradeStatus FAILED\n");
    }

    if(NET_SERVER_RegisterCb_GetUpgradeVersion(MyGetUpgradeVersion))
    {
        printf("[ConfigServerDemo] RegisterCb_GetUpgradeVersion SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetUpgradeVersion FAILED\n");
    }

    if(NET_SERVER_RegisterCb_SetUpgrade(MySetUpgrade))
    {
        printf("[ConfigServerDemo] RegisterCb_SetUpgrade SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetUpgrade FAILED\n");
    }

    /* 抓图参数回调 */
    if (NET_SERVER_RegisterCb_GetCaptureParamInfo(MyGetCaptureParamInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetCaptureParamInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetCaptureParamInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetCaptureParamInfo(MySetCaptureParamInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetCaptureParamInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetCaptureParamInfo FAILED\n");
    }

    /* 抓图任务回调 */
    if (NET_SERVER_RegisterCb_GetCapturePlanInfo(MyGetCapturePlanInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetCapturePlanInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetCapturePlanInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetCapturePlanInfo(MySetCapturePlanInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetSapturePlanInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetSapturePlanInfo FAILED\n");
    }

    /* 曝光信息回调 */
    if(NET_SERVER_RegisterCb_GetExposureInfo(MyGetExposureInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetExposureInfo SUCCESS\n");
    }
    else{
        printf("[ConfigServerDemo] RegisterCb_GetExposureInfo FAILED\n");
    }

    if(NET_SERVER_RegisterCb_SetExposureInfo(MySetExposureInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetExposureInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetExposureInfo FAILED\n");
    }

    /* 日夜转换信息回调*/
    if(NET_SERVER_RegisterCb_GetDayNightInfo(MyGetDayNightInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetDayNightInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetDayNightInfo FAILED\n");
    }

    if(NET_SERVER_RegisterCb_SetDayNightInfo(MySetDayNightInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetDayNightInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetDayNightInfo FAILED\n");
    }

    /* 背光信息回调 */
    if(NET_SERVER_RegisterCb_GetBackLightInfo(MyGetBackLightInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetBackLightInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetBackLightInfo FAILED\n");
    }

    if(NET_SERVER_RegisterCb_SetBackLightInfo(MySetBackLightInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetBackLightInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetBackLightInfo FAILED\n");
    }

    /* 降噪信息回调 */
    if(NET_SERVER_RegisterCb_GetDenoiseInfo(MyGetDenoiseInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetDenoiseInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetDenoiseInfo FAILED\n");
    }

    if(NET_SERVER_RegisterCb_SetDenoiseInfo(MySetDenoiseInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetDenoiseInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetDenoiseInfo FAILED\n");
    }

    /* 白平衡回调*/
    if(NET_SERVER_RegisterCb_GetWhiteBalanceInfo(MyGetWhiteBalanceInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_GetWhiteBalanceInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetWhiteBalanceInfo SUCCESS\n");
    }

    if(NET_SERVER_RegisterCb_SetWhiteBalanceInfo(MySetWhiteBalanceInfo))
    {
        printf("[ConfigServerDemo] RegisterCb_SetWhiteBalanceInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetWhiteBalanceInfo SUCCESS\n");
    }

    /* 对讲相关*/
    if (NET_SERVER_RegisterCb_SetTalkbackState(MySetTalkbackStateCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetTalkbackState SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetTalkbackState FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetTalkbackToStream(MySetTalkbackToStreamCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetTalkbackToStream SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetTalkbackToStream FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetTalkbackFromStream(MyGetTalkbackFromStreamCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetTalkbackFromStream SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetTalkbackFromStream FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetReplayUrl(MyGetReplayUrlCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetReplayUrl SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetReplayUrl FAILED\n");
    }

    if (NET_SERVER_RegisterCb_ControlReplay(MyControlReplayCb))
    {
        printf("[ConfigServerDemo] RegisterCb_ControlReplay SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_ControlReplay FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetReplayRecordList(MyGetReplayRecordListCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetReplayRecordList SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetReplayRecordList FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetReplayTalkback(MySetReplayTalkbackCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetReplayTalkback SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetReplayTalkback FAILED\n");
    }

    /* 人脸抓拍配置回调 */
    if (NET_SERVER_RegisterCb_GetFaceCaptureInfo(MyGetFaceCaptureInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetFaceCaptureInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetFaceCaptureInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetFaceCaptureInfo(MySetFaceCaptureInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetFaceCaptureInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetFaceCaptureInfo FAILED\n");
    }

    /* 人脸比对配置回调 */
    if (NET_SERVER_RegisterCb_SetFaceCompareInfo(MySetFaceCompareInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetFaceCompareInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetFaceCompareInfo FAILED\n");
    }

    /* 目标库配置回调 */
    if (NET_SERVER_RegisterCb_AddTargetLib(MyAddTargetLibCb))
    {
        printf("[ConfigServerDemo] RegisterCb_AddTargetLib SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_AddTargetLib FAILED\n");
    }

    if (NET_SERVER_RegisterCb_DelTargetLib(MyDelTargetLibCb))
    {
        printf("[ConfigServerDemo] RegisterCb_DelTargetLib SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_DelTargetLib FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetTargetLib(MySetTargetLibCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetTargetLib SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetTargetLib FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetTargetLib(MyGetTargetLibCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetTargetLib SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetTargetLib FAILED\n");
    }

    /* 人脸信息配置回调 */
    if (NET_SERVER_RegisterCb_AddFaceInfo(MyAddFaceInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_AddFaceInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_AddFaceInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_DelFaceInfo(MyDelFaceInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_DelFaceInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_DelFaceInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_SetFaceInfo(MySetFaceInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_SetFaceInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_SetFaceInfo FAILED\n");
    }

    if (NET_SERVER_RegisterCb_GetFaceInfo(MyGetFaceInfoCb))
    {
        printf("[ConfigServerDemo] RegisterCb_GetFaceInfo SUCCESS\n");
    }
    else
    {
        printf("[ConfigServerDemo] RegisterCb_GetFaceInfo FAILED\n");
    }
}

int main(int argc, char* argv[])
{
    ConfigureByArgs(argc, argv);
    printf("===========================================\n");
    printf("   NetTVSDK Server Config Demo\n");
    printf("   - 支持 NET_GetDevConfig/NET_SetDevConfig\n");
    printf("   - 示例命令：DEVICECFG / NETWORKCFG / OSDCAPCFG / MOTIONALARM / TAMPERALARM / CROSSLINEALARM / INTRUSIONALARM\n");
    printf("===========================================\n");
    printf("Press Ctrl+C to stop.\n\n");

    /* 注册信号处理，支持 Ctrl+C 退出 */
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    /* 初始化日志 */
    initSdkLogBySize("ConfigServerDemo", "/opt/course/ConfigServerDemo.log", MAX_LOG_SIZE, MAX_LOG_FILES);
    syncPrintf(1);
    setLogLevel(NETSDK_LOG_TRACE);

    /* 初始化默认配置 */
    InitDefaultConfig();

    /* 注册业务相关回调 */
    RegisterCallbacks();

    /* 启动 SDK 服务端 */
    printf("[ConfigServerDemo] Starting server on port %d, username=%s ...\n",
           g_serverPort,
           g_serverUsername);
    if (!NET_SERVER_Init(g_serverPort, g_serverUsername, g_serverPassword))
    {
        printf("[ConfigServerDemo] NET_SERVER_Init FAILED!\n");
        return -1;
    }

    printf("[ConfigServerDemo] Server started successfully.\n");
    g_voiceComDumpFp = fopen(DEMO_VOICECOM_SERVER_RECV_DUMP, "wb");
    if (!g_voiceComDumpFp)
    {
        printf("[ConfigServerDemo][VoiceCom] open recv dump failed, continue without dump: %s\n",
               DEMO_VOICECOM_SERVER_RECV_DUMP);
    }
    if (NET_SERVER_RegisterCb_VoiceComPlay(MyVoiceComPlayCb) &&
        NET_SERVER_RegisterCb_VoiceComCapture(MyVoiceComCaptureCb, NULL) &&
        NET_SERVER_StartVoiceComServer(DEMO_VOICECOM_PORT))
    {
        printf("[ConfigServerDemo][VoiceCom] Server started on port %d.\n", DEMO_VOICECOM_PORT);
        printf("[ConfigServerDemo][VoiceCom] Received audio dump: %s\n",
               DEMO_VOICECOM_SERVER_RECV_DUMP);
    }
    else
    {
        printf("[ConfigServerDemo][VoiceCom] Server start failed.\n");
    }
    printf("[ConfigServerDemo] Waiting for client config requests...\n");

    while (g_running)
    {
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    printf("[ConfigServerDemo] Cleaning up server...\n");
    NET_SERVER_StopVoiceComServer();
    if (g_voiceComDumpFp)
    {
        fclose(g_voiceComDumpFp);
        g_voiceComDumpFp = NULL;
    }
    NET_SERVER_Cleanup();
    printf("[ConfigServerDemo] Bye.\n");

    return 0;
}

