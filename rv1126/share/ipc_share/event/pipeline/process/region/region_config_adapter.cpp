/**
 * @FilePath     : region_config_adapter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 区域事件配置适配器实现
 */

#include "region_config_adapter.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_UNIFIED_EVENT_PIPELINE

namespace AiPipeline_NS
{
namespace Region_NS
{
namespace
{
/**
 * @brief   : 判断多边形是否自交
 * @param    {const std::vector<Geometry_NS::Point_S> &} vecPoly：多边形顶点
 * @return   {bool} true：自交
 * @note    : 检查不相邻边是否相交；少于 4 点跳过自交检查
 */
bool is_polygon_self_intersecting(const std::vector<Geometry_NS::Point_S> &vecPoly)
{
    const size_t nSize = vecPoly.size();
    if (nSize < 4U)
    {
        return false;
    }

    /* 检查所有不相邻的边对 */
    for (size_t i = 0; i < nSize; ++i)
    {
        const size_t iNext = (i + 1U) % nSize;
        for (size_t j = i + 2U; j < nSize; ++j)
        {
            /* 跳过首尾相邻的边 */
            if (i == 0U && j == nSize - 1U)
            {
                continue;
            }
            const size_t jNext = (j + 1U) % nSize;
            if (Geometry_NS::segments_intersect(vecPoly[i], vecPoly[iNext], vecPoly[j], vecPoly[jNext]))
            {
                return true;
            }
        }
    }
    return false;
}
} // namespace

int CRegionConfigAdapter::adapt(const RawRegionConfig_S &stRawConfig, RegionConfig_S &stOut) const
{
    stOut = RegionConfig_S();
    stOut.bEnabled = stRawConfig.bEnabled;

    /* 逐条规则校验并归一化 */
    for (const auto &stRawRule : stRawConfig.vecRules)
    {
        RegionRuleConfig_S stRule;
        if (!adapt_rule(stRawRule, stRule))
        {
            dlog_warn("区域配置规则[%d]无效，跳过该规则", stRawRule.nRuleId);
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
            dlog_warn("区域配置无有效规则，处理器保持禁用");
        }
        stOut.bEnabled = false;
        return ERR_PARAM;
    }

    return OK;
}

bool CRegionConfigAdapter::adapt_rule(const RawRegionRule_S &stRawRule, RegionRuleConfig_S &stOutRule) const
{
    /* 事件类型校验 */
    if (stRawRule.enEventType != Event::Type_E::INTRUSION &&
        stRawRule.enEventType != Event::Type_E::ENTER_REGION &&
        stRawRule.enEventType != Event::Type_E::LEAVE_REGION)
    {
        dlog_warn("区域规则事件类型非法[%d]", static_cast<int>(stRawRule.enEventType));
        return false;
    }

    /* 多边形顶点数量校验，至少 3 点 */
    if (stRawRule.vecPolygon.size() < 3U)
    {
        return false;
    }

    /* 1920×1080 基准归一化 */
    const FrameSize_S stConfigSize{ REGION_CONFIG_WIDTH, REGION_CONFIG_HEIGHT };
    std::vector<Geometry_NS::Point_S> vecNormalized;
    vecNormalized.reserve(stRawRule.vecPolygon.size());
    for (const auto &stPoint : stRawRule.vecPolygon)
    {
        vecNormalized.emplace_back(Geometry_NS::normalize_point(stPoint.dX, stPoint.dY, stConfigSize));
    }

    /* 自交检查 */
    if (is_polygon_self_intersecting(vecNormalized))
    {
        dlog_warn("区域规则[%d]多边形自交", stRawRule.nRuleId);
        return false;
    }

    stOutRule.enEventType = stRawRule.enEventType;
    stOutRule.vecPolygon = std::move(vecNormalized);
    stOutRule.nTimeThresholdSec = stRawRule.nTimeThresholdSec;
    stOutRule.nSensitivity = stRawRule.nSensitivity;
    stOutRule.nDetectionTargetMask = stRawRule.nDetectionTargetMask;
    stOutRule.nRuleId = stRawRule.nRuleId;
    return true;
}
} // namespace Region_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
