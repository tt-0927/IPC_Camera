/**
 * @FilePath     : isp_capability_validator.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 11:39:09
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : ISP平台功能和参数范围一致性校验
 */

#pragma once

#include "isp_capability_profile.h"

/**
 * @brief ISP 平台功能和参数范围一致性校验工具。
 */
namespace IspCapabilityValidator_NS
{

/**
 * @brief   : 校验平台功能和参数范围是否自洽
 * @param    {const ISP::IspCapabilityProfile_S&} stProfile：平台功能和参数范围
 * @return   {int} OK：校验通过，ERR_PARAM：存在不一致
 * @note    : 校验规则包括：互斥能力冲突、支持功能集合非空、range合法性、
 *            场景与IR-CUT/红外灯依赖关系
 */
int validate_profile(const ISP::IspCapabilityProfile_S &stProfile);

} // namespace IspCapabilityValidator_NS
