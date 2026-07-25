#pragma once

#include <vector>

namespace Ai0630_NS
{
    /* ---------------- 时间段结构 ---------------- */
    struct Segment_S
    {
        long long nStartTime = 0; /* 秒 */
        long long nEndTime   = 0;
    };

    struct ClassParamParam_S
    {
        /* ---- 课堂整体指标 ---- */
        int       nClassTime        = 0;
        int       nClassScore       = 0;
        long long nTeachDuration    = 0; /* 教师讲授时长 */
        long long nInteractDuration = 0; /* 师生互动时长 */
        long long nGuideDuration    = 0; /* 指导学生时长 */
        long long nWalkDuration     = 0; /* 巡视时长 */

        long long nHeadDownCount = 0;    /* 全班低头次数 */
        long long nBehaviorCount = 0;    /* 全班行为次数 */

        long long nJoyCount      = 0;    /* 全班快乐次数 */
        long long nSurpriseCount = 0;    /* 全班惊喜次数 */
        long long nAngerCount    = 0;    /* 全班愤怒次数 */
        long long nDisgustCount  = 0;    /* 全班厌恶次数 */
        long long nFearCount     = 0;    /* 全班恐惧次数 */
        long long nSadnessCount  = 0;    /* 全班悲伤次数 */
        long long nNeutralCount  = 0;    /* 全班中性次数 */
        long long nEmoCount      = 0;    /* 全班总表情次数 */

        /* ---- 学生行为 / 情绪最长段 ---- */
        Segment_S stLongestFocus;
        Segment_S stLongestDistract;
        Segment_S stLongestExcited;
        Segment_S stLongestLow;

        std::vector<Segment_S> vecQuizTime;        /* 指导学生（提问）时间段 */
        std::vector<Segment_S> vecInteractionTime; /* 互动时间段 */


        int nMaxFocus = 0; /* 最大专注度 */
        int nMinFocus = 0; /* 最小专注度 */
    };

    template<typename MapT, typename KeyT>
    auto getOrDefault(
        const MapT&                       map,
        const KeyT&                       key,
        const typename MapT::mapped_type& def = {})
    {
        auto it = map.find(key);
        return (it != map.end()) ? it->second : def;
    }

}    // namespace Ai0630_NS