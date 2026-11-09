/**
 * @FilePath     : attendance.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-23 14:13:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 14:26:00
 * @Description  : 考勤账本实现
 */

#include "attendance.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "classroom_fmt.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* 行键的两个域：身份优先。加前缀而不是直接存 ID 字符串，是因为平台侧的 personId
   与我们自己的跟踪 ID 都是十进制数字——不加前缀时"7"到底是谁，只有崩溃时才分得清 */
const char ATTEND_KEY_TRACK = 'T';
const char ATTEND_KEY_PERSON = 'P';

int64_t clampI64(int64_t llV, int64_t llMin, int64_t llMax)
{
    /* 逆序区间纠正 */
    if (llMin > llMax)
    {
        const int64_t llTmp = llMin;
        llMin = llMax;
        llMax = llTmp;
    }
    if (llV < llMin)
    {
        return llMin;
    }
    return (llV > llMax) ? llMax : llV;
}

std::string trackKey(int32_t nTrackId)
{
    char szBuf[24] = {0};
    snprintf(szBuf, sizeof(szBuf), "%c%d", ATTEND_KEY_TRACK, nTrackId);
    return std::string(szBuf);
}

std::string personKey(const std::string &strPersonId)
{
    return std::string(1, ATTEND_KEY_PERSON) + strPersonId;
}

}

IFaceGallery::~IFaceGallery() = default;

bool CNullFaceGallery::match(const float *pafFeat, int32_t nDim, int64_t llNowMs, CFaceMatch_S &rstOut)
{
    /* 空底库只做一个动作 */
    (void)pafFeat;
    (void)nDim;
    (void)llNowMs;
    rstOut = CFaceMatch_S();
    return false;
}

bool attendanceParamOk(const CAttendParam_S &rstParam, std::string &rstrErr)
{
    if (rstParam.llPresentMinMs < CLASSROOM_ATTEND_PRESENT_MIN_BEAT_MS)
    {
        rstrErr = "present_min_ms below one beat";
        return false;
    }
    /* 增量夹取必须为正：给 0 等于把"两次观测之间过了多久"整个抹掉，
       时长永远累不起来，到课人数恒为 0 而没有任何一条日志会说为什么 */
    if (rstParam.llGapMaxMs <= 0)
    {
        rstrErr = "gap_max_ms must be positive";
        return false;
    }
    if (rstParam.llRowIdleMs <= 0)
    {
        rstrErr = "row_idle_ms must be positive";
        return false;
    }
    if (rstParam.nMinFacePx < 0)
    {
        rstrErr = "face_min_px negative";
        return false;
    }
    /* 远带三条：边界值本身合法（=nMinFacePx 即远带空集=旧形态），
       但两条"远带更严"的不变式必须在这里钉死——放开了就是"小脸更容易命名"，
       误识率顶穿 P5 验收线还无人报错 */
    if (rstParam.nFaceFarMinPx < 0 || rstParam.nFaceFarMinPx > rstParam.nMinFacePx)
    {
        rstrErr = "face_far_min_px out of [0, face_min_px]";
        return false;
    }
    if (!(rstParam.fFarMatchScore > 0.0f) || rstParam.fFarMatchScore > 1.0f ||
        rstParam.fFarMatchScore < rstParam.fMinMatchScore)
    {
        rstrErr = "face_far_match_score out of (0,1] or below match_score_min";
        return false;
    }
    if (rstParam.nFarMinHits < rstParam.nMinMatchHits)
    {
        rstrErr = "face_far_min_hits must be >= match_min_hits";
        return false;
    }
    if (rstParam.nMinMatchHits < 1)
    {
        rstrErr = "match_min_hits must be >= 1";
        return false;
    }
    /* 与表情那两个门同样用 fail-safe 写法：NaN 过不了任何区间比较，
       而 NaN 恰恰是从配置文件里能读出来的值（"0.7x" 这类笔误会被解析成 0 或 NaN） */
    if (!(rstParam.fMinMatchScore > 0.0f) || rstParam.fMinMatchScore > 1.0f)
    {
        rstrErr = "match_score_min out of range";
        return false;
    }
    if (rstParam.nExpectedNum < -1)
    {
        rstrErr = "expected_num below -1";
        return false;
    }
    return true;
}

CAttendanceLedger::CAttendanceLedger()
    : m_stParam()
    , m_stStat()
    , m_pGallery(nullptr)
    , m_nSessionExpected(-1)
    , m_bClosed(false)
    , m_llCloseMs(0)
{
    /* 构造期定容，此后不改容量。60 行 × 每行两个短串 + 定长字段，
       实际字节在几十 KB 以内，且考勤功能关掉也只是这些字节不涨，不会另起线程 */
    m_vecRow.resize(CLASSROOM_MAX_PERSONS);
    m_vecVoteId.resize(CLASSROOM_MAX_PERSONS);
    m_vecVoteNum.assign(static_cast<size_t>(CLASSROOM_MAX_PERSONS), 0);
}

void CAttendanceLedger::setParam(const CAttendParam_S &rstParam)
{
    std::string strErr;
    if (!attendanceParamOk(rstParam, strErr))
    {
        dlog_warn("classroom attendance: 参数被拒 (%s)", strErr.c_str());
        return;
    }
    /* 门限改了不清名单：已命名的人不会因为运维调了一下分数门就消失。
       llPresentMinMs 调小也不补算——已累加的时长是真的，只是当初没过门，
       补算要回放每拍观测，而账本不留（内存预算） */
    m_stParam = rstParam;
    /* 时长门调低后，原来没够的人要在**下一次被观测到**时才过门。这里不主动扫表补计，
       代价写明白：改完门限到下一次点名之间，到课人数可能比新参数下应有的值偏小 */
}

void CAttendanceLedger::setGallery(IFaceGallery *pGallery)
{
    m_pGallery = pGallery;
}

bool CAttendanceLedger::galleryAvailable() const
{
    return (nullptr != m_pGallery) && m_pGallery->available();
}

int32_t CAttendanceLedger::rowInUse() const
{
    int32_t nNum = 0;
    for (const auto &stRow : m_vecRow)
    {
        if (!stRow.strKey.empty())
        {
            ++nNum;
        }
    }
    return nNum;
}

int32_t CAttendanceLedger::findRowByTrack(int32_t nTrackId) const
{
    if (nTrackId < 0)
    {
        return -1;
    }
    for (size_t i = 0; i < m_vecRow.size(); ++i)
    {
        if (m_vecRow[i].nTrackId == nTrackId && !m_vecRow[i].strKey.empty())
        {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

int32_t CAttendanceLedger::findRowByKey(const std::string &strKey) const
{
    if (strKey.empty())
    {
        return -1;
    }
    for (size_t i = 0; i < m_vecRow.size(); ++i)
    {
        if (m_vecRow[i].strKey == strKey)
        {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void CAttendanceLedger::clearRow(int32_t nRow)
{
    if (nRow < 0 || nRow >= static_cast<int32_t>(m_vecRow.size()))
    {
        return;
    }
    m_vecRow[static_cast<size_t>(nRow)] = CAttendRec_S();
    m_vecVoteId[static_cast<size_t>(nRow)].clear();
    m_vecVoteNum[static_cast<size_t>(nRow)] = 0;
}

void CAttendanceLedger::mergeRow(int32_t nTo, int32_t nFrom)
{
    if (nTo < 0 || nFrom < 0 || nTo == nFrom)
    {
        return;
    }
    CAttendRec_S &stTo = m_vecRow[static_cast<size_t>(nTo)];
    const CAttendRec_S &stFrom = m_vecRow[static_cast<size_t>(nFrom)];

    /* 首次时刻取早的那个、末次取晚的那个、时长取和：
       这条记录现在代表"这个人这一节课"，而两行本就是同一个人被切开的前后段 */
    if (stTo.llFirstMs <= 0 || (stFrom.llFirstMs > 0 && stFrom.llFirstMs < stTo.llFirstMs))
    {
        stTo.llFirstMs = stFrom.llFirstMs;
    }
    if (stFrom.llLastMs > stTo.llLastMs)
    {
        stTo.llLastMs = stFrom.llLastMs;
        /* 换到更晚的那一端，同时把跟踪 ID 也换过去：此后这一行由新 ID 延续。
           只留一个 ID 是有意的——同一个人被切成多段时新 ID 会不断进来，
           但**新 ID 的那一行在合并后消失**，所以人数不会跟着涨 */
        stTo.nTrackId = stFrom.nTrackId;
        stTo.fLastX = stFrom.fLastX;
        stTo.fLastY = stFrom.fLastY;
        stTo.fLastHeadNorm = stFrom.fLastHeadNorm;
    }
    stTo.llSeenMs += stFrom.llSeenMs;
    stTo.nSampleNum += stFrom.nSampleNum;
    stTo.nMatchNum += stFrom.nMatchNum;
    if (stFrom.fBestScore > stTo.fBestScore)
    {
        stTo.fBestScore = stFrom.fBestScore;
    }
    if (stTo.nSeatId < 0)
    {
        stTo.nSeatId = stFrom.nSeatId;
    }
    if (stFrom.bCounted)
    {
        stTo.bCounted = true;
    }
    if (!stTo.bCounted && stTo.llSeenMs >= m_stParam.llPresentMinMs)
    {
        /* 合并本身可能就让时长第一次过门（两段各 3 秒的人）：过门在合并处再判一次，
           否则这个人要等到下一拍才被计入，而那一拍可能已经是下节课了 */
        stTo.bCounted = true;
    }
    ++m_stStat.u64Merge;
    clearRow(nFrom);
}

int32_t CAttendanceLedger::allocRow(int32_t nTrackId, int32_t nSeatId, int64_t llNowMs,
                                    float fNormX, float fNormY, float fHeadNorm)
{
    /* 先按座位复用**未命名**的行，再考虑占用空槽。顺序反过来的后果是可预见的：
       跟踪器换 ID 后老行还留在表里，新行另起一格，两个人各过半程时长门 → 到课人数 +1，
       而且没有任何一处会报错。座位未命名的行是同一个人，这是这间教室的先验 */
    if (nSeatId >= 0)
    {
        for (size_t i = 0; i < m_vecRow.size(); ++i)
        {
            CAttendRec_S &stRow = m_vecRow[i];
            if (!stRow.strKey.empty() && !stRow.bNamed && stRow.nSeatId == nSeatId)
            {
                /* 座位复用也要过 llRowIdleMs 时限闸 */
                const int64_t llIdle = llNowMs - stRow.llLastMs;
                if (llIdle > m_stParam.llRowIdleMs)
                {
                    continue;   /* 这行已经闲到可回收：不当成"同一个人接着坐" */
                }
                /* 改绑到新 ID 而不清时长：段与段之间是同一个人，前一段的在场是真的。
                   **本裁剪版没有座位层**（座位先验已移除），nSeatId 恒为 -1，
                   因此这条分支实际不会触发；保留它是为了将来接回座位分配时口径只有一处 */
                stRow.nTrackId = nTrackId;
                /**
                 * strKey 必须跟着改。未命名行的键是 "T<trackId>"，也就是**从跟踪 ID 派生的句柄**：
                 * 只换 ID 不换键，这个键就成了一个指向别人的旧句柄。之后那个旧 ID 被别人用上
                 * （跟踪器的 ID 会重发）时，allocRow 的空槽分支按同一个规则算出同样的键，
                 * 于是两行共用一个 strKey —— 而 records()/roster() 交出去的就是这个键，
                 * 平台侧按 strKey 去重会把两个活人
                 * 并成一个，现象正是"名单上少一个人"且设备端毫无异常。
                 * 这条只在座位复用上改，命名行不需要：命名后键是 P<personId>，与 ID 无关
                 */
                stRow.strKey = trackKey(nTrackId);
                /* 末次时刻只许往前走：本行留着上一段的在场，而这条观测的时刻可以比它更早
                   （时刻倒退的观测在本账本里是被接受的，见 observe 的 llDelta > 0 那道闸）。
                   直接覆盖会让 llLastMs 落到 llFirstMs 之前，报表上的"最后一次进画面"
                   早于"第一次进画面"——#60  fuzz 的 I6 就是钉这一对的 */
                if (llNowMs > stRow.llLastMs)
                {
                    stRow.llLastMs = llNowMs;
                }
                ++m_stStat.u64SeatRebind;
                return static_cast<int32_t>(i);
            }
        }
    }
    /* 同位置重入：跟踪器把一个人切成两段时，新 ID 第一次要行 */
    if (fNormX >= 0.0f && fNormY >= 0.0f && fHeadNorm > 0.0f)
    {
        const float fRadius = CLASSROOM_ATTEND_REENTRY_RADIUS_HEADS * fHeadNorm;
        int32_t nNear = -1;
        float fBest = 0.0f;
        for (size_t i = 0; i < m_vecRow.size(); ++i)
        {
            const CAttendRec_S &stRow = m_vecRow[i];
            if (stRow.strKey.empty() || stRow.bNamed)
            {
                continue;
            }
            if (stRow.nTrackId == nTrackId || stRow.fLastX < 0.0f || stRow.fLastY < 0.0f ||
                stRow.fLastHeadNorm <= 0.0f)
            {
                continue;
            }
            const int64_t llGap = llNowMs - stRow.llLastMs;
            if (llGap < CLASSROOM_ATTEND_REENTRY_MIN_GAP_MS || llGap > m_stParam.llRowIdleMs)
            {
                continue;
            }
            const float fRatio = fHeadNorm / stRow.fLastHeadNorm;
            if (fRatio < CLASSROOM_ATTEND_REENTRY_SCALE_MIN ||
                fRatio > CLASSROOM_ATTEND_REENTRY_SCALE_MAX)
            {
                continue;
            }
            const float fDx = fNormX - stRow.fLastX;
            const float fDy = fNormY - stRow.fLastY;
            const float fDist = std::sqrt(fDx * fDx + fDy * fDy);
            if (fDist > fRadius)
            {
                continue;
            }
            if (nNear < 0 || fDist < fBest)
            {
                nNear = static_cast<int32_t>(i);
                fBest = fDist;
            }
        }
        if (nNear >= 0)
        {
            CAttendRec_S &stRow = m_vecRow[static_cast<size_t>(nNear)];
            /* 与座位复用同一套改绑纪律：ID 换了，键必须跟着换 */
            stRow.nTrackId = nTrackId;
            stRow.strKey = trackKey(nTrackId);
            if (llNowMs > stRow.llLastMs)
            {
                stRow.llLastMs = llNowMs;
            }
            stRow.fLastX = fNormX;
            stRow.fLastY = fNormY;
            stRow.fLastHeadNorm = fHeadNorm;
            ++m_stStat.u64ReentryMerge;
            return nNear;
        }
    }

    for (size_t i = 0; i < m_vecRow.size(); ++i)
    {
        if (m_vecRow[i].strKey.empty())
        {
            m_vecRow[i].strKey = trackKey(nTrackId);
            m_vecRow[i].nTrackId = nTrackId;
            m_vecRow[i].nSeatId = nSeatId;
            m_vecRow[i].llFirstMs = llNowMs;
            m_vecRow[i].llLastMs = llNowMs;
            m_vecRow[i].fLastX = -1.0f;
            m_vecRow[i].fLastY = -1.0f;
            m_vecRow[i].fLastHeadNorm = -1.0f;
            return static_cast<int32_t>(i);
        }
    }
    /* 表里没有空槽：先回收最久没见的未命名行。已计到课的行**永不回收**——
       那一行是"到课人数"这个数本身的载体，回收就等于把一个人从名单上抹掉 */
    int32_t nOldest = -1;
    for (size_t i = 0; i < m_vecRow.size(); ++i)
    {
        CAttendRec_S &stRow = m_vecRow[i];
        /* 已计到课的行也要跳过，而且这条比前一条更要紧 */
        if (stRow.bNamed || stRow.bCounted || stRow.strKey.empty())
        {
            continue;
        }
        if (nOldest < 0 || stRow.llLastMs < m_vecRow[static_cast<size_t>(nOldest)].llLastMs)
        {
            nOldest = static_cast<int32_t>(i);
        }
    }
    if (nOldest >= 0 && (llNowMs - m_vecRow[static_cast<size_t>(nOldest)].llLastMs) > m_stParam.llRowIdleMs)
    {
        ++m_stStat.u64Recycle;
        clearRow(nOldest);
        return allocRow(nTrackId, nSeatId, llNowMs);
    }
    ++m_stStat.u64RowFull;
    return -1;
}

void CAttendanceLedger::bindIdentity(int32_t nRow, const CFaceMatch_S &stMatch)
{
    if (nRow < 0 || nRow >= static_cast<int32_t>(m_vecRow.size()))
    {
        return;
    }
    CAttendRec_S &stRow = m_vecRow[static_cast<size_t>(nRow)];
    const std::string strKey = personKey(stMatch.strPersonId);
    const int32_t nExist = findRowByKey(strKey);
    if (nExist >= 0 && nExist != nRow)
    {
        /* 同身份的另一条已在（同一个人被跟踪器切成两段，两段各自被比对过）：
           把本行并进那一行。命名与 ID 一律沿用**已有的那一行**——它先落定，
           且报表里同一个人出现两个名字比出现一个旧名字糟得多 */
        mergeRow(nExist, nRow);
        return;
    }

    bool bTrunc = false;
    stRow.strKey = strKey;
    stRow.strPersonId = stMatch.strPersonId;
    stRow.strName = truncateUtf8(stMatch.strName, CLASSROOM_ATTEND_NAME_MAX, bTrunc);
    if (bTrunc)
    {
        ++m_stStat.u64NameTruncated;
        /* 不 dlog 内容本身：姓名是个人信息，日志会随文件被导出到设备外 */
        dlog_warn("classroom attendance: 姓名截断到 %d 字节", CLASSROOM_ATTEND_NAME_MAX);
    }
    stRow.bNamed = true;
    ++m_stStat.u64Named;
}

bool CAttendanceLedger::observe(const CAttendObs_S &rstObs)
{
    ++m_stStat.u64Obs;

    if (m_bClosed)
    {
        /* 已收尾还来观测：说明还有一路在途的结果没排干。丢掉并计数——
           已经发出去的报表不能被一条迟到的观测改数字，那是"平台看到的名单会自己变" */
        ++m_stStat.u64RejectObs;
        return false;
    }
    if (!m_stParam.bEnable)
    {
        return false;
    }
    if (rstObs.nTrackId < 0 || rstObs.llFrameMs <= 0)
    {
        ++m_stStat.u64RejectObs;
        return false;
    }

    /* 换拍先把上一拍的人数并进峰值。放在逐行处理之前 */
    rollBeat(rstObs.llFrameMs, rstObs.nFramePersons);

    int32_t nRow = findRowByTrack(rstObs.nTrackId);
    if (nRow < 0)
    {
        nRow = allocRow(rstObs.nTrackId, rstObs.nSeatId, rstObs.llFrameMs,
                        rstObs.fNormX, rstObs.fNormY, rstObs.fHeadNorm);
    }
    if (nRow < 0)
    {
        return false;
    }

    CAttendRec_S &stRow = m_vecRow[static_cast<size_t>(nRow)];
    if (rstObs.nSeatId >= 0 && stRow.nSeatId < 0)
    {
        stRow.nSeatId = rstObs.nSeatId;
    }
    else if (rstObs.nSeatId >= 0 && rstObs.nSeatId != stRow.nSeatId)
    {
        /* 只计数、不改写：见下面那段「不改写」的说明。这个分支原先完全静默，
           现场无法从心跳判断座位先验与账本是否已经分岔 */
        ++m_stStat.u64SeatDrift;
    }
    if (rstObs.fNormX >= 0.0f && rstObs.fNormY >= 0.0f && rstObs.fHeadNorm > 0.0f)
    {
        stRow.fLastX = rstObs.fNormX;
        stRow.fLastY = rstObs.fNormY;
        stRow.fLastHeadNorm = rstObs.fHeadNorm;
    }
    /* 行上已有号而这条观测也带号且不是同一个号：不改写。
       本裁剪版不产出座位号（恒为 -1），这一格只保留结构，留给将来接回座位分配时用 */

    /* 在场时长：只按行的首末时刻增量累加，**不过人脸门** */
    const int64_t llDelta = rstObs.llFrameMs - stRow.llLastMs;
    if (llDelta > 0)
    {
        stRow.llLastMs = rstObs.llFrameMs;
        /* 增量被 llGapMaxMs 夹住：不夹的话"进来 3 秒、消失 40 分钟、再进来 3 秒"
           会被读成在场 40 分钟。长消失后的那次补计偏小是设计，不是丢数据 */
        stRow.llSeenMs += clampI64(llDelta, 0, m_stParam.llGapMaxMs);
    }
    ++stRow.nSampleNum;

    if (!stRow.bCounted && stRow.llSeenMs >= m_stParam.llPresentMinMs)
    {
        stRow.bCounted = true;
    }
    /* 本拍在场人数：同一拍里一行只计一次；本拍内的重复（同座位/同位置）
       与本拍人头上限在这一步挡掉。到课人数取各拍的最大值（见 presentNum） */
    if (stRow.bCounted && stRow.llCountBeatMs != m_llBeatMs)
    {
        stRow.llCountBeatMs = m_llBeatMs;
        if (!isDuplicateCounted(stRow, rstObs))
        {
            ++m_nBeatCounted;
        }
    }

    /* ---- 命名链路：尺寸门与质量门只在这里生效---- */
    if (!rstObs.bMatchTried)
    {
        /* 没做过比对。三种情况：G_FACE 空窗、这拍没检出脸、引擎本轮没排到他
           （人脸与姿态一样是按人轮询的）。共同点是**不能推任何东西**——
           尤其不能把"这拍没名字"读成"他没身份"。
           但"脸在而没比"与"根本没有脸"要分开计：前者查底库与排程，后者查人脸那段
           为什么不出框（face try/det），合成一个数等于让现场两条都查一遍 */
        if (rstObs.nFaceW > 0 && rstObs.nFaceH > 0)
        {
            ++m_stStat.u64NoMatch;
        }
        else
        {
            ++m_stStat.u64NoFace;
        }
        return true;
    }
    if (stRow.bNamed)
    {
        /* 已命名就不再改：单帧误匹配把名字挂错人的代价（名单上永久错一个人）
           高于"后来出现更高分的匹配"的收益。要纠偏由平台在课后按 fBestScore 人工做 */
        return true;
    }

    /* 尺寸门分两档：≥ nMinFacePx 走近带原门；
       [nFaceFarMinPx, nMinFacePx) 走远带门（见下面那半段）；< nFaceFarMinPx 仍旧否死。
       配 face_far_min_px = face_min_px 时远带是空集，本函数与旧形态逐字一致 */
    const int32_t nFaceShort = std::max(0, (rstObs.nFaceW < rstObs.nFaceH) ? rstObs.nFaceW : rstObs.nFaceH);
    const bool bFarBand = (nFaceShort < m_stParam.nMinFacePx);
    if (nullptr == m_pGallery || !m_pGallery->available())
    {
        ++m_stStat.u64GalleryAbsent;
    }
    if (!rstObs.stMatch.bHit)
    {
        ++m_stStat.u64MatchMiss;
        return true;
    }
    if (bFarBand && nFaceShort < m_stParam.nFaceFarMinPx)
    {
        /* 分数再高也不命名：一张 40 px 的脸比对分 0.8 是模型在猜，
           而这条链路的验收线是误识 ≤1%（P5）。尺寸门放在账本这一侧是有意的——
           它是名字落定前的最后一道，引擎换人实现时这道门不会跟着丢 */
        ++m_stStat.u64FaceSmall;
        return true;
    }
    /* 比对门按档取尺（#89）：远带的脸在旧形态里死在上面那道尺寸门、连比对门都摸不到，
       现在给它们一条**更严**的路——分门抬到 fFarMatchScore、票数抬到 nFarMinHits。
       远带没过门同记 u64ScoreLow：那也是"分数不够"，band 的差异已由 fartry/farhit
       这一对读数单独交代，不必再开第三个口径 */
    if (bFarBand)
    {
        ++m_stStat.u64FaceFarTry;
    }
    const float fGateScore = bFarBand ? m_stParam.fFarMatchScore : m_stParam.fMinMatchScore;
    if (!(rstObs.stMatch.fScore >= fGateScore) || rstObs.stMatch.strPersonId.empty())
    {
        /* 分数不足与 ID 为空同处计：两者都是"这次命中不能用"。
           刻意不复用 u64ScoreLow 记 ID 为空——那种上游缺陷要能在计数上被分出来，
           但也不为它单开一个字段：现场读到 non-zero 的 score_low 时先查阈值即可 */
        ++m_stStat.u64ScoreLow;
        return true;
    }
    if (bFarBand)
    {
        ++m_stStat.u64FaceFarHit;
    }
    else
    {
        ++m_stStat.u64MatchHit;
    }
    if (rstObs.stMatch.fScore > stRow.fBestScore)
    {
        stRow.fBestScore = rstObs.stMatch.fScore;
    }
    ++stRow.nMatchNum;

    /* 投票：够 nMinMatchHits 次**同一个 personId** 才落定。
       中途换了别的 ID 就从 1 重数——两个人长相接近时，来回换着命中永远都攒不够，
       这正是我们要的结果（宁可不命名，也不在两个人之间反复改名）。
       落定门槛按**这一拍**的档取（#89）：票是同一支，尺子跟着当前这张脸走——
       人在往讲台走近带帧攒的票远带帧接着用，反之远带帧要在远带档自己够 3 票 */
    const std::string &strVote = m_vecVoteId[static_cast<size_t>(nRow)];
    if (strVote != rstObs.stMatch.strPersonId)
    {
        m_vecVoteId[static_cast<size_t>(nRow)] = rstObs.stMatch.strPersonId;
        m_vecVoteNum[static_cast<size_t>(nRow)] = 1;
    }
    else
    {
        ++m_vecVoteNum[static_cast<size_t>(nRow)];
    }
    const int32_t nNeedHits = bFarBand ? m_stParam.nFarMinHits : m_stParam.nMinMatchHits;
    if (m_vecVoteNum[static_cast<size_t>(nRow)] >= nNeedHits)
    {
        bindIdentity(nRow, rstObs.stMatch);
        m_vecVoteId[static_cast<size_t>(nRow)].clear();
        m_vecVoteNum[static_cast<size_t>(nRow)] = 0;
    }
    return true;
}

bool CAttendanceLedger::mergeSamePerson(int32_t nTrackOld, int32_t nTrackNew)
{
    /* 两次查找都要在改表**之前**做完：合并会把幸存行的跟踪 ID 换成较晚那一头的，
       先换后查的话第二次 findRowByTrack 找的是刚被改掉的那个 ID，返回的是同一行，
       于是"两行并一行"变成"自己跟自己合并"——mergeRow 首句的 nTo==nFrom 会挡掉它，
       挡掉之后这个人的旧行留在表上，人数照样虚高，而且是一条查不出原因的虚高 */
    if (m_bClosed || !m_stParam.bEnable)
    {
        return false;
    }
    if (nTrackOld < 0 || nTrackNew < 0 || nTrackOld == nTrackNew)
    {
        return false;
    }
    const int32_t nOld = findRowByTrack(nTrackOld);
    const int32_t nNew = findRowByTrack(nTrackNew);
    if (nOld < 0 || nNew < 0 || nOld == nNew)
    {
        return false;
    }
    /* 命名的那一行不动：它的并合由 bindIdentity 那条路负责 */
    if (m_vecRow[static_cast<size_t>(nOld)].bNamed || m_vecRow[static_cast<size_t>(nNew)].bNamed)
    {
        return false;
    }
    /* 幸存的是旧 ID 那一行：它带着这个座位上的前一段历史（首次时刻更早、时长是真的）。
       但这一行的两个身份句柄必须在合并之后**显式**落到新 ID 上，不能指望 mergeRow 换：
       ① nTrackId —— mergeRow 只在"另一头更晚"时才换跟踪 ID，而本层的判据是"旧 ID 这一拍
          不在场"（先验折出来的），与两行的时刻先后无关。乱序帧、或旧行恰好被喂过一个更晚的
          时刻，就会让幸存行留在一个已经没人用的 ID 上——那一行从此再也找不到人，
          这个人的到课时长少掉后一半，报表上只是"到课 3.2 分钟"，不报错；
       ② strKey —— #60 的 fuzz 在座位复用那条路上抓到过同一处：未命名行的键就是
          "T<trackId>"，一个**从跟踪 ID 派生的句柄**。留着旧键等于留一个指向别人的句柄，
          而跟踪器的 ID 会重发：旧 ID 落到别人头上时空槽分支按同一规则算出
          同样的键，两行共用一个 strKey，平台侧按 strKey 去重 → 名单少一个人，设备端一个错都不报 */
    mergeRow(nOld, nNew);
    CAttendRec_S &stRow = m_vecRow[static_cast<size_t>(nOld)];
    stRow.nTrackId = nTrackNew;
    stRow.strKey = trackKey(nTrackNew);
    ++m_stStat.u64SeatMerge;
    return true;
}

/* 到课去重三道,同座位,同位置,人头数上限 */
bool CAttendanceLedger::isDuplicateCounted(const CAttendRec_S &stRow, const CAttendObs_S &stObs)
{
    for (const auto &stOther : m_vecRow)
    {
        /* 只认同拍里**已经计进去**的行：跨拍的行是“同时在场的另一个人”，
           拿它去否掉这一行会把一整排学生并成一个 */
        if (&stOther == &stRow || stOther.llCountBeatMs != m_llBeatMs)
        {
            continue;
        }
        if (stRow.nSeatId >= 0 && stOther.nSeatId == stRow.nSeatId)
        {
            ++m_stStat.u64DupSeat;
            return true;
        }
        if (stRow.fLastX >= 0.0f && stRow.fLastY >= 0.0f && stRow.fLastHeadNorm > 0.0f &&
            stOther.fLastX >= 0.0f && stOther.fLastY >= 0.0f && stOther.fLastHeadNorm > 0.0f)
        {
            const int64_t llGap = stObs.llFrameMs - stOther.llLastMs;
            if (llGap >= 0 && llGap <= m_stParam.llRowIdleMs)
            {
                const float fHead = (stRow.fLastHeadNorm > stOther.fLastHeadNorm)
                                        ? stRow.fLastHeadNorm : stOther.fLastHeadNorm;
                const float fDx = stRow.fLastX - stOther.fLastX;
                const float fDy = stRow.fLastY - stOther.fLastY;
                if (std::sqrt(fDx * fDx + fDy * fDy) <=
                    CLASSROOM_ATTEND_REENTRY_RADIUS_HEADS * fHead)
                {
                    ++m_stStat.u64DupPos;
                    return true;
                }
            }
        }
    }

    if (stObs.nFramePersons > 0 && m_nBeatCounted >= stObs.nFramePersons)
    {
        ++m_stStat.u64DupHead;
        return true;
    }
    return false;
}

void CAttendanceLedger::finishBeat()
{
    int32_t nBeat = m_nBeatCounted;
    if (m_nBeatPersons > 0 && nBeat > m_nBeatPersons)
    {
        nBeat = m_nBeatPersons;
    }
    if (nBeat > m_nPresentPeak)
    {
        m_nPresentPeak = nBeat;
    }
}

void CAttendanceLedger::rollBeat(int64_t llFrameMs, int32_t nFramePersons)
{
    if (llFrameMs != m_llBeatMs)
    {
        finishBeat();
        m_llBeatMs = llFrameMs;
        m_nBeatCounted = 0;
        m_nBeatPersons = 0;
    }
    if (nFramePersons > m_nBeatPersons)
    {
        m_nBeatPersons = nFramePersons;
    }
}

int32_t CAttendanceLedger::presentNum() const
{
    /* 到课人数 = “各拍同时在场人数”的峰值。当前这一拍还没收尾，
       一并参与取最大，这样“刚进来就过门限”的那个人当拍就能看见 */
    int32_t nBeat = m_nBeatCounted;
    if (m_nBeatPersons > 0 && nBeat > m_nBeatPersons)
    {
        nBeat = m_nBeatPersons;
    }
    return (nBeat > m_nPresentPeak) ? nBeat : m_nPresentPeak;
}

int32_t CAttendanceLedger::namedNum() const
{
    int32_t nNum = 0;
    for (const auto &stRow : m_vecRow)
    {
        if (stRow.bCounted && stRow.bNamed)
        {
            ++nNum;
        }
    }
    return nNum;
}

int32_t CAttendanceLedger::expectedNum() const
{
    /* 三级来源，优先级写在这里而不是散在调用点：课表 > 配置 > 底库。
       三档都可能是"不知道"，返回 -1 让上层上报"无应到值"而不是猜一个 */
    if (m_nSessionExpected >= 0)
    {
        return m_nSessionExpected;
    }
    if (m_stParam.nExpectedNum >= 0)
    {
        return m_stParam.nExpectedNum;
    }
    if (nullptr != m_pGallery)
    {
        const int32_t nEnroll = m_pGallery->enrollNum();
        if (nEnroll >= 0)
        {
            return nEnroll;
        }
    }
    return -1;
}

int32_t CAttendanceLedger::absentNum() const
{
    const int32_t nExpected = expectedNum();
    if (nExpected < 0)
    {
        /* 未知应到不能返回 0：0 会被平台读成"一个没缺"，而实情是"设备不知道应到几个"。
           CSV 26 要的是差异值，那就照差异值给——给不出来就明说给不出来 */
        return -1;
    }
    /* 有符号：>0 是缺勤，<0 是超员（旁听/走错教室/以及人数被 ID 抖动撑大）。
       超员会以负差异出现在这张表上，这是"到课人数被跟踪器换 ID 撑高"最直接的读数 */
    return nExpected - presentNum();
}

int32_t CAttendanceLedger::roster(std::vector<CAttendRec_S> &rvecOut) const
{
    rvecOut.clear();
    for (const auto &stRow : m_vecRow)
    {
        if (stRow.bCounted && stRow.bNamed)
        {
            rvecOut.push_back(stRow);
        }
    }
    /* 按首次进画面上课的顺序：报表要能复现，而"表遍历序"在两拍之间会变 */
    std::sort(rvecOut.begin(), rvecOut.end(), [](const CAttendRec_S &stA, const CAttendRec_S &stB)
    {
        if (stA.llFirstMs != stB.llFirstMs)
        {
            return stA.llFirstMs < stB.llFirstMs;
        }
        return stA.strPersonId < stB.strPersonId;
    });
    return static_cast<int32_t>(rvecOut.size());
}

int32_t CAttendanceLedger::records(std::vector<CAttendRec_S> &rvecOut) const
{
    rvecOut.clear();
    for (const auto &stRow : m_vecRow)
    {
        if (!stRow.strKey.empty())
        {
            rvecOut.push_back(stRow);
        }
    }
    std::sort(rvecOut.begin(), rvecOut.end(), [](const CAttendRec_S &stA, const CAttendRec_S &stB)
    {
        if (stA.llFirstMs != stB.llFirstMs)
        {
            return stA.llFirstMs < stB.llFirstMs;
        }
        return stA.nTrackId < stB.nTrackId;
    });
    return static_cast<int32_t>(rvecOut.size());
}

void CAttendanceLedger::beginSession(int32_t nExpectedNum)
{
    reset();
    m_nSessionExpected = nExpectedNum;
}

void CAttendanceLedger::closeSession(int64_t llNowMs)
{
    if (m_bClosed)
    {
        return;
    }
    m_bClosed = true;
    m_llCloseMs = llNowMs;
    /* 只封不删：报表在这之后还要读名单，而读到的是同一份快照。
       真正的清账在 beginSession（下一节课）或 reset */
    dlog_info("classroom attendance: 考勤会话收尾 到场=%d 认名=%d 行数=%d",
              presentNum(), namedNum(), rowInUse());
}

void CAttendanceLedger::reset()
{
    for (auto &stRow : m_vecRow)
    {
        stRow = CAttendRec_S();
    }
    for (auto &strVote : m_vecVoteId)
    {
        strVote.clear();
    }
    m_vecVoteNum.assign(static_cast<size_t>(CLASSROOM_MAX_PERSONS), 0);
    m_stStat = CAttendStat_S();
    m_bClosed = false;
    m_llCloseMs = 0;
    m_nSessionExpected = -1;
    /* 峰值口径的四格同样要清：漏一格就会把上一节课的峰值带进下一节 */
    m_llBeatMs = 0;
    m_nBeatCounted = 0;
    m_nBeatPersons = 0;
    m_nPresentPeak = 0;
}

void CAttendanceLedger::snapshot(CAttendSnap_S &rsnapOut) const
{
    rsnapOut = CAttendSnap_S();
    rsnapOut.bEnable = m_stParam.bEnable;
    rsnapOut.bClosed = m_bClosed;
    if (!m_stParam.bEnable)
    {
        /* 关掉考勤时四个数保持 -1：现场把考勤关了，读数是"这台设备不报考勤"，
           而不是"这节课 0 人到课" */
        return;
    }
    rsnapOut.nPresentNum = presentNum();
    rsnapOut.nNamedNum = namedNum();
    rsnapOut.nExpectedNum = expectedNum();
    rsnapOut.nAbsentNum = absentNum();
    /* 应到未知时 absentNum() 返回的那个 -1 是哨兵而不是"超员一人"，
       两者在报文里必须能分开，所以这里给的是"应到底知不知道" */
    rsnapOut.bAbsentKnown = (rsnapOut.nExpectedNum >= 0);
    roster(rsnapOut.vecRoster);
}

std::string CAttendanceLedger::statText() const
{
    char szBuf[640] = {0};
    const int nLen = snprintf(szBuf, sizeof(szBuf),
                              "att obs=%llu rej=%llu noface=%llu small=%llu nom=%llu miss=%llu low=%llu hit=%llu "
                              "named=%llu merge=%llu smerge=%llu rmer=%llu rebind=%llu drift=%llu full=%llu rec=%llu gal=%llu trunc=%llu "
                              "dup=%llu/%llu/%llu "
                              "fartry=%llu farhit=%llu "
                              "rows=%d pres=%d nm=%d exp=%d abs=%d %s",
                              static_cast<unsigned long long>(m_stStat.u64Obs),
                              static_cast<unsigned long long>(m_stStat.u64RejectObs),
                              static_cast<unsigned long long>(m_stStat.u64NoFace),
                              static_cast<unsigned long long>(m_stStat.u64FaceSmall),
                              static_cast<unsigned long long>(m_stStat.u64NoMatch),
                              static_cast<unsigned long long>(m_stStat.u64MatchMiss),
                              static_cast<unsigned long long>(m_stStat.u64ScoreLow),
                              static_cast<unsigned long long>(m_stStat.u64MatchHit),
                              static_cast<unsigned long long>(m_stStat.u64Named),
                              static_cast<unsigned long long>(m_stStat.u64Merge),
                              static_cast<unsigned long long>(m_stStat.u64SeatMerge),
                              static_cast<unsigned long long>(m_stStat.u64ReentryMerge),
                              static_cast<unsigned long long>(m_stStat.u64SeatRebind),
                              static_cast<unsigned long long>(m_stStat.u64SeatDrift),
                              static_cast<unsigned long long>(m_stStat.u64RowFull),
                              static_cast<unsigned long long>(m_stStat.u64Recycle),
                              static_cast<unsigned long long>(m_stStat.u64GalleryAbsent),
                              static_cast<unsigned long long>(m_stStat.u64NameTruncated),
                              static_cast<unsigned long long>(m_stStat.u64DupSeat),
                              static_cast<unsigned long long>(m_stStat.u64DupPos),
                              static_cast<unsigned long long>(m_stStat.u64DupHead),
                              static_cast<unsigned long long>(m_stStat.u64FaceFarTry),
                              static_cast<unsigned long long>(m_stStat.u64FaceFarHit),
                              rowInUse(), presentNum(), namedNum(), expectedNum(), absentNum(),
                              m_bClosed ? "closed" : "open");
    if (nLen < 0 || nLen >= static_cast<int>(sizeof(szBuf)))
    {
        return "att stat=truncated";
    }
    return std::string(szBuf);
}
}
