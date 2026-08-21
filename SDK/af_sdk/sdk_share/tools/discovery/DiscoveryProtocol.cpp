/**
 * @file DiscoveryProtocol.cpp
 * @brief 设备发现协议 JSON 序列化实现
 */
#include "DiscoveryProtocol.h"

#include "Json.h"
#include <cstdio>
#include <cstring>

namespace discovery {

/* ---- 常量 ---- */
static const char* kKeyProbe  = "Probe";
static const char* kKeyType   = "Type";
static const char* kValDiscovery = "discovery";
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

std::string build_response_json(const NET_TV_DISCOVERY_DEVICE_INFO_S& info)
{
    Json::Object* root = Json::init();
    Json::Object* probe = Json::init();

    Json::add(probe, kKeyType,           kValDiscovery);
    Json::add(probe, kKeyDeviceName,     info.szDeviceName);
    Json::add(probe, kKeyDeviceID,       info.szDeviceID);
    Json::add(probe, kKeyDeviceType,     info.szDeviceType);
    Json::add(probe, kKeyIPv4Address,    info.szIPv4Address);
    Json::add(probe, kKeyIPv4SubnetMask, info.szIPv4SubnetMask);
    Json::add(probe, kKeyIPv4Gateway,    info.szIPv4Gateway);
    Json::add(probe, kKeyMACAddress,     info.szMACAddress);
    Json::add(probe, kKeyFirmwareVersion,info.szFirmwareVersion);
    Json::add(probe, kKeyHttpPort,       static_cast<int>(info.dwHttpPort));
    Json::add(probe, kKeyManufacturer,   info.szManufacturer);

    Json::add(root, kKeyProbe, probe);
    std::string result = Json::to_string(root);
    Json::deinit(root);
    return result;
}

bool parse_response_json(const std::string& json_str,
                         NET_TV_DISCOVERY_DEVICE_INFO_S& info)
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
    info.dwHttpPort = static_cast<UINT32>(port > 0 ? port : 0);

    safe_str(kKeyDeviceName,     info.szDeviceName,      sizeof(info.szDeviceName));
    safe_str(kKeyDeviceID,       info.szDeviceID,        sizeof(info.szDeviceID));
    safe_str(kKeyDeviceType,     info.szDeviceType,      sizeof(info.szDeviceType));
    safe_str(kKeyIPv4Address,    info.szIPv4Address,     sizeof(info.szIPv4Address));
    safe_str(kKeyIPv4SubnetMask, info.szIPv4SubnetMask,  sizeof(info.szIPv4SubnetMask));
    safe_str(kKeyIPv4Gateway,    info.szIPv4Gateway,     sizeof(info.szIPv4Gateway));
    safe_str(kKeyMACAddress,     info.szMACAddress,      sizeof(info.szMACAddress));
    safe_str(kKeyFirmwareVersion,info.szFirmwareVersion, sizeof(info.szFirmwareVersion));
    safe_str(kKeyManufacturer,   info.szManufacturer,    sizeof(info.szManufacturer));

    Json::deinit(root);
    return true;
}

}  // namespace discovery
