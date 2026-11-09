/**
 * @FilePath     : model_pool.h
 * @Author       : leiyy
 * @Date         : 2026-09-29 16:28:37
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-09-29 16:28:37
 * @Description  : 模型池
 */

#pragma once

#include <atomic>
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
#include "classroom_mem.h"

namespace Classroom
{
/* 课堂侧可能注册的模型总数上限 */
#define CLASSROOM_MAX_MODELS (24)

/* 推理后端名 */
#define CLASSROOM_BACKEND_YOLOV5 ("yolov5")
#define CLASSROOM_BACKEND_YOLOV8 ("yolov8")
#define CLASSROOM_BACKEND_YOLOV8_POINT ("yolov8_point")
#define CLASSROOM_BACKEND_RAW          ("raw")

/* 驻留量未实测的模型按此值保守记账*/
#define CLASSROOM_MODEL_ASSUME_RESIDENT_KB (134 * 1024)

/**
 * @brief 模型静态描述：注册期给出，运行期只读
 */
typedef struct _ModelDesc_
{
    std::string  strKey;              /* 全进程唯一键，如 "FastPose" */
    std::string  strJsonPath;         /* aiConfig*.json 绝对路径 */
    std::string  strModelPath;        /* .rknn 绝对路径，加载前 access() 预检 */
    ModelGroup_E enGroup = ModelGroup_E::GROUP_OTHER;

    int64_t      llResidentKb = 0;
    uint32_t     u32InputW = 0;       /* 模型输入宽，供裁剪/归一化前置使用 */
    uint32_t     u32InputH = 0;
    bool         bFloatInput = false; /* FastPose 为 fp16 输入，需归一化；uint8 输入可直喂 RGA */
    /* 推理后端选择 */
    std::string  strBackend = CLASSROOM_BACKEND_RAW;

    bool sameAs(const _ModelDesc_ &stOther) const
    {
        return strKey == stOther.strKey &&
               strJsonPath == stOther.strJsonPath &&
               strModelPath == stOther.strModelPath &&
               enGroup == stOther.enGroup &&
               llResidentKb == stOther.llResidentKb &&
               u32InputW == stOther.u32InputW &&
               u32InputH == stOther.u32InputH &&
               bFloatInput == stOther.bFloatInput &&
               strBackend == stOther.strBackend;
    }
} CModelDesc_S;

/**
 * @brief 推理后端抽象
 * @note  run() 只允许在 CInferScheduler 的 NPU worker 线程上调用，因此本接口内部不需要任何锁
 */
class IModelHandle
{
public:
    IModelHandle() = default;
    virtual ~IModelHandle();

    IModelHandle(const IModelHandle &) = delete;
    IModelHandle &operator=(const IModelHandle &) = delete;

    /**
     * @brief   : 单次同步推理
     * @param    {const uint8_t*} pucInput：按模型 pre_process 约定排好的连续输入张量
     * @param    {size_t} nInBytes：输入字节数，实现必须校验与自身期望一致
     * @param    {std::vector<float>&} rvfOutput：打平输出（由调用方复用，避免每帧重新分配）
     * @return   {bool} false 表示本帧无结果，上层按「证据不足」处理，不得当成 0 目标
     */
    virtual bool run(const uint8_t *pucInput, size_t nInBytes, std::vector<float> &rvfOutput) = 0;

    /* ---- 两段式推理 ----
       1) 一个上下文同时只许一帧在飞 ⇒ 同模型键的任务必须串行（调度器按 key 挡）；
       2) runStart/runWait 必须成对，且只许在 CInferScheduler 的 worker 线程上调用。
       默认实现 = 不支持：调度器对这类后端原样走同步 run()，行为与流水线之前完全一致 */
    virtual bool asyncCapable() const { return false; }

    /**
     * @brief   : 两段式第一段：校验并提交输入，让 NPU 开跑，不等待结果
     * @return   {bool} false = 校验失败或提交被拒
     */
    virtual bool runStart(const uint8_t *pucInput, size_t nInBytes)
    {
        (void)pucInput;
        (void)nInBytes;
        return false;
    }

    /**
     * @brief   : 两段式第二段：等上一帧跑完并取回输出（与 run() 同一套拷贝/释放契约）
     * @return   {bool} false = 没有待收的帧或取回失败
     */
    virtual bool runWait(std::vector<float> &rvfOutput)
    {
        (void)rvfOutput;
        return false;
    }

    /* 真实驻留（KB），0 表示未知；池子的预算以此为准，不用文件大小 */
    virtual int64_t residentKb() const = 0;

    /**
     * @brief   : 下发本拍的置信度/NMS 门限
     * @note    : 由检测器每拍调用，句柄内部自行判「变了才下发」。默认空实现=该后端不吃这两个值
     *            （分类/骨骼类模型没有框门限），传负数或 >=1 表示「不改，沿用模型 JSON 的值」
     */
    virtual void setThresholds(float fConfidence, float fNms)
    {
        (void)fConfidence;
        (void)fNms;
    }
};

typedef std::shared_ptr<IModelHandle> ModelPtr;

/* 加载函数注入点：失败返回 nullptr */
typedef std::function<ModelPtr(const CModelDesc_S &)> ModelLoaderFn;

/**
 * @brief 模型租约
 */
class CModelLease
{
public:
    CModelLease() = default;
    ~CModelLease();

    CModelLease(CModelLease &&stOther);
    CModelLease &operator=(CModelLease &&stOther);
    CModelLease(const CModelLease &) = delete;
    CModelLease &operator=(const CModelLease &) = delete;

    bool valid() const { return nullptr != m_spHandle; }
    IModelHandle *get() const { return m_spHandle.get(); }
    IModelHandle *operator->() const { return m_spHandle.get(); }
    const ModelPtr &shared() const { return m_spHandle; }

private:
    friend class CModelPool;
    /* 控制块由池与租约共同持有：租约先亡只减计数，池卸载只放句柄，两者互不悬空 */
    struct Ctrl
    {
        std::atomic<int32_t> nUsers{0};
        std::atomic<int64_t> llLastUsedMs{0};
    };
    CModelLease(ModelPtr spHandle, std::shared_ptr<Ctrl> spCtrl);

private:
    ModelPtr m_spHandle;
    std::shared_ptr<Ctrl> m_spCtrl;
};

/**
 * @brief 单个模型的运行期可见状态（供健康度/降级决策与平台侧观测）
 */
typedef struct _ModelState_
{
    std::string    strKey;
    ModelGroup_E   enGroup = ModelGroup_E::GROUP_OTHER;
    BackendState_E enState = BackendState::DISABLED;
    bool           bResident = false;
    int32_t        nUsers = 0;
    int64_t        llResidentKb = 0;
} CModelState_S;

/**
 * @brief 池累计计数（metrics 出口，无锁快照）
 */
typedef struct _ModelPoolStat_
{
    uint64_t u64LoadOk = 0;
    uint64_t u64LoadFail = 0;
    uint64_t u64LoadFailCooling = 0;
    /* 加载完成时发现描述已被热更替换、于是丢弃这颗句柄的次数 */
    uint64_t u64LoadDiscard = 0;
    uint64_t u64Unload = 0;
    uint64_t u64RejectBudget = 0;
    /* 让路次数：内存是够的，只是对方组还在时间片里。与上一条分开数——
       混在一起时"预算在拒绝"和"轮转在自我确认"看不出区别，轮转退不出去就没人能发现 */
    uint64_t u64RejectSlice = 0;
    uint64_t u64RejectMissing = 0;
    uint64_t u64GroupSwitch = 0;
    int64_t  llResidentKb = 0;      /* 当前驻留总量：句柄报真实值，未报的按 Desc 记账 */
    int32_t  nResidentNum = 0;
    int64_t  llLastLoadMs = 0;      /* 最近一次加载耗时*/
} CModelPoolStat_S;

/**
 * @brief   : 模型驻留参数
 * @note    : 时间片取
 */
typedef struct _ModelPoolParam_
{
    int64_t llIdleUnloadMs = static_cast<int64_t>(CLASSROOM_MODEL_IDLE_UNLOAD_MS);
    /* 必须给其它模块留的整机余量。默认值不能是 0：0 等于把内存闸门整个关掉，
       而"设备上没有配置文件、按内置默认起线"恰恰是现场最常见形态，默认就该校验。
    */
    int64_t llReserveKb = 61440;
    /* 加载失败冷却窗（毫秒）*/
    int64_t llLoadFailCooldownMs = static_cast<int64_t>(CLASSROOM_MODEL_LOAD_FAIL_COOLDOWN_MS);
    int64_t llSlicePoseMs = 20000;  /* G_POSE 单次驻留时间片 */
    int64_t llSliceFaceMs = 8000;   /* G_FACE 单次驻留时间片 */
    bool    bEvictThread = true;    /* 关掉则由外部周期调 evictIdle() */
} CModelPoolParam_S;

/**
 * @brief 模型池
 * @note  线程安全：所有公开方法可并发。加载耗时（百 ms 级）一律在业务锁外进行
 */
class CModelPool
{
public:
    CModelPool() = default;
    ~CModelPool();

    CModelPool(const CModelPool &) = delete;
    CModelPool &operator=(const CModelPool &) = delete;

    void setLoader(ModelLoaderFn fnLoader);
    void setParam(const CModelPoolParam_S &stParam);
    CModelPoolParam_S param() const;

    /**
     * @brief   : 注册模型描述（不加载）
     * @return   {bool} 键重复、参数非法或超出 CLASSROOM_MAX_MODELS 返回 false
     */
    bool registerModel(const CModelDesc_S &stDesc);

    /**
     * @brief   : 启动卸载巡检线程（可选，参数 bEvictThread 控制）
     */
    bool start();
    void stop();

    /**
     * @brief   : 取模型，必要时触发加载
     * @param    {const std::string&} strKey：模型键
     * @param    {BackendState_E&} renState：OUT=结果状态，仅 READY 才可推理
     * @param    {int64_t} llDeadlineMs：愿意等到的最晚时刻（classroomNowMs 口径的绝对值）。
     *            正在加载时最多等到该时刻；传 0 表示完全不等待（推理热线程用这个）
     * @return   {CModelLease} 失败时 valid()==false，且 renState 说明原因：
     *            DISABLED 未注册 / MODEL_MISSING 模型文件不在 / OUT_OF_BUDGET 整机内存不够
     *            GROUP_BUSY 内存够，是对方组的时间片还没用完（让路，不是缺预算）
     *            LOAD_FAILED 加载失败或没等到
     *            （stateOf()/snapshot() 还可能给 UNLOADED = 已注册但此刻没驻留；acquire 不会返回它，
     *              走到这里就一定尝试过加载）
     * @note    : 拿不到租约时上层必须输出 unknown，不得按「无目标」统计
     */
    CModelLease acquire(const std::string &strKey, BackendState_E &renState, int64_t llDeadlineMs);

    /* 不加载地查询状态，供「功能开了但模型没到」的上报路径使用 */
    BackendState_E stateOf(const std::string &strKey) const;
    bool descOf(const std::string &strKey, CModelDesc_S &stDesc) const;

    /**
     * @brief   : 不加载、不计租约地碰一下已驻留的句柄
     * @return   {bool} false 表示未驻留（未加载 / 被时间片换下），此时 fnHandle 一次都没被调用
     */
    bool peek(const std::string &strKey, const std::function<void(IModelHandle &)> &fnHandle);

    /* 主动卸载一组（功能关闭时立即回收，不等空闲超时） */
    int32_t unloadGroup(ModelGroup_E enGroup);
    /**
     * @brief   : 主动卸载一颗（同一组里仍要留着别的颗时用）
     * @return   {bool} true = 卸掉了；false = 没这颗、或它没驻留、或 nUsers>0
     */
    bool unloadKey(const std::string &strKey);
    /* 全部卸载（进程退出/降级） */
    void unloadAll();

    /* 巡检一次空闲模型并卸载；外部定时器也可自行调用 */
    int32_t evictIdle();

    std::vector<CModelState_S> snapshot() const;
    CModelPoolStat_S stat() const;

    /**
     * @brief   : 纯预算判定
     * @param    {CMemInfo_S} stMem：当前内存采样
     * @param    {int64_t} llResidentKb：池内已驻留总量
     * @param    {const CModelDesc_S&} stDesc：拟新增模型
     * @param    {int64_t} llReserveKb：整机底线余量
     * @return   {bool} 允许加载返回 true
     */
    static bool canAdmit(const CMemInfo_S &stMem, int64_t llResidentKb,
                         const CModelDesc_S &stDesc, int64_t llReserveKb);

private:
    struct Entry
    {
        CModelDesc_S stDesc;
        ModelPtr     spHandle;                              /* 强引用 = 驻留 */
        std::shared_ptr<CModelLease::Ctrl> spCtrl;          /* 与租约共享的用户计数 */
        BackendState_E enState = BackendState::UNLOADED;
        BackendState_E enLoggedState = BackendState::DISABLED;
        bool bLoading = false;
        int64_t llLastLoadFailMs = 0;                       /* 上次加载失败时刻：冷却窗的起点，成功时清零 */
        int64_t llSliceUntilMs = 0;                         /* 重资源组的时间片到期时刻 */
        int64_t llChargedKb = 0;                            /* 本次驻留实际计入 m_llResidentKb 的量 */
    };

    /* 重资源组轮转仲裁：需要驻留 enGroup 时，把对方组的时间片到期的驻留让出来 */
    bool makeRoomLocked(const Entry &stWant, int64_t llNowMs);
    /* 卸掉一个驻留并退回它当初实际占用的额度：三条卸载路径共用，避免退错口径 */
    void releaseLocked(Entry &stEntry);
    /* 拒绝类日志的去重：状态与上次已落日志的相同时返回 false（调用方刚把 enState 赋好，
       传进来比对即可，不必另存旧值——多一处状态就多一处会忘）。在 m_mtx 内调用 */
    static bool logIfChanged(Entry &stEntry, BackendState_E enNew);
    void evictLoop();
    /* 真正调用 loader，必须在业务锁外 */
    ModelPtr loadViaLoader(const Entry &stEntry, int64_t &llCostMs);

private:
    mutable std::mutex m_mtx;
    std::condition_variable m_cvLoaded;
    std::mutex m_mtxLoad;                 /* 串行化加载：同时只有一个模型在 load */
    std::map<std::string, Entry> m_mapEntries;

    ModelLoaderFn m_fnLoader;
    CModelPoolParam_S m_stParam;
    CModelPoolStat_S m_stStat;
    int64_t m_llResidentKb = 0;

    std::thread m_thEvict;
    bool m_bRunning = false;
};
}
