/**
 * @FilePath     : platform_manager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-08 17:44:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-29 14:24:16
 * @Description  : 平台管理
 */

#if CAP_GARBAGE_STATION_PLATFORM
#include "platform_manager.h"
#include "platform_register_crypto.h"
#include "mqtt_sdk_gateway.h"
#include "platform_sdk_adapter.h"
#include "httplib.h"
#include "path_define.h"
#include "convert_interface.h"
#include "push_stream.h"
#include "rtsp_server.h"
#include "av_configure.h"
#include "network_manage.h"
#include "storage_manage.h"
#include "user_manage.h"
#include "IpcRet.h"
#include "dlog.h"
#include "mqtt_manager.h"
#include "mqtt_topic_define.h"
#include "system_define.h"
#include "system_manage.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <vector>
#include <chrono>
#include <cerrno>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <openssl/crypto.h>



namespace
{
constexpr const char *MQTT_DEVICE_REGISTER_COMMAND = "NET_DEVICE_REGISTER";
constexpr const char *PLATFORM_REGISTER_CREDENTIAL_KEY_ID = "platform-rsa-2026-08";
constexpr const char *PLATFORM_REGISTER_CREDENTIAL_PUBLIC_KEY = CA_MQTT_TRUST_KEY "platform_register_public.pem";

void cleanse_string(std::string &strValue)
{
    if (!strValue.empty())
    {
        OPENSSL_cleanse(&strValue[0], strValue.size());
        strValue.clear();
    }
}

std::string escape_json_string(const std::string &strValue)
{
    static const char HEX_DIGITS[] = "0123456789abcdef";
    std::string strEscaped;
    strEscaped.reserve(strValue.size());

    for (const unsigned char ch : strValue)
    {
        switch (ch)
        {
        case '"':
            strEscaped += "\\\"";
            break;
        case '\\':
            strEscaped += "\\\\";
            break;
        case '\b':
            strEscaped += "\\b";
            break;
        case '\f':
            strEscaped += "\\f";
            break;
        case '\n':
            strEscaped += "\\n";
            break;
        case '\r':
            strEscaped += "\\r";
            break;
        case '\t':
            strEscaped += "\\t";
            break;
        default:
            if (ch < 0x20)
            {
                strEscaped += "\\u00";
                strEscaped += HEX_DIGITS[(ch >> 4) & 0x0f];
                strEscaped += HEX_DIGITS[ch & 0x0f];
            }
            else
            {
                strEscaped += static_cast<char>(ch);
            }
            break;
        }
    }

    return strEscaped;
}

std::string build_register_credential_plaintext(const std::string &strRtspUrl,
                                                const std::string &strUsername,
                                                const std::string &strPassword)
{
    return "{\"RtspUrl\":\"" + escape_json_string(strRtspUrl) +
           "\",\"Username\":\"" + escape_json_string(strUsername) +
           "\",\"Password\":\"" + escape_json_string(strPassword) + "\"}";
}

std::string build_register_credential_aad(const std::string &strDeviceSn,
                                          const std::string &strRequestId,
                                          long long nTimestampMs,
                                          const char *pUplinkType,
                                          const char *pStreamMode)
{
    std::ostringstream oss;
    oss << MQTT_DEVICE_REGISTER_COMMAND << "|1|" << strDeviceSn << "|" << strRequestId << "|" << nTimestampMs << "|"
        << (pUplinkType ? pUplinkType : "") << "|" << (pStreamMode ? pStreamMode : "");
    return oss.str();
}

bool get_platform_route_interface(const std::string &strHost,
                                  int nPort,
                                  std::string &strInterface,
                                  std::string &strLocalIp)
{
    strInterface.clear();
    strLocalIp.clear();

    if (strHost.empty() || nPort <= 0 || nPort > 65535)
    {
        return false;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *pResult = nullptr;
    const std::string strPort = std::to_string(nPort);
    if (getaddrinfo(strHost.c_str(), strPort.c_str(), &hints, &pResult) != 0)
    {
        dlog_warn("无法解析 MQTT Broker 地址，无法识别取流出口: host[%s]", strHost.c_str());
        return false;
    }

    struct sockaddr_in stLocalAddr;
    memset(&stLocalAddr, 0, sizeof(stLocalAddr));
    bool bFoundRoute = false;

    for (struct addrinfo *pAddr = pResult; pAddr != nullptr; pAddr = pAddr->ai_next)
    {
        const int nSocket = socket(pAddr->ai_family, pAddr->ai_socktype, pAddr->ai_protocol);
        if (nSocket < 0)
        {
            continue;
        }

        if (connect(nSocket, pAddr->ai_addr, pAddr->ai_addrlen) == 0)
        {
            socklen_t nAddrLen = sizeof(stLocalAddr);
            if (getsockname(nSocket, reinterpret_cast<struct sockaddr *>(&stLocalAddr), &nAddrLen) == 0)
            {
                bFoundRoute = true;
            }
        }
        close(nSocket);

        if (bFoundRoute)
        {
            break;
        }
    }
    freeaddrinfo(pResult);

    if (!bFoundRoute)
    {
        dlog_warn("无法获取到 MQTT Broker 的路由出口: host[%s], port[%d]", strHost.c_str(), nPort);
        return false;
    }

    char achIp[INET_ADDRSTRLEN] = {0};
    if (inet_ntop(AF_INET, &stLocalAddr.sin_addr, achIp, sizeof(achIp)) == nullptr)
    {
        return false;
    }
    strLocalIp = achIp;

    struct ifaddrs *pInterfaces = nullptr;
    if (getifaddrs(&pInterfaces) != 0)
    {
        dlog_warn("无法枚举网络接口，无法识别取流出口: ip[%s]", strLocalIp.c_str());
        return false;
    }

    for (struct ifaddrs *pIfa = pInterfaces; pIfa != nullptr; pIfa = pIfa->ifa_next)
    {
        if (pIfa->ifa_addr == nullptr || pIfa->ifa_addr->sa_family != AF_INET)
        {
            continue;
        }

        const struct sockaddr_in *pIfAddr = reinterpret_cast<const struct sockaddr_in *>(pIfa->ifa_addr);
        if (pIfAddr->sin_addr.s_addr == stLocalAddr.sin_addr.s_addr)
        {
            strInterface = pIfa->ifa_name;
            break;
        }
    }
    freeifaddrs(pInterfaces);
    return !strInterface.empty();
}

bool is_wireless_uplink_interface(const std::string &strInterface)
{
    return strInterface.compare(0, 2, "wl") == 0 ||
           strInterface.compare(0, 4, "wwan") == 0 ||
           strInterface.compare(0, 3, "ppp") == 0 ||
           strInterface.compare(0, 5, "rmnet") == 0 ||
           strInterface.compare(0, 3, "usb") == 0;
}

std::string build_rtmp_main_url(const std::string &strHost, int nPort, const std::string &strDeviceSn)
{
    if (strHost.empty() || strDeviceSn.empty() || nPort <= 0 || nPort > 65535)
    {
        return std::string();
    }

    return "rtmp://" + strHost + ":" + std::to_string(nPort) + "/live/" + strDeviceSn + "-main";
}
} // namespace

// --- Base64 编码实现 ---
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

static const char *FACE_NV21_UPLOAD_DIR = "/opt/course/upload";

/*
 * Convert::read_file() 只检查文件是否存在且非空，JSON 解析失败时仍可能保留结构体默认值。
 * 网络 重连必须避免把损坏的配置文件误当作默认平台配置，因此先严格检查字段类型后再转换。
 */
static int read_platform_config_for_reconnect(const std::string &path,
                                              Network::Platform_Info_t &stInfo)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        return ERR_FREAD;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string jsonData = buffer.str();
    if (jsonData.empty())
    {
        return ERR_FREAD;
    }

    cJSON *pRoot = cJSON_Parse(jsonData.c_str());
    if (pRoot == nullptr || !cJSON_IsObject(pRoot))
    {
        if (pRoot != nullptr)
        {
            cJSON_Delete(pRoot);
        }
        return ERR_PARSE;
    }

    cJSON *pServerIp = cJSON_GetObjectItemCaseSensitive(pRoot, "server_ip");
    cJSON *pServerPort = cJSON_GetObjectItemCaseSensitive(pRoot, "server_port");
    cJSON *pRtmpPort = cJSON_GetObjectItemCaseSensitive(pRoot, "RtmpPort");
    cJSON *pMqttPort = cJSON_GetObjectItemCaseSensitive(pRoot, "MqttPort");
    cJSON *pUser = cJSON_GetObjectItemCaseSensitive(pRoot, "user");
    cJSON *pPassword = cJSON_GetObjectItemCaseSensitive(pRoot, "password");
    cJSON *pEnable = cJSON_GetObjectItemCaseSensitive(pRoot, "enable");
    cJSON *pCustom = cJSON_GetObjectItemCaseSensitive(pRoot, "Custom");

    const bool bFieldsValid =
        cJSON_IsString(pServerIp) && cJSON_IsNumber(pServerPort) &&
        cJSON_IsNumber(pRtmpPort) && cJSON_IsNumber(pMqttPort) &&
        cJSON_IsString(pUser) && cJSON_IsString(pPassword) &&
        cJSON_IsBool(pEnable) && cJSON_IsBool(pCustom);
    cJSON_Delete(pRoot);

    if (!bFieldsValid)
    {
        return ERR_PARSE;
    }

    Convert::to_struct(jsonData, stInfo);
    return OK;
}

static bool file_exists(const std::string &path)
{
    return !path.empty() && access(path.c_str(), F_OK) == 0;
}

static long long file_size(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        return -1;
    }
    return (long long)st.st_size;
}

static std::string dirname_of(const std::string &path)
{
    const size_t pos = path.find_last_of('/');
    if (pos == std::string::npos)
    {
        return ".";
    }
    if (pos == 0)
    {
        return "/";
    }
    return path.substr(0, pos);
}

static std::string basename_of(const std::string &path)
{
    const size_t queryPos = path.find_first_of("?#");
    const std::string cleanPath = (queryPos == std::string::npos) ? path : path.substr(0, queryPos);
    const size_t pos = cleanPath.find_last_of("/\\");
    if (pos == std::string::npos)
    {
        return cleanPath;
    }
    return cleanPath.substr(pos + 1);
}

static bool ensure_directory(const std::string &dir)
{
    if (dir.empty())
    {
        return false;
    }
    if (dir == "/")
    {
        return true;
    }

    std::string current;
    size_t pos = 0;
    if (dir[0] == '/')
    {
        current = "/";
        pos = 1;
    }

    while (pos <= dir.size())
    {
        size_t next = dir.find('/', pos);
        std::string part = dir.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
        if (!part.empty())
        {
            if (!current.empty() && current.back() != '/')
            {
                current += '/';
            }
            current += part;

            struct stat st;
            if (stat(current.c_str(), &st) != 0)
            {
                if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST)
                {
                    return false;
                }
            }
            else if (!S_ISDIR(st.st_mode))
            {
                return false;
            }
        }

        if (next == std::string::npos)
        {
            break;
        }
        pos = next + 1;
    }

    return true;
}

static bool get_json_string(cJSON *pObj, const char *key, std::string &out)
{
    cJSON *pItem = cJSON_GetObjectItemCaseSensitive(pObj, key);
    if (cJSON_IsString(pItem) && pItem->valuestring)
    {
        out = pItem->valuestring;
        return true;
    }
    return false;
}

static bool get_json_int(cJSON *pObj, const char *key, int &out)
{
    cJSON *pItem = cJSON_GetObjectItemCaseSensitive(pObj, key);
    if (cJSON_IsNumber(pItem))
    {
        out = pItem->valueint;
        return true;
    }
    return false;
}

static std::string normalize_mqtt_command(const std::string &command)
{
    std::string result;
    result.reserve(command.size());
    for (char ch : command)
    {
        unsigned char uch = static_cast<unsigned char>(ch);
        if (!std::isspace(uch))
        {
            result.push_back(static_cast<char>(std::toupper(uch)));
        }
    }

    /* 新 SDK 使用 NET_* 命令名，兼容平台仍可能下发的历史 NET_TV_* 命令。 */
    static const std::string kLegacyPrefix = "NET_TV_";
    if (result.compare(0, kLegacyPrefix.size(), kLegacyPrefix) == 0)
    {
        result.replace(0, kLegacyPrefix.size(), "NET_");
    }

    return result;
}

static void set_json_string(cJSON *pObj, const char *key, const std::string &value)
{
    cJSON *pItem = cJSON_GetObjectItemCaseSensitive(pObj, key);
    if (cJSON_IsString(pItem))
    {
        cJSON_SetValuestring(pItem, value.c_str());
    }
    else if (pItem)
    {
        cJSON_ReplaceItemInObjectCaseSensitive(pObj, key, cJSON_CreateString(value.c_str()));
    }
    else
    {
        cJSON_AddStringToObject(pObj, key, value.c_str());
    }
}

static void set_json_int(cJSON *pObj, const char *key, int value)
{
    cJSON *pItem = cJSON_GetObjectItemCaseSensitive(pObj, key);
    if (pItem)
    {
        cJSON_ReplaceItemInObjectCaseSensitive(pObj, key, cJSON_CreateNumber(value));
    }
    else
    {
        cJSON_AddNumberToObject(pObj, key, value);
    }
}

static std::string make_error_json(const std::string &error)
{
    cJSON *pRoot = cJSON_CreateObject();
    if (!pRoot)
    {
        return "{\"error\":\"unknown\"}";
    }

    cJSON_AddStringToObject(pRoot, "error", error.c_str());
    char *pJson = cJSON_PrintUnformatted(pRoot);
    std::string strPayload = pJson ? pJson : "{\"error\":\"unknown\"}";
    if (pJson)
    {
        free(pJson);
    }
    cJSON_Delete(pRoot);
    return strPayload;
}

static bool read_binary_file(const std::string &strPath, std::string &strContent)
{
    strContent.clear();
    std::ifstream ifs(strPath.c_str(), std::ios::binary);
    if (!ifs.is_open())
    {
        return false;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    strContent = oss.str();
    return ifs.good() || ifs.eof();
}

/* 文件名会进入HTTP multipart和平台存储，这里只保留安全字符，避免SN或外部传参中带路径分隔符 */
static std::string sanitize_filename_part(const std::string &strValue)
{
    std::string strResult;
    strResult.reserve(strValue.size());
    for (char ch : strValue)
    {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch) || ch == '-' || ch == '_' || ch == '.')
        {
            strResult.push_back(ch);
        }
        else
        {
            strResult.push_back('_');
        }
    }
    return strResult.empty() ? "unknown" : strResult;
}

/* 平台上报统一使用毫秒时间戳；事件时间缺失时退化为设备当前时间 */
static std::string make_timestamp_ms_tag(long long llTimestampMs)
{
    if (llTimestampMs <= 0)
    {
        llTimestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    }

    return std::to_string(llTimestampMs);
}

static bool is_digits_string(const std::string &strValue)
{
    if (strValue.empty())
    {
        return false;
    }

    for (char ch : strValue)
    {
        if (!std::isdigit(static_cast<unsigned char>(ch)))
        {
            return false;
        }
    }
    return true;
}

/* 本地抓拍文件名格式：yyyyMMdd_HHmmssSSS_事件类型_序号.jpg，上传平台时转成Unix毫秒时间戳 */
static bool parse_capture_timestamp_ms_from_path(const std::string &strImagePath, long long &llTimestampMs)
{
    llTimestampMs = 0;
    const std::string strName = basename_of(strImagePath);
    if (strName.size() < 19 || strName[8] != '_' || strName[18] != '_')
    {
        return false;
    }

    const std::string strDate = strName.substr(0, 8);
    const std::string strTime = strName.substr(9, 9);
    if (!is_digits_string(strDate) || !is_digits_string(strTime))
    {
        return false;
    }

    const int nYear = std::atoi(strDate.substr(0, 4).c_str());
    const int nMonth = std::atoi(strDate.substr(4, 2).c_str());
    const int nDay = std::atoi(strDate.substr(6, 2).c_str());
    const int nHour = std::atoi(strTime.substr(0, 2).c_str());
    const int nMin = std::atoi(strTime.substr(2, 2).c_str());
    const int nSec = std::atoi(strTime.substr(4, 2).c_str());
    const int nMs = std::atoi(strTime.substr(6, 3).c_str());

    if (nYear < 1970 || nMonth < 1 || nMonth > 12 || nDay < 1 || nDay > 31 ||
        nHour < 0 || nHour > 23 || nMin < 0 || nMin > 59 || nSec < 0 || nSec > 59 ||
        nMs < 0 || nMs > 999)
    {
        return false;
    }

    struct tm tmValue;
    std::memset(&tmValue, 0, sizeof(tmValue));
    tmValue.tm_year = nYear - 1900;
    tmValue.tm_mon = nMonth - 1;
    tmValue.tm_mday = nDay;
    tmValue.tm_hour = nHour;
    tmValue.tm_min = nMin;
    tmValue.tm_sec = nSec;
    tmValue.tm_isdst = -1;

    const std::time_t seconds = std::mktime(&tmValue);
    if (seconds < 0)
    {
        return false;
    }

    struct tm tmCheck;
    localtime_r(&seconds, &tmCheck);
    if (tmCheck.tm_year != tmValue.tm_year || tmCheck.tm_mon != tmValue.tm_mon ||
        tmCheck.tm_mday != tmValue.tm_mday || tmCheck.tm_hour != tmValue.tm_hour ||
        tmCheck.tm_min != tmValue.tm_min || tmCheck.tm_sec != tmValue.tm_sec)
    {
        return false;
    }

    llTimestampMs = static_cast<long long>(seconds) * 1000LL + nMs;
    return true;
}

/* 默认命名规则：设备SN_事件类型_毫秒时间戳_序号.jpg */
static std::string build_event_image_file_name(const std::string &strDeviceSn,
                                               int nEventType,
                                               const std::string &strEventTimeTag)
{
    std::ostringstream oss;
    oss << sanitize_filename_part(strDeviceSn) << "_" << nEventType << "_"
        << strEventTimeTag << "_1.jpg";
    return oss.str();
}

static void parse_event_image_upload_response(const std::string &strBody,
                                              CPlatformManager::EventImageUploadResponse &outResponse)
{
    cJSON *pRoot = cJSON_Parse(strBody.c_str());
    if (!pRoot)
    {
        return;
    }

    cJSON *pStatusCode = cJSON_GetObjectItemCaseSensitive(pRoot, "status_code");
    if (cJSON_IsNumber(pStatusCode))
    {
        outResponse.status_code = pStatusCode->valueint;
    }

    cJSON *pStatus = cJSON_GetObjectItemCaseSensitive(pRoot, "status");
    if (cJSON_IsString(pStatus) && pStatus->valuestring)
    {
        outResponse.status = pStatus->valuestring;
    }

    cJSON *pMessage = cJSON_GetObjectItemCaseSensitive(pRoot, "message");
    if (cJSON_IsString(pMessage) && pMessage->valuestring)
    {
        outResponse.message = pMessage->valuestring;
    }

    cJSON *pData = cJSON_GetObjectItemCaseSensitive(pRoot, "data");
    if (cJSON_IsObject(pData))
    {
        get_json_string(pData, "url", outResponse.image_url);
        if (outResponse.image_url.empty())
        {
            get_json_string(pData, "image_url", outResponse.image_url);
        }
        if (outResponse.image_url.empty())
        {
            get_json_string(pData, "ImageUrl", outResponse.image_url);
        }
        if (outResponse.image_url.empty())
        {
            get_json_string(pData, "path", outResponse.image_url);
        }
        if (outResponse.image_url.empty())
        {
            get_json_string(pData, "Path", outResponse.image_url);
        }
        if (outResponse.image_path.empty())
        {
            get_json_string(pData, "image_path", outResponse.image_path);
        }
        if (outResponse.file_name.empty())
        {
            get_json_string(pData, "file_name", outResponse.file_name);
        }
    }
    else if (cJSON_IsString(pData) && pData->valuestring)
    {
        outResponse.image_url = pData->valuestring;
    }

    if (outResponse.image_url.empty())
    {
        get_json_string(pRoot, "url", outResponse.image_url);
    }
    if (outResponse.image_url.empty())
    {
        get_json_string(pRoot, "image_url", outResponse.image_url);
    }
    if (outResponse.image_url.empty())
    {
        get_json_string(pRoot, "path", outResponse.image_url);
    }

    cJSON_Delete(pRoot);
}

static bool is_event_image_upload_business_ok(const CPlatformManager::EventImageUploadResponse &stResponse,
                                              int nHttpStatus)
{
    return stResponse.status_code == 0 || stResponse.status_code == 200 || stResponse.status_code == nHttpStatus;
}

static bool extract_task_response(const std::string &strTaskResult, int &nReturn, std::string &strData)
{
    cJSON *pRoot = cJSON_Parse(strTaskResult.c_str());
    if (!pRoot)
    {
        return false;
    }

    bool bParsed = false;
    cJSON *pReturn = cJSON_GetObjectItemCaseSensitive(pRoot, "Return");
    if (cJSON_IsNumber(pReturn))
    {
        nReturn = pReturn->valueint;
        bParsed = true;
    }

    cJSON *pData = cJSON_GetObjectItemCaseSensitive(pRoot, "Data");
    if (pData)
    {
        char *pJson = cJSON_PrintUnformatted(pData);
        if (pJson)
        {
            strData = pJson;
            free(pJson);
        }
    }
    else
    {
        strData = "{}";
    }

    cJSON_Delete(pRoot);
    return bParsed;
}

std::string CPlatformManager::base64_encode(const std::string &input)
{
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t in_len = input.size();
    const char *bytes_to_encode = input.c_str();

    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

/**
 * @brief 登录实现
 * @param host 服务器地址 (例如 "183.129.224.253")
 * @param port 服务器端口 (例如 4910)
 * @param user 用户名
 * @param password 密码
 * @param out_response 返回的响应结构体
 */
bool CPlatformManager::login(const std::string &host,
                            int port,
                            const std::string &user,
                            const std::string &password,
                            const bool &enable,
                            const bool &Custom,
                            LoginResponse &out_response,
                            bool bIgnoreEnable)
{
    bool result = false;
    g_custom = Custom;
    g_enable = enable;

    if (!enable && !bIgnoreEnable)
    {
        return result;
    }
    // 1. 准备数据
    std::string encoded_password = base64_encode(password);
    // 构建表单数据: user=admin&password=QWF...
    std::string body_str = "user=" + user + "&password=" + encoded_password;
    // 2. 发起请求
    std::string target_host = Custom ? host : host_;
    int target_port = Custom ? port : port_;
    httplib::Client cli(target_host, target_port);
    custom_host = host;
    custom_post = port;
    cli.set_connection_timeout(5, 0);

    // 设置请求头
    httplib::Headers headers = {
        { "Content-Type", "application/x-www-form-urlencoded" }
    };

    // 【关键修正】这里必须传入第四个参数 "application/x-www-form-urlencoded"
    // 否则编译器会混淆，以为你传的是 Params 类型的 Map
    auto res = cli.Post(login_path_, headers, body_str, "application/x-www-form-urlencoded");
    if (res)
    {
        dlog_info("平台登录响应: %s", res->body.c_str());
    }
    else
    {
        dlog_error("平台登录请求失败: 无响应");
    }
    // 3. 处理响应
    if (res && res->status == 200)
    {
        cJSON *response_root = cJSON_Parse(res->body.c_str());

        if (response_root)
        {
            // 解析 JSON (保持不变)
            cJSON *status_code_item = cJSON_GetObjectItemCaseSensitive(response_root, "status_code");
            cJSON *status_item = cJSON_GetObjectItemCaseSensitive(response_root, "status");
            cJSON *message_item = cJSON_GetObjectItemCaseSensitive(response_root, "message");

            if (cJSON_IsNumber(status_code_item))
                out_response.status_code = status_code_item->valueint;
            if (cJSON_IsString(status_item))
                out_response.status = status_item->valuestring;
            if (cJSON_IsString(message_item))
                out_response.message = message_item->valuestring;

            cJSON *data_item = cJSON_GetObjectItemCaseSensitive(response_root, "data");
            if (data_item && cJSON_IsObject(data_item))
            {
                cJSON *access_token = cJSON_GetObjectItemCaseSensitive(data_item, "access_token");
                cJSON *token_type = cJSON_GetObjectItemCaseSensitive(data_item, "token_type");
                cJSON *expires_in = cJSON_GetObjectItemCaseSensitive(data_item, "expires_in");
                cJSON *phone = cJSON_GetObjectItemCaseSensitive(data_item, "phone");
                cJSON *need_modify = cJSON_GetObjectItemCaseSensitive(data_item, "need_modify_password");

                if (cJSON_IsString(access_token))
                {
                    out_response.data.access_token = access_token->valuestring;
                    this->access_token_ = access_token->valuestring;
                }

                if (cJSON_IsString(token_type))
                {
                    this->token_type_ = token_type->valuestring;
                    out_response.data.token_type = token_type->valuestring;
                }
                if (cJSON_IsNumber(expires_in))
                    out_response.data.expires_in = expires_in->valueint;
                if (cJSON_IsString(phone))
                    out_response.data.phone = phone->valuestring;
                if (cJSON_IsBool(need_modify))
                    out_response.data.need_modify_password = (bool) need_modify->valueint;
            }

            if (out_response.status == "success")
            {
                login_user = user;
                login_password = password;
                result = true;
                /* 登录成功，持久化配置 */
                save_config();
            }

            cJSON_Delete(response_root);
        }
    }
    else
    {
        if (res)
        {
            dlog_error("平台登录 HTTP 错误: %d", res->status);
        }
        else
        {
            dlog_error("平台登录连接错误: %s:%d", target_host.c_str(), target_port);
        }
    }

    return result;
}

bool CPlatformManager::apply_platform_config(const ::Network::Platform_Info_t &stInfo)
{
    const auto is_valid_port = [](int nPort) {
        return nPort > 0 && nPort <= 65535;
    };

    /* 关闭平台时允许保留已有参数；启用自定义平台时必须提供完整连接参数。 */
    if (stInfo.enable && stInfo.Custom &&
        (stInfo.server_ip.empty() || !is_valid_port(stInfo.server_port) ||
         !is_valid_port(stInfo.rtmp_port) || !is_valid_port(stInfo.mqtt_port) ||
         stInfo.user.empty() || stInfo.password.empty()))
    {
        dlog_error("平台配置参数无效: host=%s, httpPort=%d, rtmpPort=%d, mqttPort=%d, userEmpty=%d",
                   stInfo.server_ip.c_str(),
                   stInfo.server_port,
                   stInfo.rtmp_port,
                   stInfo.mqtt_port,
                   stInfo.user.empty());
        return false;
    }

    /* SDK owns every platform connection and must accept the complete configuration first. */
    if (CPlatformSdkAdapter::instance()->apply_config(stInfo) != OK)
    {
        dlog_error("SDK 平台模块拒绝当前配置");
        return false;
    }

    /* 网页的四类参数统一写入运行时状态，供 HTTP、MQTT、RTMP 三条链路使用。 */
    custom_host = stInfo.server_ip;
    custom_post = stInfo.server_port;
    m_nRtmpPort = stInfo.rtmp_port;
    m_nMqttPort = stInfo.mqtt_port;
    login_user = stInfo.user;
    login_password = stInfo.password;
    g_enable = stInfo.enable;
    g_custom = stInfo.Custom;
    /* 网页保存新配置后，不再沿用本次启动的强制连接状态。 */
    m_bBootConnectOverride.store(false);
    return true;
}

void CPlatformManager::getlogininfo(::Network::LoginInfo &retLoginInfo)
{

    retLoginInfo.login_password = login_password;
    retLoginInfo.login_user = login_user;
    retLoginInfo.enable = g_enable;
    retLoginInfo.Custom = g_custom;
    retLoginInfo.host = g_custom ? custom_host : host_;
    retLoginInfo.port = g_custom ? custom_post : port_;
}

void CPlatformManager::getplatforminfo(::Network::Platform_Info_t &retPlatformIfo)
{
    Network::Platform_Info_t stInfo;
    if (Convert::read_file(PLATFORM_CONFIG_FILE, stInfo) != OK)
    {
        dlog_warn("读取平台配置文件失败: %s", PLATFORM_CONFIG_FILE);
    }
    retPlatformIfo = stInfo;
}

std::string CPlatformManager::get_access_token() const
{
    return access_token_;
}

bool CPlatformManager::storeDevice(const StoreDevice &device, const std::string &token, StoreResponse &out_response)
{
    // 1. 构建 JSON 请求体
    cJSON *json_root = cJSON_CreateObject();

    // 必须字段
    cJSON_AddStringToObject(json_root, "sn", device.sn.c_str());
    cJSON_AddStringToObject(json_root, "name", device.name.c_str());
    cJSON_AddStringToObject(json_root, "ip", device.ip.c_str());
    cJSON_AddNumberToObject(json_root, "port", device.port);
    if (device.account.empty())
    {
        cJSON_AddStringToObject(json_root, "account", login_user.c_str());
    }
    else
    {
        cJSON_AddStringToObject(json_root, "account", device.account.c_str());
    }
    if (device.password.empty())
    {
        cJSON_AddStringToObject(json_root, "password", login_password.c_str());
    }
    else
    {
        cJSON_AddStringToObject(json_root, "password", device.password.c_str());
    }
    cJSON_AddStringToObject(json_root, "protocol", device.protocol.c_str());
    cJSON_AddStringToObject(json_root, "resolution", device.resolution.c_str());
    cJSON_AddStringToObject(json_root, "storage", device.storage.c_str());
    cJSON_AddStringToObject(json_root, "use_storage", device.use_storage.c_str());
    cJSON_AddNumberToObject(json_root, "type_id", 1);

    // 可选字段
    if (!device.version.empty())
        cJSON_AddStringToObject(json_root, "version", device.version.c_str());
    if (!device.mac_address.empty())
        cJSON_AddStringToObject(json_root, "mac_address", device.mac_address.c_str());
    if (!device.live_url.empty())
        cJSON_AddStringToObject(json_root, "live_url", device.live_url.c_str());
    if (!device.playback_url.empty())
        cJSON_AddStringToObject(json_root, "playback_url", device.playback_url.c_str());

    // 序列化 JSON
    char *json_string = cJSON_PrintUnformatted(json_root);
    std::string body_str = json_string;
    cJSON_free(json_string);
    cJSON_Delete(json_root);

    // 2. 发起 HTTP 请求
    const std::string target_host = g_custom ? custom_host : host_;
    const int target_port = g_custom ? custom_post : port_;
    httplib::Client cli(target_host, target_port);
    cli.set_connection_timeout(5, 0);

    // 设置 Headers
    httplib::Headers headers = {
        {  "Content-Type", "application/json" },
        { "Authorization",  "Bearer " + token }  // 拼接 Bearer
    };

    dlog_info("平台注册设备请求: %s", body_str.c_str());
    auto res = cli.Post(store_path_, headers, body_str, "application/json");
    if (res)
    {
        dlog_info("平台注册设备响应: %s", res->body.c_str());
    }
    else
    {
        dlog_error("平台注册设备请求失败: 无响应");
    }

    // 3. 解析响应
    if (res && res->status == 200)
    {

        cJSON *response_root = cJSON_Parse(res->body.c_str());

        if (response_root)
        {

            // 基本信息
            cJSON *status_code_item = cJSON_GetObjectItemCaseSensitive(response_root, "status_code");
            cJSON *status_item = cJSON_GetObjectItemCaseSensitive(response_root, "status");
            cJSON *message_item = cJSON_GetObjectItemCaseSensitive(response_root, "message");

            if (cJSON_IsNumber(status_code_item))
                out_response.status_code = status_code_item->valueint;
            if (cJSON_IsString(status_item))
                out_response.status = status_item->valuestring;
            if (cJSON_IsString(message_item))
                out_response.message = message_item->valuestring;
            if (out_response.status_code != 200)
            {
                dlog_error("平台注册设备 HTTP 错误: %d, message: %s", out_response.status_code, out_response.message.c_str());
                cJSON_Delete(response_root);
                return false;
            }
            // Data 数据
            cJSON *data_item = cJSON_GetObjectItemCaseSensitive(response_root, "data");
            if (data_item && cJSON_IsObject(data_item))
            {
                cJSON *sn = cJSON_GetObjectItemCaseSensitive(data_item, "sn");
                cJSON *name = cJSON_GetObjectItemCaseSensitive(data_item, "name");
                cJSON *version = cJSON_GetObjectItemCaseSensitive(data_item, "version");
                cJSON *status = cJSON_GetObjectItemCaseSensitive(data_item, "status");
                cJSON *mac = cJSON_GetObjectItemCaseSensitive(data_item, "mac_address");
                cJSON *account = cJSON_GetObjectItemCaseSensitive(data_item, "account");
                cJSON *password = cJSON_GetObjectItemCaseSensitive(data_item, "password");
                cJSON *live = cJSON_GetObjectItemCaseSensitive(data_item, "live_url");
                cJSON *playback = cJSON_GetObjectItemCaseSensitive(data_item, "playback_url");
                cJSON *updated = cJSON_GetObjectItemCaseSensitive(data_item, "updated_at");
                cJSON *created = cJSON_GetObjectItemCaseSensitive(data_item, "created_at");
                cJSON *id = cJSON_GetObjectItemCaseSensitive(data_item, "id");
                cJSON *uuid = cJSON_GetObjectItemCaseSensitive(data_item, "device_uuid");

                if (cJSON_IsString(sn))
                    out_response.data.sn = sn->valuestring;
                if (cJSON_IsString(name))
                    out_response.data.name = name->valuestring;
                if (cJSON_IsString(version))
                    out_response.data.version = version->valuestring;
                if (cJSON_IsNumber(status))
                    out_response.data.status = status->valueint;
                if (cJSON_IsString(mac))
                    out_response.data.mac_address = mac->valuestring;
                if (cJSON_IsString(account))
                    out_response.data.account = account->valuestring;
                if (cJSON_IsString(password))
                    out_response.data.password = password->valuestring;
                if (cJSON_IsString(live))
                    out_response.data.live_url = live->valuestring;
                if (cJSON_IsString(playback))
                    out_response.data.playback_url = playback->valuestring;
                if (cJSON_IsString(updated))
                    out_response.data.updated_at = updated->valuestring;
                if (cJSON_IsString(created))
                    out_response.data.created_at = created->valuestring;
                if (cJSON_IsNumber(id))
                    out_response.data.id = id->valueint;
                if (cJSON_IsString(uuid))
                    out_response.data.device_uuid = uuid->valuestring;
            }

            cJSON_Delete(response_root);
            return true;
        }
    }
    else
    {
        if (res)
        {
            dlog_error("平台注册设备 HTTP 错误: %d", res->status);
        }
        else
        {
            dlog_error("平台注册设备连接失败: %s:%d", target_host.c_str(), target_port);
        }
    }
    return false;
}

/**
 * @brief 上报工单实现
 */
bool CPlatformManager::reportWorkOrder(const WorkOrderRequest &workOrder,
                                       const std::string &token,
                                       WorkOrderResponse &out_response)
{
    // 1. 构建 JSON 请求体
    cJSON *json_root = cJSON_CreateObject();

    // 非必须字段，但如果有值则添加
    if (!workOrder.device_sn.empty())
        cJSON_AddStringToObject(json_root, "device_sn", workOrder.device_sn.c_str());
    if (workOrder.type != 0)
        cJSON_AddNumberToObject(json_root, "type", workOrder.type);
    if (!workOrder.images.empty())
        cJSON_AddStringToObject(json_root, "images", workOrder.images.c_str());
    if (!workOrder.online.empty())
        cJSON_AddStringToObject(json_root, "online", workOrder.online.c_str());

    char *json_string = cJSON_PrintUnformatted(json_root);
    std::string body_str = json_string;
    cJSON_free(json_string);
    cJSON_Delete(json_root);

    // 2. 发起 HTTP 请求
    const std::string target_host = g_custom ? custom_host : host_;
    const int target_port = g_custom ? custom_post : port_;
    httplib::Client cli(target_host, target_port);
    cli.set_connection_timeout(5, 0);

    httplib::Headers headers = {
        {  "Content-Type", "application/json" },
        { "Authorization",  "Bearer " + token }
    };

    auto res = cli.Post(workorder_path_, headers, body_str, "application/json");

    // 3. 解析响应
    if (res && res->status == 200)
    {
        cJSON *response_root = cJSON_Parse(res->body.c_str());
        if (response_root)
        {
            cJSON *code_item = cJSON_GetObjectItemCaseSensitive(response_root, "status_code");
            cJSON *status_item = cJSON_GetObjectItemCaseSensitive(response_root, "status");
            cJSON *msg_item = cJSON_GetObjectItemCaseSensitive(response_root, "message");
            cJSON *data_item = cJSON_GetObjectItemCaseSensitive(response_root, "data");

            if (cJSON_IsNumber(code_item))
                out_response.status_code = code_item->valueint;
            if (cJSON_IsString(status_item))
                out_response.status = status_item->valuestring;
            if (cJSON_IsString(msg_item))
                out_response.message = msg_item->valuestring;
            if (cJSON_IsNumber(data_item))
                out_response.data = data_item->valueint;

            cJSON_Delete(response_root);
            return true;
        }
    }
    else
    {
        if (res)
        {
            dlog_error("平台注册设备 HTTP 错误: %d", res->status);
        }
        else
        {
            dlog_error("平台注册设备连接失败: %s:%d", target_host.c_str(), target_port);
        }
    }
    return false;
}

bool CPlatformManager::upload_event_image(const EventImageUploadRequest &request,
                                          EventImageUploadResponse &out_response)
{
    if (request.image_path.empty())
    {
        dlog_error("事件图片上传失败：图片路径为空");
        return false;
    }

    /* 调用方通常只传事件和图片路径；设备SN在平台登录/MQTT初始化后由管理类兜底补齐 */
    std::string strDeviceSn = request.device_sn.empty() ? m_strMqttClientId : request.device_sn;
    if (strDeviceSn.empty())
    {
        System::DeviceInfo_S stDeviceInfo;
        SystemManage::instance()->get_device_info(stDeviceInfo);
        strDeviceSn = stDeviceInfo.serialNumber;
    }

    long long llUploadTimestamp = 0;
    if (parse_capture_timestamp_ms_from_path(request.image_path, llUploadTimestamp))
    {
        dlog_info("事件图片上传使用抓拍文件时间戳：path[%s], timestamp[%lld]",
                  request.image_path.c_str(),
                  llUploadTimestamp);
    }
    else
    {
        llUploadTimestamp = request.timestamp;
        dlog_warn("事件图片上传解析抓拍文件名时间失败，使用事件时间戳兜底：path[%s], timestamp[%lld]",
                  request.image_path.c_str(),
                  llUploadTimestamp);
    }

    const std::string strEventTimeTag = make_timestamp_ms_tag(llUploadTimestamp);
    const std::string strFileName = request.file_name.empty()
                                        ? build_event_image_file_name(strDeviceSn, request.event_type, strEventTimeTag)
                                        : sanitize_filename_part(request.file_name);
    out_response.file_name = strFileName;
    out_response.image_path = request.image_path;

    if (!file_exists(request.image_path))
    {
        dlog_error("事件图片上传失败：图片不存在[%s]", request.image_path.c_str());
        return false;
    }

    if (access_token_.empty())
    {
        dlog_error("事件图片上传失败：平台 access_token 为空");
        return false;
    }

    /* 图片本体按二进制读入，由httplib生成multipart/form-data，避免base64带来的体积膨胀 */
    std::string strImageContent;
    if (!read_binary_file(request.image_path, strImageContent) || strImageContent.empty())
    {
        dlog_error("事件图片上传失败：读取图片失败[%s]", request.image_path.c_str());
        return false;
    }

    const std::string target_host = g_custom ? custom_host : host_;
    const int target_port = g_custom ? custom_post : port_;

    httplib::Headers headers = {
        {"Authorization", "Bearer " + access_token_}
    };

    const std::string strUploadTime = request.time.empty() ? strEventTimeTag : request.time;
    const std::string strEventType = std::to_string(request.event_type);

    dlog_info("开始上传事件图片：path[%s], file[%s], size[%zu], event[%d], sn[%s]",
              request.image_path.c_str(),
              strFileName.c_str(),
              strImageContent.size(),
              request.event_type,
              strDeviceSn.c_str());

    httplib::Client cli(target_host, target_port);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(30, 0);

    /* 平台upload_screen接口文档要求的字段：device_sn/event_type/time/up_screen */
    httplib::MultipartFormDataItems items = {
        {"device_sn", strDeviceSn, "", ""},
        {"event_type", strEventType, "", ""},
        {"time", strUploadTime, "", ""},
        {"up_screen", strImageContent, strFileName, "image/jpeg"}
    };

    dlog_info("事件图片上传尝试：field[up_screen], event_type[%s], time[%s], url[%s:%d%s]",
              strEventType.c_str(),
              strUploadTime.c_str(),
              target_host.c_str(),
              target_port,
              event_image_upload_path_.c_str());

    auto res = cli.Post(event_image_upload_path_, headers, items);
    if (!res)
    {
        dlog_error("事件图片上传失败：平台无响应[%s:%d%s]",
                   target_host.c_str(),
                   target_port,
                   event_image_upload_path_.c_str());
        return false;
    }

    out_response.status_code = res->status;
    out_response.file_name = strFileName;
    out_response.image_path = request.image_path;
    parse_event_image_upload_response(res->body, out_response);

    if (res->status < 200 || res->status >= 300)
    {
        dlog_error("事件图片上传 HTTP 失败：event_type[%s], status[%d], body[%s]",
                   strEventType.c_str(),
                   res->status,
                   res->body.c_str());
        return false;
    }

    if (!is_event_image_upload_business_ok(out_response, res->status))
    {
        dlog_error("事件图片上传业务失败：event_type[%s], status_code[%d], message[%s], body[%s]",
                   strEventType.c_str(),
                   out_response.status_code,
                   out_response.message.c_str(),
                   res->body.c_str());
        return false;
    }

    dlog_info("事件图片上传成功：field[up_screen], event_type[%s], file[%s], url[%s]",
              strEventType.c_str(),
              out_response.file_name.c_str(),
              out_response.image_url.c_str());
    return true;
}

/**
 * @brief 获取设备列表实现
 */
/**
 * @brief 获取设备列表实现
 */
bool CPlatformManager::getDeviceList(const std::string &token, DeviceListResponse &out_response)
{
    // 1. 发起 GET 请求
    const std::string target_host = g_custom ? custom_host : host_;
    const int target_port = g_custom ? custom_post : port_;
    httplib::Client cli(target_host, target_port);
    cli.set_connection_timeout(5, 0);

    httplib::Headers headers = {
        { "Authorization", "Bearer " + token }
    };

    auto res = cli.Get(device_list_path_, headers);

    // 2. 处理响应
    if (res && res->status == 200)
    {
        cJSON *response_root = cJSON_Parse(res->body.c_str());
        if (response_root)
        {
            // --- 解析顶层信息 ---
            cJSON *code_item = cJSON_GetObjectItemCaseSensitive(response_root, "status_code");
            cJSON *status_item = cJSON_GetObjectItemCaseSensitive(response_root, "status");
            cJSON *msg_item = cJSON_GetObjectItemCaseSensitive(response_root, "message");

            if (cJSON_IsNumber(code_item))
                out_response.status_code = code_item->valueint;
            if (cJSON_IsString(status_item))
                out_response.status = status_item->valuestring;
            if (cJSON_IsString(msg_item))
                out_response.message = msg_item->valuestring;

            // --- 解析 data 数组 ---
            cJSON *data_array = cJSON_GetObjectItemCaseSensitive(response_root, "data");
            if (cJSON_IsArray(data_array))
            {
                int size = cJSON_GetArraySize(data_array);
                for (int i = 0; i < size; ++i)
                {
                    cJSON *item = cJSON_GetArrayItem(data_array, i);
                    DeviceItem device;

                    // 基础字段
                    cJSON *id_val = cJSON_GetObjectItemCaseSensitive(item, "id");
                    if (cJSON_IsNumber(id_val))
                        device.id = id_val->valueint;

                    cJSON *sn_val = cJSON_GetObjectItemCaseSensitive(item, "sn");
                    if (cJSON_IsString(sn_val))
                        device.sn = sn_val->valuestring;

                    cJSON *name_val = cJSON_GetObjectItemCaseSensitive(item, "name");
                    if (cJSON_IsString(name_val))
                        device.name = name_val->valuestring;

                    // 新增补全字段
                    cJSON *area_id_val = cJSON_GetObjectItemCaseSensitive(item, "area_id");
                    if (cJSON_IsNumber(area_id_val))
                        device.area_id = area_id_val->valueint;

                    cJSON *version_val = cJSON_GetObjectItemCaseSensitive(item, "version");
                    if (cJSON_IsString(version_val))
                        device.version = version_val->valuestring;

                    cJSON *status_val = cJSON_GetObjectItemCaseSensitive(item, "status");
                    if (cJSON_IsNumber(status_val))
                        device.status = status_val->valueint;

                    cJSON *mac_val = cJSON_GetObjectItemCaseSensitive(item, "mac_address");
                    if (cJSON_IsString(mac_val))
                        device.mac_address = mac_val->valuestring;

                    cJSON *ip_val = cJSON_GetObjectItemCaseSensitive(item, "ip");
                    if (cJSON_IsString(ip_val))
                        device.ip = ip_val->valuestring;

                    cJSON *port_val = cJSON_GetObjectItemCaseSensitive(item, "port");
                    if (cJSON_IsString(port_val))
                        device.port = port_val->valuestring;

                    cJSON *account_val = cJSON_GetObjectItemCaseSensitive(item, "account");
                    if (cJSON_IsString(account_val))
                        device.account = account_val->valuestring;

                    cJSON *password_val = cJSON_GetObjectItemCaseSensitive(item, "password");
                    if (cJSON_IsString(password_val))
                        device.password = password_val->valuestring;

                    cJSON *live_val = cJSON_GetObjectItemCaseSensitive(item, "live_url");
                    if (cJSON_IsString(live_val))
                        device.live_url = live_val->valuestring;

                    cJSON *playback_val = cJSON_GetObjectItemCaseSensitive(item, "playback_url");
                    if (cJSON_IsString(playback_val))
                        device.playback_url = playback_val->valuestring;

                    cJSON *created_val = cJSON_GetObjectItemCaseSensitive(item, "created_at");
                    if (cJSON_IsString(created_val))
                        device.created_at = created_val->valuestring;
                    // 如果 created_at 可能是 null，这里默认保持为空字符串，或者可以特殊标记

                    cJSON *updated_val = cJSON_GetObjectItemCaseSensitive(item, "updated_at");
                    if (cJSON_IsString(updated_val))
                        device.updated_at = updated_val->valuestring;

                    cJSON *deleted_val = cJSON_GetObjectItemCaseSensitive(item, "deleted_at");
                    if (cJSON_IsString(deleted_val))
                        device.deleted_at = deleted_val->valuestring;
                    // 如果是 null，保持为空字符串

                    cJSON *protocol_val = cJSON_GetObjectItemCaseSensitive(item, "protocol");
                    if (cJSON_IsString(protocol_val))
                        device.protocol = protocol_val->valuestring;

                    cJSON *resolution_val = cJSON_GetObjectItemCaseSensitive(item, "resolution");
                    if (cJSON_IsString(resolution_val))
                        device.resolution = resolution_val->valuestring;

                    cJSON *storage_val = cJSON_GetObjectItemCaseSensitive(item, "storage");
                    if (cJSON_IsNumber(storage_val))
                        device.storage = storage_val->valueint;

                    cJSON *use_storage_val = cJSON_GetObjectItemCaseSensitive(item, "use_storage");
                    if (cJSON_IsNumber(use_storage_val))
                        device.use_storage = use_storage_val->valueint;

                    cJSON *uuid_val = cJSON_GetObjectItemCaseSensitive(item, "device_uuid");
                    if (cJSON_IsString(uuid_val))
                        device.device_uuid = uuid_val->valuestring;

                    // 将填充好的设备加入列表
                    out_response.data.push_back(device);
                }
            }
            cJSON_Delete(response_root);
            return true;
        }
    }
    else
    {
        if (res)
        {
            std::cerr << "DeviceList HTTP Error: " << res->status << std::endl;
        }
        else
        {
            std::cerr << "DeviceList Connection Failed" << std::endl;
        }
    }
    return false;
}

CPlatformManager::~CPlatformManager()
{
    deinit();
}

int CPlatformManager::init()
{
    const std::lock_guard<std::recursive_mutex> lock(m_mtxPlatformOperation);

    if (m_bInited)
    {
        dlog_warn("平台管理模块已初始化");
        return OK;
    }

    /* Load persisted values but defer worker startup until TVSDK callbacks are registered. */
    Network::Platform_Info_t stInfo;
    if (load_config())
    {
        dlog_info("平台管理模块从配置文件恢复登录信息成功");
        stInfo.server_ip = custom_host;
        stInfo.server_port = custom_post;
        stInfo.rtmp_port = m_nRtmpPort;
        stInfo.mqtt_port = m_nMqttPort;
        stInfo.user = login_user;
        stInfo.password = login_password;
        stInfo.enable = g_enable;
        stInfo.Custom = g_custom;
    }
    else
    {
        dlog_info("平台管理模块配置文件不存在或读取失败，等待网页首次配置");
    }

    m_bBootConnectOverride.store(false);
    m_bStopAutoLogin.store(true);
    if (CPlatformSdkAdapter::instance()->apply_config(stInfo) != OK)
    {
        dlog_error("平台管理模块初始化 SDK 配置失败");
        return ERR;
    }

    m_bInited = true;
    return OK;
}

int CPlatformManager::deinit()
{
    const std::lock_guard<std::recursive_mutex> lock(m_mtxPlatformOperation);
    if (!m_bInited)
    {
        return OK;
    }

    m_bStopAutoLogin.store(true);
    m_bBootConnectOverride.store(false);
    const int nStopResult = CPlatformSdkAdapter::instance()->stop_runtime();
    m_bInited = false;
    dlog_info("平台管理模块反初始化完成");
    return nStopResult;
}

bool CPlatformManager::load_config()
{
    Network::Platform_Info_t stInfo;
    if (Convert::read_file(PLATFORM_CONFIG_FILE, stInfo) != OK)
    {
        dlog_warn("读取平台配置文件失败: %s", PLATFORM_CONFIG_FILE);
        return false;
    }

    custom_host   = stInfo.server_ip;
    custom_post   = stInfo.server_port;
    m_nMqttPort   = stInfo.mqtt_port;
    login_user    = stInfo.user;
    login_password = stInfo.password;
    g_enable      = stInfo.enable;
    g_custom      = stInfo.Custom;
    m_nRtmpPort   = stInfo.rtmp_port;
    
    Network::Platform_Info_t stDefaultInfo;
    bool bNeedSaveConfig = false;
    // if (custom_host == "172.16.25.125")
    // {
    //     dlog_warn("检测到平台地址仍为临时测试平台[%s:%d]，还原为[%s:%d]",
    //               custom_host.c_str(),
    //               custom_post,
    //               stDefaultInfo.server_ip.c_str(),
    //               stDefaultInfo.server_port);
    //     custom_host = stDefaultInfo.server_ip;
    //     custom_post = stDefaultInfo.server_port;
    //     bNeedSaveConfig = true;
    // }

    // if (m_nRtmpPort == 1935)
    // {
    //     dlog_warn("检测到RTMP端口仍为临时测试端口[%d]，还原为[%d]",
    //               m_nRtmpPort,
    //               stDefaultInfo.rtmp_port);
    //     m_nRtmpPort = stDefaultInfo.rtmp_port;
    //     bNeedSaveConfig = true;
    // }

    // if (login_user == MQTT_PLATFORM_DEFAULT_USERNAME &&
    //     login_password == MQTT_PLATFORM_DEFAULT_PASSWORD)
    // {
    //     dlog_warn("检测到平台HTTP登录账号仍为MQTT账号[%s]，改用平台HTTP默认账号[%s]",
    //               login_user.c_str(), stDefaultInfo.user.c_str());
    //     login_user = stDefaultInfo.user;
    //     login_password = stDefaultInfo.password;
    //     bNeedSaveConfig = true;
    // }

    if (bNeedSaveConfig)
    {
        save_config();
    }

    dlog_info("加载平台配置: host=%s, port=%d, mqtt_port=%d, rtmp_port=%d, enable=%d, custom=%d, login_user=%s, login_passwordLen=%lu",
              custom_host.c_str(),
              custom_post,
              m_nMqttPort,
              m_nRtmpPort,
              g_enable,
              g_custom,
              login_user.c_str(),
              static_cast<unsigned long>(login_password.size()));
    return true;
}

bool CPlatformManager::save_config()
{
    Network::Platform_Info_t stInfo;
    stInfo.server_ip = custom_host.empty() ? host_ : custom_host;
    stInfo.server_port = custom_post > 0 ? custom_post : port_;
    stInfo.mqtt_port = m_nMqttPort > 0 ? m_nMqttPort : MQTT_PLATFORM_DEFAULT_PORT;
    stInfo.user = login_user;
    stInfo.password = login_password;
    stInfo.enable = g_enable;
    stInfo.Custom = g_custom;
    stInfo.rtmp_port = m_nRtmpPort;

    if (Convert::write_file(PLATFORM_CONFIG_FILE, stInfo) != OK)
    {
        dlog_error("保存平台配置文件失败: %s", PLATFORM_CONFIG_FILE);
        return false;
    }

    dlog_info("保存平台配置成功: host=%s, port=%d, enable=%d",
              stInfo.server_ip.c_str(), stInfo.server_port, stInfo.enable);
    return true;
}

bool CPlatformManager::register_current_device(const std::string &strToken,
                                               const std::string &strAccount,
                                               const std::string &strPassword)
{
    if (strToken.empty())
    {
        dlog_error("平台注册设备失败：access_token 为空");
        return false;
    }

    ::System::DeviceInfo_S stDeviceInfo;
    ::Network::Info_S stNetInfo;
    Video_NS::VideoConfig_S stVideoConfig;
    StorageManage_NS::StorageManage_S stStorageManageParam;

    SystemManage::instance()->get_device_info(stDeviceInfo);
    CNetworkManage::instance()->get_system_networkInfo(stNetInfo);

    stVideoConfig.nId = 0;
    CAVConfigure::instance()->get_configure(stVideoConfig);
    CStorageManage::instance()->get_storageManage_param(stStorageManageParam);

    CPlatformManager::StoreDevice req;
    CPlatformManager::StoreResponse resp;
    dlog_error("stDeviceInfo.serialNumber: %s",stDeviceInfo.serialNumber.c_str());
    req.sn = stDeviceInfo.serialNumber.empty() ? std::to_string(stDeviceInfo.deviceID) : stDeviceInfo.serialNumber;
    req.name = stDeviceInfo.deviceName;
    req.version = stDeviceInfo.systemVersion;
    req.account = strAccount;
    req.password = strPassword;
    req.ip = stNetInfo.stIp.ipv4Ip;
    req.port = 554;
    req.mac_address = stNetInfo.stIp.physicalAddress;
    req.protocol = "rtsp";
    req.resolution = std::to_string(stVideoConfig.stVideoResolution.nWidth) + "x" +
                     std::to_string(stVideoConfig.stVideoResolution.nHeight);
    req.storage = stStorageManageParam.strAvailableSpace;
    if (!stStorageManageParam.strAvailableSpace.empty() && !stStorageManageParam.strRecordRemainingSpace.empty())
    {
        req.use_storage = std::to_string(std::stof(stStorageManageParam.strAvailableSpace) -
                                         std::stof(stStorageManageParam.strRecordRemainingSpace));
    }
    else
    {
        req.use_storage = "10";
        req.storage = "10";
    }

    const bool bRegistered = storeDevice(req, strToken, resp);
    if (!bRegistered || resp.status != "success" || resp.status_code != 200)
    {
        dlog_error("平台注册设备失败：status[%s] status_code[%d] message[%s]",
                   resp.status.c_str(), resp.status_code, resp.message.c_str());
        return false;
    }

    dlog_info("平台注册设备成功：sn[%s] name[%s] device_uuid[%s]",
              resp.data.sn.c_str(), resp.data.name.c_str(), resp.data.device_uuid.c_str());
    return true;
}

void CPlatformManager::auto_login_loop()
{
    pthread_setname_np(pthread_self(), "PlatformLogin");
    while (!m_bStopAutoLogin.load())
    {

        bool bPlatformActive = false;
        bool bNeedRelogin = false;
        {
            /* 与网页保存和网络切换共享状态锁，避免读取到半更新的平台参数。 */
            const std::lock_guard<std::recursive_mutex> lock(m_mtxPlatformOperation);
            bPlatformActive = g_enable || m_bBootConnectOverride.load();
            bNeedRelogin = m_bNetworkReloginPending.load() || access_token_.empty();
        }

        if (!bPlatformActive)
        {
            /* 网页关闭平台后保留线程，但不再继续发起重试连接。 */
            std::this_thread::sleep_for(std::chrono::seconds(AUTO_LOGIN_RETRY_INTERVAL_SEC));
            continue;
        }

        /* 若已登录（有有效 token），无需重试 */
        //if (!access_token_.empty())
        //const bool bNeedRelogin = m_bNetworkReloginPending.load() || access_token_.empty();
        if (!bNeedRelogin)
        {
            std::this_thread::sleep_for(std::chrono::seconds(AUTO_LOGIN_RETRY_INTERVAL_SEC));
            continue;
        }

        /* 达到最大重试次数则退出（0 表示无限重试） */
        //if (AUTO_LOGIN_MAX_RETRIES > 0 && m_nRetryCount >= AUTO_LOGIN_MAX_RETRIES)
        if (AUTO_LOGIN_MAX_RETRIES > 0 &&
            m_nRetryCount.load() >= AUTO_LOGIN_MAX_RETRIES)
        {
            dlog_warn("平台自动登录达到最大重试次数 %d，停止重试", AUTO_LOGIN_MAX_RETRIES);
            break;
        }

        // LoginResponse out_response;
        // std::string target_host = g_custom ? custom_host : host_;
        // int target_port = g_custom ? custom_post : port_;

        // dlog_info("平台自动登录重试第 %d 次: host=%s, port=%d",
        //           m_nRetryCount + 1, target_host.c_str(), target_port);

        // bool bSuccess = login(target_host, target_port, login_user, login_password, g_enable, g_custom, out_response);
        // if (bSuccess)
        dlog_info("平台自动登录重试第 %d 次", m_nRetryCount.load() + 1);
        const int nReloginRet = change_net_relogin();
        if (nReloginRet == OK)
        {
            dlog_info("平台自动登录成功");
            // register_current_device(out_response.data.access_token, login_user, login_password);
            // /* 登录成功后，更新推流地址 */
            // relogin_and_update_stream();
            // break;
            m_bNetworkReloginPending.store(false);
            m_nRetryCount.store(0);
        }
        else
        {
            dlog_error("平台自动登录失败，%d 秒后重试", AUTO_LOGIN_RETRY_INTERVAL_SEC);
            //m_nRetryCount++;
            m_nRetryCount.fetch_add(1);
        }

        std::this_thread::sleep_for(std::chrono::seconds(AUTO_LOGIN_RETRY_INTERVAL_SEC));
    }
}

void CPlatformManager::ensure_auto_login_thread()
{
    std::lock_guard<std::mutex> lock(m_mtxAutoLoginLifecycle);
    if (m_autoLoginThread.joinable())
    {
        return;
    }

    //m_bStopAutoLogin.store(false);
    m_nRetryCount.store(0);
    m_autoLoginThread = std::thread(&CPlatformManager::auto_login_loop, this);
    dlog_info("平台自动登录重试线程已启动");
}

std::unique_lock<std::recursive_mutex> CPlatformManager::lock_platform_operation()
{
    return std::unique_lock<std::recursive_mutex>(m_mtxPlatformOperation);
}

int CPlatformManager::reconnect_from_persisted_config()
{
    const std::lock_guard<std::recursive_mutex> lock(m_mtxPlatformOperation);

    if (!m_bInited)
    {
        dlog_warn("网络 切换时平台管理模块尚未初始化，跳过配置重载");
        return ERR_UNINIT;
    }

    Network::Platform_Info_t stInfo;
    const int nReadRet = read_platform_config_for_reconnect(PLATFORM_CONFIG_FILE, stInfo);
    if (nReadRet != OK)
    {
        dlog_warn("网络 切换时读取平台配置失败或格式不完整: %s, ret=%d",
                  PLATFORM_CONFIG_FILE, nReadRet);
        return nReadRet;
    }

    /* Apply one complete persisted snapshot before notifying the SDK route monitor. */
    if (!apply_platform_config(stInfo))
    {
        dlog_error("网络 切换时应用平台配置失败");
        return ERR_PARAM;
    }
    return CPlatformSdkAdapter::instance()->notify_network_changed();
}

int CPlatformManager::change_net_relogin()
{
    const std::lock_guard<std::recursive_mutex> lock(m_mtxPlatformOperation);
    return CPlatformSdkAdapter::instance()->notify_network_changed();
}

int CPlatformManager::relogin_and_update_stream()
{
    const std::lock_guard<std::recursive_mutex> lock(m_mtxPlatformOperation);
    return CPlatformSdkAdapter::instance()->notify_network_changed();
}

int CPlatformManager::init_mqtt()
{
    return CPlatformSdkAdapter::instance()->start_runtime();
}

int CPlatformManager::restart_mqtt()
{
    return CPlatformSdkAdapter::instance()->restart_runtime();
}


void CPlatformManager::deinit_mqtt()
{
    CPlatformSdkAdapter::instance()->stop_runtime();
}

/**
 * @brief 设置 CTaskManage 实例并初始化 MQTT SDK 网关
 * @param pTaskManage 任务管理器指针
 */
void CPlatformManager::set_taskManage(CTaskManage *pTaskManage)
{
    CMqttSdkGateway::set_task_manage(pTaskManage);
    if (CPlatformSdkAdapter::instance()->register_host_callbacks() != OK ||
        CPlatformSdkAdapter::instance()->start_runtime() != OK)
    {
        dlog_error("平台管理模块启动 SDK 平台运行时失败");
        return;
    }
    dlog_info("平台管理模块已向 SDK 注入 IPC 能力并启动平台运行时");
}

void CPlatformManager::start_mqtt_command_worker()
{
    /* lock: 生命周期锁覆盖“检查 joinable 到创建线程”的完整区间，避免并发初始化产生两个消费者。 */
    std::lock_guard<std::mutex> lockLifecycle(m_mtxMqttCommandLifecycle);
    if (m_mqttCommandThread.joinable())
    {
        return;
    }

    {
        /* lock: 重启时丢弃上一轮遗留命令，并与 SDK 回调的入队操作互斥。 */
        std::lock_guard<std::mutex> lockQueue(m_mtxMqttCommandQueue);
        m_deqMqttCommands.clear();
        m_nLastMqttCommandDropLogMs = 0;
    }
    m_uMqttCommandDropCount.store(0);
    m_bStopMqttCommand.store(false);
    m_mqttCommandThread = std::thread(&CPlatformManager::mqtt_command_loop, this);
    dlog_info("MQTT 平台命令工作线程已启动，队列上限=%d", static_cast<int>(MQTT_COMMAND_QUEUE_MAX_SIZE));
}

void CPlatformManager::stop_mqtt_command_worker()
{
    /* lock: join 必须与 start 串行，防止旧线程尚未退出时被重新启动。 */
    std::lock_guard<std::mutex> lockLifecycle(m_mtxMqttCommandLifecycle);
    if (!m_mqttCommandThread.joinable())
    {
        return;
    }

    {
        /* memory: 停止时主动释放尚未执行的 Payload 副本；正在执行的命令允许完成后再退出。 */
        std::lock_guard<std::mutex> lockQueue(m_mtxMqttCommandQueue);
        m_bStopMqttCommand.store(true);
        m_deqMqttCommands.clear();
    }
    m_cvMqttCommand.notify_all();
    m_mqttCommandThread.join();
    dlog_info("MQTT 平台命令工作线程已停止，累计拒绝命令=%llu", static_cast<unsigned long long>(m_uMqttCommandDropCount.load()));
}

bool CPlatformManager::enqueue_mqtt_command(const std::string &strTopic, const std::string &strPayload)
{
    /* lock: 入队、容量检查和限频时间戳必须属于同一临界区，避免多个回调同时突破容量上限。 */
    std::lock_guard<std::mutex> lockQueue(m_mtxMqttCommandQueue);
    if (m_bStopMqttCommand.load())
    {
        return false;
    }

    if (m_deqMqttCommands.size() >= MQTT_COMMAND_QUEUE_MAX_SIZE)
    {
        const uint64_t uDropCount = m_uMqttCommandDropCount.fetch_add(1) + 1;
        /* 使用 steady_clock，避免校时导致限频窗口被错误拉长或缩短。 */
        const int64_t nNowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now().time_since_epoch())
                                   .count();
        if (m_nLastMqttCommandDropLogMs == 0 || nNowMs - m_nLastMqttCommandDropLogMs >= MQTT_COMMAND_DROP_LOG_INTERVAL_MS)
        {
            m_nLastMqttCommandDropLogMs = nNowMs;
            dlog_warn("MQTT 平台命令队列已满，拒绝新命令，queue=%d, drop=%llu",
                      static_cast<int>(m_deqMqttCommands.size()),
                      static_cast<unsigned long long>(uDropCount));
        }
        return false;
    }

    /* memory: 在 SDK 回调返回前深拷贝 Topic/Payload；底层 MQTT 缓冲区随后可能被释放。 */
    MqttCommand_S stCommand;
    stCommand.strTopic = strTopic;
    stCommand.strPayload = strPayload;
    m_deqMqttCommands.push_back(std::move(stCommand));
    m_cvMqttCommand.notify_one();
    return true;
}

void CPlatformManager::on_mqtt_message(const std::string &strTopic, const std::string &strPayload)
{
    /* perf: 此函数运行在 MQTT SDK 回调上下文，只允许复制数据并入队，禁止解析JSON或执行同步任务。 */
    if (!enqueue_mqtt_command(strTopic, strPayload))
    {
        return;
    }
}

void CPlatformManager::mqtt_command_loop()
{
    /* trace: 固定线程名便于 top/perf 统计区分 MQTT 业务耗时与 MQTT SDK 回调耗时。 */
    pthread_setname_np(pthread_self(), "MqttCommand");
    dlog_info("MQTT 平台命令工作线程运行");

    while (true)
    {
        MqttCommand_S stCommand;
        {
            std::unique_lock<std::mutex> lockQueue(m_mtxMqttCommandQueue);
            /* worker 只在停止或存在命令时唤醒，避免空队列轮询占用 CPU。 */
            m_cvMqttCommand.wait(lockQueue,
                                 [this]()
                                 {
                                     return m_bStopMqttCommand.load() || !m_deqMqttCommands.empty();
                                 });
            if (m_bStopMqttCommand.load())
            {
                break;
            }

            /* memory: 将队首所有权移交给当前循环，解锁后再执行可能阻塞的 HTTP/任务逻辑。 */
            stCommand = std::move(m_deqMqttCommands.front());
            m_deqMqttCommands.pop_front();
        }

        process_mqtt_command(stCommand.strTopic, stCommand.strPayload);
    }

    dlog_info("MQTT 平台命令工作线程退出");
}

/**
 * @brief 处理收到的 MQTT 命令消息
 * @param strTopic 消息 Topic
 * @param strPayload 消息内容（JSON）
 * @note  处理流程：
 *        1. 解析 JSON 提取 Command/RequestId/Token/Data
 *        2. 校验 Token（防止未授权访问）
 *        3. 优先查找自定义处理器，未命中则通过 MQTT SDK 网关转发到 CTaskManage
 */
void CPlatformManager::process_mqtt_command(const std::string &strTopic, const std::string &strPayload)
{
    dlog_debug("MQTT 命令开始处理，Topic[%s]，长度[%zu]", strTopic.c_str(), strPayload.size());

    /* 解析 JSON */
    cJSON *pRoot = strPayload.empty() ? nullptr : cJSON_Parse(strPayload.c_str());
    if (!pRoot)
    {
        dlog_error("MQTT 消息 JSON 解析失败");
        return;
    }

    /* 提取 Command 和 RequestId */
    cJSON *pCommand = cJSON_GetObjectItemCaseSensitive(pRoot, "Command");
    cJSON *pRequestId = cJSON_GetObjectItemCaseSensitive(pRoot, "RequestId");
    cJSON *pData = cJSON_GetObjectItemCaseSensitive(pRoot, "Data");
    if (!pData)
    {
        pData = cJSON_GetObjectItemCaseSensitive(pRoot, "data");
    }

    if (!cJSON_IsString(pCommand))
    {
        dlog_error("MQTT 消息缺少 Command 字段");
        cJSON_Delete(pRoot);
        return;
    }

    std::string strCommand = pCommand->valuestring;
    std::string strRequestId = cJSON_IsString(pRequestId) ? pRequestId->valuestring : "";
    std::string strData = "{}";

    if (pData != nullptr)
    {
        char *pDataStr = cJSON_PrintUnformatted(pData);
        if (pDataStr)
        {
            strData = pDataStr;
            free(pDataStr);
        }
    }

    cJSON_Delete(pRoot);

    /*
     * 鉴权说明：授权由 MQTT Broker 连接层保障
     * · 设备连接 Broker 时使用设备账号，仅能操作 device/{SN}/# Topic
     * · 第三方平台连接 Broker 时使用平台账号，仅能操作 device/+/command Topic
     * · Broker 层面的 ACL 已确保只有合法连接才能发送命令，消息层不做重复校验
     */

    /* lock: 仅在锁内复制处理器；业务回调可能执行HTTP或任务，禁止在m_mtxMqttHandlers内调用。 */
    CommandHandler fnHandler;
    {
        std::lock_guard<std::mutex> lock(m_mtxMqttHandlers);
        auto it = m_mapMqttHandlers.find(strCommand);
        if (it != m_mapMqttHandlers.end())
        {
            fnHandler = it->second;
        }
    }
    if (fnHandler)
    {
        dlog_info("MQTT 命令分发：%s", strCommand.c_str());
        fnHandler(strRequestId, strData);
        return;
    }

    /* 未命中自定义处理器，尝试通过 MQTT SDK 网关转发 */
    if (CMqttSdkGateway::is_command_supported(strCommand))
    {
        dlog_info("MQTT SDK 网关转发命令：%s", strCommand.c_str());

        const std::string strNormalizedCommand = normalize_mqtt_command(strCommand);
        if (strNormalizedCommand == "NET_ADD_FACE_INFO" || strNormalizedCommand == "NET_SET_FACE_INFO")
        {
            std::string strError;
            if (!prepare_face_image_command(strNormalizedCommand, strData, strError))
            {
                dlog_error("MQTT 人脸命令预处理失败：%s", strError.c_str());
                publish_response(strCommand, strRequestId, -1, make_error_json(strError));
                return;
            }
        }

        if (CMqttSdkGateway::is_get_command(strCommand))
        {
            /* GET 命令：同步执行并返回结果 */
            std::string strResult;
            int nRet = CMqttSdkGateway::execute_get(strCommand, strData, strResult);
            if (!strResult.empty())
            {
                int nTaskReturn = nRet;
                std::string strTaskData = "{}";
                if (extract_task_response(strResult, nTaskReturn, strTaskData))
                {
                    nRet = nTaskReturn;
                    strResult = strTaskData;
                }
            }

            if (nRet == 0)
            {
                publish_response(strCommand, strRequestId, 0, strResult.empty() ? "{}" : strResult);
            }
            else
            {
                dlog_error("MQTT SDK 网关 GET 命令[%s]执行失败：%d", strCommand.c_str(), nRet);
                publish_response(strCommand, strRequestId, nRet, "{\"error\":\"execute get failed\"}");
            }
        }
        else
        {
            /* SET 命令：执行后返回状态 */
            std::string strSetResult;
            int nRet = CMqttSdkGateway::execute_set(strCommand, strData, strSetResult);
            if (!strSetResult.empty())
            {
                int nTaskReturn = nRet;
                std::string strTaskData = "{}";
                if (extract_task_response(strSetResult, nTaskReturn, strTaskData))
                {
                    nRet = nTaskReturn;
                    strSetResult = strTaskData;
                }
            }

            if (nRet == 0)
            {
                publish_response(strCommand, strRequestId, 0, strSetResult.empty() ? "{}" : strSetResult);
            }
            else
            {
                dlog_error("MQTT SDK 网关 SET 命令[%s]执行失败：%d", strCommand.c_str(), nRet);
                publish_response(strCommand, strRequestId, nRet, "{\"error\":\"execute set failed\"}");
            }
        }
    }
    else
    {
        dlog_warn("MQTT 未知命令：%s", strCommand.c_str());
        publish_response(strCommand, strRequestId, -1, "{\"error\":\"unsupported command\"}");
    }
}

bool CPlatformManager::prepare_face_image_command(const std::string &strCommand,
                                                  std::string &strData,
                                                  std::string &strError)
{
    const std::string strNormalizedCommand = normalize_mqtt_command(strCommand);
    if (strNormalizedCommand != "NET_ADD_FACE_INFO" && strNormalizedCommand != "NET_SET_FACE_INFO")
    {
        return true;
    }

    cJSON *pData = cJSON_Parse(strData.c_str());
    if (!pData || !cJSON_IsObject(pData))
    {
        strError = "face command data is not json";
        if (pData)
        {
            cJSON_Delete(pData);
        }
        return false;
    }

    cJSON *pId = cJSON_GetObjectItemCaseSensitive(pData, "Id");
    int nLowerId = 0;
    if (!pId && get_json_int(pData, "id", nLowerId))
    {
        set_json_int(pData, "Id", nLowerId);
    }

    bool bRet = true;
    if (strNormalizedCommand == "NET_ADD_FACE_INFO")
    {
        bRet = ensure_face_nv21_local(pData, strError);
    }

    if (bRet)
    {
        char *pOut = cJSON_PrintUnformatted(pData);
        if (pOut)
        {
            strData = pOut;
            free(pOut);
        }
    }

    cJSON_Delete(pData);
    return bRet;
}

bool CPlatformManager::ensure_face_nv21_local(cJSON *pData, std::string &strError)
{
    if (!pData)
    {
        strError = "face data is null";
        return false;
    }

    std::string strBinPath;
    get_json_string(pData, "BinPath", strBinPath);

    int nWidth = 0;
    int nHeight = 0;
    get_json_int(pData, "PicWidth", nWidth);
    get_json_int(pData, "PicHeight", nHeight);
    if (nWidth <= 0 || nHeight <= 0)
    {
        strError = "PicWidth/PicHeight is invalid";
        return false;
    }
    const long long nExpectSize = (long long)nWidth * (long long)nHeight * 3 / 2;

    std::string strDownloadUrl;
    if (!get_json_string(pData, "PicUrl", strDownloadUrl) &&
        !get_json_string(pData, "DownloadUrl", strDownloadUrl) &&
        !get_json_string(pData, "Nv21Url", strDownloadUrl) &&
        !get_json_string(pData, "FileUrl", strDownloadUrl))
    {
        get_json_string(pData, "PicPath", strDownloadUrl);
    }

    if (strBinPath.empty())
    {
        std::string strName = basename_of(strDownloadUrl);
        if (strName.empty())
        {
            strName = "face.nv21";
        }
        strBinPath = std::string(FACE_NV21_UPLOAD_DIR) + "/" + strName;
        set_json_string(pData, "BinPath", strBinPath);
    }

    bool bNeedDownload = !file_exists(strBinPath);
    if (!bNeedDownload && nExpectSize > 0)
    {
        const long long nActualSize = file_size(strBinPath);
        if (nActualSize != nExpectSize)
        {
            dlog_warn("本地人脸 NV21 大小不匹配，将重新下载：path[%s] expect[%lld] actual[%lld]",
                      strBinPath.c_str(), nExpectSize, nActualSize);
            bNeedDownload = true;
        }
    }

    if (!bNeedDownload)
    {
        dlog_info("人脸 NV21 文件已存在：%s", strBinPath.c_str());
    }
    else
    {
        if (strDownloadUrl.empty())
        {
            strError = "BinPath not exists and PicUrl/DownloadUrl/Nv21Url/FileUrl/PicPath is empty";
            return false;
        }

        const std::string strResolvedUrl = resolve_platform_file_url(strDownloadUrl);
        if (!download_file_to_path(strResolvedUrl, strBinPath, strError))
        {
            return false;
        }
    }

    if (nExpectSize > 0)
    {
        const long long nActualSize = file_size(strBinPath);
        if (nActualSize != nExpectSize)
        {
            std::ostringstream oss;
            oss << "NV21 size mismatch path=" << strBinPath
                << " expect=" << nExpectSize
                << " actual=" << nActualSize;
            strError = oss.str();
            return false;
        }
        if (nActualSize <= 2147483647LL)
        {
            set_json_int(pData, "PicSize", (int)nActualSize);
        }
    }

    return true;
}

bool CPlatformManager::download_file_to_path(const std::string &strUrl,
                                             const std::string &strLocalPath,
                                             std::string &strError)
{
    if (strUrl.empty() || strLocalPath.empty())
    {
        strError = "download url or local path is empty";
        return false;
    }

    const std::string httpPrefix = "http://";
    const std::string httpsPrefix = "https://";
    bool bHttps = false;
    size_t nSchemeLen = 0;
    if (strUrl.compare(0, httpPrefix.size(), httpPrefix) == 0)
    {
        nSchemeLen = httpPrefix.size();
    }
    else if (strUrl.compare(0, httpsPrefix.size(), httpsPrefix) == 0)
    {
        bHttps = true;
        nSchemeLen = httpsPrefix.size();
    }
    else
    {
        strError = "unsupported download url: " + strUrl;
        return false;
    }

    if (bHttps)
    {
        strError = "https download is not supported by current httplib build";
        return false;
    }

    const size_t nPathPos = strUrl.find('/', nSchemeLen);
    const std::string strHostPort = (nPathPos == std::string::npos) ? strUrl.substr(nSchemeLen) : strUrl.substr(nSchemeLen, nPathPos - nSchemeLen);
    const std::string strPath = (nPathPos == std::string::npos) ? "/" : strUrl.substr(nPathPos);
    if (strHostPort.empty())
    {
        strError = "download host is empty: " + strUrl;
        return false;
    }

    std::string strHost = strHostPort;
    int nPort = 80;
    const size_t nColonPos = strHostPort.find(':');
    if (nColonPos != std::string::npos)
    {
        strHost = strHostPort.substr(0, nColonPos);
        nPort = std::atoi(strHostPort.substr(nColonPos + 1).c_str());
        if (nPort <= 0)
        {
            strError = "invalid download port: " + strUrl;
            return false;
        }
    }

    if (!ensure_directory(dirname_of(strLocalPath)))
    {
        strError = "create local directory failed: " + dirname_of(strLocalPath);
        return false;
    }

    httplib::Client cli(strHost, nPort);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(30, 0);

    dlog_info("开始下载人脸 NV21 文件：%s -> %s", strUrl.c_str(), strLocalPath.c_str());

    httplib::Headers headers;
    const std::string strPlatformHost = g_custom ? custom_host : host_;
    if (!access_token_.empty() && strHost == strPlatformHost)
    {
        headers.emplace("Authorization", "Bearer " + access_token_);
    }

    auto res = headers.empty() ? cli.Get(strPath) : cli.Get(strPath, headers);
    if (!res)
    {
        strError = "download request failed: " + strUrl;
        return false;
    }
    if (res->status != 200)
    {
        std::ostringstream oss;
        oss << "download http status " << res->status << ": " << strUrl;
        strError = oss.str();
        return false;
    }
    if (res->body.empty())
    {
        strError = "download body is empty: " + strUrl;
        return false;
    }

    const std::string strTmpPath = strLocalPath + ".tmp";
    {
        std::ofstream ofs(strTmpPath.c_str(), std::ios::binary | std::ios::trunc);
        if (!ofs.is_open())
        {
            strError = "open tmp file failed: " + strTmpPath;
            return false;
        }
        ofs.write(res->body.data(), (std::streamsize)res->body.size());
        if (!ofs.good())
        {
            strError = "write tmp file failed: " + strTmpPath;
            ofs.close();
            std::remove(strTmpPath.c_str());
            return false;
        }
    }

    if (std::rename(strTmpPath.c_str(), strLocalPath.c_str()) != 0)
    {
        strError = "rename tmp file failed: " + strLocalPath;
        std::remove(strTmpPath.c_str());
        return false;
    }

    dlog_info("人脸 NV21 文件下载完成：%s，大小[%zu]", strLocalPath.c_str(), res->body.size());
    return true;
}

std::string CPlatformManager::resolve_platform_file_url(const std::string &strPathOrUrl) const
{
    if (strPathOrUrl.compare(0, 7, "http://") == 0 ||
        strPathOrUrl.compare(0, 8, "https://") == 0)
    {
        return strPathOrUrl;
    }

    const std::string strHost = g_custom ? custom_host : host_;
    const int nPort = g_custom ? custom_post : port_;
    std::string strUrl = "http://" + strHost;
    if (nPort > 0 && nPort != 80)
    {
        strUrl += ":" + std::to_string(nPort);
    }

    if (strPathOrUrl.empty() || strPathOrUrl[0] != '/')
    {
        strUrl += "/";
    }
    strUrl += strPathOrUrl;
    return strUrl;
}

/**
 * @brief 注册 MQTT 自定义命令处理器
 * @note  注册在此的命令优先于 MQTT SDK 网关处理
 *        适用于需要特殊处理逻辑的命令（非标准 SDK 命令透传）
 *        大部分 SDK 命令（NET_GET_xxx / NET_SET_xxx）由 on_mqtt_message 自动通过网关转发
 */
void CPlatformManager::register_mqtt_handlers()
{
    std::lock_guard<std::mutex> lock(m_mtxMqttHandlers);

    dlog_info("MQTT 命令处理器注册完成，标准 SDK 命令将通过网关自动转发");
}

int CPlatformManager::publish_event(const std::string &strCommand,
                                    const std::string &strData,
                                    const std::string &strRequestId)
{
    const auto stNow = std::chrono::system_clock::now();
    const long long llTimestampMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            stNow.time_since_epoch())
            .count();
    const std::string strEffectiveRequestId = strRequestId.empty()
                                                  ? "event-" + std::to_string(llTimestampMs)
                                                  : strRequestId;
    return CPlatformSdkAdapter::instance()->report_event(
        strCommand,
        strEffectiveRequestId,
        strData,
        0,
        std::string(),
        0,
        llTimestampMs,
        std::string(),
        false,
        false);
}

int CPlatformManager::publish_response(const std::string &strCommand,
                                       const std::string &strRequestId,
                                       int nReturn,
                                       const std::string &strData)
{
    if (!m_pstMqtt || !m_pstMqtt->is_connected())
    {
        dlog_warn("MQTT 未连接，无法发布响应");
        return ERR_UNINIT;
    }

    /* 构造 JSON */
    cJSON *pRoot = cJSON_CreateObject();
    cJSON_AddStringToObject(pRoot, "Command", strCommand.c_str());
    cJSON_AddStringToObject(pRoot, "RequestId", strRequestId.c_str());
    cJSON_AddNumberToObject(pRoot, "Return", nReturn);
    
    cJSON *pData = cJSON_Parse(strData.c_str());
    if (pData)
    {
        cJSON_AddItemToObject(pRoot, "Data", pData);
    }
    else
    {
        cJSON_AddObjectToObject(pRoot, "Data");
    }

    char *pJson = cJSON_PrintUnformatted(pRoot);
    std::string strPayload = pJson;
    free(pJson);
    cJSON_Delete(pRoot);

    /* 发布到响应 Topic */
    std::string strTopic = MQTT_TOPIC_RESPONSE(m_strMqttClientId);
    return m_pstMqtt->publish(strTopic, strPayload, MQTT_QOS_RESPONSE);
}

void CPlatformManager::on_mqtt_connection_changed(bool bConnected, const std::string &strReason)
{
    dlog_info("MQTT 连接状态变化：connected=%d, reason=%s", bConnected ? 1 : 0, strReason.c_str());

    if (bConnected)
    {
        /* 订阅恢复后先上报取流信息，再通知设备在线。 */
        publish_device_register();
        publish_device_status(true, "connect");
    }
    /* 离线状态由 LWT 自动发布（异常断开）或 deinit() 主动发布（正常关机），这里不需要额外处理 */
}

int CPlatformManager::publish_device_register()
{
    if (!m_pstMqtt || !m_pstMqtt->is_connected())
    {
        dlog_warn("MQTT 未连接，无法发布设备注册信息");
        return ERR_UNINIT;
    }

    std::string strRouteInterface;
    std::string strLocalIp;
    const bool bRouteResolved = get_platform_route_interface(m_strMqttBroker,
                                                             m_nMqttPort,
                                                             strRouteInterface,
                                                             strLocalIp);
    const bool bUseRtsp = bRouteResolved && !is_wireless_uplink_interface(strRouteInterface);
    const char *pUplinkType = !bRouteResolved ? "unknown" : (bUseRtsp ? "wired" : "wireless");
    const char *pStreamMode = bUseRtsp ? "rtsp" : "rtmp";

    if (!bRouteResolved)
    {
        /* 未能取得路由出口时保守使用设备主动推流，避免平台回拉不可达的私网 RTSP。 */
        dlog_warn("未识别 MQTT 路由出口，设备注册将按 RTMP 上报");
    }

    std::string strRtspUrl;
    if (CRtspServer::instance()->isInit())
    {
        const char *pRtspUrl = CRtspServer::instance()->getRtspUrl(RTSP_CHN_MAIN, false);
        if (pRtspUrl != nullptr)
        {
            strRtspUrl = pRtspUrl;
        }
    }

    if (strRtspUrl.empty())
    {
        dlog_warn("RTSP 主码流地址不可用，仍发布设备注册信息: sn[%s]", m_strMqttClientId.c_str());
    }

    const std::string strPlatformHost = g_custom ? custom_host : host_;
    const std::string strRtmpUrl = build_rtmp_main_url(strPlatformHost, m_nRtmpPort, m_strMqttClientId);

    const auto now = std::chrono::system_clock::now();
    const auto nTimestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  now.time_since_epoch())
                                  .count();
    const std::string strRequestId = "register-" + m_strMqttClientId + "-" + std::to_string(nTimestampMs);
    PlatformRegisterCrypto::EncryptedCredential_S stEncryptedCredential;
    std::string strCredentialError;
    bool bCredentialEncrypted = false;

    if (bUseRtsp && !strRtspUrl.empty())
    {
        const std::string strCameraAccount = USER_DEFAULT_NAME;
        std::string strCameraPassword = CUserManage::instance()->get_passwd(strCameraAccount);
        if (strCameraPassword.empty())
        {
            dlog_warn("未获取到 RTSP 管理员密码，设备注册凭据将使用空密码: sn[%s]",
                      m_strMqttClientId.c_str());
        }

        std::string strCredentialPlaintext = build_register_credential_plaintext(strRtspUrl,
                                                                                  strCameraAccount,
                                                                                  strCameraPassword);
        const std::string strAad = build_register_credential_aad(m_strMqttClientId,
                                                                 strRequestId,
                                                                 nTimestampMs,
                                                                 pUplinkType,
                                                                 pStreamMode);
        bCredentialEncrypted = PlatformRegisterCrypto::encrypt_credential(PLATFORM_REGISTER_CREDENTIAL_PUBLIC_KEY,
                                                                            PLATFORM_REGISTER_CREDENTIAL_KEY_ID,
                                                                            strCredentialPlaintext,
                                                                            strAad,
                                                                            stEncryptedCredential,
                                                                            strCredentialError);
        cleanse_string(strCredentialPlaintext);
        cleanse_string(strCameraPassword);
    }
    else if (bUseRtsp)
    {
        strCredentialError = "RTSP URL unavailable";
    }

    if (bUseRtsp && !bCredentialEncrypted)
    {
        dlog_warn("设备注册凭据加密失败，注册消息不携带任何RTSP凭据: sn[%s], reason[%s], publicKey[%s]",
                  m_strMqttClientId.c_str(),
                  strCredentialError.c_str(),
                  PLATFORM_REGISTER_CREDENTIAL_PUBLIC_KEY);
    }

    cJSON *pRoot = cJSON_CreateObject();
    if (pRoot == nullptr)
    {
        return ERR;
    }

    cJSON_AddStringToObject(pRoot, "Command", MQTT_DEVICE_REGISTER_COMMAND);
    cJSON_AddStringToObject(pRoot, "RequestId", strRequestId.c_str());

    cJSON *pData = cJSON_CreateObject();
    cJSON_AddStringToObject(pData, "Sn", m_strMqttClientId.c_str());
    cJSON_AddStringToObject(pData, "UplinkType", pUplinkType);
    cJSON_AddStringToObject(pData, "UplinkInterface", bRouteResolved ? strRouteInterface.c_str() : "");
    cJSON_AddStringToObject(pData, "LocalIp", bRouteResolved ? strLocalIp.c_str() : "");
    cJSON_AddStringToObject(pData, "StreamMode", pStreamMode);
    cJSON_AddStringToObject(pData, "Timestamp", std::to_string(nTimestampMs).c_str());

    if (bCredentialEncrypted)
    {
        cJSON *pCredential = cJSON_AddObjectToObject(pData, "Credential");
        cJSON_AddNumberToObject(pCredential, "Version", 1);
        cJSON_AddStringToObject(pCredential, "Algorithm", stEncryptedCredential.strAlgorithm.c_str());
        cJSON_AddStringToObject(pCredential, "KeyId", stEncryptedCredential.strKeyId.c_str());
        cJSON_AddStringToObject(pCredential, "EncryptedKey", stEncryptedCredential.strEncryptedKey.c_str());
        cJSON_AddStringToObject(pCredential, "Nonce", stEncryptedCredential.strNonce.c_str());
        cJSON_AddStringToObject(pCredential, "Ciphertext", stEncryptedCredential.strCiphertext.c_str());
        cJSON_AddStringToObject(pCredential, "Tag", stEncryptedCredential.strTag.c_str());
    }
    else
    {
        cJSON_AddStringToObject(pData, "CredentialState", bUseRtsp ? "unavailable" : "not_required");
    }

    if (!bUseRtsp)
    {
        cJSON *pRtmp = cJSON_AddObjectToObject(pData, "Rtmp");
        cJSON_AddStringToObject(pRtmp, "Url", strRtmpUrl.c_str());
    }
    cJSON_AddItemToObject(pRoot, "Data", pData);

    char *pJson = cJSON_PrintUnformatted(pRoot);
    const std::string strPayload = pJson ? pJson : "{}";
    if (pJson != nullptr)
    {
        cJSON_free(pJson);
    }
    cJSON_Delete(pRoot);

    const std::string strTopic = MQTT_TOPIC_REGISTER(m_strMqttClientId);
    const int nRet = m_pstMqtt->publish(strTopic, strPayload, MQTT_QOS_RESPONSE);
    if (nRet == OK)
    {
        dlog_info("设备注册信息已提交发送: topic[%s], sn[%s], uplink[%s], interface[%s], mode[%s]",
                  strTopic.c_str(),
                  m_strMqttClientId.c_str(),
                  pUplinkType,
                  bRouteResolved ? strRouteInterface.c_str() : "",
                  pStreamMode);
    }
    else
    {
        dlog_warn("设备注册信息发布失败: ret[%d], sn[%s], mode[%s]",
                  nRet,
                  m_strMqttClientId.c_str(),
                  pStreamMode);
    }
    return nRet;
}

void CPlatformManager::start_status_heartbeat()
{
    std::lock_guard<std::mutex> lifecycleLock(m_mtxStatusHeartbeatLifecycle);
    if (m_statusHeartbeatThread.joinable())
    {
        return;
    }

    m_bStopStatusHeartbeat.store(false);
    m_statusHeartbeatThread = std::thread(&CPlatformManager::status_heartbeat_loop, this);
    dlog_info("设备在线状态心跳线程已启动，间隔=%d秒", STATUS_HEARTBEAT_INTERVAL_SEC);
}

void CPlatformManager::stop_status_heartbeat()
{
    std::lock_guard<std::mutex> lifecycleLock(m_mtxStatusHeartbeatLifecycle);
    if (!m_statusHeartbeatThread.joinable())
    {
        return;
    }

    /* 轮询间隔为 1 秒，停止路径最多等待 1 秒即可回收线程。 */
    m_bStopStatusHeartbeat.store(true);
    m_statusHeartbeatThread.join();
    dlog_info("设备在线状态心跳线程已停止");
}

void CPlatformManager::status_heartbeat_loop()
{
    pthread_setname_np(pthread_self(), "PlatformHeart");
    dlog_info("设备在线状态心跳线程运行");
    /*
     * 使用 steady_clock 计时配合 1 秒轮询，规避目标环境中长时间定时等待
     * 无法观测的问题；同时不受系统时间调整影响，并保证停止及时生效。
     */
    auto nextHeartbeatTime = std::chrono::steady_clock::now() +
                             std::chrono::seconds(STATUS_HEARTBEAT_INTERVAL_SEC);

    while (!m_bStopStatusHeartbeat.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (m_bStopStatusHeartbeat.load())
        {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now < nextHeartbeatTime)
        {
            continue;
        }
        nextHeartbeatTime = now + std::chrono::seconds(STATUS_HEARTBEAT_INTERVAL_SEC);
        if (m_pstMqtt != nullptr && m_pstMqtt->is_connected())
        {
            publish_device_status(true, "heartbeat");
        }
        else
        {
            dlog_info("MQTT 在线心跳到期但 MQTT 未连接，跳过发送");
        }
        //heartbeatLock.lock();
    }

    dlog_info("设备在线状态心跳线程退出");
}

int CPlatformManager::publish_device_status(bool bOnline, const std::string &strReason)
{
    NET_PlatformRuntimeStatus_S stStatus;
    if (!CPlatformSdkAdapter::instance()->get_status(stStatus))
    {
        return ERR_UNINIT;
    }
    dlog_debug("设备状态由 SDK 维护: requestedOnline[%d], reason[%s], mqtt[%d]",
               bOnline ? 1 : 0,
               strReason.c_str(),
               stStatus.bMqttConnected != FALSE ? 1 : 0);
    return !bOnline || stStatus.bMqttConnected != FALSE ? OK : ERR_UNINIT;
}

void CPlatformManager::disable_mqtt_for_platform()
{
    CPlatformSdkAdapter::instance()->stop_runtime();
}

#endif
