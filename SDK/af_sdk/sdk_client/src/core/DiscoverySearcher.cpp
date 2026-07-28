/**
 * @file DiscoverySearcher.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CDiscoverySearcher 模块实现
 * 功能说明：
 * 1. 实现 CDiscoverySearcher 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */

#include "DiscoverySearcher.h"

#include "DiscoveryProtocol.h"
#include "PlatformCompat.h"
#include <chrono>
#include <cstdio>
#include <cstring>

#define NETSDK_DISCOVERY_PROBE_COUNT  3
#define NETSDK_DISCOVERY_PROBE_DELAY_US  (100 * 1000)  /* 100ms */

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 在指定网络接口上发送发现报文并收集设备响应。
 * @param [in] szInterfaceIP 用于发送发现报文的本地接口 IPv4 地址。
 * @param [in] dwTimeoutMs 等待设备响应的超时时间，单位为毫秒。
 * @param [out] pDeviceList 接收发现到的设备信息的数组。
 * @param [in] nMaxCount pDeviceList 可容纳的最大设备数量。
 * @param [out] pnOutCount 实际发现到的设备数量。
 * @return 0 表示成功；负值表示套接字或发现流程失败。
 */
int CDiscoverySearcher::search(const char* szInterfaceIP,
                               UINT32 dwTimeoutMs,
                               NET_TV_DISCOVERY_DEVICE_INFO_S* pDeviceList,
                               int nMaxCount,
                               int* pnOutCount)
{
    if (!pDeviceList || nMaxCount <= 0 || !pnOutCount) return -1;
    *pnOutCount = 0;

    int ret = create_socket(szInterfaceIP);
    if (ret < 0) return ret;

    send_probes();
    ret = recv_responses(dwTimeoutMs, pDeviceList, nMaxCount, pnOutCount);
    close_socket();
    return ret;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 create_socket 对应的处理。
 * @param [in] szInterfaceIP 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int CDiscoverySearcher::create_socket(const char* szInterfaceIP)
{
#ifdef _WIN32
    m_nSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_nSocket == INVALID_SOCKET_FD) {
        fprintf(stderr, "discovery: socket failed, error=%d\n", NETSDK_SOCKET_GET_ERROR());
        return -2;
    }
    NETSDK_SOCKET_SET_NONBLOCK(m_nSocket);
#else
    m_nSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (m_nSocket == INVALID_SOCKET_FD) { fprintf(stderr, "discovery: socket failed\n"); return -2; }
#endif

    /* bind any port — 服务端单播回包会到这个端口 */
    struct sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = 0;
    if (bind(m_nSocket, reinterpret_cast<struct sockaddr*>(&bind_addr),
             sizeof(bind_addr)) != 0) {
        fprintf(stderr, "discovery: bind failed, error=%d\n", NETSDK_SOCKET_GET_ERROR());
        close_socket();
        return -3;
    }

    /* 不需要 IP_ADD_MEMBERSHIP：服务端回包是单播，直接走内核协议栈收 */

    /* 如果指定了网口，绑定到该网口 */
    if (szInterfaceIP && szInterfaceIP[0] != '\0') {
        m_strInterfaceIp = szInterfaceIP;
        struct in_addr iface_addr{};
#ifdef _WIN32
        inet_pton(AF_INET, szInterfaceIP, &iface_addr);
#else
        iface_addr.s_addr = inet_addr(szInterfaceIP);
#endif
        setsockopt(m_nSocket, IPPROTO_IP, IP_MULTICAST_IF,
                   reinterpret_cast<const char*>(&iface_addr), sizeof(iface_addr));
    }

    /* 组播 TTL */
    int ttl = NET_TV_DISCOVERY_TTL;
    setsockopt(m_nSocket, IPPROTO_IP, IP_MULTICAST_TTL,
               reinterpret_cast<const char*>(&ttl), sizeof(ttl));

    return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 close_socket 定义的内部处理。
 * @return 无返回值。
 */

void CDiscoverySearcher::close_socket()
{
    if (m_nSocket != INVALID_SOCKET_FD) {
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 send_probes 对应的处理。
 * @return 返回该处理的状态或结果。
 */

int CDiscoverySearcher::send_probes()
{
    struct sockaddr_in dst_addr{};
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port   = htons(NET_TV_DISCOVERY_MCAST_PORT);
#ifdef _WIN32
    inet_pton(AF_INET, NET_TV_DISCOVERY_MCAST_ADDR, &dst_addr.sin_addr);
#else
    dst_addr.sin_addr.s_addr = inet_addr(NET_TV_DISCOVERY_MCAST_ADDR);
#endif

    std::string probe = discovery::build_probe_json();

    int ret = 0;
    for (int i = 0; i < NETSDK_DISCOVERY_PROBE_COUNT; ++i) {
        ssize_t sent = sendto(m_nSocket, probe.data(), static_cast<int>(probe.size()), 0,
                              reinterpret_cast<struct sockaddr*>(&dst_addr),
                              sizeof(dst_addr));
        if (sent < 0) { fprintf(stderr, "discovery: sendto failed, error=%d\n", NETSDK_SOCKET_GET_ERROR()); ret = -2; }
        if (i < NETSDK_DISCOVERY_PROBE_COUNT - 1) {
            NETSDK_MICRO_SLEEP(NETSDK_DISCOVERY_PROBE_DELAY_US);
        }
    }
    return ret;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 recv_responses 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

int CDiscoverySearcher::recv_responses(UINT32 dwTimeoutMs,
                                       NET_TV_DISCOVERY_DEVICE_INFO_S* pDeviceList,
                                       int nMaxCount,
                                       int* pnOutCount)
{
    int count = 0;
    auto start_time = std::chrono::steady_clock::now();

    char buf[4096];

    while (count < nMaxCount) {
        auto now = std::chrono::steady_clock::now();
        long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        long remaining_ms = static_cast<long>(dwTimeoutMs) - elapsed_ms;
        if (remaining_ms <= 0) break;

        struct pollfd pfd{};
        pfd.fd = static_cast<int>(m_nSocket);
        pfd.events = POLLIN;

        int ret = NETSDK_POLL(&pfd, 1, static_cast<int>(remaining_ms));
        if (ret < 0) break;
        if (ret == 0) break;

        struct sockaddr_in from_addr{};
        socklen_t from_len = sizeof(from_addr);
        ssize_t n = recvfrom(m_nSocket, buf, static_cast<int>(sizeof(buf) - 1), 0,
                             reinterpret_cast<struct sockaddr*>(&from_addr),
                             &from_len);
        if (n <= 0) continue;

        buf[n] = '\0';
        NET_TV_DISCOVERY_DEVICE_INFO_S info{};
        if (!discovery::parse_response_json(std::string(buf, n), info)) continue;

        if (is_duplicate(info, pDeviceList, count)) continue;

        pDeviceList[count++] = info;
    }

    *pnOutCount = count;
    return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 is_duplicate 对应的数据。
 * @return 返回该处理的状态或结果。
 */

bool CDiscoverySearcher::is_duplicate(const NET_TV_DISCOVERY_DEVICE_INFO_S& info,
                                      const NET_TV_DISCOVERY_DEVICE_INFO_S* pList,
                                      int nCount) const
{
    for (int i = 0; i < nCount; ++i) {
        if (std::strcmp(info.szDeviceID, pList[i].szDeviceID) == 0
            && info.szDeviceID[0] != '\0') {
            return true;
        }
    }
    return false;
}
