/**
 * @file social_relationship_processor.hpp
 * @brief 社交关系分析事件处理器
 */

#pragma once

#include <mutex>
#include <vector>

#include "alarm_define.h"              /* Alarm::SocialRelationshipAnalysis_S */
#include "face_recognition_define.hpp" /* FaceTargetResult_S */

/**
 * @brief 社交关系分析事件处理器：人脸管线目标按灵敏度门槛产出候选目标
 * @note 纯规则判定，不碰模型与图像；规则可来自配置线程，算法线程串行 process
 */
class CSocialRelationshipProcessor
{
public:
    /* 事件规则整体替换（灵敏度门槛） */
    void setRules(const Alarm::SocialRelationshipAnalysis_S &stConfig);

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
    Alarm::SocialRelationshipAnalysis_S m_stRules;  /* 事件规则快照 */
};
