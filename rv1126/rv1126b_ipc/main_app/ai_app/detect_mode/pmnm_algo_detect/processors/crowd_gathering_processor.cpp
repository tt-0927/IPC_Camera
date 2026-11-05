/*
 * @Description: 人员聚集处理器实现（区域内人员面积占比判定，纯判定不含配置和报警）
 */

#include "crowd_gathering_processor.hpp"

#include <cmath>

CCrowdGatheringProcessor::CCrowdGatheringProcessor(Event::Type enEventType)
    : m_enEventType(enEventType)
{
}

CCrowdGatheringProcessor::~CCrowdGatheringProcessor()
{
}

void CCrowdGatheringProcessor::setRules(const std::vector<CrowdGatheringRule_S> &vecRules)
{
    std::vector<CrowdGatheringRule_S> vecNewRules = vecRules;
    for (size_t i = 0; i < vecNewRules.size(); ++i)
    {
        if (vecNewRules[i].nRuleId < 0)
        {
            vecNewRules[i].nRuleId = static_cast<int>(i);
        }
    }

    std::lock_guard<std::mutex> lock(m_mutexRules);
    m_vecRules = vecNewRules;
}

void CCrowdGatheringProcessor::reset()
{
    /* 纯空间判定，无历史状态需要清理 */
}

int CCrowdGatheringProcessor::process(int64_t nTimestampMs,
                                       const std::vector<TrackedTarget_S> &vecTracked,
                                       std::vector<CrowdGatheringTrigger_S> &vecTriggers)
{
    vecTriggers.clear();

    /* 规则快照 */
    std::vector<CrowdGatheringRule_S> vecRules;
    {
        std::lock_guard<std::mutex> lock(m_mutexRules);
        vecRules = m_vecRules;
    }

    if (vecRules.empty())
    {
        return 0;
    }

    /* 遍历每个规则区域 */
    for (size_t i = 0; i < vecRules.size(); ++i)
    {
        const CrowdGatheringRule_S &stRule = vecRules[i];

        if (stRule.vecPolygon.size() < 3)
        {
            continue;
        }

        /* 计算区域面积 */
        float fPolygonArea = polygonArea(stRule.vecPolygon);
        if (fPolygonArea <= 0.0f)
        {
            continue;
        }

        /* 统计区域内人员面积 */
        float fPersonArea = 0.0f;
        for (const auto &target : vecTracked)
        {
            fPersonArea += rectInPolygonArea(target, stRule.vecPolygon);
        }

        /* 计算面积占比 */
        float fRatio = (fPersonArea / fPolygonArea) * 100.0f;

        /* 判断是否超过阈值 */
        if (fRatio >= static_cast<float>(stRule.nSensitivity))
        {
            CrowdGatheringTrigger_S stTrigger;
            stTrigger.enEventType = m_enEventType;
            stTrigger.nRuleId = stRule.nRuleId;
            stTrigger.nTimestampMs = nTimestampMs;
            stTrigger.fAreaRatio = fRatio;
            stTrigger.nThreshold = stRule.nSensitivity;
            vecTriggers.push_back(stTrigger);
        }
    }

    return 0;
}

/* ========================= 几何计算工具函数 ========================= */

bool CCrowdGatheringProcessor::pointInPolygon(const cv::Point& pt, const std::vector<cv::Point>& polygon)
{
    bool bInside = false;
    int nCount = static_cast<int>(polygon.size());

    for (int i = 0, j = nCount - 1; i < nCount; j = i++)
    {
        if (((polygon[i].y > pt.y) != (polygon[j].y > pt.y)) &&
            (pt.x < (polygon[j].x - polygon[i].x) * (pt.y - polygon[i].y) /
             (polygon[j].y - polygon[i].y) + polygon[i].x))
        {
            bInside = !bInside;
        }
    }

    return bInside;
}

float CCrowdGatheringProcessor::polygonArea(const std::vector<cv::Point>& polygon)
{
    float fArea = 0.0f;
    int nCount = static_cast<int>(polygon.size());

    for (int i = 0; i < nCount; i++)
    {
        int j = (i + 1) % nCount;
        fArea += polygon[i].x * polygon[j].y;
        fArea -= polygon[j].x * polygon[i].y;
    }

    return std::abs(fArea) / 2.0f;
}

float CCrowdGatheringProcessor::rectArea(const TrackedTarget_S& target)
{
    float fWidth = target.fX2 - target.fX1;
    float fHeight = target.fY2 - target.fY1;
    return (fWidth > 0 && fHeight > 0) ? (fWidth * fHeight) : 0.0f;
}

float CCrowdGatheringProcessor::rectInPolygonArea(const TrackedTarget_S& target, const std::vector<cv::Point>& polygon)
{
    cv::Point center(static_cast<int>((target.fX1 + target.fX2) / 2),
                     static_cast<int>((target.fY1 + target.fY2) / 2));

    if (pointInPolygon(center, polygon))
    {
        return rectArea(target);
    }

    return 0.0f;
}
