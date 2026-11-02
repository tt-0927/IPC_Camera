/**
 * @file kitchen_unrelated_person_processor.cpp
 * @brief 后厨无关人员进入事件处理器
 */

#include "kitchen_unrelated_person_processor.hpp"

#include <algorithm>

void CKitchenUnrelatedPersonProcessor::setRules(
    const Alarm::KitchenUnrelatedPersonEntry_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutexRules);
    m_stRules = stConfig;
}

float CKitchenUnrelatedPersonProcessor::sensitivityToConfidence(
    unsigned int nSensitivity)
{
    /* 灵敏度[1,100]反向映射置信度门槛[0.9,0.1]，灵敏度越高门槛越低 */
    constexpr float fConfidenceMax = 0.9F;
    constexpr float fConfidenceMin = 0.1F;
    const int nClamped = std::clamp(static_cast<int>(nSensitivity), 1, 100);
    const float fConfidence =
        fConfidenceMax -
        (nClamped - 1) * (fConfidenceMax - fConfidenceMin) / (100 - 1);
    return std::clamp(fConfidence, fConfidenceMin, fConfidenceMax);
}

int CKitchenUnrelatedPersonProcessor::process(
    const DetectContext_S &stDetectContext,
    std::vector<FaceTargetResult_S> &vstCandidates)
{
    vstCandidates.clear();

    /* 规则快照：setRules 可能来自配置线程 */
    Alarm::KitchenUnrelatedPersonEntry_S stRules;
    {
        std::lock_guard<std::mutex> lock(m_mutexRules);
        stRules = m_stRules;
    }

    const float fConfidenceThreshold = sensitivityToConfidence(
        stRules.stRule.nSensitivity);

    const std::vector<FaceTargetResult_S> &vstTargets = stDetectContext.vstDetectResult;
    vstCandidates.reserve(vstTargets.size());
    for (const auto &stTarget : vstTargets)
    {
        /* 属性分析失败(bAttributeReady=false)不进候选，避免侧脸/模糊人脸误报 */
        if (stTarget.stDetectResult.fConfidence >= fConfidenceThreshold &&
            stTarget.bAttributeReady &&
            !stTarget.stAttributeResult.bIsMask)
        {
            vstCandidates.push_back(stTarget);
        }
    }
    return static_cast<int>(vstCandidates.size());
}
