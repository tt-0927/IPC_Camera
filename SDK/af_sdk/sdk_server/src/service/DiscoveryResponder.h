/**
 * @file DiscoveryResponder.h
 * @brief 设备发现服务端 — AF_PACKET 接收 + UDP/L2 双路回包
 *
 * 收到客户端探测包后，同时通过:
 *   1. 标准 UDP 单播回包（覆盖同网段 + 路由可达场景）
 *   2. AF_PACKET L2 以太网帧回包（覆盖同广播域跨子网场景）
 */
#ifndef DISCOVERY_RESPONDER_H_
#define DISCOVERY_RESPONDER_H_

#ifndef NETTVSDK_COMMON_H
#include "NetTVSDKCommon.h"
#endif

#include "PlatformCompat.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief 设备信息回调类型
 * @param pInfo [OUT] 由回调填充设备信息
 */
using DiscoveryDeviceInfoCallback =
    std::function<void(NET_TV_DISCOVERY_DEVICE_INFO_S* pInfo)>;

class DiscoveryResponder {
public:
    DiscoveryResponder() = default;
    ~DiscoveryResponder() { stop(); }

    /**
     * @brief 初始化，获取本机 MAC 等信息
     * @param szInterfaceName 网卡名称 (如 "eth0")
     * @return 0 成功，<0 失败
     */
    int init(const char* szInterfaceName);

    /**
     * @brief 设置设备信息回调
     */
    void set_device_info_callback(DiscoveryDeviceInfoCallback cb) {
        m_deviceInfoCb = std::move(cb);
    }

    /**
     * @brief 启动响应线程
     * @return 0 成功，<0 失败
     */
    int start();

    /**
     * @brief 停止响应线程
     */
    void stop();

    bool is_running() const { return m_running.load(); }

private:
    void receive_thread();

    /* AF_PACKET 原始套接字操作 */
    bool init_raw_socket();
    void close_raw_socket();

    /* 解析原始帧，提取 UDP 负载和客户端地址 */
    bool parse_frame(const uint8_t* data, ssize_t len,
                     std::vector<uint8_t>& udp_payload,
                     uint8_t client_mac[6],
                     uint32_t& client_ip, uint16_t& client_port);

    /* 构建 L2 以太网响应帧 */
    std::vector<uint8_t> build_l2_response(
        const uint8_t client_mac[6],
        uint32_t client_ip, uint16_t client_port,
        const NET_TV_DISCOVERY_DEVICE_INFO_S& info) const;

    /* 标准 UDP 单播回包 */
    int send_udp_response(uint32_t client_ip, uint16_t client_port,
                          const NET_TV_DISCOVERY_DEVICE_INFO_S& info);

    int m_rawFd{-1};
    int m_ifindex{0};
    uint8_t m_localMac[6]{};
    std::string m_ifaceName;
    uint32_t m_localIP{0};

    int m_udpSock{-1};   /* UDP 回包套接字 */
    int m_igmpSock{-1};   /* IGMP 组播加入套接字，触发交换机 IGMP Snooping 转发 */

    DiscoveryDeviceInfoCallback m_deviceInfoCb;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
};

#endif  // DISCOVERY_RESPONDER_H_
