/**
 * @FilePath     : event_statistics_tvsdk_reporter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-25 09:13:14
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 10:41:30
 * @Description  : 事件统计 TVSDK 上报适配器实现
 */

#include "event_statistics_tvsdk_reporter.h"

#include <memory>
#include <string>

#include "event_linkage_dict.h"
#include "dlog.h"

namespace
{
/**
 * @brief   : 将统计目标快照转换为 TVSDK 目标负载
 * @param    {EventStatistics_NS::TargetSnapshot_S} &stSnapshot：统计目标快照
 * @return   {EventTvSdkTarget_S} TVSDK 目标负载
 */
EventTvSdkTarget_S build_tvsdk_target(const EventStatistics_NS::TargetSnapshot_S &stSnapshot)
{
    /* TVSDK 目标负载只保留协议需要的基础字段，避免透传算法内部结构 */
    EventTvSdkTarget_S stTarget;
    stTarget.nTrackId = stSnapshot.nTrackId;
    stTarget.nRuleId = stSnapshot.nRuleId;
    stTarget.nSnapshotType = static_cast<int>(stSnapshot.enSnapshotType);
    stTarget.nLeft = stSnapshot.stRect.nX1;
    stTarget.nTop = stSnapshot.stRect.nY1;
    stTarget.nRight = stSnapshot.stRect.nX2;
    stTarget.nBottom = stSnapshot.stRect.nY2;
    stTarget.llTimestampMs = stSnapshot.llTimestampMs;
    stTarget.nDirection = stSnapshot.nDirection;
    stTarget.mapExtras = stSnapshot.mapExtras;
    return stTarget;
}

/**
 * @brief   : 将统计报告转换为 TVSDK 统计负载
 * @param    {EventStatistics_NS::Report_S} &stReport：统计报告
 * @return   {EventTvSdkPayload_S} TVSDK 统计负载
 */
EventTvSdkPayload_S build_tvsdk_payload(const EventStatistics_NS::Report_S &stReport)
{
    /* 统计类周期上报只传摘要和目标列表，全景图字段预留为空 */
    EventTvSdkPayload_S stPayload;
    stPayload.enType = EventTvSdkPayloadType_E::STATISTICS;
    stPayload.stStatistics.nRuleId = stReport.nRuleId;
    stPayload.stStatistics.llTimestampMs = stReport.llFrameTimestampMs;
    stPayload.stStatistics.nReportSeq = stReport.nReportSeq;
    stPayload.stStatistics.nEnterCount = stReport.nEnterCount;
    stPayload.stStatistics.nLeaveCount = stReport.nLeaveCount;
    stPayload.stStatistics.nTotalCount = stReport.nTotalCount;
    stPayload.stStatistics.nCurrentPeopleCount = stReport.nCurrentPeopleCount;
    stPayload.stStatistics.nAverageStayTimeSec = stReport.nAverageStayTimeSec;
    stPayload.stStatistics.stPanoramaImage.vecJpeg = stReport.stPanoramaImage.vecJpeg;
    stPayload.stStatistics.stPanoramaImage.nWidth = stReport.stPanoramaImage.nWidth;
    stPayload.stStatistics.stPanoramaImage.nHeight = stReport.stPanoramaImage.nHeight;
    stPayload.stStatistics.stPanoramaImage.strTag = stReport.stPanoramaImage.strTag;

    switch (stReport.enStatisticsType)
    {
    case EventStatistics_NS::StatisticsType_E::PEOPLE_FLOW:
        stPayload.stStatistics.nStatisticsType = static_cast<int>(EventTvSdkStatisticsType_E::PEOPLE_FLOW);
        break;
    case EventStatistics_NS::StatisticsType_E::PEOPLE_DENSITY:
        stPayload.stStatistics.nStatisticsType = static_cast<int>(EventTvSdkStatisticsType_E::PEOPLE_DENSITY);
        break;
    default:
        break;
    }

    for (const auto &stTarget : stReport.vecTargets)
    {
        stPayload.stStatistics.vecTargets.emplace_back(build_tvsdk_target(stTarget));
    }
    return stPayload;
}

/**
 * @brief   : 将统计类型转换为事件类型
 * @param    {StatisticsType_E} enStatisticsType：统计类型
 * @return   {Event::Type_E} 事件类型
 */
Event::Type_E get_event_type(EventStatistics_NS::StatisticsType_E enStatisticsType)
{
#if CAP_AI_PEOPLE_STATISTICS
    switch (enStatisticsType)
    {
    case EventStatistics_NS::StatisticsType_E::PEOPLE_FLOW:
        return Event::Type_E::PEOPLE_FLOW_STATISTICS;
    case EventStatistics_NS::StatisticsType_E::PEOPLE_DENSITY:
        return Event::Type_E::PEOPLE_DENSITY_DETECTION;
    default:
        break;
    }
#else
    (void)enStatisticsType;
#endif
    return Event::Type_E::UNKNOWN;
}
} // namespace

void CTvSdkEventStatisticsReporter::report(const EventStatistics_NS::Report_S &stReport)
{
    EventTriggerContext_S stContext;
    stContext.enEventType = get_event_type(stReport.enStatisticsType);
    if (stContext.enEventType == Event::Type_E::UNKNOWN)
    {
        dlog_warn("[统计推送诊断] reporter::get_event_type 返回 UNKNOWN, 统计类型[%d]",
                  static_cast<int>(stReport.enStatisticsType));
        return;
    }

    stContext.bEventEnded = false;
    stContext.nChnId = stReport.nChnId;
    stContext.llTimestamp = stReport.llFrameTimestampMs;
    stContext.mapAttrs = stReport.mapExtras;
    stContext.mapAttrs["rule_id"] = std::to_string(stReport.nRuleId);
    stContext.pTvSdkPayload = std::make_shared<EventTvSdkPayload_S>(build_tvsdk_payload(stReport));

    dlog_info("[统计推送诊断] reporter::report 构建上下文完成: 事件类型[%d] 通道[%d] 目标数[%zu] 全景图大小[%zu]",
              static_cast<int>(stContext.enEventType), stContext.nChnId,
              stReport.vecTargets.size(), stReport.stPanoramaImage.vecJpeg.size());

    EventLinkageDict::push_tvsdk_event_alarm(stContext);
}
