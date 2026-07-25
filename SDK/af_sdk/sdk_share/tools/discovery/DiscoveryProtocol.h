/**
 * @file DiscoveryProtocol.h
 * @brief 设备发现协议 JSON 组包 / 解包
 *
 * 探测包:  {"Probe":{"Type":"discovery"}}
 * 响应包:  {"Probe":{"DeviceName":"...","DeviceID":"...", ...}}
 */
#ifndef DISCOVERY_PROTOCOL_H_
#define DISCOVERY_PROTOCOL_H_

// 避免与合并后的 NetTVSDK.h 冲突
// 如果 NETTVSDK_COMMON_H 已定义（通过 NetTVSDK.h），则不再包含 NetTVSDKCommon.h
#ifndef NETTVSDK_COMMON_H
#include "NetTVSDKCommon.h"
#endif
#include <string>

namespace discovery {

/**
 * @brief 构建探测包 JSON 字符串
 * @return {"Probe":{"Type":"discovery"}}
 */
std::string build_probe_json();

/**
 * @brief 解析探测包，校验是否为合法发现请求
 * @param json_str 原始 JSON 字符串
 * @return true 合法探测包，false 非法
 */
bool parse_probe_json(const std::string& json_str);

/**
 * @brief 构建响应包 JSON 字符串
 * @param info 设备信息
 * @return 设备发现响应 JSON 字符串
 */
std::string build_response_json(const NET_DiscoveryDeviceInfo_S& info);

/**
 * @brief 解析响应包 JSON 字符串
 * @param json_str 原始 JSON 字符串
 * @param info 输出设备信息
 * @return true 解析成功，false 失败
 */
bool parse_response_json(const std::string& json_str,
                         NET_DiscoveryDeviceInfo_S& info);

}  // namespace discovery

#endif  // DISCOVERY_PROTOCOL_H_
