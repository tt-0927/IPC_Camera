/**
 * @FilePath     : hvf_people_flow_state_store.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-22 18:44:48
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-23 09:37:25
 * @Description  : HVF 人流统计运行态状态仓库实现
 */

#include "hvf_people_flow_state_store.hpp"

#if CAP_AI_PEOPLE_STATISTICS
#include <algorithm>
#include <ctime>

#include "dlog.h"

namespace
{
/**
 * @brief   : 将毫秒时间戳转换成本地时间
 * @param    {long long} llNowMs：毫秒时间戳
 * @return   {std::tm} 本地时间
 */
std::tm to_local_tm(long long llNowMs)
{
    /* time_t 使用秒级时间戳，毫秒部分不参与定时清零判断 */
    const std::time_t nowSeconds = static_cast<std::time_t>(llNowMs / 1000);
    /* 本地时间结构，供定时清零按自然日判断 */
    std::tm stLocalTime;
    localtime_r(&nowSeconds, &stLocalTime);
    return stLocalTime;
}

/**
 * @brief   : 获取本地日期序号
 * @param    {std::tm} &stLocalTime：本地时间
 * @return   {long long} 日期序号
 */
long long get_day_key(const std::tm &stLocalTime)
{
    return static_cast<long long>(stLocalTime.tm_year) * 1000 + stLocalTime.tm_yday;
}

/**
 * @brief   : 判断当前时间是否已到定时清零时间点
 * @param    {std::tm} &stLocalTime：本地时间
 * @param    {Common::Time_S} &stExecuteTime：配置的清零时间
 * @return   {bool} true：已到达 false：未到达
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

namespace HVFDetectInternal
{
void CHVFPeopleFlowStateStore::setConfig(const Alarm::PeopleFlowStatistics_S &stAlgoCfg)
{
    m_stAlgoCfg = stAlgoCfg;
}

void CHVFPeopleFlowStateStore::onEnter(const EventStatistics_NS::TargetSnapshot_S &stSnapshot)
{
    ++m_nEnterCount;
    m_nCurrentStayCount = (m_nEnterCount >= m_nLeaveCount) ? (m_nEnterCount - m_nLeaveCount) : 0;
    m_vecEnterTargets.emplace_back(stSnapshot);
}

void CHVFPeopleFlowStateStore::onLeave(const EventStatistics_NS::TargetSnapshot_S &stSnapshot)
{
    ++m_nLeaveCount;
    m_nCurrentStayCount = (m_nEnterCount >= m_nLeaveCount) ? (m_nEnterCount - m_nLeaveCount) : 0;
    m_vecLeaveTargets.emplace_back(stSnapshot);
}

void CHVFPeopleFlowStateStore::setCurrentStayCount(uint32_t nCurrentStayCount)
{
    m_nCurrentStayCount = nCurrentStayCount;
}

void CHVFPeopleFlowStateStore::maybeTimedReset(long long llNowMs)
{
    if (!m_stAlgoCfg.stTimedReset.bEnable)
    {
        return;
    }

    /* 同一天只在配置时间点之后触发一次清零，避免每帧重复清零 */
    const std::tm stLocalTime = to_local_tm(llNowMs);
    /* 当前自然日唯一标识，用于避免同一天重复清零 */
    const long long llDayKey = get_day_key(stLocalTime);
    if (m_llLastResetDay == llDayKey || !is_reset_time_reached(stLocalTime, m_stAlgoCfg.stTimedReset.stExecuteTime))
    {
        return;
    }

    clear();
    m_llLastResetDay = llDayKey;
    dlog_info("人流统计定时清零完成，day_key[%lld]", llDayKey);
}

EventStatistics_NS::Report_S CHVFPeopleFlowStateStore::buildAndConsumeReport(int nChnId,
                                                                             long long llNowMs,
                                                                             bool bForce)
{
    /* 当前构建的人流统计报告，构建后会消费本轮目标缓存 */
    EventStatistics_NS::Report_S stReport;
    stReport.enStatisticsType = EventStatistics_NS::StatisticsType_E::PEOPLE_FLOW;
    stReport.enEventType = Event::Type_E::PEOPLE_FLOW_STATISTICS;
    stReport.enAlarmEventType = Event::Type_E::PEOPLE_FLOW_STATISTICS;
    stReport.nChnId = nChnId;
    stReport.nRuleId = 0;
    stReport.llFrameTimestampMs = llNowMs;
    stReport.nReportSeq = ++m_nReportSeq;
    stReport.nEnterCount = m_nEnterCount;
    stReport.nLeaveCount = m_nLeaveCount;
    stReport.nTotalCount = m_nEnterCount + m_nLeaveCount;
    stReport.vecTargets.insert(stReport.vecTargets.end(), m_vecEnterTargets.begin(), m_vecEnterTargets.end());
    stReport.vecTargets.insert(stReport.vecTargets.end(), m_vecLeaveTargets.begin(), m_vecLeaveTargets.end());
    stReport.mapExtras["force"] = bForce ? "1" : "0";
    stReport.mapExtras["statistics_type"] = std::to_string(static_cast<int>(m_stAlgoCfg.enStatisticsType));

    m_vecEnterTargets.clear();
    m_vecLeaveTargets.clear();
    return stReport;
}

EventStatistics_NS::Report_S CHVFPeopleFlowStateStore::buildReportSnapshot(int nChnId,
                                                                           long long llNowMs,
                                                                           bool bForce) const
{
    /* 当前构建的人流统计只读快照，不消费目标缓存，避免影响正式 reporter 链路 */
    EventStatistics_NS::Report_S stReport;
    stReport.enStatisticsType = EventStatistics_NS::StatisticsType_E::PEOPLE_FLOW;
    stReport.enEventType = Event::Type_E::PEOPLE_FLOW_STATISTICS;
    stReport.enAlarmEventType = Event::Type_E::PEOPLE_FLOW_STATISTICS;
    stReport.nChnId = nChnId;
    stReport.nRuleId = 0;
    stReport.llFrameTimestampMs = llNowMs;
    stReport.nReportSeq = m_nReportSeq + 1;
    stReport.nEnterCount = m_nEnterCount;
    stReport.nLeaveCount = m_nLeaveCount;
    stReport.nTotalCount = m_nEnterCount + m_nLeaveCount;
    stReport.vecTargets.insert(stReport.vecTargets.end(), m_vecEnterTargets.begin(), m_vecEnterTargets.end());
    stReport.vecTargets.insert(stReport.vecTargets.end(), m_vecLeaveTargets.begin(), m_vecLeaveTargets.end());
    stReport.mapExtras["force"] = bForce ? "1" : "0";
    stReport.mapExtras["statistics_type"] = std::to_string(static_cast<int>(m_stAlgoCfg.enStatisticsType));
    return stReport;
}

Event::Type_E CHVFPeopleFlowStateStore::getStayAlarmEventType() const
{
    if (m_stAlgoCfg.stStayAlarm.stSevere.bEnable &&
        m_nCurrentStayCount >= m_stAlgoCfg.stStayAlarm.stSevere.nThreshold)
    {
        return Event::Type_E::PEOPLE_FLOW_STAY_SEVERE;
    }

    if (m_stAlgoCfg.stStayAlarm.stMedium.bEnable &&
        m_nCurrentStayCount >= m_stAlgoCfg.stStayAlarm.stMedium.nThreshold)
    {
        return Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM;
    }

    if (m_stAlgoCfg.stStayAlarm.stNormal.bEnable &&
        m_nCurrentStayCount >= m_stAlgoCfg.stStayAlarm.stNormal.nThreshold)
    {
        return Event::Type_E::PEOPLE_FLOW_STAY_NORMAL;
    }

    return Event::Type_E::PEOPLE_FLOW_STATISTICS;
}

uint32_t CHVFPeopleFlowStateStore::getEnterCount() const
{
    return m_nEnterCount;
}

uint32_t CHVFPeopleFlowStateStore::getLeaveCount() const
{
    return m_nLeaveCount;
}

uint32_t CHVFPeopleFlowStateStore::getTotalCount() const
{
    return m_nEnterCount + m_nLeaveCount;
}

uint32_t CHVFPeopleFlowStateStore::getCurrentStayCount() const
{
    return m_nCurrentStayCount;
}

void CHVFPeopleFlowStateStore::clear()
{
    m_nEnterCount = 0;
    m_nLeaveCount = 0;
    m_nCurrentStayCount = 0;
    m_nReportSeq = 0;
    m_vecEnterTargets.clear();
    m_vecLeaveTargets.clear();
}
} // namespace HVFDetectInternal
#endif
