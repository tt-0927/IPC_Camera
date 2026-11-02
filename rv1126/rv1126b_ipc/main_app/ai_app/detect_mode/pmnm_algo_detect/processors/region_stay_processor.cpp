/*
 * @Author: zhengxh
 * @Date: 2026-10-12 20:23:33
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-12 20:25:48
 * @Description: 区域停留处理器实现（区域内 + 停留时长 + 灵敏度三要素状态机）
 */

#include "region_stay_processor.hpp"

#include <algorithm>

/* 灵敏度[1,100] 反向映射置信度区间，灵敏度越高门槛越低 */
#define SENS_CONFIDENCE_MAX (0.9f)
#define SENS_CONFIDENCE_MIN (0.1f)

/* 目标从跟踪输出消失后的状态保留时长 */
#define TARGET_ABSENCE_TIMEOUT_MS (3000)

CRegionStayProcessor::CRegionStayProcessor(Event::Type enEventType)
 : m_enEventType(enEventType)
{
}

CRegionStayProcessor::~CRegionStayProcessor()
{
}

void CRegionStayProcessor::setRules(const std::vector<RegionRule_S> &vecRules)
{
    std::vector<RegionRule_S> vecNewRules = vecRules;
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

void CRegionStayProcessor::reset()
{
    m_mapTargets.clear();
}

float CRegionStayProcessor::sensitivityToConfidence(unsigned int nSensitivity)
{
    int nClampedSens = std::clamp(static_cast<int>(nSensitivity), 1, 100);
    float fConfidence = SENS_CONFIDENCE_MAX -
                        (nClampedSens - 1) * (SENS_CONFIDENCE_MAX - SENS_CONFIDENCE_MIN) / (100 - 1);

    return std::clamp(fConfidence, SENS_CONFIDENCE_MIN, SENS_CONFIDENCE_MAX);
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

int CRegionStayProcessor::process(int64_t nTimestampMs,
                                  const std::vector<TrackedTarget_S> &vecTracked,
                                  std::vector<RegionStayTrigger_S> &vecTriggers)
{
    vecTriggers.clear();

    /* 规则快照，判定全程无锁 */
    std::vector<RegionRule_S> vecRules;
    {
        std::lock_guard<std::mutex> lock(m_mutexRules);
        vecRules = m_vecRules;
    }
    if (vecRules.empty())
    {
        m_mapTargets.clear();
        return 0;
    }

    auto fillTrigger = [&](const RegionRule_S &stRule, const TrackedTarget_S &stTarget, int64_t nStayMs) {
        RegionStayTrigger_S stTrigger;
        stTrigger.enEventType     = m_enEventType;
        stTrigger.nRuleId         = stRule.nRuleId;
        stTrigger.nTimestampMs    = nTimestampMs;
        stTrigger.nTrackId        = stTarget.nTrackId;
        stTrigger.fConfidence     = stTarget.fConfidence;
        stTrigger.fX1             = stTarget.fX1;
        stTrigger.fY1             = stTarget.fY1;
        stTrigger.fX2             = stTarget.fX2;
        stTrigger.fY2             = stTarget.fY2;
        stTrigger.nStayDurationMs = nStayMs;
        vecTriggers.push_back(stTrigger);
    };

    for (size_t i = 0; i < vecTracked.size(); ++i)
    {
        const TrackedTarget_S &stTarget = vecTracked[i];
        if (stTarget.nTrackId < 0)
        {
            continue;
        }

        TargetState_S &stTargetState = m_mapTargets[stTarget.nTrackId];
        stTargetState.nLastSeenMs = nTimestampMs;

        for (size_t j = 0; j < vecRules.size(); ++j)
        {
            const RegionRule_S &stRule = vecRules[j];

            /* 置信度门槛不满足：不推进该规则状态 */
            if (stTarget.fConfidence < sensitivityToConfidence(stRule.nSensitivity))
            {
                continue;
            }

            bool bInRegion = isPointInRegion((stTarget.fX1 + stTarget.fX2) / 2.0f, stTarget.fY2, stRule.stRegion);

            RuleState_S &stRuleState = stTargetState.mapRuleState[stRule.nRuleId];
            if (!bInRegion)
            {
                /* 离开区域即复位 */
                stRuleState.bInRegion = false;
                stRuleState.nEnterMs  = 0;
                continue;
            }

            stRuleState.bInRegion = true;

            if (stRuleState.nEnterMs <= 0)
            {
                /* 首帧进入起表 */
                stRuleState.nEnterMs = nTimestampMs;
            }

            /* 阈值 0 区域内每帧触发；阈值 >0 满周期触发，触发后复位重新计时 */
            int64_t nThresholdMs = static_cast<int64_t>(stRule.nTimeThresholdSec) * 1000;
            int64_t nStayMs = nTimestampMs - stRuleState.nEnterMs;
            if (nStayMs >= nThresholdMs)
            {
                fillTrigger(stRule, stTarget, nStayMs);
                stRuleState.nEnterMs = nTimestampMs;
            }
        }
    }

    /* 帧末清理：连续超时未出现的目标删除全部状态 */
    for (auto it = m_mapTargets.begin(); it != m_mapTargets.end();)
    {
        if (nTimestampMs - it->second.nLastSeenMs > TARGET_ABSENCE_TIMEOUT_MS)
        {
            it = m_mapTargets.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return 0;
}
