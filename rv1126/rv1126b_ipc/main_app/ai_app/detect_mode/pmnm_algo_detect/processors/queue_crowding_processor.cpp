/*
 * @Description: 排队拥堵处理器实现（区域内多人IOU静止判定 + 人数统计状态机）
 */

#include "queue_crowding_processor.hpp"

#include <algorithm>

/* 目标从跟踪输出消失后的状态保留时长 */
#define TARGET_ABSENCE_TIMEOUT_MS (3000)

CQueueCrowdingProcessor::CQueueCrowdingProcessor(Event::Type enEventType)
 : m_enEventType(enEventType)
{
}

CQueueCrowdingProcessor::~CQueueCrowdingProcessor()
{
}

void CQueueCrowdingProcessor::setRules(const std::vector<QueueCrowdingRule_S> &vecRules)
{
    std::vector<QueueCrowdingRule_S> vecNewRules = vecRules;
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

void CQueueCrowdingProcessor::reset()
{
    m_mapTargets.clear();
}

/* 灵敏度[1,100] → IOU阈值/静止时长/人数阈值 */
void CQueueCrowdingProcessor::sensitivityToThresholds(unsigned int nSensitivity,
                                                       float &fIOUThreshold,
                                                       int &nStaticTimeSec,
                                                       int &nPersonThreshold)
{
    int nClampedSens = std::clamp(static_cast<int>(nSensitivity), 1, 100);

    if (nClampedSens <= 33)
    {
        /* 低灵敏度：更严格的IOU、更长的静止时间、更多的人数 */
        fIOUThreshold  = 0.95f;
        nStaticTimeSec = 10;
        nPersonThreshold = 8;
    }
    else if (nClampedSens <= 66)
    {
        /* 中灵敏度 */
        fIOUThreshold  = 0.90f;
        nStaticTimeSec = 6;
        nPersonThreshold = 5;
    }
    else
    {
        /* 高灵敏度：更宽松的IOU、更短的静止时间、更少的人数 */
        fIOUThreshold  = 0.85f;
        nStaticTimeSec = 3;
        nPersonThreshold = 3;
    }
}

float CQueueCrowdingProcessor::calculateIOU(float ax1, float ay1, float ax2, float ay2,
                                             float bx1, float by1, float bx2, float by2)
{
    float x1 = std::max(ax1, bx1);
    float y1 = std::max(ay1, by1);
    float x2 = std::min(ax2, bx2);
    float y2 = std::min(ay2, by2);

    float fIntersection = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
    float fArea1 = (ax2 - ax1) * (ay2 - ay1);
    float fArea2 = (bx2 - bx1) * (by2 - by1);
    float fUnion = fArea1 + fArea2 - fIntersection;

    if (fUnion <= 0.0f) return 0.0f;
    return fIntersection / fUnion;
}

/* 点是否在多边形内（射线法，点数<3 恒为外） */
static bool isPointInRegion(float fX, float fY, const Alarm::Region_S &stRegion)
{
    size_t nCount = std::min(static_cast<size_t>(stRegion.nPointNum), stRegion.aPoint.size());
    if (nCount < 3)
    {
        return false;
    }

    bool bInside = false;
    for (size_t i = 0, j = nCount - 1; i < nCount; j = i, ++i)
    {
        const Common::PosF_S &stCur = stRegion.aPoint[i];
        const Common::PosF_S &stPre = stRegion.aPoint[j];

        if (((stCur.fY > fY) != (stPre.fY > fY)) &&
            (fX < (stPre.fX - stCur.fX) * (fY - stCur.fY) / (stPre.fY - stCur.fY) + stCur.fX))
        {
            bInside = !bInside;
        }
    }

    return bInside;
}

int CQueueCrowdingProcessor::process(int64_t nTimestampMs,
                                      const std::vector<TrackedTarget_S> &vecTracked,
                                      std::vector<QueueCrowdingTrigger_S> &vecTriggers)
{
    vecTriggers.clear();

    /* 规则快照，判定全程无锁 */
    std::vector<QueueCrowdingRule_S> vecRules;
    {
        std::lock_guard<std::mutex> lock(m_mutexRules);
        vecRules = m_vecRules;
    }
    if (vecRules.empty())
    {
        m_mapTargets.clear();
        return 0;
    }

    /* 本帧出现的跟踪ID集合 */
    std::map<int, bool> mapSeenCurFrame;

    /* 第1遍：更新每个目标的状态（IOU判定 + 静止计时） */
    for (size_t i = 0; i < vecTracked.size(); ++i)
    {
        const TrackedTarget_S &stTarget = vecTracked[i];
        if (stTarget.nTrackId < 0)
        {
            continue;
        }

        TargetState_S &stState = m_mapTargets[stTarget.nTrackId];
        stState.nLastSeenMs = nTimestampMs;
        mapSeenCurFrame[stTarget.nTrackId] = true;

        /* 对每条规则独立判定 */
        for (size_t j = 0; j < vecRules.size(); ++j)
        {
            const QueueCrowdingRule_S &stRule = vecRules[j];
            RuleState_S &stRuleState = stState.mapRuleState[stRule.nRuleId];

            /* 灵敏度 → 阈值 */
            float fIOUThreshold = 0.90f;
            int nStaticTimeSec = 6;
            int nPersonThreshold = 5;
            sensitivityToThresholds(stRule.nSensitivity, fIOUThreshold, nStaticTimeSec, nPersonThreshold);

            /* 脚底中心点是否在区域内 */
            float fFootX = (stTarget.fX1 + stTarget.fX2) / 2.0f;
            float fFootY = stTarget.fY2;
            bool bInRegion = isPointInRegion(fFootX, fFootY, stRule.stRegion);

            stRuleState.bInRegion = bInRegion;

            if (!bInRegion)
            {
                /* 离开区域：复位静止计时 */
                stRuleState.nStaticStartMs = 0;
                continue;
            }

            /* 在区域内：首帧只记录，不计算IOU */
            if (!stState.bHasLastBbox)
            {
                /* 首帧：不判定静止，等下一帧 */
                continue;
            }

            /* 计算与上一帧的IOU */
            float fIOU = calculateIOU(stState.fX1, stState.fY1, stState.fX2, stState.fY2,
                                      stTarget.fX1, stTarget.fY1, stTarget.fX2, stTarget.fY2);

            if (fIOU >= fIOUThreshold)
            {
                /* 静止：记录或累加静止时间 */
                if (stRuleState.nStaticStartMs <= 0)
                {
                    stRuleState.nStaticStartMs = nTimestampMs;
                }
            }
            else
            {
                /* 运动：复位静止计时 */
                stRuleState.nStaticStartMs = 0;
            }
        }

        /* 更新上一帧bbox（放在规则循环外面，只更新一次） */
        stState.fX1 = stTarget.fX1;
        stState.fY1 = stTarget.fY1;
        stState.fX2 = stTarget.fX2;
        stState.fY2 = stTarget.fY2;
        stState.bHasLastBbox = true;
    }

    /* 帧末清理：离开区域或超时未出现的目标 */
    for (auto it = m_mapTargets.begin(); it != m_mapTargets.end();)
    {
        int nTrackId = it->first;
        TargetState_S &stState = it->second;

        if (nTimestampMs - stState.nLastSeenMs > TARGET_ABSENCE_TIMEOUT_MS)
        {
            /* 超时未出现：删除整个目标 */
            it = m_mapTargets.erase(it);
        }
        else if (mapSeenCurFrame.find(nTrackId) == mapSeenCurFrame.end())
        {
            /* 本帧未出现（但还没超时）：重置所有规则的静止状态 */
            for (auto &rulePair : stState.mapRuleState)
            {
                rulePair.second.nStaticStartMs = 0;
                rulePair.second.bInRegion = false;
            }
            ++it;
        }
        else
        {
            ++it;
        }
    }

    /* 第2遍：统计区域内静止人数并判定触发 */
    for (size_t j = 0; j < vecRules.size(); ++j)
    {
        const QueueCrowdingRule_S &stRule = vecRules[j];

        float fIOUThreshold = 0.90f;
        int nStaticTimeSec = 6;
        int nPersonThreshold = 5;
        sensitivityToThresholds(stRule.nSensitivity, fIOUThreshold, nStaticTimeSec, nPersonThreshold);

        int nStaticPersonCount = 0;
        int64_t nThresholdMs = static_cast<int64_t>(nStaticTimeSec) * 1000;

        for (const auto &pair : m_mapTargets)
        {
            const TargetState_S &stState = pair.second;

            /* 检查该规则的状态 */
            auto ruleIt = stState.mapRuleState.find(stRule.nRuleId);
            if (ruleIt == stState.mapRuleState.end())
            {
                continue;
            }

            const RuleState_S &stRuleState = ruleIt->second;

            /* 检查是否在区域内 */
            if (!stRuleState.bInRegion)
            {
                continue;
            }

            /* 检查静止时长是否达标 */
            if (stRuleState.nStaticStartMs > 0 &&
                (nTimestampMs - stRuleState.nStaticStartMs) >= nThresholdMs)
            {
                nStaticPersonCount++;
            }
        }

        /* 达到人数阈值则触发 */
        if (nStaticPersonCount >= nPersonThreshold)
        {
            QueueCrowdingTrigger_S stTrigger;
            stTrigger.enEventType = m_enEventType;
            stTrigger.nRuleId = stRule.nRuleId;
            stTrigger.nTimestampMs = nTimestampMs;
            stTrigger.nStaticPersonCount = nStaticPersonCount;
            stTrigger.nPersonThreshold = nPersonThreshold;
            vecTriggers.push_back(stTrigger);
        }
    }

    return 0;
}
