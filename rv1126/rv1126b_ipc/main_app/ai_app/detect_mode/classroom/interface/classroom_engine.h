/**
 * @FilePath     : classroom_engine.h
 * @Author       : leiyy
 * @Date         : 2026-10-17 22:18:07
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 12:05:00
 * @Description  : 课堂分析引擎
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "attendance.h"
#include "board_ocr.h"
#include "classroom_config.h"
#include "classroom_mem.h"
#include "classroom_metric_log.h"
#include "classroom_warm_policy.h"   /* CWarmNeed_S：暖组判据的入参打包（见 warmNeedOf） */
#include "crop_unit.h"
#include "data_center.h"
#include "event_merger.h"
#include "feature_frame.h"
#include "frame_bus.h"
#include "infer_scheduler.h"
#include "model_pool.h"
#include "perceive_pipeline.h"
#include "report_format.h"
#include "report_spool.h"
#include "rule_engine.h"
#include "stu_track.h" 
#include "seat_prior.h" 
#include "student_behavior.h"
#include "teacher_behavior.h"
#include "tvsdk_stats.h"

namespace Classroom
{
/* 节拍线程唤醒周期 */
#define CLASSROOM_TICK_PERIOD_MS (100)

/* 预热时等模型加载完成的上限 */
#define CLASSROOM_WARM_WAIT_MS (2000)

/* 内存守护采样周期与心跳指标周期 */
#define CLASSROOM_GUARD_PERIOD_MS (5000)
#define CLASSROOM_METRIC_PERIOD_MS (10000)

/* 配置文件轮询周期：一次 stat() 系统调用的代价，换来现场改文件即热生效 */
#define CLASSROOM_CONFIG_WATCH_MS (2000)

/* 送帧入槽的最小间隔 */
#define CLASSROOM_FRAME_MIN_INTERVAL_MS (60)

/**
 * @brief 感知回调注入点
 * @param  {const FramePtr&}        spPixel：本拍要分析的像素帧（引用计数，函数返回即可释放）
 * @param  {const ConfigPtr&}       spConf ：与本次分析同一份的配置快照（一帧之内参数不分叉）
 * @param  {const FeatureFramePtr&} spOut  ：引擎借出的可写特征帧，往里填人员行
 * @return {bool} false 表示本拍无产出，引擎不发布该特征帧
 */
typedef std::function<bool(const FramePtr &, const ConfigPtr &, const FeatureFramePtr &)> CPerceiveFn;

/**
 * @brief   : 感知层读数注入点（与 CPerceiveFn 方向相反的那条线）
 * @return   {std::string} 一行 key=value 文本，由感知层自己格式化
 */
typedef std::function<std::string()> CPerceiveStatFn;

/**
 * @brief   : 取走某个跟踪 ID 手上那份人脸比对结论（考勤段用，方向是「引擎→感知」）
 * @param    {int32_t} nTrackId：跟踪 ID
 * @param    {CFaceMatch_S&} rstOut：OUT=仅返回 true 时有意义
 * @return   {bool} false=这个 ID 此刻没有待取的结论
 */
typedef std::function<bool(int32_t nTrackId, CFaceMatch_S &rstOut)> CFaceMatchTakeFn;

/**
 * @brief 报文出口：拿一份已经序列化好的报文文本，发出去，回答成没成
 * @param  {uint32_t} u32Type   ：SpoolType_E（周期报表 / 课堂收尾 / 事件），跟着报文一起落盘
 * @param  {const std::string&} strPayload：JSON 文本，egress 侧组好的完整报文
 * @return {bool} true=通道已接受（进发送队列即算接受）；false=这次没发出去
 */
typedef std::function<bool(uint32_t u32Type, const std::string &strPayload)> CUploadFn;

/**
 * @brief 即时事件告警通道
 * @param stEvent：要上送的事件
 * @param llWallMs：与报表同一个墙钟锚点的读数，供 llTimestampMs 使用
 * @return true=平台侧接受了这一条；false=没送达（没客户端/服务不可用/推送失败）
 */
typedef std::function<bool(const CBehaviorEvent_S &stEvent, int64_t llWallMs)> CEventAlarmFn;

/* 引擎持有的租约表：一个名字，两处用（成员声明与 releaseModel 的迭代器签名）。
   重抄一遍 std::map<std::string, CModelLease> 的写法迟早会只改一处 */
typedef std::map<std::string, CModelLease> ModelLeaseMap;

/**
 * @brief 引擎自身观测
 */
typedef struct _EngineStat_
{
    bool     bRunning = false;
    bool     bGeomStale = false;              /* 几何配置与总线不一致（源尺寸变化被拒 / 热换失败），需重启才生效 */
    bool     bNeedRotate = false;             /* 已进入重资源组时间片轮转（内存实测放不下的证据） */
    /* 起线至今毫秒数。**派生量**：只有 stat() 读数时现算，任何写入路径都不该碰它，
       否则"恒等于 now - boot"这个不变量就被拆成两处维护了 */
    int64_t  llUpMs = 0;
    int64_t  llConfigVersion = 0;
    GuardLevel_E enLevel = GuardLevel_E::GL_NORMAL;
    int32_t  nSampleStretchPercent = 100;     /* 降采样系数，100 表示不拉长 */

    uint64_t u64FramesIn = 0;                 /* 取流线程送进来的帧数 */
    uint64_t u64FramesRejected = 0;           /* 尺寸/长度不符或未就绪被拒（上游配置错，非 0 必须查） */
    uint64_t u64FramesThrottled = 0;          /* 未到最小入槽间隔被节流：设计行为，正常约为 in 的 2/3 */
    uint64_t u64Ticks = 0;
    uint64_t u64PerceiveRounds = 0;           /* 实际跑了的感知拍数 */
    uint64_t u64PerceiveFail = 0;
    /* u64PerceiveFail 里属于「关档边沿」的那部分：本拍开始时基础组已不在板。
       主开关一关，组当场释放，而总线上还压着一帧在途——这一帧必然失败，且只有一帧。
       两数之差才是稳态故障数；只有总数时，一次正常的开关动作会被读成算法在挂 */
    uint64_t u64PerceiveFailEdge = 0;
    /* 引擎从事件环取走条数。与规则引擎自己的 evt=begin/end 对照才看得出「产了没人取」：
       两个数一旦长期分叉，就是某条节拍路径绕过了取数那一步，环每满 64 条丢一条最旧 */
    uint64_t u64RuleEvents = 0;
    uint64_t u64WarmOk = 0;                   /* 预热拿到租约的次数 */
    uint64_t u64WarmFail = 0;                 /* 预热被拒（缺件/预算/时间片），持续增长说明模型没到位 */
    uint64_t u64LevelChange = 0;
    uint64_t u64HeavySwitch = 0;              /* 重资源组切换次数，轮转节奏的看护点 */
    uint32_t u32HeldNum = 0;                  /* 引擎当前持有的模型租约数 */

    /* ---- 出口侧三个数（P3）：必须分开，现场问的是三件不同的事 ---- */
    uint64_t u64UploadOk = 0;
    uint64_t u64UploadFail = 0;
    uint64_t u64SessionClose = 0;
    bool     bUploadWired = false;            /* false=没接通道，此时报表只落日志、刻意不落盘 */

    int64_t  llRssBaseKb = 0;                 /* 起线 RSS，空跑验收看的是相对它的增量 */
    int64_t  llRssMaxKb = 0;
    int64_t  llAvailMinKb = 0;                /* 历史最低水位，比末值更能说明有没有险到过 */
    int64_t  llLastRssKb = 0;
    int64_t  llLastAvailKb = 0;
    int64_t  llLastMmzKb = 0;
    uint32_t u32ThreadNumMax = 0;             /* ≤12 线程看护 */
} CEngineStat_S;

/**
 * @brief   : 课堂分析引擎
 */
class CClassroomEngine : public IPerceiveHost
{
public:
    CClassroomEngine() = default;
    ~CClassroomEngine() override;

    CClassroomEngine(const CClassroomEngine &) = delete;
    CClassroomEngine &operator=(const CClassroomEngine &) = delete;

    /**
     * @brief   : 注入模型加载器
     */
    void setLoader(ModelLoaderFn fnLoader);

    /**
     * @brief   : 注入感知回调
     */
    void setPerceiveFn(CPerceiveFn fnPerceive);

    /**
     * @brief   : 注入感知读数取串函数（不注入则心跳与 statusJson 不带感知段）
     */
    void setPerceiveStatFn(CPerceiveStatFn fnStat);
    /**
     * @brief   : 注入「取走某跟踪 ID 手上那份人脸比对结论」（不注入则考勤只记时长、永不命名）
     */
    void setFaceMatchTakeFn(CFaceMatchTakeFn fnTake);
    /**
     * @brief   : 注入报文出口（不注入则报表只落日志，且**不**落滞留环）
     */
    void setUploadFn(CUploadFn fnUpload);

    /**
     * @brief   : 注入即时事件告警通道
     */
    void setEventAlarmFn(CEventAlarmFn fnEventAlarm);

    /**
     * @brief   : 注入平台统计推送通道
     */
    void setStatsSink(IStatsSink *pStatsSink);

    /**
     * @brief   : 装配并启动
     * @param    {const std::string&} strConfigPath：课堂配置 JSON 绝对路径，空串表示用默认路径
     * @param    {std::string&} rstrErr：OUT=失败原因（写日志与 Web 回显）
     * @return   {bool} 任一步失败即整体失败并已回滚已建资源（绝不半启动：半启动的线程与
     *            半套参数是现场最难复现的一类问题）
     */
    bool init(const std::string &strConfigPath, std::string &rstrErr);
    void deinit();
    bool isRunning() const;

    /**
     * @brief   : 像素帧入口（取流线程调用，非阻塞）
     * @param    {const void*} pNv12：NV12 首地址
     * @param    {int64_t} nLen：可用字节数
     * @param    {int} nW / nH：该帧实际宽高
     * @param    {uint64_t} u64PtsUs：推流 PTS
     * @return   {bool} true 已入总线；false 丢帧（未启用 / 未就绪 / 尺寸与配置不符 / 无空闲槽）
     */
    bool onFrame(const void *pNv12, int64_t nLen, int nW, int nH, uint64_t u64PtsUs);

    /**
     * @brief   : 配置热换（下发线程调用；引擎内部只换快照，应用动作在下一拍完成）
     * @return   {bool} false 时 rstrErr 说明第一条硬错误，旧快照继续生效
     */
    bool reloadConfigFromFile(const std::string &strPath, std::string &rstrErr);
    bool reloadConfigFromText(const std::string &strJson, std::string &rstrErr);

    ConfigPtr config() const;
    CEngineStat_S stat() const;

    /**
     * @brief   : 最近一次成功换入的配置 JSON 原文
     * @return   {std::string} 空串表示"从没有成功换入过"（只用默认值跑）
     */
    std::string configText() const;

    /**
     * @brief   : 当前状态快照（JSON 文本）
     */
    std::string statusJson() const;

    /**
     * @brief   : 课程分析报告页（只读面板）的一份结构化读数
     * @return   {std::string} JSON：session / attendance / latest / behaviors
     */
    std::string reportJson() const;

    /* 以下出口给 P1 起的感知层用，生命周期与本引擎一致。
       前三个同时是 IPerceiveHost 的实现：感知层经接口拿，不认引擎类型 */
    CModelPool       &pool() override      { return m_stPool; }
    CInferScheduler  *scheduler() override { return m_spSched.get(); }
    CFrameBus        &frameBus()   { return m_stBus; }
    CFeatureFrameBus &featureBus() { return m_stFeatureBus; }

    /**
     * @brief   : 取某模型的裁剪单元（随组预热创建、随组卸载释放）
     * @return   {CropUnitPtr} 未创建返回 nullptr；调用方不得跨组切换缓存该指针
     */
    CropUnitPtr cropUnitOf(const std::string &strKey) const override;

    /**
     * @brief   : 底库（IPerceiveHost 的实现）：非拥有，所有权在 m_spGallery
     * @return   {IFaceGallery*} 永不为空——装配失败时也是一具 CNullFaceGallery
     */
    IFaceGallery *faceGallery() override { return m_spGallery.get(); }

    /**
     * @brief   : 人头是否在给定区域组内
     */
    bool headInRegion(const CRectf_S &stHeadBox,
                      const std::vector<int32_t> &vecRegionIds) override
    {
        if (vecRegionIds.empty())
        {
            return true;
        }
        bool bConfigError = false;
        const bool bIn = m_stRule.regions().inAnyRegion(vecRegionIds, 0.5f, stHeadBox, bConfigError);
        return bIn || bConfigError;
    }

    /**
     * @brief   : 待复核名单（IPerceiveHost 的实现）：非拥有，直接转发规则层那份
     */
    const std::vector<int32_t> &phoneRecheckIds() const override { return m_stRule.phoneRecheckIds(); }

    /**
     * @brief   : NPU 等待期的紧急度预计算（IPerceiveHost 挂点，optimization.urgency_precompute）
     */
    void precomputeUrgency(int64_t llNowMs) override { (void)llNowMs; }

private:

    void stopModules();
    void watchConfigFile(int64_t llNowMs);
    /**
     * @brief   : 把新快照落到各消费方（池子、裁剪、规则引擎）
     * @param    {int64_t} llNowMs：节拍线程单调时刻，透传给规则引擎用于给被关掉的行为补发 END
     */
    void applySnapshot(const ConfigPtr &spSnap, int64_t llNowMs);
    /**
     * @brief   : 按 gallery.* 建/换底库，并把同一个指针交给考勤账本
     */
    void applyGallery(const CConfigSnapshot_S &stSnap);
    /* 把配置里的模型表同步进池子描述表：路径/分组/驻留账改了必须落地，否则"下发成功"
       只是 JSON 换了一版而池子还在用旧表——"配了不生效"比崩一次难查得多。
       已驻留的条目不换描述（换了预算记账就和实际对不上），只把何时生效说清楚 */
    void syncModelDescs(const CConfigSnapshot_S &stSnap);
    void warmGroups(const CConfigSnapshot_S &stSnap, int64_t llNowMs);
    void releaseGroup(ModelGroup_E enGroup);
    /**
     * @brief   : 放掉**一颗**模型（租约 + 裁剪档 + 驻留），返回 m_mapHeld 里它的下一格
     */
    ModelLeaseMap::iterator releaseModel(ModelLeaseMap::iterator ritHeld);
    void ensureCropUnit(const CModelDesc_S &stDesc, const CConfigSnapshot_S &stSnap);
    void dropCropUnits(ModelGroup_E enGroup);
    void dropCropUnit(const std::string &strKey);
    bool groupHeld(ModelGroup_E enGroup) const;
    /**
     * @brief   : 把引擎那几个暖组相关的成员打包成判据的入参（状态不外移，只"这一拍问一句"）
     */
    CWarmNeed_S warmNeedOf() const;
    /**
     * @brief   : 期望驻留**组**集合 = 配置需求 ∪ 两份事件触发的保留期，再按守护档位与轮转裁剪
     * @param    {int64_t} llNowMs：本拍时刻，只用来判那两份闩到没到期
     */
    uint32_t effectiveGroupMask(const CConfigSnapshot_S &stSnap, int64_t llNowMs) const;

    void tickLoop();
    void tickOnce(int64_t llNowMs);
    /**
     * @brief   : 跑一拍感知（含节拍判定、像素帧去重、特征帧发布）
     */
    void perceiveStep(const ConfigPtr &spSnap, int64_t llNowMs);
    /**
     * @brief   : 喂规则引擎并把这一拍产出的事件全部取走
     */
    void ruleStep(int64_t llNowMs);
    /**
     * @brief   : 一条事件的出口
     */
    void reportEvent(const CBehaviorEvent_S &stEvent);
    /* 把合并器输出环排空，逐条走事件出口 */
    void drainMerger();
    /* 合并后真正出门的一条事件 */
    void emitEvent(const CBehaviorEvent_S &stEvent);
    /* 事件日志的限流闸：返回 true=这一条值得落日志。
       窗口内超过配额就静默计数，窗口翻页时由调用方补一条抑制摘要 */
    bool eventLogAdmit(int64_t llNowMs, uint64_t &ru64Suppressed);
    /* 即时事件通道：单事件一条 kind=event 报文，
       事件发生那一拍就出门，不等最长 60s 的周期报表 */
    void deliverEventPush(const CBehaviorEvent_S &stEvent);
    /**
     * @brief   : 考勤账本喂观测：把本拍新特征帧里的每一行变成一条在场观测
     */
    void attendStep(int64_t llNowMs);
    /**
     * @brief   : 群体指标采样（每 group_interval_ms 一条），并把会话累计量积分到该时刻
     */
    void dataStep(int64_t llNowMs);
    /**
     * @brief   : 把这一拍的事实喂给平台统计状态机
     * @param    {const ConfigPtr&} spSnap：本拍配置（考勤开关与巡视开关从这里取）
     * @param    {const CGroupMetric_S&} stMetric：本拍群体样本（参与度/纪律/人数）
     * @param    {const ConstFeatureFramePtr&} spFrame：本拍特征帧（聚集判定的头框与位置）
     * @param    {int32_t} nRowNum：m_vecRowInput 有效行数
     */
    void statsFeedStep(const ConfigPtr &spSnap, const CGroupMetric_S &stMetric,
                       const ConstFeatureFramePtr &spFrame, int32_t nRowNum, int64_t llNowMs);
    /* 平台统计出包时点：排在 dataStep 之后（本拍最新事实先落进状态机） */
    void statsStep(int64_t llNowMs);
    /**
     * @brief   : 座位热力折样。与群体采样同一拍、同一个步长，因此排在 dataStep 里
     * @param    {const ConstFeatureFramePtr&} spFrame：本拍依据的特征帧（几何从这里来，画格子要用）
     * @param    {int32_t} nRowNum：m_vecRowInput 里本拍有效的行数（0 = 空场）
     */
    void seatStep(const ConstFeatureFramePtr &spFrame, int32_t nRowNum, int32_t nStepMs,
                  int64_t llConfVer);
    /**
     * @brief   : 学生机位轨迹折样
     * @param    {int32_t} nRowNum：m_vecRowInput 里本拍有效的行数（0 = 空场）
     */
    void stuTrackStep(int32_t nRowNum, int64_t llNowMs);
    /**
     * @brief   : 板书采集时机的一拍
     */
    void boardStep(int64_t llNowMs);
    /**
     * @brief   : OCR 后端换入/换出
     */
    void ocrAttach(const CConfigSnapshot_S &stSnap);
    /**
     * @brief   : 把待发的板书记录取进一份报表素材（受条数与字节两个上限约束）
     * @param    {CReportBundle_S&} rstBundle：IN/OUT，填入 vecBoard / bBoardEnable / nBoardState
     */
    void drainBoard(CReportBundle_S &rstBundle);
    /**
     * @brief   : 取时段读数进报表（每次带上还在延长的那一段，闭合段最多带 CLASSROOM_SCENE_REPORT_MAX 条）
     */
    void drainScene(CReportBundle_S &rstBundle);
    /**
     * @brief   : 把一条群体样本折成场景事实并喂进时段推断
     */
    void sceneStep(const CGroupMetric_S &stMetric, const ConfigPtr &spSnap, int64_t llNowMs);
    /**
     * @brief   : 到点出一份报表：聚合 → 出口 → 补传，并看护会话时长上限
     */
    void tryReport(int64_t llNowMs);
    /**
     * @brief   : 把一份报表素材落成报文文本并送出去（成功直发 / 失败落盘，二选一）
     * @param    {uint32_t} u32Type：SpoolType_E，随报文一起落盘，补传时原样交回通道
     */
    void deliverReport(const CReportBundle_S &stBundle, uint32_t u32Type);
    /**
     * @brief   : 会话收尾：结算账本、出一份收尾报表并让账本从零开始记下一节
     * @param    {const char*} pszReason：只进日志，写的是"谁让收的尾"
     *            （enable_off / session_timeout），现场回答"一节课怎么变两节"全靠它
     */
    void closeSession(int64_t llNowMs, const char *pszReason);
    /**
     * @brief   : 补传滞留环里的积压，预算 = 每拍 spool_drain_max_kb
     */
    void drainSpool();
    /**
     * @brief   : 滞留环参数换入（init 与 applySnapshot 共用，只在节拍线程调）
     * @return   {bool} false 时 rstrErr 说明原因；失败不影响报表直发路径
     */
    bool spoolApply(const CConfigSnapshot_S &stSnap, std::string &rstrErr);
    /**
     * @brief   : 心跳落点换入（init 与 applySnapshot 共用，只在节拍线程调）
     */
    void metricApply(const CConfigSnapshot_S &stSnap);
    void guardSample(int64_t llNowMs);
    void applyGuardLevel(GuardLevel_E enNew);
    void rotateHeavyIfDue(int64_t llNowMs, const CConfigSnapshot_S &stSnap);
    void sliceBusySampleBegin(void);    /* 片起点清增量账（武装与换片两处片首调用） */
    int64_t sliceOfGroup(ModelGroup_E enGroup) const;
    void metrics(int64_t llNowMs);
    bool beatDue(int64_t &rlLastMs, int64_t llNowMs, int32_t nIntervalMs) const;

private:
    CClassroomConfig m_stConfig;
    CFrameBus        m_stBus;
    CFeatureFrameBus m_stFeatureBus;
    CModelPool       m_stPool;
    std::unique_ptr<CInferScheduler> m_spSched;

    /* 模型键 → 裁剪单元。条目数 ≤ CLASSROOM_MAX_MODELS，随组驻留而生灭：
       组不驻留时它的输入缓冲没有存在的理由 */
    mutable std::mutex m_mtxCrop;
    std::map<std::string, CropUnitPtr> m_mapCrop;

    /* ---- 以下成员只在节拍线程读写，不需要锁 ---- */
    /* 引擎持有的模型租约：持有即驻留。功能关 → 放租约 → 立刻 unloadGroup，
       不靠空闲超时兜底（超时兜底会让"关了功能"到"内存回来"之间白占一个 IDLE_UNLOAD 窗口） */
    ModelLeaseMap m_mapHeld;
    std::map<std::string, BackendState_E> m_mapWarmState;   /* 只为「状态变了才打日志」服务 */
    uint32_t m_u32WantedMask = 0;
    /* 上一拍"该驻留哪几颗"的指纹（按 vecModels 下标置位） */
    uint32_t m_u32ModelMask = 0;

    /* ---- 考勤账本（只在节拍线程写；报表侧读的是 snapshot() 的拷贝，不碰账本本体）---- */
    std::unique_ptr<IFaceGallery> m_spGallery{ new CNullFaceGallery() };
    CAttendanceLedger m_stAttend;
    uint32_t m_u32AttendSeq = 0;      /* 已喂过观测的最后一张特征帧序号 */
    bool     m_bAttendOpen = false;   /* 本节账是否开着。false 时下一条观测开一节（含收尾之后） */
    /* ---- 座位热力 ---- */
    CSeatHeat m_stSeat;
    /* 上一次建图所用的配置版本号（只在节拍线程读写）。区域表是按"版本号变了就重算"驱动的，
       热力图必须跟着同一个信号：重画学生席区是现场常规操作，而格子号与外接框都来自那块区域 */
    int64_t m_llSeatGridVer = -1;
    /* ---- 座位先验—— */
    CSeatPrior m_stPrior;
    /* ---- 学生机位轨迹：与热力、先验同一条线、同一采样拍 ---- */
    CStuTrack m_stStu;

    /* ---- 板书记录：只在节拍线程读写，与座位热力同一线程域 ---- */
    CNullOcrBackend m_stOcrNull;
    CPPOcrBackend   m_stOcrPP;
    CBoardWatcher m_stBoard;
    /* 已经生效的那份字典路径（只在节拍线程读写）。留它而不是每次换版都 init 一遍：
       换后端会把在途作业与那格结果一起丢掉，而"运维只是改了个无关字段"是每次下发都发生的事 */
    std::string     m_strOcrDictApplied;
    /* 镜像/旋转的装法下"每次采集都交不出一张能认的图"是一条永久状态，而它在读数上
       只是 board 段里 st= 那一格不动。一次换配置说一遍就够（每拍说一遍会把日志环占掉，
       见 #50 那条口径）；换版时清回去，因为装法本身就是配置项 */
    bool            m_bOcrMountWarned = false;
    int64_t       m_llBoardRectVer = -1;

    /* ---- 课堂场景时段：只在节拍线程被喂与取，无锁 ----
       m_u64BoardEdgeSeen 是把"板书边沿计数"变成"上次采样以来有没有发生过一次"的游标。
       留这个游标而不是让场景层去读 watcher：那样时段层就依赖了板书的实现，
       而它需要的只是一句"这段时间里老师写过板" */
    CSceneInfer m_stScene;
    uint64_t    m_u64BoardEdgeSeen = 0;

    /* ---- P3 群体统计与上报（全部只在节拍线程写，读侧走 CDataCenter 自己的锁）---- */
    CDataCenter m_stData;
    /* 平台统计推送：只吃引擎事实、只在节拍线程推进；推不出去时自己有界重试 */
    CStatsHub m_stStatsHub;
    /* 统计侧读账本记录的复用缓冲（只有群体采样那一拍用，60 行上限，避免每 5s 新分配一次） */
    std::vector<CAttendRec_S> m_vecAttendScratch;
    /* 事件合并器（P0-2）。挂在 reportEvent 与聚合环之间：事件先进它，
       可能被并成一段再出去。与规则环同一个线程域（节拍线程），因此无锁 */
    CEventMerger m_stMerger;
    int64_t m_llLastSampleMs = 0;       /* 上一条群体样本的时刻 */
    int64_t m_llLastReportMs = 0;       /* 上一次出报表的时刻 */
    /* 事件日志限流：窗口内最多 MAX 条原文，超出的只计数，翻页补一条摘要。
       事件日志是聚合层出错时唯一的对照物，不能删——但抖动风暴时它能一分钟冲几百行，
       把 mlog 环与业务环一起灌满，那也是事故 */
    int64_t  m_llEvtLogWinMs = 0;       /* 当前窗口起点（0 = 还没开张） */
    uint32_t m_u32EvtLogWin = 0;        /* 窗口内已落的条数 */
    uint64_t m_u64EvtLogHeld = 0;       /* 窗口内被抑制的条数 */
    /* 即时事件通道的观测：推送条数与未推条数（未接通道/没进环都算未推） */
    uint64_t m_u64EvtPush = 0;
    uint64_t m_u64EvtPushSkip = 0;
    /* 即时告警通道的三个数分开记：attempt=判定该送、fail=通道说没送到。
       两个数之差是「平台真收到了几条」——现场问「平台怎么没触发查询」时，
       先看 attempt 有没有涨：不涨是判定没出事件，涨而 fail 同涨是没客户端/推送失败。
       atomic 的理由与 sys= 那些计数相同：statusJson 在心跳线程读、这里在节拍线程写，
       32 位设备上一次 64 位非原子读会撕裂成一个谁也没写过的数 */
    std::atomic<uint64_t> m_u64EvtAlarmAttempt{0};
    std::atomic<uint64_t> m_u64EvtAlarmOk{0};
    std::atomic<uint64_t> m_u64EvtAlarmFail{0};
    CEventAlarmFn m_fnEventAlarm;      /* 未注入时 207/209 的 BEGIN 不上送（只进报表） */
    /* 断网滞留落盘。做成成员而不是文件级单例：它的参数（目录、容量、补传预算）
       来自配置快照，因而必须跟着引擎起停；单例会让"改了配置要重建"变成"要记得去戳它一下" */
    CReportSpool m_stSpool;
    CSpoolParam_S m_stSpoolParam;       /* 已生效的那一份，用来判"这次下发到底变了没有" */
    /* 上面那份参数是不是真的应用成功过。只比参数不够：失败时参数会被回滚成默认值，
       而默认值恰好等于出厂配置，一比就"没变"，一次临时失败于是冻成永久没有补传 */
    bool     m_bSpoolApplied = false;
    /* 心跳落点。成员而不是文件级单例的理由比滞留环那条更硬：
       它的计数（lines/fb/head）是**本进程这一轮**的读数，而抖动测试会把引擎起停上百次——
       进程活着的单例会把上一轮的计数摊进这一轮的心跳里，看着像漏了 fb 其实是对的账错了轮 */
    CClassroomMetricLog m_stMetricLog;
    /* 上一次落点给自己的那句话（"变了才出声"的比较基准，也是 statusJson 的现成答案） */
    std::string m_strMetricHow = "mlog=unconfigured";
    /* 出口是否处于"上次没发出去"的状态：只用来打一条恢复边沿的日志。
       断网 4 小时里失败那条已经打过一次，恢复时若不出声，现场只能靠猜"什么时候通的" */
    bool     m_bUploadBroken = false;
    /* 逐人生效位图的临时表：容量 = CLASSROOM_MAX_PERSONS，构造期定死后运行期只覆写不扩容 */
    std::vector<CRowStatInput_S> m_vecRowInput;
    /* 热力折样的临时表：与上一张同容量、同下标（seatStep 逐行搬运），运行期同样不再分配 */
    std::vector<CSeatObs_S> m_vecSeatObs;
    /* 座位先验的输入表：与上一张同容量、同下标（同一批行、同一个行序），运行期同样不再分配 */
    std::vector<CSeatPriorIn_S> m_vecPriorIn;
    /* 轨迹折样的临时表：与上面两张同容量、同下标（stuTrackStep 逐行搬运）*/
    std::vector<CStuTrackObs_S> m_vecStuObs;

    bool     m_bNeedRotate = false;      /* 池子说这两组此刻装不下（腾完仍不够，或对方还在片里让路） */
    /* 本轮轮转的武装时刻（classroomNowMs 口径）：轮转开着就再也问不到池子"两组现在装得下吗"，
       所以压力是否过去只能每 CLASSROOM_ROTATE_RECONSIDER_MS 关一次轮转、按原始目标重问才知道 */
    int64_t  m_llRotateArmedMs = 0;
    ModelGroup_E m_enHeavyTurn = ModelGroup_E::GROUP_POSE;
    int64_t  m_llHeavyUntilMs = 0;
    /* 动态轮转周期的片内采样（判据分档见 classroom_warm_policy.h 的 CLASSROOM_SLICE_BUSY_*）：
       不读 npuBusyPercent()——那把尺的窗口归 metrics() 的心跳管（读完即清零），两家共用一个窗口
       会互相清账。这里自己攒增量：每拍看一眼调度器的累计忙毫秒，涨多少攒多少；心跳清过账的那一拍
       （当前值 < 上一拍值）把当前值整笔收进来——清零前后的每一毫秒都仍落在本片窗口里 */
    int64_t  m_llSliceBusySeenMs = 0;      /* 本片攒下的 NPU 忙毫秒增量 */
    int64_t  m_llSliceBusyPrevMs = -1;     /* 上一拍看到的累计忙毫秒（-1 = 尚未开账） */
    int64_t  m_llSliceT0Ms = 0;            /* 本片起点（classroomNowMs 口径） */
    double   m_ldHeavyLastBusyPct[2] = {-1.0, -1.0};   /* 各重组上一片的占用存档 [0]=POSE [1]=FACE；-1 = 没量过 */
    /* 玩手机复核的驻留到期时间（不是 bool）：候选名单逐拍有空位（人抬一下头就掉出名单），
       存成到期时间把这种抖动合成一段连续的暖组期，存成 bool 就是每次抖动一组往返 */
    int64_t  m_llPhoneWarmUntilMs = 0;
    /* 板书侧的"最近有活动"时刻（不是到期时刻） */
    int64_t  m_llBoardLastActMs = 0;

    /* 规则引擎与它的采样器：都只在节拍线程被驱动（stat()/statJson() 读的是发布件，任意线程可调）。
       声明顺序刻意是「采样器在前、引擎在后」：引擎持的是采样器的裸指针，
       成员按声明逆序析构，于是引擎先亡、采样器后亡——反过来就是给析构期留一个悬垂指针，
       而 deinit 之后的崩溃比 deinit 之中的难查一个量级 */
    CStudentBehaviorSampler m_stStudent;
    CTeacherBehaviorSampler m_stTeacher;
    CRuleEngine             m_stRule;
    /* 考勤检测区域：本拍被区域挡掉的行数（累计）与引用坏掉的次数 */
    std::atomic<uint64_t>   m_stAttendRegionSkip{0};
    std::atomic<uint64_t>   m_u64AttendRegionCfgErr{0};

    ModelLoaderFn m_fnLoader;
    CPerceiveFn   m_fnPerceive;
    CPerceiveStatFn m_fnPerceiveStat;
    CFaceMatchTakeFn m_fnFaceTake;     /* 未注入时考勤只记在场时长，一个名字都不起（见 CFaceMatchTakeFn） */
    CUploadFn m_fnUpload;              /* 未注入时报表只落日志，且不落滞留环（见 CUploadFn） */

    std::thread m_thTick;
    mutable std::mutex m_mtxTick;
    std::condition_variable m_cvTick;
    bool m_bRunning = false;

    int64_t  m_llBootMs = 0;
    int64_t  m_llAppliedVersion = -1;
    int64_t  m_llLastHeadBeatMs = 0;
    int64_t  m_llLastGuardMs = 0;
    int64_t  m_llLastMetricMs = 0;
    int64_t  m_llLastWarmMs = 0;
    uint32_t m_u32PixelSeqHandled = 0;
    bool     m_bGeomStale = false;

    /* 守护回涨迟滞：连续 N 个采样周期水位充裕才允许降一档 */
    int32_t  m_nRecoverStreak = 0;
    GuardLevel_E m_enLevel = GuardLevel_E::GL_NORMAL;

    /* ---- 配置文件看护（只在节拍线程读写）---- */
    std::string m_strConfigPath;          /* init() 定死的绝对路径，运行期不换 */
    int64_t  m_llLastConfigCheckMs = 0;
    int64_t  m_llConfigMTimeSec = 0;      /* 上次采纳的 mtime（秒）与 size，两者一起变才算改过 */
    int64_t  m_llConfigSize = 0;
    /* 上次采纳配置的**内容**指纹 */
    uint64_t m_u64ConfigHash = 0;
    bool     m_bConfigFilePresent = false;

    mutable std::mutex m_mtxStat;
    CEngineStat_S      m_stStat;
    /* 感知读数缓存：节拍线程在 metrics 里取串写进来，statusJson 在同一个锁内读缓存。
       不能让 statusJson 直接调感知层的读数——那组计数器无锁，跨线程取会撕裂（见 CPerceiveStatFn） */
    std::string        m_strPerceiveDetail;
    uint64_t m_u64FrameSeqGen = 0;
    int64_t  m_llLastFrameAcceptMs = 0;  /* 上次入槽时刻（节流用）：只在取流线程写，与计数同锁保护 */
};
}
