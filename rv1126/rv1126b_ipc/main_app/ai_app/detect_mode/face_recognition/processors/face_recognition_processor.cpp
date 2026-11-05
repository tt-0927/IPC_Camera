/**
 * @file face_recognition_processor.cpp
 * @brief 人脸识别事件处理器
 */

#include "face_recognition_processor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace
{
constexpr int DEFAULT_MIN_IPD = 20; /* 最小瞳距默认值，单位像素 */
}

void CFaceRecognitionProcessor::setRules(const Alarm::FaceRecognition_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutexRules);
    m_stRules = stConfig;
}

float CFaceRecognitionProcessor::sensitivityToConfidence(unsigned int nSensitivity)
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

int CFaceRecognitionProcessor::process(
    const std::vector<FaceBaseTarget_S> &vstTargets,
    std::vector<int> &vstCandidateIndices)
{
    vstCandidateIndices.clear();

    /* 规则快照：setRules 可能来自配置线程 */
    Alarm::FaceRecognition_S stRules;
    {
        std::lock_guard<std::mutex> lock(m_mutexRules);
        stRules = m_stRules;
    }

    /* 置信度门槛与检测区域始终过滤；抓拍开启时附加抓拍规则（遮挡区域+瞳距） */
    const float fConfidenceThreshold = sensitivityToConfidence(stRules.nSensitivity);
    const bool bRegionConfigured = isRegionConfigured(stRules.stRegion);
    const bool bCaptureEnable = stRules.bCaptureEnable;

    vstCandidateIndices.reserve(vstTargets.size());
    for (size_t i = 0; i < vstTargets.size(); ++i)
    {
        const FaceDetectedTarget_S &stResult = vstTargets[i].stDetectResult;
        if (stResult.fConfidence < fConfidenceThreshold ||
            (bRegionConfigured && !isInRegion(stRules.stRegion, stResult)) ||
            (bCaptureEnable && !isCaptureCandidate(stResult, stRules.stCaptureRule)))
        {
            continue;
        }
        vstCandidateIndices.push_back(static_cast<int>(i));
    }
    return static_cast<int>(vstCandidateIndices.size());
}

bool CFaceRecognitionProcessor::isInRegion(
    const Alarm::Region_S &stRegion,
    const FaceDetectedTarget_S &stResult)
{
    if (!isRegionConfigured(stRegion))
    {
        return false;
    }

    const float fCenterX = (stResult.fX1 + stResult.fX2) * 0.5F;
    const float fCenterY = (stResult.fY1 + stResult.fY2) * 0.5F;
    int nIntersectCount = 0;
    constexpr float EPSILON = 1e-6F;

    for (size_t i = 0; i < stRegion.nPointNum; ++i)
    {
        const Common::PosF_S &stPoint1 = stRegion.aPoint[i];
        const Common::PosF_S &stPoint2 =
            stRegion.aPoint[(i + 1) % stRegion.nPointNum];
        const float fDeltaY = stPoint2.fY - stPoint1.fY;
        if (std::abs(fDeltaY) < EPSILON)
        {
            continue;
        }

        if ((stPoint1.fY > fCenterY) != (stPoint2.fY > fCenterY))
        {
            const float fIntersectX = (stPoint2.fX - stPoint1.fX) *
                                          (fCenterY - stPoint1.fY) / fDeltaY +
                                      stPoint1.fX;
            if (fCenterX < fIntersectX)
            {
                ++nIntersectCount;
            }
        }
    }

    return nIntersectCount % 2 == 1;
}

bool CFaceRecognitionProcessor::isRegionConfigured(const Alarm::Region_S &stRegion)
{
    if (!stRegion.IsValid())
    {
        return false;
    }

    double dTwiceArea = 0.0;
    for (size_t i = 0; i < stRegion.nPointNum; ++i)
    {
        const Common::PosF_S &stPoint1 = stRegion.aPoint[i];
        const Common::PosF_S &stPoint2 =
            stRegion.aPoint[(i + 1) % stRegion.nPointNum];
        dTwiceArea += static_cast<double>(stPoint1.fX) * stPoint2.fY -
                      static_cast<double>(stPoint2.fX) * stPoint1.fY;
    }

    constexpr double MIN_REGION_TWICE_AREA = 1e-3;
    return std::abs(dTwiceArea) > MIN_REGION_TWICE_AREA;
}

bool CFaceRecognitionProcessor::isCaptureCandidate(
    const FaceDetectedTarget_S &stResult,
    const Alarm::FaceRecognitionCaptureRule_S &stRule)
{
    for (const auto &stShieldedRegion : stRule.vstShieldedRegion)
    {
        if (isRegionConfigured(stShieldedRegion) &&
            isInRegion(stShieldedRegion, stResult))
        {
            return false;
        }
    }

    int nMinIpd = DEFAULT_MIN_IPD;
    if (!stRule.stMinIpdRect.isEmpty() &&
        stRule.stMinIpdRect.IsValid())
    {
        nMinIpd = stRule.stMinIpdRect.nWidth;
    }

    /* 关键点为定长数组，模型未输出时瞳距为 0，由最小瞳距阈值自然拒绝 */
    const float fIpd = std::abs(stResult.vPoint[2] - stResult.vPoint[0]);
    return fIpd >= static_cast<float>(nMinIpd);
}
