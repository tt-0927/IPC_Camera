/**
 * @FilePath     : statistics_timer.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-06 17:08:40
 * @Description  : 统计代码运行耗时
 */

#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>
#include <iomanip>
#include <iostream>
#include <algorithm>

class CStatisticsTimer
{
public:
    struct TimerStats
    {
        double total_time = 0.0;   /* 总耗时（ms） */
        size_t count = 0;          /* 调用次数 */
        double max_time = 0.0;     /* 最大单次耗时 */
        double min_time = 0.0;     /* 最小单次耗时 */

        double AvgTime() const
        {
            return count > 0 ? total_time / count : 0.0;
        }
    };

    explicit CStatisticsTimer(const std::string& strName);
    ~CStatisticsTimer();

    void PrintCurrent(double elapsed_ms) const;
    static void PrintAllStats();
    static void ClearAllStats();

private:
    std::string m_strName;
    std::chrono::high_resolution_clock::time_point m_startTime;

    static std::unordered_map<std::string, TimerStats> s_timerStats;
    static std::mutex s_mutex;
};
