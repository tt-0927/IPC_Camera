/**
 * @file discovery_client_demo.cpp
 * @brief 设备发现客户端 Demo
 *
 * 功能:
 *   1. 发送 UDP 组播探测包
 *   2. 接收服务端响应
 *   3. 打印发现的设备列表
 *
 * 用法:
 *   ./DiscoveryClientDemo [interface_ip] [timeout_ms]
 *   默认: interface_ip=NULL(默认路由), timeout_ms=3000
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

#define MAX_DEVICES 64

int main(int argc, char* argv[])
{
    const char* iface_ip  = (argc > 1) ? argv[1] : NULL;
    int timeout_ms        = (argc > 2) ? atoi(argv[2]) : 3000;

    printf("╔══════════════════════════════════════╗\n");
    printf("║   Discovery Client Demo             ║\n");
    printf("║   iface: %-27s ║\n", iface_ip ? iface_ip : "(default)");
    printf("║   timeout: %-24d ms ║\n", timeout_ms);
    printf("╚══════════════════════════════════════╝\n\n");

    NET_DiscoveryDeviceInfo_S devices[MAX_DEVICES];
    int count = 0;

    printf("[discovery-client] searching...\n");

    BOOL ret = NET_TV_Discovery_Search(iface_ip,
                                        static_cast<UINT32>(timeout_ms),
                                        devices,
                                        MAX_DEVICES,
                                        &count);

    if (!ret) {
        printf("[discovery-client] search failed, err=%d\n",
               NET_TV_GetLastError());
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
