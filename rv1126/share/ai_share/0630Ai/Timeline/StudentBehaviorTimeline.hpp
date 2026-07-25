#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


/* ============================================================
 *                    学生行为枚举
 * ============================================================ */
/*
 * 学生在某一时间点的互斥行为
 */
enum class StudentBehavior_E
{
    ACTION_NULL = -1, /* 未识别到 */
    LISTEN      = 0,  /* 听讲（专注） */
    PRACTICE,         /* 实践（专注） */
    DEMO,             /* 演示（专注） */
    READ,             /* 阅读（涣散） */
    DISCUSS,          /* 讨论（涣散） */
    DOWN_DESK,        /* 趴桌（涣散） */
};

/* ============================================================
 *                    行为时间点
 * ============================================================ */
/*
 * 单次行为识别结果
 */
struct BehaviorPoint_S
{
    long long         nClassTime = 0; /* 课堂时间 */
    StudentBehavior_E enBehavior = StudentBehavior_E::ACTION_NULL;
};

/* ============================================================
 *                    行为时间段
 * ============================================================ */
/*
 * 连续同一种行为形成的时间段
 */
struct BehaviorSegment_S
{
    long long nStartTime = 0;
    long long nEndTime   = 0;

    long long duration() const
    {
        return nEndTime - nStartTime;
    }
};

/* ============================================================
 *                    专注度曲线点
 * ============================================================ */
/*
 * 基于“行为数量窗口”的专注度统计结果
 */
struct FocusPoint_S
{
    long long nStartTime = 0; /* 窗口起始时间 */
    long long nEndTime   = 0; /* 窗口结束时间 */
    float     fFocus     = 0; /* 专注度（0~100） */
};

/* ============================================================
 *                    统计结果
 * ============================================================ */
struct StudentBehaviorResult_S
{
    /* 1. 专注度变化曲线（按 30 条行为数据分段） */
    std::vector<FocusPoint_S> vecFocusCurve;

    /* 2. 每种行为出现次数 */
    std::unordered_map<StudentBehavior_E, int> mapBehaviorCount;

    /* 3. 每种行为的连续时间段 */
    std::unordered_map<StudentBehavior_E,
                       std::vector<BehaviorSegment_S>>
        mapBehaviorSegments;

    /* 每种行为的总时长 */
    std::unordered_map<StudentBehavior_E, long long> mapBehaviorTotalDuration;


    /* 4. 最长连续专注时间段 */
    BehaviorSegment_S stLongestFocus;

    /* 5. 最长连续涣散时间段 */
    BehaviorSegment_S stLongestDistract;

    /* 6. 专注度统计（%） */
    float fMaxFocus = 0.0f;
    float fMinFocus = 0.0f;
    float fAvgFocus = 0.0f;

    /* 7. 注意力集中 / 涣散占比（%） */
    float fFocusRatioPercent    = 0.0f; /* 专注度 >= 70 */
    float fDistractRatioPercent = 0.0f; /* 专注度 <= 40 */
};

/* ============================================================
 *                StudentBehaviorTimeline
 * ============================================================ */
/*
 * 单学生行为与专注分析时间线
 */
class StudentBehaviorTimeline
{
public:

    /* 添加行为打点（允许乱序） */
    void addBehavior(long long nClassTime, StudentBehavior_E enBehavior);

    /* 统计并生成结果 */
    StudentBehaviorResult_S finalize();

    /* 清空 */
    void reset();

    /* 打印统计信息（返回字符串） */
    std::string toString() const;

    /*
     * @brief 打印接口（由外部决定如何输出）
     * @param printer 例如：[](const std::string& s){ printf("%s\n", s.c_str()); }
     */
    void print(const std::function<void(const std::string&)>& printer) const;

    static const std::vector<BehaviorSegment_S>&
        getBehaviorSegmentsSafe(
            const std::unordered_map<StudentBehavior_E,
                                     std::vector<BehaviorSegment_S>>& map,
            StudentBehavior_E                                         e)
    {
        static const std::vector<BehaviorSegment_S> kEmpty;
        auto                                        it = map.find(e);
        return (it != map.end()) ? it->second : kEmpty;
    }

private:

    /* 专注 / 涣散判断 */
    bool isFocusBehavior(StudentBehavior_E e) const;
    bool isDistractBehavior(StudentBehavior_E e) const;

    /* 构建专注度曲线（30 条行为为一窗，不足用实际数量） */
    void buildFocusCurveByCount(std::vector<FocusPoint_S>& outCurve) const;

private:

    std::vector<BehaviorPoint_S> m_points;
};
