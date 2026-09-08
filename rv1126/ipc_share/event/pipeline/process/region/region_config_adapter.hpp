/**
 * @FilePath     : region_config_adapter.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 区域事件配置适配器，1920×1080 像素坐标归一化到 [0,1]
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include "region_types.hpp"

namespace AiPipeline_NS
{
namespace Region_NS
{
/**
 * @brief   : 区域事件配置适配器：只在配置边界做一次坐标转换
 */
class CRegionConfigAdapter
{
public:
    /**
     * @brief   : 转换并校验区域配置
     * @param    {const RawRegionConfig_S &} stRawConfig：1920×1080 像素坐标配置（平台无关）
     * @param    {RegionConfig_S &} stOut：输出归一化内部配置
     * @return   {int} OK：成功，ERR_PARAM：配置非法（输出配置保持禁用状态）
     */
    int adapt(const RawRegionConfig_S &stRawConfig, RegionConfig_S &stOut) const;

private:
    /**
     * @brief   : 校验并归一化单条规则的多边形
     * @param    {const RawRegionRule_S &} stRawRule：原始规则
     * @param    {RegionRuleConfig_S &} stOutRule：输出归一化规则
     * @return   {bool} true：有效
     */
    bool adapt_rule(const RawRegionRule_S &stRawRule, RegionRuleConfig_S &stOutRule) const;
};
} // namespace Region_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
