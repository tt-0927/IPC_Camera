/**
 * @FilePath     : event_merger.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-20 10:20:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 10:20:00
 * @Description  : 行为事件合并器实现
 */

#include "event_merger.h"

#include <cmath>
#include <cstdio>

#include "dlog.h"

namespace Classroom
{
bool eventBoxNear(const CRectf_S &stA, const CRectf_S &stB, float fDistRatio)
{
    const float fWA = stA.width();
    const float fHA = stA.height();
    const float fWB = stB.width();
    const float fHB = stB.height();
    if (fWA <= 0.0f || fHA <= 0.0f || fWB <= 0.0f || fHB <= 0.0f || fDistRatio <= 0.0f)
    {
        return false;
    }

    const float fDX = 0.5f * (stA.fX1 + stA.fX2) - 0.5f * (stB.fX1 + stB.fX2);
    const float fDY = 0.5f * (stA.fY1 + stA.fY2) - 0.5f * (stB.fY1 + stB.fY2);
    const float fDist = std::sqrt(fDX * fDX + fDY * fDY);
    /* 参照尺寸取两框对角线的均值：同一个头两次检测的框尺寸本来就有抖动，
       拿单边当尺子会因「哪一边大一点」而左右摇摆 */
    const float fRef = 0.5f * (std::sqrt(fWA * fWA + fHA * fHA) +
                               std::sqrt(fWB * fWB + fHB * fHB));
    return fDist <= fRef * fDistRatio;
}

void CEventMerger::init(const CEventMergerParam_S &stParam)
{
    m_stParam = stParam;
    reset();
}

void CEventMerger::reset()
{
    for (int32_t i = 0; i < CLASSROOM_MERGE_PENDING_MAX; ++i)
    {
        m_astPending[i] = CMergeEntry_S();
    }
    m_u32OutHead = 0;
    m_u32OutNum = 0;
    m_stStat = CEventMergerStat_S();
    m_bReady = true;
}

int32_t CEventMerger::findPairable(const CBehaviorEvent_S &stBegin)
{
    /* 212 教师跟踪**永不跨 ID 并段** */
    if (Behavior_E::TEA_TRACK == stBegin.enBehavior)
    {
        return -1;
    }

    for (int32_t i = 0; i < CLASSROOM_MERGE_PENDING_MAX; ++i)
    {
        const CMergeEntry_S &stEntry = m_astPending[i];
        if (!stEntry.bUsed || !stEntry.bEndValid ||
            stEntry.enBehavior != stBegin.enBehavior ||
            stEntry.nLastId == stBegin.nPersonId)
        {
            /* 同 id 不并：那是「真离开又回来」，两条都是真事件 */
            continue;
        }
        if (stEntry.stPendingEnd.bSubjectCut)
        {
            /* 这条 END 是主体翻转收的尾（教师交接/角色改判），不是丢检换 ID */
            ++m_stStat.u64SealBlock;
            continue;
        }

        const int64_t llGap = stBegin.llMs - stEntry.stPendingEnd.llMs;
        /* 上界=窗口：挂起的 END 只等这么久。下界取负窗口做合理性兜底：
           BEGIN 的 llMs 回推到本轮第一个 TRUE，慢门限的行为会回推到挂起 END 之前，
           两条「自称重叠」的同位置不同人片段恰恰是翻转的铁证，一样并；
           但差出半个窗口以上的时间戳只能是乱序脏数据，不接 */
        if (llGap > static_cast<int64_t>(m_stParam.nWindowMs) ||
            llGap < -static_cast<int64_t>(m_stParam.nWindowMs))
        {
            continue;
        }
        if (!eventBoxNear(stEntry.stPendingEnd.stHeadBox, stBegin.stHeadBox, m_stParam.fDistRatio))
        {
            continue;
        }
        return i;
    }
    return -1;
}

int32_t CEventMerger::findChainEnd(const CBehaviorEvent_S &stEnd)
{
    for (int32_t i = 0; i < CLASSROOM_MERGE_PENDING_MAX; ++i)
    {
        const CMergeEntry_S &stEntry = m_astPending[i];
        /* 只认「链开着」（已吞掉接手 BEGIN、bEndValid 已清）的槽：
           bEndValid=true 的槽里那条 END 有自己的归属，别拿第二条 END 去覆盖它 */
        if (stEntry.bUsed && !stEntry.bEndValid &&
            stEntry.enBehavior == stEnd.enBehavior &&
            stEntry.nLastId == stEnd.nPersonId)
        {
            return i;
        }
    }
    return -1;
}

int32_t CEventMerger::oldestSlot() const
{
    int32_t nOld = -1;
    int64_t llOldest = 0;
    for (int32_t i = 0; i < CLASSROOM_MERGE_PENDING_MAX; ++i)
    {
        const CMergeEntry_S &stEntry = m_astPending[i];
        if (stEntry.bUsed && stEntry.bEndValid)
        {
            if (nOld < 0 || stEntry.stPendingEnd.llMs < llOldest)
            {
                nOld = i;
                llOldest = stEntry.stPendingEnd.llMs;
            }
        }
    }
    return nOld;
}

void CEventMerger::sealPending(int32_t nPersonId)
{
    /* 主体刚翻转：这个人挂在窗口里的 END 属于旧身份。哪怕它是证据先断、门自己收的尾
       （不带 bSubjectCut），也不能再接继任者的 BEGIN —— 那是换人，不是换 ID */
    for (int32_t i = 0; i < CLASSROOM_MERGE_PENDING_MAX; ++i)
    {
        CMergeEntry_S &stEntry = m_astPending[i];
        if (stEntry.bUsed && stEntry.bEndValid && stEntry.nLastId == nPersonId &&
            !stEntry.stPendingEnd.bSubjectCut)
        {
            stEntry.stPendingEnd.bSubjectCut = true;
            ++m_stStat.u64SealMark;
        }
    }
}

void CEventMerger::emit(const CBehaviorEvent_S &stEvent)
{
    /* 输出环按容量丢最旧：它只是 feed/tick/flush 与引擎排空之间的一小段缓冲，
       引擎在每次 feed/tick 之后立刻排空，正常永远用不到两位数的深度 */
    if (m_u32OutNum >= CLASSROOM_MERGE_OUT_RING)
    {
        m_u32OutHead = (m_u32OutHead + 1) % CLASSROOM_MERGE_OUT_RING;
        --m_u32OutNum;
    }
    m_astOut[(m_u32OutHead + m_u32OutNum) % CLASSROOM_MERGE_OUT_RING] = stEvent;
    ++m_u32OutNum;
}

void CEventMerger::emitMerged(CMergeEntry_S &rstEntry)
{
    CBehaviorEvent_S stOut = rstEntry.stPendingEnd;
    stOut.nPersonId = rstEntry.nFirstId;
    stOut.llHeldMs = rstEntry.llSumHeldMs + rstEntry.stPendingEnd.llHeldMs;
    emit(stOut);
    ++m_stStat.u64EndOut;
    rstEntry = CMergeEntry_S();
}

void CEventMerger::feed(const CBehaviorEvent_S &stEvent)
{
    if (!m_bReady || stEvent.nPersonId < 0)
    {
        return;
    }
    ++m_stStat.u64Fed;

    if (stEvent.bBegin)
    {
        const int32_t nIdx = findPairable(stEvent);
        if (nIdx >= 0)
        {
            CMergeEntry_S &stEntry = m_astPending[nIdx];
            stEntry.llSumHeldMs += stEntry.stPendingEnd.llHeldMs;
            stEntry.nLastId = stEvent.nPersonId;
            stEntry.stPendingEnd = CBehaviorEvent_S();
            stEntry.bEndValid = false;
            ++m_stStat.u64BeginSwallow;
            dlog_info("classroom merge: 吞并开始事件 目标=%d %s 开机毫秒=%lld "
                      "(链首目标=%d 累计=%lldms)",
                      stEvent.nPersonId, behaviorCnName(stEvent.enBehavior),
                      static_cast<long long>(stEvent.llMs),
                      stEntry.nFirstId, static_cast<long long>(stEntry.llSumHeldMs));
            return;
        }
        emit(stEvent);
        ++m_stStat.u64BeginPass;
        return;
    }

    /* END 先认链：上一段已吞过接手 BEGIN 的槽在等它 */
    const int32_t nChain = findChainEnd(stEvent);
    if (nChain >= 0)
    {
        CMergeEntry_S &stEntry = m_astPending[nChain];
        stEntry.stPendingEnd = stEvent;
        stEntry.bEndValid = true;
        ++m_stStat.u64EndIn;
        return;
    }

    int32_t nSlot = -1;
    for (int32_t i = 0; i < CLASSROOM_MERGE_PENDING_MAX; ++i)
    {
        if (!m_astPending[i].bUsed)
        {
            nSlot = i;
            break;
        }
    }
    if (nSlot < 0)
    {
        /* 表满：放行挂得最旧的一条再占用。满座同拍集体结束行为时才可能发生，
           丢的是「合并形」不是「事件本身」——原样放行的那条仍是真事件 */
        nSlot = oldestSlot();
        if (nSlot < 0)
        {
            /* 全表都是开着链的槽（极端：60 人全在翻转中）——无处可挂，丢这条 END 并计数。
               它的 BEGIN 已在上游，平台侧表现为一条未结束的事件，与环满丢 END 同形状 */
            ++m_stStat.u64Evict;
            dlog_warn("classroom merge: 待并表全被未闭合链占满，结束事件被丢弃 "
                      "(目标=%d %s)",
                      stEvent.nPersonId, behaviorCnName(stEvent.enBehavior));
            return;
        }
        emitMerged(m_astPending[nSlot]);
        ++m_stStat.u64Evict;
    }

    CMergeEntry_S &stEntry = m_astPending[nSlot];
    stEntry.bUsed = true;
    stEntry.enBehavior = stEvent.enBehavior;
    stEntry.nFirstId = stEvent.nPersonId;
    stEntry.nLastId = stEvent.nPersonId;
    stEntry.llSumHeldMs = 0;
    stEntry.stPendingEnd = stEvent;
    stEntry.bEndValid = true;
    ++m_stStat.u64EndIn;
}

void CEventMerger::tick(int64_t llNowMs)
{
    if (!m_bReady)
    {
        return;
    }
    for (int32_t i = 0; i < CLASSROOM_MERGE_PENDING_MAX; ++i)
    {
        CMergeEntry_S &stEntry = m_astPending[i];
        if (stEntry.bUsed && stEntry.bEndValid &&
            (llNowMs - stEntry.stPendingEnd.llMs) > static_cast<int64_t>(m_stParam.nWindowMs))
        {
            emitMerged(stEntry);
        }
    }
}

void CEventMerger::flushAll()
{
    if (!m_bReady)
    {
        return;
    }
    for (int32_t i = 0; i < CLASSROOM_MERGE_PENDING_MAX; ++i)
    {
        CMergeEntry_S &stEntry = m_astPending[i];
        /* 开着链的槽不动：它的 END 还在后面，收尾不该替它编一个结束时刻 */
        if (stEntry.bUsed && stEntry.bEndValid)
        {
            emitMerged(stEntry);
        }
    }
}

bool CEventMerger::pollEvent(CBehaviorEvent_S &stOut)
{
    if (0 == m_u32OutNum)
    {
        return false;
    }
    stOut = m_astOut[m_u32OutHead];
    m_u32OutHead = (m_u32OutHead + 1) % CLASSROOM_MERGE_OUT_RING;
    --m_u32OutNum;
    return true;
}

CEventMergerStat_S CEventMerger::stat() const
{
    CEventMergerStat_S stStat = m_stStat;
    int32_t nPending = 0;
    for (int32_t i = 0; i < CLASSROOM_MERGE_PENDING_MAX; ++i)
    {
        if (m_astPending[i].bUsed)
        {
            ++nPending;
        }
    }
    stStat.nPendingNum = nPending;
    return stStat;
}

std::string CEventMerger::statLine() const
{
    const CEventMergerStat_S stStat = stat();
    char szBuf[160] = {0};
    snprintf(szBuf, sizeof(szBuf),
             "merge fed=%llu bpass=%llu bswallow=%llu ein=%llu eout=%llu evict=%llu pend=%d",
             static_cast<unsigned long long>(stStat.u64Fed),
             static_cast<unsigned long long>(stStat.u64BeginPass),
             static_cast<unsigned long long>(stStat.u64BeginSwallow),
             static_cast<unsigned long long>(stStat.u64EndIn),
             static_cast<unsigned long long>(stStat.u64EndOut),
             static_cast<unsigned long long>(stStat.u64Evict),
             stStat.nPendingNum);
    return std::string(szBuf);
}
}
