/**
 * @FilePath     : person_tracker.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-12 10:40:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-12 10:40:00
 * @Description  : 人头跟踪器实现（贪心一对一匹配 + 归一化中心距离主门限）
 */

#include "person_tracker.h"

#include <algorithm>
#include <cmath>

#include "dlog.h"

namespace Classroom
{
namespace
{
/* 框对角线：归一化中心距离的除数。用对角线而非边长，是因为斜向位移的合成距离
   才和"人移动了多少"这个直觉对齐 */
inline float diagOf(const CRectf_S &stBox)
{
    const float fW = stBox.width();
    const float fH = stBox.height();
    return std::sqrt(fW * fW + fH * fH);
}

/* 框平滑系数：新检测权重。0.5 是折中——太小会让抖动框跟着检测漂移走，
   太大会让真实移动滞后一到两帧（巡视轨迹判定会看到"人停在原地"） */
const float kAlphaSmooth = 0.5f;

/* 速度硬钳：超出人体可能速度的样本按上限计，非有限值按 0 计。
   理由见调用处——EMA 之后仍必须有一次上界，否则坏样本会把外推点甩出真实位置。

   ★ 单位：fVx/fVy 与对外给的 fVxPxPerMs 都是 **像素/毫秒**，而
     CLASSROOM_TRACK_MAX_SPEED_PX_S 定义在 **像素/秒**（与 subject_arbiter 的
     CLASSROOM_MOVE_MAX_SPEED_PX_S 同口径同值）。第一版直接拿两者比较，钳限实际是
     600 px/ms = 600,000px/s，等于没钳——主机用例当场打红（9.6px/ms 穿钳而过，
     正是它该拦的那个值）。故这里除以 1000 换成同一量纲。 */
const float kMaxSpeedPxPerMs = CLASSROOM_TRACK_MAX_SPEED_PX_S / 1000.0f;

inline float clampSpeed(float fSpeed)
{
    if (!std::isfinite(fSpeed))
    {
        return 0.0f;
    }
    if (fSpeed > kMaxSpeedPxPerMs)
    {
        return kMaxSpeedPxPerMs;
    }
    if (fSpeed < -kMaxSpeedPxPerMs)
    {
        return -kMaxSpeedPxPerMs;
    }
    return fSpeed;
}
}

void CPersonTracker::setParam(const CTrackerParam_S &stParam)
{
    m_stParam = stParam;

    if (m_stParam.fGateNormDist <= 0.0f)
    {
        m_stParam.fGateNormDist = CLASSROOM_TRACK_GATE_NORM_DIST;
    }
    if (m_stParam.fMoveSlackPxPerMs < 0.0f)
    {
        /* 0 是合法值（纯静态门限，等价于关掉运动容差），负数才是没配好 */
        m_stParam.fMoveSlackPxPerMs = CLASSROOM_TRACK_MOVE_SLACK_PX_PER_MS;
    }
    if (m_stParam.nMinHits < 1)
    {
        m_stParam.nMinHits = CLASSROOM_TRACK_MIN_HITS;
    }
    if (m_stParam.nKeepMs < 0)
    {
        m_stParam.nKeepMs = CLASSROOM_TRACK_KEEP_MS;
    }
}

void CPersonTracker::clearStat()
{
    m_stStat = CTrackerStat_S();
    m_stStat.nNextId = m_nNextId;
}

void CPersonTracker::reset(bool bResetId)
{
    m_vecTracks.clear();
    m_vecOut.clear();

    if (bResetId)
    {
        /* 只给单测和"整机换镜头几何"这种真需要清零的场合用。
           正常运行永远不传 true：ID 撞号会让时间门把上一个人的持续算到下一个人头上 */
        m_nNextId = 1;
    }

    m_stStat.nNextId = m_nNextId;
}

int32_t CPersonTracker::confirmedNum() const
{
    int32_t nNum = 0;

    for (const CTrackInternal_S &stTrack : m_vecTracks)
    {
        if (stTrack.bConfirmed)
        {
            ++nNum;
        }
    }
    return nNum;
}

float CPersonTracker::matchCost(const CTrackInternal_S &stTrack, const CDetection_S &stDet,
                                int64_t llNowMs) const
{
    /* 回收窗口已过的轨迹不参与本拍关联。这一条必须在匹配处而不仅靠末尾的回收那一支：
       update() 是"先关联、后回收"，而回收要求"本拍未命中"、命中又把 nMiss 清零 ——
       只要门限还放超窗轨迹进来，缺席十几秒的号就会带着上一主人的累计命中数原地复活。
       真机可达：update() 只在人头检测成功那一拍被调（perceive_pipeline.cpp:158 在 runHead
       失败时走不到），模型没驻留/时间片不到时这个间隔就是十几秒。
       挡在这里之后本拍它必然未命中，回收那一支同拍把它清掉，不需要额外的擦除遍历。
       llLastMs 为 0 表示"时基未知"（调用方按帧序推进），不参与年龄判定。 */
    if (stTrack.llLastMs > 0 &&
        llNowMs > stTrack.llLastMs + static_cast<int64_t>(m_stParam.nKeepMs))
    {
        return -1.0f;
    }

    const float fDiagTrack = diagOf(stTrack.stBox);
    const float fDiagDet = diagOf(stDet.stBox);
    /* 除数取两者均值：只用轨迹对角线会在"新目标从小变大"时把匹配距离算小，误关联 */
    const float fDiag = (fDiagTrack + fDiagDet) * 0.5f;

    /* ★ NaN 防护：下面三处「大于/小于」比较在操作数为 NaN 时
       **全部取反**——`NaN <= x` 是 false ⇒ 「NaN 对角线」这一挡形同虚设，于是这条轨迹
       要么根本进不到这里，要么一路通过后面每一道门。而 NaN 框在这一层是可达的：
       检测框由 modelToCanvas 按源尺寸/画布尺寸换算，源尺寸为 0 或缩放因子异常时
       坐标就是 NaN；一次 NaN 被平滑进 stBox 之后，这条轨迹从此把每一拍递给它的
       第一个检测都当成自己——表现为「人数对不上」「同一个人被并到下一个人头上」，
       而日志里一个错都不报。
       所以判据一律写成「必须是有限值且……」的正向形式：`!(x > t)` 把 NaN 与 ≤t 一起拒掉。 */
    if (!(fDiag > 1.0f))
    {
        return -1.0f;
    }

    const int64_t llDtMs = (llNowMs > stTrack.llLastMs) ? (llNowMs - stTrack.llLastMs) : 0;
    const float fDtMs = static_cast<float>(llDtMs);

    /* 按速度外推到本帧时刻再比距离。不做外推的话，走动的人在 5fps 下每帧位移
      （~48px）远大于静止门限（~22px），每个人都会被拆成一串新 ID —— 而这恰是
       课堂最需要的教师巡视轨迹。速度为 0 的新轨迹退化成纯距离门限，不受影响 */
    const CPointf_S stCtl = stTrack.stBox.center();
    const float fPredX = stCtl.fX + stTrack.fVx * fDtMs;
    const float fPredY = stCtl.fY + stTrack.fVy * fDtMs;

    const CPointf_S stCD = stDet.stBox.center();
    const float fDx = stCD.fX - fPredX;
    const float fDy = stCD.fY - fPredY;
    const float fDist = std::sqrt(fDx * fDx + fDy * fDy);

    /* 容差 = 静态门限 + 运动容差，再封顶：
       遮挡一秒后轨迹可能已跑出很远，不封顶会让一条久未更新的轨迹吞掉附近任何新出现的人头 */
    float fBudget = m_stParam.fGateNormDist * fDiag + m_stParam.fMoveSlackPxPerMs * fDtMs;
    const float fCap = CLASSROOM_TRACK_MAX_GATE_DIAG * fDiag;

    if (fBudget > fCap)
    {
        fBudget = fCap;
    }
    /* 同上：fDist 含 NaN（检测框或预测框有一个是 NaN）时必须拒，不能放进成本排序 */
    if (!(fDist <= fBudget))
    {
        return -1.0f;
    }

    /* 门限内再排优先级：用"相对自身容差"的比值而非绝对距离，
       否则小框（远处学生）永远抢不过大框（近处学生），近处轨迹会把远处检测吸走 */
    const float fNorm = fDist / fBudget;

    /* 尺寸比惩罚（#93-2，相对量）：乘在成本上而不是加一道硬门——比值=1 时乘数恰为 1，
       普通目标的成本与旧式逐位相同；尺寸不符的候选被抬高后，贪心自动让位。
       权重为 0 时整段跳过，与改造前完全同路（配置层的回滚通道）。
       IoU 项的语义不变：重叠度高说明确实是同一个目标，让"边缘处两个挨着的头"
       优先匹配到重叠的那个 */
    float fCost = fNorm * (1.0f - 0.3f * rectIoU(stTrack.stBox, stDet.stBox));
    /* NaN 只会让这一段的乘数变成 NaN 从而把整条成本变成 NaN；上面两道门已挡掉主路径，
       这里再挡一次是为了「配置把尺寸惩罚关掉以外的任何路径」都拿不到 NaN 成本——
       成本是 NaN 时排序比较全 false，贪心会把它排到队首（谁都不小于它） */
    float fRatio = (fDiagTrack > fDiagDet) ? (fDiagTrack / fDiagDet)
                                           : (fDiagDet / fDiagTrack);
    if (m_stParam.fSizePenalty > 0.0f && fDiagTrack > 1.0f && fDiagDet > 1.0f &&
        std::isfinite(fRatio) && fRatio > 0.0f)
    {
        fCost *= 1.0f + m_stParam.fSizePenalty * (fRatio - 1.0f);
    }
    return fCost;
}

void CPersonTracker::associate(int32_t nDetNum, std::vector<int32_t> &rvecDetOfTrack,
                               std::vector<int32_t> &rvecTrackOfDet)
{
    const int32_t nTrackNum = static_cast<int32_t>(m_vecTracks.size());

    rvecDetOfTrack.assign(nTrackNum, -1);
    rvecTrackOfDet.assign(nDetNum, -1);

    if (nTrackNum <= 0 || nDetNum <= 0)
    {
        return;
    }

    /* 代价表已在 update() 里按 (track, det) 展开并排好序（m_vecDetOrder 存扁平下标，
       不可关联的配对根本没进表），这里只做"贪心取最小、双方占用即跳过"，
       一对一保证不会被一个高 IoU 检测抢走多条轨迹 */
    for (int32_t nOrder : m_vecDetOrder)
    {
        const int32_t nT = nOrder / nDetNum;
        const int32_t nD = nOrder % nDetNum;

        if (nT >= nTrackNum || nD >= nDetNum)
        {
            continue;
        }
        if (rvecDetOfTrack[nT] >= 0 || rvecTrackOfDet[nD] >= 0)
        {
            continue;
        }

        rvecDetOfTrack[nT] = nD;
        rvecTrackOfDet[nD] = nT;
    }
}

const std::vector<CTrackItem_S> &CPersonTracker::update(const std::vector<CDetection_S> &vecDet, int64_t llNowMs)
{
    m_vecOut.clear();

    /* 只保留尺寸合法的检测：小框中心不稳，留着会抢邻座 ID（见 nMinBoxEdge 注释）。
       这里不做 NMS——检测器输出的框由模型侧 NMS 保证互不重叠，重复一遍只会误伤相邻两人 */
    std::vector<const CDetection_S *> &vecPick = m_vecPick;
    std::vector<int32_t> &vecPickIdx = m_vecPickIdx;
    vecPick.clear();
    vecPickIdx.clear();

    for (int32_t nDet = 0; nDet < static_cast<int32_t>(vecDet.size()); ++nDet)
    {
        const CDetection_S &stDet = vecDet[nDet];

        if (stDet.stBox.width() < static_cast<float>(m_stParam.nMinBoxEdge) ||
            stDet.stBox.height() < static_cast<float>(m_stParam.nMinBoxEdge))
        {
            continue;
        }
        if (stDet.fScore <= 0.0f || stDet.fScore > 1.0f)
        {
            continue;
        }
        if (static_cast<int32_t>(vecPick.size()) >= CLASSROOM_TRACK_MAX)
        {
            /* 超出容量说明上游没做检出数上限，这里保底丢弃并计数（需求 10 的有界红线） */
            ++m_stStat.u64TracksDropped;
            break;
        }
        vecPick.push_back(&stDet);
        /* 下标必须在入选时就记：过滤器会丢掉小框/坏分数，pick 位置不等于入参位置，
           而感知层拿到的 nDetIdx 是它自己那张 vecDet 的下标 */
        vecPickIdx.push_back(nDet);
    }

    const int32_t nTrackNum = static_cast<int32_t>(m_vecTracks.size());
    const int32_t nDetNum = static_cast<int32_t>(vecPick.size());

    m_stStat.u64Updates++;
    m_stStat.u64Detections += static_cast<uint64_t>(nDetNum);

    /* 建代价表：nTrackNum×nDetNum 个 float，两块缓冲都在构造期定容，运行期零分配 */
    std::vector<int32_t> &vecDetOfTrack = m_vecDetOfTrack;
    std::vector<int32_t> &vecTrackOfDet = m_vecTrackOfDet;

    if (nTrackNum > 0 && nDetNum > 0)
    {
        m_vecCost.assign(static_cast<size_t>(nTrackNum) * nDetNum, -1.0f);
        m_vecDetOrder.clear();
        m_vecDetOrder.reserve(m_vecCost.size());

        for (int32_t nT = 0; nT < nTrackNum; ++nT)
        {
            for (int32_t nD = 0; nD < nDetNum; ++nD)
            {
                const int32_t nIdx = nT * nDetNum + nD;
                const float fCost = matchCost(m_vecTracks[nT], *vecPick[nD], llNowMs);

                /* 负值=超门限，不进有序表：升序排序会把它排在最前，
                   留到匹配阶段再判负等于让每一轮都先撞一遍不可能配对 */
                if (fCost < 0.0f)
                {
                    continue;
                }
                m_vecCost[nIdx] = fCost;
                m_vecDetOrder.push_back(nIdx);
            }
        }

        std::sort(m_vecDetOrder.begin(), m_vecDetOrder.end(),
                  [this](int32_t nA, int32_t nB) { return m_vecCost[nA] < m_vecCost[nB]; });
    }
    else
    {
        m_vecCost.clear();
        m_vecDetOrder.clear();
    }

    associate(nDetNum, vecDetOfTrack, vecTrackOfDet);

    /* 1) 命中：更新框/速度；未命中的先不删，等时间片到（见 3)） */
    for (int32_t nT = 0; nT < nTrackNum; ++nT)
    {
        CTrackInternal_S &stTrack = m_vecTracks[nT];
        const int32_t nD = vecDetOfTrack[nT];

        if (nD < 0)
        {
            stTrack.bAliveThisFrame = false;
            stTrack.nDetIdx = -1;
            continue;
        }

        const CDetection_S &stDet = *vecPick[nD];
        stTrack.bAliveThisFrame = true;
        stTrack.nMiss = 0;
        stTrack.nDetIdx = vecPickIdx[nD];
        ++stTrack.nHits;
        stTrack.stRawBox = stDet.stBox;
        stTrack.fScore = stDet.fScore;
        stTrack.enSource = stDet.enSource;

        const CPointf_S stOldC = stTrack.stBox.center();

        stTrack.stBox.fX1 = stTrack.stBox.fX1 + kAlphaSmooth * (stDet.stBox.fX1 - stTrack.stBox.fX1);
        stTrack.stBox.fY1 = stTrack.stBox.fY1 + kAlphaSmooth * (stDet.stBox.fY1 - stTrack.stBox.fY1);
        stTrack.stBox.fX2 = stTrack.stBox.fX2 + kAlphaSmooth * (stDet.stBox.fX2 - stTrack.stBox.fX2);
        stTrack.stBox.fY2 = stTrack.stBox.fY2 + kAlphaSmooth * (stDet.stBox.fY2 - stTrack.stBox.fY2);

        const int64_t llDtMs = (llNowMs > stTrack.llLastMs) ? (llNowMs - stTrack.llLastMs) : 0;

        if (llDtMs > 0)
        {
            const CPointf_S stNewC = stTrack.stBox.center();
            /* 速度同样平滑：单帧跳变（漏检一帧后框跳回真位置）会被当成"移动很快"，
               进而把巡视判定刷成抖动 */
            const float fNewVx = (stNewC.fX - stOldC.fX) / static_cast<float>(llDtMs);
            const float fNewVy = (stNewC.fY - stOldC.fY) / static_cast<float>(llDtMs);
            stTrack.fVx = stTrack.fVx + kAlphaSmooth * (fNewVx - stTrack.fVx);
            stTrack.fVy = stTrack.fVy + kAlphaSmooth * (fNewVy - stTrack.fVy);
            /* EMA 只削幅度不设上界：一次 3000px/s 的坏样本经 α 平滑后仍会把速度抬到几百，
               而速度会直接进 matchCost 的外推 predPos = cur + v×Δt。那之后连续几拍的
               预测点都在真实位置之外，本该匹配上的检测全部"离预测点太远"而失配——
               表现是同一个人被拆成 2~3 个新 ID，而人数/行为连贯性全跟着碎。
               所以平滑之后必须过一次硬钳：超过人体可能速度的样本只按上限计，
               等真实位移出来再把 EMA 拉回来（约 5 拍收敛）。
               非有限值（NaN/Inf）一律按 0：速度一旦是 NaN，外推点就是 NaN，
               而 matchCost 里"NaN > 预算"恒 false ⇒ 该轨迹吞掉附近一切检测。 */
            stTrack.fVx = clampSpeed(stTrack.fVx);
            stTrack.fVy = clampSpeed(stTrack.fVy);
        }

        stTrack.llLastMs = llNowMs;

        if (!stTrack.bConfirmed && stTrack.nHits >= m_stParam.nMinHits)
        {
            stTrack.bConfirmed = true;
        }
        ++m_stStat.u64Matched;
    }

    /* 2) 新建：未匹配的检测立刻给 ID，但 bConfirmed=false，由行为层按命中数门控。
       不"确认后才可见"是有意的：那样第一帧就丢一个真人，5fps 下等于白丢 200ms 证据 */
    for (int32_t nD = 0; nD < nDetNum; ++nD)
    {
        if (nD < static_cast<int32_t>(vecTrackOfDet.size()) && vecTrackOfDet[nD] >= 0)
        {
            continue;
        }
        if (static_cast<int32_t>(m_vecTracks.size()) >= CLASSROOM_TRACK_MAX)
        {
            /* +1 记的是「这一拍有检测没起上轨」，不是「丢了几个检测」：break 之后本拍剩余候选
               一起放弃，再数个数只会让读数随检测顺序漂移。语义见 CTrackerStat_S 的注释 */
            ++m_stStat.u64TrackPoolFull;
            break;
        }

        const CDetection_S &stDet = *vecPick[nD];
        /* NaN/Inf 检测不建轨迹：检测框经 modelToCanvas 按源尺寸换算，
           源尺寸/缩放因子异常时坐标就是 NaN；而"非有限"在下面的 matchCost 里
           **骗得过每一道门比较**（`NaN <= 1.0` 为 false），所以这里不挡的话，
           一条 NaN 轨迹从此把每拍递给它的第一个检测都当成自己——日志一个错都不报，
           表现是「人数对不上 / 同一个人被并到别人名下」，是最难查的那一类静默失效。
           丢掉一帧是可恢复的（下一拍坐标正常就重新建），错一次 ID 则要污染一节课。 */
        if (!std::isfinite(stDet.stBox.fX1) || !std::isfinite(stDet.stBox.fY1) ||
            !std::isfinite(stDet.stBox.fX2) || !std::isfinite(stDet.stBox.fY2))
        {
            continue;
        }
        if (stDet.stBox.width() <= 0.0f || stDet.stBox.height() <= 0.0f)
        {
            /* 反向/零面积框同样不建轨迹：中心与对角线都退化 */
            continue;
        }
        CTrackInternal_S stNew;
        stNew.nId = m_nNextId++;
        stNew.stBox = stDet.stBox;
        stNew.stRawBox = stDet.stBox;
        stNew.nHits = 1;
        stNew.fScore = stDet.fScore;
        stNew.enSource = stDet.enSource;
        stNew.llLastMs = llNowMs;
        stNew.nDetIdx = vecPickIdx[nD];
        stNew.bAliveThisFrame = true;
        stNew.bConfirmed = (m_stParam.nMinHits <= 1);

        m_vecTracks.push_back(stNew);
        ++m_stStat.u64TracksCreated;
    }

    /* 3) 回收：按时间而非帧数（帧率会随降级变化）。原地压缩，不产生新分配 */
    int32_t nKeep = 0;

    for (int32_t nT = 0; nT < static_cast<int32_t>(m_vecTracks.size()); ++nT)
    {
        CTrackInternal_S &stTrack = m_vecTracks[nT];

        if (!stTrack.bAliveThisFrame)
        {
            ++stTrack.nMiss;
        }

        const bool bExpired = (stTrack.nMiss > 0) &&
                              (llNowMs > 0 && stTrack.llLastMs > 0) &&
                              (llNowMs - stTrack.llLastMs > static_cast<int64_t>(m_stParam.nKeepMs));

        if (bExpired)
        {
            continue;
        }
        if (nKeep != nT)
        {
            m_vecTracks[nKeep] = stTrack;
        }
        ++nKeep;
    }
    m_vecTracks.resize(nKeep);

    /* 4) 出参：只输出本帧有框的轨迹（丢失中的不输出——下游若按它算人数会虚高，
       而"这一帧没看到"本来就该由行为层的中断容忍去处理，不是靠跟踪器补帧） */
    m_vecOut.reserve(m_vecTracks.size());

    for (const CTrackInternal_S &stTrack : m_vecTracks)
    {
        if (!stTrack.bAliveThisFrame)
        {
            continue;
        }

        CTrackItem_S stItem;
        stItem.nTrackId = stTrack.nId;
        stItem.stBox = stTrack.stBox;
        stItem.stRawBox = stTrack.stRawBox;
        stItem.nHits = stTrack.nHits;
        stItem.nConsecutiveMiss = 0;
        stItem.bConfirmed = stTrack.bConfirmed;
        stItem.bNewThisFrame = (stTrack.nHits <= 1);
        stItem.nDetIdx = stTrack.nDetIdx;
        stItem.fVxPxPerMs = stTrack.fVx;
        stItem.fVyPxPerMs = stTrack.fVy;
        stItem.enSource = stTrack.enSource;
        stItem.fScore = stTrack.fScore;
        m_vecOut.push_back(stItem);
    }

    m_stStat.nActiveMax = std::max(m_stStat.nActiveMax, static_cast<int32_t>(m_vecTracks.size()));
    m_stStat.nConfirmedMax = std::max(m_stStat.nConfirmedMax, confirmedNum());
    m_stStat.nNextId = m_nNextId;

    return m_vecOut;
}

}
