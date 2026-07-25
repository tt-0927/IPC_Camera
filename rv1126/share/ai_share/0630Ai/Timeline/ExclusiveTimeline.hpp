#pragma once
#include <algorithm>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Ai0630_NS
{
    struct TimeSegment_S
    {
        long long nStartTime = 0;
        long long nEndTime   = 0;
    };

    template<typename T>
    class ExclusiveTimeline
    {
    public:

        explicit ExclusiveTimeline(long long mergeThresholdMs = 500)
            : m_nMergeThreshold(mergeThresholdMs)
        {
        }

        /**
         * @brief 单帧添加一个互斥状态
         * @note 同一时间只允许一个状态
         */
        void addState(long long nClassTime, T state)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_statesMap[nClassTime] = state;
        }

        /**
         * @brief 统计时间段 / 总时长 / 出现次数
         */
        void finalize()
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_statesMap.empty())
            {
                return;
            }

            /* 1. 转成 vector 并按时间排序 */
            std::vector<std::pair<long long, T>> vecData(
                m_statesMap.begin(), m_statesMap.end());

            std::sort(vecData.begin(), vecData.end(),
                      [](const auto& a, const auto& b) {
                return a.first < b.first;
            });

            /* 2. 遍历生成统计结果 */
            for (size_t i = 0; i < vecData.size(); ++i)
            {
                long long nCur  = vecData[i].first;
                long long nNext = (i + 1 < vecData.size()) ? vecData[i + 1].first : nCur;

                T st = vecData[i].second;

                long long nDuration = std::max(0LL, nNext - nCur);

                auto& info = m_statsMap[st];

                /* ---------- 新增：次数统计 ---------- */
                info.nCount++;

                /* ---------- 原有：时长统计 ---------- */
                info.nTotalDurations += nDuration;

                /* ---------- 原有：时间段合并 ---------- */
                if (!info.vecSegments.empty() &&
                    m_nMergeThreshold >= 0 &&
                    nCur - info.vecSegments.back().nEndTime <= m_nMergeThreshold)
                {
                    info.vecSegments.back().nEndTime = nNext;
                }
                else
                {
                    info.vecSegments.push_back({ nCur, nNext });
                }
            }
        }

        struct StateStat
        {
            long long                  nTotalDurations = 0; /* 总时长 */
            int                        nCount          = 0; /* ⭐ 出现次数 */
            std::vector<TimeSegment_S> vecSegments;         /* 时间段 */
        };

        /**
         * @brief 获取所有状态统计信息
         */
        const std::unordered_map<T, StateStat>& getStats() const
        {
            return m_statsMap;
        }

        /**
         * @brief 清空
         */
        void reset()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_statesMap.clear();
            m_statsMap.clear();
        }

    private:

        mutable std::mutex m_mutex;

        /* 单帧互斥状态 */
        std::unordered_map<long long, T> m_statesMap;

        /* 统计结果 */
        std::unordered_map<T, StateStat> m_statsMap;

        long long m_nMergeThreshold;
    };

}    // namespace Ai0630_NS
