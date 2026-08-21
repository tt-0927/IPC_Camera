/**
 * @FilePath     : isp_tuning_builder.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:17:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 10:55:46
 * @Description  : Hi3516 ISP配置画像构建与Sensor静态资源解析接口
 */

#pragma once

#include <string>

#include "isp_tuning_profile.h"

/* 构建 Hi3516 ISP 调参数据。 */
namespace IspTuningBuilder_NS
{
/**
 * @brief   : 加载参数映射和运行策略，并绑定编译期Sensor静态资源
 * @param    {const std::string &} strConfigDir：ISP配置目录
 * @param    {Hi3516TuningProfile_S &} stProfile：输出调参数据
 * @return   {int} OK：成功，非OK：失败
 * @note     : 机型与焦距差异来自配置文件，编译期只保留Sensor驱动资源选择
 */
int build_tuning_profile(const std::string &strConfigDir, Hi3516TuningProfile_S &stProfile);

/**
 * @brief   : 获取编译期传感器类型值
 * @param    {void}
 * @return   {int} sample_sns_type对应的传感器类型值
 * @note     : ISP底层先于调参数据设置时使用默认值；Sensor宏仍只在本构建器读取。
 */
int get_sensor_type();
} // namespace IspTuningBuilder_NS
