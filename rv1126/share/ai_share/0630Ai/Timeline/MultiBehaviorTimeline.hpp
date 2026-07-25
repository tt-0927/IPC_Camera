#pragma once
#include <algorithm>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>


namespace Ai0630_NS
{
    /* 行为时间段结构体 */
    struct BehaviorSegment_S
    {
        long long nStartTime = 0; /* 开始时间 */
        long long nEndTime   = 0; /* 结束时间 */
    };

    /**
     * @brief MultiBehaviorTimeline（重写版）
     * 用途：用于“全班行为”（非互斥）的时间段统计
     * 特点：
     *   - 每种行为可以同时存在（不互斥）
     *   - 单帧只关心：该行为在这一帧“是否存在”（0/1）
     *   - 输入：addState(时间, 行为, 0/1)
     *   - 输出：每个行为的连续时间段
     */
    template<typename T>
    class MultiBehaviorTimeline
    {
    public:

        explicit MultiBehaviorTimeline(long long mergeMs = 2000)
            : m_mergeThreshold(mergeMs)
        {
        }

        /**
         * @brief 添加某时刻的行为存在状态
         * @param t        课堂时间（ms）
         * @param behavior 行为类型
         * @param exist    0=不存在，1=存在
         */
        void addState(long long t, T behavior, int exist)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_rawStates[behavior].push_back({ t, exist });
        }

        /**
         * @brief 根据 0/1 打点，生成连续时间段
         */
        void finalize()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_segments.clear();

            for (auto& kv : m_rawStates)
            {
                T     behavior = kv.first;
                auto& vec      = kv.second;
                if (vec.empty())
                {
                    continue;
                }

                /* 按时间排序 */
                std::sort(vec.begin(), vec.end(),
                          [](const auto& a, const auto& b) {
                    return a.first < b.first;
                });

                bool              inSeg = false;
                BehaviorSegment_S seg;

                for (size_t i = 0; i < vec.size(); ++i)
                {
                    long long t   = vec[i].first;
                    int       val = vec[i].second;

                    if (val == 1) /* 行为存在 */
                    {
                        if (!inSeg)
                        {
                            inSeg          = true;
                            seg.nStartTime = t;
                        }
                    }
                    else /* 行为不存在 */
                    {
                        if (inSeg)
                        {
                            seg.nEndTime = t;
                            m_segments[behavior].push_back(seg);
                            inSeg = false;
                        }
                    }
                }

                /* 结尾仍在段内 */
                if (inSeg)
                {
                    seg.nEndTime = vec.back().first;
                    m_segments[behavior].push_back(seg);
                }
            }
        }

        /**
         * @brief 获取所有行为的时间段结果
         * @return std::unordered_map<T, std::vector<BehaviorSegment_S>>&
         */
        const auto& getSegments() const
        {
            return m_segments;
        }

        /**
         * @brief 清空
         */
        void reset()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_rawStates.clear();
            m_segments.clear();
        }

    private:

        mutable std::mutex m_mutex;
        long long          m_mergeThreshold;

        /* 原始打点：行为 -> [(时间, 0/1), ...] */
        std::unordered_map<T, std::vector<std::pair<long long, int>>> m_rawStates;

        /* 输出：行为 -> 时间段列表 */
        std::unordered_map<T, std::vector<BehaviorSegment_S>> m_segments;
    };

}    // namespace Ai0630_NS
