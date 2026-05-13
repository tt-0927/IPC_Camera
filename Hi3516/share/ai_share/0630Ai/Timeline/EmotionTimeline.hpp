#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


/* ------------------------- 情绪枚举 ------------------------- */
enum class Emotion_E
{
    ANGER = 0, /* 愤怒 */
    DISGUST,   /* 厌恶 */
    FEAR,      /* 恐惧 */
    JOY,       /* 快乐 */
    NEUTRAL,   /* 中性 */
    SADNESS,   /* 悲伤 */
    SURPRISE   /* 惊喜 */
};

/* ------------------------- 时间点 ------------------------- */
struct EmotionPoint_S
{
    long long nClassTime = 0;
    Emotion_E enEmotion  = Emotion_E::NEUTRAL;
};

/* ------------------------- 时间段 ------------------------- */
struct EmotionSegment_S
{
    long long nStartTime = 0;
    long long nEndTime   = 0;

    long long duration() const
    {
        return nEndTime - nStartTime;
    }
};

/* ------------------------- 结果结构 ------------------------- */
struct EmotionTimelineResult_S
{
    /* 情绪变化曲线（按时间排序） */
    std::vector<EmotionPoint_S> vecCurve;

    /* 每种情绪的出现次数 */
    std::unordered_map<Emotion_E, int> mapEmotionCount;

    /* 最长兴奋段（JOY / SURPRISE） */
    EmotionSegment_S stLongestExcited;

    /* 最长低落段（ANGER / DISGUST / FEAR / SADNESS） */
    EmotionSegment_S stLongestLow;
};

/* ============================================================
 *                      EmotionTimeline
 * ============================================================ */
class EmotionTimeline
{
public:

    /* 添加单个情绪点（允许乱序） */
    void addEmotion(long long nClassTime, Emotion_E enEmotion);

    /* 计算所有统计结果 */
    EmotionTimelineResult_S finalize();

    /* 清空 */
    void reset();

    /* 打印情绪时间线 */
    std::string toString() const;

    void print(const std::function<void(const std::string&)>& printer) const;

private:

    bool isExcited(Emotion_E e) const;
    bool isLow(Emotion_E e) const;

private:

    std::vector<EmotionPoint_S> m_points;
};
