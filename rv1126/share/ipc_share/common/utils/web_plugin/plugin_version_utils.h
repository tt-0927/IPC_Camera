/**
 * @FilePath     : plugin_version_utils.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 11:04:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 11:34:01
 * @Description  : 板端网页插件版本读取工具声明
 */

#pragma once

#include <string>

/**
 * @brief   : 板端网页插件版本读取工具
 * @note    : 版本以 S81appinit 维护的 IpcComponents.exe 软链接目标为准，
 *            不依赖业务程序的编译期版本常量。
 */
namespace PluginVersionUtils_NS
{
/**
 * @brief   : 获取当前板端实际启用的网页插件版本
 * @return  {std::string} 成功返回例如“V2.0.16”；链接、目标文件或命名非法时返回“Unknown”
 * @note    : 仅接受 IpcComponents-V<版本>.exe 格式，返回值保留文件名中的 V 前缀，
 *            与 device_info.json 的 PluginVersion 字段格式保持一致。
 */
std::string get_active_version();
} // namespace PluginVersionUtils_NS
