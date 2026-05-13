/**
 * @file alarm_demo.c
 * @author tianl (tianl@kfb.cn)
 * @date 2026-1-21
 * 
 * @brief 模拟报警事件推送Demo
 * 功能说明：
 * 1. 初始化SDK服务器
 * 2. 定时模拟推送不同类型的报警事件
 * 3. 演示如何使用NET_TV_SERVER_PushAlarmInfo推送报警信息
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
void InitAlarmInfo(NET_TV_ALARMER_S* pAlarmInfo, const char* deviceName, 
                   const char* deviceIP, const char* serialNumber, 
                   const BYTE* macAddr)
{
    memset(pAlarmInfo, 0, sizeof(NET_TV_ALARMER_S));
    
    // 设置设备名称
    if (deviceName)
    {
        strncpy(pAlarmInfo->szDeviceName, deviceName, NET_TV_LEN_32 - 1);
        pAlarmInfo->szDeviceName[NET_TV_LEN_32 - 1] = '\0';
    }
    
    // 设置设备IP
    if (deviceIP)
    {
        strncpy(pAlarmInfo->szDeviceIP, deviceIP, 127);
        pAlarmInfo->szDeviceIP[127] = '\0';
    }
    
    // 设置序列号
    if (serialNumber)
    {
        memcpy(pAlarmInfo->szSerialNumber, serialNumber, 
               strlen(serialNumber) < NET_TV_LEN_64 ? strlen(serialNumber) : NET_TV_LEN_64);
    }
    
    // 设置MAC地址
    if (macAddr)
    {
        memcpy(pAlarmInfo->byMacAddr, macAddr, NET_TV_LEN_6);
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
    NET_TV_ALARMER_S stAlarmInfo;
    NET_TV_ALARM_BASIC_INFO_S stBasic = {0};
    
    printf("[Demo] Pushing Motion Detection Alarm...\n");
    
    InitAlarmInfo(&stAlarmInfo, "Camera-01", "192.168.1.100", 
                  "SN202312120001", NULL);

    stBasic.dwAlarmType = NET_TV_ALARM_MOTION_DETECT;
    stBasic.dwAlarmInputNumber = 1;
    
    if (NET_TV_SERVER_PushAlarmInfo(&stAlarmInfo, NET_TV_ALARM_BASE_BASIC, &stBasic, (INT32)sizeof(stBasic)))
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
    NET_TV_ALARMER_S stAlarmInfo;
    NET_TV_ALARM_RULE_INFO_S stRule = {0};
    
    printf("[Demo] Pushing Intrusion Alarm...\n");
    
    BYTE macAddr[NET_TV_LEN_6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    InitAlarmInfo(&stAlarmInfo, "Camera-02", "192.168.1.101", 
                  "SN202312120002", macAddr);

    stRule.dwAlarmType = NET_TV_ALARM_INTRUSION;
    stRule.dwRuleID = 1001;
    strncpy(stRule.szRuleName, "IntrusionRule-1", sizeof(stRule.szRuleName) - 1);
    stRule.szRuleName[sizeof(stRule.szRuleName) - 1] = '\0';
    
    if (NET_TV_SERVER_PushAlarmInfo(&stAlarmInfo, NET_TV_ALARM_BASE_RULE, &stRule, (INT32)sizeof(stRule)))
    {
        printf("[Demo] Intrusion Alarm pushed successfully!\n");
    }
    else
    {
        printf("[Demo] Failed to push Intrusion Alarm!\n");
    }
}

void PushVideoLossAlarm()
{
    NET_TV_ALARMER_S stAlarmInfo;
    NET_TV_ALARM_EXCEPTION_INFO_S stEx = {0};
    
    printf("[Demo] Pushing Video Loss Alarm...\n");
    
    InitAlarmInfo(&stAlarmInfo, "Camera-03", "192.168.1.102", 
                  "SN202312120003", NULL);

    stEx.dwAlarmType = NET_TV_ALARM_VIDEO_LOSS;
    stEx.dwChannel = 1;
    stEx.dwDiskNo = 0;
    stEx.dwStatus = 1; // 1=触发
    
    if (NET_TV_SERVER_PushAlarmInfo(&stAlarmInfo, NET_TV_ALARM_BASE_EXCEPTION, &stEx, (INT32)sizeof(stEx)))
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
    NET_TV_ALARMER_S stAlarmInfo;
    NET_TV_ALARM_EXCEPTION_INFO_S stEx = {0};
    
    printf("[Demo] Pushing Disk Full Alarm...\n");
    
    BYTE macAddr[NET_TV_LEN_6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    InitAlarmInfo(&stAlarmInfo, "NVR-01", "192.168.1.200", 
                  "SN202312120100", macAddr);

    stEx.dwAlarmType = NET_TV_ALARM_DISK_FULL;
    stEx.dwChannel = 0;
    stEx.dwDiskNo = 1;
    stEx.dwStatus = 1; // 1=触发
    
    if (NET_TV_SERVER_PushAlarmInfo(&stAlarmInfo, NET_TV_ALARM_BASE_EXCEPTION, &stEx, (INT32)sizeof(stEx)))
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
    NET_TV_ALARMER_S stAlarmInfo;
    static NET_TV_ALARM_STATISTICS_INFO_S stStat;

    printf("[Demo] Pushing People Flow Statistics Alarm...\n");

    memset(&stStat, 0, sizeof(stStat));
    InitAlarmInfo(&stAlarmInfo, "Camera-04", "192.168.1.103",
                  "SN202312120004", NULL);

    stStat.dwAlarmType = NET_TV_ALARM_PEOPLE_FLOW_STATISTICS;
    stStat.dwChannel = 1;
    stStat.dwStatisticsType = NET_TV_STATISTICS_TYPE_PEOPLE_FLOW;
    stStat.dwRuleID = 1;
    stStat.llTimestampMs = (INT64)time(NULL) * 1000;
    stStat.dwReportSeq = 1;
    stStat.dwEnterCount = 12;
    stStat.dwLeaveCount = 5;
    stStat.dwTotalCount = stStat.dwEnterCount + stStat.dwLeaveCount;
    stStat.dwCurrentPeopleCount = 7;
    stStat.dwAverageStayTimeSec = 5;
    stStat.dwTargetCount = 1;
    stStat.stTargets[0].nTrackID = 1001;
    stStat.stTargets[0].dwRuleID = stStat.dwRuleID;
    stStat.stTargets[0].dwSnapshotType = 1;
    stStat.stTargets[0].nLeft = 120;
    stStat.stTargets[0].nTop = 80;
    stStat.stTargets[0].nRight = 260;
    stStat.stTargets[0].nBottom = 360;
    stStat.stTargets[0].llTimestampMs = stStat.llTimestampMs;
    stStat.stTargets[0].nDirection = 1;

    if (NET_TV_SERVER_PushAlarmInfo(&stAlarmInfo, NET_TV_ALARM_PEOPLE_FLOW_STATISTICS, &stStat, (INT32)sizeof(stStat)))
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
    NET_TV_ALARMER_S stAlarmInfo;
    NET_TV_ALARM_FACE_COMPARE_INFO_S stCompare = {0};
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

    stCompare.dwAlarmType = NET_TV_ALARM_FACE_COMPARE;
    stCompare.dwChannel = 1;
    stCompare.llTimestampMs = (INT64)time(NULL) * 1000;
    stCompare.nEventId = 1001;
    stCompare.nCompResult = 1;
    stCompare.nFaceId = 10001;
    stCompare.nSimilarity = 88;
    strncpy(stCompare.szFaceName, "FaceCompareDemo", sizeof(stCompare.szFaceName) - 1);
    stCompare.szFaceName[sizeof(stCompare.szFaceName) - 1] = '\0';
    strncpy(stCompare.szFaceLibName, "DemoFaceLib", sizeof(stCompare.szFaceLibName) - 1);
    stCompare.szFaceLibName[sizeof(stCompare.szFaceLibName) - 1] = '\0';
    strncpy(stCompare.szLibFacePath, "/demo/face/lib/10001.jpg", sizeof(stCompare.szLibFacePath) - 1);
    stCompare.szLibFacePath[sizeof(stCompare.szLibFacePath) - 1] = '\0';
    strncpy(stCompare.szCapFacePath, "/demo/face/capture/10001.jpg", sizeof(stCompare.szCapFacePath) - 1);
    stCompare.szCapFacePath[sizeof(stCompare.szCapFacePath) - 1] = '\0';
    strncpy(stCompare.szCapImagePath, "/demo/face/capture/panorama.jpg", sizeof(stCompare.szCapImagePath) - 1);
    stCompare.szCapImagePath[sizeof(stCompare.szCapImagePath) - 1] = '\0';
    memcpy(stCompare.byLibFaceImg, demoJpeg, sizeof(demoJpeg));
    stCompare.dwLibFaceImgLen = (UINT32)sizeof(demoJpeg);
    memcpy(stCompare.byCapFaceImg, demoJpeg, sizeof(demoJpeg));
    stCompare.dwCapFaceImgLen = (UINT32)sizeof(demoJpeg);

    if (NET_TV_SERVER_PushAlarmInfo(&stAlarmInfo, NET_TV_ALARM_FACE_COMPARE, &stCompare, (INT32)sizeof(stCompare)))
    {
        printf("[Demo] Face Compare Alarm pushed successfully! faceId=%d name=%s lib=%s result=%d similarity=%d\n",
               stCompare.nFaceId,
               stCompare.szFaceName,
               stCompare.szFaceLibName,
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
NET_TV_COMMON_ECODE_E MyDeviceInfoCb(LPNET_TV_DEVICE_INFO_S pInfo) 
{
    if (!pInfo)
    {
        return NET_TV_E_FAILED;
    }

    // 示例：填充最基本的设备信息（字段见 NET_TV_DEVICE_INFO_S）
    pInfo->dwDevType = 0;
    pInfo->wAlarmInPortNum = 2;
    pInfo->wAlarmOutPortNum = 1;
    pInfo->dwChannelNum = 4;
    return NET_TV_E_SUCCEED;
}

// 注册回调函数
void AddRegisterCb()
{
    NET_TV_SERVER_RegisterCb_GetDeviceInfo(MyDeviceInfoCb);
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
    CHAR szUserName[NET_TV_LEN_132] = "admin";
    CHAR szPassword[NET_TV_LEN_132] = "Admin@123456";
    
    if (argc > 2)
    {
        strncpy(szUserName, argv[2], NET_TV_LEN_132 - 1);
        szUserName[NET_TV_LEN_132 - 1] = '\0';
    }
    
    if (argc > 3)
    {
        strncpy(szPassword, argv[3], NET_TV_LEN_132 - 1);
        szPassword[NET_TV_LEN_132 - 1] = '\0';
    }
    
    // 初始化SDK服务器
    printf("[Demo] Initializing SDK Server on port %u...\n", dwPort);
    if (!NET_TV_SERVER_Init(dwPort, szUserName, szPassword))
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
    NET_TV_SERVER_Cleanup();
    printf("[Demo] Demo stopped. Total alarms pushed: %d\n", pushCount);
    
    return 0;
}
