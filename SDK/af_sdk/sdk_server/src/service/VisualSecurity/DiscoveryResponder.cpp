/**
 * @file DiscoveryResponder.cpp
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

#ifndef _WIN32
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get_iface_mac 对应的数据。
 * @param [in] iface 函数处理参数。
 * @param [in] mac 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
static bool get_iface_mac(const char* iface, uint8_t mac[6])
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    bool ok = (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0);
    close(fd);
    if (ok) std::memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
    return ok;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get_iface_ip 对应的数据。
 * @param [in] iface 函数处理参数。
 * @param [in,out] ip 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool get_iface_ip(const char* iface, uint32_t& ip)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    bool ok = (ioctl(fd, SIOCGIFADDR, &ifr) == 0);
    close(fd);
    if (ok) {
        ip = reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_addr)->sin_addr.s_addr;
    }
    return ok;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get_iface_index 对应的数据。
 * @param [in] iface 函数处理参数。
 * @param [in,out] ifindex 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool get_iface_index(const char* iface, int& ifindex)
{
    ifindex = static_cast<int>(if_nametoindex(iface));
    return ifindex > 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 attach_discovery_filter 定义的内部处理。
 * @param [in] fd 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static bool attach_discovery_filter(int fd)
{
    const uint32_t mcast_addr = ntohl(inet_addr(NET_TV_DISCOVERY_MCAST_ADDR));

    struct sock_filter filter[] = {
        /* Ethernet type: IPv4 */
        BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 12),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ETH_P_IP, 0, 8),
        /* IP protocol: UDP */
        BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 23),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_UDP, 0, 6),
        /* IP destination: discovery multicast address */
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, 30),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, mcast_addr, 0, 4),
        /* UDP destination port, accounting for variable IP header length */
        BPF_STMT(BPF_LDX | BPF_B | BPF_MSH, 14),
        BPF_STMT(BPF_LD | BPF_H | BPF_IND, 16),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, NET_TV_DISCOVERY_MCAST_PORT, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, 0xFFFF),
        BPF_STMT(BPF_RET | BPF_K, 0),
    };

    struct sock_fprog prog{};
    prog.len = static_cast<unsigned short>(sizeof(filter) / sizeof(filter[0]));
    prog.filter = filter;

    if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof(prog)) < 0) {
        perror("discovery-responder: attach filter");
        return false;
    }
    return true;
}
#endif /* _WIN32 */

/* ---- CDiscoveryResponder ---- */

int CDiscoveryResponder::init(const char* szInterfaceName)
{
    if (!szInterfaceName || szInterfaceName[0] == '\0') return -1;
    m_strInterfaceName = szInterfaceName;

#ifdef _WIN32
    /* Windows: 使用普通 UDP 组播接收，不支持 AF_PACKET raw socket */
    m_aLocalMac[0] = 0; m_aLocalMac[1] = 0; m_aLocalMac[2] = 0;
    m_aLocalMac[3] = 0; m_aLocalMac[4] = 0; m_aLocalMac[5] = 0;
    m_uLocalIp = 0;
    m_nInterfaceIndex = 0;

    /* 创建 UDP 组播接收套接字 */
    m_nUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_nUdpSocket == NETSDK_DEMO_INVALID_SOCKET) {
        fprintf(stderr, "discovery-responder: socket failed, error=%d\n", WSAGetLastError());
        return -6;
    }

    /* 允许端口复用 */
    int reuse = 1;
    setsockopt(m_nUdpSocket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    /* 绑定到组播地址和端口 */
    struct sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = inet_addr(NET_TV_DISCOVERY_MCAST_ADDR);
    bind_addr.sin_port = htons(NET_TV_DISCOVERY_MCAST_PORT);
    if (bind(m_nUdpSocket, reinterpret_cast<struct sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
        fprintf(stderr, "discovery-responder: bind failed, error=%d\n", WSAGetLastError());
        NETSDK_SOCKET_CLOSE(m_nUdpSocket);
        m_nUdpSocket = NETSDK_DEMO_INVALID_SOCKET;
        return -6;
    }

    /* 加入组播组 */
    struct ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr(NET_TV_DISCOVERY_MCAST_ADDR);
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
    if (!get_iface_mac(szInterfaceName, m_aLocalMac)) return -2;
    if (!get_iface_ip(szInterfaceName, m_uLocalIp))   return -3;
    if (!get_iface_index(szInterfaceName, m_nInterfaceIndex)) return -4;

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
        mreq.imr_multiaddr.s_addr = inet_addr(NET_TV_DISCOVERY_MCAST_ADDR);
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
bool CDiscoveryResponder::parse_frame(const uint8_t* data, ssize_t len,
                                      std::vector<uint8_t>& udp_payload,
                                      uint8_t client_mac[6],
                                      uint32_t& client_ip, uint16_t& client_port)
{
    if (len < static_cast<ssize_t>(sizeof(struct ethhdr))) return false;

    const auto* eth = reinterpret_cast<const struct ethhdr*>(data);
    if (ntohs(eth->h_proto) != ETH_P_IP) return false;

    std::memcpy(client_mac, eth->h_source, 6);

    /* 跳过以太网头 */
    const uint8_t* ip_data = data + sizeof(struct ethhdr);
    ssize_t ip_len = len - static_cast<ssize_t>(sizeof(struct ethhdr));
    if (ip_len < static_cast<ssize_t>(sizeof(struct iphdr))) return false;

    const auto* ip = reinterpret_cast<const struct iphdr*>(ip_data);
    if (ip->version != 4) return false;
    if (ip->protocol != IPPROTO_UDP) return false;

    /* 检查目标 IP 是否匹配组播地址 */
    if (ip->daddr != inet_addr(NET_TV_DISCOVERY_MCAST_ADDR)) return false;

    client_ip = ip->saddr;

    /* 跳过 IP 头 */
    ssize_t ip_hdr_len = ip->ihl * 4;
    const uint8_t* udp_data = ip_data + ip_hdr_len;
    ssize_t udp_len = ip_len - ip_hdr_len;
    if (udp_len < static_cast<ssize_t>(sizeof(struct udphdr))) return false;

    const auto* udp = reinterpret_cast<const struct udphdr*>(udp_data);
    uint16_t dst_port = ntohs(udp->dest);
    if (dst_port != NET_TV_DISCOVERY_MCAST_PORT) return false;

    client_port = ntohs(udp->source);

    /* UDP 负载 */
    ssize_t payload_len = udp_len - static_cast<ssize_t>(sizeof(struct udphdr));
    const uint8_t* payload = udp_data + sizeof(struct udphdr);
    if (payload_len <= 0) return false;

    udp_payload.assign(payload, payload + payload_len);
    return true;
}

/* ---- L2 response (Linux only, uses AF_PACKET raw socket) ---- */

static uint16_t ip_checksum(const void* data, size_t len)
{
    uint32_t sum = 0;
    const uint16_t* p = static_cast<const uint16_t*>(data);
    for (size_t i = 0; i < len / 2; ++i) {
        sum += p[i];
    }
    if (len & 1) {
        sum += static_cast<const uint8_t*>(data)[len - 1];
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 build_l2_response 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

std::vector<uint8_t> CDiscoveryResponder::build_l2_response(
    const uint8_t client_mac[6],
    uint32_t client_ip, uint16_t client_port,
    const NET_DiscoveryDeviceInfo_S& info) const
{
    std::string json = discovery::build_response_json(info);
    uint16_t udp_total_len = static_cast<uint16_t>(sizeof(struct udphdr) + json.size());
    uint16_t ip_total_len  = static_cast<uint16_t>(sizeof(struct iphdr) + udp_total_len);

    std::vector<uint8_t> frame(sizeof(struct ethhdr) + ip_total_len);
    uint8_t* p = frame.data();

    /* Ethernet */
    auto* eth = reinterpret_cast<struct ethhdr*>(p);
    std::memcpy(eth->h_dest,   client_mac,  6);
    std::memcpy(eth->h_source, m_aLocalMac,   6);
    eth->h_proto = htons(ETH_P_IP);
    p += sizeof(struct ethhdr);

    /* IP header */
    auto* ip = reinterpret_cast<struct iphdr*>(p);
    std::memset(ip, 0, sizeof(*ip));
    ip->version  = 4;
    ip->ihl      = 5;
    ip->tot_len  = htons(ip_total_len);
    ip->id       = htons(static_cast<uint16_t>(rand() & 0xFFFF));
    ip->ttl      = 64;
    ip->protocol = IPPROTO_UDP;
    ip->saddr    = m_uLocalIp;
    ip->daddr    = client_ip;
    ip->check    = 0;
    ip->check    = ip_checksum(ip, sizeof(struct iphdr));
    p += sizeof(struct iphdr);

    /* UDP header */
    auto* udp = reinterpret_cast<struct udphdr*>(p);
    udp->source = htons(NET_TV_DISCOVERY_MCAST_PORT);
    udp->dest   = htons(client_port);
    udp->len    = htons(udp_total_len);
    udp->check  = 0;
    p += sizeof(struct udphdr);

    /* payload */
    std::memcpy(p, json.data(), json.size());

    return frame;
}
#endif /* _WIN32 */

/* ---- standard UDP response ---- */

int CDiscoveryResponder::send_udp_response(uint32_t client_ip, uint16_t client_port,
                                           const NET_DiscoveryDeviceInfo_S& info)
{
    std::string json = discovery::build_response_json(info);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(client_port);
    addr.sin_addr.s_addr = client_ip;

    ssize_t ret = sendto(m_nUdpSocket, json.data(), json.size(), 0,
                         reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    return ret >= 0 ? 0 : -1;
}

/* ---- main receive loop ---- */

void CDiscoveryResponder::receive_thread()
{
#ifdef _WIN32
    /* Windows: 使用 UDP 组播接收，不支持 AF_PACKET raw socket */
    constexpr auto kProbeLogInterval = std::chrono::seconds(60);
    char buf[4096];
    int probe_count = 0;
    int invalid_probe_count = 0;
    int udp_ok_count = 0;
    int udp_fail_count = 0;
    auto last_probe_log = std::chrono::steady_clock::now();

    printf("[discovery-responder] thread started (Windows UDP multicast mode)\n");
    fflush(stdout);

    while (m_bRunning.load()) {
        struct sockaddr_in from_addr{};
        int from_len = sizeof(from_addr);
        int n = recvfrom(m_nUdpSocket, buf, sizeof(buf) - 1, 0,
                         reinterpret_cast<struct sockaddr*>(&from_addr), &from_len);
        if (n <= 0) {
            int err = WSAGetLastError();
            if (err == WSAEINTR || err == WSAEWOULDBLOCK) continue;
            fprintf(stderr, "discovery-responder: recvfrom failed, error=%d\n", err);
            break;
        }

        buf[n] = '\0';
        uint32_t client_ip = from_addr.sin_addr.s_addr;
        uint16_t client_port = ntohs(from_addr.sin_port);

        /* 校验探测包 */
        std::string probe_str(buf, n);
        if (!discovery::parse_probe_json(probe_str)) {
            invalid_probe_count++;
            continue;
        }

        probe_count++;

        /* 获取本机设备信息 */
        NET_DiscoveryDeviceInfo_S devInfo{};
        m_fnDeviceInfoCallback(&devInfo);

        /* UDP 单播回包 */
        int udp_ret = send_udp_response(client_ip, client_port, devInfo);
        if (udp_ret == 0) {
            udp_ok_count++;
        } else {
            udp_fail_count++;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_ip, ip_str, sizeof(ip_str));
            printf("[discovery-responder] udp response failed to %s:%d\n", ip_str, client_port);
            fflush(stdout);
        }

        auto now = std::chrono::steady_clock::now();
        if (now - last_probe_log >= kProbeLogInterval) {
            printf("[discovery-responder] summary probes=%d invalid=%d udp_ok=%d udp_fail=%d\n",
                   probe_count, invalid_probe_count, udp_ok_count, udp_fail_count);
            fflush(stdout);

            probe_count = 0;
            invalid_probe_count = 0;
            udp_ok_count = 0;
            udp_fail_count = 0;
            last_probe_log = now;
        }
    }

    printf("[discovery-responder] thread exiting (Windows)\n");
    fflush(stdout);
#else
    /* Linux: 使用 AF_PACKET raw socket 接收 */
    constexpr int kFilteredLogInterval = 5000;
    constexpr auto kProbeLogInterval = std::chrono::seconds(60);
    uint8_t buf[ETH_FRAME_LEN];
    int frame_count = 0;
    int drop_count = 0;
    int probe_count = 0;
    int invalid_probe_count = 0;
    int udp_ok_count = 0;
    int udp_fail_count = 0;
    int l2_ok_count = 0;
    int l2_fail_count = 0;
    auto last_probe_log = std::chrono::steady_clock::now();

    printf("[discovery-responder] thread started, listening on ifindex=%d\n", m_nInterfaceIndex);
    fflush(stdout);

    while (m_bRunning.load()) {
        ssize_t n = recvfrom(m_nRawFd, buf, sizeof(buf), 0, nullptr, nullptr);
        if (n <= 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            perror("discovery-responder: recvfrom");
            break;
        }

        frame_count++;

        std::vector<uint8_t> payload;
        uint8_t client_mac[6];
        uint32_t client_ip = 0;
        uint16_t client_port = 0;

        if (!parse_frame(buf, n, payload, client_mac, client_ip, client_port)) {
            drop_count++;
            /* 普通网卡流量很多，降低过滤统计日志频率，避免刷屏。 */
            if (drop_count % kFilteredLogInterval == 0) {
                printf("[discovery-responder] recv=%d filtered=%d\n", frame_count, drop_count);
                fflush(stdout);
            }
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_ip, ip_str, sizeof(ip_str));
        probe_count++;

        /* 校验探测包 */
        std::string probe_str(reinterpret_cast<const char*>(payload.data()), payload.size());
        if (!discovery::parse_probe_json(probe_str)) {
            invalid_probe_count++;
            continue;
        }

        /* 获取本机设备信息 */
        NET_DiscoveryDeviceInfo_S devInfo{};
        m_fnDeviceInfoCallback(&devInfo);

        /* 1. 标准 UDP 单播回包 */
        int udp_ret = send_udp_response(client_ip, client_port, devInfo);
        if (udp_ret == 0) {
            udp_ok_count++;
        } else {
            udp_fail_count++;
            printf("[discovery-responder] udp response failed to %s:%d\n", ip_str, client_port);
            fflush(stdout);
        }

        /* 2. AF_PACKET L2 回包（跨子网同广播域） */
        auto l2frame = build_l2_response(client_mac, client_ip, client_port, devInfo);

        struct sockaddr_ll sll{};
        sll.sll_family   = AF_PACKET;
        sll.sll_ifindex  = m_nInterfaceIndex;
        sll.sll_hatype   = 1;  /* ARPHRD_ETHER */
        sll.sll_halen    = ETH_ALEN;
        std::memcpy(sll.sll_addr, client_mac, ETH_ALEN);

        ssize_t l2_ret = sendto(m_nRawFd, l2frame.data(), l2frame.size(), 0,
                                reinterpret_cast<struct sockaddr*>(&sll), sizeof(sll));
        if (l2_ret >= 0) {
            l2_ok_count++;
        } else {
            l2_fail_count++;
            perror("discovery-responder: l2 sendto");
        }

        auto now = std::chrono::steady_clock::now();
        if (now - last_probe_log >= kProbeLogInterval) {
            printf("[discovery-responder] summary probes=%d invalid=%d udp_ok=%d udp_fail=%d l2_ok=%d l2_fail=%d last=%s:%d\n",
                   probe_count, invalid_probe_count,
                   udp_ok_count, udp_fail_count,
                   l2_ok_count, l2_fail_count,
                   ip_str, client_port);
            fflush(stdout);

            probe_count = 0;
            invalid_probe_count = 0;
            udp_ok_count = 0;
            udp_fail_count = 0;
            l2_ok_count = 0;
            l2_fail_count = 0;
            last_probe_log = now;
        }
    }

    printf("[discovery-responder] thread exiting, recv=%d filtered=%d\n", frame_count, drop_count);
    fflush(stdout);
#endif
}
