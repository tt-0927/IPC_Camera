/**
 * @FilePath     : network_utils.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-16 19:54:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-16 20:01:52
 * @Description  : 网络工具
 */

#pragma once

#include <string>
#include <sstream>
#include <vector>

/**
* @brief   : 网络工具
*/
namespace NetworkUtils_NS
{
    /**
     * @brief   : 是否为有效的IP地址
     * @param    {string&} strIp 需要判断的IP地址
     * @return   {bool} true 有效 false 无效
     */
    bool isValidIpAddress(const std::string& strIp);

    /**
     * @brief   : 比较两个IP地址是否相同
     * @note    : 先验证格式，再比较内容
     * @param    {string} &ip1 IP地址1
     * @param    {string} &ip2 IP地址2
     * @return   {bool} true 相同 false 不同
     */
    bool areIpsEqual(const std::string &ip1, const std::string &ip2);

}  // namespace NetworkUtils_NS
