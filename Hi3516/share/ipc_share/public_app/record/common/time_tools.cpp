/**
 * @FilePath     : time_tools.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-30 09:55:52
 * @Description  : 时间工具
 */

#include "time_tools.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string Time::get_yyyymmdd()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm ltm = *std::localtime(&now_c);

    std::ostringstream oss;
    oss << std::put_time(&ltm, "%Y%m%d");
    return oss.str();
}

std::string Time::get_hhmmss()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm ltm = *std::localtime(&now_c);

    std::ostringstream oss;
    oss << std::put_time(&ltm, "%H%M%S");
    return oss.str();
}

std::string Time::get_curTime()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm ltm = *std::localtime(&now_c);

    std::ostringstream oss;
    oss << std::put_time(&ltm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::time_t Time::get_seconds()
{
    auto now = std::chrono::system_clock::now();
    auto now_s = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return now_s;
}

std::time_t Time::get_milliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return now_ms;
}
