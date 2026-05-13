/**
 * @FilePath     : ssh_service.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-16 10:25:57
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-16 14:28:14
 * @Description  : SSH 服务管理
 */

#pragma once

#include <string>

#include "system_define.h"

class SshServiceManager
{
public:
    /**
     * @brief   : 应用 SSH 服务配置
     * @param    {System::SecurityServices_S &} stInfo：安全服务配置
     * @param    {bool} bAutoMaintain：自动维护是否开启
     * @return   {int} 0：成功，非0：失败
     * @note    : 会同步处理 SSH 启停、自动关闭脚本和倒计时回填
     */
    int apply(System::SecurityServices_S &stInfo, bool bAutoMaintain);

    /**
     * @brief   : 获取 SSH 剩余运行时间
     * @param    {int} port：SSH 服务端口
     * @param    {std::string &} strCountdown：剩余时间字符串
     * @return   {int} 0：成功，非0：失败
     * @note    : 当 SSH 已关闭时，统一返回默认的 08:00:00
     */
    int get_countdown(int port, std::string &strCountdown) const;
};
