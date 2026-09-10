/**
 * @FilePath     : isp_config_value.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 11:53:59
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-14 14:25:34
 * @Description  : ISP固定命令配置variant类型定义
 */

#pragma once

#include <variant>

#include "isp_define.h"

namespace ISP
{

/**
 * @brief 固定命令配置的统一variant类型。
 * @note  涵盖九类固定命令配置，供command service统一校验、持久化和应用。
 */
using IspConfigValue_T = std::variant<SceneType_E,
                                      ImageParam_S,
                                      ExposureAttr_S,
                                      DayNightAttr_S,
                                      BackLightArrt_S,
                                      AwbAttr_S,
                                      DnrAttr_S,
                                      VideoAdjust_S,
                                      SceneSchedule_S>;

} // namespace ISP
