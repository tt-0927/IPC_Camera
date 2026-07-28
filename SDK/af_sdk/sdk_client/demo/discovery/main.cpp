/**
 * @file main.cpp
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


#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "NetTVSDKClientInterface.h"

#define NETSDK_DEMO_MAX_DEVICES 64
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 运行当前 Demo 的主流程。
 * @param [in] argc 函数处理参数。
 * @param [in,out] argv 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int main(int argc, char* argv[])
{
    const char* iface_ip  = (argc > 1) ? argv[1] : NULL;
    int timeout_ms        = (argc > 2) ? atoi(argv[2]) : 3000;

    printf("╔══════════════════════════════════════╗\n");
    printf("║   Discovery Client Demo             ║\n");
    printf("║   iface: %-27s ║\n", iface_ip ? iface_ip : "(default)");
    printf("║   timeout: %-24d ms ║\n", timeout_ms);
    printf("╚══════════════════════════════════════╝\n\n");

    NET_DiscoveryDeviceInfo_S devices[NETSDK_DEMO_MAX_DEVICES];
    int count = 0;

    printf("[discovery-client] searching...\n");

    BOOL ret = NET_Discovery_Search(iface_ip,
                                        static_cast<UINT32>(timeout_ms),
                                        devices,
                                        NETSDK_DEMO_MAX_DEVICES,
                                        &count);

    if (!ret) {
        printf("[discovery-client] search failed, err=%d\n",
               NET_GetLastError());
        return -1;
    }

    printf("[discovery-client] found %d device(s)\n\n", count);

    if (count == 0) {
        printf("┌──────────────────────────────────────┐\n");
        printf("│  No devices found on LAN             │\n");
        printf("│  Make sure DiscoveryServer is running│\n");
        printf("│  on a device in the same network.    │\n");
        printf("└──────────────────────────────────────┘\n");
        return 0;
    }

    for (int i = 0; i < count; ++i) {
        const NET_DiscoveryDeviceInfo_S& d = devices[i];
        printf("┌──────────────────────────────────────┐\n");
        printf("│ Device #%d\n", i + 1);
        printf("├──────────────────────────────────────┤\n");
        printf("│ Name:      %-25s │\n", d.strDeviceName);
        printf("│ ID:        %-25s │\n", d.strDeviceID);
        printf("│ Type:      %-25s │\n", d.strDeviceType);
        printf("│ IP:        %-25s │\n", d.strIPv4Address);
        printf("│ Netmask:   %-25s │\n", d.strIPv4SubnetMask);
        printf("│ Gateway:   %-25s │\n", d.strIPv4Gateway);
        printf("│ MAC:       %-25s │\n", d.strMACAddress);
        printf("│ Firmware:  %-25s │\n", d.strFirmwareVersion);
        printf("│ HTTP Port: %-25u │\n", d.uHttpPort);
        printf("│ Vendor:    %-25s │\n", d.strManufacturer);
        printf("└──────────────────────────────────────┘\n\n");
    }

    return 0;
}
