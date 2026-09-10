/**
 * @FilePath     : time_tools.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-30 09:54:03
 * @Description  : 时间工具
 */

#pragma once

#include <chrono>
#include <string>

class Time {
public:
    /** 获取当前日期，格式为YYYYMMDD */
    static std::string get_yyyymmdd();
    /** 获取当前时间，格式为HHMMSS */
    static std::string get_hhmmss();
    /** 获取当前日期和时间，格式为YYYYMMDD_HHMMSS */
    static std::string get_curTime();
    /** 获取当前时间的秒数 */
    static std::time_t get_seconds();
    /** 获取当前时间的毫秒数 */
    static std::time_t get_milliseconds();
};