/**
 * @FilePath     : seat_prior.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-27 13:57:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-27 13:57:00
 * @Description  : 「格子 → 座位」分配层实现
 */

#include "seat_prior.h"

#include <cmath>
#include <cstdio>

#include "attendance.h"

namespace Classroom
{
namespace
{
/* 见 CSlot_S 的三态 */
const int32_t SEAT_PRIOR_VACATED = -2;

/* 就近认领的门限 */
float seatPriorGate(float fCellW)
{
    return (fCellW > 0.0f) ? fCellW / static_cast<float>(CLASSROOM_SEAT_PRIOR_MAX_PER_CELL) : 0.0f;
}

/* 坐标有效性：与 cellOf 同一套 fail-safe 写法 */
float seatPriorX(float fNormX)
{
    if (!(fNormX >= 0.0f) || !(fNormX <= 1.0f))
    {
        return -1.0f;
    }
    return fNormX;
}

/* 本拍在画面里出现过吗 */
bool personSeen(const int32_t *panId, int32_t nNum, int32_t nPersonId)
{
    for (int32_t i = 0; i < nNum; ++i)
    {
        if (panId[i] == nPersonId)
        {
            return true;
        }
    }
    return false;
}

/* 领号的四档，数值就是优先序（小者胜） */
const int32_t SEAT_PRIOR_RANK_NONE = 0;
const int32_t SEAT_PRIOR_RANK_INHERIT = 1;
const int32_t SEAT_PRIOR_RANK_RECLAIM = 2;
const int32_t SEAT_PRIOR_RANK_FRESH = 3;
const int32_t SEAT_PRIOR_RANK_BORROW = 4;

/* "没有距离证据"的哨兵。归一化坐标两两之差不超过 1，而门限 = 格宽/每格席数 更小 */
const float SEAT_PRIOR_NO_DIST = 2.0f;
}


bool seatPriorParamOk(const CSeatPriorConf_S &rstConf, std::string &rstrErr)
{
    /* 0 合法：含义是"关掉跨 ID 继承，只维持已经领到的号"。调试时想知道
       "那些 inherit 到底是同一个人回来了，还是另一个人坐了过去"，就把它调成 0 看差值 */
    if (rstConf.nInheritBeats < 0 || rstConf.nInheritBeats > 1000)
    {
        rstrErr = "inherit_beats out of [0,1000]";
        return false;
    }
    if (rstConf.nReleaseBeats < 1 || rstConf.nReleaseBeats > 10000)
    {
        rstrErr = "release_beats out of [1,10000]";
        return false;
    }
    /* 这条是本层的命门，所以由参数自检硬拒而不是靠注释 */
    if (rstConf.nReleaseBeats <= rstConf.nInheritBeats)
    {
        rstrErr = "release_beats must be greater than inherit_beats (no reservation band left)";
        return false;
    }
    return true;
}

bool seatPriorIdleOk(int32_t nReleaseBeats, int32_t nSampleMs, int64_t llRowIdleMs,
                     std::string &rstrErr)
{
    /* 三个入参分属三个配置块（seat_prior / sampling / attendance） */
    if (nSampleMs <= 0 || llRowIdleMs <= 0 || nReleaseBeats <= 0)
    {
        rstrErr = "release_beats / sampling.group_interval_ms / attendance.row_idle_ms must be > 0";
        return false;
    }
    const int64_t llReleaseMs = static_cast<int64_t>(nReleaseBeats) * static_cast<int64_t>(nSampleMs);
    const int64_t llNeedMs = 2 * llRowIdleMs;
    if (llReleaseMs < llNeedMs)
    {
        rstrErr = "seat_prior.release_beats*sampling.group_interval_ms("
                  + std::to_string(llReleaseMs) + "ms) must be >= 2*attendance.row_idle_ms("
                  + std::to_string(llNeedMs) + "ms)";
        return false;
    }
    return true;
}

CSeatPrior::CSeatPrior()
{
    /* 构造期就把表抹干净：m_astSlot 是定长数组，成员初始化给了默认值，
       但 reset() 之后要能回到同一状态，所以两处共用 wipe() */
    wipe();
}

void CSeatPrior::wipe()
{
    for (int32_t i = 0; i < CLASSROOM_SEAT_PRIOR_MAX_SLOTS; ++i)
    {
        m_astSlot[static_cast<size_t>(i)] = CSlot_S();
    }
    /* 配对表跟着一起清：它只在一次 fold 与它的消费点之间有效，
       留着一节的旧配对等于让"下一次消费"可能读到上一次的证据 */
    m_nPairNum = 0;
}

void CSeatPrior::setParam(const CSeatPriorConf_S &rstConf)
{
    /* 换参数不清表。这里的状态是"谁坐在哪一席"，与 n_hold/n_release 这两个数没有编码关系
       （不像热力的格子编号那样改边数就换含义），清表反而会让全班在同一拍丢掉座位号 */
    m_stConf = rstConf;
}

int32_t CSeatPrior::slotIndexOf(int32_t nCell, int32_t nOrdinal) const
{
    if (nCell < 0 || nCell >= CLASSROOM_SEAT_MAX_CELLS
        || nOrdinal < 0 || nOrdinal >= CLASSROOM_SEAT_PRIOR_MAX_PER_CELL)
    {
        return -1;
    }
    return nCell * CLASSROOM_SEAT_PRIOR_MAX_PER_CELL + nOrdinal;
}

int32_t CSeatPrior::indexOf(int32_t nPersonId) const
{
    if (nPersonId < 0)
    {
        return -1;
    }
    /* 线性扫 512 项而不是哈希表 */
    for (int32_t i = 0; i < CLASSROOM_SEAT_PRIOR_MAX_SLOTS; ++i)
    {
        if (m_astSlot[static_cast<size_t>(i)].nPersonId == nPersonId)
        {
            return i;
        }
    }
    return -1;
}

int32_t CSeatPrior::seatIdOf(int32_t nPersonId) const
{
    const int32_t nSlot = indexOf(nPersonId);
    if (nSlot < 0)
    {
        return -1;
    }
    /* 代次编进号里：席位号在本代次内唯一，跨代次可比的是"座位"而不是"这个整数" */
    const int64_t llNum =
        static_cast<int64_t>(m_u64Epoch % CLASSROOM_SEAT_PRIOR_EPOCH_WRAP) *
            CLASSROOM_SEAT_PRIOR_MAX_SLOTS +
        static_cast<int64_t>(nSlot) + 1;
    return static_cast<int32_t>(llNum);
}

int32_t CSeatPrior::seatedNum() const
{
    int32_t nNum = 0;
    for (int32_t i = 0; i < CLASSROOM_SEAT_PRIOR_MAX_SLOTS; ++i)
    {
        if (m_astSlot[static_cast<size_t>(i)].nPersonId >= 0)
        {
            ++nNum;
        }
    }
    return nNum;
}

const CSeatPriorPair_S *CSeatPrior::inheritPairs(int32_t &rnNum) const
{
    rnNum = m_nPairNum;
    return m_astPair;
}

void CSeatPrior::reset()
{
    wipe();
    m_u64Beat = 0;
    m_bEpochSeen = false;
    m_u64Epoch = 0;
    m_stStat = CStat_S();
}

void CSeatPrior::fold(const CSeatPriorIn_S *patIn, int32_t nNum, uint64_t u64GridEpoch, float fCellW)
{
    m_nPairNum = 0;
    if (nNum < 0 || (nullptr == patIn && nNum > 0))
    {
        return;
    }
    ++m_u64Beat;

    if (!m_bEpochSeen)
    {
        m_bEpochSeen = true;
        m_u64Epoch = u64GridEpoch;
    }
    else if (u64GridEpoch != m_u64Epoch)
    {
        /* 格子图换过含义 → 整表作废。只清不涨代次也不行（见 seatIdOf）：
           清表只保证"不再发旧号"，不保证"新号不与旧号撞" */
        m_u64Epoch = u64GridEpoch;
        wipe();
        ++m_stStat.u64EpochWipe;
    }

    /* 上界钳两次：nNum 由调用方给，而本函数在下面按它开栈上数组。
       越界的后果不是"少处理几个人"而是栈写坏——那会在完全无关的地方崩 */
    const int32_t nRowNum = (nNum > CLASSROOM_MAX_PERSONS) ? CLASSROOM_MAX_PERSONS : nNum;
    const float fGate = seatPriorGate(fCellW);

    int32_t anOwn[CLASSROOM_MAX_PERSONS];
    int32_t anCell[CLASSROOM_MAX_PERSONS];
    int32_t anId[CLASSROOM_MAX_PERSONS];
    /* 本拍要解绑的旧席（换格的人） */
    int32_t anVacate[CLASSROOM_MAX_PERSONS];
    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const CSeatPriorIn_S &stIn = patIn[nRow];
        anId[nRow] = stIn.nPersonId;
        anOwn[nRow] = -1;
        anVacate[nRow] = -1;
        anCell[nRow] = -1;
        if (anId[nRow] < 0)
        {
            continue;    /* 空行：不占号、不计数（计数只数"有身份的人"，见 u64NoCell 的含义）*/
        }
        if (personSeen(anId, nRow, anId[nRow]))
        {
            /* 同一个跟踪 ID 这一拍出现了两行（跟踪器合并/重建的瞬时重复）：只认第一行。
               不挡的话第③遍会给他再领一席，而拍尾的延迟解绑会把**两席一起**标成"搬走"——
               他自己反而一个号都不剩，下一拍从头再来，考勤那一行就跟着抖 */
            anId[nRow] = -1;
            continue;
        }
        anOwn[nRow] = indexOf(stIn.nPersonId);
        /* 宽度不可用（0 / NaN）就等于这一拍没有任何位置证据可用：不发新号，也不动旧号。
           宁可让全班这一拍都读 -1，也不要凭一个猜的格宽决定谁坐哪一席 */
        const bool bCellUsable = (fGate > 0.0f) && (stIn.nCell >= 0);
        anCell[nRow] = bCellUsable ? stIn.nCell : -1;
        if (!bCellUsable)
        {
            ++m_stStat.u64NoCell;
        }
    }

    /* 第②遍：在场号主回原位。先做满一遍再做第③遍，这条顺序就是"在场号主不可被抢"的实现 */
    bool abClaim[CLASSROOM_SEAT_PRIOR_MAX_SLOTS];
    for (int32_t i = 0; i < CLASSROOM_SEAT_PRIOR_MAX_SLOTS; ++i)
    {
        abClaim[static_cast<size_t>(i)] = false;
    }
    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        if (anId[nRow] < 0 || anOwn[nRow] < 0 || anCell[nRow] < 0)
        {
            continue;
        }
        CSlot_S &stSlot = m_astSlot[static_cast<size_t>(anOwn[nRow])];
        if (stSlot.nPersonId != anId[nRow] || slotCellOf(anOwn[nRow]) != anCell[nRow])
        {
            continue;    /* 人挪了格：他的号在第③遍处理，这一遍只管"还在自己那一格" */
        }
        stSlot.u64LastBeat = m_u64Beat;
        const float fX = seatPriorX(patIn[nRow].fNormX);
        if (fX >= 0.0f)
        {
            stSlot.fX = fX;
        }
        abClaim[static_cast<size_t>(anOwn[nRow])] = true;
        ++m_stStat.u64Hold;
    }

    /* 第③遍：没号的人领号。按输入序处理（不是按横坐标序）：输入序由跟踪器给，
       同一批人在同一拍里的相对顺序是稳的，按坐标序反而会让两个坐标相邻的人在抖动的边界上换号 */
    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const int32_t nSelf = anId[nRow];
        if (nSelf < 0 || anCell[nRow] < 0)
        {
            continue;
        }
        const int32_t nCell = anCell[nRow];
        const int32_t nOwn = anOwn[nRow];
        if (nOwn >= 0 && abClaim[static_cast<size_t>(nOwn)])
        {
            continue;    /* 第②遍已经回过原位 */
        }
        if (nOwn >= 0)
        {
            /* 他换格了：旧席留到拍尾再解绑（见 anVacate）。不解绑就会有一个人同时挂着两个号，
               而 indexOf 只能返回先扫到的那个——考勤侧读到的就是"他每拍换一个座位" */
            anVacate[nRow] = nOwn;
        }

        const float fX = seatPriorX(patIn[nRow].fNormX);

        /* 一次扫完这一格的四席，每席给一个档位，取档位最小者（同档取距离小者，再同取序数小者）。
           四档的读法：
           1 就近继承
           2 同位回收
           3 空位
           4 远处借用

           四档共用的两道闸：号主本拍在画面里（哪怕这一拍没被算进任何格——站起来了、被走道判掉、
           热力那一拍没读到他），他的号这一拍谁也不许领，闲了几拍都不行*/
        int32_t nTake = -1;
        int32_t nRank = SEAT_PRIOR_RANK_NONE;
        float fBest = SEAT_PRIOR_NO_DIST;
        for (int32_t nOrd = 0; nOrd < CLASSROOM_SEAT_PRIOR_MAX_PER_CELL; ++nOrd)
        {
            const int32_t nSlot = slotIndexOf(nCell, nOrd);
            if (nSlot < 0 || abClaim[static_cast<size_t>(nSlot)])
            {
                continue;
            }
            const CSlot_S &stSlot = m_astSlot[static_cast<size_t>(nSlot)];
            int32_t nSlotRank = SEAT_PRIOR_RANK_NONE;
            float fDist = SEAT_PRIOR_NO_DIST;
            if (SEAT_PRIOR_VACATED != stSlot.nPersonId && stSlot.nPersonId < 0)
            {
                nSlotRank = SEAT_PRIOR_RANK_FRESH;    /* -1：本代次从没被占，没有锚点可比 */
            }
            else
            {
                /* 走到这里挂着两种"没有号主"之外的情形：>=0（挂在某个不在场者身上）
                   和 -2（号主搬走了）。两者都要过下面两道闸，但只有前者能走第 1 档 */
                if (stSlot.nPersonId >= 0 && personSeen(anId, nRowNum, stSlot.nPersonId))
                {
                    continue;
                }
                if (stSlot.u64LastBeat >= m_u64Beat)
                {
                    continue;
                }
                /* 任一侧没有坐标证据就是"隔多远说不清"（-1 的横坐标进不了 fabs 的比较，
                   NaN 已经被 seatPriorX 换成 -1），说不清就不算同一把椅子 */
                fDist = (fX >= 0.0f && stSlot.fX >= 0.0f) ? std::fabs(fX - stSlot.fX)
                                                          : SEAT_PRIOR_NO_DIST;
                const bool bSameSeat = (fDist <= fGate);
                const uint64_t u64Gap = m_u64Beat - stSlot.u64LastBeat;
                if (bSameSeat && stSlot.nPersonId >= 0
                    && u64Gap <= static_cast<uint64_t>(m_stConf.nInheritBeats))
                {
                    nSlotRank = SEAT_PRIOR_RANK_INHERIT;
                }
                else if (bSameSeat && u64Gap >= static_cast<uint64_t>(m_stConf.nReleaseBeats))
                {
                    nSlotRank = SEAT_PRIOR_RANK_RECLAIM;
                }
                else if (u64Gap >= static_cast<uint64_t>(m_stConf.nReleaseBeats))
                {
                    nSlotRank = SEAT_PRIOR_RANK_BORROW;
                }
            }
            if (SEAT_PRIOR_RANK_NONE == nSlotRank)
            {
                continue;
            }
            if (nTake < 0 || nSlotRank < nRank
                || (nSlotRank == nRank && fDist < fBest))
            {
                nTake = nSlot;
                nRank = nSlotRank;
                fBest = fDist;
            }
        }

        if (nTake < 0)
        {
            /* 挤不下。宁可发 -1（这个人这一拍没有先验，考勤另起一行、到课人数可能多 1）
               也绝不发一个已经在别人手里的号（到课人数少 1 且无声）——
               两条都是错，但 -1 是**可数**的错：u64NoSlot 每 +1 都在心跳里看得见 */
            ++m_stStat.u64NoSlot;
            continue;
        }

        CSlot_S &stSlot = m_astSlot[static_cast<size_t>(nTake)];
        /* 旧号主必须在改写之前取走：第③遍之后这一格上只剩新 ID，
           "他把这一席交给了谁"这件事就没有别的落点了（表本身不存历史）*/
        const int32_t nPrevId = stSlot.nPersonId;
        stSlot.nPersonId = nSelf;
        stSlot.u64LastBeat = m_u64Beat;
        if (fX >= 0.0f)
        {
            stSlot.fX = fX;
        }
        abClaim[static_cast<size_t>(nTake)] = true;
        switch (nRank)
        {
            case SEAT_PRIOR_RANK_INHERIT:
            {
                ++m_stStat.u64Inherit;
                /* 四档里只有这一档是"同一个人"的证据：同位回收与远处借用都要等过让号期
                   （那两档的语义就是"坐下来的可以是另一个人"），空位更没有前手。
                   nPrevId >= 0 是这一档自己的前提（挂着号主才可能被判为不在场），
                   写成条件而不是省掉：将来若有人把 -2 那一态也放进第 1 档，
                   这里少一个"把 -2 当成某个人交给考勤"的坑 */
                if (nPrevId >= 0 && m_nPairNum < CLASSROOM_MAX_PERSONS)
                {
                    m_astPair[static_cast<size_t>(m_nPairNum)].nTrackOld = nPrevId;
                    m_astPair[static_cast<size_t>(m_nPairNum)].nTrackNew = nSelf;
                    ++m_nPairNum;
                }
                break;
            }
            case SEAT_PRIOR_RANK_FRESH: { ++m_stStat.u64New; break; }
            default:                    { ++m_stStat.u64Steal; break; }    /* 回收 / 借用 */
        }
    }

    /* 拍尾解绑换格者的旧席 */
    for (int32_t nRow = 0; nRow < nRowNum; ++nRow)
    {
        const int32_t nOld = anVacate[nRow];
        if (nOld < 0 || m_astSlot[static_cast<size_t>(nOld)].nPersonId != anId[nRow])
        {
            continue;
        }
        m_astSlot[static_cast<size_t>(nOld)].nPersonId = SEAT_PRIOR_VACATED;
    }
}

/* 格子号从槽位下标反推 */
int32_t CSeatPrior::slotCellOf(int32_t nSlot) const

{
    if (nSlot < 0 || nSlot >= CLASSROOM_SEAT_PRIOR_MAX_SLOTS)
    {
        return -1;
    }
    return nSlot / CLASSROOM_SEAT_PRIOR_MAX_PER_CELL;
}

std::string CSeatPrior::statText() const

{
    char szBuf[256] = {0};
    const int nLen = snprintf(szBuf, sizeof(szBuf),
                              "sprior beat=%llu hold=%llu inherit=%llu new=%llu steal=%llu "
                              "noslot=%llu nocell=%llu wipe=%llu seated=%d epoch=%llu",
                              static_cast<unsigned long long>(m_u64Beat),
                              static_cast<unsigned long long>(m_stStat.u64Hold),
                              static_cast<unsigned long long>(m_stStat.u64Inherit),
                              static_cast<unsigned long long>(m_stStat.u64New),
                              static_cast<unsigned long long>(m_stStat.u64Steal),
                              static_cast<unsigned long long>(m_stStat.u64NoSlot),
                              static_cast<unsigned long long>(m_stStat.u64NoCell),
                              static_cast<unsigned long long>(m_stStat.u64EpochWipe),
                              seatedNum(),
                              static_cast<unsigned long long>(m_u64Epoch));
    if (nLen < 0 || nLen >= static_cast<int>(sizeof(szBuf)))
    {
        return "sprior stat=truncated";
    }
    return std::string(szBuf);
}

int32_t applySeatPriorLinks(const CSeatPrior &stPrior, bool bConsume, CAttendanceLedger &stLedger)
{
    /* 关着就一条也不送 */
    if (!bConsume)
    {
        return 0;
    }

    int32_t nNum = 0;
    const CSeatPriorPair_S *patPair = stPrior.inheritPairs(nNum);
    int32_t nDone = 0;
    /* 按配对表的顺序 */
    for (int32_t i = 0; i < nNum; ++i)
    {
        if (stLedger.mergeSamePerson(patPair[static_cast<size_t>(i)].nTrackOld,
                                     patPair[static_cast<size_t>(i)].nTrackNew))
        {
            ++nDone;
        }
    }
    return nDone;
}

}    /* namespace Classroom */
