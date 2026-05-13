/**
 * @FilePath     : hvf_people_flow_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-22 18:44:48
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:18:02
 * @Description  : HVF 人流统计处理器
 */

#pragma once

#include <memory>

#include "algorithm.hpp"
#include "hvf_detect_context.hpp"
#include "hvf_detect_common.hpp"
#include "hvf_people_flow_state_store.hpp"
#include "target_index_manager.hpp"

namespace HVFDetectInternal
{
#if CAP_AI_PEOPLE_STATISTICS
class CHVFPeopleFlowProcessor
{
public:
    /**
     * @brief   : 构造 HVF 人流统计处理器
     */
    CHVFPeopleFlowProcessor();

    /**
     * @brief   : 设置人流统计使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置人流统计参数
     * @param    {PeopleFlowStatistics_S} &stAlgoCfg：人流统计配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::PeopleFlowStatistics_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 设置统计上报器
     * @param    {IEventStatisticsReporter} &pReporter：统计上报器
     * @return   {void}
     */
    void setReporter(const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter);

    /**
     * @brief   : 处理单帧 HVF 人流统计结果
     * @param    {SHVFProcessContext &} stContext：单帧处理上下文
     * @return   {void}
     */
    void process(SHVFProcessContext &stContext);

    /**
     * @brief   : 清空人流统计运行态结果
     * @return   {void}
     */
    void clearStatisticsResult();

    /**
     * @brief   : 获取当前是否使能
     * @return   {bool} true：使能 false：关闭
     */
    bool isEnabled() const;

private:
    /**
     * @brief   : 判断是否到达统计数据周期上报时间
     * @param    {long long} llNowMs：当前毫秒时间戳
     * @return   {bool} true：需要上报 false：无需上报
     */
    bool shouldEmitStatisticsReport(long long llNowMs);

    /* 人流统计配置 */
    Alarm::PeopleFlowStatistics_S m_stAlgoCfg;
    /* 人流统计目标索引管理器 */
    CTargetIndexManager20 m_indexManager{ "HVFPeopleFlowIndex" };
    /* 跨线跟踪状态，单规则线按目标索引存储 */
    BoundaryTrackStatus_S m_stTrackStatus[SVP_AIDETECT_MAX_OUTPUT_RECT_NUM];
    /* 人流统计累计状态仓库 */
    CHVFPeopleFlowStateStore m_stateStore;
    /* 滞留普通报警状态机 */
    CAlarmStateMachine m_normalAlarmStateMachine;
    /* 滞留中度报警状态机 */
    CAlarmStateMachine m_mediumAlarmStateMachine;
    /* 滞留严重报警状态机 */
    CAlarmStateMachine m_severeAlarmStateMachine;
    /* 业务统计上报器 */
    std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> m_pReporter;
    /* 统计数据上报间隔，默认 1 分钟，单位毫秒 */
    long long m_llStatisticsReportIntervalMs = 1 * 60 * 1000;
    /* 测试使用 10 秒 */
    // long long m_llStatisticsReportIntervalMs = 10 * 1000;
    /* 上一次统计数据上报时间戳，单位毫秒 */
    long long m_llLastStatisticsReportTs = 0;
};
#endif
} // namespace HVFDetectInternal
