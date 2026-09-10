/**
 * @FilePath     : time_utils.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-09 14:31:20
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 11:31:42
 * @Description  : 时间工具
 */

#pragma once

#include <ctime>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <time.h>

/**
 * @brief   : 时间工具
 */
namespace TimeUtils_NS
{
    /* 检测系统时间是否有突变 */
    typedef struct TimeJumpCheck 
    {
        static constexpr double THRESHOLD_S   = 10.0;   /* 判断突变的阈值，单位:s */
        static constexpr int    NEEDED_IN_A_ROW = 1;    /* 连续突变次数确认 */ 

        timespec last_real  {};
        timespec last_mono  {};
        int suspicious = 0;

        void init() 
        {
            clock_gettime(CLOCK_REALTIME, &last_real);
            clock_gettime(CLOCK_MONOTONIC, &last_mono);
            suspicious = 0;
        }

        /* 返回 true：确认时间跳变 */
        double probe() 
        {
            timespec now_real{}, now_mono{};
            clock_gettime(CLOCK_REALTIME, &now_real);
            clock_gettime(CLOCK_MONOTONIC, &now_mono);

            double elapsed_real = (now_real.tv_sec  - last_real.tv_sec ) +
                                (now_real.tv_nsec - last_real.tv_nsec) / 1e9;
            double elapsed_mono = (now_mono.tv_sec  - last_mono.tv_sec ) +
                                (now_mono.tv_nsec - last_mono.tv_nsec) / 1e9;

            last_real = now_real;
            last_mono = now_mono;

            if (elapsed_mono > THRESHOLD_S) 
            {
                /* 自己被调度卡住了，不算跳变 */ 
                suspicious = 0;
                return 0.0;
            }

            if (std::fabs(elapsed_real - elapsed_mono) > THRESHOLD_S) 
            {
                ++suspicious;
                if (suspicious >= NEEDED_IN_A_ROW) 
                {
                    suspicious = 0;
                    // printf(" ======== 时间发生跳变 %lf ======== \n", (elapsed_real - elapsed_mono));
                    return elapsed_real - elapsed_mono;
                }
            } 
            else 
            {
                suspicious = 0;
            }
            return 0.0;
        }
    }TimeJumpCheck_S;

    /**
     * @brief   : 获取当前的星期几
     * @return   {int} 星期几 (1-7，1表示星期一，7表示星期日)
     */
    int getTodayDayOfWeek();

    /**
     * @brief   : 获取当前本地月份
     * @return   {int} 当前月份，取值 1-12
     * @note    : 基于本地时区计算，适用于按自然月组织的计划与配置
     */
    int get_today_month();

    /**
     * @brief   : 获取自当天开始的秒数
     * @return   {int} 自当天开始的秒数
     */
    int getSecondsSinceStartOfDay();

    /**
     * @brief   : 获取当前毫秒级别时间戳
     * @return   {long long} 毫秒级别时间戳
     */
    long long get_currentTimestampMs();

    /**
     * @brief   : 获取单调递增毫秒计数
     * @return  {long long} 单调递增毫秒计数
     * @note    : 仅用于计算时间间隔，不受系统校时、NTP、时区切换影响，不能用于日期展示或文件命名
     */
    long long get_monotonicTimestampMs();

    /**
     * @brief   : 获取当前秒级别时间戳
     * @return   {long long} 秒级别时间戳
     */
    long long get_currentTimestampS();

    /**
     * @brief   : 获取当前日期
     * @return   {std::string} YYYYMMDD 格式 年月日
     */
    std::string get_currentDate();

    /**
     * @brief   : 获取当前日期
     * @return   {std::string} YYYY-MM-DD 格式 年月日
     */
    std::string get_currentDateWithDash();

    /**
     * @brief   : 获取当前时间
     * @return   {std::string} %H%M%S 格式:(时分秒)
     */
    std::string get_currentTime();

    /**
     * @brief   : 获取当前时间（毫秒级）
     * @return   {std::string} %H%M%S%3d 格式:(时分秒毫秒) 例如: 143025123
     */
    std::string get_currentTimeMs();

    /**
     * @brief   : 获取当前时间
     * @return   {std::string} %H:%M:%S 格式:(时:分:秒)
     */
    std::string get_currentTimeWithColon();

    /**
     * @brief   : 获取当前时间指定的字符串输出格式
     * @return   {std::string} %H:%M:%S 格式:(时:分:秒)
     */
    std::string get_currentTimeAndFormat(const char *strTimeFormat);

    /**
     * @brief   : 获取当前日期指定的字符串输出格式
     * @return   {std::string} %Y-%m-%d 格式:(年-月-日)
     */
    std::string get_currentDateAndFormat(const char *strTimeFormat);
    
    /**
     * @brief       : 获取当前日期与时间
     * @return       {std::string} YYYY-MM-DD HH:MM:SS 格式:(年-月-日 时:分:秒)
     * @note        : 示例：2019-08-06 05:31:39
     */
    std::string get_currentDateAndTimeNoT();

    /**
     * @brief       : 获取当前日期与时间
     * @return       {std::string} YYYY-MM-DDTHH:MM:SS 格式:(年-月-日T时:分:秒)
     * @note        : 示例：2019-08-06T05:31:39
     */
    std::string get_currentDateAndTime();

    /**
     * @brief   : 将毫秒级时间戳转换为日期字符串
     * @param   timestamp_ms 毫秒级时间戳
     * @return  {std::string} 日期字符串，格式: YYYY-MM-DD
     */
    std::string timestamp_to_date(long long timestamp_ms);

    /**
     * @brief   : 将毫秒级时间戳转换为时间字符串
     * @param   timestamp_ms 毫秒级时间戳
     * @return  {std::string} 时间字符串，格式: HH:MM:SS
     */
    std::string timestamp_to_time(long long timestamp_ms);

}  // namespace TimeUtils_NS
