/**
 * @FilePath     : stu_track.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-03 11:15:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 11:15:00
 * @Description  : 学生机位轨迹累加器
 */

#include "stu_track.h"

#include <cstdio>
#include <cstring>

#include "dlog.h"

namespace Classroom
{
const char *stuTrackWhyName(uint32_t u32WhyBit)
{
    switch (u32WhyBit)
    {
        case CLASSROOM_STU_WHY_STAND: return "stand";
        case CLASSROOM_STU_WHY_BACK:  return "back";
        default:                      return "bad";
    }
}

const char *stuTrackGateName(int32_t nGate)
{
    switch (nGate)
    {
        case STU_TRACK_GATE_NONE: return "none";
        case STU_TRACK_GATE_BUSY: return "busy";
        case STU_TRACK_GATE_FREE: return "free";
        default:                  return "bad";
    }
}

int32_t CStuTrack::indexOf(int32_t nPersonId) const
{
    if (nPersonId < 0)
    {
        return -1;   /* 没有跟踪号的人不进表：报上去平台连"这是同一个人"都判不了 */
    }
    for (int32_t i = 0; i < CLASSROOM_STU_TRACK_PERSON_MAX; ++i)
    {
        if (m_astRow[i].nPersonId == nPersonId)
        {
            return i;
        }
    }
    return -1;
}

int32_t CStuTrack::rowFor(int32_t nPersonId)
{
    const int32_t nExist = indexOf(nPersonId);
    if (nExist >= 0)
    {
        return nExist;
    }

    for (int32_t i = 0; i < CLASSROOM_STU_TRACK_PERSON_MAX; ++i)
    {
        if (m_astRow[i].nPersonId < 0)
        {
            m_astRow[i] = CStuTrackPerson_S();
            m_astRow[i].nPersonId = nPersonId;
            return i;
        }
    }

    /* 满表：让"最久没有新点"的那条线走。为什么不是让第一条（先到先得）：
       表里的行序就是领号顺序，按序让位会让第一排那个一直站着的人独占一条线到底，
       而刚进画面的那个人永远记不上——分布偏到一侧且心跳上看不出来。
       按最后更新让位则总是牺牲"最不活跃的"那条，而他本来也不会再有新点进来 */
    int32_t nStale = 0;
    int64_t llStale = 0;
    for (int32_t i = 0; i < CLASSROOM_STU_TRACK_PERSON_MAX; ++i)
    {
        const CStuTrackPerson_S &stRow = m_astRow[static_cast<size_t>(i)];
        const int64_t llLast = (stRow.nPtNum > 0)
                               ? stRow.astPt[static_cast<size_t>(stRow.nPtNum - 1)].llMonoMs : 0;
        if (0 == i || llLast < llStale)
        {
            llStale = llLast;
            nStale = i;
        }
    }
    dlog_warn("classroom stu_track: %d 行满，挤掉最久未更新的人 %d (last %lldms)",
              CLASSROOM_STU_TRACK_PERSON_MAX, m_astRow[static_cast<size_t>(nStale)].nPersonId,
              static_cast<long long>(llStale));
    ++m_nEvictPeriod;
    ++m_u64EvictTotal;
    m_astRow[static_cast<size_t>(nStale)] = CStuTrackPerson_S();
    m_astRow[static_cast<size_t>(nStale)].nPersonId = nPersonId;
    return nStale;
}

void CStuTrack::appendPoint(CStuTrackPerson_S &rstRow, int64_t llNowMs, float fX, float fY)
{
    if (rstRow.nPtNum >= CLASSROOM_STU_TRACK_POINT_MAX)
    {
        /* 抽稀：保住**最早那一点**、牺牲次旧那一点。于是每条线永远含他开始的那一刻，
           退化只发生在中段（而中段每个点都带绝对时刻，平台从间距就读得出稀到哪一档）。
           丢最旧的替代做法会把一个人整段的前期悄悄抹掉——报表上看不出少了一段，
           而轨迹"从哪儿开始走"恰恰是这功能唯一要回答的话 */
        for (int32_t i = 1; i + 1 < rstRow.nPtNum; ++i)
        {
            rstRow.astPt[static_cast<size_t>(i)] = rstRow.astPt[static_cast<size_t>(i + 1)];
        }
        --rstRow.nPtNum;
        ++m_nThinPeriod;
        ++m_u64ThinTotal;
    }
    CStuTrackPoint_S &stPt = rstRow.astPt[static_cast<size_t>(rstRow.nPtNum)];
    stPt.llMonoMs = llNowMs;
    stPt.fNormX = fX;
    stPt.fNormY = fY;
    ++rstRow.nPtNum;
    ++m_u64Points;
}

void CStuTrack::fold(const CStuTrackObs_S *patObs, int32_t nNum, int32_t nGate, int64_t llNowMs)
{
    ++m_u64Folds;
    m_nGate = nGate;

    if (STU_TRACK_GATE_FREE != nGate)
    {
        /* 门没开（讲台有人，或讲台压根没画）：不产点，也**不清账**。
           清账会把上一拍到现在的点抹掉，而那些点还没报过——需求要的是"这一周期内走过哪"，
           周期里门开关了几次是常事，一关就把攒下的半截轨迹扔掉等于自己制造空洞 */
        return;
    }

    if (nNum <= 0)
    {
        return;   /* 空场：这一拍没有任何人可记，门的状态已经记在上面了 */
    }
    if (nullptr == patObs)
    {
        return;   /* 有行数却没表是调用方的错，不猜——少一拍点比读野指针便宜 */
    }

    for (int32_t nRow = 0; nRow < nNum; ++nRow)
    {
        const CStuTrackObs_S &stObs = patObs[static_cast<size_t>(nRow)];
        const uint32_t u32Why = (stObs.bStand ? CLASSROOM_STU_WHY_STAND : 0U) |
                                (stObs.bBack ? CLASSROOM_STU_WHY_BACK : 0U);
        if (stObs.nPersonId < 0)
        {
            /* 没有跟踪号的人**必须先拒**，而不是照样占一条线：下面 rowFor 会给他一整行，
               而 snapshot 又按"id<0 不占报表位置"把他跳过——结果是这一行既不出现在报文里、
               又吃掉了八条线里的一条。少一个人是可数的（evict），
               而"八条线里有一条永远不上报"是不可数的 */
            continue;
        }
        if (!stObs.bStudent || 0 == u32Why)
        {
            continue;   /* 不是这条链要的人：坐着且面向镜头的学生、以及确认归属的教师 */
        }

        if (!(stObs.fNormX >= 0.0f && stObs.fNormX <= 1.0f) ||
            !(stObs.fNormY >= 0.0f && stObs.fNormY <= 1.0f))
        {
            ++m_nNoPosPeriod;
            ++m_u64NoPosTotal;
            continue;
        }
        CStuTrackPerson_S &stRow = m_astRow[static_cast<size_t>(rowFor(stObs.nPersonId))];
        stRow.u32Why |= u32Why;
        appendPoint(stRow, llNowMs, stObs.fNormX, stObs.fNormY);
    }
}

void CStuTrack::snapshot(CStuTrackSnap_S &rsnapOut)
{
    rsnapOut = CStuTrackSnap_S();
    rsnapOut.nGate = m_nGate;

    for (int32_t i = 0; i < CLASSROOM_STU_TRACK_PERSON_MAX; ++i)
    {
        const CStuTrackPerson_S &stRow = m_astRow[static_cast<size_t>(i)];
        if (stRow.nPersonId < 0 || stRow.nPtNum <= 0)
        {
            continue;   /* 空行与"领了号但一个点都没落上"的行都不占报表里的位置 */
        }
        rsnapOut.astPerson[static_cast<size_t>(rsnapOut.nPersonNum)] = stRow;
        ++rsnapOut.nPersonNum;
    }
    rsnapOut.nEvictNum = m_nEvictPeriod;
    rsnapOut.nThinNum = m_nThinPeriod;
    rsnapOut.nNoPosNum = m_nNoPosPeriod;

    /* 快照即清（队列语义，与 vecEvents 同）：同一条线不会在两份报表里出现两次。
       刻意不清累计计数与 m_nGate——门的状态是"此刻的事实"，不是本周期攒下的数 */
    for (int32_t i = 0; i < CLASSROOM_STU_TRACK_PERSON_MAX; ++i)
    {
        m_astRow[static_cast<size_t>(i)] = CStuTrackPerson_S();
    }
    m_nEvictPeriod = 0;
    m_nThinPeriod = 0;
    m_nNoPosPeriod = 0;
}

void CStuTrack::reset()
{
    for (int32_t i = 0; i < CLASSROOM_STU_TRACK_PERSON_MAX; ++i)
    {
        m_astRow[static_cast<size_t>(i)] = CStuTrackPerson_S();
    }
    m_nEvictPeriod = 0;
    m_nThinPeriod = 0;
    m_nNoPosPeriod = 0;
}

int32_t CStuTrack::rowCount() const
{
    int32_t nNum = 0;
    for (int32_t i = 0; i < CLASSROOM_STU_TRACK_PERSON_MAX; ++i)
    {
        if (m_astRow[static_cast<size_t>(i)].nPersonId >= 0)
        {
            ++nNum;
        }
    }
    return nNum;
}

std::string CStuTrack::statText() const
{
    char szBuf[192] = {0};
    const int nLen = snprintf(szBuf, sizeof(szBuf),
                              "strk gate=%s fold=%llu pt=%llu row=%d "
                              "evict=%d|%llu thin=%d|%llu nopos=%d|%llu",
                              stuTrackGateName(m_nGate),
                              static_cast<unsigned long long>(m_u64Folds),
                              static_cast<unsigned long long>(m_u64Points),
                              rowCount(),
                              m_nEvictPeriod, static_cast<unsigned long long>(m_u64EvictTotal),
                              m_nThinPeriod, static_cast<unsigned long long>(m_u64ThinTotal),
                              m_nNoPosPeriod, static_cast<unsigned long long>(m_u64NoPosTotal));
    if (nLen < 0 || nLen >= static_cast<int>(sizeof(szBuf)))
    {
        return "strk stat=truncated";
    }
    return std::string(szBuf);
}

}/* namespace Classroom */
