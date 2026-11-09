/**
 * @FilePath     : feature_frame.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-23 17:02:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 17:02:00
 * @Description  : 特征帧总线实现
 */

#include "feature_frame.h"

#include "classroom_clock.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* 借出/发布被拒时的日志限流：与 CFrameBus 同一口径，避免异常期把日志刷爆 */
bool shouldLog(uint64_t u64Counter)
{
    return (1 == u64Counter || 0 == u64Counter % 250);
}
}

int64_t poseCycleMs(int32_t nRoundMs, int32_t nPerRound, int32_t nPersonNum)
{
    /* 节拍没量出来就返回 0，而不是替感知层编一个"看起来合理"的一拍人数：
       编出来的数会直接决定后排同学还有没有证据，而第一拍本来就没有产能事实可读 */
    if (nRoundMs <= 0 || nPerRound <= 0)
    {
        return 0;
    }
    const int32_t nPersons = (nPersonNum > 0) ? nPersonNum : 1;
    const int32_t nRound   = (nPersons + nPerRound - 1) / nPerRound;
    return static_cast<int64_t>(nRound) * nRoundMs;
}

int32_t poseEvidenceTtlMs(int32_t nRoundMs, int32_t nPerRound, int32_t nPersonNum)
{
    const int64_t llCycle = poseCycleMs(nRoundMs, nPerRound, nPersonNum);
    if (llCycle <= 0)
    {
        /* 退回下限而不是 0：0 的语义是"骨骼一产出就过期"，那是最窄、不是最保守。
           第一拍之后感知层就会把实测节拍写进帧，这个分支只活几拍 */
        return CLASSROOM_POSE_TTL_MIN_MS;
    }

    /* 宁可算宽不算窄：算窄了后排同学永久 UNKNOWN（表现为整节后排零事件、
       日志里一切正常），算宽了只是判定晚几拍，而晚几拍由时长门挡着。
       上限不是装饰：乘出来的窗口能到分钟级，「这副骨骼还作不作数」这件事就此消失——
       事件会挂在三拍前那个人身上一直不结束。钳位真咬到时要计数，见 poseCapacityShort */
    const int64_t llTtl = llCycle * CLASSROOM_POSE_TTL_ROUNDS;
    return static_cast<int32_t>(clampValue<int64_t>(llTtl, CLASSROOM_POSE_TTL_MIN_MS,
                                                    CLASSROOM_POSE_TTL_MAX_MS));
}

bool poseCapacityShort(int32_t nRoundMs, int32_t nPerRound, int32_t nPersonNum)
{
    const int64_t llCycle = poseCycleMs(nRoundMs, nPerRound, nPersonNum);
    /* 判的是"乘完余量仍然装不进上限"，也就是上面那条钳位**真的咬到了**：
       咬到时下游拿到的窗口已经小于一整圈，圈尾的人会被判证据过期。
       这件事必须是一条能看见的计数，不能只活在注释里 */
    return (llCycle > 0 && llCycle * CLASSROOM_POSE_TTL_ROUNDS > CLASSROOM_POSE_TTL_MAX_MS);
}

void poseCadenceFeed(CPoseCadence_S &stCadence, int32_t nTried, int64_t llNowMs)
{
    if (nTried <= 0 || llNowMs <= 0)
    {
        return;     /* 没做推理的拍不是样本 */
    }

    if (stCadence.llLastWorkMs > 0 && llNowMs > stCadence.llLastWorkMs)
    {
        const int64_t llGapMs = llNowMs - stCadence.llLastWorkMs;
        /* α=1/4（new = old + (sample-old)/4 = (3*old+sample)/4）：再快一档会在两拍之间
           来回跳，再慢一档会让「模型刚换回来」那种真实变慢拖几条才反映出来。
           第一个样本直接取值，不做平均——冷启动时手里只有一条事实，没有先验可言 */
        stCadence.llRoundEwmaMs = (stCadence.llRoundEwmaMs > 0)
                                      ? ((3 * stCadence.llRoundEwmaMs + llGapMs) / 4)
                                      : llGapMs;
    }
    stCadence.llLastWorkMs = llNowMs;

    const int32_t nTried100 = nTried * 100;
    stCadence.nPerRoundEwma100 = (stCadence.nPerRoundEwma100 > 0)
                                     ? ((3 * stCadence.nPerRoundEwma100 + nTried100) / 4)
                                     : nTried100;
}

void poseCadenceOf(const CPoseCadence_S &stCadence, int32_t &rnRoundMs, int32_t &rnPerRound)
{
    rnRoundMs = (stCadence.llRoundEwmaMs > 0)
                    ? static_cast<int32_t>(clampValue<int64_t>(stCadence.llRoundEwmaMs, 0, INT32_MAX))
                    : 0;
    /* 人次一律**向下**取整（不是四舍五入）：估成 1.5 个人/拍时读成 2，圈长立刻少一半、
       窗口跟着少一半——那正好是 #69-A 那条漏检的方向。读成 1 只是把窗口放宽一档，
       代价由上限钳位与 poseCapacityShort 兜着。两害相权：漏检不可见、事件晚结束可见 */
    rnPerRound = stCadence.nPerRoundEwma100 / 100;
}

CFeatureFrameBus::~CFeatureFrameBus()
{
    deinit();
}

bool CFeatureFrameBus::init(int nSlotNum, int nMaxPersons)
{
    const int nSlots = (nSlotNum > 0) ? nSlotNum : CLASSROOM_FEATURE_SLOTS;
    const int nCap   = (nMaxPersons > 0) ? nMaxPersons : CLASSROOM_MAX_PERSONS;

    if (nSlots > CLASSROOM_FEATURE_SLOTS * 4 || nCap > CLASSROOM_MAX_PERSONS)
    {
        /* 上限刻意收紧：容量翻倍等于稳态内存翻倍，而本层的内存图景必须是「init 时算得出来」的，
           运行期改不动的数才能进预算表 */
        dlog_error("feature_bus init: 入参越界 slots=%d cap=%d（上限 %d/%d）",
                   nSlots, nCap, CLASSROOM_FEATURE_SLOTS * 4, CLASSROOM_MAX_PERSONS);
        return false;
    }

    std::vector<std::shared_ptr<Slot>> apSlot;
    try
    {
        for (int i = 0; i < nSlots; ++i)
        {
            auto spSlot = std::make_shared<Slot>();
            spSlot->stFrame.vecPersons.resize(static_cast<size_t>(nCap));
            apSlot.push_back(spSlot);
        }
    }
    catch (const std::bad_alloc &)
    {
        dlog_error("feature_bus init: 申请 %d 槽 × %d 人失败（内存不足）", nSlots, nCap);
        return false;
    }

    const size_t nSlotBytes = sizeof(CFeatureFrame_S) + sizeof(CPersonFeature_S) * static_cast<size_t>(nCap);

    std::lock_guard<std::mutex> stLock(m_mtx);
    m_apSlot    = std::move(apSlot);
    m_nCapacity = nCap;
    m_nLatest   = -1;
    m_u32SeqGen = 0;
    m_bReady    = true;
    m_stStat    = CFeatureBusStat_S();
    m_stStat.u32Slots = static_cast<uint32_t>(nSlots);
    m_stStat.u32SlotBytes = static_cast<uint32_t>(nSlotBytes);

    dlog_info("feature_bus 就绪: slots=%d persons=%d 每槽=%zu KB 常驻=%zu KB",
              nSlots, nCap, nSlotBytes / 1024, nSlotBytes * static_cast<size_t>(nSlots) / 1024);
    return true;
}

void CFeatureFrameBus::deinit()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady)
    {
        return;
    }
    m_bReady = false;

    uint32_t nBusy = 0;
    for (const auto &spSlot : m_apSlot)
    {
        if (spSlot && spSlot.use_count() > 1)
        {
            ++nBusy;
        }
    }
    if (nBusy > 0)
    {
        /* 消费者还握着快照：先停消费者线程再来 deinit，这里只留痕不阻塞（与帧总线一致） */
        dlog_warn("feature_bus deinit: 仍有 %u 个槽被消费者持有", nBusy);
    }

    m_apSlot.clear();
    m_nLatest = -1;
    m_nCapacity = 0;
    dlog_info("feature_bus 已释放");
}

bool CFeatureFrameBus::isReady() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_bReady;
}

FeatureFramePtr CFeatureFrameBus::acquire()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady)
    {
        return FeatureFramePtr();
    }

    const int nSlot = findFreeSlotLocked();
    if (nSlot < 0)
    {
        ++m_stStat.u64DroppedNoSlot;
        if (shouldLog(m_stStat.u64DroppedNoSlot))
        {
            dlog_warn("feature_bus 无空闲槽，本轮感知结果丢弃（累计丢弃=%llu）",
                      static_cast<unsigned long long>(m_stStat.u64DroppedNoSlot));
        }
        return FeatureFramePtr();
    }

    std::shared_ptr<Slot> &spSlot = m_apSlot[nSlot];
    spSlot->stFrame.clear();

    /* 别名 shared_ptr：与槽共享控制块但只暴露帧本体，空闲判定依旧是 use_count()==1 */
    return FeatureFramePtr(spSlot, &spSlot->stFrame);
}

bool CFeatureFrameBus::publish(FeatureFramePtr &rspFrame)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady)
    {
        rspFrame.reset();
        return false;
    }
    if (!rspFrame)
    {
        ++m_stStat.u64RejectedArg;
        return false;
    }

    int nIdx = -1;
    for (size_t i = 0; i < m_apSlot.size(); ++i)
    {
        if (&m_apSlot[i]->stFrame == rspFrame.get())
        {
            nIdx = static_cast<int>(i);
            break;
        }
    }
    if (nIdx < 0)
    {
        /* 借出的帧被搬到别的线程/别的总线上发布，属于调用方缺陷，必须显式失败而不是收下 */
        ++m_stStat.u64RejectedArg;
        if (shouldLog(m_stStat.u64RejectedArg))
        {
            dlog_error("feature_bus publish: 帧 %p 不属于本总线 (rejected=%llu)",
                       rspFrame.get(), static_cast<unsigned long long>(m_stStat.u64RejectedArg));
        }
        rspFrame.reset();
        return false;
    }

    CFeatureFrame_S &stFrame = m_apSlot[nIdx]->stFrame;
    stFrame.u32Seq = ++m_u32SeqGen;
    if (stFrame.llMonotonicMs <= 0)
    {
        /* 感知线程通常会在拿到像素的那一刻打时间戳（更接近真实时延），
           没打则在这里补，保证下游拿到的 llMonotonicMs 恒 >0 */
        stFrame.llMonotonicMs = classroomNowMs();
    }

    m_nLatest = nIdx;
    ++m_stStat.u64Published;
    if (stFrame.bPersonTruncated)
    {
        ++m_stStat.u64TruncatedFrames;
    }
    if (static_cast<uint32_t>(stFrame.nPersonNum) > m_stStat.u32PersonPeak)
    {
        m_stStat.u32PersonPeak = static_cast<uint32_t>(stFrame.nPersonNum);
    }
    updateBusyLocked();

    rspFrame.reset();
    return true;
}

ConstFeatureFramePtr CFeatureFrameBus::latest() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (!m_bReady || m_nLatest < 0 || m_nLatest >= static_cast<int>(m_apSlot.size()))
    {
        return ConstFeatureFramePtr();
    }
    const std::shared_ptr<Slot> &spSlot = m_apSlot[m_nLatest];
    if (!spSlot)
    {
        return ConstFeatureFramePtr();
    }
    return ConstFeatureFramePtr(spSlot, &spSlot->stFrame);
}

int32_t CFeatureFrameBus::capacity() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_nCapacity;
}

CFeatureBusStat_S CFeatureFrameBus::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stStat;
}

void CFeatureFrameBus::resetStat()
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    const uint32_t u32Slots = m_stStat.u32Slots;
    const uint32_t u32Bytes = m_stStat.u32SlotBytes;
    m_stStat = CFeatureBusStat_S();
    m_stStat.u32Slots = u32Slots;
    m_stStat.u32SlotBytes = u32Bytes;
}

int CFeatureFrameBus::findFreeSlotLocked() const
{
    const int nTotal = static_cast<int>(m_apSlot.size());
    if (nTotal <= 0)
    {
        return -1;
    }

    /* 从最新槽之后环形查找。两条判据缺一不可：
       - use_count()>1：仍有消费者在读，覆写它就是数据竞争；
       - idx==m_nLatest：这一槽是「当前对外的那一帧」，即使暂时没人引用也不能写。
         少了后一条，会出现「无消费者的两帧之间被就地改写」的撕裂读，
         而撕裂帧的人数表会让 CDurationGate 把同一个人相邻两帧判成两种状态。 */
    const int nStart = (m_nLatest < 0) ? 0 : (m_nLatest + 1) % nTotal;
    for (int i = 0; i < nTotal; ++i)
    {
        const int nIdx = (nStart + i) % nTotal;
        if (nIdx == m_nLatest)
        {
            continue;
        }
        const std::shared_ptr<Slot> &spSlot = m_apSlot[nIdx];
        if (!spSlot || spSlot.use_count() <= 1)
        {
            return nIdx;
        }
    }
    return -1;
}

void CFeatureFrameBus::updateBusyLocked()
{
    uint32_t nBusy = 0;
    for (const auto &spSlot : m_apSlot)
    {
        if (spSlot && spSlot.use_count() > 1)
        {
            ++nBusy;
        }
    }
    if (nBusy > m_stStat.u32BusyMax)
    {
        m_stStat.u32BusyMax = nBusy;
    }
}
}
