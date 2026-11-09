/**
 * @FilePath     : board_ocr.h
 * @Author       : leiyy
 * @Date         : 2026-11-03 13:49:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 13:49:00
 * @Description  : 板书 OCR 的设备侧后端
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "classroom_define.h"
#include "infer_scheduler.h"
#include "model_pool.h"
#include "ocr_decode.h"
#include "ocr_pipeline.h"
#include "rga_pool.h"

namespace Classroom
{
/* 单次推理（一次 det 或一次 rec）的等待上限 */
#define CLASSROOM_OCR_INFER_WAIT_MS (800)

/* 一次作业最多等多少毫秒还没被取走就作废 */
#define CLASSROOM_OCR_JOB_STALE_MS (15000)

/**
 * @brief 后端参数
 */
typedef struct _OcrBackendParam_
{
    std::string   strDictPath;                              /* 字典绝对路径（由 ocrDictPathOf 推出） */
    COcrDbParam_S stDb;                                     /* DB 三道门与容量 */
    int32_t       nInferWaitMs = CLASSROOM_OCR_INFER_WAIT_MS;
    /* 调度优先级（0 最高）。取现档最低 */
    int32_t       nPriority = CLASSROOM_INFER_PRIORITY_BOARD_OCR;
} COcrBackendParam_S;

/**
 * @brief 一次板面识别作业的结束读数（引擎按它回填 CBoardWatcher::onOcrResult）
 */
typedef struct _OcrJobResult_
{
    uint32_t  u32Seq = 0;         /* 就是 submitJob 的返回值，引擎按它配对请求（见 markCaptureSubmitted） */
    int64_t   llDueMs = 0;        /* 这次请求对应的到期时刻（board_watcher 的记账口径）：
                                     投出去时给的那个数，原样带回，引擎不另算一份 */
    bool      bOk = false;        /* true=可信，**含空板**（vecText 为空即空板） */
    OcrPipeStage_E enStage = OcrPipeStage_E::OCR_PIPE_BAD_PARAM;
    BackendState_E enState = BackendState::MODEL_MISSING;   /* bOk=false 时的原因细分 */
    std::vector<CBoardText_S> vecText;
} COcrJobResult_S;

/**
 * @brief 后端自身的观测（与 COcrPipeStat_S 分开的理由：那一族是"识别得怎么样"，
 *        这一族是"这条线程有没有在跑"，查错时是两条完全不同的路）
 */
typedef struct _OcrBackendStat_
{
    uint64_t u64JobIn = 0;            /* 被接受的作业数 */
    uint64_t u64JobRejectBusy = 0;    /* 信箱 occupied 被拒（含上一张还在跑与结果没被取走） */
    uint64_t u64JobDone = 0;          /* worker 做完并放进结果格的作业数 */
    uint64_t u64JobStale = 0;         /* 出队时发现已过 CLASSROOM_OCR_JOB_STALE_MS，不跑 */
    uint64_t u64ShapeBad = 0;         /* 形状解不出来（D3 的降级档），每次作业计一次 */
    uint64_t u64NoModel = 0;          /* 模型没驻留（peek 失败） */
    uint64_t u64RgaFail = 0;          /* 一次 RGA 搬运被拒（对齐/越界） */
    uint64_t u64SubmitFail = 0;       /* 调度器拒收（未运行 / 队列满且优先级不够） */
    uint64_t u64InferTimeout = 0;     /* 等到上限仍无回调 */
    uint64_t u64InferUnready = 0;     /* 回调报 IR_MODEL_UNREADY */
    uint64_t u64InferFail = 0;        /* 回调报后端失败 / 过期 / 被取消 */
    int64_t  llLastJobMs = 0;         /* 最近一次成功作业的**总**耗时（det + 逐行 rec） */
} COcrBackendStat_S;

/**
 * @brief   : PP-OCR 后端：det 概率图 → 行框 → 逐行 RGA 裁剪 → rec → CTC
 * @note    : 线程模型：recognize() 与 submitJob()/takeResult() 都可被外部线程调，
 *            信箱与状态自带锁；而**一次识别（recognizeBuf 及其往下调的
 *            runDet/cropRow/runRec）全进程同一时刻只有一个在跑**，由 m_mtxRun 保证——
 *            异步入口在 ClsOcr 线程上拿它，同步入口在调用方线程上拿它
 */
class CPPOcrBackend : public IOcrBackend, public IOcrStagePort
{
public:
    CPPOcrBackend() = default;
    ~CPPOcrBackend() override;

    CPPOcrBackend(const CPPOcrBackend &) = delete;
    CPPOcrBackend &operator=(const CPPOcrBackend &) = delete;

    /**
     * @brief   : 装配依赖并起 worker 线程
     * @param    {CInferScheduler*} pSched：非拥有，须比本对象活得久
     * @param    {CModelPool*} pPool：同上，只用于 peek（见 D3）
     * @return   {bool} false=参数非法、字典装载失败或线程起不来。
     *                  字典装载失败是**部署缺项**而不是运行期错误：它在这颗模型的目录里，
     *                  文件不在就是没投放，当场报比每次识别报一次有用
     */
    bool init(CInferScheduler *pSched, CModelPool *pPool, const COcrBackendParam_S &stParam);
    /**
     * @brief   : 停线程并释放两块常驻缓冲（D6/D8）。可重复调
     * @note    : 调用方必须先确保不再 submitJob；引擎的 deinit 顺序已保证（节拍线程先停）
     */
    void deinit();

    /* 能力位：init 成功且字典可用。引擎用它决定给 watcher 挂本后端还是挂空后端 */
    bool available() const;

    /**
     * @brief   : 投一张已经裁好的板面图（异步，不阻塞调用线程）
     * @param    {const RgaBufPtr&} rspRgb：det 输入画布，**尺寸必须等于 det 模型的输入**
     *            （引擎按 crop 档直出，引用计数持有 ⇒ 不必再拷一次，见 D6/D7）
     * @return   {uint32_t} **作业号**（0=没投出去：信箱那一格还占着）。非 0 的那个号会原样
     *            出现在 COcrJobResult_S::u32Seq 上，调用方按它配对请求
     * @note    : 号只由本类数（m_u32JobSeq 自增），调用方不再自己数第二份：两份计数器只要
     *            有一边重启就永远配不上，症状是"结果全被判成迟到"。
     *            不阻塞的理由见 D4；返回 0 时本次采集该就地回填失败，
     *            否则 watcher 的在途标记要等 llInflightTimeoutMs 才放行（现象是"几十秒不动一下"）
     */
    uint32_t submitJob(const RgaBufPtr &rspRgb, int32_t nW, int32_t nH, int64_t llDueMs);

    /**
     * @brief   : 取一次已完成的作业读数（取走即清空那一格）
     * @return   {bool} false=还没有结果
     */
    bool takeResult(COcrJobResult_S &rstOut);

    /* 信箱那一格是否还占着（有作业在途或有结果没取走）——引擎用它决定这一拍要不要发起采集 */
    bool busy() const;

    /* 换配置里的 DB 门限（热更）。不影响字典与线程 */
    void setDbParam(const COcrDbParam_S &stDb);

    COcrBackendStat_S stat() const;
    COcrPipeStat_S    pipeStat() const;
    COcrDbStat_S      dbStat() const;

    /* ---- IOcrBackend：同步入口，见 D7 ---- */
    bool recognize(const uint8_t *pucRgb, int32_t nW, int32_t nH,
                   std::vector<CBoardText_S> &rvecOut) override;
    const char *name() const override { return "ppocr"; }

private:
    /* ---- IOcrStagePort：只在 ClsOcr 线程上被 ocr_pipeline 调（见类注释的线程模型） ---- */
    bool runDet(const uint8_t *pucRgb, int32_t nW, int32_t nH,
                std::vector<float> &rvecMap) override;
    bool cropRow(const uint8_t *pucSrc, int32_t nSrcW, int32_t nSrcH, const COcrBox_S &stBox,
                 int32_t nCanvasW, int32_t nCanvasH, int32_t nFilledW,
                 COcrRowCanvas_S &rstOut) override;
    bool runRec(const COcrRowCanvas_S &stCanvas, std::vector<float> &rvecOut) override;

    /* det/rec 画布尺寸与输出形状（D2/D3）。已解过时只回读，不再 peek 句柄 */
    bool ensureShapes();
    /* 一次板面识别（det + 逐行 rec）：recognize 与 worker 共用的那一份实现。
       进门自持 m_mtxRun，所以两个入口可以同时活着 */
    bool recognizeBuf(const RgaBufPtr &rspRgb, int32_t nW, int32_t nH,
                      COcrJobResult_S &rstOut, int64_t *pllCostMs);
    void workerLoop();
    /* 提交一次推理并同步等回调（本类自己那一版，不借 CPerceivePipeline 的槽：
       那套协议要照顾"每拍一条、上一拍还在途"的背压，而这里一次作业内串行、没有跨拍复用） */
    bool submitAndWaitInfer(const std::string &strKey, const RgaBufPtr &rspBuf,
                            std::vector<float> &rvecOut);

    CInferScheduler *m_pSched = nullptr;
    CModelPool      *m_pPool = nullptr;
    COcrBackendParam_S m_stParam;

    COcrTokenTable  m_stToken;
    COcrDetShape_S  m_stDetShape;
    COcrRecShape_S  m_stRecShape;
    int32_t m_nCanvasW = 0;       /* rec 画布宽（句柄给的模型输入宽，D2） */
    int32_t m_nCanvasH = 0;
    uint32_t m_u32DetInW = 0;     /* det 模型输入宽：作业送来的图必须正好是这个尺寸（不匹配就不跑） */
    uint32_t m_u32DetInH = 0;
    bool    m_bShapeOk = false;
    bool    m_bShapeDead = false; /* 解过一次解不出来：不再重试（D3） */

    /* D6/D7：一次识别只让一张板子在设备上跑。它保护的是 m_spDetIn / m_spRowBuf
       这两格"本次识别正在用的载体"，不是信箱、不是计数 */
    std::mutex m_mtxRun;
    /* 本次识别正在用的 det 输入载体：异步=作业缓冲（零拷贝），同步=调用方那次 memcpy 的落点。
       runDet 只认这一格而不认传进来的裸指针——调度器要的是引用计数缓冲，裸指针进不了那条协议（D7） */
    RgaBufPtr m_spDetIn;
    RgaBufPtr m_spRowBuf;         /* rec 行画布，RGA 直写 + padRecCanvasRight 摊开 */

    mutable std::mutex m_mtx;
    std::condition_variable m_cv;
    /* 信箱（D4）：一格作业 + 一格结果 */
    RgaBufPtr m_spJobRgb;
    int32_t m_nJobW = 0, m_nJobH = 0;
    int64_t m_llJobDueMs = 0;
    uint32_t m_u32JobSeq = 0;
    bool m_bJobIn = false;
    COcrJobResult_S m_stResult;
    bool m_bResultReady = false;

    std::thread m_thWorker;
    bool m_bRunning = false;

    /* 一次推理的等待位。写侧是 NPU worker 线程上的回调、读侧是持有 m_mtxRun 的那条线程
       （ClsOcr 或同步入口的调用方），所以它必须自带锁；而它**只有一格**，
       靠 m_u32InferSeq 把超时之后迟到的旧回调整份丢掉（见 submitAndWaitInfer） */
    std::mutex m_mtxInfer;
    std::condition_variable m_cvInfer;
    bool m_bInferDone = false;
    uint32_t m_u32InferSeq = 0;
    bool m_bInferOk = false;
    InferResultReason_E m_enInferReason = InferResultReason::IR_BACKEND_FAIL;
    std::vector<float> m_vecInferRaw;

    COcrBackendStat_S m_stBackendStat;    /* m_mtx 保护 */
    COcrPipeStat_S    m_stPipeStat;       /* m_mtxRun 保护（同一时刻只有一个入口在跑） */
    COcrDbStat_S      m_stDbStat;         /* 同上 */
};
}
