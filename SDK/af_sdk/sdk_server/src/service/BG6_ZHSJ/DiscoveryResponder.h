/**
 * @file CDiscoveryResponder.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CDiscoveryResponder 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CDiscoveryResponder 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */

#ifndef NETSDK_DISCOVERY_RESPONDER_H
#define NETSDK_DISCOVERY_RESPONDER_H

#ifndef NETSDK_COMMON_H
#include "NetTVSDKCommon.h"
#endif

#include "PlatformCompat.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设备信息回调类型
 * @param pInfo [out] 由回调填充设备信息
 */
using DiscoveryDeviceInfoCallback =
    std::function<void(NET_DiscoveryDeviceInfo_S* pInfo)>;

using DiscoverySetNetworkCallback =
    std::function<bool(const NET_PoeNetworkConfig_S& config)>;

class CDiscoveryResponder {
public:
    CDiscoveryResponder() = default;
    ~CDiscoveryResponder() { stop(); }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 初始化，获取本机 MAC 等信息
     * @param szInterfaceName 网卡名称 (如 "eth0")
     * @return 0 成功，<0 失败
     */
    int init(const char* szInterfaceName);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置设备信息回调
     */
    void set_device_info_callback(DiscoveryDeviceInfoCallback cb) {
        m_fnDeviceInfoCallback = std::move(cb);
    }

    void set_network_callback(DiscoverySetNetworkCallback cb) {
        m_fnSetNetworkCallback = std::move(cb);
    }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 启动响应线程
     * @return 0 成功，<0 失败
     */
    int start();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 停止响应线程
     */
    void stop();

    bool is_running() const { return m_bRunning.load(); }

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
        const NET_DiscoveryDeviceInfo_S& info) const;

    /* 标准 UDP 单播回包 */
    int send_udp_response(uint32_t client_ip, uint16_t client_port,
                          const NET_DiscoveryDeviceInfo_S& info);

    int m_nRawFd{-1};
    int m_nInterfaceIndex{0};
    uint8_t m_aLocalMac[6]{};
    std::string m_strInterfaceName;
    uint32_t m_uLocalIp{0};

    socket_fd_t m_nUdpSocket{INVALID_SOCKET_FD}; /* UDP 回包套接字 */
    socket_fd_t m_nIgmpSocket{INVALID_SOCKET_FD}; /* IGMP 组播加入套接字，触发交换机 IGMP Snooping 转发 */

    DiscoveryDeviceInfoCallback m_fnDeviceInfoCallback;
    DiscoverySetNetworkCallback m_fnSetNetworkCallback;
    std::thread m_stThread;
    std::atomic<bool> m_bRunning{false};
};

#endif  /* NETSDK_DISCOVERY_RESPONDER_H */
