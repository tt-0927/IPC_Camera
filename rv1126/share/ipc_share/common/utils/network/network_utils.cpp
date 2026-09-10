/**
 * @FilePath     : network_utils.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-16 19:54:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-29 10:44:27
 * @DescrstrIption  : 网络工具
 */

#include "network_utils.h"
#include "dlog.h"

namespace NetworkUtils_NS
{
    /**
     * @brief   : 是否为有效的IP地址
     * @param    {string&} strIp
     * @return   {bool} true 有效 false 无效
     */
    bool isValidIpAddress(const std::string &strIp)
    {
        /* 基本长度检查：最短"0.0.0.0"(7字符)，最长"255.255.255.255"(15字符) */
        if (strIp.length() < 7 || strIp.length() > 15)
        {
            dlog_debug("strIp.length(): %d",strIp.length());
            return false;
        }

        std::vector<std::string> parts;
        std::stringstream ssIp(strIp);
        std::string part;

        /* 按点分割IP地址 */
        while (std::getline(ssIp, part, '.'))
        {
            parts.push_back(part);
        }

        /* IP必须由4个部分组成 */
        if (parts.size() != 4)
        {
            return false;
        }

        for (const auto &p : parts)
        {
            /* 每个部分不能是空字符串 */
            if (p.empty() || p.length() > 3)  /* 最多3位数字 */
            {
                return false;
            }

            /* 每个部分只能包含数字 */
            for (char c : p)
            {
                if (!isdigit(c))
                {
                    return false;
                }
            }

            /* 检查前导零（"0"是有效的，但"01"、"001"等无效） */
            if (p.length() > 1 && p[0] == '0')
            {
                return false;
            }

            /* 转换为整数检查范围（0-255）*/
            int num;
            try
            {
                num = std::stoi(p);
            }
            catch (const std::exception&)
            {
                /* 转换失败（如数字过大） */
                return false;
            }

            if (num < 0 || num > 255)
            {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief   : 比较两个IP地址是否相同
     * @note    : 先验证格式，再比较内容
     * @param    {string} &ip1 IP地址1
     * @param    {string} &ip2 IP地址2
     * @return   {bool} true 相同 false 不同
     */
    bool areIpsEqual(const std::string &ip1, const std::string &ip2)
    {
        /* 如果字符串完全相同，直接返回true（性能优化） */
        if (ip1 == ip2)
        {
            /* 但仍需验证格式有效性 */
            return isValidIpAddress(ip1);
        }

        /* 先验证两个IP的格式是否有效 */
        if (!isValidIpAddress(ip1) || !isValidIpAddress(ip2))
        {
            return false; // 格式无效时视为不相等
        }

        /* 字符串不同但都是有效IP，则不相等 */
        return false;
    }

} // namespace NetworkUtils_NS