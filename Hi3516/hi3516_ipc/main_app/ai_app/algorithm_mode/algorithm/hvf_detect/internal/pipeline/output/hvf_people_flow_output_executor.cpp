/**
 * @FilePath     : hvf_people_flow_output_executor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 人流统计新链路输出执行器实现
 */

#include "hvf_people_flow_output_executor.hpp"

#include <algorithm>
#include <string>

#include "IpcRet.h"
#include "dlog.h"
#include "event_linkage_types.h"
#include "normalized_geometry.hpp"

#if CAP_AI_PEOPLE_STATISTICS

namespace HVFDetectInternal
{
namespace
{
/**
 * @brief   : 获取滞留报警等级文本
 * @param    {Event::Type_E} enEventType：滞留报警事件类型
 * @return   {std::string} 报警等级文本
 */
std::string alarm_level_text(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    case Event::Type_E::PEOPLE_FLOW_STAY_NORMAL:
        return "normal";
    case Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM:
        return "medium";
    case Event::Type_E::PEOPLE_FLOW_STAY_SEVERE:
        return "severe";
    default:
        return "none";
    }
}
} // namespace

CHVFPeopleFlowOutputExecutor::CHVFPeopleFlowOutputExecutor() = default;

void CHVFPeopleFlowOutputExecutor::set_reporter(const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
    m_pReporter = pReporter;
}

void CHVFPeopleFlowOutputExecutor::set_image_provider(AiPipeline_NS::IFrameImageProvider *pProvider)
{
    m_pImageProvider = pProvider;
}

void CHVFPeopleFlowOutputExecutor::set_native_result_size(const AiPipeline_NS::FrameSize_S &stSize)
{
    m_stNativeResultSize = stSize;
}

void CHVFPeopleFlowOutputExecutor::process(const AiPipeline_NS::ProcessorOutput_S &stOutput, std::vector<Common::RectInfo_S> &vstRectInfo)
{
    /* 统计报告提交 */
    for (const auto &stDraft : stOutput.vecStatisticsDrafts)
    {
        /* 人员密度类型草稿由密度输出执行器消费，此处跳过 */
        if (stDraft.nStatisticsType == static_cast<int>(EventStatistics_NS::StatisticsType_E::PEOPLE_DENSITY))
        {
            continue;
        }
        EventStatistics_NS::Report_S stReport = build_report(stDraft);
        if (m_pReporter != nullptr)
        {
            fill_report_images(stReport, stOutput);
            m_pReporter->report(stReport);
        }
    }

    /* 滞留三级报警状态机 */
    for (const auto &stCondition : stOutput.vecEventConditions)
    {
        const EventTriggerContext_S stContext = build_event_context(stCondition);
        switch (stCondition.enEventType)
        {
        case Event::Type_E::PEOPLE_FLOW_STAY_NORMAL:
            m_normalAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        case Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM:
            m_mediumAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        case Event::Type_E::PEOPLE_FLOW_STAY_SEVERE:
            m_severeAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        default:
            break;
        }
    }

    /* OSD 叠加：归一化框转回原生结果坐标后追加到本帧汇总框数组 */
    for (const auto &stOverlayItem : stOutput.vecOverlayItems)
    {
        if (!AiPipeline_NS::Geometry_NS::is_valid_frame_size(m_stNativeResultSize))
        {
            continue;
        }
        const Common::RectInfo_S stRectInfo = AiPipeline_NS::Geometry_NS::denormalize_rect(stOverlayItem.stRect, m_stNativeResultSize);
        if (stRectInfo.nX2 > stRectInfo.nX1 && stRectInfo.nY2 > stRectInfo.nY1)
        {
            vstRectInfo.emplace_back(stRectInfo);
        }
    }
}

EventStatistics_NS::Report_S CHVFPeopleFlowOutputExecutor::build_report(const AiPipeline_NS::StatisticsReportDraft_S &stDraft)
{
    EventStatistics_NS::Report_S stReport;
    stReport.enStatisticsType = EventStatistics_NS::StatisticsType_E::PEOPLE_FLOW;
    stReport.enEventType = Event::Type_E::PEOPLE_FLOW_STATISTICS;
    stReport.enAlarmEventType = Event::Type_E::PEOPLE_FLOW_STATISTICS;
    stReport.nChnId = stDraft.nChannelId;
    stReport.nRuleId = stDraft.nRuleId;
    stReport.llFrameTimestampMs = stDraft.llTimestampMs;
    stReport.nReportSeq = stDraft.nReportSeq;
    stReport.nEnterCount = stDraft.nEnterCount;
    stReport.nLeaveCount = stDraft.nLeaveCount;
    stReport.nTotalCount = stDraft.nTotalCount;
    stReport.mapExtras["force"] = stDraft.bForceReport ? "1" : "0";

    /* 目标快照：归一化框转回原生结果坐标，保持现有协议字段语义 */
    stReport.vecTargets.reserve(stDraft.vecTargets.size());
    for (const auto &stTarget : stDraft.vecTargets)
    {
        EventStatistics_NS::TargetSnapshot_S stSnapshot;
        stSnapshot.nTrackId = static_cast<int>(stTarget.ullTrackId);
        stSnapshot.nRuleId = stDraft.nRuleId;
        stSnapshot.enSnapshotType = (stTarget.nSnapshotType == static_cast<int>(EventStatistics_NS::SnapshotType_E::LEAVE))
                                        ? EventStatistics_NS::SnapshotType_E::LEAVE
                                        : EventStatistics_NS::SnapshotType_E::ENTER;
        stSnapshot.stRect = AiPipeline_NS::Geometry_NS::denormalize_rect(stTarget.stRect, m_stNativeResultSize);
        stSnapshot.llTimestampMs = stDraft.llTimestampMs;
        stSnapshot.nDirection = stTarget.nDirection;
        stReport.vecTargets.emplace_back(std::move(stSnapshot));
    }
    return stReport;
}

EventTriggerContext_S CHVFPeopleFlowOutputExecutor::build_event_context(const AiPipeline_NS::EventCondition_S &stCondition)
{
    /* 联动上下文，承载规则匹配所需的摘要属性 */
    EventTriggerContext_S stContext;
    stContext.enEventType = stCondition.enEventType;
    stContext.nChnId = stCondition.nChannelId;
    stContext.llTimestamp = stCondition.llWallTimestampMs;
    stContext.mapAttrs["rule_id"] = std::to_string(stCondition.nRuleId);
    stContext.mapAttrs["enter_count"] = std::to_string(stCondition.nEnterCount);
    stContext.mapAttrs["leave_count"] = std::to_string(stCondition.nLeaveCount);
    stContext.mapAttrs["total_count"] = std::to_string(stCondition.nTotalCount);
    stContext.mapAttrs["current_stay_count"] = std::to_string(stCondition.nCurrentStayCount);
    stContext.mapAttrs["alarm_level"] = alarm_level_text(stCondition.enEventType);
    return stContext;
}

void CHVFPeopleFlowOutputExecutor::fill_report_images(EventStatistics_NS::Report_S &stReport,
                                                      const AiPipeline_NS::ProcessorOutput_S &stOutput)
{
    if (m_pImageProvider == nullptr || !m_pReporter->shouldBuildHeavyPayload())
    {
        return;
    }

    /* 本报告关联的图片请求按顺序消费；人员密度请求由密度输出执行器消费 */
    std::vector<const AiPipeline_NS::ImageRequest_S *> vecRequests;
    for (const auto &stRequest : stOutput.vecImageRequests)
    {
        if (stRequest.nReportSeq == stReport.nReportSeq &&
            stRequest.nStatisticsType != static_cast<int>(EventStatistics_NS::StatisticsType_E::PEOPLE_DENSITY))
        {
            vecRequests.emplace_back(&stRequest);
        }
    }
    std::sort(vecRequests.begin(),
              vecRequests.end(),
              [](const AiPipeline_NS::ImageRequest_S *pLeft, const AiPipeline_NS::ImageRequest_S *pRight)
              {
                  return pLeft->nOrder < pRight->nOrder;
              });

    for (const auto *pRequest : vecRequests)
    {
        EventStatistics_NS::ImagePayload_S stImage;
        if (pRequest->bPanorama)
        {
            if (m_pImageProvider->build_panorama(stImage) == OK)
            {
                stReport.stPanoramaImage = std::move(stImage);
            }
            else
            {
                dlog_warn("人流统计全景图构建失败，报告[%u]继续提交", stReport.nReportSeq);
            }
        }
        else
        {
            if (m_pImageProvider->build_target(pRequest->stCropRect, stImage) == OK)
            {
                stReport.vecTargetImages.emplace_back(std::move(stImage));
            }
            else
            {
                dlog_warn("人流统计目标图构建失败，报告[%u]继续提交", stReport.nReportSeq);
            }
        }
    }
}

void CHVFPeopleFlowOutputExecutor::reset()
{
    m_normalAlarmStateMachine.reset();
    m_mediumAlarmStateMachine.reset();
    m_severeAlarmStateMachine.reset();
}
} // namespace HVFDetectInternal

#endif // CAP_AI_PEOPLE_STATISTICS
