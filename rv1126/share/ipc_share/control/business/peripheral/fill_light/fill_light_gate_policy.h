/**
 * @FilePath     : fill_light_gate_policy.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 10:39:59
 * @Description  : 外设补光一级总控时间准入与物理功率限幅纯策略声明
 */

#pragma once

#include "fill_light_gate_state.h"
#include "peripheral_fill_light_config.h"

namespace FillLightPolicy_NS
{

/**
 * @brief   : 根据外设补光配置和当前时间计算一级总控状态
 * @param    {const Peripheral_NS::FillLightGlobalConfig_S&} stConfig：外设补光配置
 * @param    {int} nNowSecOfDay：当前当天秒数，允许超出一天范围
 * @return   {Peripheral_NS::FillLightGateState_S} 总控准入、功率上限和禁止原因
 */
Peripheral_NS::FillLightGateState_S evaluate_gate(const Peripheral_NS::FillLightGlobalConfig_S &stConfig, int nNowSecOfDay);

/**
 * @brief   : 计算受全局物理功率上限约束的实际灯光强度
 * @param    {unsigned int} nRequestedLevel：场景或告警请求强度
 * @param    {unsigned int} nPowerLimitPercent：外设全局功率上限
 * @return   {unsigned int} 四舍五入后的实际输出强度，范围为0到100
 */
unsigned int calculate_output_level(unsigned int nRequestedLevel, unsigned int nPowerLimitPercent);

} // namespace FillLightPolicy_NS
