/**
 * @FilePath     : people_head_detect_common.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 09:13:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:31:25
 * @Description  : 人头检测通用工具实现
 */

#include "people_head_detect_common.hpp"

#include <string>

#include "dlog.h"

namespace PeopleHeadDetectInternal
{
void convert_crowd_config_and_enable(Alarm::CrowdGathering_S &stAlgoCfg, int nWidth, int nHeight)
{
    if (stAlgoCfg.aRule.empty())
    {
        stAlgoCfg.bEnable = false;
        return;
    }

    /* 所有规则都完成坐标转换，只要存在一个合法区域即可保持使能 */
    bool bHasValidRegion = false;
    for (auto &stRule : stAlgoCfg.aRule)
    {
        /* 当前规则区域转换到人头模型输入分辨率 */
        stRule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);
        if (stRule.stRegion.IsValid())
        {
            bHasValidRegion = true;
        }
    }

    if (!bHasValidRegion)
    {
        stAlgoCfg.bEnable = false;
        dlog_warn("人员聚集配置无有效区域，关闭人员聚集处理器");
    }
}

#if CAP_AI_PEOPLE_STATISTICS
void convert_density_config_and_enable(Alarm::PeopleDensityDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    /* 人员密度检测区域转换到人头模型输入分辨率 */
    stAlgoCfg.stDetectRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);
    if (!stAlgoCfg.stDetectRegion.IsValid())
    {
        stAlgoCfg.bEnable = false;
        dlog_warn("人员密度配置无有效区域，关闭人员密度处理器");
    }
}

std::string get_density_level_text(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    case Event::Type_E::PEOPLE_DENSITY_NORMAL:
        return "normal";
    case Event::Type_E::PEOPLE_DENSITY_MEDIUM:
        return "medium";
    case Event::Type_E::PEOPLE_DENSITY_SEVERE:
        return "severe";
    default:
        return "none";
    }
}

EventTriggerContext_S build_density_context(const SPeopleHeadProcessContext &stContext,
                                            Event::Type_E enEventType,
                                            uint32_t nPeopleCount,
                                            Event::Type_E enAlarmEventType)
{
    /* 人员密度联动上下文仅承载等级报警需要的规则匹配摘要，不承担周期统计数据上报 */
    EventTriggerContext_S stLinkageContext;
    stLinkageContext.enEventType = enEventType;
    stLinkageContext.nChnId = stContext.nChnId;
    stLinkageContext.llTimestamp = stContext.llNowMs;
    stLinkageContext.mapAttrs["rule_id"] = "0";
    stLinkageContext.mapAttrs["current_people_count"] = std::to_string(nPeopleCount);
    stLinkageContext.mapAttrs["alarm_level"] = get_density_level_text(enAlarmEventType);
    return stLinkageContext;
}

EventStatistics_NS::TargetSnapshot_S build_density_snapshot(const Inference_NS::BoxData_S &stBoxData,
                                                            long long llNowMs)
{
    /* 人员密度当前区域目标快照，记录人头框和统计时间 */
    EventStatistics_NS::TargetSnapshot_S stSnapshot;
    stSnapshot.nRuleId = 0;
    stSnapshot.enSnapshotType = EventStatistics_NS::SnapshotType_E::REGION_CURRENT;
    stSnapshot.stRect.nX1 = stBoxData.stBoxs.nX1;
    stSnapshot.stRect.nY1 = stBoxData.stBoxs.nY1;
    stSnapshot.stRect.nX2 = stBoxData.stBoxs.nX2;
    stSnapshot.stRect.nY2 = stBoxData.stBoxs.nY2;
    stSnapshot.llTimestampMs = llNowMs;
    return stSnapshot;
}
#endif
} // namespace PeopleHeadDetectInternal
