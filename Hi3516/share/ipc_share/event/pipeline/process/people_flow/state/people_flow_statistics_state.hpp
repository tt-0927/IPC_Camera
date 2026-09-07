/**
 * @FilePath     : people_flow_statistics_state.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 人流统计累计状态仓库，负责计数、报告序号与定时清零
 */

#pragma once

#if CAP_AI_PEOPLE_STATISTICS

#include <cstdint>
#include <vector>

#include "people_flow_types.hpp"
#include "processor_output.hpp"

namespace AiPipeline_NS
{
namespace PeopleFlow_NS
{
/* 人流统计累计状态仓库 */
class CPeopleFlowStatisticsState
{
public:
    /**
     * @brief   : 构造统计状态仓库
     */
    CPeopleFlowStatisticsState() = default;

    /**
     * @brief   : 更新内部配置缓存
     * @param    {const PeopleFlowConfig_S &} stConfig：归一化内部配置
     * @return   {void}
     */
    void set_config(const PeopleFlowConfig_S &stConfig);

    /**
     * @brief   : 记录进入目标并累加计数
     * @param    {const ReportTargetDraft_S &} stTarget：进入目标草稿
     * @return   {void}
     */
    void on_enter(const ReportTargetDraft_S &stTarget);

    /**
     * @brief   : 记录离开目标并累加计数
     * @param    {const ReportTargetDraft_S &} stTarget：离开目标草稿
     * @return   {void}
     */
    void on_leave(const ReportTargetDraft_S &stTarget);

    /**
     * @brief   : 按定时清零配置检查是否到达清零时间
     * @param    {int64_t} llNowMs：当前 wall clock 毫秒时间戳
     * @return   {void}
     * @note    : 同一天只执行一次，避免每帧重复清零
     */
    void maybe_timed_reset(int64_t llNowMs);

    /**
     * @brief   : 构建并消费本轮统计报告草稿
     * @param    {int} nChannelId：通道号
     * @param    {int64_t} llNowMs：当前 wall clock 毫秒时间戳
     * @param    {bool} bForce：是否强制上报
     * @return   {StatisticsReportDraft_S} 统计报告草稿
     * @note    : 递增正式上报序号并清空进入/离开目标缓存
     */
    StatisticsReportDraft_S build_and_consume_report(int nChannelId, int64_t llNowMs, bool bForce);

    /**
     * @brief   : 获取当前滞留人数对应的报警事件类型
     * @return   {Event::Type_E} 报警事件类型，无三级报警时返回人流统计主事件
     */
    Event::Type_E get_stay_alarm_event_type() const;

    /**
     * @brief   : 获取累计进入人数
     * @return   {uint32_t} 累计进入人数
     */
    uint32_t enter_count() const;

    /**
     * @brief   : 获取累计离开人数
     * @return   {uint32_t} 累计离开人数
     */
    uint32_t leave_count() const;

    /**
     * @brief   : 获取累计通行总人数
     * @return   {uint32_t} 累计通行总人数
     */
    uint32_t total_count() const;

    /**
     * @brief   : 获取当前滞留人数
     * @return   {uint32_t} 当前滞留人数
     */
    uint32_t current_stay_count() const;

    /**
     * @brief   : 获取最近报告序号
     * @return   {uint32_t} 报告序号
     */
    uint32_t report_seq() const;

    /**
     * @brief   : 清空全部统计运行态
     * @return   {void}
     */
    void clear();

private:
    /* 内部配置缓存 */
    PeopleFlowConfig_S m_stConfig;
    /* 累计进入人数 */
    uint32_t m_nEnterCount = 0;
    /* 累计离开人数 */
    uint32_t m_nLeaveCount = 0;
    /* 当前滞留人数 */
    uint32_t m_nCurrentStayCount = 0;
    /* 统计报告递增序号 */
    uint32_t m_nReportSeq = 0;
    /* 上一次定时清零的日序号，避免同一天重复清零 */
    int64_t m_llLastResetDay = -1;
    /* 本轮进入目标草稿缓存 */
    std::vector<ReportTargetDraft_S> m_vecEnterTargets;
    /* 本轮离开目标草稿缓存 */
    std::vector<ReportTargetDraft_S> m_vecLeaveTargets;
};
} // namespace PeopleFlow_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_STATISTICS
