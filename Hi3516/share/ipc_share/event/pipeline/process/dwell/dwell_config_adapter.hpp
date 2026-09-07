/**
 * @FilePath     : dwell_config_adapter.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 驻留事件配置适配器，1920×1080 像素坐标归一化到 [0,1]
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include "dwell_types.hpp"

namespace AiPipeline_NS
{
namespace Dwell_NS
{
/**
 * @brief   : 驻留事件配置适配器：只在配置边界做一次坐标转换
 */
class CDwellConfigAdapter
{
public:
    /**
     * @brief   : 转换并校验驻留配置
     * @param    {const RawDwellConfig_S &} stRawConfig：1920×1080 像素坐标配置（平台无关）
     * @param    {DwellConfig_S &} stOut：输出归一化内部配置
     * @return   {int} OK：成功，ERR_PARAM：配置非法（输出配置保持禁用状态）
     */
    int adapt(const RawDwellConfig_S &stRawConfig, DwellConfig_S &stOut) const;

private:
    /**
     * @brief   : 校验并归一化单条规则的多边形
     * @param    {const RawDwellRule_S &} stRawRule：原始规则
     * @param    {DwellRuleConfig_S &} stOutRule：输出归一化规则
     * @return   {bool} true：有效
     */
    bool adapt_rule(const RawDwellRule_S &stRawRule, DwellRuleConfig_S &stOutRule) const;
};
} // namespace Dwell_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
