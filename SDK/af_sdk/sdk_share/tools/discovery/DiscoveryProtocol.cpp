/**
 * @file DiscoveryProtocol.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief DiscoveryProtocol 模块实现
 * 功能说明：
 * 1. 实现 DiscoveryProtocol 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */

#include "DiscoveryProtocol.h"

#include "Json.h"
#include "PlatformCompat.h"
#include <cstdio>
#include <cstring>

namespace discovery {

/* ---- 常量 ---- */
static const char* kKeyProbe  = "Probe";
static const char* kKeyType   = "Type";
static const char* kValDiscovery = "discovery";
static const char* kValSetNetwork = "set_network";
static const char* kKeyTargetIP = "TargetIP";
static const char* kKeySubnetMask = "SubnetMask";
static const char* kKeyGateway = "Gateway";
static const char* kKeySetGateway = "SetGateway";
static const char* kKeyIPv4DHCP = "IPv4DHCP";
static const char* kKeyDeviceName    = "DeviceName";
static const char* kKeyDeviceID      = "DeviceID";
static const char* kKeyDeviceType    = "DeviceType";
static const char* kKeyIPv4Address   = "IPv4Address";
static const char* kKeyIPv4SubnetMask = "IPv4SubnetMask";
static const char* kKeyIPv4Gateway   = "IPv4Gateway";
static const char* kKeyMACAddress    = "MACAddress";
static const char* kKeyFirmwareVersion = "FirmwareVersion";
static const char* kKeyHttpPort      = "HttpPort";
static const char* kKeyManufacturer  = "Manufacturer";
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 build_probe_json 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

std::string build_probe_json()
{
    Json::Object* root = Json::init();
    Json::Object* probe = Json::init();
    Json::add(probe, kKeyType, kValDiscovery);
    Json::add(root, kKeyProbe, probe);
    std::string result = Json::to_string(root);
    Json::deinit(root);
    return result;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 parse_probe_json 对应的数据。
 * @param [in] json_str 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool parse_probe_json(const std::string& json_str)
{
    if (json_str.empty()) return false;

    Json::Object* root = Json::init(json_str.c_str());
    if (!root) return false;

    Json::Object* probe = Json::get(root, kKeyProbe);
    std::string type;
    bool valid = probe && Json::get(probe, kKeyType, type) && type == kValDiscovery;
    Json::deinit(root);
    return valid;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 build_response_json 定义的内部处理。
 * @param [in] info 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string build_response_json(const NET_DiscoveryDeviceInfo_S& info)
{
    Json::Object* root = Json::init();
    Json::Object* probe = Json::init();

    Json::add(probe, kKeyType,           kValDiscovery);
    Json::add(probe, kKeyDeviceName,     info.strDeviceName);
    Json::add(probe, kKeyDeviceID,       info.strDeviceID);
    Json::add(probe, kKeyDeviceType,     info.strDeviceType);
    Json::add(probe, kKeyIPv4Address,    info.strIPv4Address);
    Json::add(probe, kKeyIPv4SubnetMask, info.strIPv4SubnetMask);
    Json::add(probe, kKeyIPv4Gateway,    info.strIPv4Gateway);
    Json::add(probe, kKeyMACAddress,     info.strMACAddress);
    Json::add(probe, kKeyFirmwareVersion,info.strFirmwareVersion);
    Json::add(probe, kKeyHttpPort,       static_cast<int>(info.uHttpPort));
    Json::add(probe, kKeyManufacturer,   info.strManufacturer);

    Json::add(root, kKeyProbe, probe);
    std::string result = Json::to_string(root);
    Json::deinit(root);
    return result;
}

namespace {

bool valid_ipv4(const std::string& value)
{
    struct in_addr address{};
    return !value.empty() && inet_pton(AF_INET, value.c_str(), &address) == 1;
}

bool valid_mac(const std::string& value)
{
    unsigned int bytes[6]{};
    char tail = '\0';
    int count = std::sscanf(value.c_str(), "%2x:%2x:%2x:%2x:%2x:%2x%c",
                            &bytes[0], &bytes[1], &bytes[2], &bytes[3],
                            &bytes[4], &bytes[5], &tail);
    if (count != 6) {
        count = std::sscanf(value.c_str(), "%2x-%2x-%2x-%2x-%2x-%2x%c",
                            &bytes[0], &bytes[1], &bytes[2], &bytes[3],
                            &bytes[4], &bytes[5], &tail);
    }
    if (count != 6) return false;
    for (unsigned int byte : bytes) {
        if (byte > 0xff) return false;
    }
    return true;
}

bool fits_field(const std::string& value, size_t capacity)
{
    return value.size() < capacity;
}

}  // namespace

std::string build_set_network_json(const NET_PoeNetworkConfig_S& config)
{
    Json::Object* root = Json::init();
    Json::Object* request = Json::init();
    Json::add(request, kKeyType, kValSetNetwork);
    Json::add(request, kKeyMACAddress, config.szMACAddress);
    Json::add(request, kKeyTargetIP, config.szTargetIP);
    Json::add(request, kKeySubnetMask, config.szSubnetMask);
    Json::add(request, kKeyGateway, config.szGateway);
    Json::add(request, kKeySetGateway, config.bSetGateway != 0);
    Json::add(request, kKeyIPv4DHCP, config.bIPv4DHCP != 0);
    Json::add(root, kKeyProbe, request);
    std::string result = Json::to_string(root);
    Json::deinit(root);
    return result;
}

bool parse_set_network_json(const std::string& json_str,
                            NET_PoeNetworkConfig_S& config)
{
    if (json_str.empty()) return false;

    Json::Object* root = Json::init(json_str.c_str());
    if (!root) return false;
    Json::Object* request = Json::get(root, kKeyProbe);
    std::string type;
    std::string mac;
    std::string target_ip;
    std::string subnet_mask;
    std::string gateway;
    bool set_gateway = false;
    bool dhcp = false;

    bool valid = request &&
        Json::get(request, kKeyType, type) && type == kValSetNetwork &&
        Json::get(request, kKeyMACAddress, mac) &&
        Json::get(request, kKeyTargetIP, target_ip) &&
        Json::get(request, kKeySubnetMask, subnet_mask) &&
        Json::get(request, kKeySetGateway, set_gateway) &&
        Json::get(request, kKeyIPv4DHCP, dhcp);

    if (valid) {
        valid = valid_mac(mac) && valid_ipv4(target_ip) &&
                valid_ipv4(subnet_mask) &&
                fits_field(mac, sizeof(config.szMACAddress)) &&
                fits_field(target_ip, sizeof(config.szTargetIP)) &&
                fits_field(subnet_mask, sizeof(config.szSubnetMask));
    }

    if (valid && Json::get(request, kKeyGateway, gateway)) {
        valid = (!set_gateway || valid_ipv4(gateway)) &&
                fits_field(gateway, sizeof(config.szGateway));
    } else if (valid && set_gateway) {
        valid = false;
    }

    if (valid) {
        std::memset(&config, 0, sizeof(config));
        std::snprintf(config.szMACAddress, sizeof(config.szMACAddress), "%s", mac.c_str());
        std::snprintf(config.szTargetIP, sizeof(config.szTargetIP), "%s", target_ip.c_str());
        std::snprintf(config.szSubnetMask, sizeof(config.szSubnetMask), "%s", subnet_mask.c_str());
        if (!gateway.empty()) {
            std::snprintf(config.szGateway, sizeof(config.szGateway), "%s", gateway.c_str());
        }
        config.bSetGateway = set_gateway ? TRUE : FALSE;
        config.bIPv4DHCP = dhcp ? TRUE : FALSE;
    }
    Json::deinit(root);
    return valid;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 parse_response_json 对应的数据。
 * @return 返回该处理的状态或结果。
 */

bool parse_response_json(const std::string& json_str,
                         NET_DiscoveryDeviceInfo_S& info)
{
    if (json_str.empty()) return false;

    Json::Object* root = Json::init(json_str.c_str());
    if (!root) return false;

    Json::Object* probe = Json::get(root, kKeyProbe);
    if (!probe) { Json::deinit(root); return false; }

    std::string type;
    if (!Json::get(probe, kKeyType, type) || type != kValDiscovery) {
        Json::deinit(root); return false;
    }

    std::memset(&info, 0, sizeof(info));

    auto safe_str = [&](const char* key, char* dst, size_t max_len) {
        std::string val;
        if (Json::get(probe, key, val)) {
            std::snprintf(dst, max_len, "%s", val.c_str());
        }
    };

    int port = 0;
    Json::get(probe, kKeyHttpPort, port);
    info.uHttpPort = static_cast<UINT32>(port > 0 ? port : 0);

    safe_str(kKeyDeviceName,     info.strDeviceName,      sizeof(info.strDeviceName));
    safe_str(kKeyDeviceID,       info.strDeviceID,        sizeof(info.strDeviceID));
    safe_str(kKeyDeviceType,     info.strDeviceType,      sizeof(info.strDeviceType));
    safe_str(kKeyIPv4Address,    info.strIPv4Address,     sizeof(info.strIPv4Address));
    safe_str(kKeyIPv4SubnetMask, info.strIPv4SubnetMask,  sizeof(info.strIPv4SubnetMask));
    safe_str(kKeyIPv4Gateway,    info.strIPv4Gateway,     sizeof(info.strIPv4Gateway));
    safe_str(kKeyMACAddress,     info.strMACAddress,      sizeof(info.strMACAddress));
    safe_str(kKeyFirmwareVersion,info.strFirmwareVersion, sizeof(info.strFirmwareVersion));
    safe_str(kKeyManufacturer,   info.strManufacturer,    sizeof(info.strManufacturer));

    Json::deinit(root);
    return true;
}

}  /* namespace discovery */
