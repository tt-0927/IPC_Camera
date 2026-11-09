/**
 * @FilePath     : infer_scheduler.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-10 13:52:08
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-10 13:52:08
 * @Description  : 单线程推理调度器实现
 */

#include "infer_scheduler.h"

#include <pthread.h>

#include "classroom_clock.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* EWMA 系数：新样本权重。0.2 ≈ 平滑约 5 次，够快跟上一节课的节奏变化又不被单次抖动带跑 */
const double INFER_COST_EWMA_ALPHA = 0.2;

/* 在飞槽位数 */
constexpr int32_t N_INFLIGHT_SLOTS = 2;
}

CInferScheduler::CInferScheduler(CModelPool *pPool)
    : m_pPool(pPool)
{
}

CInferScheduler::~CInferScheduler()
{
    stop();
}

bool CInferScheduler::start(uint32_t nQueueMax)
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    if (m_bRunning)
    {
        return true;
    }
    if (nullptr == m_pPool)
    {
        dlog_error("infer_scheduler: 未注入模型池，无法启动");
        return false;
    }
    if (0 == nQueueMax)
    {
        nQueueMax = CLASSROOM_INFER_QUEUE_MAX;
    }
    if (nQueueMax > CLASSROOM_INFER_QUEUE_MAX)
    {
        /* 队列加深只会增加延迟与在借缓冲数，容量上限是设计值，不由配置放大 */
        dlog_warn("infer_scheduler: 队列深度 %u 超出设计值，收敛为 %u", nQueueMax, CLASSROOM_INFER_QUEUE_MAX);
        nQueueMax = CLASSROOM_INFER_QUEUE_MAX;
    }

    m_u32QueueMax = nQueueMax;
    m_vecQueue.clear();
    m_vecQueue.reserve(nQueueMax);
    m_stStat = CInferSchedulerStat_S();
    m_stStat.u32QueueMax = nQueueMax;
    m_llBudgetBeginMs = classroomNowMs();
    m_bRunning = true;
    m_thWorker = std::thread(&CInferScheduler::workerLoop, this);
    dlog_info("infer_scheduler 已启动，队列深度上限 %u", nQueueMax);
    return true;
}

void CInferScheduler::stop()
{
    std::vector<CInferReq_S> vecLeft;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (!m_bRunning)
        {
            return;
        }
        /* 清队与关闸同一把锁 */
        m_bRunning = false;
        vecLeft.swap(m_vecQueue);
    }
    m_cv.notify_all();

    /* 回调里再调 stop() 会自 join 死锁，这里挡一层 */
    if (m_thWorker.joinable() && m_thWorker.get_id() != std::this_thread::get_id())
    {
        m_thWorker.join();
    }
    for (auto &stReq : vecLeft)
    {
        CInferResult_S stResult;
        stResult.bOk = false;
        stResult.enReason = InferResultReason::IR_CANCELLED;
        stResult.u32Seq = stReq.u32Seq;
        finishTask(stReq, stResult);    /* 让业务把持有的帧引用和输入缓冲放掉 */
    }
    if (!vecLeft.empty())
    {
        dlog_info("infer_scheduler 已停止，%zu 个排队任务被取消", vecLeft.size());
    }
}

bool CInferScheduler::submit(CInferReq_S &&stReq)
{
    CInferReq_S stEvicted;
    std::vector<CInferReq_S> vecStale;    /* 在队过期的任务，锁外回调 */

    if (stReq.strModelKey.empty() || nullptr == stReq.spInput || 0 == stReq.u32InBytes ||
        !stReq.fnDone || stReq.nPriority < 0 || stReq.nPriority >= CLASSROOM_INFER_PRIORITY_LEVEL)
    {
        dlog_warn("infer_scheduler: 请求非法，key=%s input=%p bytes=%u cb=%d prio=%d",
                  stReq.strModelKey.c_str(), static_cast<const void *>(stReq.spInput.get()),
                  stReq.u32InBytes, stReq.fnDone ? 1 : 0, stReq.nPriority);
        return false;
    }

    bool bInserted = false;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (!m_bRunning)
        {
            return false;
        }
        if (0 == stReq.llSubmitMs)
        {
            stReq.llSubmitMs = classroomNowMs();
        }

        /* 在队任务持有的输入缓冲（借自 CRgaPool，每模型键只有 2 块）不能被队列无限期扣押 */
        if (!m_vecQueue.empty())
        {
            const int64_t llNow = classroomNowMs();
            for (auto it = m_vecQueue.begin(); it != m_vecQueue.end(); )
            {
                if (it->llDeadlineMs > 0 && llNow > it->llDeadlineMs)
                {
                    vecStale.push_back(std::move(*it));
                    it = m_vecQueue.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            if (!vecStale.empty())
            {
                m_stStat.u64DroppedStale += vecStale.size();
            }
        }

        bInserted = insertLocked(std::move(stReq), &stEvicted);
        if (bInserted)
        {
            ++m_stStat.u64Submitted;
            m_cv.notify_all();
        }
    }

    /* 被挤掉与在队过期的任务都在锁外回调：回调里业务可能回头调 submit()/acquire()，
       不能持锁调用 */
    for (auto &stStale : vecStale)
    {
        CInferResult_S stResult;
        stResult.bOk = false;
        stResult.enReason = InferResultReason::IR_STALE_DROPPED;
        stResult.u32Seq = stStale.u32Seq;
        finishTask(stStale, stResult);
    }
    if (stEvicted.fnDone)
    {
        CInferResult_S stResult;
        stResult.bOk = false;
        stResult.enReason = InferResultReason::IR_STALE_DROPPED;
        stResult.u32Seq = stEvicted.u32Seq;
        finishTask(stEvicted, stResult);
    }
    return bInserted;
}

CInferSchedulerStat_S CInferScheduler::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stStat;
}

uint32_t CInferScheduler::pending() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return static_cast<uint32_t>(m_vecQueue.size());
}

double CInferScheduler::avgCostMs(const std::string &strKey) const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    auto it = m_mapAvgCostMs.find(strKey);
    return (it == m_mapAvgCostMs.end()) ? 0.0 : it->second;
}

double CInferScheduler::npuBusyPercent() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);

    const int64_t llNowMs = classroomNowMs();
    const int64_t llElapsed = llNowMs - m_llBudgetBeginMs;
    if (llElapsed <= 0)
    {
        return 0.0;
    }
    /* 在途执行按"落在本窗口内的部分"计入：窗口起点之后才开跑的那次，只摊它超出起点的时长 */
    int64_t llInflightMs = 0;
    if (!m_strExecKey.empty() && m_llExecBeginMs > 0)
    {
        const int64_t llWinBegin = m_llBudgetBeginMs > m_llExecBeginMs ? m_llBudgetBeginMs : m_llExecBeginMs;
        llInflightMs = llNowMs - llWinBegin;
        if (llInflightMs < 0)
        {
            llInflightMs = 0;
        }
    }
    const double dRatio = static_cast<double>(m_llNpuBusyMs + llInflightMs) * 100.0
                          / static_cast<double>(llElapsed);
    return (dRatio > 100.0) ? 100.0 : dRatio;
}

void CInferScheduler::resetNpuBudget()
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_llNpuBusyMs = 0;
    m_llBudgetBeginMs = classroomNowMs();
}

int64_t CInferScheduler::llCurrentExecMs(std::string &rstrKey) const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    if (m_strExecKey.empty() || m_llExecBeginMs <= 0)
    {
        rstrKey.clear();
        return -1;    /* 此刻没在执行——与 0（刚开跑）分开 */
    }
    rstrKey = m_strExecKey;
    const int64_t llMs = classroomNowMs() - m_llExecBeginMs;
    return (llMs < 0) ? 0 : llMs;
}

int64_t CInferScheduler::llQueueWaitMs() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    if (m_vecQueue.empty())
    {
        return 0;
    }
    /* 队列按 (优先级, 入队时刻) 有序，队首不是"最老"而是"最该跑"——这里要的是等待最长那条，
       容量 ≤ CLASSROOM_INFER_QUEUE_MAX，线性扫一遍的成本可以忽略 */
    int64_t llOldestSubmitMs = 0;
    for (const auto &stItem : m_vecQueue)
    {
        if (stItem.llSubmitMs > 0 && (0 == llOldestSubmitMs || stItem.llSubmitMs < llOldestSubmitMs))
        {
            llOldestSubmitMs = stItem.llSubmitMs;
        }
    }
    if (0 == llOldestSubmitMs)
    {
        return 0;
    }
    const int64_t llWait = classroomNowMs() - llOldestSubmitMs;
    return (llWait < 0) ? 0 : llWait;
}

void CInferScheduler::creditTaskLocked(InferResultReason_E enReason, const std::string &strKey,
                                       int64_t llCostMs)
{
    switch (enReason)
    {
        case InferResultReason::IR_OK:
            ++m_stStat.u64DoneOk;
            break;
        case InferResultReason::IR_STALE_DROPPED:
            ++m_stStat.u64DroppedStale;
            break;
        case InferResultReason::IR_MODEL_UNREADY:
            ++m_stStat.u64DroppedUnready;
            break;
        default:
            ++m_stStat.u64BackendFail;
            break;
    }

    /* 只有真跑过的任务才有忙时可记、有耗时可入 EWMA：
       stale/unready 的 llCostMs 是 0，key 也可能是空串 */
    if (llCostMs <= 0 || strKey.empty())
    {
        return;
    }
    m_llNpuBusyMs += llCostMs;
    m_stStat.llNpuBusyMs = m_llNpuBusyMs;

    /* 模型数受 CLASSROOM_MAX_MODELS 约束，这里只防住「未知键无限进表」这一种失控 */
    auto itCost = m_mapAvgCostMs.find(strKey);
    if (itCost == m_mapAvgCostMs.end())
    {
        if (m_mapAvgCostMs.size() >= CLASSROOM_MAX_MODELS)
        {
            dlog_warn("infer_scheduler: 耗时统计表已满，不再跟踪 %s", strKey.c_str());
        }
        else
        {
            m_mapAvgCostMs[strKey] = static_cast<double>(llCostMs);
        }
    }
    else
    {
        itCost->second = itCost->second * (1.0 - INFER_COST_EWMA_ALPHA) +
                         static_cast<double>(llCostMs) * INFER_COST_EWMA_ALPHA;
    }
}

void CInferScheduler::refreshExecMarkerLocked(int64_t llOldestBeginMs, const std::string &strOldestKey)
{
    if (strOldestKey.empty())
    {
        /* 读侧见空串即 -1：不留一根过期的柱子 */
        m_llExecBeginMs = 0;
        m_strExecKey.clear();
        return;
    }
    m_llExecBeginMs = llOldestBeginMs;
    m_strExecKey = strOldestKey;
}

void CInferScheduler::startClaimed(CInferReq_S &&stTask, SInflightSlot *astSlot)
{
    const int64_t llNow = classroomNowMs();
    CInferResult_S stResult;
    stResult.u32Seq = stTask.u32Seq;

    if (stTask.llDeadlineMs > 0 && llNow > stTask.llDeadlineMs)
    {
        {
            std::lock_guard<std::mutex> stLock(m_mtx);
            creditTaskLocked(InferResultReason::IR_STALE_DROPPED, std::string(), 0);
        }
        stResult.enReason = InferResultReason::IR_STALE_DROPPED;
        finishTask(stTask, stResult);
        return;
    }

    /* 不等待加载：拿不到就立刻回报未就绪，本帧由上层出 unknown */
    BackendState_E enState = BackendState::LOAD_FAILED;
    CModelLease stLease = m_pPool->acquire(stTask.strModelKey, enState, 0);
    if (!stLease.valid())
    {
        {
            std::lock_guard<std::mutex> stLock(m_mtx);
            creditTaskLocked(InferResultReason::IR_MODEL_UNREADY, std::string(), 0);
        }
        stResult.enReason = InferResultReason::IR_MODEL_UNREADY;
        stResult.enModelState = enState;
        finishTask(stTask, stResult);
        return;
    }

    IModelHandle *pHandle = stLease.get();

    const bool bBecameOldest = !astSlot[0].bUsed && !astSlot[1].bUsed;
    if (bBecameOldest)
    {
        std::lock_guard<std::mutex> stLockExec(m_mtx);
        m_llExecBeginMs = classroomNowMs();
        m_strExecKey = stTask.strModelKey;
    }

    /* 同步回退：后端没有两段式（共享仓那路），原样走阻塞 run()——
       占槽、收帧这些流水线机制都不为它们付代价，行为与流水线之前一致 */
    if (!pHandle->asyncCapable())
    {
        const uint64_t llBeginUs = classroomNowUs();
        m_vfWorkerOutput.clear();
        const bool bRunOk = stLease->run(stTask.spInput->data(), stTask.u32InBytes, m_vfWorkerOutput);
        const int64_t llCostMs = (static_cast<int64_t>(classroomNowUs() - llBeginUs) + 500) / 1000;

        stResult.bOk = bRunOk;
        stResult.llCostMs = llCostMs;
        stResult.enReason = bRunOk ? InferResultReason::IR_OK : InferResultReason::IR_BACKEND_FAIL;
        stResult.enModelState = BackendState::READY;
        stResult.pvfOutput = &m_vfWorkerOutput;

        {
            std::lock_guard<std::mutex> stLock(m_mtx);
            creditTaskLocked(stResult.enReason, stTask.strModelKey, llCostMs);
            /* 在途标记：有别的帧在飞就归它（更老），否则这条刚跑完的已经出结果，清空 */
            if (!astSlot[0].bUsed && !astSlot[1].bUsed)
            {
                refreshExecMarkerLocked(0, std::string());
            }
            else if (astSlot[0].bUsed)
            {
                refreshExecMarkerLocked((astSlot[0].llBeginUs + 500) / 1000,
                                        astSlot[0].stTask.strModelKey);
            }
            else
            {
                refreshExecMarkerLocked((astSlot[1].llBeginUs + 500) / 1000,
                                        astSlot[1].stTask.strModelKey);
            }
        }
        finishTask(stTask, stResult);
        return;
    }

    /* 两段式提交。先看有没有空槽 */
    SInflightSlot *pstFree = !astSlot[0].bUsed ? &astSlot[0]
                             : (!astSlot[1].bUsed ? &astSlot[1] : nullptr);
    if (nullptr == pstFree)
    {
        dlog_error("infer_scheduler: 异步启动却无空闲槽（bug），key=%s",
                   stTask.strModelKey.c_str());
        {
            std::lock_guard<std::mutex> stLock(m_mtx);
            creditTaskLocked(InferResultReason::IR_BACKEND_FAIL, std::string(), 0);
        }
        stResult.enReason = InferResultReason::IR_BACKEND_FAIL;
        finishTask(stTask, stResult);
        return;
    }

    const uint64_t llBeginUs = classroomNowUs();
    if (!pHandle->runStart(stTask.spInput->data(), stTask.u32InBytes))
    {
        /* 后端声称可以两段式却当场提交失败：本帧无结果。没占槽，标记归位给剩余在飞 */
        {
            std::lock_guard<std::mutex> stLock(m_mtx);
            ++m_stStat.u64AsyncFallback;
            creditTaskLocked(InferResultReason::IR_BACKEND_FAIL, std::string(), 0);
            if (!astSlot[0].bUsed && !astSlot[1].bUsed)
            {
                refreshExecMarkerLocked(0, std::string());
            }
        }
        stResult.enReason = InferResultReason::IR_BACKEND_FAIL;
        finishTask(stTask, stResult);
        return;
    }

    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        ++m_stStat.u64AsyncStarts;
        const uint32_t u32Inflight = 1u + (astSlot[0].bUsed ? 1u : 0u) + (astSlot[1].bUsed ? 1u : 0u);
        if (u32Inflight > m_stStat.u32OverlapPeak)
        {
            m_stStat.u32OverlapPeak = u32Inflight;
        }
    }

    pstFree->bUsed = true;
    pstFree->stTask = std::move(stTask);
    pstFree->stLease = std::move(stLease);
    pstFree->llBeginUs = llBeginUs;
}

void CInferScheduler::collectSlot(SInflightSlot *astSlot, SInflightSlot &stSlot)
{
    /* 先收结果再动槽：runWait 会阻塞到这一帧真跑完（设备上是 rknn_wait） */
    const bool bWaitOk = stSlot.stLease->runWait(stSlot.vfOutput);
    const int64_t llCostMs =
        (static_cast<int64_t>(classroomNowUs() - stSlot.llBeginUs) + 500) / 1000;

    CInferResult_S stResult;
    stResult.u32Seq = stSlot.stTask.u32Seq;
    stResult.bOk = bWaitOk;
    stResult.llCostMs = llCostMs;
    stResult.enReason = bWaitOk ? InferResultReason::IR_OK : InferResultReason::IR_BACKEND_FAIL;
    stResult.enModelState = BackendState::READY;
    stResult.pvfOutput = &stSlot.vfOutput;

    /* 取走任务与租约（租约在 worker 线程析构归还计数），槽位立刻让给下一帧。
       vfOutput 刻意留在槽里：clear 保留容量，下一次零分配 */
    CInferReq_S stTask = std::move(stSlot.stTask);
    CModelLease stLease = std::move(stSlot.stLease);
    stSlot.bUsed = false;
    stSlot.llBeginUs = 0;

    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        creditTaskLocked(stResult.enReason, stTask.strModelKey, llCostMs);
        /* 在途标记让给剩下的那帧（它才是现在最老的）；没有就清空 */
        if (astSlot[0].bUsed)
        {
            refreshExecMarkerLocked((astSlot[0].llBeginUs + 500) / 1000,
                                    astSlot[0].stTask.strModelKey);
        }
        else if (astSlot[1].bUsed)
        {
            refreshExecMarkerLocked((astSlot[1].llBeginUs + 500) / 1000,
                                    astSlot[1].stTask.strModelKey);
        }
        else
        {
            refreshExecMarkerLocked(0, std::string());
        }
    }
    finishTask(stTask, stResult);
}

void CInferScheduler::workerLoop()
{
    pthread_setname_np(pthread_self(), "ClsInfer");

    SInflightSlot astSlot[N_INFLIGHT_SLOTS];

    while (true)
    {
        /* 把能开工的先全部开工（有空槽 + 出队闸 */
        for (;;)
        {
            CInferReq_S stClaimed;
            bool bHasClaim = false;
            {
                std::unique_lock<std::mutex> stLock(m_mtx);
                if (m_bRunning && !m_vecQueue.empty())
                {
                    SInflightSlot *pstFree = !astSlot[0].bUsed ? &astSlot[0]
                                             : (!astSlot[1].bUsed ? &astSlot[1] : nullptr);
                    if (nullptr != pstFree)
                    {
                        /* 队首与在飞同键时**不再整队停摆**——从队首向后找第一条
                           键不与任何在飞帧冲突的任务出队。比它靠前的同键任务此刻反正
                           跑不了（一个上下文只许一帧在飞），让它们身后的不同键高优先级
                           任务先走，NPU 不在串行等待里空转。扫描从队首开始，同键的
                           相对先后原样保持（FIFO 不被跨键让路打乱） */
                        for (auto it = m_vecQueue.begin(); it != m_vecQueue.end(); ++it)
                        {
                            const bool bClash =
                                (astSlot[0].bUsed && astSlot[0].stTask.strModelKey == it->strModelKey) ||
                                (astSlot[1].bUsed && astSlot[1].stTask.strModelKey == it->strModelKey);
                            if (!bClash)
                            {
                                stClaimed = std::move(*it);
                                m_vecQueue.erase(it);
                                bHasClaim = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (!bHasClaim)
            {
                break;
            }
            startClaimed(std::move(stClaimed), astSlot);
        }

        /* 有在飞就 FIFO 收最老一帧（阻塞到它真跑完） */
        if (astSlot[0].bUsed)
        {
            collectSlot(astSlot, astSlot[0]);
        }
        else if (astSlot[1].bUsed)
        {
            collectSlot(astSlot, astSlot[1]);
        }
        else
        {
            std::unique_lock<std::mutex> stLock(m_mtx);
            if (m_bRunning && m_vecQueue.empty())
            {
                m_cv.wait(stLock, [this] { return (!m_bRunning) || (!m_vecQueue.empty()); });
            }
            if (!m_bRunning)
            {
                break;    /* 在飞帧已在上面收完；排队任务由 stop() 统一取消 */
            }
        }
    }

    /* 收尾不弃帧 */
    for (int32_t i = 0; i < N_INFLIGHT_SLOTS; ++i)
    {
        if (astSlot[i].bUsed)
        {
            collectSlot(astSlot, astSlot[i]);
        }
    }
}

bool CInferScheduler::insertLocked(CInferReq_S &&stReq, CInferReq_S *pstEvicted)
{
    if (m_vecQueue.size() >= static_cast<size_t>(m_u32QueueMax))
    {
        if (m_vecQueue.empty())
        {
            ++m_stStat.u64DroppedFull;
            return false;
        }

        /* 队列已按 (优先级, 入队时刻) 有序，末尾即「优先级最低且最老」的那个 */
        const CInferReq_S &stWorst = m_vecQueue.back();
        if (stReq.nPriority >= stWorst.nPriority)
        {
            ++m_stStat.u64DroppedFull;
            return false;
        }
        if (nullptr != pstEvicted)
        {
            *pstEvicted = std::move(m_vecQueue.back());
        }
        m_vecQueue.pop_back();
    }

    size_t nPos = m_vecQueue.size();
    for (size_t i = 0; i < m_vecQueue.size(); ++i)
    {
        const CInferReq_S &stItem = m_vecQueue[i];
        if (stItem.nPriority > stReq.nPriority)
        {
            nPos = i;
            break;
        }
    }
    m_vecQueue.insert(m_vecQueue.begin() + nPos, std::move(stReq));

    if (m_vecQueue.size() > m_stStat.u32QueuePeak)
    {
        m_stStat.u32QueuePeak = static_cast<uint32_t>(m_vecQueue.size());
    }
    return true;
}

void CInferScheduler::finishTask(const CInferReq_S &stReq, const CInferResult_S &stResult)
{
    if (!stReq.fnDone)
    {
        return;
    }
    stReq.fnDone(stResult);
}
}
