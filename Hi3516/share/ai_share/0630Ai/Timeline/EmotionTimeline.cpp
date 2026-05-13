#include "EmotionTimeline.hpp"

#include <sstream>

/* 添加情绪 */
void EmotionTimeline::addEmotion(long long nClassTime, Emotion_E enEmotion)
{
    m_points.push_back({ nClassTime, enEmotion });
}

/* 核心统计 */
EmotionTimelineResult_S EmotionTimeline::finalize()
{
    EmotionTimelineResult_S res;

    if (m_points.empty())
    {
        return res;
    }

    /* 1. 按时间排序 */
    std::sort(m_points.begin(), m_points.end(),
              [](const EmotionPoint_S& a, const EmotionPoint_S& b) {
        return a.nClassTime < b.nClassTime;
    });

    res.vecCurve = m_points;

    /* 2. 统计每种情绪数量 */
    for (const auto& pt : m_points)
    {
        res.mapEmotionCount[pt.enEmotion]++;
    }

    /* 3. 扫描最长兴奋 / 低落时间段 */
    EmotionSegment_S curExcited, curLow;

    for (size_t i = 0; i < m_points.size(); ++i)
    {
        const auto& cur = m_points[i];
        long long   nextTime =
            (i + 1 < m_points.size()) ? m_points[i + 1].nClassTime : cur.nClassTime;

        /* ---------- 兴奋 ---------- */
        if (isExcited(cur.enEmotion))
        {
            if (curExcited.nStartTime == 0)
            {
                curExcited.nStartTime = cur.nClassTime;
            }
            curExcited.nEndTime = nextTime;
        }
        else
        {
            if (curExcited.duration() > res.stLongestExcited.duration())
            {
                res.stLongestExcited = curExcited;
            }
            curExcited = {};
        }

        /* ---------- 低落 ---------- */
        if (isLow(cur.enEmotion))
        {
            if (curLow.nStartTime == 0)
            {
                curLow.nStartTime = cur.nClassTime;
            }
            curLow.nEndTime = nextTime;
        }
        else
        {
            if (curLow.duration() > res.stLongestLow.duration())
            {
                res.stLongestLow = curLow;
            }
            curLow = {};
        }
    }

    /* 处理结尾 */
    if (curExcited.duration() > res.stLongestExcited.duration())
    {
        res.stLongestExcited = curExcited;
    }

    if (curLow.duration() > res.stLongestLow.duration())
    {
        res.stLongestLow = curLow;
    }

    return res;
}

/* 清空 */
void EmotionTimeline::reset()
{
    m_points.clear();
}

std::string EmotionTimeline::toString() const
{
    std::ostringstream oss;

    oss << "================ EmotionTimeline ================\n";
    oss << "情绪打点数量: " << m_points.size() << "\n";

    for (const auto& p : m_points)
    {
        oss << "  t=" << p.nClassTime
            << ", emotion=" << static_cast<int>(p.enEmotion)
            << "\n";
    }

    oss << "=================================================\n";

    return oss.str();
}

void EmotionTimeline::print(
    const std::function<void(const std::string&)>& printer) const
{
    if (printer)
    {
        printer(toString());
    }
}

/* 是否为兴奋情绪 */
bool EmotionTimeline::isExcited(Emotion_E e) const
{
    return (e == Emotion_E::JOY ||
            e == Emotion_E::SURPRISE);
}

/* 是否为低落情绪 */
bool EmotionTimeline::isLow(Emotion_E e) const
{
    return (e == Emotion_E::ANGER ||
            e == Emotion_E::DISGUST ||
            e == Emotion_E::FEAR ||
            e == Emotion_E::SADNESS);
}
