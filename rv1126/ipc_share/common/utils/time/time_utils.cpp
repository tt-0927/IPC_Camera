/**
 * @FilePath     : time_utils.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-09 14:31:17
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 11:32:08
 * @Description  : 时间工具
 */

#include "time_utils.h"

namespace TimeUtils_NS
{
    /**
     * @brief   : 获取当前的星期几
     * @return   {int} 星期几 (1-7，1表示星期一，7表示星期日)
     */
    int getTodayDayOfWeek()
    {
        /* 获取当前时间点 */
        auto now = std::chrono::system_clock::now();

        /* 转换为 time_t */
        std::time_t t = std::chrono::system_clock::to_time_t(now);

        /* 转换为 tm 结构 */
        std::tm localTime;
        /* 使用线程安全的 localtime_r */
        localtime_r(&t, &localTime);

        /* tm_wday 返回 0 表示星期天，1 表示星期一，依此类推 */
        return (localTime.tm_wday == 0) ? 7 : localTime.tm_wday;
    }

    /**
     * @brief   : 获取当前本地月份
     * @return   {int} 当前月份，取值 1-12
     * @note    : 使用 localtime_r 避免多个业务线程同时读取本地日期时竞争静态缓冲区
     */
    int get_today_month()
    {
        /* 当前时间快照只用于取得本地日历月份，避免跨月瞬间重复取时。 */
        const auto stNow = std::chrono::system_clock::now();
        const std::time_t stTime = std::chrono::system_clock::to_time_t(stNow);
        std::tm stLocalTime;
        localtime_r(&stTime, &stLocalTime);
        return stLocalTime.tm_mon + 1;
    }

    /**
     * @brief   : 获取自当天开始的秒数
     * @return   {int} 自当天开始的秒数
     */
    int getSecondsSinceStartOfDay()
    {
        /* 获取当前时间点 */
        auto now = std::chrono::system_clock::now();

        /* 转换为 time_t */
        std::time_t t = std::chrono::system_clock::to_time_t(now);

        /* 转换为 tm 结构 */
        std::tm localTime;
        localtime_r(&t, &localTime); // 线程安全版本

        /* 计算今天到现在的秒数 */
        int secondsSinceStartOfDay = localTime.tm_hour * 3600 + localTime.tm_min * 60 + localTime.tm_sec;
        return secondsSinceStartOfDay;
    }

    /**
     * @brief   : 获取当前毫秒级别时间戳
     * @return   {long long} 毫秒级别时间戳
     */
    long long get_currentTimestampMs()
    {
        // 获取当前系统时间点
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        return now_ms;
    }

    /**
     * @brief   : 获取单调递增毫秒计数
     * @return  {long long} 单调递增毫秒计数
     * @note    : 用于录制时长、音视频同步等间隔计算，避免系统时间跳变导致时间轴回退
     */
    long long get_monotonicTimestampMs()
    {
        struct timespec stTime;
        if (clock_gettime(CLOCK_MONOTONIC, &stTime) != 0)

        {
            return 0;
        }

        return static_cast<long long>(stTime.tv_sec) * 1000 + stTime.tv_nsec / 1000000;
    }

    /**
     * @brief   : 获取当前秒级别时间戳
     * @return   {long long} 秒级别时间戳
     */
    long long get_currentTimestampS()
    {
        // 获取当前系统时间点
        auto now = std::chrono::system_clock::now();
        auto now_s = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        return now_s;
    }

    /**
     * @brief   : 获取当前日期
     * @return   {std::string} YYYYMMDD 格式 年月日
     */
    std::string get_currentDate()
    {
        /* 获取当前时间点 */
        auto now = std::chrono::system_clock::now();
        /* 转换为时间类型 */
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

        /* 格式化为 YYYYMMDD 格式的字符串 */
        char buffer[9]; /*  "YYYYMMDD" 长度为 8 字符，+1 用于 '\0' */
        std::strftime(buffer, sizeof(buffer), "%Y%m%d", std::localtime(&in_time_t));
        return std::string(buffer);
    }

    /**
     * @brief   : 获取当前日期
     * @return   {std::string} YYYY-MM-DD 格式 年月日
     */
    std::string get_currentDateWithDash()
    {
        /* 获取当前时间点 */
        auto now = std::chrono::system_clock::now();
        /* 转换为时间类型 */
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

        /* 格式化为 YYYY-MM-DD 格式的字符串 */
        char buffer[11]; /*  "YYYY-MM-DD" 长度为 10 字符，+1 用于 '\0' */
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&in_time_t));
        return std::string(buffer);
    }

    /**
     * @brief   : 获取当前时间
     * @return   {std::string} %H%M%S 格式:(时分秒)
     */
    std::string get_currentTime()
    {
        /* 获取当前时间点 */
        auto now = std::chrono::system_clock::now();
        /* 转换为时间类型 */
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

        /* 格式化为 %H%M%S 格式的字符串 */
        char buffer[7]; /*  "%H%M%S" 长度为 6 字符，+1 用于 '\0' */
        std::strftime(buffer, sizeof(buffer), "%H%M%S", std::localtime(&in_time_t));
        return std::string(buffer);
    }

    /**
     * @brief   : 获取当前时间（毫秒级）
     * @return   {std::string} %H%M%S%3d 格式:(时分秒毫秒) 例如: 143025123
     */
    std::string get_currentTimeMs()
    {
        /* 获取当前时间点 */
        auto now = std::chrono::system_clock::now();

        /* 转换为时间类型 */
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

        /* 获取毫秒部分 */
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        /* 格式化为 %H%M%S 格式的字符串 */
        char buffer[13]; /* "HHMMSSmmm\0" 长度为 9 字符，+1 用于 '\0' */
        std::strftime(buffer, 7, "%H%M%S", std::localtime(&in_time_t));

        /* 添加毫秒部分 */
        snprintf(buffer + 6, 4, "%03d", static_cast<int>(ms.count()));

        return std::string(buffer);
    }

    /**
     * @brief   : 获取当前时间
     * @return   {std::string} %H:%M:%S 格式:(时:分:秒)
     */
    std::string get_currentTimeWithColon()
    {
        /* 获取当前时间点 */
        auto now = std::chrono::system_clock::now();
        /* 转换为时间类型 */
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

        /* 格式化为 %H:%M:%S 格式的字符串 */
        char buffer[11]; /*  "%H:%M:%S" 长度为 8 字符，+1 用于 '\0' */
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&in_time_t));
        return std::string(buffer);
    }

    /**
     * @brief   : 获取当前时间指定的字符串输出格式
     * @return   {std::string} %H:%M:%S 格式:(时:分:秒)
     */
    std::string get_currentTimeAndFormat(const char *strTimeFormat)
    {

        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm ltm = *std::localtime(&now_c);

        std::ostringstream oss;
        oss << std::put_time(&ltm, strTimeFormat);
        return oss.str();
    }
    
    /**
     * @brief   : 获取当前日期指定的字符串输出格式
     * @return   {std::string} %Y-%m-%d 格式:(年-月-日)
     */
    std::string get_currentDateAndFormat(const char *strTimeFormat)
    {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm ltm = *std::localtime(&now_c);

        std::ostringstream oss;
        oss << std::put_time(&ltm, strTimeFormat);
        return oss.str();
    }

    /**
     * @brief       : 获取当前日期与时间
     * @return       {std::string} YYYY-MM-DD HH:MM:SS 格式:(年-月-日 时:分:秒)
     * @note        : 示例：2019-08-06 05:31:39
     */
    std::string get_currentDateAndTimeNoT()
    {
        /* 获取当前时间点 */
        auto now = std::chrono::system_clock::now();
        /* 转换为时间类型 */
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

        /* 格式化为 YYYY-MM-DDTHH:MM:SS 格式的字符串 */
        char buffer[20];              /*  "YYYY-MM-DD HH:MM:SS" 长度为 19 字符，+1 用于 '\0' */
        struct tm tm;                 /* 栈上的结构体，避免静态缓冲区竞争 */
        localtime_r(&in_time_t, &tm); /* 线程安全的本地时间解析 */
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
        return std::string(buffer);
    }

    /**
     * @brief       : 获取当前日期与时间
     * @return       {std::string} YYYY-MM-DDTHH:MM:SS 格式:(年-月-日T时:分:秒)
     * @note        : 示例：2019-08-06T05:31:39
     */
    std::string get_currentDateAndTime()
    {
        /* 获取当前时间点 */
        auto now = std::chrono::system_clock::now();
        /* 转换为时间类型 */
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

        /* 格式化为 YYYY-MM-DDTHH:MM:SS 格式的字符串 */
        char buffer[20];              /*  "YYYY-MM-DDTHH:MM:SS" 长度为 19 字符，+1 用于 '\0' */
        struct tm tm;                 /* 栈上的结构体，避免静态缓冲区竞争 */
        localtime_r(&in_time_t, &tm); /* 线程安全的本地时间解析 */
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);
        return std::string(buffer);
    }

    /**
     * @brief   : 将毫秒级时间戳转换为日期字符串
     * @param   timestamp_ms 毫秒级时间戳
     * @return  {std::string} 日期字符串，格式: YYYY-MM-DD
     */
    std::string timestamp_to_date(long long timestamp_ms)
    {
        /* 将毫秒转换为秒 */
        auto timestamp_s = timestamp_ms / 1000;

        /* 转换为time_t类型 */
        std::time_t time = static_cast<std::time_t>(timestamp_s);

        /* 转换为本地时间 */
        std::tm *tm_info = std::localtime(&time);

        /* 格式化为YYYY-MM-DD */
        char buffer[11];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm_info);

        return std::string(buffer);
    }

    /**
     * @brief   : 将毫秒级时间戳转换为时间字符串
     * @param   timestamp_ms 毫秒级时间戳
     * @return  {std::string} 时间字符串，格式: HH:MM:SS
     */
    std::string timestamp_to_time(long long timestamp_ms)
    {
        /* 将毫秒转换为秒 */
        auto timestamp_s = timestamp_ms / 1000;

        /* 转换为time_t类型 */
        std::time_t time = static_cast<std::time_t>(timestamp_s);

        /* 转换为本地时间 */
        std::tm *tm_info = std::localtime(&time);

        /* 格式化为HH:MM:SS */
        char buffer[9];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", tm_info);

        return std::string(buffer);
    }

} // namespace TimeUtils_NS
