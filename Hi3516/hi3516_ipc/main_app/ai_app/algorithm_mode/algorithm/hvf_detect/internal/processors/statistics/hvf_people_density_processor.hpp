/**
 * @FilePath     : hvf_people_density_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-26 15:01:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-26 15:19:30
 * @Description  : HVF 人员密度处理器
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "algorithm.hpp"
#include "event_alarm/statistics/event_statistics_reporter.hpp"
#include "hvf_detect_common.hpp"
#include "hvf_detect_context.hpp"

#if CAP_AI_PEOPLE_DENSITY_V2 && !CAP_AI_PEOPLE_STATISTICS
#error "CAP_AI_PEOPLE_STATISTICS must be enabled when CAP_AI_PEOPLE_DENSITY_V2 is enabled"
#endif

namespace HVFDetectInternal
{
#if CAP_AI_PEOPLE_DENSITY_V2
class CHVFPeopleDensityProcessor
{
public:
    /**
     * @brief   : 设置人员密度使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置人员密度参数
     * @param    {PeopleDensityDetection_S} &stAlgoCfg：人员密度配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::PeopleDensityDetection_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 设置统计上报器
     * @param    {IEventStatisticsReporter} &pReporter：统计上报器
     * @return   {void}
     */
    void setReporter(const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter);

    /**
     * @brief   : 处理单帧 HVF 人员密度结果
     * @param    {SHVFProcessContext &} stContext：单帧处理上下文
     * @return   {void}
     */
    void process(SHVFProcessContext &stContext);

    /**
     * @brief   : 获取当前是否使能
     * @return   {bool} true：使能 false：关闭
     */
    bool isEnabled() const;

private:
    /**
     * @brief   : 根据人数计算报警事件类型
     * @param    {uint32_t} nPeopleCount：当前人数
     * @return   {Event::Type_E} 报警事件类型
     */
    Event::Type_E calculateAlarmEventType(uint32_t nPeopleCount) const;

    /**
     * @brief   : 判断是否到达统计数据周期上报时间
     * @param    {long long} llNowMs：当前毫秒时间戳
     * @return   {bool} true：需要上报 false：无需上报
     */
    bool shouldEmitStatisticsReport(long long llNowMs);

    /**
     * @brief   : 更新人员密度区间占用累计值
     * @param    {long long} llNowMs：当前毫秒时间戳
     * @param    {uint32_t} nPeopleCount：当前帧区域人数
     * @return   {void}
     */
    void updateStayTimeAccumulator(long long llNowMs, uint32_t nPeopleCount);

    /**
     * @brief   : 计算当前上报区间的平均停留时间
     * @return   {uint32_t} 平均停留时间，单位秒
     */
    uint32_t calculateAverageStayTimeSec() const;

    /**
     * @brief   : 重置人员密度区间停留统计
     * @param    {uint32_t} nCurrentPeopleCount：重置后延续到下一区间的当前人数
     * @return   {void}
     */
    void resetStayTimeAccumulator(uint32_t nCurrentPeopleCount);

    /**
     * @brief   : 构建人员密度统计全景图
     * @param    {ot_video_frame_info} *pFrameInfo：当前检测帧
     * @param    {ImagePayload_S} &stImage：输出全景图负载
     * @return   {bool} true：构建成功 false：构建失败
     */
    bool buildPanoramaImage(ot_video_frame_info *pFrameInfo, EventStatistics_NS::ImagePayload_S &stImage) const;

    /**
     * @brief   : 输出人员密度统计报告
     * @param    {SHVFProcessContext} &stContext：单帧处理上下文
     * @param    {uint32_t} nPeopleCount：当前人数
     * @param    {Event::Type_E} enAlarmEventType：当前报警事件类型
     * @param    {vector<TargetSnapshot_S>} &vecTargets：当前区域目标快照
     * @return   {void}
     */
    void emitDensityReport(const SHVFProcessContext &stContext,
                           uint32_t nPeopleCount,
                           Event::Type_E enAlarmEventType,
                           const std::vector<EventStatistics_NS::TargetSnapshot_S> &vecTargets);

private:
    /* 人员密度配置 */
    Alarm::PeopleDensityDetection_S m_stAlgoCfg;
    /* 人员密度普通报警状态机 */
    CAlarmStateMachine m_normalAlarmStateMachine;
    /* 人员密度中度报警状态机 */
    CAlarmStateMachine m_mediumAlarmStateMachine;
    /* 人员密度严重报警状态机 */
    CAlarmStateMachine m_severeAlarmStateMachine;
    /* 业务统计上报器 */
    std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> m_pReporter;
    /* 统计数据上报间隔，默认 1 分钟，单位毫秒 */
    long long m_llStatisticsReportIntervalMs = 1 * 60 * 1000;
    /* 上一次统计数据上报时间戳，单位毫秒 */
    long long m_llLastStatisticsReportTs = 0;
    /* 上一次停留时间采样时间戳，单位毫秒 */
    long long m_llStaySampleLastTs = 0;
    /* 当前上报区间累计人时，单位为 人*毫秒 */
    long long m_llStayOccupancyMs = 0;
    /* 上一次采样时区域内人数，用于代表相邻采样点之间的人数状态 */
    uint32_t m_nStayLastPeopleCount = 0;
    /* 当前上报区间内观测到的最大区域人数，用于估算本区间出现过的人数规模 */
    uint32_t m_nStayMaxPeopleCount = 0;
    /* 统计报告递增序号 */
    uint32_t m_nReportSeq = 0;
};
#endif
} // namespace HVFDetectInternal
