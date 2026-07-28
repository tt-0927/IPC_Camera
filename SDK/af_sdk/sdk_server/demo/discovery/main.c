/**
 * @file main.c
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief SDK 设备发现 Demo
 * 功能说明：
 * 1. 实现 main 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
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

#define NETSDK_DEMO_LOG_MAX_SIZE  (20 * 1024 * 1024)  /* 20MB */
#define NETSDK_DEMO_LOG_MAX_FILES (10)

static volatile sig_atomic_t g_running = 1;
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 signal_handler 定义的内部处理。
 * @param [in] signum 函数处理参数。
 * @return 无返回值。
 */

static void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n[discovery-server] received signal %d, stopping...\n", signum);
        g_running = 0;
    }
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设备发现信息回调 — 填充本机 NVR 信息
 */
static void NET_TV_STDCALL FillDeviceInfo(NET_DiscoveryDeviceInfo_S* pInfo)
{
    if (!pInfo) return;

    memset(pInfo, 0, sizeof(*pInfo));

    /* 以下字段按实际设备信息填写 */
    snprintf(pInfo->strDeviceName,      sizeof(pInfo->strDeviceName),      "NVR-Demo");
    snprintf(pInfo->strDeviceID,        sizeof(pInfo->strDeviceID),        "SN-20240601-0001");
    snprintf(pInfo->strDeviceType,      sizeof(pInfo->strDeviceType),      "NVR");
    snprintf(pInfo->strIPv4Address,     sizeof(pInfo->strIPv4Address),     "172.16.25.100");
    snprintf(pInfo->strIPv4SubnetMask,  sizeof(pInfo->strIPv4SubnetMask),  "255.255.255.0");
    snprintf(pInfo->strIPv4Gateway,     sizeof(pInfo->strIPv4Gateway),     "172.16.25.1");
    snprintf(pInfo->strMACAddress,      sizeof(pInfo->strMACAddress),      "aa:bb:cc:dd:ee:ff");
    snprintf(pInfo->strFirmwareVersion, sizeof(pInfo->strFirmwareVersion), "V1.0.0");
    pInfo->uHttpPort = 80;
    snprintf(pInfo->strManufacturer,    sizeof(pInfo->strManufacturer),    "ITC");

    printf("[discovery-server] device info filled: name[%s] id[%s] ip[%s]\n",
           pInfo->strDeviceName, pInfo->strDeviceID, pInfo->strIPv4Address);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 运行当前 Demo 的主流程。
 * @param [in] argc 函数处理参数。
 * @param [in,out] argv 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

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
    initSdkLogBySize(".", "discovery_server", NETSDK_DEMO_LOG_MAX_SIZE, NETSDK_DEMO_LOG_MAX_FILES);
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
