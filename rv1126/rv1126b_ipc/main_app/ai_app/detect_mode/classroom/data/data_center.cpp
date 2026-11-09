/**
 * @FilePath     : data_center.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-22 14:47:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-22 14:48:00
 * @Description  : 数据聚合中心实现
 */

#include "data_center.h"

#include <algorithm>
#include <cstdio>

#include "classroom_clock.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* 环容量的硬上界 */
constexpr int32_t RING_NUM_MAX_TIMES = 4;

/* 两次采样之间允许积分的最大间隔。超过就当作**断档**而不是"这段时间维持上一拍的状态" */
constexpr int64_t ACCUM_GAP_TIMES = 3;

inline int32_t behaviorColOf(Behavior_E enBehavior)
{
    return CDurationGate::behaviorColumn(enBehavior);
}

/* 情绪人次：离散计数，不参与时间积分 */
inline void addExprSamples(CSessionAccum_S &rstSess, const CGroupMetric_S &stSample)
{
    for (int nExpr = 0; nExpr < CLASSROOM_EXPRESSION_CLASS_NUM; ++nExpr)
    {
        const int32_t nNum = stSample.nExprNum[nExpr];
        if (nNum > 0)
        {
            rstSess.u64ExprNum[nExpr] += static_cast<uint64_t>(nNum);
        }
    }
}
}

bool CDataCenter::init(const CDataCenterParam_S &stParam)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (stParam.nSampleRingNum <= 0 || stParam.nEventRingNum <= 0 ||
        stParam.nSampleRingNum > CLASSROOM_METRIC_RING * RING_NUM_MAX_TIMES ||
        stParam.nEventRingNum > CLASSROOM_EVENT_RING * RING_NUM_MAX_TIMES ||
        stParam.nReportBatchEvent <= 0 || stParam.llReportIntervalMs <= 0)
    {
        dlog_error("classroom data: 参数非法 sample=%d event=%d batch=%d report=%lldms",
                   stParam.nSampleRingNum, stParam.nEventRingNum, stParam.nReportBatchEvent,
                   static_cast<long long>(stParam.llReportIntervalMs));
        return false;
    }

    /* resize 而不是 reserve：环的语义要求每一位都可写，容量一次定死后运行期不再变化，
       于是这块内存的大小在 init 当天就能算出来*/
    m_vecSample.assign(static_cast<size_t>(stParam.nSampleRingNum), CGroupMetric_S());
    m_vecEvent.assign(static_cast<size_t>(stParam.nEventRingNum), CBehaviorEvent_S());

    m_stParam = stParam;
    m_u64SampleIn = 0;
    m_u64SampleSent = 0;
    m_nSampleNum = 0;
    m_u64EventIn = 0;
    m_u64EventTaken = 0;
    m_u64EventLost = 0;
    m_u64EventLostSent = 0;
    m_nEventNum = 0;
    m_stSession = CSessionAccum_S();
    m_stStat = CDataCenterStat_S();
    m_bReady = true;

    dlog_info("classroom data: 就绪，ring=%d|%d batch=%d report=%lldms，hold=%.1fKB",
              stParam.nSampleRingNum, stParam.nEventRingNum, stParam.nReportBatchEvent,
              static_cast<long long>(stParam.llReportIntervalMs),
              static_cast<double>(m_vecSample.capacity() * sizeof(CGroupMetric_S) +
                                  m_vecEvent.capacity() * sizeof(CBehaviorEvent_S)) / 1024.0);
    return true;
}

void CDataCenter::deinit()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    m_bReady = false;
    m_vecSample.clear();
    m_vecSample.shrink_to_fit();
    m_vecEvent.clear();
    m_vecEvent.shrink_to_fit();
    m_stSession = CSessionAccum_S();

    /* 环与游标、观察件一起归零 */
    m_u64SampleIn = 0;
    m_u64SampleSent = 0;
    m_nSampleNum = 0;
    m_u64EventIn = 0;
    m_u64EventTaken = 0;
    m_u64EventLost = 0;
    m_u64EventLostSent = 0;
    m_nEventNum = 0;
    m_stStat = CDataCenterStat_S();
}

bool CDataCenter::isReady() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_bReady;
}

void CDataCenter::pushSample(const CGroupMetric_S &stSample)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady || m_vecSample.empty() || stSample.llSampleMs <= 0)
    {
        return;
    }

    const size_t nCap = m_vecSample.size();
    m_vecSample[static_cast<size_t>(m_u64SampleIn % nCap)] = stSample;
    ++m_u64SampleIn;
    ++m_stStat.u64SampleIn;

    if (m_nSampleNum < nCap)
    {
        ++m_nSampleNum;
    }
    else
    {
        /* 覆盖最旧 */
        ++m_stStat.u64SampleOverwrite;
    }
    m_stStat.nSampleHold = static_cast<int32_t>(m_nSampleNum);

    accumulateLocked(stSample, stSample.llSampleMs);
}

CGroupMetric_S CDataCenter::latest() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    CGroupMetric_S stOut;
    if (!m_bReady || (0 == m_nSampleNum) || m_vecSample.empty())
    {
        return stOut;
    }
    const size_t nCap = m_vecSample.size();
    stOut = m_vecSample[static_cast<size_t>((m_u64SampleIn - 1) % nCap)];
    return stOut;
}

uint64_t CDataCenter::eventUnsentLocked() const
{
    /* 环内可读区间是 (ullIn - nNum, ullIn]，所以"已经不在环里"的最大序号 = ullIn - nNum。
       m_nEventNum 恒 <= 累计写入条数，这里不会下溢 */
    const uint64_t ullGone = m_u64EventIn - m_nEventNum;
    const uint64_t ullFloor = std::max(m_u64EventTaken, ullGone);

    return (m_u64EventIn > ullFloor) ? (m_u64EventIn - ullFloor) : 0;
}

uint64_t CDataCenter::pushEvent(const CBehaviorEvent_S &stEvent)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady || m_vecEvent.empty())
    {
        return 0;
    }

    const size_t nCap = m_vecEvent.size();
    /* 序号在入库这一拍分配 */
    const uint64_t u64Seq = m_u64EventIn + 1;
    m_vecEvent[static_cast<size_t>(m_u64EventIn % nCap)] = stEvent;
    m_vecEvent[static_cast<size_t>(m_u64EventIn % nCap)].u64EventSeq = u64Seq;
    ++m_u64EventIn;
    ++m_stStat.u64EventIn;

    if (m_nEventNum < nCap)
    {
        ++m_nEventNum;
    }
    else
    {
        ++m_stStat.u64EventOverwrite;
        if (m_u64EventIn - nCap > m_u64EventTaken)
        {
            ++m_u64EventLost;
        }
    }
    m_stStat.nEventHold = static_cast<int32_t>(m_nEventNum);
    m_stStat.u64EventUnsent = eventUnsentLocked();

    if (stEvent.nPersonId < 0)
    {
        return u64Seq;    /* 不该出现：防御性挡一刀，避免把无效行记进会话账 */
    }
    if (stEvent.bBegin)
    {
        ++m_stSession.u64BeginEvt;
        /* 逐行为的次数 */
        const int32_t nCol = behaviorColOf(stEvent.enBehavior);
        if (nCol >= 0 && nCol < CDurationGate::BEHAVIOR_COLUMN_NUM)
        {
            ++m_stSession.u64BeginAt[nCol];
        }
    }
    else
    {
        ++m_stSession.u64EndEvt;
    }
    return u64Seq;
}

void CDataCenter::accumulateLocked(const CGroupMetric_S &stSample, int64_t llToMs)
{
    CSessionAccum_S &stSess = m_stSession;

    if (stSess.llBeginMs <= 0)
    {
        stSess.llBeginMs = llToMs;
    }
    if (stSample.nPersonNum > 0)
    {
        /* 「有人在场」的时刻单独记（#95 静默收尾的起点）：llLastMs 每拍都推，
           分不出「人走光了」和「流断了」——这两件事的处置相反 */
        stSess.llLastPresentMs = llToMs;
    }
    if (stSess.llLastMs <= 0)
    {
        /* 第一条只定起点，不积分：没有"上一拍"就没有可归属的区间。
           把第一条当成"从 0 时刻起一直生效"会让会话第一分钟的占比虚高到不可用。
           但它确实是一个样本，计入 nSampleNum —— 报表可信度看的是"有几个样本" */
        ++stSess.nSampleNum;
        if (stSample.nPersonNum > stSess.nPersonMax)
        {
            stSess.nPersonMax = stSample.nPersonNum;
        }
        if (stSample.bTruncated)
        {
            stSess.bTruncatedSeen = true;
        }
        addExprSamples(stSess, stSample);   /* 第一条是有效样本：情绪人脸照记一次 */
        stSess.llLastMs = llToMs;
        return;
    }
    if (llToMs <= stSess.llLastMs)
    {
        return;    /* 时钟不回退是前提，回退了（换时基、回放混进来）就跳过这一段而不是积负数 */
    }

    addExprSamples(stSess, stSample);

    const int64_t llDt = llToMs - stSess.llLastMs;
    const int64_t llGapMax = m_stParam.llReportIntervalMs * ACCUM_GAP_TIMES;
    if (llDt > llGapMax)
    {
        /* 断档：只把起点推到现在，不补这段的账。
           现象上是"这节课的站立时长少了 10 分钟"，而真实情况是那 10 分钟根本没有证据 */
        ++m_stStat.u64AccumGap;
        stSess.llLastMs = llToMs;
        return;
    }

    if (stSample.nPersonNum > 0)
    {
        stSess.llPresentMs += llDt;
    }
    for (int nCol = 0; nCol < CDurationGate::BEHAVIOR_COLUMN_NUM; ++nCol)
    {
        if (stSample.nActive[nCol] > 0)
        {
            stSess.llActiveMs[nCol] += llDt * stSample.nActive[nCol];
        }
    }
    if (stSample.nPersonNum > stSess.nPersonMax)
    {
        stSess.nPersonMax = stSample.nPersonNum;
    }
    if (stSample.bTruncated)
    {
        stSess.bTruncatedSeen = true;
    }
    ++stSess.nSampleNum;
    stSess.llLastMs = llToMs;
}

bool CDataCenter::buildReport(int64_t llNowMs, CReportBundle_S &rstOut)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    rstOut = CReportBundle_S();
    if (!m_bReady || 0 == m_u64SampleIn)
    {
        ++m_stStat.u64ReportSkip;
        return false;
    }

    const size_t nCap = m_vecSample.size();
    const uint64_t ullOldest = m_u64SampleIn - m_nSampleNum + 1;    /* 环里还留着的最旧序号 */
    uint64_t ullFrom = (m_u64SampleSent + 1 > ullOldest) ? m_u64SampleSent + 1 : ullOldest;

    for (uint64_t ullSeq = ullFrom; ullSeq <= m_u64SampleIn; ++ullSeq)
    {
        rstOut.vecTimeline.push_back(m_vecSample[static_cast<size_t>((ullSeq - 1) % nCap)]);
    }
    rstOut.stLatest = m_vecSample[static_cast<size_t>((m_u64SampleIn - 1) % nCap)];
    m_u64SampleSent = m_u64SampleIn;

    /* 事件是队列语义：只带走没取过的，且一次最多 nReportBatchEvent 条。
       不带全部是为了不让"满座 + 行为密集"那一拍把报文撑成不可发的尺寸；
       剩下的留在环里下次带走，环满了覆盖最旧 —— 于是 nEventDropped 必须给出去，
       平台侧据此显示"事件较多，此处仅展示部分"，而不是悄悄少报 */
    const size_t nEvCap = m_vecEvent.size();
    const uint64_t ullEvOldest = (m_u64EventIn >= m_nEventNum) ? (m_u64EventIn - m_nEventNum + 1) : 1;
    uint64_t ullEvFrom = (m_u64EventTaken + 1 > ullEvOldest) ? m_u64EventTaken + 1 : ullEvOldest;
    while (ullEvFrom <= m_u64EventIn &&
           static_cast<int32_t>(rstOut.vecEvents.size()) < m_stParam.nReportBatchEvent)
    {
        rstOut.vecEvents.push_back(m_vecEvent[static_cast<size_t>((ullEvFrom - 1) % nEvCap)]);
        ++ullEvFrom;
    }
    m_u64EventTaken = ullEvFrom - 1;
    rstOut.nEventDropped = static_cast<int32_t>(eventUnsentLocked());
    m_stStat.u64EventUnsent = rstOut.nEventDropped;

    /* 丢失给的是**本次周期内的增量**：平台每收到一份报表都要能回答"这段时间我少看了几条"，
       给累计值的话它得自己记住上一份读到哪儿，那等于把配对责任推给对端 */
    rstOut.nEventLost = static_cast<int32_t>(m_u64EventLost - m_u64EventLostSent);
    m_u64EventLostSent = m_u64EventLost;

    rstOut.stSession = m_stSession;
    rstOut.bSessionReset = false;
    ++m_stStat.u64ReportBuild;

    (void)llNowMs;    /* 取数时刻由样本自己带；这里不用它，避免"按墙钟补一段没证据的账" */
    return true;
}

bool CDataCenter::closeSession(int64_t llNowMs, CReportBundle_S &rstOut)
{
    if (!buildReport(llNowMs, rstOut))
    {
        return false;
    }

    std::lock_guard<std::mutex> stLock(m_mtx);
    rstOut.bSessionReset = true;
    /* 收尾后把会话账本清零，但保留环里的样本：下一节课的第一份报表仍要能接上时间线，
       而账本不能接 —— 否则"这节课"的占比里混着上一节课的人 */
    m_stSession = CSessionAccum_S();
    return true;
}

CDataCenterStat_S CDataCenter::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stStat;
}

void CDataCenter::resetStat()
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stStat = CDataCenterStat_S();
    m_stStat.nSampleHold = static_cast<int32_t>(m_nSampleNum);
    m_stStat.nEventHold = static_cast<int32_t>(m_nEventNum);
    m_stStat.u64EventUnsent = eventUnsentLocked();
}

CSessionAccum_S CDataCenter::session() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stSession;
}

void CDataCenter::setReportIntervalMs(int64_t llReportMs)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (llReportMs <= 0 || llReportMs == m_stParam.llReportIntervalMs)
    {
        /* 非正在生效的值不打日志：一次下发没动这个键也会走到这里，
           而配置换入是热路径，心跳里凭空多一串"report interval changed"会带偏排查 */
        return;
    }

    dlog_info("classroom data: 上报间隔 %lldms -> %lldms（gap 基线 %lldms）",
              static_cast<long long>(m_stParam.llReportIntervalMs),
              static_cast<long long>(llReportMs),
              static_cast<long long>(llReportMs * ACCUM_GAP_TIMES));
    m_stParam.llReportIntervalMs = llReportMs;
}

std::string CDataCenter::statLine() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    char szBuf[256] = {0};
    snprintf(szBuf, sizeof(szBuf),
             "samp=%llu ow=%llu hold=%d/%d evt=%llu|%d unsent=%llu build=%llu skip=%llu gap=%llu "
             "present=%llds max=%d",
             static_cast<unsigned long long>(m_stStat.u64SampleIn),
             static_cast<unsigned long long>(m_stStat.u64SampleOverwrite),
             m_stStat.nSampleHold, m_stParam.nSampleRingNum,
             static_cast<unsigned long long>(m_stStat.u64EventIn), m_stStat.nEventHold,
             static_cast<unsigned long long>(m_stStat.u64EventUnsent),
             static_cast<unsigned long long>(m_stStat.u64ReportBuild),
             static_cast<unsigned long long>(m_stStat.u64ReportSkip),
             static_cast<unsigned long long>(m_stStat.u64AccumGap),
             static_cast<long long>(m_stSession.llPresentMs / 1000), m_stSession.nPersonMax);
    return std::string(szBuf);
}

std::string CDataCenter::statJson() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    const int nStandCol = behaviorColOf(Behavior_E::STU_STAND_UP);
    const int nUpCol = behaviorColOf(Behavior_E::STU_HEAD_UP);
    const int nPodiumCol = behaviorColOf(Behavior_E::TEA_ENTER_PODIUM);
    const int nPatrolCol = behaviorColOf(Behavior_E::TEA_PATROL);

    char szBuf[640] = {0};
    snprintf(szBuf, sizeof(szBuf),
             "{\"sample_in\":%llu,\"sample_overwrite\":%llu,\"sample_hold\":%d,"
             "\"event_in\":%llu,\"event_unsent\":%llu,\"report_build\":%llu,\"report_skip\":%llu,"
             "\"accum_gap\":%llu,\"session_begin_ms\":%lld,\"present_s\":%lld,"
             "\"stand_person_s\":%lld,\"headup_person_s\":%lld,\"person_max\":%d,\"samples\":%d,"
             "\"truncated\":%d,"
             "\"tea_podium_s\":%lld,\"tea_patrol_s\":%lld,\"tea_patrol_times\":%llu}",
             static_cast<unsigned long long>(m_stStat.u64SampleIn),
             static_cast<unsigned long long>(m_stStat.u64SampleOverwrite),
             m_stStat.nSampleHold,
             static_cast<unsigned long long>(m_stStat.u64EventIn),
             static_cast<unsigned long long>(m_stStat.u64EventUnsent),
             static_cast<unsigned long long>(m_stStat.u64ReportBuild),
             static_cast<unsigned long long>(m_stStat.u64ReportSkip),
             static_cast<unsigned long long>(m_stStat.u64AccumGap),
             static_cast<long long>(m_stSession.llBeginMs),
             static_cast<long long>(m_stSession.llPresentMs / 1000),
             static_cast<long long>((nStandCol >= 0 ? m_stSession.llActiveMs[nStandCol] : 0) / 1000),
             static_cast<long long>((nUpCol >= 0 ? m_stSession.llActiveMs[nUpCol] : 0) / 1000),
             m_stSession.nPersonMax, m_stSession.nSampleNum, m_stSession.bTruncatedSeen ? 1 : 0,
             static_cast<long long>((nPodiumCol >= 0 ? m_stSession.llActiveMs[nPodiumCol] : 0) / 1000),
             static_cast<long long>((nPatrolCol >= 0 ? m_stSession.llActiveMs[nPatrolCol] : 0) / 1000),
             static_cast<unsigned long long>(
                 nPatrolCol >= 0 ? m_stSession.u64BeginAt[nPatrolCol] : 0));
    return std::string(szBuf);
}
}
