/*
 * @Author: zhengxh
 * @Date: 2026-10-12 20:23:33
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-13 14:29:56
 * @Description: 区域停留处理器声明（同一目标在区域内持续停留达时间阈值触发，纯判定不含跟踪）
 */

#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

#include "alarm_define.h"       /* Alarm::Region_S */
#include "event_define.h"       /* Event::Type */
#include "algorithm_define.hpp" /* TrackedTarget_S */

/* 区域判定规则 */
typedef struct _RegionRule_S_
{
    int nRuleId = -1;                   /* <0 时 setRules 按序分配 */
    Alarm::Region_S stRegion;           /* 坐标与检测框同分辨率 */
    unsigned int nTimeThresholdSec = 0; /* 停留阈值（秒），0=区域内每帧触发 */
    unsigned int nSensitivity = 50;     /* [1,100] 反向映射置信度门槛 */
} RegionRule_S;

/* 触发结果：含触发目标信息（类别由上层回填） */
typedef struct _RegionStayTrigger_S_
{
    Event::Type enEventType = Event::Type::UNKNOWN;
    int nRuleId = -1;
    int64_t nTimestampMs = 0;
    int nTrackId = -1;
    float fConfidence = 0.0f;
    float fX1 = 0.0f, fY1 = 0.0f, fX2 = 0.0f, fY2 = 0.0f;
    int64_t nStayDurationMs = 0;        /* 区域内持续停留时长 */
} RegionStayTrigger_S;

/**
 * @brief  : 区域停留处理器
 * @note   : 纯判定组件，无跟踪无线程，由算法线程串行驱动；不感知类别，一个实例仅喂单一类别目标
 */
class CRegionStayProcessor
{
public:
    explicit CRegionStayProcessor(Event::Type enEventType);
    ~CRegionStayProcessor();

    /* 规则整体替换（不清目标状态，区域几何变更时应先 reset()） */
    void setRules(const std::vector<RegionRule_S> &vecRules);

    /* 清空全部目标判定状态 */
    void reset();

    /**
     * @brief   : 一帧跟踪目标 → 达阈值停留事件
     * @note    : nTimestampMs 与跟踪环节同源且单调不减；无规则时清空输出返回
     */
    int process(int64_t nTimestampMs,
                const std::vector<TrackedTarget_S> &vecTracked,
                std::vector<RegionStayTrigger_S> &vecTriggers);

private:
    /* 目标×规则 的判定状态 */
    typedef struct _RuleState_S_
    {
        bool bInRegion = false;
        int64_t nEnterMs = 0;   /* 本轮进入区域时刻，0=不在区域 */
    } RuleState_S;

    /* 目标状态 */
    typedef struct _TargetState_S_
    {
        int64_t nLastSeenMs = 0;                  /* 最近出现时刻，超时清理判据 */
        std::map<int, RuleState_S> mapRuleState;  /* 规则ID → 状态 */
    } TargetState_S;

    static float sensitivityToConfidence(unsigned int nSensitivity);

private:
    Event::Type m_enEventType;
    std::mutex m_mutexRules;                   /* setRules 可能来自配置线程 */
    std::vector<RegionRule_S> m_vecRules;
    std::map<int, TargetState_S> m_mapTargets; /* 跟踪ID → 状态 */
};
