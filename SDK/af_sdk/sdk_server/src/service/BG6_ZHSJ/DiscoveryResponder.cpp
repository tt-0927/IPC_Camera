/**
 * @file CDiscoveryResponder.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CDiscoveryResponder 模块实现
 * 功能说明：
 * 1. 实现 CDiscoveryResponder 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */

#include "DiscoveryResponder.h"

#include "DiscoveryProtocol.h"
#include "NetSdkLog.h"
#include "PlatformCompat.h"

#include <cstdio>
#include <cstring>
#include <chrono>

#ifndef _WIN32
#include <linux/filter.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#endif

/* ---- helpers ---- */

static bool same_mac(const char* pstrLhs, const char* pstrRhs)
{
    if (!pstrLhs || !pstrRhs || pstrLhs[0] == '\0' || pstrRhs[0] == '\0') return false;

    unsigned int uLeft[6]{};
    unsigned int uRight[6]{};
    char chTail = '\0';
    int nLeftCount = std::sscanf(pstrLhs, "%2x:%2x:%2x:%2x:%2x:%2x%c",
                                 &uLeft[0], &uLeft[1], &uLeft[2], &uLeft[3],
                                 &uLeft[4], &uLeft[5], &chTail);
    if (nLeftCount != 6) {
        nLeftCount = std::sscanf(pstrLhs, "%2x-%2x-%2x-%2x-%2x-%2x%c",
                                 &uLeft[0], &uLeft[1], &uLeft[2], &uLeft[3],
                                 &uLeft[4], &uLeft[5], &chTail);
    }

    chTail = '\0';
    int nRightCount = std::sscanf(pstrRhs, "%2x:%2x:%2x:%2x:%2x:%2x%c",
                                  &uRight[0], &uRight[1], &uRight[2], &uRight[3],
                                  &uRight[4], &uRight[5], &chTail);
    if (nRightCount != 6) {
        nRightCount = std::sscanf(pstrRhs, "%2x-%2x-%2x-%2x-%2x-%2x%c",
                                  &uRight[0], &uRight[1], &uRight[2], &uRight[3],
                                  &uRight[4], &uRight[5], &chTail);
    }

    if (nLeftCount != 6 || nRightCount != 6) return false;
    for (int i = 0; i < 6; ++i) {
        if (uLeft[i] > 0xff || uRight[i] > 0xff || uLeft[i] != uRight[i]) {
            return false;
        }
    }
    return true;
}

#ifndef _WIN32
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get_iface_mac 对应的数据。
 * @param [in] pstrIface 函数处理参数。
 * @param [in] byMac 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
static bool get_iface_mac(const char* pstrIface, uint8_t byMac[6])
{
    int nFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (nFd < 0) return false;

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, pstrIface, IFNAMSIZ - 1);
    bool bOk = (ioctl(nFd, SIOCGIFHWADDR, &ifr) == 0);
    close(nFd);
    if (bOk) std::memcpy(byMac, ifr.ifr_hwaddr.sa_data, 6);
    return bOk;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get_iface_ip 对应的数据。
 * @param [in] pstrIface 函数处理参数。
 * @param [in,out] uIp 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool get_iface_ip(const char* pstrIface, uint32_t& uIp)
{
    int nFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (nFd < 0) return false;

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, pstrIface, IFNAMSIZ - 1);
    bool bOk = (ioctl(nFd, SIOCGIFADDR, &ifr) == 0);
    close(nFd);
    if (bOk) {
        uIp = reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_addr)->sin_addr.s_addr;
    }
    return bOk;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get_iface_index 对应的数据。
 * @param [in] pstrIface 函数处理参数。
 * @param [in,out] nIfindex 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool get_iface_index(const char* pstrIface, int& nIfindex)
{
    nIfindex = static_cast<int>(if_nametoindex(pstrIface));
    return nIfindex > 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 attach_discovery_filter 定义的内部处理。
 * @param [in] nFd 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool attach_discovery_filter(int nFd)
{
    const uint32_t uMcastAddr = ntohl(inet_addr(NET_DISCOVERY_MCAST_ADDR));

    struct sock_filter filter[] = {
        /* Ethernet type: IPv4 */
        BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 12),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ETH_P_IP, 0, 8),
        /* IP protocol: UDP */
        BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 23),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_UDP, 0, 6),
        /* IP destination: discovery multicast address */
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, 30),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, uMcastAddr, 0, 4),
        /* UDP destination port, accounting for variable IP header length */
        BPF_STMT(BPF_LDX | BPF_B | BPF_MSH, 14),
        BPF_STMT(BPF_LD | BPF_H | BPF_IND, 16),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, NET_DISCOVERY_MCAST_PORT, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, 0xFFFF),
        BPF_STMT(BPF_RET | BPF_K, 0),
    };

    struct sock_fprog prog{};
    prog.len = static_cast<unsigned short>(sizeof(filter) / sizeof(filter[0]));
    prog.filter = filter;

    if (setsockopt(nFd, SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof(prog)) < 0) {
        perror("discovery-responder: attach filter");
        return false;
    }
    return true;
}
#endif /* _WIN32 */

/* ---- CDiscoveryResponder ---- */

int CDiscoveryResponder::init(const char* pstrInterfaceName)
{
    if (!pstrInterfaceName || pstrInterfaceName[0] == '\0') return -1;
    m_strInterfaceName = pstrInterfaceName;

#ifdef _WIN32
    /* Windows: 使用普通 UDP 组播接收，不支持 AF_PACKET raw socket */
    m_aLocalMac[0] = 0; m_aLocalMac[1] = 0; m_aLocalMac[2] = 0;
    m_aLocalMac[3] = 0; m_aLocalMac[4] = 0; m_aLocalMac[5] = 0;
    m_uLocalIp = 0;
    m_nInterfaceIndex = 0;

    /* 创建 UDP 组播接收套接字 */
    m_nUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_nUdpSocket == INVALID_SOCKET_FD) {
        fprintf(stderr, "discovery-responder: socket failed, error=%d\n", WSAGetLastError());
        return -6;
    }

    /* 允许端口复用 */
    int nReuse = 1;
    setsockopt(m_nUdpSocket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&nReuse), sizeof(nReuse));

    /* 绑定到组播地址和端口 */
    struct sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = inet_addr(NET_DISCOVERY_MCAST_ADDR);
    bind_addr.sin_port = htons(NET_DISCOVERY_MCAST_PORT);
    if (bind(m_nUdpSocket, reinterpret_cast<struct sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
        fprintf(stderr, "discovery-responder: bind failed, error=%d\n", WSAGetLastError());
        NETSDK_SOCKET_CLOSE(m_nUdpSocket);
        m_nUdpSocket = INVALID_SOCKET_FD;
        return -6;
    }

    /* 加入组播组 */
    struct ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr(NET_DISCOVERY_MCAST_ADDR);
    mreq.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(m_nUdpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   reinterpret_cast<const char*>(&mreq), sizeof(mreq)) < 0) {
        fprintf(stderr, "discovery-responder: IGMP join failed, error=%d\n", WSAGetLastError());
    }

    printf("[discovery-responder] init ok (Windows UDP multicast mode): iface=%s\n",
           m_strInterfaceName.c_str());
    fflush(stdout);
    return 0;
#else
    if (!get_iface_mac(pstrInterfaceName, m_aLocalMac)) return -2;
    if (!get_iface_ip(pstrInterfaceName, m_uLocalIp))   return -3;
    if (!get_iface_index(pstrInterfaceName, m_nInterfaceIndex)) return -4;

    if (!init_raw_socket()) return -5;

    /* 创建 UDP 回包套接字 */
    m_nUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_nUdpSocket < 0) {
        close_raw_socket();
        return -6;
    }

    /* bind to interface for UDP send */
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, m_strInterfaceName.c_str(), IFNAMSIZ - 1);
    setsockopt(m_nUdpSocket, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));

    /* 加入组播组 — 触发 IGMP join，告诉交换机转发该组播流到此端口 */
    m_nIgmpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_nIgmpSocket >= 0) {
        struct ip_mreq mreq{};
        mreq.imr_multiaddr.s_addr = inet_addr(NET_DISCOVERY_MCAST_ADDR);
        mreq.imr_interface.s_addr = m_uLocalIp;
        if (setsockopt(m_nIgmpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                       &mreq, sizeof(mreq)) < 0) {
            perror("discovery-responder: IGMP join");
        }
    }

    printf("[discovery-responder] init ok: iface=%s ifindex=%d ip=0x%x mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
           m_strInterfaceName.c_str(), m_nInterfaceIndex, m_uLocalIp,
           m_aLocalMac[0], m_aLocalMac[1], m_aLocalMac[2],
           m_aLocalMac[3], m_aLocalMac[4], m_aLocalMac[5]);
    fflush(stdout);

    return 0;
#endif
}

#ifndef _WIN32
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 init_raw_socket 对应的处理。
 * @return 返回该处理的状态或结果。
 */
bool CDiscoveryResponder::init_raw_socket()
{
    m_nRawFd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (m_nRawFd < 0) return false;

    if (!attach_discovery_filter(m_nRawFd)) {
        close_raw_socket();
        return false;
    }

    /* 设置 1s 接收超时，确保 stop() 能及时退出 recvfrom */
    struct timeval tv{};
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    setsockopt(m_nRawFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* bind to specific interface */
    struct sockaddr_ll sll{};
    sll.sll_family   = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_IP);
    sll.sll_ifindex  = m_nInterfaceIndex;
    if (bind(m_nRawFd, reinterpret_cast<struct sockaddr*>(&sll), sizeof(sll)) < 0) {
        close_raw_socket();
        return false;
    }
    return true;
}
#endif /* _WIN32 */

void CDiscoveryResponder::close_raw_socket()
{
#ifndef _WIN32
    if (m_nRawFd >= 0)  { close(m_nRawFd);  m_nRawFd  = -1; }
    if (m_nIgmpSocket >= 0) { close(m_nIgmpSocket); m_nIgmpSocket = -1; }
#endif
    if (m_nUdpSocket != INVALID_SOCKET_FD) { NETSDK_SOCKET_CLOSE(m_nUdpSocket); m_nUdpSocket = INVALID_SOCKET_FD; }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 start 对应的处理。
 * @return 返回该处理的状态或结果。
 */

int CDiscoveryResponder::start()
{
    if (m_bRunning.exchange(true)) return 0;  /* already running */
    if (!m_fnDeviceInfoCallback) return -1;          /* callback not set */

    m_stThread = std::thread(&CDiscoveryResponder::receive_thread, this);
    return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 stop 对应的处理。
 * @return 无返回值。
 */

void CDiscoveryResponder::stop()
{
    m_bRunning.store(false);
    if (m_stThread.joinable()) {
        m_stThread.join();
    }
    close_raw_socket();
}

/* ---- frame parsing (Linux only, uses AF_PACKET raw socket) ---- */

#ifndef _WIN32
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 parse_frame 对应的数据。
 * @return 返回该处理的状态或结果。
 */
bool CDiscoveryResponder::parse_frame(const uint8_t* pbyData, ssize_t lLen,
                                      std::vector<uint8_t>& vecUdpPayload,
                                      uint8_t byClientMac[6],
                                      uint32_t& uClientIp, uint16_t& uClientPort)
{
    if (lLen < static_cast<ssize_t>(sizeof(struct ethhdr))) return false;

    const auto* eth = reinterpret_cast<const struct ethhdr*>(pbyData);
    if (ntohs(eth->h_proto) != ETH_P_IP) return false;

    std::memcpy(byClientMac, eth->h_source, 6);

    /* 跳过以太网头 */
    const uint8_t* pbyIpData = pbyData + sizeof(struct ethhdr);
    ssize_t lIpLen = lLen - static_cast<ssize_t>(sizeof(struct ethhdr));
    if (lIpLen < static_cast<ssize_t>(sizeof(struct iphdr))) return false;

    const auto* ip = reinterpret_cast<const struct iphdr*>(pbyIpData);
    if (ip->version != 4) return false;
    if (ip->protocol != IPPROTO_UDP) return false;

    /* 检查目标 IP 是否匹配组播地址 */
    if (ip->daddr != inet_addr(NET_DISCOVERY_MCAST_ADDR)) return false;

    uClientIp = ip->saddr;

    /* 跳过 IP 头 */
    ssize_t lIpHdrLen = ip->ihl * 4;
    const uint8_t* pbyUdpData = pbyIpData + lIpHdrLen;
    ssize_t lUdpLen = lIpLen - lIpHdrLen;
    if (lUdpLen < static_cast<ssize_t>(sizeof(struct udphdr))) return false;

    const auto* udp = reinterpret_cast<const struct udphdr*>(pbyUdpData);
    uint16_t uDstPort = ntohs(udp->dest);
    if (uDstPort != NET_DISCOVERY_MCAST_PORT) return false;

    uClientPort = ntohs(udp->source);

    /* UDP 负载 */
    ssize_t lPayloadLen = lUdpLen - static_cast<ssize_t>(sizeof(struct udphdr));
    const uint8_t* pbyPayload = pbyUdpData + sizeof(struct udphdr);
    if (lPayloadLen <= 0) return false;

    vecUdpPayload.assign(pbyPayload, pbyPayload + lPayloadLen);
    return true;
}

/* ---- L2 response (Linux only, uses AF_PACKET raw socket) ---- */

static uint16_t ip_checksum(const void* pData, size_t uLen)
{
    uint32_t uSum = 0;
    const uint16_t* p = static_cast<const uint16_t*>(pData);
    for (size_t i = 0; i < uLen / 2; ++i) {
        uSum += p[i];
    }
    if (uLen & 1) {
        uSum += static_cast<const uint8_t*>(pData)[uLen - 1];
    }
    while (uSum >> 16) {
        uSum = (uSum & 0xFFFF) + (uSum >> 16);
    }
    return static_cast<uint16_t>(~uSum);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 build_l2_response 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

std::vector<uint8_t> CDiscoveryResponder::build_l2_response(
    const uint8_t byClientMac[6],
    uint32_t uClientIp, uint16_t uClientPort,
    const NET_DiscoveryDeviceInfo_S& stInfo) const
{
    std::string json = discovery::build_response_json(stInfo);
    uint16_t uUdpTotalLen = static_cast<uint16_t>(sizeof(struct udphdr) + json.size());
    uint16_t uIpTotalLen  = static_cast<uint16_t>(sizeof(struct iphdr) + uUdpTotalLen);

    std::vector<uint8_t> frame(sizeof(struct ethhdr) + uIpTotalLen);
    uint8_t* p = frame.data();

    /* Ethernet */
    auto* eth = reinterpret_cast<struct ethhdr*>(p);
    std::memcpy(eth->h_dest,   byClientMac,  6);
    std::memcpy(eth->h_source, m_aLocalMac,   6);
    eth->h_proto = htons(ETH_P_IP);
    p += sizeof(struct ethhdr);

    /* IP header */
    auto* ip = reinterpret_cast<struct iphdr*>(p);
    std::memset(ip, 0, sizeof(*ip));
    ip->version  = 4;
    ip->ihl      = 5;
    ip->tot_len  = htons(uIpTotalLen);
    ip->id       = htons(static_cast<uint16_t>(rand() & 0xFFFF));
    ip->ttl      = 64;
    ip->protocol = IPPROTO_UDP;
    ip->saddr    = m_uLocalIp;
    ip->daddr    = uClientIp;
    ip->check    = 0;
    ip->check    = ip_checksum(ip, sizeof(struct iphdr));
    p += sizeof(struct iphdr);

    /* UDP header */
    auto* udp = reinterpret_cast<struct udphdr*>(p);
    udp->source = htons(NET_DISCOVERY_MCAST_PORT);
    udp->dest   = htons(uClientPort);
    udp->len    = htons(uUdpTotalLen);
    udp->check  = 0;
    p += sizeof(struct udphdr);

    /* payload */
    std::memcpy(p, json.data(), json.size());

    return frame;
}
#endif /* _WIN32 */

/* ---- standard UDP response ---- */

int CDiscoveryResponder::send_udp_response(uint32_t uClientIp, uint16_t uClientPort,
                                           const NET_DiscoveryDeviceInfo_S& stInfo)
{
    std::string json = discovery::build_response_json(stInfo);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(uClientPort);
    addr.sin_addr.s_addr = uClientIp;

    ssize_t lRet = sendto(m_nUdpSocket, json.data(), json.size(), 0,
                         reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    return lRet >= 0 ? 0 : -1;
}

/* ---- main receive loop ---- */

void CDiscoveryResponder::receive_thread()
{
#ifdef _WIN32
    /* Windows: 使用 UDP 组播接收，不支持 AF_PACKET raw socket */
    constexpr auto kProbeLogInterval = std::chrono::seconds(60);
    char strBuf[4096];
    int nProbeCount = 0;
    int nInvalidProbeCount = 0;
    int nUdpOkCount = 0;
    int nUdpFailCount = 0;
    auto stLastProbeLog = std::chrono::steady_clock::now();

    printf("[discovery-responder] thread started (Windows UDP multicast mode)\n");
    fflush(stdout);

    while (m_bRunning.load()) {
        struct sockaddr_in from_addr{};
        int nFromLen = sizeof(from_addr);
        int n = recvfrom(m_nUdpSocket, strBuf, sizeof(strBuf) - 1, 0,
                         reinterpret_cast<struct sockaddr*>(&from_addr), &nFromLen);
        if (n <= 0) {
            int nErr = WSAGetLastError();
            if (nErr == WSAEINTR || nErr == WSAEWOULDBLOCK) continue;
            fprintf(stderr, "discovery-responder: recvfrom failed, error=%d\n", nErr);
            break;
        }

        strBuf[n] = '\0';
        uint32_t uClientIp = from_addr.sin_addr.s_addr;
        uint16_t uClientPort = ntohs(from_addr.sin_port);

        std::string strProbe(strBuf, n);
        NET_PoeNetworkConfig_S stNetworkConfig{};
        if (discovery::parse_set_network_json(strProbe, stNetworkConfig)) {
            NET_DiscoveryDeviceInfo_S stDevInfo{};
            m_fnDeviceInfoCallback(&stDevInfo);
            if (!same_mac(stNetworkConfig.strMACAddress, stDevInfo.strMACAddress)) {
                nInvalidProbeCount++;
                continue;
            }

            const bool bHasCallback = static_cast<bool>(m_fnSetNetworkCallback);
            const bool bApplied = bHasCallback &&
                                 m_fnSetNetworkCallback(stNetworkConfig);
            printf("[discovery-responder] set network mac=%s result=%s\n",
                   stNetworkConfig.strMACAddress,
                   !bHasCallback ? "callback-not-registered" :
                   (bApplied ? "success" : "failed"));
            fflush(stdout);
            continue;
        }

        /* 校验探测包 */
        if (!discovery::parse_probe_json(strProbe)) {
            nInvalidProbeCount++;
            continue;
        }

        nProbeCount++;

        /* 获取本机设备信息 */
        NET_DiscoveryDeviceInfo_S stDevInfo{};
        m_fnDeviceInfoCallback(&stDevInfo);

        /* UDP 单播回包 */
        int nUdpRet = send_udp_response(uClientIp, uClientPort, stDevInfo);
        if (nUdpRet == 0) {
            nUdpOkCount++;
        } else {
            nUdpFailCount++;
            char strIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &uClientIp, strIp, sizeof(strIp));
            printf("[discovery-responder] udp response failed to %s:%d\n", strIp, uClientPort);
            fflush(stdout);
        }

        auto now = std::chrono::steady_clock::now();
        if (now - stLastProbeLog >= kProbeLogInterval) {
            printf("[discovery-responder] summary probes=%d invalid=%d udp_ok=%d udp_fail=%d\n",
                   nProbeCount, nInvalidProbeCount, nUdpOkCount, nUdpFailCount);
            fflush(stdout);

            nProbeCount = 0;
            nInvalidProbeCount = 0;
            nUdpOkCount = 0;
            nUdpFailCount = 0;
            stLastProbeLog = now;
        }
    }

    printf("[discovery-responder] thread exiting (Windows)\n");
    fflush(stdout);
#else
    /* Linux: 使用 AF_PACKET raw socket 接收 */
    constexpr int kFilteredLogInterval = 5000;
    constexpr auto kProbeLogInterval = std::chrono::seconds(60);
    uint8_t byBuf[ETH_FRAME_LEN];
    int nFrameCount = 0;
    int nDropCount = 0;
    int nProbeCount = 0;
    int nInvalidProbeCount = 0;
    int nUdpOkCount = 0;
    int nUdpFailCount = 0;
    int nL2OkCount = 0;
    int nL2FailCount = 0;
    auto stLastProbeLog = std::chrono::steady_clock::now();
    
    printf("[discovery-responder] thread started, listening on ifindex=%d\n", m_nInterfaceIndex);
    fflush(stdout);
    
    while (m_bRunning.load()) {
        ssize_t n = recvfrom(m_nRawFd, byBuf, sizeof(byBuf), 0, nullptr, nullptr);
        if (n <= 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            perror("discovery-responder: recvfrom");
            break;
        }
    
        nFrameCount++;
    
        std::vector<uint8_t> vecPayload;
        uint8_t byClientMac[6];
        uint32_t uClientIp = 0;
        uint16_t uClientPort = 0;
    
        if (!parse_frame(byBuf, n, vecPayload, byClientMac, uClientIp, uClientPort)) {
            nDropCount++;
            /* 普通网卡流量很多，降低过滤统计日志频率，避免刷屏。 */
            if (nDropCount % kFilteredLogInterval == 0) {
                printf("[discovery-responder] recv=%d filtered=%d\n", nFrameCount, nDropCount);
                fflush(stdout);
            }
            continue;
        }
    
        char strIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &uClientIp, strIp, sizeof(strIp));
        nProbeCount++;
    
        std::string strProbe(reinterpret_cast<const char*>(vecPayload.data()), vecPayload.size());
        NET_PoeNetworkConfig_S stNetworkConfig{};
        if (discovery::parse_set_network_json(strProbe, stNetworkConfig)) {
            NET_DiscoveryDeviceInfo_S stDevInfo{};
            m_fnDeviceInfoCallback(&stDevInfo);
            if (!same_mac(stNetworkConfig.strMACAddress, stDevInfo.strMACAddress)) {
                nInvalidProbeCount++;
                continue;
            }
    
            const bool bHasCallback = static_cast<bool>(m_fnSetNetworkCallback);
            const bool bApplied = bHasCallback &&
                                 m_fnSetNetworkCallback(stNetworkConfig);
            printf("[discovery-responder] set network mac=%s result=%s\n",
                   stNetworkConfig.strMACAddress,
                   !bHasCallback ? "callback-not-registered" :
                   (bApplied ? "success" : "failed"));
            fflush(stdout);
            continue;
        }
    
        /* 校验探测包 */
        if (!discovery::parse_probe_json(strProbe)) {
            nInvalidProbeCount++;
            continue;
        }
    
        /* 获取本机设备信息 */
        NET_DiscoveryDeviceInfo_S stDevInfo{};
        m_fnDeviceInfoCallback(&stDevInfo);
    
        /* 1. 标准 UDP 单播回包 */
        int nUdpRet = send_udp_response(uClientIp, uClientPort, stDevInfo);
        if (nUdpRet == 0) {
            nUdpOkCount++;
        } else {
            nUdpFailCount++;
            printf("[discovery-responder] udp response failed to %s:%d\n", strIp, uClientPort);
            fflush(stdout);
        }
    
        /* 2. AF_PACKET L2 回包（跳子网同广播域） */
        auto vecL2Frame = build_l2_response(byClientMac, uClientIp, uClientPort, stDevInfo);
    
        struct sockaddr_ll sll{};
        sll.sll_family   = AF_PACKET;
        sll.sll_ifindex  = m_nInterfaceIndex;
        sll.sll_hatype   = 1;  /* ARPHRD_ETHER */
        sll.sll_halen    = ETH_ALEN;
        std::memcpy(sll.sll_addr, byClientMac, ETH_ALEN);
    
        ssize_t lL2Ret = sendto(m_nRawFd, vecL2Frame.data(), vecL2Frame.size(), 0,
                                reinterpret_cast<struct sockaddr*>(&sll), sizeof(sll));
        if (lL2Ret >= 0) {
            nL2OkCount++;
        } else {
            nL2FailCount++;
            perror("discovery-responder: l2 sendto");
        }
    
        auto now = std::chrono::steady_clock::now();
        if (now - stLastProbeLog >= kProbeLogInterval) {
            printf("[discovery-responder] summary probes=%d invalid=%d udp_ok=%d udp_fail=%d l2_ok=%d l2_fail=%d last=%s:%d\n",
                   nProbeCount, nInvalidProbeCount,
                   nUdpOkCount, nUdpFailCount,
                   nL2OkCount, nL2FailCount,
                   strIp, uClientPort);
            fflush(stdout);
    
            nProbeCount = 0;
            nInvalidProbeCount = 0;
            nUdpOkCount = 0;
            nUdpFailCount = 0;
            nL2OkCount = 0;
            nL2FailCount = 0;
            stLastProbeLog = now;
        }
    }
    
    printf("[discovery-responder] thread exiting, recv=%d filtered=%d\n", nFrameCount, nDropCount);
    fflush(stdout);
#endif
}
