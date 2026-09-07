/**
 * @FilePath     : exhibition_density_output_executor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-26 15:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 人员密度新链路输出执行器实现
 */

#include "exhibition_density_output_executor.hpp"

#include <string>

#include "IpcRet.h"
#include "dlog.h"
#include "event_linkage_types.h"
#include "normalized_geometry.hpp"

#if CAP_AI_PEOPLE_DENSITY_PIPELINE

namespace
{
/* 统计类型数值：与 EventStatistics_NS::StatisticsType_E::PEOPLE_DENSITY 对齐 */
constexpr int DENSITY_STATISTICS_TYPE = static_cast<int>(EventStatistics_NS::StatisticsType_E::PEOPLE_DENSITY);

/**
 * @brief   : 获取密度报警等级文本
 * @param    {Event::Type_E} enEventType：密度报警事件类型
 * @return   {std::string} 报警等级文本
 */
std::string alarm_level_text(Event::Type_E enEventType)
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
} // namespace

void CExhibitionDensityOutputExecutor::set_reporter(
    const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
    m_pReporter = pReporter;
}

void CExhibitionDensityOutputExecutor::set_image_provider(AiPipeline_NS::IFrameImageProvider *pProvider)
{
    m_pImageProvider = pProvider;
}

void CExhibitionDensityOutputExecutor::set_native_result_size(const AiPipeline_NS::FrameSize_S &stSize)
{
    m_stNativeResultSize = stSize;
}

void CExhibitionDensityOutputExecutor::process(const AiPipeline_NS::ProcessorOutput_S &stOutput)
{
    /* 密度统计报告提交：仅消费 PEOPLE_DENSITY 类型草稿 */
    for (const auto &stDraft : stOutput.vecStatisticsDrafts)
    {
        if (stDraft.nStatisticsType != DENSITY_STATISTICS_TYPE)
        {
            continue;
        }
        EventStatistics_NS::Report_S stReport = build_report(stDraft);
        if (m_pReporter != nullptr)
        {
            fill_report_panorama(stReport, stOutput);
            m_pReporter->report(stReport);
        }
    }

    /* 密度三级报警状态机 */
    for (const auto &stCondition : stOutput.vecEventConditions)
    {
        const EventTriggerContext_S stContext = build_event_context(stCondition);
        switch (stCondition.enEventType)
        {
        case Event::Type_E::PEOPLE_DENSITY_NORMAL:
            m_normalAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        case Event::Type_E::PEOPLE_DENSITY_MEDIUM:
            m_mediumAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        case Event::Type_E::PEOPLE_DENSITY_SEVERE:
            m_severeAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        default:
            break;
        }
    }
}

void CExhibitionDensityOutputExecutor::reset()
{
    m_normalAlarmStateMachine.reset();
    m_mediumAlarmStateMachine.reset();
    m_severeAlarmStateMachine.reset();
}

EventStatistics_NS::Report_S CExhibitionDensityOutputExecutor::build_report(
    const AiPipeline_NS::StatisticsReportDraft_S &stDraft)
{
    EventStatistics_NS::Report_S stReport;
    stReport.enStatisticsType = EventStatistics_NS::StatisticsType_E::PEOPLE_DENSITY;
    stReport.enEventType = Event::Type_E::PEOPLE_DENSITY_DETECTION;
    stReport.enAlarmEventType = stDraft.enAlarmEventType;
    stReport.nChnId = stDraft.nChannelId;
    stReport.nRuleId = stDraft.nRuleId;
    stReport.llFrameTimestampMs = stDraft.llTimestampMs;
    stReport.nReportSeq = stDraft.nReportSeq;
    stReport.nCurrentPeopleCount = stDraft.nCurrentPeopleCount;
    stReport.nAverageStayTimeSec = stDraft.nAverageStayTimeSec;

    /* 目标快照：归一化框转回原生结果坐标，快照类型固定为区域当前目标 */
    stReport.vecTargets.reserve(stDraft.vecTargets.size());
    for (const auto &stTarget : stDraft.vecTargets)
    {
        EventStatistics_NS::TargetSnapshot_S stSnapshot;
        stSnapshot.nTrackId = static_cast<int>(stTarget.ullTrackId);
        stSnapshot.nRuleId = stDraft.nRuleId;
        stSnapshot.enSnapshotType = EventStatistics_NS::SnapshotType_E::REGION_CURRENT;
        stSnapshot.stRect = AiPipeline_NS::Geometry_NS::denormalize_rect(stTarget.stRect, m_stNativeResultSize);
        stSnapshot.llTimestampMs = stDraft.llTimestampMs;
        stSnapshot.nDirection = 0;
        stReport.vecTargets.emplace_back(std::move(stSnapshot));
    }
    return stReport;
}

EventTriggerContext_S CExhibitionDensityOutputExecutor::build_event_context(
    const AiPipeline_NS::EventCondition_S &stCondition)
{
    /* 联动上下文与旧 HVF 密度实现对齐：仅承载规则匹配所需摘要属性 */
    EventTriggerContext_S stContext;
    stContext.enEventType = stCondition.enEventType;
    stContext.nChnId = stCondition.nChannelId;
    stContext.llTimestamp = stCondition.llWallTimestampMs;
    stContext.mapAttrs["rule_id"] = std::to_string(stCondition.nRuleId);
    stContext.mapAttrs["current_people_count"] = std::to_string(stCondition.nCurrentPeopleCount);
    stContext.mapAttrs["alarm_level"] = alarm_level_text(stCondition.enEventType);
    return stContext;
}

void CExhibitionDensityOutputExecutor::fill_report_panorama(EventStatistics_NS::Report_S &stReport,
                                                            const AiPipeline_NS::ProcessorOutput_S &stOutput)
{
    if (m_pImageProvider == nullptr || !m_pReporter->shouldBuildHeavyPayload())
    {
        return;
    }

    /* 按报告序号与统计类型关联全景图请求，避免与人流统计序号空间重叠错配 */
    for (const auto &stRequest : stOutput.vecImageRequests)
    {
        if (stRequest.nReportSeq != stReport.nReportSeq || stRequest.nStatisticsType != DENSITY_STATISTICS_TYPE ||
            !stRequest.bPanorama)
        {
            continue;
        }

        EventStatistics_NS::ImagePayload_S stImage;
        if (m_pImageProvider->build_panorama(stImage) == OK)
        {
            stReport.stPanoramaImage = std::move(stImage);
        }
        else
        {
            dlog_warn("人员密度全景图构建失败，报告[%u]继续提交", stReport.nReportSeq);
        }
        break;
    }
}

#endif // CAP_AI_PEOPLE_DENSITY_PIPELINE
