/**
 * @FilePath     : isp_profile_config_loader.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-27 10:19:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 10:55:46
 * @Description  : Hi3516 ISP参数映射与运行策略配置加载接口
 */

#pragma once

#include <string>

#include "isp_tuning_profile.h"

namespace IspProfileConfigLoader_NS
{
/**
 * @brief   : 从ISP配置目录加载参数映射和运行策略
 * @param    {const std::string &} strConfigDir：ISP配置目录
 * @param    {const std::string &} strSensorType：编译时Sensor与焦距标识
 * @param    {const std::string &} strDeviceType：编译时设备型号
 * @param    {Hi3516TuningProfile_S &} stProfile：输出只读画像
 * @return   {int} OK：成功，ERR_OPEN/ERR_PARSE/ERR_PARAM：失败
 * @note    : 两份配置必须同时完整有效，失败时不会修改输出画像
 */
int load(const std::string &strConfigDir,
         const std::string &strSensorType,
         const std::string &strDeviceType,
         Hi3516TuningProfile_S &stProfile);
} // namespace IspProfileConfigLoader_NS
