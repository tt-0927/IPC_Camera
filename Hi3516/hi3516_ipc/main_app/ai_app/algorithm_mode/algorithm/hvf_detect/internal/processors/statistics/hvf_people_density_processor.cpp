/**
 * @FilePath     : hvf_people_density_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-26 15:01:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-26 15:19:43
 * @Description  : HVF 人员密度处理器实现
 */

#include "hvf_people_density_processor.hpp"

#if CAP_AI_PEOPLE_DENSITY_V2
#include <algorithm>
#include <string>
#include <utility>

#include "IpcRet.h"
#include "dlog.h"
#include "video_frame_jpeg_encoder.hpp"

namespace
{
/**
 * @brief   : 转换人员密度区域配置并根据有效区域更新使能
 * @param    {PeopleDensityDetection_S} &stAlgoCfg：人员密度配置
 * @param    {int} nWidth：算法分辨率宽度
 * @param    {int} nHeight：算法分辨率高度
 * @return   {void}
 */
void convert_density_config_and_enable(Alarm::PeopleDensityDetection_S &stAlgoCfg, int nWidth, int nHeight)
{
    /* 人员密度检测区域转换到 HVF 模型输入分辨率 */
    stAlgoCfg.stDetectRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nWidth, nHeight);
    if (!stAlgoCfg.stDetectRegion.IsValid())
    {
        stAlgoCfg.bEnable = false;
        dlog_warn("人员密度V2配置无有效区域，关闭人员密度处理器");
    }
}

/**
 * @brief   : 将人员密度事件转换为等级文本
 * @param    {Event::Type_E} enEventType：人员密度等级事件类型
 * @return   {std::string} 等级文本，未命中等级时返回 none
 */
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

/**
 * @brief   : 构造人员密度联动上下文
 * @param    {SHVFProcessContext} &stContext：单帧处理上下文
 * @param    {Event::Type_E} enEventType：联动事件类型
 * @param    {uint32_t} nPeopleCount：当前区域人数
 * @param    {Event::Type_E} enAlarmEventType：当前命中的人员密度等级事件
 * @return   {EventTriggerContext_S} 联动上下文
 */
EventTriggerContext_S build_density_context(const HVFDetectInternal::SHVFProcessContext &stContext,
                                            Event::Type_E enEventType,
                                            uint32_t nPeopleCount,
                                            Event::Type_E enAlarmEventType)
{
    /* 人员密度联动上下文仅承载等级报警需要的规则匹配摘要，不承担周期统计数据上报 */
    EventTriggerContext_S stLinkageContext;
    stLinkageContext.enEventType = enEventType;
    stLinkageContext.nChnId = stContext.nChnId;
    stLinkageContext.llTimestamp = stContext.llTimestamp;
    stLinkageContext.mapAttrs["rule_id"] = "0";
    stLinkageContext.mapAttrs["current_people_count"] = std::to_string(nPeopleCount);
    stLinkageContext.mapAttrs["alarm_level"] = get_density_level_text(enAlarmEventType);
    return stLinkageContext;
}

/**
 * @brief   : 构造 HVF 人体目标矩形
 * @param    {ot_aidetect_object} &stObject：HVF 人体目标
 * @return   {Common::RectInfo_S} 目标矩形
 */
Common::RectInfo_S build_density_rect(const ot_aidetect_object &stObject)
{
    Common::RectInfo_S stRect;
    stRect.nX1 = stObject.detect_rect.x;
    stRect.nY1 = stObject.detect_rect.y;
    stRect.nX2 = stObject.detect_rect.x + stObject.detect_rect.width;
    stRect.nY2 = stObject.detect_rect.y + stObject.detect_rect.height;
    return stRect;
}

/**
 * @brief   : 构造人员密度目标快照
 * @param    {ot_aidetect_object} &stObject：HVF 人体目标
 * @param    {long long} llNowMs：当前时间戳
 * @return   {TargetSnapshot_S} 目标快照
 */
EventStatistics_NS::TargetSnapshot_S build_density_snapshot(const ot_aidetect_object &stObject, long long llNowMs)
{
    /* 人员密度当前区域目标快照，记录人体框和统计时间 */
    EventStatistics_NS::TargetSnapshot_S stSnapshot;
    stSnapshot.nTrackId = static_cast<int>(stObject.track_id);
    stSnapshot.nRuleId = 0;
    stSnapshot.enSnapshotType = EventStatistics_NS::SnapshotType_E::REGION_CURRENT;
    stSnapshot.stRect = build_density_rect(stObject);
    stSnapshot.llTimestampMs = llNowMs;
    return stSnapshot;
}
} // namespace

namespace HVFDetectInternal
{
void CHVFPeopleDensityProcessor::setEnabled(bool bEnable)
{
    m_stAlgoCfg.bEnable = bEnable;
    if (!bEnable)
    {
        m_normalAlarmStateMachine.reset();
        m_mediumAlarmStateMachine.reset();
        m_severeAlarmStateMachine.reset();
        m_llLastStatisticsReportTs = 0;
        m_llStaySampleLastTs = 0;
        m_llStayOccupancyMs = 0;
        m_nStayLastPeopleCount = 0;
        m_nStayMaxPeopleCount = 0;
        m_nReportSeq = 0;
    }
}

void CHVFPeopleDensityProcessor::setAlgoParamCfg(const Alarm::PeopleDensityDetection_S &stAlgoCfg,
                                                 int nWidth,
                                                 int nHeight)
{
    dlog_debug("ai_app: 设置人员密度V2侦测参数");
    m_stAlgoCfg = stAlgoCfg;
    convert_density_config_and_enable(m_stAlgoCfg, nWidth, nHeight);
    if (m_stAlgoCfg.nReportInterval > 0)
    {
        m_llStatisticsReportIntervalMs = static_cast<long long>(m_stAlgoCfg.nReportInterval) * 1000;
    }
}

void CHVFPeopleDensityProcessor::setReporter(
    const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
    m_pReporter = pReporter;
}

void CHVFPeopleDensityProcessor::process(SHVFProcessContext &stContext)
{
    if (!m_stAlgoCfg.bEnable)
    {
        return;
    }

    /* 当前帧处理时间戳，优先复用主控传入时间，缺省时退回当前系统时间 */
    const long long llNowMs = stContext.llTimestamp > 0 ? stContext.llTimestamp : static_cast<long long>(get_time_ms());
    if (stContext.llTimestamp <= 0)
    {
        stContext.llTimestamp = llNowMs;
    }

    /* 当前区域内目标快照，用于后续业务侧上报 */
    std::vector<EventStatistics_NS::TargetSnapshot_S> vecTargets;
    /* 当前帧人体类别检测结果，人员密度 V2 只统计人体目标 */
    const ot_aidetect_object_of_one_class *pstHumanClass = find_object_class(stContext.stResult, OT_AIDETECT_CLASS_HUMAN);
    /* 当前灵敏度阈值 */
    const float fSensitivityThreshold = 1.0f - static_cast<float>(m_stAlgoCfg.nSensitivity) / 100.0f;

    if (pstHumanClass != nullptr)
    {
        for (size_t i = 0; i < pstHumanClass->object_num; ++i)
        {
            /* 当前遍历到的人体目标 */
            const ot_aidetect_object &stObject = pstHumanClass->objects[i];
            if (stObject.track_status == OT_AIDETECT_TRACK_STATUS_DIE)
            {
                continue;
            }

            if ((stObject.track_status == OT_AIDETECT_TRACK_STATUS_NEW ||
                 stObject.track_status == OT_AIDETECT_TRACK_STATUS_UPDATE) &&
                stObject.detect_confidence < fSensitivityThreshold)
            {
                continue;
            }

            if (!is_in_region(m_stAlgoCfg.stDetectRegion, stObject))
            {
                continue;
            }

            add_result_to_vector(stObject, stContext.vstRectInfo);
            vecTargets.emplace_back(build_density_snapshot(stObject, llNowMs));
        }
    }

    /* 当前检测区域内的人体目标数量 */
    const uint32_t nPeopleCount = static_cast<uint32_t>(vecTargets.size());
    updateStayTimeAccumulator(llNowMs, nPeopleCount);
    /* 当前人数命中的密度等级事件，未达到阈值时返回人员密度主类型作为无等级标识 */
    const Event::Type_E enAlarmEventType = calculateAlarmEventType(nPeopleCount);

    /* 普通密度报警上下文，用于规则匹配 normal 等级 */
    EventTriggerContext_S stNormalContext = build_density_context(stContext,
                                                                  Event::Type_E::PEOPLE_DENSITY_NORMAL,
                                                                  nPeopleCount,
                                                                  enAlarmEventType);
    /* 中度密度报警上下文，用于规则匹配 medium 等级 */
    EventTriggerContext_S stMediumContext = build_density_context(stContext,
                                                                  Event::Type_E::PEOPLE_DENSITY_MEDIUM,
                                                                  nPeopleCount,
                                                                  enAlarmEventType);
    /* 严重密度报警上下文，用于规则匹配 severe 等级 */
    EventTriggerContext_S stSevereContext = build_density_context(stContext,
                                                                  Event::Type_E::PEOPLE_DENSITY_SEVERE,
                                                                  nPeopleCount,
                                                                  enAlarmEventType);

    m_normalAlarmStateMachine.handleAlarmState(enAlarmEventType == Event::Type_E::PEOPLE_DENSITY_NORMAL,
                                               stNormalContext);
    m_mediumAlarmStateMachine.handleAlarmState(enAlarmEventType == Event::Type_E::PEOPLE_DENSITY_MEDIUM,
                                               stMediumContext);
    m_severeAlarmStateMachine.handleAlarmState(enAlarmEventType == Event::Type_E::PEOPLE_DENSITY_SEVERE,
                                               stSevereContext);

    if (m_pReporter && shouldEmitStatisticsReport(llNowMs))
    {
        /* 人员密度统计数据按固定周期上报，等级报警只走状态机，不触发主事件上报 */
        emitDensityReport(stContext, nPeopleCount, enAlarmEventType, vecTargets);
    }
}

bool CHVFPeopleDensityProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}

Event::Type_E CHVFPeopleDensityProcessor::calculateAlarmEventType(uint32_t nPeopleCount) const
{
    if (m_stAlgoCfg.stDensityAlarm.stSevere.bEnable &&
        nPeopleCount >= m_stAlgoCfg.stDensityAlarm.stSevere.nThreshold)
    {
        return Event::Type_E::PEOPLE_DENSITY_SEVERE;
    }

    if (m_stAlgoCfg.stDensityAlarm.stMedium.bEnable &&
        nPeopleCount >= m_stAlgoCfg.stDensityAlarm.stMedium.nThreshold)
    {
        return Event::Type_E::PEOPLE_DENSITY_MEDIUM;
    }

    if (m_stAlgoCfg.stDensityAlarm.stNormal.bEnable &&
        nPeopleCount >= m_stAlgoCfg.stDensityAlarm.stNormal.nThreshold)
    {
        return Event::Type_E::PEOPLE_DENSITY_NORMAL;
    }

    return Event::Type_E::PEOPLE_DENSITY_DETECTION;
}

bool CHVFPeopleDensityProcessor::shouldEmitStatisticsReport(long long llNowMs)
{
    if (m_llLastStatisticsReportTs <= 0)
    {
        m_llLastStatisticsReportTs = llNowMs;
        return false;
    }

    if ((llNowMs - m_llLastStatisticsReportTs) < m_llStatisticsReportIntervalMs)
    {
        return false;
    }

    m_llLastStatisticsReportTs = llNowMs;
    return true;
}

void CHVFPeopleDensityProcessor::updateStayTimeAccumulator(long long llNowMs, uint32_t nPeopleCount)
{
    if (m_llStaySampleLastTs <= 0)
    {
        m_llStaySampleLastTs = llNowMs;
        m_nStayLastPeopleCount = nPeopleCount;
        m_nStayMaxPeopleCount = nPeopleCount;
        return;
    }

    if (llNowMs < m_llStaySampleLastTs)
    {
        dlog_warn("人员密度V2停留时间采样时间回退，重置区间统计 last[%lld] now[%lld]",
                  m_llStaySampleLastTs,
                  llNowMs);
        resetStayTimeAccumulator(nPeopleCount);
        m_llStaySampleLastTs = llNowMs;
        return;
    }

    /*
     * 区间占用近似算法不做单人轨迹停留统计，而是用相邻两帧之间的人数近似该时间片的人数状态：
     * 1. 设第 i 次采样时间为 t_i，区域人数为 c_i。
     * 2. 相邻采样区间 [t_i, t_{i+1}) 的人时贡献为 c_i * (t_{i+1} - t_i)，单位是 人*毫秒。
     * 3. 当前上报周期累计人时 occupancy_ms = Σ(c_i * Δt_i)。
     * 4. 当前上报周期估算人数规模取 max(c_i)，即周期内观测到的最大同时人数。
     * 5. 平均停留时间 average_stay_sec = occupancy_ms / max_people_count / 1000。
     */
    const long long llDeltaMs = llNowMs - m_llStaySampleLastTs;
    m_llStayOccupancyMs += static_cast<long long>(m_nStayLastPeopleCount) * llDeltaMs;
    m_nStayMaxPeopleCount = std::max(m_nStayMaxPeopleCount, nPeopleCount);
    m_llStaySampleLastTs = llNowMs;
    m_nStayLastPeopleCount = nPeopleCount;
}

uint32_t CHVFPeopleDensityProcessor::calculateAverageStayTimeSec() const
{
    if (m_nStayMaxPeopleCount == 0 || m_llStayOccupancyMs <= 0)
    {
        return 0;
    }

    /* 平均停留秒数 = 累计人时毫秒 / 周期最大人数 / 1000 */
    const long long llAverageStayMs = m_llStayOccupancyMs / static_cast<long long>(m_nStayMaxPeopleCount);
    const long long llAverageStaySec = llAverageStayMs / 1000;
    return llAverageStaySec > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(llAverageStaySec);
}

void CHVFPeopleDensityProcessor::resetStayTimeAccumulator(uint32_t nCurrentPeopleCount)
{
    m_llStayOccupancyMs = 0;
    m_nStayLastPeopleCount = nCurrentPeopleCount;
    m_nStayMaxPeopleCount = nCurrentPeopleCount;
}

void CHVFPeopleDensityProcessor::emitDensityReport(
    const SHVFProcessContext &stContext,
    uint32_t nPeopleCount,
    Event::Type_E enAlarmEventType,
    const std::vector<EventStatistics_NS::TargetSnapshot_S> &vecTargets)
{
    if (!m_pReporter)
    {
        return;
    }

    /* 人员密度业务上报结构，和事件联动上下文保持解耦 */
    EventStatistics_NS::Report_S stReport;
    stReport.enStatisticsType = EventStatistics_NS::StatisticsType_E::PEOPLE_DENSITY;
    stReport.enEventType = Event::Type_E::PEOPLE_DENSITY_DETECTION;
    stReport.enAlarmEventType = enAlarmEventType;
    stReport.nChnId = stContext.nChnId;
    stReport.nRuleId = 0;
    stReport.llFrameTimestampMs = stContext.llTimestamp;
    stReport.nReportSeq = ++m_nReportSeq;
    stReport.nCurrentPeopleCount = nPeopleCount;
    stReport.nAverageStayTimeSec = calculateAverageStayTimeSec();
    stReport.vecTargets = vecTargets;
    if (m_pReporter->shouldBuildHeavyPayload())
    {
        buildPanoramaImage(stContext.pFrameInfo, stReport.stPanoramaImage);
    }
    dlog_info("人员密度V2周期上报: 通道[%d] 序号[%u] 当前人数[%u] 目标数[%zu] 上报间隔毫秒[%lld]",
              stReport.nChnId,
              stReport.nReportSeq,
              stReport.nCurrentPeopleCount,
              stReport.vecTargets.size(),
              m_llStatisticsReportIntervalMs);
    m_pReporter->report(stReport);
    resetStayTimeAccumulator(nPeopleCount);
}

bool CHVFPeopleDensityProcessor::buildPanoramaImage(ot_video_frame_info *pFrameInfo,
                                                    EventStatistics_NS::ImagePayload_S &stImage) const
{
    stImage = EventStatistics_NS::ImagePayload_S();
    if (pFrameInfo == nullptr)
    {
        return false;
    }

    EventTvSdkImage_S stTvSdkImage;
    if (AiAppCommon::encode_video_frame_to_jpeg_memory(pFrameInfo, stTvSdkImage) != OK)
    {
        dlog_warn("人员密度V2全景图编码失败");
        return false;
    }

    stImage.vecJpeg = std::move(stTvSdkImage.vecJpeg);
    stImage.nWidth = pFrameInfo->video_frame.width;
    stImage.nHeight = pFrameInfo->video_frame.height;
    stImage.strTag = "panorama";
    return true;
}
} // namespace HVFDetectInternal
#endif
