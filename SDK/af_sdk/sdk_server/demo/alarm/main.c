/**
 * @file alarm_demo.c
 * @author tianl (tianl@kfb.cn)
 * @date 2026-1-21
 *
 * @brief 模拟报警事件推送Demo
 * 功能说明：
 * 1. 初始化SDK服务器
 * 2. 定时模拟推送不同类型的报警事件
 * 3. 演示如何使用NET_SERVER_PushAlarmInfo推送报警信息
 */

/* 日志记录单个日志文件的最大大小 */
#define MAX_LOG_SIZE  (20 * 1024 * 1024) // 20MB
/* 日志记录最大保留的日志文件数量 */
#define MAX_LOG_FILES (10)

#define SDKSERVER_PORT 9888
#define ALARM_PUSH_INTERVAL 10  // 每10秒推送一次报警事件

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "NetSdkLog.h"
#include "NetTVSDKServerInterface.h"

// 全局运行标志
volatile sig_atomic_t g_running = 1;

// 信号处理函数，用于优雅退出
void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        printf("\nReceived signal %d. Stopping alarm push demo...\n", signum);
        g_running = 0;
    }
}

// 初始化报警信息结构体
void InitAlarmInfo(NET_Alarmer_S* pAlarmInfo, const char* deviceName,
                   const char* deviceIP, const char* serialNumber,
                   const BYTE* macAddr)
{
    memset(pAlarmInfo, 0, sizeof(NET_Alarmer_S));

    // 设置设备名称
    if (deviceName)
    {
        strncpy(pAlarmInfo->strDeviceName, deviceName, NET_LEN_32 - 1);
        pAlarmInfo->strDeviceName[NET_LEN_32 - 1] = '\0';
    }

    // 设置设备IP
    if (deviceIP)
    {
        strncpy(pAlarmInfo->strDeviceIP, deviceIP, 127);
        pAlarmInfo->strDeviceIP[127] = '\0';
    }

    // 设置序列号
    if (serialNumber)
    {
        memcpy(pAlarmInfo->strSerialNumber, serialNumber,
               strlen(serialNumber) < NET_LEN_64 ? strlen(serialNumber) : NET_LEN_64);
    }

    // 设置MAC地址
    if (macAddr)
    {
        memcpy(pAlarmInfo->byMacAddr, macAddr, NET_LEN_6);
    }
    else
    {
        // 默认MAC地址: 00:11:22:33:44:55
        pAlarmInfo->byMacAddr[0] = 0x00;
        pAlarmInfo->byMacAddr[1] = 0x11;
        pAlarmInfo->byMacAddr[2] = 0x22;
        pAlarmInfo->byMacAddr[3] = 0x33;
        pAlarmInfo->byMacAddr[4] = 0x44;
        pAlarmInfo->byMacAddr[5] = 0x55;
    }
}

// 模拟不同类型的报警事件
void PushMotionDetectAlarm()
{
    NET_Alarmer_S stAlarmInfo;
    NET_AlarmBasicInfo_S stBasic = {0};

    printf("[Demo] Pushing Motion Detection Alarm...\n");

    InitAlarmInfo(&stAlarmInfo, "Camera-01", "192.168.1.100",
                  "SN202312120001", NULL);

    stBasic.uAlarmType = NET_ALARM_MOTION_DETECT;
    stBasic.uAlarmInputNumber = 1;

    if (NET_SERVER_PushAlarmInfo(&stAlarmInfo, NET_ALARM_BASE_BASIC, &stBasic, (INT32)sizeof(stBasic)))
    {
        printf("[Demo] Motion Detection Alarm pushed successfully!\n");
    }
    else
    {
        printf("[Demo] Failed to push Motion Detection Alarm!\n");
    }
}

void PushIntrusionAlarm()
{
    NET_Alarmer_S stAlarmInfo;
    NET_AlarmRuleInfo_S* pRule = (NET_AlarmRuleInfo_S*)calloc(1, sizeof(NET_AlarmRuleInfo_S));
    if (!pRule)
    {
        printf("[Demo] Failed to allocate intrusion alarm info!\n");
        return;
    }

    printf("[Demo] Pushing Intrusion Alarm...\n");

    BYTE macAddr[NET_LEN_6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    InitAlarmInfo(&stAlarmInfo, "Camera-02", "192.168.1.101",
                  "SN202312120002", macAddr);

    pRule->uAlarmType = NET_ALARM_INTRUSION;
    pRule->uRuleID = 1001;
    strncpy(pRule->strRuleName, "IntrusionRule-1", sizeof(pRule->strRuleName) - 1);
    pRule->strRuleName[sizeof(pRule->strRuleName) - 1] = '\0';

    if (NET_SERVER_PushAlarmInfo(&stAlarmInfo, NET_ALARM_BASE_RULE, pRule, (INT32)sizeof(*pRule)))
    {
        printf("[Demo] Intrusion Alarm pushed successfully!\n");
    }
    else
    {
        printf("[Demo] Failed to push Intrusion Alarm!\n");
    }

    free(pRule);
}

void PushVideoLossAlarm()
{
    NET_Alarmer_S stAlarmInfo;
    NET_AlarmExceptionInfo_S stEx = {0};

    printf("[Demo] Pushing Video Loss Alarm...\n");

    InitAlarmInfo(&stAlarmInfo, "Camera-03", "192.168.1.102",
                  "SN202312120003", NULL);

    stEx.uAlarmType = NET_ALARM_VIDEO_LOSS;
    stEx.uChannel = 1;
    stEx.uDiskNo = 0;
    stEx.uStatus = 1; // 1=触发

    if (NET_SERVER_PushAlarmInfo(&stAlarmInfo, NET_ALARM_BASE_EXCEPTION, &stEx, (INT32)sizeof(stEx)))
    {
        printf("[Demo] Video Loss Alarm pushed successfully!\n");
    }
    else
    {
        printf("[Demo] Failed to push Video Loss Alarm!\n");
    }
}

void PushDiskFullAlarm()
{
    NET_Alarmer_S stAlarmInfo;
    NET_AlarmExceptionInfo_S stEx = {0};

    printf("[Demo] Pushing Disk Full Alarm...\n");

    BYTE macAddr[NET_LEN_6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    InitAlarmInfo(&stAlarmInfo, "NVR-01", "192.168.1.200",
                  "SN202312120100", macAddr);

    stEx.uAlarmType = NET_ALARM_DISK_FULL;
    stEx.uChannel = 0;
    stEx.uDiskNo = 1;
    stEx.uStatus = 1; // 1=触发

    if (NET_SERVER_PushAlarmInfo(&stAlarmInfo, NET_ALARM_BASE_EXCEPTION, &stEx, (INT32)sizeof(stEx)))
    {
        printf("[Demo] Disk Full Alarm pushed successfully!\n");
    }
    else
    {
        printf("[Demo] Failed to push Disk Full Alarm!\n");
    }
}

void PushPeopleFlowStatisticsAlarm()
{
    NET_Alarmer_S stAlarmInfo;
    static NET_AlarmStatisticsInfo_S stStat;

    printf("[Demo] Pushing People Flow Statistics Alarm...\n");

    memset(&stStat, 0, sizeof(stStat));
    InitAlarmInfo(&stAlarmInfo, "Camera-04", "192.168.1.103",
                  "SN202312120004", NULL);

    stStat.uAlarmType = NET_ALARM_PEOPLE_FLOW_STATISTICS;
    stStat.uChannel = 1;
    stStat.uStatisticsType = NET_STATISTICS_TYPE_PEOPLE_FLOW;
    stStat.uRuleID = 1;
    stStat.llTimestampMs = (INT64)time(NULL) * 1000;
    stStat.uReportSeq = 1;
    stStat.uEnterCount = 12;
    stStat.uLeaveCount = 5;
    stStat.uTotalCount = stStat.uEnterCount + stStat.uLeaveCount;
    stStat.uCurrentPeopleCount = 7;
    stStat.uAverageStayTimeSec = 5;
    stStat.uTargetCount = 1;
    stStat.stTargets[0].nTrackID = 1001;
    stStat.stTargets[0].uRuleID = stStat.uRuleID;
    stStat.stTargets[0].uSnapshotType = 1;
    stStat.stTargets[0].nLeft = 120;
    stStat.stTargets[0].nTop = 80;
    stStat.stTargets[0].nRight = 260;
    stStat.stTargets[0].nBottom = 360;
    stStat.stTargets[0].llTimestampMs = stStat.llTimestampMs;
    stStat.stTargets[0].nDirection = 1;

    if (NET_SERVER_PushAlarmInfo(&stAlarmInfo, NET_ALARM_PEOPLE_FLOW_STATISTICS, &stStat, (INT32)sizeof(stStat)))
    {
        printf("[Demo] People Flow Statistics Alarm pushed successfully!\n");
    }
    else
    {
        printf("[Demo] Failed to push People Flow Statistics Alarm!\n");
    }
}

void PushFaceCompareAlarm()
{
    NET_Alarmer_S stAlarmInfo;
    NET_AlarmFaceCompareInfo_S stCompare = {0};
    static const BYTE demoJpeg[] = {
        0xFF, 0xD8, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x08,
        0x06, 0x06, 0x07, 0x06, 0x05, 0x08, 0x07, 0x07,
        0x07, 0x09, 0x09, 0x08, 0x0A, 0x0C, 0x14, 0x0D,
        0x0C, 0x0B, 0x0B, 0x0C, 0x19, 0x12, 0x13, 0x0F,
        0x14, 0x1D, 0x1A, 0x1F, 0x1E, 0x1D, 0x1A, 0x1C,
        0x1C, 0x20, 0x24, 0x2E, 0x27, 0x20, 0x22, 0x2C,
        0x23, 0x1C, 0x1C, 0x28, 0x37, 0x29, 0x2C, 0x30,
        0x31, 0x34, 0x34, 0x34, 0x1F, 0x27, 0x39, 0x3D,
        0x38, 0x32, 0x3C, 0x2E, 0x33, 0x34, 0x32, 0xFF,
        0xC0, 0x00, 0x0B, 0x08, 0x00, 0x01, 0x00, 0x01,
        0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0x14,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0xFF, 0xC4, 0x00, 0x14, 0x10, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00,
        0x3F, 0x00, 0x37, 0xFF, 0xD9
    };

    printf("[Demo] Pushing Face Compare Alarm...\n");

    InitAlarmInfo(&stAlarmInfo, "Camera-05", "192.168.1.104",
                  "SN202312120005", NULL);

    stCompare.uAlarmType = NET_ALARM_FACE_COMPARE;
    stCompare.uChannel = 1;
    stCompare.llTimestampMs = (INT64)time(NULL) * 1000;
    stCompare.nEventId = 1001;
    stCompare.nCompResult = 1;
    stCompare.nFaceId = 10001;
    stCompare.nSimilarity = 88;
    strncpy(stCompare.strFaceName, "FaceCompareDemo", sizeof(stCompare.strFaceName) - 1);
    stCompare.strFaceName[sizeof(stCompare.strFaceName) - 1] = '\0';
    strncpy(stCompare.strFaceLibName, "DemoFaceLib", sizeof(stCompare.strFaceLibName) - 1);
    stCompare.strFaceLibName[sizeof(stCompare.strFaceLibName) - 1] = '\0';
    strncpy(stCompare.strLibFacePath, "/demo/face/lib/10001.jpg", sizeof(stCompare.strLibFacePath) - 1);
    stCompare.strLibFacePath[sizeof(stCompare.strLibFacePath) - 1] = '\0';
    strncpy(stCompare.strCapFacePath, "/demo/face/capture/10001.jpg", sizeof(stCompare.strCapFacePath) - 1);
    stCompare.strCapFacePath[sizeof(stCompare.strCapFacePath) - 1] = '\0';
    strncpy(stCompare.strCapImagePath, "/demo/face/capture/panorama.jpg", sizeof(stCompare.strCapImagePath) - 1);
    stCompare.strCapImagePath[sizeof(stCompare.strCapImagePath) - 1] = '\0';
    memcpy(stCompare.byLibFaceImg, demoJpeg, sizeof(demoJpeg));
    stCompare.uLibFaceImgLen = (UINT32)sizeof(demoJpeg);
    memcpy(stCompare.byCapFaceImg, demoJpeg, sizeof(demoJpeg));
    stCompare.uCapFaceImgLen = (UINT32)sizeof(demoJpeg);

    if (NET_SERVER_PushAlarmInfo(&stAlarmInfo, NET_ALARM_FACE_COMPARE, &stCompare, (INT32)sizeof(stCompare)))
    {
        printf("[Demo] Face Compare Alarm pushed successfully! faceId=%d name=%s lib=%s result=%d similarity=%d\n",
               stCompare.nFaceId,
               stCompare.strFaceName,
               stCompare.strFaceLibName,
               stCompare.nCompResult,
               stCompare.nSimilarity);
    }
    else
    {
        printf("[Demo] Failed to push Face Compare Alarm!\n");
    }
}

// 模拟随机报警事件
void PushRandomAlarm()
{
    static int alarmCounter = 0;
    alarmCounter++;

    // 根据计数器选择不同类型的报警
    switch (alarmCounter % 6)
    {
        case 0:
            PushMotionDetectAlarm();
            break;
        case 1:
            PushIntrusionAlarm();
            break;
        case 2:
            PushVideoLossAlarm();
            break;
        case 3:
            PushDiskFullAlarm();
            break;
        case 4:
            PushPeopleFlowStatisticsAlarm();
            break;
        case 5:
            PushFaceCompareAlarm();
            break;
        default:
            PushMotionDetectAlarm();
            break;
    }
}

// 回调函数：获取设备信息
NET_COMMON_ECODE_E MyDeviceInfoCb(pNET_DeviceInfo_S pInfo)
{
    if (!pInfo)
    {
        return NET_E_FAILED;
    }

    // 示例：填充最基本的设备信息（字段见 NET_DeviceInfo_S）
    pInfo->uDevType = 0;
    pInfo->uAlarmInPortNum = 2;
    pInfo->uAlarmOutPortNum = 1;
    pInfo->uChannelNum = 4;
    return NET_E_SUCCEED;
}

// 注册回调函数
void AddRegisterCb()
{
    NET_SERVER_RegisterCb_GetDeviceInfo(MyDeviceInfoCb);
}

int main(int argc, char* argv[])
{
    printf("===========================================\n");
    printf("   NetTVSDK Server Alarm Push Demo\n");
    printf("===========================================\n");
    printf("This demo will simulate pushing alarm events\n");
    printf("Press Ctrl+C to stop\n");
    printf("===========================================\n\n");

    // 注册信号处理函数
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 初始化日志
    initSdkLogBySize("AlarmDemo", "/opt/course/AlarmDemo.log", MAX_LOG_SIZE, MAX_LOG_FILES);

    // 设置日志输出同步输出控制台
    syncPrintf(true);

    // 设置日志等级
    setLogLevel(NETSDK_LOG_TRACE);

    // 注册回调函数
    AddRegisterCb();

    // 获取端口号（从命令行参数或使用默认值）
    UINT32 dwPort = SDKSERVER_PORT;
    if (argc > 1)
    {
        dwPort = (UINT32)atoi(argv[1]);
    }

    // 获取用户名和密码（可选）
    CHAR szUserName[NET_LEN_132] = "admin";
    CHAR szPassword[NET_LEN_132] = "Admin@123456";

    if (argc > 2)
    {
        strncpy(szUserName, argv[2], NET_LEN_132 - 1);
        szUserName[NET_LEN_132 - 1] = '\0';
    }

    if (argc > 3)
    {
        strncpy(szPassword, argv[3], NET_LEN_132 - 1);
        szPassword[NET_LEN_132 - 1] = '\0';
    }

    // 初始化SDK服务器
    printf("[Demo] Initializing SDK Server on port %u...\n", dwPort);
    if (!NET_SERVER_Init(dwPort, szUserName, szPassword))
    {
        printf("[Demo] Failed to initialize SDK Server!\n");
        return -1;
    }
    printf("[Demo] SDK Server initialized successfully!\n");
    printf("[Demo] Server is ready to push alarm events\n\n");

    // 主循环：定时推送报警事件
    int pushCount = 0;
    time_t lastPushTime = time(NULL);

    while (g_running)
    {
        time_t currentTime = time(NULL);

        // 每ALARM_PUSH_INTERVAL秒推送一次报警
        if (currentTime - lastPushTime >= ALARM_PUSH_INTERVAL)
        {
            pushCount++;
            printf("\n[Demo] ==== Alarm Push #%d (Time: %s) ====\n",
                   pushCount, ctime(&currentTime));

            // 推送随机报警事件
            PushRandomAlarm();

            lastPushTime = currentTime;
            printf("[Demo] Next alarm will be pushed in %d seconds...\n\n",
                   ALARM_PUSH_INTERVAL);
        }

        // 休眠1秒，避免CPU占用过高
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    // 清理资源
    printf("\n[Demo] Cleaning up...\n");
    NET_SERVER_Cleanup();
    printf("[Demo] Demo stopped. Total alarms pushed: %d\n", pushCount);

    return 0;
}
