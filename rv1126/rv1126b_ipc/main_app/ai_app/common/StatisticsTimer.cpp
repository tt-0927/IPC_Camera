/**
 * @FilePath     : StatisticsTimer.cpp
 * @Author       : xiejh (xiejh@kfb.cn)
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 17:05:51
 * @Description  : 统计代码运行耗时
 */

#include "StatisticsTimer.hpp"
#include "dlog.h"
#include <unistd.h>

std::unordered_map<std::string, CStatisticsTimer::TimerStats> CStatisticsTimer::s_timerStats;
std::mutex CStatisticsTimer::s_mutex;

/* 构造函数，传入计时器名称 */
CStatisticsTimer::CStatisticsTimer(const std::string& strName)
    : m_strName(strName),
      m_startTime(std::chrono::high_resolution_clock::now())
{
    /* 记录开始时间 */
}

/* 析构函数，自动计算并输出运行时间 */
CStatisticsTimer::~CStatisticsTimer()
{
    /* 记录结束时间 */
    auto end_time = std::chrono::high_resolution_clock::now();
    
    /* 计算运行时间，单位为毫秒 */
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - m_startTime).count();
    elapsed_ms = elapsed_ms > 0 ? elapsed_ms : 0;

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto& stats = s_timerStats[m_strName];

        if (stats.count == 0)
        {
            stats.min_time = elapsed_ms;
        }

        stats.total_time += elapsed_ms;
        stats.count++;
        stats.max_time = std::max(stats.max_time, elapsed_ms);
        stats.min_time = std::min(stats.min_time, elapsed_ms);
    }
    
    /* 记录平均值 */
    double avg_time = 0.0;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        avg_time = s_timerStats[m_strName].AvgTime();
    }
    
    if (access("Timer", F_OK) == 0)
    {
        std::cout << "[Timer] " << m_strName << ": Current=" 
                << std::fixed << std::setprecision(2) << elapsed_ms << "ms | Avg=" 
                << avg_time << "ms" << std::endl;
    }
}

/* 输出统计数据 */
void CStatisticsTimer::PrintAllStats()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    
    dlog_info("ai_app: ---------------------- [Timer] Statistics ----------------------");
    dlog_info("ai_app: %-20s %8s %10s %10s %10s", "Name", "Count", "Min(ms)", "Avg(ms)", "Max(ms)");
    
    for (const auto& [name, stats] : s_timerStats)
    {
        dlog_info("ai_app: %-20s %8zu %10.2f %10.2f %10.2f", 
                name.c_str(), stats.count, stats.min_time, stats.AvgTime(), stats.max_time);
    }
}

/* 清除统计数据 */
void CStatisticsTimer::ClearAllStats()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_timerStats.clear();
}
