/**
 * @FilePath     : event_manager.hpp
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-06-06 17:00:45
 * @Description  : 事件管理窗口, 用于过滤重复事件
 */

#pragma once

#include <unordered_map>
#include <tuple>
#include <chrono>
#include <mutex>
#include <iostream>

/* 自定义哈希函数 std::tuple<int, int> */
struct tuple_hash
{
    template <class T1, class T2>
    std::size_t operator()(const std::tuple<T1, T2>& t) const
    {
        auto h1 = std::hash<T1>{}(std::get<0>(t));
        auto h2 = std::hash<T2>{}(std::get<1>(t));
        return h1 ^ (h2 << 1);
    }
};

/* 事件管理类 */
class EventManager
{
private:
    /* 事件存储结构：键为 (channel, eventType), 值为时间戳 */
    std::unordered_map<std::tuple<int, int>, std::chrono::steady_clock::time_point, tuple_hash> events;
    std::mutex mtx;
    long timeWindow;

    // 内部无锁版本，由已持有锁的方法调用
    bool shouldFilterEventUnlocked(int channel, int eventType)
    {
        auto now = std::chrono::steady_clock::now();
        auto key = std::make_tuple(channel, eventType);

        auto it = events.find(key);

        if (it != events.end())
        {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
            if (duration <= timeWindow)
            {
                return true;
            }
            /* 如果发现过期事件，直接移除 */
            events.erase(it);
        }

        return false;
    }

public:
    /* 默认构造函数, 默认时间窗口 20 秒 */
    EventManager() : timeWindow(20 * 1000) {}

    void setTimeWindow(long newTimeWindow)
    {
        std::lock_guard<std::mutex> lock(mtx);
        timeWindow = newTimeWindow;
    }

    /* 带参数的构造函数, 自定义时间窗口 */
    EventManager(long timeWindow) : timeWindow(timeWindow) {}

    /* 判断事件是否需要过滤 */
    bool shouldFilterEvent(int channel, int eventType = 0)
    {
        std::lock_guard<std::mutex> lock(mtx);
        return shouldFilterEventUnlocked(channel, eventType);
    }

    /* 处理事件 */
    bool handleEvent(int channel, int eventType = 0)
    {
        auto key = std::make_tuple(channel, eventType);

        std::lock_guard<std::mutex> lock(mtx);

        /* 判断事件是否需要过滤 */
        if (shouldFilterEventUnlocked(channel, eventType))
        {
            return false;
        }

        /* 新事件, 添加到事件列表 */
        events[key] = std::chrono::steady_clock::now();
        return true;
    }
};
