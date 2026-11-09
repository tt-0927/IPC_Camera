/**
 * @FilePath     : scene_infer.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-02 09:34:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 09:34:00
 * @Description  : 课堂场景时段推断实现（只有优先级、滞回与最短段三件事，没有任何新判据）
 */

#include "scene_infer.h"

#include <cstdio>

namespace Classroom
{
const char *CSceneInfer::sceneName(Scene_E enScene)
{
    switch (enScene)
    {
        case Scene_E::SCENE_TEACHING: return "teaching";
        case Scene_E::SCENE_QA:       return "qa";
        case Scene_E::SCENE_DISCUSS:  return "discuss";
        case Scene_E::SCENE_PRESENT:  return "present";
        case Scene_E::SCENE_PRACTICE: return "practice";
        case Scene_E::SCENE_UNKNOWN:  return "unknown";
        default:                      return "bad";   /* 与 teacherAreaName 同一条理由：
                                                        unknown 会让人去查区域与阈值，bad 才知道是枚举与实现被分开改过 */
    }
}

void CSceneInfer::setParam(const CSceneParam_S &stParam)
{
    /* 换参数不动已开的段：现场把 nEnterMs 从 30s 调到 10s 不该让正在计时的这一段凭空重开。
       反过来也不补历史——段与段之间的边界一旦落定就不再动，报表上"什么时候切的"必须可复现 */
    m_stParam = stParam;
    if (m_vecClosed.empty())
    {
        m_vecClosed.resize(CLASSROOM_SCENE_RING);   /* 首次用时定容，之后只覆写不扩容 */
    }
}

Scene_E CSceneInfer::decide(const CSceneFact_S &stFact) const
{
    /* 优先级：展示 > 讨论 > 问答 > 教授 > 练习。理由见头文件——少数人的动作赢，
       因为时段要能解释"这段课为什么被切成两半"，而全班都在抬头解释不了任何事 */
    if (stFact.nPresentNum > 0)
    {
        return Scene_E::SCENE_PRESENT;
    }

    if (1 == stFact.nDiscussPeriod ||
        stFact.nDiscussNum >= m_stParam.nDiscussMinPersons)
    {
        return Scene_E::SCENE_DISCUSS;
    }
    if (stFact.bInteractOn)
    {
        return Scene_E::SCENE_QA;
    }
    /* 举手密度只在**有分母**时才算。分母没测时拿 0 去除会得"没人举手"，
       而真值是"这一拍没证据"——这正是群体层那条口径在这层的复用 */
    const bool bRaiseOk = (stFact.nEvidenceNum > 0) &&
                          (static_cast<float>(stFact.nRaiseNum) >=
                           m_stParam.fRaiseRatioQa * static_cast<float>(stFact.nEvidenceNum));
    if (bRaiseOk)
    {
        return Scene_E::SCENE_QA;
    }
    const bool bListenOk = (1 == stFact.nListenPeriod) || !m_stParam.bTeachingNeedsListen;
    if (stFact.bTeachingOn && bListenOk)
    {
        return Scene_E::SCENE_TEACHING;
    }
    const bool bPracticeOk = (stFact.nEvidenceNum > 0) &&
                             (static_cast<float>(stFact.nHeadDownNum) >=
                              m_stParam.fHeadDownRatioPractice * static_cast<float>(stFact.nEvidenceNum));
    if (bPracticeOk)
    {
        return Scene_E::SCENE_PRACTICE;
    }
    /* 一条都不成立：判不出。课间、无人、时间片空窗、以及"老师在讲台上但听讲时段没成立"
       都落在这一档——把它们硬归进"知识教授"是本层最容易犯也最难被发现的一个错 */
    return Scene_E::SCENE_UNKNOWN;
}

void CSceneInfer::feed(const CSceneFact_S &stFact)
{
    if (m_vecClosed.empty())
    {
        m_vecClosed.resize(CLASSROOM_SCENE_RING);
    }
    if (!m_stParam.bEnable)
    {
        ++m_stStat.u64Skipped;
        return;
    }
    if (stFact.llNowMs <= 0 || stFact.llNowMs < m_llLastMs)
    {
        /* 时间戳不回退是这一层唯一能守住"段的长度是正的"的前提。回退一拍就关掉一段
           又重开一段，报表上会长出一段负时长的时段——那比少一段难查得多，所以宁可不喂 */
        ++m_stStat.u64NoFact;
        return;
    }
    const int64_t llPrevMs = m_llLastMs;
    m_llLastMs = stFact.llNowMs;
    ++m_stStat.u64Fed;

    const Scene_E enCand = decide(stFact);
    if (!bHaveSegment)
    {
        /* 第一拍无条件开一段（哪怕是 UNKNOWN）：不这样做的话"这节课第一段时间"会凭空缺失，
           而平台按整节课算占比时那几分钟会谁都不属于 */
        bHaveSegment = true;
        m_enCurrent = enCand;
        m_stOpen.enScene = enCand;
        m_stOpen.llBeginMs = stFact.llNowMs;
        m_stOpen.llEndMs = 0;
        m_stOpen.nPersonNum = (stFact.nEvidenceNum > 0) ? stFact.nEvidenceNum : 0;
        m_stOpen.nBoardNum = stFact.bBoardEdge ? 1 : 0;
        m_llAgainstSinceMs = 0;
        m_u64AgainstMs = 0;
        m_stStat.nCurrent = static_cast<int32_t>(enCand);
        return;
    }

    /* 本拍**之前**的两项读数：反证从这一拍开始时，切段边界就落在这一拍，
       段内读数也要以这两个值为准（下面 !bResume 分支存快照） */
    const int32_t nPersonPre = m_stOpen.nPersonNum;
    const int32_t nBoardPre = m_stOpen.nBoardNum;

    if (stFact.bBoardEdge)
    {
        ++m_stOpen.nBoardNum;   /* 段内的板书次数是"这段老师在讲"的旁证，也是报表上唯一能看出来的量 */
    }
    if (stFact.nEvidenceNum > 0)
    {
        m_stOpen.nPersonNum = stFact.nEvidenceNum;   /* 只留最后一次看到的分母：人数是"这段课有多少人"的读数 */
    }

    if (enCand == m_enCurrent)
    {
        /* 一致的一拍：反证累计清零的条件是**中间隔得够久**（下面那支），这里只是把
           "最近一次反证"的时刻推远，让下一次反证能看出隔了多久 */
        m_llLastAgainstMs = 0;
        m_llAgainstSinceMs = 0;
        m_u64AgainstMs = 0;
        return;
    }

    /* 反证的一拍。三段累计：
       - 上一拍不是反证、或两拍之间隔超过 nExitMs → 这一轮反证从头累计，边界也挪到这里；
       - 否则继续累计，累计到 nEnterMs 才真的切段。
       两个数各管一件事：nEnterMs 管"要连续多久才算换场景"，nExitMs 管"隔多久就算断了一次"。
       判"是不是接着上一轮"用的是**上一拍反证与否**（m_llLastAgainstMs），不是"已经累计了多少"——
       后者会让第二轮的第二拍被当成新起点，段边界往前滑一拍，enter_ms 也就跟着晚一拍 */
    const bool bResume = (0 != m_llLastAgainstMs) &&
                         ((stFact.llNowMs - m_llLastAgainstMs) <= m_stParam.nExitMs);
    if (!bResume)
    {
        m_u64AgainstMs = 0;
        m_llAgainstSinceMs = stFact.llNowMs;
        m_nPersonAtAgainst = nPersonPre;
        m_nBoardAtAgainst = nBoardPre;
    }
    else
    {
        m_u64AgainstMs += static_cast<uint64_t>(stFact.llNowMs - llPrevMs);
    }
    m_llLastAgainstMs = stFact.llNowMs;
    ++m_stStat.u64Changed;

    if (m_u64AgainstMs < static_cast<uint64_t>(m_stParam.nEnterMs))
    {
        return;   /* 还没攒够：当前段继续，什么都不切。滞回挡掉的抖动就是在这里计的 */
    }

    /* 认账：把当前段闭合到**第一次反证的那一刻**（不是现在）。切在"承认"的时刻会让每段
       都比真相晚 nEnterMs，报表上的时段边界于是永远对齐不到课程表——而这是验收时要对的东西。
       段内两项读数同样回到那一刻：人数取第一拍反证**之前**的读数，板书取那一刻的计数；
       反证窗口里新攒的板书属于**下一段**（它在时间上落在切点之后），不清零就会两边重复计 */
    const int64_t llCutMs = (m_llAgainstSinceMs > m_stOpen.llBeginMs) ? m_llAgainstSinceMs : stFact.llNowMs;
    const int32_t nPersonNow = m_stOpen.nPersonNum;
    const int32_t nBoardNow = m_stOpen.nBoardNum;
    CSceneSegment_S stDone = m_stOpen;
    stDone.nPersonNum = m_nPersonAtAgainst;
    stDone.nBoardNum = m_nBoardAtAgainst;
    stDone.llEndMs = llCutMs;
    appendClosed(stDone);

    m_enCurrent = enCand;
    m_stOpen = CSceneSegment_S();
    m_stOpen.enScene = enCand;
    m_stOpen.llBeginMs = llCutMs;
    m_stOpen.llEndMs = 0;
    m_stOpen.nPersonNum = nPersonNow;
    m_stOpen.nBoardNum = (nBoardNow >= m_nBoardAtAgainst) ? (nBoardNow - m_nBoardAtAgainst) : 0;
    m_u64AgainstMs = 0;
    m_llAgainstSinceMs = 0;
    m_llLastAgainstMs = 0;
    m_nPersonAtAgainst = 0;
    m_nBoardAtAgainst = 0;
    m_stStat.nCurrent = static_cast<int32_t>(enCand);
}

void CSceneInfer::appendClosed(const CSceneSegment_S &stSeg)
{
    const int64_t llLenMs = stSeg.llEndMs - stSeg.llBeginMs;
    if (llLenMs < m_stParam.nMinSegmentMs && m_nNum > 0)
    {
        /* 太短的段并进前一段（延长它的结束时刻），而不是单独成段：
           三秒的段在"讨论占 12 分钟"这类占比读数里是纯噪声，而它会把一节课切成几十段。
           前一段的场景与这段不同也照样并——**这里要的是"下一段开始之前一直在上一段里"**，
           不是"同场景才合并"。合并后前一段的时长会略高于真实值，代价记在 u64MergedShort 里 */
        /* 前一条的下标是 head+num-1 而不是 head-1：环没满时 head-1 落在**尾巴那端还没写过的空位**
           （只有环满时两者才重合），于是"合并"写进了一条不存在的段，前一段的结束时刻没变、
           闭合数也不涨——现象正好是"短段被静默丢掉了"，与本意相反 */
        const int32_t nLast = (m_nHead + m_nNum - 1) % CLASSROOM_SCENE_RING;
        CSceneSegment_S &stLast = m_vecClosed[static_cast<size_t>(nLast)];
        stLast.llEndMs = stSeg.llEndMs;
        if (stLast.nPersonNum < stSeg.nPersonNum)
        {
            stLast.nPersonNum = stSeg.nPersonNum;
        }
        stLast.nBoardNum += stSeg.nBoardNum;
        ++m_stStat.u64MergedShort;
        return;
    }
    if (m_nNum >= CLASSROOM_SCENE_RING)
    {
        /* 环满丢最旧：与事件环同一条口径——宁可丢历史也不覆盖调用方的游标语义 */
        m_nHead = (m_nHead + 1) % CLASSROOM_SCENE_RING;
        --m_nNum;
        ++m_stStat.u64DropOldest;
    }
    const int32_t nTail = (m_nHead + m_nNum) % CLASSROOM_SCENE_RING;
    m_vecClosed[static_cast<size_t>(nTail)] = stSeg;
    ++m_nNum;
    ++m_stStat.u64Closed;
}

bool CSceneInfer::current(CSceneSegment_S &rstOut) const
{
    if (!bHaveSegment)
    {
        return false;
    }
    rstOut = m_stOpen;
    return true;
}

void CSceneInfer::closeSession(int64_t llEndMs)
{
    if (!bHaveSegment)
    {
        return;
    }
    CSceneSegment_S stDone = m_stOpen;
    stDone.llEndMs = (llEndMs > stDone.llBeginMs) ? llEndMs : stDone.llBeginMs;
    appendClosed(stDone);
    /* 收尾后不留开的段：下一节课从"还没有段"开始，第一拍再开新段。
       留着的话那段会跨下课与上课，报表上就是一段"凌晨三点还在知识教授" */
    bHaveSegment = false;
    m_enCurrent = Scene_E::SCENE_UNKNOWN;
    m_stOpen = CSceneSegment_S();
    m_u64AgainstMs = 0;
    m_llAgainstSinceMs = 0;
    m_llLastAgainstMs = 0;
    m_nPersonAtAgainst = 0;
    m_nBoardAtAgainst = 0;
    m_stStat.nCurrent = static_cast<int32_t>(Scene_E::SCENE_UNKNOWN);
}

void CSceneInfer::reset()
{
    m_nHead = 0;
    m_nNum = 0;
    m_llLastMs = 0;
    bHaveSegment = false;
    m_enCurrent = Scene_E::SCENE_UNKNOWN;
    m_stOpen = CSceneSegment_S();
    m_u64AgainstMs = 0;
    m_llAgainstSinceMs = 0;
    m_llLastAgainstMs = 0;
    m_nPersonAtAgainst = 0;
    m_nBoardAtAgainst = 0;
    m_stStat = CSceneStat_S();
}

std::vector<CSceneSegment_S> CSceneInfer::takeClosed(int32_t nMax)
{
    const int32_t nWant = (nMax > 0) ? nMax : 1;
    const int32_t nTake = (m_nNum < nWant) ? m_nNum : nWant;
    std::vector<CSceneSegment_S> vecOut;
    vecOut.reserve(static_cast<size_t>(nTake));
    for (int32_t i = 0; i < nTake; ++i)
    {
        const int32_t nIdx = (m_nHead + i) % CLASSROOM_SCENE_RING;
        vecOut.push_back(m_vecClosed[static_cast<size_t>(nIdx)]);
        m_vecClosed[static_cast<size_t>(nIdx)] = CSceneSegment_S();
    }
    /* 只把取走的那几条挪出环：剩下的下次继续取。这里的"剩下"不是异常路径——
       一节课切得细的时候它是常态，所以取数端不能假设环已经空了 */
    m_nHead = (m_nHead + nTake) % CLASSROOM_SCENE_RING;
    m_nNum -= nTake;
    if (0 == m_nNum)
    {
        m_nHead = 0;
    }
    return vecOut;
}

int32_t CSceneInfer::closedNum() const
{
    return m_nNum;
}

std::string CSceneInfer::statText() const
{
    char szBuf[192] = {0};
    const int nLen = snprintf(szBuf, sizeof(szBuf),
                              "scene fed=%llu skip=%llu chg=%llu closed=%llu merge=%llu drop=%llu bad=%llu cur=%s pend=%d hold=%llu",
                              static_cast<unsigned long long>(m_stStat.u64Fed),
                              static_cast<unsigned long long>(m_stStat.u64Skipped),
                              static_cast<unsigned long long>(m_stStat.u64Changed),
                              static_cast<unsigned long long>(m_stStat.u64Closed),
                              static_cast<unsigned long long>(m_stStat.u64MergedShort),
                              static_cast<unsigned long long>(m_stStat.u64DropOldest),
                              static_cast<unsigned long long>(m_stStat.u64NoFact),
                              sceneName(m_enCurrent),
                              (m_u64AgainstMs > 0) ? 1 : 0,
                              static_cast<unsigned long long>(m_u64AgainstMs));
    if (nLen < 0 || static_cast<size_t>(nLen) >= sizeof(szBuf))
    {
        return "scene stat=truncated";
    }
    return std::string(szBuf);
}
}
