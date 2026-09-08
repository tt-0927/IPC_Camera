/**
 * @FilePath     : people_flow_statistics_state.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 人流统计累计状态仓库实现
 */

#include "people_flow_statistics_state.hpp"

#include <ctime>

#include "dlog.h"
#include "event_alarm/statistics/event_statistics_report.hpp"

#if CAP_AI_PEOPLE_STATISTICS

namespace AiPipeline_NS
{
namespace PeopleFlow_NS
{
namespace
{
/**
 * @brief   : 将毫秒时间戳转换成本地时间
 * @param    {int64_t} llNowMs：wall clock 毫秒时间戳
 * @return   {std::tm} 本地时间
 */
std::tm to_local_tm(int64_t llNowMs)
{
    /* time_t 使用秒级时间戳，毫秒部分不参与定时清零判断 */
    const std::time_t nowSeconds = static_cast<std::time_t>(llNowMs / 1000);
    std::tm stLocalTime{};
    localtime_r(&nowSeconds, &stLocalTime);
    return stLocalTime;
}

/**
 * @brief   : 获取本地日期序号
 * @param    {const std::tm &} stLocalTime：本地时间
 * @return   {int64_t} 日期序号
 */
int64_t get_day_key(const std::tm &stLocalTime)
{
    return static_cast<int64_t>(stLocalTime.tm_year) * 1000 + stLocalTime.tm_yday;
}

/**
 * @brief   : 判断当前时间是否已到定时清零时间点
 * @param    {const std::tm &} stLocalTime：本地时间
 * @param    {const Common::Time_S &} stExecuteTime：配置的清零时间
 * @return   {bool} true：已到达
 */
bool is_reset_time_reached(const std::tm &stLocalTime, const Common::Time_S &stExecuteTime)
{
    /* 当前本地时间折算成当天秒数，便于与配置时间点比较 */
    const int nNowSeconds = stLocalTime.tm_hour * 3600 + stLocalTime.tm_min * 60 + stLocalTime.tm_sec;
    /* 配置清零时间折算成当天秒数 */
    const int nResetSeconds = stExecuteTime.nHour * 3600 + stExecuteTime.nMinute * 60 + stExecuteTime.nSecond;
    return nNowSeconds >= nResetSeconds;
}
} // namespace

void CPeopleFlowStatisticsState::set_config(const PeopleFlowConfig_S &stConfig)
{
    m_stConfig = stConfig;
}

void CPeopleFlowStatisticsState::on_enter(const ReportTargetDraft_S &stTarget)
{
    ++m_nEnterCount;
    m_nCurrentStayCount = (m_nEnterCount >= m_nLeaveCount) ? (m_nEnterCount - m_nLeaveCount) : 0;
    ReportTargetDraft_S stSnapshot = stTarget;
    stSnapshot.nSnapshotType = static_cast<int>(EventStatistics_NS::SnapshotType_E::ENTER);
    m_vecEnterTargets.emplace_back(std::move(stSnapshot));
}

void CPeopleFlowStatisticsState::on_leave(const ReportTargetDraft_S &stTarget)
{
    ++m_nLeaveCount;
    m_nCurrentStayCount = (m_nEnterCount >= m_nLeaveCount) ? (m_nEnterCount - m_nLeaveCount) : 0;
    ReportTargetDraft_S stSnapshot = stTarget;
    stSnapshot.nSnapshotType = static_cast<int>(EventStatistics_NS::SnapshotType_E::LEAVE);
    m_vecLeaveTargets.emplace_back(std::move(stSnapshot));
}

void CPeopleFlowStatisticsState::maybe_timed_reset(int64_t llNowMs)
{
    if (!m_stConfig.bTimedResetEnabled)
    {
        return;
    }

    /* 同一天只在配置时间点之后触发一次清零，避免每帧重复清零 */
    const std::tm stLocalTime = to_local_tm(llNowMs);
    const int64_t llDayKey = get_day_key(stLocalTime);
    if (m_llLastResetDay == llDayKey || !is_reset_time_reached(stLocalTime, m_stConfig.stResetTime))
    {
        return;
    }

    clear();
    m_llLastResetDay = llDayKey;
    dlog_info("人流统计定时清零完成，day_key[%lld]", llDayKey);
}

StatisticsReportDraft_S CPeopleFlowStatisticsState::build_and_consume_report(int nChannelId, int64_t llNowMs, bool bForce)
{
    /* 当前构建的统计报告草稿，构建后消费本轮目标缓存 */
    StatisticsReportDraft_S stDraft;
    stDraft.nChannelId = nChannelId;
    stDraft.nRuleId = static_cast<int>(PEOPLE_FLOW_DEFAULT_RULE_ID);
    stDraft.llTimestampMs = llNowMs;
    stDraft.nReportSeq = ++m_nReportSeq;
    stDraft.nEnterCount = m_nEnterCount;
    stDraft.nLeaveCount = m_nLeaveCount;
    stDraft.nTotalCount = m_nEnterCount + m_nLeaveCount;
    stDraft.nCurrentStayCount = m_nCurrentStayCount;
    stDraft.bForceReport = bForce;
    stDraft.vecTargets.insert(stDraft.vecTargets.end(), m_vecEnterTargets.begin(), m_vecEnterTargets.end());
    stDraft.vecTargets.insert(stDraft.vecTargets.end(), m_vecLeaveTargets.begin(), m_vecLeaveTargets.end());

    m_vecEnterTargets.clear();
    m_vecLeaveTargets.clear();
    return stDraft;
}

Event::Type_E CPeopleFlowStatisticsState::get_stay_alarm_event_type() const
{
    /* 三级报警按严重度从高到低选择，与旧实现一致 */
    if (m_stConfig.stStayAlarmSevere.bEnable && m_nCurrentStayCount >= m_stConfig.stStayAlarmSevere.nThreshold)
    {
        return Event::Type_E::PEOPLE_FLOW_STAY_SEVERE;
    }

    if (m_stConfig.stStayAlarmMedium.bEnable && m_nCurrentStayCount >= m_stConfig.stStayAlarmMedium.nThreshold)
    {
        return Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM;
    }

    if (m_stConfig.stStayAlarmNormal.bEnable && m_nCurrentStayCount >= m_stConfig.stStayAlarmNormal.nThreshold)
    {
        return Event::Type_E::PEOPLE_FLOW_STAY_NORMAL;
    }

    return Event::Type_E::PEOPLE_FLOW_STATISTICS;
}

uint32_t CPeopleFlowStatisticsState::enter_count() const
{
    return m_nEnterCount;
}

uint32_t CPeopleFlowStatisticsState::leave_count() const
{
    return m_nLeaveCount;
}

uint32_t CPeopleFlowStatisticsState::total_count() const
{
    return m_nEnterCount + m_nLeaveCount;
}

uint32_t CPeopleFlowStatisticsState::current_stay_count() const
{
    return m_nCurrentStayCount;
}

uint32_t CPeopleFlowStatisticsState::report_seq() const
{
    return m_nReportSeq;
}

void CPeopleFlowStatisticsState::clear()
{
    m_nEnterCount = 0;
    m_nLeaveCount = 0;
    m_nCurrentStayCount = 0;
    m_nReportSeq = 0;
    m_vecEnterTargets.clear();
    m_vecLeaveTargets.clear();
}
} // namespace PeopleFlow_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_STATISTICS
