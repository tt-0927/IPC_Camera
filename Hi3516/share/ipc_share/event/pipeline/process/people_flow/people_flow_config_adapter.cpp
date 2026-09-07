/**
 * @FilePath     : people_flow_config_adapter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 人流统计配置适配器实现
 */

#include "people_flow_config_adapter.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_AI_PEOPLE_STATISTICS

namespace AiPipeline_NS
{
namespace PeopleFlow_NS
{
namespace
{
/**
 * @brief   : 判断四边形是否自交
 * @param    {const std::vector<Geometry_NS::Point_S> &} vecQuad：四边形四点
 * @return   {bool} true：自交
 * @note    : 只检查不相邻对边是否相交，交点靠近端点的退化场景按有效处理
 */
bool is_quadrilateral_self_intersecting(const std::vector<Geometry_NS::Point_S> &vecQuad)
{
    if (vecQuad.size() != 4U)
    {
        return true;
    }

    /* 对边对：(0-1, 2-3) 与 (1-2, 3-0) */
    if (Geometry_NS::segments_intersect(vecQuad[0], vecQuad[1], vecQuad[2], vecQuad[3]) ||
        Geometry_NS::segments_intersect(vecQuad[1], vecQuad[2], vecQuad[3], vecQuad[0]))
    {
        return true;
    }

    return false;
}
} // namespace

int CPeopleFlowConfigAdapter::adapt(const Alarm::PeopleFlowStatistics_S &stConfig, PeopleFlowConfig_S &stOut) const
{
    stOut = PeopleFlowConfig_S();
    stOut.bEnabled = stConfig.bEnable;
    stOut.nSensitivity = stConfig.nSensitivity;
    /* 报告间隔为 0 时沿用默认 60 秒，保持旧处理器行为 */
    stOut.nReportIntervalSec = (stConfig.nReportInterval > 0U) ? stConfig.nReportInterval : 60U;
    stOut.enStatisticsType = stConfig.enStatisticsType;
    stOut.bTimedResetEnabled = stConfig.stTimedReset.bEnable;
    stOut.stResetTime = stConfig.stTimedReset.stExecuteTime;
    stOut.stStayAlarmNormal = stConfig.stStayAlarm.stNormal;
    stOut.stStayAlarmMedium = stConfig.stStayAlarm.stMedium;
    stOut.stStayAlarmSevere = stConfig.stStayAlarm.stSevere;

    if (stConfig.nSensitivity < 1U || stConfig.nSensitivity > 100U)
    {
        dlog_warn("人流统计配置非法：灵敏度[%u]超出 [1,100]，处理器保持禁用", stConfig.nSensitivity);
        stOut.bEnabled = false;
        return ERR_PARAM;
    }

    if (!adapt_rule_line(stConfig.stRuleLine, stOut.stRule) || !adapt_region(stConfig.stDetectRegion, stOut.stRule))
    {
        dlog_warn("人流统计配置非法：规则线或检测区域无效，处理器保持禁用");
        stOut.bEnabled = false;
        return ERR_PARAM;
    }

    stOut.stRule.bValid = true;
    return OK;
}

bool CPeopleFlowConfigAdapter::adapt_rule_line(const Alarm::PeopleFlowRuleLine_S &stRuleLine, PeopleFlowRuleConfig_S &stOutRule) const
{
    /* 规则线两点重合视为退化配置 */
    if (stRuleLine.stStartPos.fX == stRuleLine.stEndPos.fX && stRuleLine.stStartPos.fY == stRuleLine.stEndPos.fY)
    {
        return false;
    }

    /* 方向只支持 A->B 或 B->A */
    if (stRuleLine.enDirection != Alarm::CrossDirection_E::A_TO_B && stRuleLine.enDirection != Alarm::CrossDirection_E::B_TO_A)
    {
        return false;
    }

    const FrameSize_S stConfigSize{ PEOPLE_FLOW_CONFIG_WIDTH, PEOPLE_FLOW_CONFIG_HEIGHT };
    stOutRule.stLineStart = Geometry_NS::normalize_point(stRuleLine.stStartPos.fX, stRuleLine.stStartPos.fY, stConfigSize);
    stOutRule.stLineEnd = Geometry_NS::normalize_point(stRuleLine.stEndPos.fX, stRuleLine.stEndPos.fY, stConfigSize);
    stOutRule.enEnterDirection = stRuleLine.enDirection;
    return true;
}

bool CPeopleFlowConfigAdapter::adapt_region(const Alarm::Region_S &stRegion, PeopleFlowRuleConfig_S &stOutRule) const
{
    /* 人流统计区域固定为四边形 */
    if (stRegion.nPointNum != 4U || stRegion.aPoint.size() < 4U)
    {
        return false;
    }

    if (!stRegion.IsValid())
    {
        return false;
    }

    const FrameSize_S stConfigSize{ PEOPLE_FLOW_CONFIG_WIDTH, PEOPLE_FLOW_CONFIG_HEIGHT };
    std::vector<Geometry_NS::Point_S> vecQuad;
    vecQuad.reserve(4U);
    for (unsigned int i = 0; i < 4U; ++i)
    {
        vecQuad.emplace_back(Geometry_NS::normalize_point(stRegion.aPoint[i].fX, stRegion.aPoint[i].fY, stConfigSize));
    }

    if (is_quadrilateral_self_intersecting(vecQuad))
    {
        dlog_warn("人流统计检测区域自交，配置无效");
        return false;
    }

    stOutRule.vecRegion = std::move(vecQuad);
    return true;
}
} // namespace PeopleFlow_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_STATISTICS
