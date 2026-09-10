/**
 * @FilePath     : people_flow_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 11:40:00
 * @Description  : 共享人流统计事件处理器实现
 */

#include "people_flow_processor.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_AI_PEOPLE_STATISTICS

namespace AiPipeline_NS
{
namespace PeopleFlow_NS
{
namespace
{
/* 目标类型位掩码：PERSON */
constexpr uint32_t PEOPLE_FLOW_OBJECT_MASK = 1U << static_cast<uint32_t>(ObjectType_E::PERSON);
/* 处理器必需能力：稳定 Track ID 与生命周期 */
constexpr uint32_t PEOPLE_FLOW_REQUIRED_CAPABILITIES = static_cast<uint32_t>(DetectionCapability_E::TRACK_ID) |
                                                       static_cast<uint32_t>(DetectionCapability_E::TRACK_LIFECYCLE);

/**
 * @brief   : 获取目标框中心点，与旧区域过滤语义一致
 * @param    {const NormalizedRect_S &} stRect：归一化目标框
 * @return   {Geometry_NS::Point_S} 中心点
 */
Geometry_NS::Point_S rect_center(const NormalizedRect_S &stRect)
{
    Geometry_NS::Point_S stCenter;
    stCenter.dX = (stRect.dLeft + stRect.dRight) / 2.0;
    stCenter.dY = (stRect.dTop + stRect.dBottom) / 2.0;
    return stCenter;
}

/**
 * @brief   : 获取目标框底边中点，与旧跨线轨迹语义一致
 * @param    {const NormalizedRect_S &} stRect：归一化目标框
 * @return   {Geometry_NS::Point_S} 底边中点
 */
Geometry_NS::Point_S rect_bottom_center(const NormalizedRect_S &stRect)
{
    Geometry_NS::Point_S stPoint;
    stPoint.dX = (stRect.dLeft + stRect.dRight) / 2.0;
    stPoint.dY = stRect.dBottom;
    return stPoint;
}

/**
 * @brief   : 获取穿越方向的反方向
 * @param    {Alarm::CrossDirection_E} enDirection：穿越方向
 * @return   {Alarm::CrossDirection_E} 反方向
 */
Alarm::CrossDirection_E reverse_direction(Alarm::CrossDirection_E enDirection)
{
    if (enDirection == Alarm::CrossDirection_E::A_TO_B)
    {
        return Alarm::CrossDirection_E::B_TO_A;
    }
    if (enDirection == Alarm::CrossDirection_E::B_TO_A)
    {
        return Alarm::CrossDirection_E::A_TO_B;
    }
    return Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;
}
} // namespace

CPeopleFlowProcessor::CPeopleFlowProcessor() = default;

int CPeopleFlowProcessor::apply_config(const PeopleFlowConfig_S &stConfig)
{
    /* 规则几何或方向是否发生变化，变化时必须清空轨迹，避免旧轨迹跨新线 */
    bool bGeometryChanged = false;
    const bool bWasEnabled = m_bConfigValid && m_stConfig.bEnabled;
    const bool bNowEnabled = stConfig.bEnabled;

    if (m_bConfigValid)
    {
        const PeopleFlowRuleConfig_S &stOldRule = m_stConfig.stRule;
        const PeopleFlowRuleConfig_S &stNewRule = stConfig.stRule;
        if (stOldRule.bValid != stNewRule.bValid || stOldRule.enEnterDirection != stNewRule.enEnterDirection ||
            stOldRule.stLineStart.dX != stNewRule.stLineStart.dX || stOldRule.stLineStart.dY != stNewRule.stLineStart.dY ||
            stOldRule.stLineEnd.dX != stNewRule.stLineEnd.dX || stOldRule.stLineEnd.dY != stNewRule.stLineEnd.dY ||
            stOldRule.vecRegion != stNewRule.vecRegion)
        {
            bGeometryChanged = true;
        }
    }

    m_stConfig = stConfig;
    m_bConfigValid = true;
    m_stateStore.set_config(m_stConfig);

    /* 禁用或规则变化时清空运行态，确保新配置从干净状态开始 */
    if (!bNowEnabled || (bWasEnabled && bGeometryChanged))
    {
        m_trackStore.reset();
        m_stateStore.clear();
        m_llLastReportMs = 0;
    }
    return OK;
}

void CPeopleFlowProcessor::clear_statistics()
{
    m_trackStore.reset();
    m_stateStore.clear();
    m_llLastReportMs = 0;
}

void CPeopleFlowProcessor::reset()
{
    m_stConfig = PeopleFlowConfig_S();
    m_bConfigValid = false;
    m_trackStore.reset();
    m_stateStore.clear();
    m_llLastReportMs = 0;
    m_stLastSummary = PeopleFlowFrameSummary_S();
}

bool CPeopleFlowProcessor::is_enabled() const
{
    return m_bConfigValid && m_stConfig.bEnabled;
}

const PeopleFlowFrameSummary_S &CPeopleFlowProcessor::last_frame_summary() const
{
    return m_stLastSummary;
}

const ProcessorDescriptor_S &CPeopleFlowProcessor::descriptor() const
{
    static const ProcessorDescriptor_S stDescriptor = {
        "CPeopleFlowProcessor",
        PEOPLE_FLOW_OBJECT_MASK,
        PEOPLE_FLOW_REQUIRED_CAPABILITIES,
        0,
    };
    return stDescriptor;
}

int CPeopleFlowProcessor::process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput)
{
    if (!m_bConfigValid || !m_stConfig.bEnabled)
    {
        return OK;
    }

    /* 定时清零使用 wall clock，保持现有周期语义 */
    m_stateStore.maybe_timed_reset(stBatch.stMetadata.llWallTimestampMs);

    /* 当前帧进入/离开计数，仅用于帧摘要 */
    uint32_t nFrameEnterCount = 0;
    uint32_t nFrameLeaveCount = 0;
    /* 诊断计数 */
    uint32_t nFramePersons = 0;       /* 通过 PERSON 类型过滤 */
    uint32_t nFrameActive = 0;        /* 有有效 Track ID 且非 ENDED */
    uint32_t nFrameValidGeom = 0;     /* 几何有效 */
    uint32_t nFramePassConf = 0;      /* 置信度通过 */
    uint32_t nFrameInRegion = 0;      /* 在检测区域内 */
    uint32_t nFrameHasHistory = 0;    /* 有历史轨迹可做跨线判断 */
    /* 当前帧是否发生跨线，用于触发强制上报并携带图片 */
    bool bForceReport = false;
    /* 强制上报时涉及的归一化目标框，仅填充发生跨线的目标 */
    std::vector<ReportTargetDraft_S> vecReportTargets;

    for (const auto &stObject : stBatch.vecObjects)
    {
        /* 人流统计只处理行人目标 */
        if (stObject.enType != ObjectType_E::PERSON)
        {
            continue;
        }
        ++nFramePersons;

        const TrackKey_S stKey{ stBatch.stMetadata.nChannelId,
                                static_cast<int>(PEOPLE_FLOW_DEFAULT_RULE_ID),
                                stObject.optTrackId.value_or(0) };

        /* ENDED 目标立即清理轨迹状态，不参与几何计算 */
        if (stObject.enTrackState == TrackState_E::ENDED)
        {
            if (stObject.optTrackId.has_value())
            {
                m_trackStore.erase(stKey);
            }
            continue;
        }

        /* 无稳定 Track ID 的目标不参与轨迹统计 */
        if (!stObject.optTrackId.has_value() || stObject.enTrackState == TrackState_E::UNAVAILABLE)
        {
            continue;
        }
        ++nFrameActive;

        /* 目标必须携带有效几何，避免退化框参与计算 */
        if (!stObject.bHasGeometry || !Geometry_NS::is_valid_normalized_rect(stObject.stRect))
        {
            continue;
        }
        ++nFrameValidGeom;

        /* 置信度阈值与旧灵敏度公式保持一致 */
        const float fConfidenceThreshold = 1.0f - static_cast<float>(m_stConfig.nSensitivity) / 100.0f;
        if (stObject.fConfidence < fConfidenceThreshold)
        {
            continue;
        }
        ++nFramePassConf;

        /* 区域过滤：使用目标框中心点 */
        const Geometry_NS::Point_S stCenter = rect_center(stObject.stRect);
        if (!m_stConfig.stRule.bValid || !Geometry_NS::point_in_polygon(stCenter, m_stConfig.stRule.vecRegion))
        {
            continue;
        }
        ++nFrameInRegion;

        /* OSD 叠加草稿：通过过滤的目标进入叠加输出 */
        OverlayItem_S stOverlayItem;
        stOverlayItem.enType = stObject.enType;
        stOverlayItem.stRect = stObject.stRect;
        stOverlayItem.optTrackId = stObject.optTrackId;
        stOutput.vecOverlayItems.emplace_back(std::move(stOverlayItem));

        /* 轨迹点：目标框底边中点 */
        const Geometry_NS::Point_S stCurrentPos = rect_bottom_center(stObject.stRect);
        const int nTrackRet = m_trackStore.update(stKey, stCurrentPos, stBatch.stMetadata.llMonotonicTimestampMs);
        if (nTrackRet != OK)
        {
            /* 容量满载且超时清理后仍无空间，限频告警并跳过该目标 */
            if ((stBatch.stMetadata.llMonotonicTimestampMs - m_llLastFullWarnMs) > 10000)
            {
                m_llLastFullWarnMs = stBatch.stMetadata.llMonotonicTimestampMs;
                dlog_warn("人流统计轨迹容量满载，通道[%d] 目标 track_id[%llu] 跳过",
                          stBatch.stMetadata.nChannelId,
                          static_cast<unsigned long long>(stKey.ullTrackId));
            }
            continue;
        }

        TrackState_S stTrackState;
        if (!m_trackStore.get(stKey, stTrackState) || !stTrackState.bHasLast)
        {
            /* 首帧只初始化轨迹，不计数 */
            continue;
        }
        ++nFrameHasHistory;

        /* 跨线方向：轨迹段相对规则线 */
        const Alarm::CrossDirection_E enCrossResult = Geometry_NS::detect_cross_direction(stTrackState.stLastPosition,
                                                                                          stTrackState.stCurrentPosition,
                                                                                          m_stConfig.stRule.stLineStart,
                                                                                          m_stConfig.stRule.stLineEnd);
        if (enCrossResult == Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID || enCrossResult == Alarm::CrossDirection_E::BOTH_WAYS)
        {
            continue;
        }

        /* 配置方向作为进入方向，反方向作为离开方向 */
        const Alarm::CrossDirection_E enEnterDirection = m_stConfig.stRule.enEnterDirection;
        const Alarm::CrossDirection_E enLeaveDirection = reverse_direction(enEnterDirection);

        ReportTargetDraft_S stTarget;
        stTarget.ullTrackId = stObject.optTrackId.value_or(0);
        stTarget.nDirection = static_cast<int>(enCrossResult);
        stTarget.stRect = stObject.stRect;

        if (enCrossResult == enEnterDirection)
        {
            m_stateStore.on_enter(stTarget);
            ++nFrameEnterCount;
            bForceReport = true;
            vecReportTargets.emplace_back(stTarget);
            dlog_info("people_flow 越线进入: 通道[%d] track_id[%llu] 方向[%d] 累计进入[%u] 累计离开[%u]",
                      stBatch.stMetadata.nChannelId,
                      static_cast<unsigned long long>(stTarget.ullTrackId),
                      static_cast<int>(enCrossResult),
                      m_stateStore.enter_count(),
                      m_stateStore.leave_count());
        }
        else if (enCrossResult == enLeaveDirection)
        {
            m_stateStore.on_leave(stTarget);
            ++nFrameLeaveCount;
            bForceReport = true;
            vecReportTargets.emplace_back(stTarget);
            dlog_info("people_flow 越线离开: 通道[%d] track_id[%llu] 方向[%d] 累计进入[%u] 累计离开[%u]",
                      stBatch.stMetadata.nChannelId,
                      static_cast<unsigned long long>(stTarget.ullTrackId),
                      static_cast<int>(enCrossResult),
                      m_stateStore.enter_count(),
                      m_stateStore.leave_count());
        }
    }

    /* 帧级诊断：展示 PERSON 目标在各过滤阶段的存活数，快速定位在哪一步被拦截 */
    if (nFramePersons > 0)
    {
        dlog_info("people_flow 帧诊断: person[%u] active[%u] 几何[%u] 置信度[%u] 区域内[%u] 有轨迹[%u] 进入[%u] 离开[%u]",
                  nFramePersons, nFrameActive, nFrameValidGeom, nFramePassConf,
                  nFrameInRegion, nFrameHasHistory, nFrameEnterCount, nFrameLeaveCount);
    }

    /* 清理超过 5 秒未更新的轨迹状态，TTL 使用 monotonic clock */
    m_trackStore.cleanup_expired(stBatch.stMetadata.llMonotonicTimestampMs);

    /* 滞留三级报警：按严重度从高到低输出事件条件 */
    const Event::Type_E enStayEventType = m_stateStore.get_stay_alarm_event_type();
    const int64_t llWallMs = stBatch.stMetadata.llWallTimestampMs;
    const int64_t llMonoMs = stBatch.stMetadata.llMonotonicTimestampMs;
    const EventCondition_S astConditions[] = {
        { Event::Type_E::PEOPLE_FLOW_STAY_NORMAL,
         enStayEventType == Event::Type_E::PEOPLE_FLOW_STAY_NORMAL,
         stBatch.stMetadata.nChannelId,
         static_cast<int>(PEOPLE_FLOW_DEFAULT_RULE_ID),
         llWallMs, llMonoMs,
         m_stateStore.enter_count(),
         m_stateStore.leave_count(),
         m_stateStore.total_count(),
         m_stateStore.current_stay_count() },
        { Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM,
         enStayEventType == Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM,
         stBatch.stMetadata.nChannelId,
         static_cast<int>(PEOPLE_FLOW_DEFAULT_RULE_ID),
         llWallMs, llMonoMs,
         m_stateStore.enter_count(),
         m_stateStore.leave_count(),
         m_stateStore.total_count(),
         m_stateStore.current_stay_count() },
        { Event::Type_E::PEOPLE_FLOW_STAY_SEVERE,
         enStayEventType == Event::Type_E::PEOPLE_FLOW_STAY_SEVERE,
         stBatch.stMetadata.nChannelId,
         static_cast<int>(PEOPLE_FLOW_DEFAULT_RULE_ID),
         llWallMs, llMonoMs,
         m_stateStore.enter_count(),
         m_stateStore.leave_count(),
         m_stateStore.total_count(),
         m_stateStore.current_stay_count() },
    };
    for (const auto &stCondition : astConditions)
    {
        stOutput.vecEventConditions.emplace_back(stCondition);
    }

    /* 报告：跨线强制上报或周期上报；生成草稿即推进内部报告状态 */
    if (bForceReport || should_emit_periodic_report(llWallMs))
    {
        StatisticsReportDraft_S stDraft = m_stateStore.build_and_consume_report(stBatch.stMetadata.nChannelId, llWallMs, bForceReport);
        /* 与旧行为一致：只有跨线强制上报才携带全景图与目标图 */
        stDraft.bNeedPanorama = bForceReport;
        stOutput.vecStatisticsDrafts.emplace_back(std::move(stDraft));

        if (bForceReport)
        {
            /* 全景图请求 */
            ImageRequest_S stPanoramaRequest;
            stPanoramaRequest.nReportSeq = m_stateStore.report_seq();
            stPanoramaRequest.bPanorama = true;
            stPanoramaRequest.nOrder = 0;
            stOutput.vecImageRequests.emplace_back(stPanoramaRequest);

            /* 目标图请求，最多 4 张 */
            uint32_t nImageCount = 0;
            for (const auto &stTarget : vecReportTargets)
            {
                if (nImageCount >= PEOPLE_FLOW_MAX_TARGET_IMAGES_PER_REPORT)
                {
                    break;
                }
                ImageRequest_S stTargetRequest;
                stTargetRequest.nReportSeq = m_stateStore.report_seq();
                stTargetRequest.bPanorama = false;
                stTargetRequest.stCropRect = stTarget.stRect;
                stTargetRequest.nOrder = static_cast<int>(nImageCount + 1);
                stOutput.vecImageRequests.emplace_back(stTargetRequest);
                ++nImageCount;
            }
        }

        /* 强制上报后重置周期计时器，避免短时间内重复上报 */
        m_llLastReportMs = llWallMs;
    }

    /* 更新帧摘要，供帧级诊断与外部消费 */
    m_stLastSummary.nChannelId = stBatch.stMetadata.nChannelId;
    m_stLastSummary.nRuleId = static_cast<int>(PEOPLE_FLOW_DEFAULT_RULE_ID);
    m_stLastSummary.ullFrameId = stBatch.stMetadata.ullFrameId;
    m_stLastSummary.nFrameEnterCount = nFrameEnterCount;
    m_stLastSummary.nFrameLeaveCount = nFrameLeaveCount;
    m_stLastSummary.nTotalEnterCount = m_stateStore.enter_count();
    m_stLastSummary.nTotalLeaveCount = m_stateStore.leave_count();
    m_stLastSummary.nCurrentStayCount = m_stateStore.current_stay_count();

    return OK;
}

bool CPeopleFlowProcessor::should_emit_periodic_report(int64_t llNowMs)
{
    if (m_llLastReportMs <= 0)
    {
        m_llLastReportMs = llNowMs;
        return false;
    }

    const int64_t llIntervalMs = static_cast<int64_t>(m_stConfig.nReportIntervalSec) * 1000;
    if ((llNowMs - m_llLastReportMs) < llIntervalMs)
    {
        return false;
    }

    /* 不在这里更新时间戳，由调用方统一处理，支持强制上报后的计时器重置 */
    return true;
}
} // namespace PeopleFlow_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_STATISTICS
