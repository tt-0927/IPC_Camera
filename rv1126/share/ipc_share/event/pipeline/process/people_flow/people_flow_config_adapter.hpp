/**
 * @FilePath     : people_flow_config_adapter.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 人流统计配置适配器，1920×1080 配置转换为归一化内部配置
 */

#pragma once

#if CAP_AI_PEOPLE_STATISTICS

#include "alarm_define.h"
#include "people_flow_types.hpp"

namespace AiPipeline_NS
{
namespace PeopleFlow_NS
{
/* 人流统计配置适配器：只在配置边界做一次坐标转换 */
class CPeopleFlowConfigAdapter
{
public:
    /**
     * @brief   : 转换并校验人流统计配置
     * @param    {const Alarm::PeopleFlowStatistics_S &} stConfig：现有 1920×1080 配置
     * @param    {PeopleFlowConfig_S &} stOut：输出归一化内部配置
     * @return   {int} OK：成功，ERR_PARAM：配置非法（输出配置保持禁用状态）
     * @note    : 规则线退化、区域非四点/自交、方向非法时返回 ERR_PARAM
     */
    int adapt(const Alarm::PeopleFlowStatistics_S &stConfig, PeopleFlowConfig_S &stOut) const;

private:
    /**
     * @brief   : 校验并转换规则线
     * @param    {const Alarm::PeopleFlowRuleLine_S &} stRuleLine：规则线配置
     * @param    {PeopleFlowRuleConfig_S &} stOutRule：输出归一化规则
     * @return   {bool} true：有效
     */
    bool adapt_rule_line(const Alarm::PeopleFlowRuleLine_S &stRuleLine, PeopleFlowRuleConfig_S &stOutRule) const;

    /**
     * @brief   : 校验并转换检测区域
     * @param    {const Alarm::Region_S &} stRegion：检测区域配置
     * @param    {PeopleFlowRuleConfig_S &} stOutRule：输出归一化规则
     * @return   {bool} true：有效
     */
    bool adapt_region(const Alarm::Region_S &stRegion, PeopleFlowRuleConfig_S &stOutRule) const;
};
} // namespace PeopleFlow_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_STATISTICS
