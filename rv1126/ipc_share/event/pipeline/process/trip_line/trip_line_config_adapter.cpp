/**
 * @FilePath     : trip_line_config_adapter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 越界事件配置适配器实现
 */

#include "trip_line_config_adapter.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_UNIFIED_EVENT_PIPELINE

namespace AiPipeline_NS
{
namespace TripLine_NS
{
int CTripLineConfigAdapter::adapt(const RawTripLineConfig_S &stRawConfig,
                                  TripLineConfig_S &stOut) const
{
    stOut = TripLineConfig_S();
    stOut.bEnabled = stRawConfig.bEnabled;

    /* 逐条规则校验并归一化 */
    for (const auto &stRawRule : stRawConfig.vecRules)
    {
        TripLineRuleConfig_S stRule;
        if (!adapt_rule(stRawRule, stRule))
        {
            dlog_warn("越界配置规则[%d]无效，跳过该规则", stRawRule.nRuleId);
            continue;
        }
        stRule.bValid = true;
        stOut.vecRules.emplace_back(std::move(stRule));
    }

    /* 无有效规则时保持禁用 */
    if (stOut.vecRules.empty())
    {
        if (stOut.bEnabled)
        {
            dlog_warn("越界配置无有效规则，处理器保持禁用");
        }
        stOut.bEnabled = false;
        return ERR_PARAM;
    }

    return OK;
}

bool CTripLineConfigAdapter::adapt_rule(const RawTripLineRule_S &stRawRule, TripLineRuleConfig_S &stOutRule) const
{
    /* 穿越方向校验 */
    if (stRawRule.enCrossDirection == Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID)
    {
        dlog_warn("越界规则[%d]穿越方向非法", stRawRule.nRuleId);
        return false;
    }

    /* 1920×1080 基准归一化 */
    const FrameSize_S stConfigSize{ TRIP_LINE_CONFIG_WIDTH, TRIP_LINE_CONFIG_HEIGHT };
    const Geometry_NS::Point_S stStartNorm = Geometry_NS::normalize_point(stRawRule.stLineStart.dX,
                                                                          stRawRule.stLineStart.dY,
                                                                          stConfigSize);
    const Geometry_NS::Point_S stEndNorm = Geometry_NS::normalize_point(stRawRule.stLineEnd.dX,
                                                                        stRawRule.stLineEnd.dY,
                                                                        stConfigSize);

    /* 退化线检查：起点终点重合视为无效（等价旧 convert_boundary_and_enable 的 bIsInit 逻辑） */
    if (stStartNorm == stEndNorm)
    {
        dlog_warn("越界规则[%d]规则线退化（起点终点重合）", stRawRule.nRuleId);
        return false;
    }

    stOutRule.stLineStart = stStartNorm;
    stOutRule.stLineEnd = stEndNorm;
    stOutRule.enCrossDirection = stRawRule.enCrossDirection;
    stOutRule.nSensitivity = stRawRule.nSensitivity;
    stOutRule.nDetectionTargetMask = stRawRule.nDetectionTargetMask;
    stOutRule.nRuleId = stRawRule.nRuleId;
    return true;
}
} // namespace TripLine_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
