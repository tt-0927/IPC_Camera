/**
 * @file kitchen_unrelated_person_processor.hpp
 * @brief 后厨无关人员进入事件处理器
 */

#pragma once

#include <mutex>
#include <vector>

#include "alarm_define.h"              /* Alarm::KitchenUnrelatedPersonEntry_S */
#include "face_recognition_define.hpp" /* FaceTargetResult_S */

/**
 * @brief 后厨无关人员进入事件处理器：人脸检测过灵敏度门槛且属性分析判定未戴口罩的目标产出候选
 * @note 纯规则判定，不碰模型与图像；规则可来自配置线程，算法线程串行 process
 */
class CKitchenUnrelatedPersonProcessor
{
public:
    /* 事件规则整体替换（灵敏度门槛） */
    void setRules(const Alarm::KitchenUnrelatedPersonEntry_S &stConfig);

    /**
     * @brief  一帧检测上下文 → 候选目标
     * @return 候选数量，无候选时为 0
     */
    int process(const DetectContext_S &stDetectContext,
                std::vector<FaceTargetResult_S> &vstCandidates);

private:
    static float sensitivityToConfidence(unsigned int nSensitivity);

private:
    std::mutex m_mutexRules;                        /* setRules 可能来自配置线程 */
    Alarm::KitchenUnrelatedPersonEntry_S m_stRules; /* 事件规则快照 */
};
