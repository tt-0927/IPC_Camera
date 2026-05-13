/**
 * @FilePath     : hvf_people_flow_state_store.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-22 18:44:48
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-23 09:37:40
 * @Description  : HVF 人流统计运行态状态仓库
 */

#pragma once

#include "alarm_define.h"
#include "event_alarm/statistics/event_statistics_report.hpp"

namespace HVFDetectInternal
{
#if CAP_AI_PEOPLE_STATISTICS
class CHVFPeopleFlowStateStore
{
public:
    /**
     * @brief   : 设置人流统计配置
     * @param    {PeopleFlowStatistics_S} &stAlgoCfg：人流统计配置
     * @return   {void}
     */
    void setConfig(const Alarm::PeopleFlowStatistics_S &stAlgoCfg);

    /**
     * @brief   : 记录进入目标
     * @param    {TargetSnapshot_S} &stSnapshot：进入目标快照
     * @return   {void}
     */
    void onEnter(const EventStatistics_NS::TargetSnapshot_S &stSnapshot);

    /**
     * @brief   : 记录离开目标
     * @param    {TargetSnapshot_S} &stSnapshot：离开目标快照
     * @return   {void}
     */
    void onLeave(const EventStatistics_NS::TargetSnapshot_S &stSnapshot);

    /**
     * @brief   : 设置当前滞留人数
     * @param    {uint32_t} nCurrentStayCount：当前滞留人数
     * @return   {void}
     */
    void setCurrentStayCount(uint32_t nCurrentStayCount);

    /**
     * @brief   : 根据定时清零配置检查是否需要清零
     * @param    {long long} llNowMs：当前毫秒时间戳
     * @return   {void}
     */
    void maybeTimedReset(long long llNowMs);

    /**
     * @brief   : 构建并消费本轮统计报告
     * @param    {int} nChnId：通道号
     * @param    {long long} llNowMs：当前毫秒时间戳
     * @param    {bool} bForce：是否强制上报
     * @return   {EventStatistics_NS::Report_S} 统计报告
     */
    EventStatistics_NS::Report_S buildAndConsumeReport(int nChnId, long long llNowMs, bool bForce);

    /**
     * @brief   : 构建本轮统计报告只读快照
     * @param    {int} nChnId：通道号
     * @param    {long long} llNowMs：当前毫秒时间戳
     * @param    {bool} bForce：是否强制上报
     * @return   {EventStatistics_NS::Report_S} 统计报告快照
     * @note    : 该接口不递增正式上报序号，也不清空进入/离开目标缓存
     */
    EventStatistics_NS::Report_S buildReportSnapshot(int nChnId, long long llNowMs, bool bForce) const;

    /**
     * @brief   : 获取当前滞留人数对应的报警事件类型
     * @return   {Event::Type_E} 报警事件类型，无三级报警时返回人流统计主事件
     */
    Event::Type_E getStayAlarmEventType() const;

    /**
     * @brief   : 获取累计进入人数
     * @return   {uint32_t} 累计进入人数
     */
    uint32_t getEnterCount() const;

    /**
     * @brief   : 获取累计离开人数
     * @return   {uint32_t} 累计离开人数
     */
    uint32_t getLeaveCount() const;

    /**
     * @brief   : 获取累计通行总人数
     * @return   {uint32_t} 累计通行总人数
     */
    uint32_t getTotalCount() const;

    /**
     * @brief   : 获取当前滞留人数
     * @return   {uint32_t} 当前滞留人数
     */
    uint32_t getCurrentStayCount() const;

    /**
     * @brief   : 清空统计运行态数据
     * @return   {void}
     */
    void clear();

private:
    /* 人流统计配置缓存 */
    Alarm::PeopleFlowStatistics_S m_stAlgoCfg;
    /* 累计进入人数 */
    uint32_t m_nEnterCount = 0;
    /* 累计离开人数 */
    uint32_t m_nLeaveCount = 0;
    /* 当前滞留人数 */
    uint32_t m_nCurrentStayCount = 0;
    /* 统计报告递增序号 */
    uint32_t m_nReportSeq = 0;
    /* 上一次定时清零的日序号，避免同一天重复清零 */
    long long m_llLastResetDay = -1;
    /* 本轮进入目标快照缓存 */
    std::vector<EventStatistics_NS::TargetSnapshot_S> m_vecEnterTargets;
    /* 本轮离开目标快照缓存 */
    std::vector<EventStatistics_NS::TargetSnapshot_S> m_vecLeaveTargets;
};
#endif
} // namespace HVFDetectInternal
