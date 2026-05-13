/**
 * @FilePath     : execution_timer.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-13 15:31:22
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 17:05:26
 * @Description  : 用于计算代码块运行时间的类
 */

#pragma once

#include <chrono>
#include <iostream>
#include <string>

class CExecutionTimer
{
public:

    /* 构造函数，传入计时器名称 */
    explicit CExecutionTimer(const std::string& strName, int nMaxMs = 0)
        : m_strName(strName),
          m_nMaxMs(nMaxMs),
          m_startTime(std::chrono::high_resolution_clock::now())
    {
        /* 记录开始时间 */
    }

    /* 析构函数，自动计算并输出运行时间 */
    ~CExecutionTimer()
    {
        /* 记录结束时间 */
        auto end_time = std::chrono::high_resolution_clock::now();

        /* 计算运行时间，单位为毫秒 */
        std::chrono::duration<double, std::milli> elapsed = end_time - m_startTime;

        /* 输出运行时间 */
        if (elapsed.count() >= m_nMaxMs)
        {
            std::cout << "Execution time of " << m_strName << ": " << elapsed.count() << " ms" << std::endl;
        }
    }

private:

    std::string m_strName;                                      /* 计时器的名称 */
    int         m_nMaxMs = 0;

    std::chrono::high_resolution_clock::time_point m_startTime; /* 记录开始时间的时间点 */
};