/**
 * @file DiscoverySearcher.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CDiscoverySearcher 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CDiscoverySearcher 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */

#ifndef NETSDK_DISCOVERY_SEARCHER_H
#define NETSDK_DISCOVERY_SEARCHER_H

#ifndef NETSDK_COMMON_H
#include "NetTVSDKCommon.h"
#endif
#include "PlatformCompat.h"
#include <string>
#include <vector>

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设备发现搜索器
 *
 * 使用 UDP 组播发送探测包，收集设备响应。
 * 同网段和路由可达场景都能覆盖。
 */
class CDiscoverySearcher {
public:
    CDiscoverySearcher()  = default;
    ~CDiscoverySearcher() = default;

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 搜索局域网内设备
     * @param szInterfaceIP   网卡 IP，NULL 用默认路由
     * @param dwTimeoutMs     等待响应的超时时间 (ms)，建议 2000~5000
     * @param pDeviceList     输出设备列表（调用方分配）
     * @param nMaxCount       设备列表最大容量
     * @param pnOutCount      输出实际发现设备数
     * @return 0 成功，<0 失败
     */
    int search(const char* szInterfaceIP,
               UINT32 dwTimeoutMs,
               NET_TV_DISCOVERY_DEVICE_INFO_S* pDeviceList,
               int nMaxCount,
               int* pnOutCount);

private:
    int create_socket(const char* szInterfaceIP);
    void close_socket();
    int send_probes();
    int recv_responses(UINT32 dwTimeoutMs,
                       NET_TV_DISCOVERY_DEVICE_INFO_S* pDeviceList,
                       int nMaxCount,
                       int* pnOutCount);

    /* 用 device_id 去重 */
    bool is_duplicate(const NET_TV_DISCOVERY_DEVICE_INFO_S& info,
                      const NET_TV_DISCOVERY_DEVICE_INFO_S* pList,
                      int nCount) const;

    socket_fd_t m_nSocket{INVALID_SOCKET_FD};
    std::string m_strInterfaceIp;
};

#endif  /* NETSDK_DISCOVERY_SEARCHER_H */
