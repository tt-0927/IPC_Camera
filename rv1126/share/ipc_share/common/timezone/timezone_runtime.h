/**
 * @FilePath     : timezone_runtime.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-05 10:08:32
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 15:51:08
 * @Description  : 进程时区运行时管理
 */

#pragma once

#include <string>

#include "IpcRet.h"

namespace TimezoneRuntime_NS
{
/**
 * @brief   : 获取默认 POSIX 时区字符串
 * @return  {const char *} 默认 POSIX 时区字符串
 */
const char *get_default_timezone();

/**
 * @brief   : 时区枚举转换为 POSIX 时区字符串
 * @param   {int} nTimeZone：设备时区枚举值
 * @return  {const char *} POSIX 时区字符串
 * @note    : POSIX 时区偏移符号和 UTC 展示符号相反，例如 UTC+8 对应 CST-8
 */
const char *to_posix_timezone(int nTimeZone);

/**
 * @brief   : 读取持久化时区配置
 * @param   {std::string &} strTimezone：输出 POSIX 时区字符串
 * @return  {IpcRet_E} OK：成功，非 OK：失败
 */
IpcRet_E read_timezone_config(std::string &strTimezone);

/**
 * @brief   : 写入持久化时区配置
 * @param   {std::string} strTimezone：POSIX 时区字符串
 * @return  {IpcRet_E} OK：成功，非 OK：失败
 */
IpcRet_E write_timezone_config(const std::string &strTimezone);

/**
 * @brief   : 刷新当前进程时区环境
 * @param   {std::string} strProcessName：当前进程名称，用于日志定位
 * @param   {std::string} strReason：刷新原因，用于日志定位
 * @return  {IpcRet_E} OK：成功，非 OK：失败
 */
IpcRet_E reload_timezone(const std::string &strProcessName, const std::string &strReason);

/**
 * @brief   : 初始化当前进程时区并启动 SIGHUP 监听线程
 * @param   {std::string} strProcessName：当前进程名称，用于日志定位
 * @return  {IpcRet_E} OK：成功，非 OK：失败
 */
IpcRet_E init_timezone_runtime(const std::string &strProcessName);

/**
 * @brief   : 通知其他业务进程重新加载时区配置
 * @param   {std::string} strSourceProcess：发起通知的进程名称，用于日志定位
 * @return  {IpcRet_E} OK：全部通知成功，非 OK：至少一个进程通知失败
 */
IpcRet_E notify_timezone_reload(const std::string &strSourceProcess);
} // namespace TimezoneRuntime_NS
