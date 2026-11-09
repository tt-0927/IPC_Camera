/**
 * @FilePath     : infer_scheduler.h
 * @Author       : leiyy
 * @Date         : 2026-10-10 13:52:08
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-10 13:52:08
 * @Description  : 推理调度器
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "classroom_define.h"
#include "model_pool.h"

namespace Classroom
{
/* 队列容量上限 */
#define CLASSROOM_INFER_QUEUE_MAX (16)

/* 优先级档数：0 最高 */
#define CLASSROOM_INFER_PRIORITY_LEVEL (7)

/* 板书 OCR 独占的那一档 */
#define CLASSROOM_INFER_PRIORITY_BOARD_OCR (CLASSROOM_INFER_PRIORITY_LEVEL - 1)

/**
 * @brief 一次推理的结束原因
 */
typedef enum class InferResultReason : int
{
    IR_OK = 0,           /* 正常出结果 */
    IR_STALE_DROPPED,    /* 开工前已过 deadline，或被更高优先级任务挤出队列 */
    IR_MODEL_UNREADY,    /* 模型不在驻留集合内（未加载 / 被时间片换下 / 预算不足） */
    IR_BACKEND_FAIL,     /* 后端 run() 返回 false */
    IR_CANCELLED,        /* 停止时排队任务被清出 */
} InferResultReason_E;

/**
 * @brief 推理结果，仅在回调期间有效
 */
typedef struct _InferResult_
{
    bool bOk = false;
    InferResultReason_E enReason = InferResultReason::IR_BACKEND_FAIL;
    BackendState_E enModelState = BackendState::LOAD_FAILED;    /* IR_MODEL_UNREADY 时的细分原因 */
    uint32_t u32Seq = 0;
    int64_t llCostMs = 0;                                       /* 本次推理耗时（含取租约） */
    const std::vector<float> *pvfOutput = nullptr;              /* 指向任务内缓冲，回调返回后失效 */
} CInferResult_S;

typedef std::function<void(const CInferResult_S &)> InferDoneFn;

/**
 * @brief 一次推理请求
 */
typedef struct _InferReq_
{
    std::string strModelKey;
    std::shared_ptr<std::vector<uint8_t>> spInput;    /* 借自 CRgaPool，worker 用完后随任务析构归还 */
    uint32_t u32InBytes = 0;
    uint32_t u32Seq = 0;
    int64_t llDeadlineMs = 0;                         /* 0 表示不设期限 */
    int32_t nPriority = 1;                            /* 0 最高，取值范围 [0, CLASSROOM_INFER_PRIORITY_LEVEL) */
    InferDoneFn fnDone;
    int64_t llSubmitMs = 0;                           /* 入队时刻，同级内先进先出 */
} CInferReq_S;

/**
 * @brief 调度器观测计数
 */
typedef struct _InferSchedulerStat_
{
    uint64_t u64Submitted = 0;
    uint64_t u64DoneOk = 0;
    uint64_t u64DroppedStale = 0;
    uint64_t u64DroppedUnready = 0;
    uint64_t u64DroppedFull = 0;
    uint64_t u64BackendFail = 0;
    uint32_t u32QueueMax = 0;
    uint32_t u32QueuePeak = 0;
    int64_t llNpuBusyMs = 0;      /* 累计真正跑推理的毫秒数，NPU 预算与自动降采样的依据 */
    /* ---- 流水线观测----
       u64AsyncStarts：两段式提交成功的次数；u64AsyncFallback：asyncCapable() 应答可以
       但 runStart 当场失败退回失败回调的次数。u32OverlapPeak 是「同时在飞」的峰值 */
    uint64_t u64AsyncStarts = 0;
    uint64_t u64AsyncFallback = 0;
    uint32_t u32OverlapPeak = 0;
} CInferSchedulerStat_S;

/**
 * @brief   : 单线程推理调度器
 */
class CInferScheduler
{
public:
    /**
     * @param {CModelPool*} pPool：非拥有指针，调用方须保证池比调度器活得久
     */
    explicit CInferScheduler(CModelPool *pPool);
    ~CInferScheduler();

    CInferScheduler(const CInferScheduler &) = delete;
    CInferScheduler &operator=(const CInferScheduler &) = delete;

    bool start(uint32_t nQueueMax = CLASSROOM_INFER_QUEUE_MAX);

    /**
     * @brief   : 停止并清出排队任务
     */
    void stop();

    /**
     * @brief   : 投递一次推理（非阻塞）
     * @return   {bool} false 表示未入队：参数非法、未运行，或队列已满且新任务不比队内最差者优先级高
     * @note    : 模型键、输入缓冲、回调三者缺一即拒绝（返回 false），这类调用方缺陷不在 worker 里兜
     */
    bool submit(CInferReq_S &&stReq);

    CInferSchedulerStat_S stat() const;
    uint32_t pending() const;

    /**
     * @brief   : 某模型近 N 次的平均推理耗时（EWMA）
     * @return   {double} 无样本返回 0
     */
    double avgCostMs(const std::string &strKey) const;

    /**
     * @brief   : 距上次 resetNpuBudget() 以来 NPU 的时间占用比
     * @return   {double} 0~100
     */
    double npuBusyPercent() const;
    void resetNpuBudget();

    /**
     * @brief   : worker 当前正在执行的那一次
     * @param    {std::string&} rstrKey：OUT=模型键，空闲时清空
     * @return   {int64_t} 已执行毫秒；-1 = 此刻没在执行
     */
    int64_t llCurrentExecMs(std::string &rstrKey) const;

    /** @brief 队首已等待的毫秒 */
    int64_t llQueueWaitMs() const;

private:
    void workerLoop();
    /* 有序插入（优先级升序、同级按入队时刻）；满了按「丢队尾最差者」腾地方，
       被丢者经 pstEvicted 带回，由 submit() 在锁外回调 */
    bool insertLocked(CInferReq_S &&stReq, CInferReq_S *pstEvicted);
    void finishTask(const CInferReq_S &stReq, const CInferResult_S &stResult);
    /* 记账只在锁内做（统计/忙比/EWMA/在途标记），回调永远在锁外 —— 同一把锁不可递归 */
    void creditTaskLocked(InferResultReason_E enReason, const std::string &strKey, int64_t llCostMs);
    /* 在途标记 = 最老在飞的那一帧（含刚收掉一帧后把标记让给第二帧的情况）；
       没有在飞帧就清空。读侧是 llCurrentExecMs() 与 npuBusyPercent() 的在途段 */
    void refreshExecMarkerLocked(int64_t llOldestBeginMs, const std::string &strOldestKey);

    /* 在飞槽（P0-3 流水线）。类型放头文件只为让 startClaimed/collectSlot 拿得到形状，
       实例本身住在 workerLoop 栈上——只有 worker 线程碰它们，不进任何临界区 */
    struct SInflightSlot
    {
        bool bUsed = false;
        CInferReq_S stTask;
        CModelLease stLease;
        int64_t llBeginUs = 0;
        std::vector<float> vfOutput;    /* 槽私有：两帧的输出互不覆写，clear 保留容量 */
    };
    /* 锁外开工一条已出队任务：过期即丢 / 未就绪即回 / 同步回退就地跑完 /
       两段式提交进空槽。回调（若本路径直接出结果）在锁外发生 */
    void startClaimed(CInferReq_S &&stTask, SInflightSlot *astSlot);
    /* 等最老在飞帧的结果、记账、清槽、锁外回调 */
    void collectSlot(SInflightSlot *astSlot, SInflightSlot &stSlot);

private:
    CModelPool *m_pPool;

    mutable std::mutex m_mtx;
    std::condition_variable m_cv;
    std::vector<CInferReq_S> m_vecQueue;         /* 有界，容量 = m_u32QueueMax */
    uint32_t m_u32QueueMax = CLASSROOM_INFER_QUEUE_MAX;

    std::thread m_thWorker;
    bool m_bRunning = false;

    std::map<std::string, double> m_mapAvgCostMs;    /* 条目数受 CLASSROOM_MAX_MODELS 约束 */
    int64_t m_llBudgetBeginMs = 0;
    int64_t m_llNpuBusyMs = 0;
    int64_t m_llExecBeginMs = 0;
    std::string m_strExecKey;

    CInferSchedulerStat_S m_stStat;

    /* 同步回退路径的输出缓冲（worker 线程私有，clear 保留容量） */
    std::vector<float> m_vfWorkerOutput;
};
}
