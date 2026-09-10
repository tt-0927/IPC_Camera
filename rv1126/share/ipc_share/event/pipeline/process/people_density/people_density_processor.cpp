/**
 * @FilePath     : people_density_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-26 15:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 共享人员密度事件处理器实现
 */

#include "people_density_processor.hpp"

#include <algorithm>

#include "IpcRet.h"
#include "dlog.h"

#if CAP_AI_PEOPLE_DENSITY_PIPELINE

namespace AiPipeline_NS
{
namespace PeopleDensity_NS
{
namespace
{
/* 目标类型位掩码：不限制（0）。分发器在批次无可接受类型目标时整体跳过处理器，
 * 而密度报警必须在人数清零帧持续收到 false 条件才能结束事件，
 * 故由处理器内部按 HEAD 类型过滤，保证空帧也参与状态机驱动 */
constexpr uint32_t PEOPLE_DENSITY_OBJECT_MASK = 0U;
/* 处理器必需能力：稳定 Track ID（目标快照携带 ID，ENDED 无状态可消费） */
constexpr uint32_t PEOPLE_DENSITY_REQUIRED_CAPABILITIES = static_cast<uint32_t>(DetectionCapability_E::TRACK_ID);
/* 统计类型数值：与 EventStatistics_NS::StatisticsType_E::PEOPLE_DENSITY 对齐 */
constexpr int PEOPLE_DENSITY_STATISTICS_TYPE = 2;
/* 目标快照类型数值：与 EventStatistics_NS::SnapshotType_E::REGION_CURRENT 对齐 */
constexpr int PEOPLE_DENSITY_SNAPSHOT_REGION_CURRENT = 2;

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
} // namespace

int CPeopleDensityProcessor::apply_config(const PeopleDensityConfig_S &stConfig)
{
    const bool bWasEnabled = m_bConfigValid && m_stConfig.bEnabled;
    const bool bNowEnabled = stConfig.bEnabled;

    m_stConfig = stConfig;
    m_bConfigValid = true;

    /* 禁用时清空运行态，避免旧累计量跨新配置上报 */
    if (bWasEnabled && !bNowEnabled)
    {
        reset();
        /* reset() 会清空配置，恢复本次应用的禁用配置 */
        m_stConfig = stConfig;
        m_bConfigValid = true;
    }
    return OK;
}

void CPeopleDensityProcessor::reset()
{
    m_stConfig = PeopleDensityConfig_S();
    m_bConfigValid = false;
    m_llLastReportMs = 0;
    m_nReportSeq = 0;
    m_llStaySampleLastMs = 0;
    m_llStayOccupancyMs = 0;
    m_nStayLastPeopleCount = 0;
    m_nStayMaxPeopleCount = 0;
}

bool CPeopleDensityProcessor::is_enabled() const
{
    return m_bConfigValid && m_stConfig.bEnabled;
}

const ProcessorDescriptor_S &CPeopleDensityProcessor::descriptor() const
{
    static const ProcessorDescriptor_S stDescriptor = {
        "CPeopleDensityProcessor",
        PEOPLE_DENSITY_OBJECT_MASK,
        PEOPLE_DENSITY_REQUIRED_CAPABILITIES,
        1,
    };
    return stDescriptor;
}

int CPeopleDensityProcessor::process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput)
{
    if (!m_bConfigValid || !m_stConfig.bEnabled)
    {
        return OK;
    }

    const int64_t llWallMs = stBatch.stMetadata.llWallTimestampMs;
    /* 灵敏度阈值公式与旧实现一致：灵敏度越高阈值越低 */
    const float fConfidenceThreshold = 1.0f - static_cast<float>(m_stConfig.nSensitivity) / 100.0f;

    /* 诊断计数 */
    uint32_t nFrameHeads = 0;     /* 通过 HEAD 类型过滤 */
    uint32_t nFrameActive = 0;    /* 有有效 Track ID 且非 ENDED */
    uint32_t nFramePassConf = 0;  /* 置信度通过 */
    uint32_t nFrameInRegion = 0;  /* 在检测区域内 */
    /* 当前区域内目标快照，用于周期上报 */
    std::vector<ReportTargetDraft_S> vecTargets;

    for (const auto &stObject : stBatch.vecObjects)
    {
        /* 人员密度只处理人头目标 */
        if (stObject.enType != ObjectType_E::HEAD)
        {
            continue;
        }
        ++nFrameHeads;

        /* ENDED 目标不参与瞬时计数；密度无轨迹存储，无需清理信号 */
        if (stObject.enTrackState == TrackState_E::ENDED)
        {
            continue;
        }

        /* 无稳定 Track ID 的目标不参与统计 */
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

        /* 置信度阈值过滤 */
        if (stObject.fConfidence < fConfidenceThreshold)
        {
            continue;
        }
        ++nFramePassConf;

        /* 区域过滤：使用目标框中心点，与旧实现一致 */
        const Geometry_NS::Point_S stCenter = rect_center(stObject.stRect);
        if (!m_stConfig.bRegionValid || !Geometry_NS::point_in_polygon(stCenter, m_stConfig.vecRegion))
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

        /* 区域内目标快照：Track ID + 归一化框，上报时由输出适配层反归一化 */
        ReportTargetDraft_S stTarget;
        stTarget.ullTrackId = stObject.optTrackId.value_or(0);
        stTarget.nSnapshotType = PEOPLE_DENSITY_SNAPSHOT_REGION_CURRENT;
        stTarget.nDirection = 0;
        stTarget.stRect = stObject.stRect;
        vecTargets.emplace_back(std::move(stTarget));
    }

    /* 当前检测区域内的人头目标数量即密度统计值 */
    const uint32_t nPeopleCount = static_cast<uint32_t>(vecTargets.size());
    update_stay_accumulator(llWallMs, nPeopleCount);

    if (nFrameHeads > 0)
    {
        dlog_info("people_density 帧诊断: head[%u] active[%u] 置信度[%u] 区域内[%u]",
                  nFrameHeads, nFrameActive, nFramePassConf, nFrameInRegion);
    }

    /* 三级密度报警：仅最高命中等级条件为 true，与旧状态机驱动语义一致 */
    const Event::Type_E enAlarmEventType = calculate_alarm_event_type(nPeopleCount);
    const int64_t llMonoMs = stBatch.stMetadata.llMonotonicTimestampMs;
    const EventCondition_S astConditions[] = {
        { Event::Type_E::PEOPLE_DENSITY_NORMAL,
         enAlarmEventType == Event::Type_E::PEOPLE_DENSITY_NORMAL,
         stBatch.stMetadata.nChannelId,
         static_cast<int>(PEOPLE_DENSITY_DEFAULT_RULE_ID),
         llWallMs, llMonoMs,
         0, 0, 0, 0,
         nPeopleCount },
        { Event::Type_E::PEOPLE_DENSITY_MEDIUM,
         enAlarmEventType == Event::Type_E::PEOPLE_DENSITY_MEDIUM,
         stBatch.stMetadata.nChannelId,
         static_cast<int>(PEOPLE_DENSITY_DEFAULT_RULE_ID),
         llWallMs, llMonoMs,
         0, 0, 0, 0,
         nPeopleCount },
        { Event::Type_E::PEOPLE_DENSITY_SEVERE,
         enAlarmEventType == Event::Type_E::PEOPLE_DENSITY_SEVERE,
         stBatch.stMetadata.nChannelId,
         static_cast<int>(PEOPLE_DENSITY_DEFAULT_RULE_ID),
         llWallMs, llMonoMs,
         0, 0, 0, 0,
         nPeopleCount },
    };
    for (const auto &stCondition : astConditions)
    {
        stOutput.vecEventConditions.emplace_back(stCondition);
    }

    /* 周期上报：生成草稿即推进报告序号并重置区间停留统计 */
    if (should_emit_periodic_report(llWallMs))
    {
        StatisticsReportDraft_S stDraft;
        stDraft.nChannelId = stBatch.stMetadata.nChannelId;
        stDraft.nRuleId = static_cast<int>(PEOPLE_DENSITY_DEFAULT_RULE_ID);
        stDraft.llTimestampMs = llWallMs;
        stDraft.nReportSeq = ++m_nReportSeq;
        stDraft.nStatisticsType = PEOPLE_DENSITY_STATISTICS_TYPE;
        stDraft.nCurrentPeopleCount = nPeopleCount;
        stDraft.nAverageStayTimeSec = average_stay_time_sec();
        stDraft.enAlarmEventType = enAlarmEventType;
        stDraft.vecTargets = std::move(vecTargets);
        stDraft.bNeedPanorama = true;
        stOutput.vecStatisticsDrafts.emplace_back(std::move(stDraft));

        /* 全景图请求，与报告序号及统计类型关联 */
        ImageRequest_S stPanoramaRequest;
        stPanoramaRequest.nReportSeq = m_nReportSeq;
        stPanoramaRequest.nStatisticsType = PEOPLE_DENSITY_STATISTICS_TYPE;
        stPanoramaRequest.bPanorama = true;
        stPanoramaRequest.nOrder = 0;
        stOutput.vecImageRequests.emplace_back(stPanoramaRequest);

        dlog_info("people_density 周期上报: 通道[%d] 序号[%u] 当前人数[%u] 平均停留[%u]s",
                  stBatch.stMetadata.nChannelId, m_nReportSeq, nPeopleCount,
                  stOutput.vecStatisticsDrafts.back().nAverageStayTimeSec);

        reset_stay_accumulator(nPeopleCount);
    }

    return OK;
}

Event::Type_E CPeopleDensityProcessor::calculate_alarm_event_type(uint32_t nPeopleCount) const
{
    /* 优先级 severe > medium > normal，触发条件为人数 >= 阈值且该等级使能 */
    if (m_stConfig.stAlarmSevere.bEnable && nPeopleCount >= m_stConfig.stAlarmSevere.nThreshold)
    {
        return Event::Type_E::PEOPLE_DENSITY_SEVERE;
    }

    if (m_stConfig.stAlarmMedium.bEnable && nPeopleCount >= m_stConfig.stAlarmMedium.nThreshold)
    {
        return Event::Type_E::PEOPLE_DENSITY_MEDIUM;
    }

    if (m_stConfig.stAlarmNormal.bEnable && nPeopleCount >= m_stConfig.stAlarmNormal.nThreshold)
    {
        return Event::Type_E::PEOPLE_DENSITY_NORMAL;
    }

    /* 未命中等级时返回主类型作为无等级标识 */
    return Event::Type_E::PEOPLE_DENSITY_DETECTION;
}

bool CPeopleDensityProcessor::should_emit_periodic_report(int64_t llNowMs)
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

    m_llLastReportMs = llNowMs;
    return true;
}

void CPeopleDensityProcessor::update_stay_accumulator(int64_t llNowMs, uint32_t nPeopleCount)
{
    if (m_llStaySampleLastMs <= 0)
    {
        m_llStaySampleLastMs = llNowMs;
        m_nStayLastPeopleCount = nPeopleCount;
        m_nStayMaxPeopleCount = nPeopleCount;
        return;
    }

    if (llNowMs < m_llStaySampleLastMs)
    {
        dlog_warn("人员密度停留时间采样时间回退，重置区间统计 last[%lld] now[%lld]",
                  static_cast<long long>(m_llStaySampleLastMs),
                  static_cast<long long>(llNowMs));
        reset_stay_accumulator(nPeopleCount);
        m_llStaySampleLastMs = llNowMs;
        return;
    }

    /*
     * 区间占用近似算法不做单人轨迹停留统计，用相邻两帧之间的人数近似该时间片的人数状态：
     * 1. 设第 i 次采样时间为 t_i，区域人数为 c_i。
     * 2. 相邻采样区间 [t_i, t_{i+1}) 的人时贡献为 c_i * (t_{i+1} - t_i)，单位 人*毫秒。
     * 3. 当前上报周期累计人时 occupancy_ms = Σ(c_i * Δt_i)。
     * 4. 人数规模取区间内 max(c_i)。
     * 5. 平均停留时间 = occupancy_ms / max_people_count / 1000。
     */
    const int64_t llDeltaMs = llNowMs - m_llStaySampleLastMs;
    m_llStayOccupancyMs += static_cast<int64_t>(m_nStayLastPeopleCount) * llDeltaMs;
    m_nStayMaxPeopleCount = std::max(m_nStayMaxPeopleCount, nPeopleCount);
    m_llStaySampleLastMs = llNowMs;
    m_nStayLastPeopleCount = nPeopleCount;
}

uint32_t CPeopleDensityProcessor::average_stay_time_sec() const
{
    if (m_nStayMaxPeopleCount == 0 || m_llStayOccupancyMs <= 0)
    {
        return 0;
    }

    /* 平均停留秒数 = 累计人时毫秒 / 周期最大人数 / 1000 */
    const int64_t llAverageStayMs = m_llStayOccupancyMs / static_cast<int64_t>(m_nStayMaxPeopleCount);
    const int64_t llAverageStaySec = llAverageStayMs / 1000;
    return llAverageStaySec > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(llAverageStaySec);
}

void CPeopleDensityProcessor::reset_stay_accumulator(uint32_t nCurrentPeopleCount)
{
    m_llStayOccupancyMs = 0;
    m_nStayLastPeopleCount = nCurrentPeopleCount;
    m_nStayMaxPeopleCount = nCurrentPeopleCount;
}
} // namespace PeopleDensity_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_DENSITY_PIPELINE
