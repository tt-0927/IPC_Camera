/*
 * @Description: 人员聚集处理器声明（区域内人员面积占比判定，纯判定不含配置和报警）
 */

#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include <opencv2/opencv.hpp>

#include "event_define.h"       /* Event::Type */
#include "algorithm_define.hpp" /* TrackedTarget_S */

/* 人员聚集判定规则 */
typedef struct _CrowdGatheringRule_S_
{
    int nRuleId = -1;                   /* <0 时 setRules 按序分配 */
    std::vector<cv::Point> vecPolygon;  /* 多边形区域（AI帧坐标系） */
    unsigned int nSensitivity = 50;     /* [1,100] 面积占比阈值 */
} CrowdGatheringRule_S;

/* 触发结果 */
typedef struct _CrowdGatheringTrigger_S_
{
    Event::Type enEventType = Event::Type::UNKNOWN;
    int nRuleId = -1;
    int64_t nTimestampMs = 0;
    float fAreaRatio = 0.0f;            /* 人员面积占比 (%) */
    unsigned int nThreshold = 0;        /* 触发阈值 (%) */
} CrowdGatheringTrigger_S;

/**
 * @brief  : 人员聚集处理器
 * @note   : 纯判定组件，无配置无报警，由调用方驱动；
 *           计算区域内人员面积占比，达阈值触发
 */
class CCrowdGatheringProcessor
{
public:
    explicit CCrowdGatheringProcessor(Event::Type enEventType);
    ~CCrowdGatheringProcessor();

    /* 规则整体替换 */
    void setRules(const std::vector<CrowdGatheringRule_S> &vecRules);

    /* 清空状态 */
    void reset();

    /**
     * @brief   : 一帧跟踪目标 → 达阈值聚集事件
     * @note    : nTimestampMs 与跟踪环节同源且单调不减；无规则时清空输出返回
     */
    int process(int64_t nTimestampMs,
                const std::vector<TrackedTarget_S> &vecTracked,
                std::vector<CrowdGatheringTrigger_S> &vecTriggers);

private:
    /* 几何计算工具函数 */
    static bool pointInPolygon(const cv::Point& pt, const std::vector<cv::Point>& polygon);
    static float polygonArea(const std::vector<cv::Point>& polygon);
    static float rectArea(const TrackedTarget_S& target);
    static float rectInPolygonArea(const TrackedTarget_S& target, const std::vector<cv::Point>& polygon);

private:
    Event::Type m_enEventType;
    std::mutex m_mutexRules;                      /* setRules 可能来自配置线程 */
    std::vector<CrowdGatheringRule_S> m_vecRules;
};
