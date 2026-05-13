/**
 * @FilePath     : people_head_density_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 09:13:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-29 15:15:57
 * @Description  : 人头检测-人员密度处理器实现
 */

#include "people_head_density_processor.hpp"

#if CAP_AI_PEOPLE_STATISTICS
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>

#include "dlog.h"
#include "IpcRet.h"
#include "video_frame_jpeg_encoder.hpp"

namespace PeopleHeadDetectInternal
{
void CPeopleHeadDensityProcessor::setEnabled(bool bEnable)
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

void CPeopleHeadDensityProcessor::setAlgoParamCfg(const Alarm::PeopleDensityDetection_S &stAlgoCfg,
                                                  int nWidth,
                                                  int nHeight)
{
    dlog_debug("ai_app: 设置人员密度侦测参数");
    m_stAlgoCfg = stAlgoCfg;
    convert_density_config_and_enable(m_stAlgoCfg, nWidth, nHeight);
    if (m_stAlgoCfg.nReportInterval > 0)
    {
        m_llStatisticsReportIntervalMs = static_cast<long long>(m_stAlgoCfg.nReportInterval) * 1000;
    }
}

void CPeopleHeadDensityProcessor::setReporter(
    const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
    m_pReporter = pReporter;
}

void CPeopleHeadDensityProcessor::process(SPeopleHeadProcessContext &stContext)
{
    if (!m_stAlgoCfg.bEnable)
    {
        return;
    }

    /* 当前区域内目标快照，用于后续业务侧上报 */
    std::vector<EventStatistics_NS::TargetSnapshot_S> vecTargets;
    for (const auto &stBoxData : stContext.vBoxDatas)
    {
        if (!is_in_region(m_stAlgoCfg.stDetectRegion, stBoxData.stBoxs))
        {
            continue;
        }

        add_result_to_vector(stBoxData, stContext.vstRectInfo);
        vecTargets.emplace_back(build_density_snapshot(stBoxData, stContext.llNowMs));
    }

    /* 当前检测区域内的人头目标数量 */
    const uint32_t nPeopleCount = static_cast<uint32_t>(vecTargets.size());
    updateStayTimeAccumulator(stContext.llNowMs, nPeopleCount);
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

    if (m_pReporter && shouldEmitStatisticsReport(stContext.llNowMs))
    {
        /* 人员密度统计数据按固定周期上报，等级报警只走状态机，不触发主事件上报 */
        emitDensityReport(stContext, nPeopleCount, enAlarmEventType, vecTargets);
    }
}

bool CPeopleHeadDensityProcessor::isEnabled() const
{
    return m_stAlgoCfg.bEnable;
}

Event::Type_E CPeopleHeadDensityProcessor::calculateAlarmEventType(uint32_t nPeopleCount) const
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

bool CPeopleHeadDensityProcessor::shouldEmitStatisticsReport(long long llNowMs)
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

void CPeopleHeadDensityProcessor::updateStayTimeAccumulator(long long llNowMs, uint32_t nPeopleCount)
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
        dlog_warn("人员密度停留时间采样时间回退，重置区间统计 last[%lld] now[%lld]",
                  m_llStaySampleLastTs,
                  llNowMs);
        resetStayTimeAccumulator(nPeopleCount);
        m_llStaySampleLastTs = llNowMs;
        return;
    }

    /*
     * 区间占用近似算法不做单人 track_id 跟踪，而是用相邻两帧之间的人数近似该时间片的人数状态：
     * 1. 设第 i 次采样时间为 t_i，区域人数为 c_i。
     * 2. 相邻采样区间 [t_i, t_{i+1}) 的人时贡献为 c_i * (t_{i+1} - t_i)，单位是 人*毫秒。
     * 3. 当前上报周期累计人时 occupancy_ms = Σ(c_i * Δt_i)。
     * 4. 当前上报周期估算人数规模取 max(c_i)，即周期内观测到的最大同时人数，避免把帧数当成人数。
     * 5. 平均停留时间 average_stay_sec = occupancy_ms / max_people_count / 1000。
     * 该口径适合人员密度没有稳定目标 ID 的场景，含义是“本上报周期内单个估算人员平均贡献的区域占用时长”。
     */
    const long long llDeltaMs = llNowMs - m_llStaySampleLastTs;
    m_llStayOccupancyMs += static_cast<long long>(m_nStayLastPeopleCount) * llDeltaMs;
    m_nStayMaxPeopleCount = std::max(m_nStayMaxPeopleCount, nPeopleCount);
    m_llStaySampleLastTs = llNowMs;
    m_nStayLastPeopleCount = nPeopleCount;
}

uint32_t CPeopleHeadDensityProcessor::calculateAverageStayTimeSec() const
{
    if (m_nStayMaxPeopleCount == 0 || m_llStayOccupancyMs <= 0)
    {
        return 0;
    }

    /*
     * 计算公式：平均停留秒数 = 累计人时毫秒 / 周期最大人数 / 1000。
     * 累计人时毫秒越大表示区域被多人长时间占用；周期最大人数用于近似本周期出现过的人员规模。
     */
    const long long llAverageStayMs = m_llStayOccupancyMs / static_cast<long long>(m_nStayMaxPeopleCount);
    const long long llAverageStaySec = llAverageStayMs / 1000;
    return llAverageStaySec > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(llAverageStaySec);
}

void CPeopleHeadDensityProcessor::resetStayTimeAccumulator(uint32_t nCurrentPeopleCount)
{
    m_llStayOccupancyMs = 0;
    m_nStayLastPeopleCount = nCurrentPeopleCount;
    m_nStayMaxPeopleCount = nCurrentPeopleCount;
}

void CPeopleHeadDensityProcessor::emitDensityReport(
    const SPeopleHeadProcessContext &stContext,
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
    stReport.llFrameTimestampMs = stContext.llNowMs;
    stReport.nReportSeq = ++m_nReportSeq;
    stReport.nCurrentPeopleCount = nPeopleCount;
    stReport.nAverageStayTimeSec = calculateAverageStayTimeSec();
    stReport.vecTargets = vecTargets;
    if (m_pReporter->shouldBuildHeavyPayload())
    {
        buildPanoramaImage(stContext.pFrameInfo, stReport.stPanoramaImage);
    }
    /* 周期上报前输出统计摘要，便于排查定时上报时的当前人数和目标列表规模 */
    dlog_info("人员密度周期上报: 通道[%d] 序号[%u] 当前人数[%u] 目标数[%zu] 上报间隔毫秒[%lld]",
              stReport.nChnId,
              stReport.nReportSeq,
              stReport.nCurrentPeopleCount,
              stReport.vecTargets.size(),
              m_llStatisticsReportIntervalMs);
    m_pReporter->report(stReport);
    resetStayTimeAccumulator(nPeopleCount);
}

bool CPeopleHeadDensityProcessor::buildPanoramaImage(ot_video_frame_info *pFrameInfo,
                                                     EventStatistics_NS::ImagePayload_S &stImage) const
{
    stImage = EventStatistics_NS::ImagePayload_S();
    if (pFrameInfo == nullptr)
    {
        return false;
    }

    const std::string strFilename = buildPanoramaTempFilePath(static_cast<long long>(get_time_ms()));
    if (AiAppCommon::encode_video_frame_to_jpeg_file(pFrameInfo, strFilename) != OK)
    {
        std::remove(strFilename.c_str());
        return false;
    }

    if (!loadJpegFile(strFilename, stImage.vecJpeg))
    {
        std::remove(strFilename.c_str());
        return false;
    }

    std::remove(strFilename.c_str());
    stImage.nWidth = pFrameInfo->video_frame.width;
    stImage.nHeight = pFrameInfo->video_frame.height;
    stImage.strTag = "panorama";
    return true;
}

bool CPeopleHeadDensityProcessor::loadJpegFile(const std::string &strFilename,
                                               std::vector<unsigned char> &vecJpeg) const
{
    vecJpeg.clear();
    std::ifstream file(strFilename, std::ios::binary);
    if (!file.is_open())
    {
        dlog_warn("读取人员密度全景临时图片失败，文件[%s]", strFilename.c_str());
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streampos nFileSize = file.tellg();
    if (nFileSize <= 0)
    {
        return false;
    }

    file.seekg(0, std::ios::beg);
    vecJpeg.resize(static_cast<size_t>(nFileSize));
    const std::streamsize nReadSize = static_cast<std::streamsize>(nFileSize);
    file.read(reinterpret_cast<char *>(vecJpeg.data()), nReadSize);
    return file.gcount() == nReadSize;
}

std::string CPeopleHeadDensityProcessor::buildPanoramaTempFilePath(long long llNowMs) const
{
    return "/tmp/people_density_panorama_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)) + "_" +
           std::to_string(llNowMs) + ".jpg";
}
} // namespace PeopleHeadDetectInternal
#endif
