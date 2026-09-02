/**
 * @file DiscoveryProber.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CDiscoveryProber 模块实现
 * 功能说明：
 * 1. 实现 CDiscoveryProber 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */

#include "DiscoveryProber.h"

#include "DiscoveryProtocol.h"
#include "PlatformCompat.h"
#include <chrono>
#include <cstdio>
#include <cstring>

#define NETSDK_DISCOVERY_PROBE_COUNT  3
#define NETSDK_DISCOVERY_PROBE_DELAY_US  (100 * 1000)  /* 100ms */

namespace {

bool is_terminated(const char* text, size_t capacity)
{
    return text && std::memchr(text, '\0', capacity) != nullptr;
}

bool valid_mac(const char* text)
{
    if (!text) return false;

    unsigned int bytes[6]{};
    char tail = '\0';
    int count = std::sscanf(text, "%2x:%2x:%2x:%2x:%2x:%2x%c",
                            &bytes[0], &bytes[1], &bytes[2], &bytes[3],
                            &bytes[4], &bytes[5], &tail);
    if (count != 6) {
        count = std::sscanf(text, "%2x-%2x-%2x-%2x-%2x-%2x%c",
                            &bytes[0], &bytes[1], &bytes[2], &bytes[3],
                            &bytes[4], &bytes[5], &tail);
    }
    if (count != 6) return false;
    for (unsigned int byte : bytes) {
        if (byte > 0xff) return false;
    }
    return true;
}

}  // namespace

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
int CDiscoveryProber::search(const char* szInterfaceIP,
                               UINT32 dwTimeoutMs,
                               NET_DiscoveryDeviceInfo_S* pDeviceList,
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

int CDiscoveryProber::set_network(const NET_PoeNetworkConfig_S& config) const
{
    if (!is_terminated(config.szInterfaceIP, sizeof(config.szInterfaceIP)) ||
        !is_terminated(config.szMACAddress, sizeof(config.szMACAddress)) ||
        !is_terminated(config.szTargetIP, sizeof(config.szTargetIP)) ||
        !is_terminated(config.szSubnetMask, sizeof(config.szSubnetMask)) ||
        !is_terminated(config.szGateway, sizeof(config.szGateway)) ||
        !valid_mac(config.szMACAddress) || config.szTargetIP[0] == '\0' ||
        config.szSubnetMask[0] == '\0' ||
        (config.bSetGateway && config.szGateway[0] == '\0')) {
        return -2;
    }

    struct in_addr address{};
    if (inet_pton(AF_INET, config.szTargetIP, &address) != 1 ||
        inet_pton(AF_INET, config.szSubnetMask, &address) != 1 ||
        (config.bSetGateway && inet_pton(AF_INET, config.szGateway, &address) != 1) ||
        (config.szInterfaceIP[0] != '\0' &&
         inet_pton(AF_INET, config.szInterfaceIP, &address) != 1)) {
        return -2;
    }

    socket_fd_t socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == INVALID_SOCKET_FD) {
        std::fprintf(stderr, "discovery: network-config socket failed, error=%d\n",
                     NETSDK_SOCKET_GET_ERROR());
        return -3;
    }

    if (config.szInterfaceIP[0] != '\0') {
        struct in_addr interface_address{};
        inet_pton(AF_INET, config.szInterfaceIP, &interface_address);
        if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_IF,
                       reinterpret_cast<const char*>(&interface_address),
                       sizeof(interface_address)) != 0) {
            NETSDK_SOCKET_CLOSE(socket_fd);
            return -3;
        }
    }

    int ttl = NET_DISCOVERY_TTL;
    setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_TTL,
               reinterpret_cast<const char*>(&ttl), sizeof(ttl));

    struct sockaddr_in destination{};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(NET_DISCOVERY_MCAST_PORT);
    if (inet_pton(AF_INET, NET_DISCOVERY_MCAST_ADDR, &destination.sin_addr) != 1) {
        NETSDK_SOCKET_CLOSE(socket_fd);
        return -3;
    }

    const std::string request = discovery::build_set_network_json(config);
    constexpr UINT32 kMaxSendCount = 32;
    const UINT32 requested_send_count = config.dwSendCount > 0
        ? config.dwSendCount
        : NETSDK_DISCOVERY_PROBE_COUNT;
    const UINT32 send_count = requested_send_count > kMaxSendCount
        ? kMaxSendCount
        : requested_send_count;
    const UINT32 delay_us = config.dwTimeoutMs > 0
        ? (config.dwTimeoutMs > 4000000U
               ? 4000000U
               : config.dwTimeoutMs * 1000U) / send_count
        : NETSDK_DISCOVERY_PROBE_DELAY_US;
    int ret = 0;
    for (UINT32 index = 0; index < send_count; ++index) {
        const ssize_t sent = sendto(
            socket_fd, request.data(), static_cast<int>(request.size()), 0,
            reinterpret_cast<struct sockaddr*>(&destination), sizeof(destination));
        if (sent < 0 || static_cast<size_t>(sent) != request.size()) {
            std::fprintf(stderr, "discovery: network-config sendto failed, error=%d\n",
                         NETSDK_SOCKET_GET_ERROR());
            ret = -3;
        }
        if (index + 1 < send_count) {
            NETSDK_MICRO_SLEEP(delay_us);
        }
    }

    NETSDK_SOCKET_CLOSE(socket_fd);
    return ret;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 创建UDP socket并配置组播参数
 * @param [in] szInterfaceIP 网卡IP地址，NULL使用默认路由
 * @return 成功返回0，失败返回-2(socket创建失败)或-3(bind失败)
 * @details 创建非阻塞UDP socket，绑定到任意端口，配置组播TTL和网口
 */
int CDiscoveryProber::create_socket(const char* szInterfaceIP)
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
    int ttl = NET_DISCOVERY_TTL;
    setsockopt(m_nSocket, IPPROTO_IP, IP_MULTICAST_TTL,
               reinterpret_cast<const char*>(&ttl), sizeof(ttl));

    return 0;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 关闭socket
 * @details 关闭UDP socket并重置为无效状态
 */
void CDiscoveryProber::close_socket()
{
    if (m_nSocket != INVALID_SOCKET_FD) {
        NETSDK_SOCKET_CLOSE(m_nSocket);
        m_nSocket = INVALID_SOCKET_FD;
    }
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送探测包到组播地址
 * @return 成功返回0，失败返回-2
 * @details 发送DISCOVERY_PROBE_COUNT个探测包，每个包间隔DISCOVERY_PROBE_DELAY_US微秒
 */
int CDiscoveryProber::send_probes()
{
    struct sockaddr_in dst_addr{};
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port   = htons(NET_DISCOVERY_MCAST_PORT);
#ifdef _WIN32
    inet_pton(AF_INET, NET_DISCOVERY_MCAST_ADDR, &dst_addr.sin_addr);
#else
    dst_addr.sin_addr.s_addr = inet_addr(NET_DISCOVERY_MCAST_ADDR);
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
 * @brief 接收设备响应
 * @param [in] dwTimeoutMs 等待响应超时时间(ms)
 * @param [out] pDeviceList 输出设备列表缓冲区
 * @param [in] nMaxCount 设备列表最大容量
 * @param [out] pnOutCount 实际发现的设备数量
 * @return 成功返回0
 * @details 使用poll等待socket可读，解析JSON响应，去重后存入设备列表
 */
int CDiscoveryProber::recv_responses(UINT32 dwTimeoutMs,
                                       NET_DiscoveryDeviceInfo_S* pDeviceList,
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
        NET_DiscoveryDeviceInfo_S info{};
        if (!discovery::parse_response_json(std::string(buf, n), info)) continue;

        if (is_duplicate(info, pDeviceList, count)) continue;

        pDeviceList[count++] = info;
    }

    *pnOutCount = count;
    return 0;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 检查设备是否已存在（去重）
 * @param [in] info 待检查的设备信息
 * @param [in] pList 已发现的设备列表
 * @param [in] nCount 已发现设备数量
 * @return 重复返回true，不重复返回false
 * @details 使用device_id字段进行去重比较
 */
bool CDiscoveryProber::is_duplicate(const NET_DiscoveryDeviceInfo_S& info,
                                      const NET_DiscoveryDeviceInfo_S* pList,
                                      int nCount) const
{
    for (int i = 0; i < nCount; ++i) {
        if (std::strcmp(info.strDeviceID, pList[i].strDeviceID) == 0
            && info.strDeviceID[0] != '\0') {
            return true;
        }
    }
    return false;
}
