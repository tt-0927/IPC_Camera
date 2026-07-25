#include "StudentBehaviorTimeline.hpp"

#include <sstream>

/* ============================================================
 *                    行为添加
 * ============================================================ */
void StudentBehaviorTimeline::addBehavior(
    long long         nClassTime,
    StudentBehavior_E enBehavior)
{
    m_points.push_back({ nClassTime, enBehavior });
}

/* ============================================================
 *                    核心统计
 * ============================================================ */
StudentBehaviorResult_S StudentBehaviorTimeline::finalize()
{
    StudentBehaviorResult_S res;

    if (m_points.size() < 2)
    {
        return res;
    }

    /* 1. 按时间排序 */
    std::sort(m_points.begin(), m_points.end(),
              [](const BehaviorPoint_S& a, const BehaviorPoint_S& b) {
        return a.nClassTime < b.nClassTime;
    });

    /* 2. 行为次数统计 */
    for (auto& p : m_points)
    {
        res.mapBehaviorCount[p.enBehavior]++;
    }

    /* 3. 行为时间段 + 最长专注 / 涣散 */
    BehaviorSegment_S                                        curFocusSeg {};
    BehaviorSegment_S                                        curDistractSeg {};
    std::unordered_map<StudentBehavior_E, BehaviorSegment_S> curSeg;

    for (size_t i = 0; i + 1 < m_points.size(); ++i)
    {
        const auto& cur  = m_points[i];
        const auto& next = m_points[i + 1];

        long long dt = next.nClassTime - cur.nClassTime;
        if (dt <= 0)
        {
            continue;
        }

        /* ---------- 每种行为的总时长 ---------- */
        res.mapBehaviorTotalDuration[cur.enBehavior] += dt;


        bool bFocus    = isFocusBehavior(cur.enBehavior);
        bool bDistract = isDistractBehavior(cur.enBehavior);

        /* 专注段 */
        if (bFocus)
        {
            if (curFocusSeg.nStartTime == 0)
            {
                curFocusSeg.nStartTime = cur.nClassTime;
            }
            curFocusSeg.nEndTime = next.nClassTime;
        }
        else
        {
            if (curFocusSeg.duration() > res.stLongestFocus.duration())
            {
                res.stLongestFocus = curFocusSeg;
            }
            curFocusSeg = {};
        }

        /* 涣散段 */
        if (bDistract)
        {
            if (curDistractSeg.nStartTime == 0)
            {
                curDistractSeg.nStartTime = cur.nClassTime;
            }
            curDistractSeg.nEndTime = next.nClassTime;
        }
        else
        {
            if (curDistractSeg.duration() > res.stLongestDistract.duration())
            {
                res.stLongestDistract = curDistractSeg;
            }
            curDistractSeg = {};
        }

        /* 行为段 */
        auto& seg = curSeg[cur.enBehavior];
        if (seg.nStartTime == 0)
        {
            seg.nStartTime = cur.nClassTime;
        }
        seg.nEndTime = next.nClassTime;

        if (m_points[i + 1].enBehavior != cur.enBehavior)
        {
            res.mapBehaviorSegments[cur.enBehavior].push_back(seg);
            curSeg[cur.enBehavior] = {};
        }
    }

    /* 收尾 */
    if (curFocusSeg.duration() > res.stLongestFocus.duration())
    {
        res.stLongestFocus = curFocusSeg;
    }
    if (curDistractSeg.duration() > res.stLongestDistract.duration())
    {
        res.stLongestDistract = curDistractSeg;
    }
    for (auto& kv : curSeg)
    {
        if (kv.second.duration() > 0)
        {
            res.mapBehaviorSegments[kv.first].push_back(kv.second);
        }
    }

    /* 4. 专注度曲线（30 条行为窗口） */
    buildFocusCurveByCount(res.vecFocusCurve);

    /* 5. 专注度统计 + 占比（按窗口） */
    if (!res.vecFocusCurve.empty())
    {
        float sum      = 0.0f;
        int   focusWin = 0;
        int   disWin   = 0;

        res.fMaxFocus = res.vecFocusCurve.front().fFocus;
        res.fMinFocus = res.vecFocusCurve.front().fFocus;

        for (const auto& p : res.vecFocusCurve)
        {
            sum           += p.fFocus;
            res.fMaxFocus  = std::max(res.fMaxFocus, p.fFocus);
            res.fMinFocus  = std::min(res.fMinFocus, p.fFocus);

            if (p.fFocus >= 70.0f)
            {
                focusWin++;
            }
            if (p.fFocus <= 40.0f)
            {
                disWin++;
            }
        }

        res.fAvgFocus = sum / res.vecFocusCurve.size();
        res.fFocusRatioPercent =
            100.0f * focusWin / res.vecFocusCurve.size();
        res.fDistractRatioPercent =
            100.0f * disWin / res.vecFocusCurve.size();
    }

    return res;
}

/* ============================================================
 *                    清空
 * ============================================================ */
void StudentBehaviorTimeline::reset()
{
    m_points.clear();
}

std::string StudentBehaviorTimeline::toString() const
{
    std::ostringstream oss;

    oss << "================ StudentBehaviorTimeline ================\n";
    oss << "行为打点数量: " << m_points.size() << "\n";

    for (const auto& p : m_points)
    {
        oss << "  t=" << p.nClassTime
            << ", behavior=" << static_cast<int>(p.enBehavior)
            << "\n";
    }

    oss << "=========================================================\n";

    return oss.str();
}

void StudentBehaviorTimeline::print(
    const std::function<void(const std::string&)>& printer) const
{
    if (printer)
    {
        printer(toString());
    }
}

/* ============================================================
 *                    行为分类判断
 * ============================================================ */
bool StudentBehaviorTimeline::isFocusBehavior(StudentBehavior_E e) const
{
    return (e == StudentBehavior_E::LISTEN ||
            e == StudentBehavior_E::PRACTICE ||
            e == StudentBehavior_E::DEMO);
}

bool StudentBehaviorTimeline::isDistractBehavior(StudentBehavior_E e) const
{
    return (e == StudentBehavior_E::READ ||
            e == StudentBehavior_E::DISCUSS ||
            e == StudentBehavior_E::DOWN_DESK);
}

/* ============================================================
 *        构建专注度曲线（按行为数量窗口）
 * ============================================================ */
/*
 * 规则：
 *  - 每 30 条行为数据为一个窗口
 *  - 不足 30 条也构成一个窗口
 *  - 专注度 = 专注行为数量 / 实际窗口数量 * 100
 */
void StudentBehaviorTimeline::buildFocusCurveByCount(
    std::vector<FocusPoint_S>& outCurve) const
{
    constexpr int WINDOW_SIZE = 30;

    for (size_t i = 0; i < m_points.size(); i += WINDOW_SIZE)
    {
        size_t startIdx = i;
        size_t endIdx   = std::min(i + WINDOW_SIZE, m_points.size());
        size_t winSize  = endIdx - startIdx;

        if (winSize == 0)
        {
            continue;
        }

        int nFocusCnt = 0;
        for (size_t j = startIdx; j < endIdx; ++j)
        {
            if (isFocusBehavior(m_points[j].enBehavior))
            {
                nFocusCnt++;
            }
        }

        FocusPoint_S pt;
        pt.nStartTime = m_points[startIdx].nClassTime;
        pt.nEndTime   = m_points[endIdx - 1].nClassTime;
        pt.fFocus     = 100.0f * nFocusCnt / winSize;

        outCurve.push_back(pt);
    }
}