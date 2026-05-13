/***
 * @FilePath     : behavior_calculator.cpp
 * @Author       : zhengxh (zhengxh@kfb.cn)
 * @Date         : 2026-04-03 14:00:00
 * @LastEditors  : zhengxh
 * @LastEditTime : 2026-04-03 14:00:00
 * @Description  : 学生行为分析计算器实现，基于逐帧行为检测结果跟踪各行为类型的持续状态，
 *                 满足时长阈值时触发回调，并记录行为事件时间线与课堂表现统计。
 */
#ifdef ENABLE_AI_STUDENT
#include "behavior_calculator.hpp"
#include "time_utils.h"
#include "dlog.h"
#include <cstdio>
#include "StudentBehaviorExt.hpp"
#include "ai_student_define.h"

using namespace AiStudentBusiness_NS;

/* 行为持续 2 秒触发首次回调 */
static constexpr int64_t BEHAVIOR_THRESHOLD_FIRST_S = 2;
/* 行为持续 10 秒触发第二次回调 */
static constexpr int64_t BEHAVIOR_THRESHOLD_SECOND_S = 10;

void CBehaviorCalculator::setTotal(int nTotal)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_nTotal = nTotal;
}

void CBehaviorCalculator::setCourseStartTime(int64_t nStartTime)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_nCourseStartTime = nStartTime;
}

void CBehaviorCalculator::setCallback(BehaviorCallback callback)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_callback = std::move(callback);
}

bool CBehaviorCalculator::handle(const std::vector<StudentBehavior_NS::Behavior_S> &vBehaviors, int nCount)
{
    if (nCount < 0)
        return false;

    std::unique_lock<std::shared_mutex> lock(m_mutex);
    int64_t                             nNow = TimeUtils_NS::get_currentTimestampS();

    /* 判定当前帧主导行为 */
    int nDominant = detectDominantBehavior(vBehaviors, nCount);

    /* 主导行为发生切换时，结束旧行为、激活新行为 */
    if (nDominant != m_nCurrentDominant)
    {
        /* 结束上一个主导行为 */
        if (m_nCurrentDominant > 0)
        {
            auto enOld = static_cast<PlatformBehaviorType_E>(m_nCurrentDominant);
            endBehavior(enOld, nNow);
        }
        /* 激活新主导行为 */
        if (nDominant > 0)
        {
            auto enNew = static_cast<PlatformBehaviorType_E>(nDominant);
            activateBehavior(enNew, nNow);
        }
        m_nCurrentDominant = nDominant;
    }

    /* 主导行为未切换时，检查是否达到持续时长阈值 */
    if (m_nCurrentDominant > 0)
    {
        auto enCurrent = static_cast<PlatformBehaviorType_E>(m_nCurrentDominant);
        checkBehaviorThreshold(enCurrent, nNow);
    }

    return true;
}

bool CBehaviorCalculator::getBehaviorTimeline(std::vector<BehaviorRecord> &vRecords) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    vRecords = m_vTimeline;
    return true;
}

bool CBehaviorCalculator::getPerformance(StudentPerformance &stOut) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    /* TODO: 基于 m_vTimeline 和 m_mapDurationAccum 计算课堂表现，待后续实现 */
    stOut = StudentPerformance{
        78,
        "学生专注度均值为78",
        "学生注意力涣散时间未发生于本课堂",
        "学生情绪较低落未发生于本课堂"};
    return true;
}

void CBehaviorCalculator::reset()
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_nCurrentDominant = -1;
    m_mapBehaviorState.clear();
    m_vTimeline.clear();
    m_mapDurationAccum.clear();
}

int CBehaviorCalculator::detectDominantBehavior(const std::vector<StudentBehavior_NS::Behavior_S> &vBehaviors, int nCount) const
{
    BehaviorCnt_S stBehaviorCnt;
    for (auto &iter : vBehaviors)
    {
        if (iter.nBehavior == StudentBehavior_NS::BendingHead)
            stBehaviorCnt.nBendingHeadCnt++;
        if (iter.nBehavior == StudentBehavior_NS::RaisingHead)
            stBehaviorCnt.nRaisingHeadCnt++;
        if (iter.nBehavior == StudentBehavior_NS::TurningHead)
            stBehaviorCnt.nTurningHeadCnt++;
        if (iter.nBehavior == StudentBehavior_NS::RaisingHand)
            stBehaviorCnt.nRaisingHandCnt++;
        if (iter.nBehavior == StudentBehavior_NS::Standing)
            stBehaviorCnt.nStandingCnt++;
        if (iter.nBehavior == StudentBehavior_NS::TurningBody)
            stBehaviorCnt.nTurningBodyCnt++;
        if (iter.nBehavior == StudentBehavior_NS::LyingOnDesk)
            stBehaviorCnt.nLyingOnDeskCnt++;
    }
    dlog_debug("学生行为分析：【低头：%d】 【抬头：%d】 【转头：%d】 【举手：%d】 【站立：%d】 【转身：%d】 【趴桌：%d】\n",
               stBehaviorCnt.nBendingHeadCnt,
               stBehaviorCnt.nRaisingHeadCnt,
               stBehaviorCnt.nTurningHeadCnt,
               stBehaviorCnt.nRaisingHandCnt,
               stBehaviorCnt.nStandingCnt,
               stBehaviorCnt.nTurningBodyCnt,
               stBehaviorCnt.nLyingOnDeskCnt);

    // if (isDemonstration(stBehaviorCnt, nCount) || stBehaviorCnt.nStandingCnt == nMax)
    // {
    //     return DEMONSTRATION;
    // }

    if (stBehaviorCnt.nStandingCnt > 1)
    {
        return DEMONSTRATION;
    }

    if (isPraxis(stBehaviorCnt, nCount))
    {
        return PRAXIS;
    }

    if (isDiscussion(stBehaviorCnt, nCount))
    {
        return DISCUSSION;
    }

    if (isRead(stBehaviorCnt, nCount))
    {
        return READ;
    }

    /* 默认返回听讲 */
    return LISTEN_TO_TALK;
}

std::string CBehaviorCalculator::calcElapsedTime(int64_t nNow) const
{
    int64_t nElapsed = nNow - m_nCourseStartTime;
    if (nElapsed < 0)
        nElapsed = 0;

    int nHours   = static_cast<int>(nElapsed / 3600);
    int nMinutes = static_cast<int>((nElapsed % 3600) / 60);
    int nSeconds = static_cast<int>(nElapsed % 60);

    char achBuf[16] = {0};
    snprintf(achBuf, sizeof(achBuf), "%02d:%02d:%02d", nHours, nMinutes, nSeconds);
    return std::string(achBuf);
}

void CBehaviorCalculator::activateBehavior(PlatformBehaviorType_E enType, int64_t nNow)
{
    BehaviorState_S &stState = m_mapBehaviorState[enType];
    stState.bActive          = true;
    stState.nStartTime       = nNow;
    stState.bTriggered2s     = false;
    stState.bTriggered10s    = false;
}

void CBehaviorCalculator::checkBehaviorThreshold(PlatformBehaviorType_E enType, int64_t nNow)
{
    auto it = m_mapBehaviorState.find(enType);
    if (it == m_mapBehaviorState.end() || !it->second.bActive)
        return;

    BehaviorState_S &stState   = it->second;
    int64_t          nDuration = nNow - stState.nStartTime;

    /* 持续 2 秒触发首次回调（行为开始） */
    if (!stState.bTriggered2s && nDuration >= BEHAVIOR_THRESHOLD_FIRST_S)
    {
        stState.bTriggered2s = true;

        BehaviorRecord stRecord;
        stRecord.bState         = true;
        stRecord.strTriggerTime = calcElapsedTime(nNow);
        stRecord.eBehaviorType  = enType;
        m_vTimeline.push_back(stRecord);

        if (m_callback)
        {
            m_callback(stRecord);
        }
    }

    /* 持续 10 秒触发第二次回调（行为稳定确认） */
    if (!stState.bTriggered10s && nDuration >= BEHAVIOR_THRESHOLD_SECOND_S)
    {
        stState.bTriggered10s = true;

        BehaviorRecord stRecord;
        stRecord.bState         = true;
        stRecord.strTriggerTime = calcElapsedTime(nNow);
        stRecord.eBehaviorType  = enType;
        m_vTimeline.push_back(stRecord);

        if (m_callback)
        {
            m_callback(stRecord);
        }
    }
}

void CBehaviorCalculator::endBehavior(PlatformBehaviorType_E enType, int64_t nNow)
{
    auto it = m_mapBehaviorState.find(enType);
    if (it == m_mapBehaviorState.end() || !it->second.bActive)
        return;

    BehaviorState_S &stState = it->second;

    /* 累计该行为的持续时长 */
    int64_t nDuration = nNow - stState.nStartTime;
    m_mapDurationAccum[enType] += nDuration;

    /* 仅当已触发过开始回调时才记录结束事件 */
    if (stState.bTriggered2s)
    {
        BehaviorRecord stRecord;
        stRecord.bState         = false;
        stRecord.strTriggerTime = calcElapsedTime(nNow);
        stRecord.eBehaviorType  = enType;
        m_vTimeline.push_back(stRecord);
        if (m_callback)
        {
            m_callback(stRecord);
        }
    }

    stState.bActive = false;
}

bool CBehaviorCalculator::isListenToTalk(const BehaviorCnt_S &stCnt, int nPresentCount) const
{
    const float threshold = 0.5;
    int         cnt       = stCnt.nRaisingHeadCnt;
    float       t         = static_cast<float>(cnt) / nPresentCount;

    dlog_debug("听讲-抬头： %d/%d (size/total) threshold %f\n", cnt, nPresentCount, threshold);
    if (t >= threshold)
    {
        return true;
    }
    return false;
}

bool CBehaviorCalculator::isPraxis(const BehaviorCnt_S &stCnt, int nPresentCount) const
{
    const float threshold = 0.1;
    int         cnt       = stCnt.nRaisingHandCnt;
    float       t         = static_cast<float>(cnt) / nPresentCount;

    dlog_debug("实践-举手： %d/%d (size/total) threshold %f\n", cnt, nPresentCount, threshold);
    if (t >= threshold)
    {
        return true;
    }
    return false;
}

bool CBehaviorCalculator::isDemonstration(const BehaviorCnt_S &stCnt, int nPresentCount) const
{
    const float threshold = 0.5;
    int         cnt       = stCnt.nStandingCnt;
    float       t         = static_cast<float>(cnt) / nPresentCount;

    dlog_debug("演示-站立： %d/%d (size/total) threshold %f\n", cnt, nPresentCount, threshold);
    if (t >= threshold)
    {
        return true;
    }
    return false;
}

bool CBehaviorCalculator::isRead(const BehaviorCnt_S &stCnt, int nPresentCount) const
{
    const float threshold = 0.5;
    int         cnt       = stCnt.nBendingHeadCnt;
    float       t         = static_cast<float>(cnt) / nPresentCount;

    dlog_debug("阅读-低头： %d/%d (size/total) threshold %f\n", cnt, nPresentCount, threshold);
    if (t >= threshold)
    {
        return true;
    }
    return false;
}

bool CBehaviorCalculator::isDiscussion(const BehaviorCnt_S &stCnt, int nPresentCount) const
{
    const float threshold = 0.5;
    int         cnt       = stCnt.nTurningHeadCnt + stCnt.nTurningBodyCnt;
    float       t         = static_cast<float>(cnt) / nPresentCount;

    dlog_debug("讨论-转头/转身： %d/%d (size/total) threshold %f\n", cnt, nPresentCount, threshold);
    if (t >= threshold)
    {
        return true;
    }
    return false;
}

#endif
