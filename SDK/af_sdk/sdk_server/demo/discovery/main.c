/**
 * @file discovery_server_demo.c
 * @brief 设备发现服务端 Demo
 *
 * 功能:
 *   1. 注册设备信息回调，填充 NVR 本机信息
 *   2. 启动 DiscoveryResponder，监听 AF_PACKET + 标准 UDP 双路回包
 *   3. Ctrl+C 优雅退出
 *
 * 用法:
 *   ./DiscoveryServerDemo [interface]
 *   默认网卡: eth0
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "NetSdkLog.h"
#include "NetTVSDKServerInterface.h"

#define MAX_LOG_SIZE  (20 * 1024 * 1024)  /* 20MB */
#define MAX_LOG_FILES (10)

static volatile sig_atomic_t g_running = 1;

static void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n[discovery-server] received signal %d, stopping...\n", signum);
        g_running = 0;
    }
}

/**
 * @brief 设备发现信息回调 — 填充本机 NVR 信息
 */
static void STDCALL FillDeviceInfo(NET_TV_DISCOVERY_DEVICE_INFO_S* pInfo)
{
    if (!pInfo) return;

    memset(pInfo, 0, sizeof(*pInfo));

    /* 以下字段按实际设备信息填写 */
    snprintf(pInfo->szDeviceName,      sizeof(pInfo->szDeviceName),      "NVR-Demo");
    snprintf(pInfo->szDeviceID,        sizeof(pInfo->szDeviceID),        "SN-20240601-0001");
    snprintf(pInfo->szDeviceType,      sizeof(pInfo->szDeviceType),      "NVR");
    snprintf(pInfo->szIPv4Address,     sizeof(pInfo->szIPv4Address),     "172.16.25.100");
    snprintf(pInfo->szIPv4SubnetMask,  sizeof(pInfo->szIPv4SubnetMask),  "255.255.255.0");
    snprintf(pInfo->szIPv4Gateway,     sizeof(pInfo->szIPv4Gateway),     "172.16.25.1");
    snprintf(pInfo->szMACAddress,      sizeof(pInfo->szMACAddress),      "aa:bb:cc:dd:ee:ff");
    snprintf(pInfo->szFirmwareVersion, sizeof(pInfo->szFirmwareVersion), "V1.0.0");
    pInfo->dwHttpPort = 80;
    snprintf(pInfo->szManufacturer,    sizeof(pInfo->szManufacturer),    "ITC");

    printf("[discovery-server] device info filled: name[%s] id[%s] ip[%s]\n",
           pInfo->szDeviceName, pInfo->szDeviceID, pInfo->szIPv4Address);
}

int main(int argc, char* argv[])
{
    const char* iface = (argc > 1) ? argv[1] : "eth0";

    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    printf("╔══════════════════════════════════════╗\n");
    printf("║   Discovery Server Demo             ║\n");
    printf("║   interface: %-22s ║\n", iface);
    printf("╚══════════════════════════════════════╝\n\n");

    /* 初始化日志 */
    initSdkLogBySize(".", "discovery_server", MAX_LOG_SIZE, MAX_LOG_FILES);
    setLogLevel(NETSDK_LOG_INFO);
    syncPrintf(1);

    /* 注册设备发现信息回调（启动前必须调用） */
    if (!NET_TV_SERVER_RegisterCb_GetDiscoveryDeviceInfo(FillDeviceInfo)) {
        printf("[discovery-server] FATAL: register callback failed\n");
        return -1;
    }
    printf("[discovery-server] callback registered\n");

    /* 启动设备发现响应服务 */
    if (!NET_TV_SERVER_Discovery_Start(iface)) {
        printf("[discovery-server] FATAL: start failed on iface[%s]\n", iface);
        return -1;
    }
    printf("[discovery-server] started on iface[%s], waiting for probes...\n\n", iface);

    /* 等待退出信号 */
    while (g_running) {
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    /* 停止 */
    NET_TV_SERVER_Discovery_Stop();
    printf("[discovery-server] stopped\n");

    NET_TV_SERVER_Cleanup();
    printf("[discovery-server] cleanup done, exiting\n");
    return 0;
}
