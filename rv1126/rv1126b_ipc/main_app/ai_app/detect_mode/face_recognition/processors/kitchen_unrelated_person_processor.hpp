/**
 * @file kitchen_unrelated_person_processor.hpp
 * @brief 后厨无关人员进入事件处理器
 */

#pragma once

#include <mutex>
#include <vector>

#include "alarm_define.h"              /* Alarm::KitchenUnrelatedPersonEntry_S */
#include "face_attribute_model.hpp"   /* FaceAttributeTarget_S */
#include "face_recognition_define.hpp" /* FaceBaseTarget_S */

/**
 * @brief 后厨无关人员进入事件处理器：人脸检测过灵敏度门槛且属性分析判定未戴口罩的目标产出候选下标
 * @note 纯规则判定，不碰模型与图像；规则可来自配置线程，算法线程串行 process
 */
class CKitchenUnrelatedPersonProcessor
{
public:
    /* 事件规则整体替换（灵敏度门槛） */
    void setRules(const Alarm::KitchenUnrelatedPersonEntry_S &stConfig);

    /**
     * @brief  基础目标 + 属性分析结果（均对齐输入向量）→ 候选下标
     * @return 候选数量，无候选时为 0
     */
    int process(const std::vector<FaceBaseTarget_S> &vstTargets,
                const std::vector<FaceAttributeTarget_S> &vstAttributeResults,
                const std::vector<bool> &vbAttributeReady,
                std::vector<int> &vstCandidateIndices);

private:
    static float sensitivityToConfidence(unsigned int nSensitivity);

private:
    std::mutex m_mutexRules;                        /* setRules 可能来自配置线程 */
    Alarm::KitchenUnrelatedPersonEntry_S m_stRules; /* 事件规则快照 */
};
