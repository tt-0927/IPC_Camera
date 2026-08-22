/*
 * @FilePath     : sdk_new/sdk_server/src/business/UrlParamUtils.h
 * @Author       : ITC
 * @Date         : 2026-08-19
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-19
 * @Description  : URL参数解析公共工具（业务层共用）
 *                 提供HTTP查询字符串中参数的解析能力，供各业务模块
 *                 （DeviceConfigBusiness/NvrBusiness/IpcBusiness等）统一使用，
 *                 避免各业务类重复实现解析逻辑。
 *                 - ParseIntParam    : 解析整型参数
 *                 - ParseStringParam : 解析字符串参数（自动URL解码）
 *                 - UrlDecode        : URL百分号解码
 */

#ifndef SDK_SERVER_BUSINESS_URL_PARAM_UTILS_H
#define SDK_SERVER_BUSINESS_URL_PARAM_UTILS_H

#include <string>
#include <cctype>
#include <cstdlib>

/**
 * URL百分号解码
 * @details 将 %XX 形式的转义字符还原为原始字节，'+' 还原为空格
 * @param value 待解码的URL编码字符串
 * @return 解码后的字符串
 */
inline std::string UrlDecode(const std::string& value)
{
    std::string out;
    out.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i)
    {
        char ch = value[i];
        if (ch == '+')
        {
            out.push_back(' ');
            continue;
        }

        if (ch == '%' && i + 2 < value.size() &&
            std::isxdigit((unsigned char)value[i + 1]) &&
            std::isxdigit((unsigned char)value[i + 2]))
        {
            char hex[3] = {value[i + 1], value[i + 2], '\0'};
            out.push_back((char)std::strtol(hex, NULL, 16));
            i += 2;
            continue;
        }

        out.push_back(ch);
    }

    return out;
}

/**
 * 解析URL查询参数中的整数值
 * @details 从 "key1=value1&key2=value2" 形式的查询串中按key定位并解析整数值
 * @param url_param URL参数字符串（如 "ChnId=1&Type=2"）
 * @param key 参数名
 * @param defaultVal 默认值（未找到或解析失败时返回）
 * @return 解析到的整数值，未找到或解析失败则返回默认值
 */
inline int ParseIntParam(const std::string& url_param, const std::string& key, int defaultVal = 0)
{
    std::string searchKey = key + "=";
    size_t pos = url_param.find(searchKey);
    if (pos == std::string::npos)
    {
        return defaultVal;
    }

    size_t valueStart = pos + searchKey.length();
    size_t valueEnd = url_param.find('&', valueStart);
    if (valueEnd == std::string::npos)
    {
        valueEnd = url_param.length();
    }

    std::string valueStr = url_param.substr(valueStart, valueEnd - valueStart);
    try
    {
        return std::stoi(valueStr);
    }
    catch (...)
    {
        return defaultVal;
    }
}

/**
 * 解析URL查询参数中的字符串值
 * @details 从 "key1=value1&key2=value2" 形式的查询串中按key定位字符串值，
 *          并自动进行URL百分号解码
 * @param url_param URL参数字符串
 * @param key 参数名
 * @param defaultVal 默认值（未找到时返回）
 * @return 解码后的字符串值，未找到则返回默认值
 */
inline std::string ParseStringParam(const std::string& url_param, const std::string& key, const std::string& defaultVal = "")
{
    std::string searchKey = key + "=";
    size_t pos = url_param.find(searchKey);
    if (pos == std::string::npos)
    {
        return defaultVal;
    }

    size_t valueStart = pos + searchKey.length();
    size_t valueEnd = url_param.find('&', valueStart);
    if (valueEnd == std::string::npos)
    {
        valueEnd = url_param.length();
    }

    return UrlDecode(url_param.substr(valueStart, valueEnd - valueStart));
}

#endif /* SDK_SERVER_BUSINESS_URL_PARAM_UTILS_H */
