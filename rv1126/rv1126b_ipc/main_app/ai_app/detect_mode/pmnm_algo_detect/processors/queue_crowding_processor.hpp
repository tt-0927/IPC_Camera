/*
 * @Description: 排队拥堵处理器声明（区域内多人IOU静止判定，纯判定不含跟踪）
 */

#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

#include "alarm_define.h"       /* Alarm::Region_S */
#include "event_define.h"       /* Event::Type */
#include "algorithm_define.hpp" /* TrackedTarget_S */

/* 排队拥堵判定规则 */
typedef struct _QueueCrowdingRule_S_
{
    int nRuleId = -1;                   /* <0 时 setRules 按序分配 */
    Alarm::Region_S stRegion;           /* 坐标与检测框同分辨率 */
    unsigned int nSensitivity = 50;     /* [1,100] 灵敏度 */
} QueueCrowdingRule_S;

/* 触发结果 */
typedef struct _QueueCrowdingTrigger_S_
{
    Event::Type enEventType = Event::Type::UNKNOWN;
    int nRuleId = -1;
    int64_t nTimestampMs = 0;
    int nStaticPersonCount = 0;         /* 区域内静止人数 */
    int nPersonThreshold = 0;           /* 触发阈值 */
} QueueCrowdingTrigger_S;

/**
 * @brief  : 排队拥堵处理器
 * @note   : 纯判定组件，无跟踪无线程，由算法线程串行驱动；
 *           基于IOU判定目标是否静止，统计区域内静止人数，达阈值触发
 */
class CQueueCrowdingProcessor
{
public:
    explicit CQueueCrowdingProcessor(Event::Type enEventType);
    ~CQueueCrowdingProcessor();

    /* 规则整体替换（不清目标状态，区域几何变更时应先 reset()） */
    void setRules(const std::vector<QueueCrowdingRule_S> &vecRules);

    /* 清空全部目标判定状态 */
    void reset();

    /**
     * @brief   : 一帧跟踪目标 → 达阈值拥堵事件
     * @note    : nTimestampMs 与跟踪环节同源且单调不减；无规则时清空输出返回
     */
    int process(int64_t nTimestampMs,
                const std::vector<TrackedTarget_S> &vecTracked,
                std::vector<QueueCrowdingTrigger_S> &vecTriggers);

private:
    /* 每条规则的静止状态 */
    typedef struct _RuleState_S_
    {
        int64_t nStaticStartMs = 0;     /* 静止开始时刻，0=非静止 */
        bool bInRegion = false;         /* 当前帧是否在区域内 */
    } RuleState_S;

    /* 目标状态：上一帧bbox + 每规则静止状态 */
    typedef struct _TargetState_S_
    {
        int64_t nLastSeenMs = 0;                                /* 最近出现时刻 */
        bool bHasLastBbox = false;                              /* 是否有上一帧bbox（首帧为false） */
        float fX1 = 0.0f, fY1 = 0.0f, fX2 = 0.0f, fY2 = 0.0f; /* 上一帧bbox */
        std::map<int, RuleState_S> mapRuleState;                /* 规则ID → 静止状态 */
    } TargetState_S;

    /* 灵敏度 → IOU阈值/静止时长/人数阈值 */
    static void sensitivityToThresholds(unsigned int nSensitivity,
                                        float &fIOUThreshold,
                                        int &nStaticTimeSec,
                                        int &nPersonThreshold);

    /* 计算两个矩形框的IOU */
    static float calculateIOU(float ax1, float ay1, float ax2, float ay2,
                              float bx1, float by1, float bx2, float by2);

private:
    Event::Type m_enEventType;
    std::mutex m_mutexRules;                      /* setRules 可能来自配置线程 */
    std::vector<QueueCrowdingRule_S> m_vecRules;
    std::map<int, TargetState_S> m_mapTargets;    /* 跟踪ID → 状态 */
};
