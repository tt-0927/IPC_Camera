/**
 * @file social_relationship_processor.cpp
 * @brief 社交关系分析事件处理器
 */

#include "social_relationship_processor.hpp"

#include <algorithm>

void CSocialRelationshipProcessor::setRules(
    const Alarm::SocialRelationshipAnalysis_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutexRules);
    m_stRules = stConfig;
}

float CSocialRelationshipProcessor::sensitivityToConfidence(unsigned int nSensitivity)
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

int CSocialRelationshipProcessor::process(
    const DetectContext_S &stDetectContext,
    std::vector<FaceTargetResult_S> &vstCandidates)
{
    vstCandidates.clear();

    /* 规则快照：setRules 可能来自配置线程 */
    Alarm::SocialRelationshipAnalysis_S stRules;
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
        if (stTarget.stDetectResult.fConfidence >= fConfidenceThreshold)
        {
            vstCandidates.push_back(stTarget);
        }
    }
    return static_cast<int>(vstCandidates.size());
}
