/*
 * @Author: zhengxh
 * @Date: 2026-11-02
 * @Description: 社交关系分析事件子检测器声明
 */

#pragma once

#include <mutex>
#include <vector>

#include "algorithm.hpp"                    /* CAlarmStateMachine */
#include "face_sub_detector.hpp"            /* IFaceSubDetector */
#include "social_relationship_processor.hpp"

/**
 * @brief  : 社交关系分析事件子检测器
 * @note   : 无特化模型，基础管线目标按灵敏度门槛过滤后驱动状态机上报；
 *           配置自订阅（updateConfig 自行拉取），无线程，由算法线程串行驱动
 */
class CSocialRelationshipSubDetector : public IFaceSubDetector
{
public:
    CSocialRelationshipSubDetector() = default;
    ~CSocialRelationshipSubDetector() override = default;

    void updateConfig() override;
    int process(const DetectContext_S &stDetectContext) override;
    bool isEventEnabled() const override;
    void reset() override;

private:
    CSocialRelationshipProcessor m_processor; /* 规则判断器 */
    CAlarmStateMachine m_stateMachine;        /* 报警状态机 */

    mutable std::mutex m_mutexConfig;         /* updateConfig 与算法线程互斥 */
    bool m_bEnable = false;                   /* 配置快照：事件使能 */
};
