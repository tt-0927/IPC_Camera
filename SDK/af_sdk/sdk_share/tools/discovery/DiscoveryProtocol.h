/**
 * @file DiscoveryProtocol.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DiscoveryProtocol 模块接口与类型定义
 * 功能说明：
 * 1. 声明 DiscoveryProtocol 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */

#ifndef NETSDK_DISCOVERY_PROTOCOL_H
#define NETSDK_DISCOVERY_PROTOCOL_H

/* 避免与合并后的 NetTVSDK.h 冲突 */
/* 如果 NETSDK_COMMON_H 已定义（通过 NetTVSDK.h），则不再包含 NetTVSDKCommon.h */
#ifndef NETSDK_COMMON_H
#include "NetTVSDKCommon.h"
#endif
#include <string>

namespace discovery {

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 构建探测包 JSON 字符串
 * @return {"Probe":{"Type":"discovery"}}
 */
std::string build_probe_json();

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 解析探测包，校验是否为合法发现请求
 * @param json_str 原始 JSON 字符串
 * @return true 合法探测包，false 非法
 */
bool parse_probe_json(const std::string& json_str);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 构建响应包 JSON 字符串
 * @param info 设备信息
 * @return 设备发现响应 JSON 字符串
 */
std::string build_response_json(const NET_DiscoveryDeviceInfo_S& info);

/**
 * @brief 构建免登录网络配置组播 JSON。
 * @param [in] config 目标设备及网络配置。
 * @return 可直接作为组播负载发送的 JSON 字符串。
 */
std::string build_set_network_json(const NET_PoeNetworkConfig_S& config);

/**
 * @brief 解析并校验免登录网络配置组播 JSON。
 * @param [in] json_str 原始 JSON 字符串。
 * @param [out] config 解析后的目标设备及网络配置。
 * @return true 表示解析和参数校验成功，false 表示报文非法。
 */
bool parse_set_network_json(const std::string& json_str,
                            NET_PoeNetworkConfig_S& config);

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 解析响应包 JSON 字符串
 * @param json_str 原始 JSON 字符串
 * @param info 输出设备信息
 * @return true 解析成功，false 失败
 */
bool parse_response_json(const std::string& json_str,
                         NET_DiscoveryDeviceInfo_S& info);

}  /* namespace discovery */

#endif  /* NETSDK_DISCOVERY_PROTOCOL_H */
