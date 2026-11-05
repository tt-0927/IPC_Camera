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
#include <cctype>

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
    Json::Object* pRoot = Json::init();
    Json::Object* pProbe = Json::init();
    Json::add(pProbe, kKeyType, kValDiscovery);
    Json::add(pRoot, kKeyProbe, pProbe);
    std::string strResult = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return strResult;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 parse_probe_json 对应的数据。
 * @param [in] strJson 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool parse_probe_json(const std::string& strJson)
{
    if (strJson.empty()) return false;

    Json::Object* pRoot = Json::init(strJson.c_str());
    if (!pRoot) return false;

    Json::Object* pProbe = Json::get(pRoot, kKeyProbe);
    std::string strType;
    bool bValid = pProbe && Json::get(pProbe, kKeyType, strType) && strType == kValDiscovery;
    Json::deinit(pRoot);
    return bValid;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 build_response_json 定义的内部处理。
 * @param [in] stInfo 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string build_response_json(const NET_DiscoveryDeviceInfo_S& stInfo)
{
    Json::Object* pRoot = Json::init();
    Json::Object* pProbe = Json::init();

    Json::add(pProbe, kKeyType,           kValDiscovery);
    Json::add(pProbe, kKeyDeviceName,     stInfo.strDeviceName);
    Json::add(pProbe, kKeyDeviceID,       stInfo.strDeviceID);
    Json::add(pProbe, kKeyDeviceType,     stInfo.strDeviceType);
    Json::add(pProbe, kKeyIPv4Address,    stInfo.strIPv4Address);
    Json::add(pProbe, kKeyIPv4SubnetMask, stInfo.strIPv4SubnetMask);
    Json::add(pProbe, kKeyIPv4Gateway,    stInfo.strIPv4Gateway);
    Json::add(pProbe, kKeyMACAddress,     stInfo.strMACAddress);
    Json::add(pProbe, kKeyFirmwareVersion,stInfo.strFirmwareVersion);
    Json::add(pProbe, kKeyHttpPort,       static_cast<int>(stInfo.uHttpPort));
    Json::add(pProbe, kKeyManufacturer,   stInfo.strManufacturer);

    Json::add(pRoot, kKeyProbe, pProbe);
    std::string strResult = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return strResult;
}

namespace {

bool valid_ipv4(const std::string& strValue)
{
    struct in_addr stAddress{};
    return !strValue.empty() && inet_pton(AF_INET, strValue.c_str(), &stAddress) == 1;
}

bool valid_mac(const std::string& strValue)
{
    unsigned int auBytes[6]{};
    char chTail = '\0';
    int nCount = std::sscanf(strValue.c_str(), "%2x:%2x:%2x:%2x:%2x:%2x%c",
                            &auBytes[0], &auBytes[1], &auBytes[2], &auBytes[3],
                            &auBytes[4], &auBytes[5], &chTail);
    if (nCount != 6) {
        nCount = std::sscanf(strValue.c_str(), "%2x-%2x-%2x-%2x-%2x-%2x%c",
                            &auBytes[0], &auBytes[1], &auBytes[2], &auBytes[3],
                            &auBytes[4], &auBytes[5], &chTail);
    }
    if (nCount != 6) return false;
    for (unsigned int uByte : auBytes) {
        if (uByte > 0xff) return false;
    }
    return true;
}

bool fits_field(const std::string& strValue, size_t uCapacity)
{
    return strValue.size() < uCapacity;
}

}  // namespace

std::string build_set_network_json(const NET_PoeNetworkConfig_S& stConfig)
{
    Json::Object* pRoot = Json::init();
    Json::Object* pRequest = Json::init();
    Json::add(pRequest, kKeyType, kValSetNetwork);
    Json::add(pRequest, kKeyMACAddress, stConfig.strMACAddress);
    Json::add(pRequest, kKeyTargetIP, stConfig.strTargetIP);
    Json::add(pRequest, kKeySubnetMask, stConfig.strSubnetMask);
    Json::add(pRequest, kKeyGateway, stConfig.strGateway);
    Json::add(pRequest, kKeySetGateway, stConfig.bSetGateway != 0);
    Json::add(pRequest, kKeyIPv4DHCP, stConfig.bIPv4DHCP != 0);
    Json::add(pRoot, kKeyProbe, pRequest);
    std::string strResult = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return strResult;
}

bool parse_set_network_json(const std::string& strJson,
                            NET_PoeNetworkConfig_S& stConfig)
{
    if (strJson.empty()) return false;

    Json::Object* pRoot = Json::init(strJson.c_str());
    if (!pRoot) return false;
    Json::Object* pRequest = Json::get(pRoot, kKeyProbe);
    std::string strType;
    std::string strMac;
    std::string strTargetIp;
    std::string strSubnetMask;
    std::string strGateway;
    bool bSetGateway = false;
    bool bDhcp = false;

    bool bValid = pRequest &&
        Json::get(pRequest, kKeyType, strType) && strType == kValSetNetwork &&
        Json::get(pRequest, kKeyMACAddress, strMac) &&
        Json::get(pRequest, kKeyTargetIP, strTargetIp) &&
        Json::get(pRequest, kKeySubnetMask, strSubnetMask) &&
        Json::get(pRequest, kKeySetGateway, bSetGateway) &&
        Json::get(pRequest, kKeyIPv4DHCP, bDhcp);

    if (bValid) {
        bValid = valid_mac(strMac) && valid_ipv4(strTargetIp) &&
                valid_ipv4(strSubnetMask) &&
                fits_field(strMac, sizeof(stConfig.strMACAddress)) &&
                fits_field(strTargetIp, sizeof(stConfig.strTargetIP)) &&
                fits_field(strSubnetMask, sizeof(stConfig.strSubnetMask));
    }

    if (bValid && Json::get(pRequest, kKeyGateway, strGateway)) {
        bValid = valid_ipv4(strGateway) &&
                fits_field(strGateway, sizeof(stConfig.strGateway));
    } else if (bValid && bSetGateway) {
        bValid = false;
    }

    if (bValid) {
        std::memset(&stConfig, 0, sizeof(stConfig));
        std::snprintf(stConfig.strMACAddress, sizeof(stConfig.strMACAddress), "%s", strMac.c_str());
        std::snprintf(stConfig.strTargetIP, sizeof(stConfig.strTargetIP), "%s", strTargetIp.c_str());
        std::snprintf(stConfig.strSubnetMask, sizeof(stConfig.strSubnetMask), "%s", strSubnetMask.c_str());
        if (!strGateway.empty()) {
            std::snprintf(stConfig.strGateway, sizeof(stConfig.strGateway), "%s", strGateway.c_str());
        }
        stConfig.bSetGateway = bSetGateway ? 1 : 0;
        stConfig.bIPv4DHCP = bDhcp ? 1 : 0;
    }
    Json::deinit(pRoot);
    return bValid;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 parse_response_json 对应的数据。
 * @return 返回该处理的状态或结果。
 */

bool parse_response_json(const std::string& strJson,
                         NET_DiscoveryDeviceInfo_S& stInfo)
{
    if (strJson.empty()) return false;

    Json::Object* pRoot = Json::init(strJson.c_str());
    if (!pRoot) return false;

    Json::Object* pProbe = Json::get(pRoot, kKeyProbe);
    if (!pProbe) { Json::deinit(pRoot); return false; }

    std::string strType;
    if (!Json::get(pProbe, kKeyType, strType) || strType != kValDiscovery) {
        Json::deinit(pRoot); return false;
    }

    std::memset(&stInfo, 0, sizeof(stInfo));

    auto fnSafeStr = [&](const char* pstrKey, char* pstrDst, size_t uMaxLen) {
        std::string strVal;
        if (Json::get(pProbe, pstrKey, strVal)) {
            std::snprintf(pstrDst, uMaxLen, "%s", strVal.c_str());
        }
    };

    int nPort = 0;
    Json::get(pProbe, kKeyHttpPort, nPort);
    stInfo.uHttpPort = static_cast<UINT32>(nPort > 0 ? nPort : 0);

    fnSafeStr(kKeyDeviceName,     stInfo.strDeviceName,      sizeof(stInfo.strDeviceName));
    fnSafeStr(kKeyDeviceID,       stInfo.strDeviceID,        sizeof(stInfo.strDeviceID));
    fnSafeStr(kKeyDeviceType,     stInfo.strDeviceType,      sizeof(stInfo.strDeviceType));
    fnSafeStr(kKeyIPv4Address,    stInfo.strIPv4Address,     sizeof(stInfo.strIPv4Address));
    fnSafeStr(kKeyIPv4SubnetMask, stInfo.strIPv4SubnetMask,  sizeof(stInfo.strIPv4SubnetMask));
    fnSafeStr(kKeyIPv4Gateway,    stInfo.strIPv4Gateway,     sizeof(stInfo.strIPv4Gateway));
    fnSafeStr(kKeyMACAddress,     stInfo.strMACAddress,      sizeof(stInfo.strMACAddress));
    fnSafeStr(kKeyFirmwareVersion,stInfo.strFirmwareVersion, sizeof(stInfo.strFirmwareVersion));
    fnSafeStr(kKeyManufacturer,   stInfo.strManufacturer,    sizeof(stInfo.strManufacturer));

    Json::deinit(pRoot);
    return true;
}

}  /* namespace discovery */
