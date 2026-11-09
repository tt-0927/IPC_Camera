/**
 * @FilePath     : board_ocr.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-03 13:49:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 13:49:00
 * @Description  : 板书 OCR 后端的设备侧实现
 */

#include "board_ocr.h"

/* RGB888 每像素字节数 */
#define CLASSROOM_OCR_RGB_BYTES_PER_PX (3u)

#include <chrono>
#include <cstring>
#include <pthread.h>

#include "classroom_clock.h"
#include "common_process.h"
#include "dlog.h"
#include "raw_model_handle.h"

namespace Classroom
{
namespace
{
/* det 输入与行裁剪的目标通道序。**这里只允许一处定义**：换 R/B 不会报错，
   只会让整幅图的每个字都变成"认得出但认错了"，而 rec 的训练侧用的是 RGB。
   引擎侧的裁剪档必须与它同值 */
constexpr int OCR_RGA_FORMAT_RGB = RK_FORMAT_RGB_888;

/* deadline = 等待上限 × 该值，与 CPerceivePipeline 同一口径：
   调用方不等了之后任务仍要跑完才能出回调，deadline 先到会把"慢一拍"变成"每拍白提交一次" */
constexpr int OCR_DEADLINE_TIMES = 2;

/* 把 peek 里要抄出来的东西放一起：fnHandle 在池锁内执行，只能做常数时间的赋值类操作，
   所以这里把首路形状**逐维拷进栈上的结构**而不是存句柄指针（存指针等于把句柄带出池锁，
   而句柄可能在下一次卸载时被销毁——那是一座出了锁就成立的悬空桥） */
struct CRawFacts_
{
    bool bFound = false;
    CRawTensorShape_S stShape0;
    uint32_t u32InW = 0;
    uint32_t u32InH = 0;
    uint32_t u32OutNum = 0;
};

bool readRawFacts(CModelPool &rPool, const std::string &strKey, CRawFacts_ &rstFacts)
{
    rstFacts = CRawFacts_();
    rPool.peek(strKey, [&rstFacts](IModelHandle &rstHandle)
    {
        /* 后端选错不会报错，只会让后处理按错的张量布局解析（model_pool.h 里那条同一风险）。
           本篇只服务 raw 后端：不是 CRawModelHandle 就当"没有这一路"，交调用方降级 */
        const CRawModelHandle *pRaw = dynamic_cast<const CRawModelHandle *>(&rstHandle);
        if (nullptr == pRaw)
        {
            return;
        }
        rstFacts.bFound = true;
        rstFacts.u32InW = pRaw->modelInputWidth();
        rstFacts.u32InH = pRaw->modelInputHeight();
        rstFacts.u32OutNum = pRaw->outputTensorNum();
        rstFacts.stShape0 = pRaw->outputShape(0);
    });
    return rstFacts.bFound;
}
}

CPPOcrBackend::~CPPOcrBackend()
{
    deinit();
}

bool CPPOcrBackend::init(CInferScheduler *pSched, CModelPool *pPool,
                         const COcrBackendParam_S &stParam)
{
    deinit();

    if (nullptr == pSched || nullptr == pPool || stParam.nInferWaitMs <= 0)
    {
        dlog_error("classroom ocr: 初始化被拒, sched=%p pool=%p wait=%d",
                   (void *)pSched, (void *)pPool, stParam.nInferWaitMs);
        return false;
    }
    /* 优先级越界的后果不是报错而是**每次提交被当场判成 bad request**
       （infer_scheduler.h 里那条同一现象）：这里先挡住，读数才分得清"没跑"与"跑了被拒" */
    if (stParam.nPriority < 0 || stParam.nPriority >= CLASSROOM_INFER_PRIORITY_LEVEL)
    {
        dlog_error("classroom ocr: 优先级 %d 超出 [0,%d)",
                   stParam.nPriority, CLASSROOM_INFER_PRIORITY_LEVEL);
        return false;
    }

    std::string strErr;
    COcrTokenTable stToken;
    if (!stToken.loadFromFile(stParam.strDictPath, strErr))
    {
        dlog_error("classroom ocr: 词典不可用: %s", strErr.c_str());
        return false;
    }

    m_pSched = pSched;
    m_pPool = pPool;
    m_stParam = stParam;
    m_stToken = stToken;
    m_bRunning = true;
    try
    {
        m_thWorker = std::thread(&CPPOcrBackend::workerLoop, this);
    }
    catch (const std::system_error &)
    {
        m_bRunning = false;
        dlog_error("classroom ocr: 工作线程创建失败");
        return false;
    }
    dlog_info("classroom ocr: 后端已启动, tokens=%u wait=%dms prio=%d",
              m_stToken.numTokens(), m_stParam.nInferWaitMs, m_stParam.nPriority);
    return true;
}

void CPPOcrBackend::deinit()
{
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        m_bRunning = false;
        m_bJobIn = false;
        m_spJobRgb.reset();
        m_stResult.vecText.clear();
        m_bResultReady = false;
    }
    m_cv.notify_all();
    if (m_thWorker.joinable())
    {
        m_thWorker.join();
    }
    /* 行画布在这里归还（D6） */
    m_spDetIn.reset();
    m_spRowBuf.reset();
    m_pSched = nullptr;
    m_pPool = nullptr;
}

bool CPPOcrBackend::available() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_bRunning && m_stToken.ready();
}

void CPPOcrBackend::setDbParam(const COcrDbParam_S &stDb)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stParam.stDb = stDb;
}

uint32_t CPPOcrBackend::submitJob(const RgaBufPtr &rspRgb, int32_t nW, int32_t nH, int64_t llDueMs)
{
    if (!rspRgb || nW <= 0 || nH <= 0 || llDueMs <= 0)
    {
        return 0;
    }
    std::lock_guard<std::mutex> stLock(m_mtx);
    if (!m_bRunning || m_bJobIn || m_bResultReady)
    {
        /* 结果没取走也算占着：一张没人读的板子不该被第二张顶掉，
           否则报表上会出现"采了两次而第一次的内容永远消失" */
        ++m_stBackendStat.u64JobRejectBusy;
        return 0;
    }
    m_spJobRgb = rspRgb;
    m_nJobW = nW;
    m_nJobH = nH;
    m_llJobDueMs = llDueMs;
    /* 序号在这里定，结果格由 worker 整份写入（它才知道耗时与失败档）。
       这里不清 m_stResult：那一格只在 m_bResultReady 为真时被读，而上一条取走时已清零 */
    ++m_u32JobSeq;
    m_bJobIn = true;
    ++m_stBackendStat.u64JobIn;
    m_cv.notify_all();
    return m_u32JobSeq;
}

bool CPPOcrBackend::takeResult(COcrJobResult_S &rstOut)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    if (!m_bResultReady)
    {
        return false;
    }
    rstOut = std::move(m_stResult);
    m_stResult = COcrJobResult_S();
    m_bResultReady = false;
    return true;
}

bool CPPOcrBackend::busy() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_bJobIn || m_bResultReady;
}

COcrBackendStat_S CPPOcrBackend::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stBackendStat;
}

COcrPipeStat_S CPPOcrBackend::pipeStat() const
{
    /* 与 CPerceivePipeline::stat() 同一口径（不加锁、直接拷贝）：m_mtxRun 保证了
       任一时刻只有一个写者，所以读侧最坏是"少看了这几次"，不会读到半个结构。
       每档都是整数累加：查错要的是差值，不是同一瞬间的一致快照 */
    return m_stPipeStat;
}

COcrDbStat_S CPPOcrBackend::dbStat() const
{
    return m_stDbStat;
}

bool CPPOcrBackend::recognize(const uint8_t *pucRgb, int32_t nW, int32_t nH,
                              std::vector<CBoardText_S> &rvecOut)
{
    rvecOut.clear();
    if (nullptr == pucRgb || nW <= 0 || nH <= 0)
    {
        return false;
    }
    /* 同步入口付一次整幅拷贝 + 一次缓冲分配（D7）：调度器只认引用计数缓冲，而本签名给的是裸指针。
       实际调用方（引擎）走 submitJob，这条只留给"外部拿着一张图就想认一次"的用法，
       以及没有 worker 的场合（例如一次性诊断工具）——是诊断路径就不为它省这一次 malloc，
       缓冲做成调用的局部量，它和 m_spDetIn 之间就不再需要任何跨调用状态 */
    const size_t nBytes = static_cast<size_t>(nW) * static_cast<size_t>(nH) * CLASSROOM_OCR_RGB_BYTES_PER_PX;
    RgaBufPtr spRgb = std::make_shared<std::vector<uint8_t>>();
    spRgb->resize(nBytes);
    std::memcpy(spRgb->data(), pucRgb, nBytes);

    COcrJobResult_S stRes;
    if (!recognizeBuf(spRgb, nW, nH, stRes, nullptr))
    {
        return false;   /* 每一档失败已在 recognizeBuf / submitAndWaitInfer 里计过 */
    }
    rvecOut.swap(stRes.vecText);
    return true;
}

bool CPPOcrBackend::ensureShapes()
{
    if (m_bShapeOk || m_bShapeDead)
    {
        return m_bShapeOk;
    }
    if (nullptr == m_pPool || nullptr == m_pSched)
    {
        return false;   /* 没装配：不是降级判据，调用方按"还没开始跑"处理 */
    }

    CRawFacts_ stDet, stRec;
    if (!readRawFacts(*m_pPool, CLASSROOM_MODEL_KEY_OCR_DET, stDet) ||
        !readRawFacts(*m_pPool, CLASSROOM_MODEL_KEY_OCR_REC, stRec))
    {
        /* 两颗没同时驻留是**常态**（G_OTHER 组按需加载、时间片轮转），不算判据失败：
           留到下一次作业再解，D3 那条"不重试"只针对解出矛盾形状的那种 */
        return false;
    }
    /* 首路形状全零 = 该路自述不可用（fillFromRaw 拒过）；多路输出（2 通道的 DB 头）
       在这里判死而不是留给 DET_SIZE 档：那种模型每次识别都会撞那一档，
       而读数上表现为"这块板子今天一直读不出"，没人会想到是交付件换了头 */
    if (1 != stDet.u32OutNum || 0 == stDet.stShape0.u32NumDims ||
        1 != stRec.u32OutNum || 0 == stRec.stShape0.u32NumDims)
    {
        m_bShapeDead = true;
        dlog_error("classroom ocr: 输出布局不可用, det out=%u dims=%u rec out=%u dims=%u",
                   stDet.u32OutNum, stDet.stShape0.u32NumDims,
                   stRec.u32OutNum, stRec.stShape0.u32NumDims);
        return false;
    }

    /* D2：画布尺寸从句柄读。RGA 的两条硬对齐在这里先挡（不挡住就是每行裁不出来，
       而失败原因会被记成 u64CropFail，查错方向查到裁剪上） */
    const int32_t nCanvasW = static_cast<int32_t>(stRec.u32InW);
    const int32_t nCanvasH = static_cast<int32_t>(stRec.u32InH);
    if (0 != (nCanvasW % 4) || 0 != (nCanvasH % 2))
    {
        m_bShapeDead = true;
        dlog_error("classroom ocr: rec 画布 %dx%d 不满足 RGA 对齐 (W%%4, H%%2)",
                   nCanvasW, nCanvasH);
        return false;
    }

    COcrDetShape_S stDetShape;
    if (!resolveDetMapShape(stDet.stShape0, static_cast<int32_t>(stDet.u32InW),
                            static_cast<int32_t>(stDet.u32InH), stDetShape))
    {
        m_bShapeDead = true;
        dlog_error("classroom ocr: det 图形状歧义, in=%ux%u dims0=%u",
                   stDet.u32InW, stDet.u32InH, stDet.stShape0.u32NumDims);
        return false;
    }
    COcrRecShape_S stRecShape;
    if (!resolveRecShape(stRec.stShape0, nCanvasW, m_stToken, stRecShape))
    {
        m_bShapeDead = true;
        dlog_error("classroom ocr: rec 形状歧义, canvas=%dx%d dims0=%u tokens=%u",
                   nCanvasW, nCanvasH, stRec.stShape0.u32NumDims, m_stToken.numTokens());
        return false;
    }

    m_stDetShape = stDetShape;
    m_stRecShape = stRecShape;
    m_nCanvasW = nCanvasW;
    m_nCanvasH = nCanvasH;
    m_u32DetInW = stDet.u32InW;
    m_u32DetInH = stDet.u32InH;
    m_bShapeOk = true;
    /* 只打一次：这是"这台机器的 OCR 到底按什么尺寸在跑"的唯一现场证据，
       而它不含任何文字（PII 红线） */
    dlog_info("classroom ocr: det 图 %dx%d (in %dx%d), rec T=%u C=%u shift=%u canvas=%dx%d",
              m_stDetShape.nMapW, m_stDetShape.nMapH, m_u32DetInW, m_u32DetInH,
              m_stRecShape.nTimeStep, m_stRecShape.u32ClassNum, m_stRecShape.u32DictShift,
              m_nCanvasW, m_nCanvasH);
    return true;
}

bool CPPOcrBackend::recognizeBuf(const RgaBufPtr &rspRgb, int32_t nW, int32_t nH,
                                 COcrJobResult_S &rstOut, int64_t *pllCostMs)
{
    /* 一次只让一张板子过设备（D7）：m_spDetIn / m_spRowBuf / 三张计数表都是"本次识别"的状态，
       两个入口同时进来就会互相踩——而踩出来的读数全部合法（框有、字也有，只是字是另一张图的） */
    std::lock_guard<std::mutex> stRunLock(m_mtxRun);

    rstOut.vecText.clear();
    rstOut.bOk = false;
    rstOut.enStage = OcrPipeStage_E::OCR_PIPE_BAD_PARAM;

    if (!ensureShapes())
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (m_bShapeDead)
        {
            ++m_stBackendStat.u64ShapeBad;
            rstOut.enState = BackendState::LOAD_FAILED;    /* 布局对不上，重启也不会好 */
        }
        else
        {
            ++m_stBackendStat.u64NoModel;
            rstOut.enState = m_pPool ? m_pPool->stateOf(CLASSROOM_MODEL_KEY_OCR_DET)
                                     : BackendState::DISABLED;
        }
        return false;
    }

    const size_t nNeed = static_cast<size_t>(nW) * static_cast<size_t>(nH) * CLASSROOM_OCR_RGB_BYTES_PER_PX;
    if (!rspRgb || rspRgb->size() != nNeed ||
        nW != static_cast<int32_t>(m_u32DetInW) || nH != static_cast<int32_t>(m_u32DetInH))
    {
        /* 送进来的图与模型输入不符 ⇒ 一次都别跑。让它跑的后果是"识别得很差"，
           而很差在任何读数上都不是错误 */
        std::lock_guard<std::mutex> stLock(m_mtx);
        ++m_stBackendStat.u64RgaFail;
        rstOut.enState = BackendState::READY;
        return false;
    }

    m_spDetIn = rspRgb;
    COcrDbParam_S stDb;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        stDb = m_stParam.stDb;        /* 热更只在这一刻取一次：一次作业内不许换门限 */
    }

    const int64_t llBeginMs = classroomNowMs();
    const OcrPipeStage_E enStage = ocrRunBoard(m_stDetShape, m_stRecShape, m_nCanvasW, m_nCanvasH,
                                               stDb, m_stToken, *this, rspRgb->data(), nW, nH,
                                               rstOut.vecText, &m_stPipeStat, &m_stDbStat);
    m_spDetIn.reset();

    rstOut.enStage = enStage;
    rstOut.bOk = (OcrPipeStage_E::OCR_PIPE_OK == enStage);
    rstOut.enState = rstOut.bOk ? BackendState::READY : BackendState::LOAD_FAILED;
    if (nullptr != pllCostMs)
    {
        *pllCostMs = classroomNowMs() - llBeginMs;
    }
    return rstOut.bOk;
}

bool CPPOcrBackend::runDet(const uint8_t *pucRgb, int32_t nW, int32_t nH,
                           std::vector<float> &rvecMap)
{
    /* pucRgb 与 m_spDetIn 必须是同一块：ocr_pipeline 拿它换算行框坐标，
       而送去推理的是缓冲本身。两处不是同一份就是"框是这张图、字是那张图" */
    if (!m_spDetIn || pucRgb != m_spDetIn->data() ||
        m_spDetIn->size() != static_cast<size_t>(nW) * static_cast<size_t>(nH) * CLASSROOM_OCR_RGB_BYTES_PER_PX)
    {
        return false;
    }
    return submitAndWaitInfer(CLASSROOM_MODEL_KEY_OCR_DET, m_spDetIn, rvecMap);
}

bool CPPOcrBackend::cropRow(const uint8_t *pucSrc, int32_t nSrcW, int32_t nSrcH,
                            const COcrBox_S &stBox, int32_t nCanvasW, int32_t nCanvasH,
                            int32_t nFilledW, COcrRowCanvas_S &rstOut)
{
    rstOut.pData = nullptr;
    rstOut.nBytes = 0;

    const size_t nNeed = static_cast<size_t>(nCanvasW) * static_cast<size_t>(nCanvasH) * CLASSROOM_OCR_RGB_BYTES_PER_PX;
    if (nCanvasW != m_nCanvasW || nCanvasH != m_nCanvasH || nFilledW < 4 ||
        nFilledW > nCanvasW || 0 != (nFilledW % 4) || !m_spDetIn ||
        pucSrc != m_spDetIn->data())
    {
        return false;   /* 编排层算出的宽度自相矛盾：这是编程错误，不是这一行裁不出来 */
    }
    if (!m_spRowBuf)
    {
        m_spRowBuf = std::make_shared<std::vector<uint8_t>>();
    }
    if (m_spRowBuf->size() != nNeed)
    {
        m_spRowBuf->resize(nNeed);
    }

    /* RGA 的四条对齐硬要求：坐标与裁剪尺寸取偶、目标宽 4 对齐。
       自己先取一遍偶而不是等它内部的 &~1：取完太小是"这框本来就采不了"，
       等它报回来只知道"裁不出"（与 crop_unit.cpp 同一处理） */
    const int32_t nCx = stBox.nX1 & ~1;
    const int32_t nCy = stBox.nY1 & ~1;
    int32_t nCw = (stBox.nX2 & ~1) - nCx;
    int32_t nCh = (stBox.nY2 & ~1) - nCy;
    if (nCx < 0 || nCy < 0 || nCx >= nSrcW || nCy >= nSrcH)
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        ++m_stBackendStat.u64RgaFail;
        return false;
    }
    if (nCx + nCw > nSrcW)
    {
        nCw = nSrcW - nCx;
    }
    if (nCy + nCh > nSrcH)
    {
        nCh = nSrcH - nCy;
    }
    nCw &= ~1;
    nCh &= ~1;
    if (nCw < 2 || nCh < 2)
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        ++m_stBackendStat.u64RgaFail;
        return false;
    }

    const bool bRgaOk = rga_image_transform(const_cast<uint8_t *>(pucSrc), nSrcW, nSrcH,
                                            OCR_RGA_FORMAT_RGB, m_spRowBuf->data(),
                                            nFilledW, nCanvasH, OCR_RGA_FORMAT_RGB,
                                            nCx, nCy, nCw, nCh, 0);
    if (!bRgaOk)
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        ++m_stBackendStat.u64RgaFail;
        return false;
    }
    /* RGA 直出的是密集的 nFilledW×nCanvasH；摊成画布宽并在右侧补 127 是这一步做的 */
    padRecCanvasRight(m_spRowBuf->data(), nCanvasW, nCanvasH, nFilledW, CLASSROOM_OCR_REC_PAD);
    rstOut.pData = m_spRowBuf->data();
    rstOut.nBytes = nNeed;
    return true;
}

bool CPPOcrBackend::runRec(const COcrRowCanvas_S &stCanvas, std::vector<float> &rvecOut)
{
    if (!m_spRowBuf || stCanvas.pData != m_spRowBuf->data() ||
        stCanvas.nBytes != m_spRowBuf->size())
    {
        return false;
    }
    return submitAndWaitInfer(CLASSROOM_MODEL_KEY_OCR_REC, m_spRowBuf, rvecOut);
}

bool CPPOcrBackend::submitAndWaitInfer(const std::string &strKey, const RgaBufPtr &rspBuf,
                                       std::vector<float> &rvecOut)
{
    rvecOut.clear();
    if (nullptr == m_pSched || !rspBuf || rspBuf->empty())
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        ++m_stBackendStat.u64SubmitFail;
        return false;
    }

    const uint32_t u32Seq = [&]
    {
        std::lock_guard<std::mutex> stLock(m_mtxInfer);
        m_bInferDone = false;
        m_bInferOk = false;
        return ++m_u32InferSeq;
    }();

    CInferReq_S stReq;
    stReq.strModelKey = strKey;
    stReq.spInput = rspBuf;
    stReq.u32InBytes = static_cast<uint32_t>(rspBuf->size());
    stReq.u32Seq = u32Seq;
    stReq.nPriority = m_stParam.nPriority;
    stReq.llSubmitMs = classroomNowMs();
    stReq.llDeadlineMs = stReq.llSubmitMs +
                         static_cast<int64_t>(m_stParam.nInferWaitMs) * OCR_DEADLINE_TIMES;
    stReq.fnDone = [this, u32Seq](const CInferResult_S &stRes)
    {
        std::lock_guard<std::mutex> stLock(m_mtxInfer);
        if (u32Seq != m_u32InferSeq)
        {
            return;     /* 超时放行之后迟到的旧回调：整份结果丢弃（同 CPerceivePipeline 那条） */
        }
        m_bInferOk = (stRes.bOk && nullptr != stRes.pvfOutput);
        m_enInferReason = stRes.enReason;
        if (m_bInferOk)
        {
            m_vecInferRaw = *stRes.pvfOutput;   /* 必须复制：指针指向 worker 的复用缓冲 */
        }
        else
        {
            m_vecInferRaw.clear();
        }
        m_bInferDone = true;
        m_cvInfer.notify_all();
    };

    if (!m_pSched->submit(std::move(stReq)))
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        ++m_stBackendStat.u64SubmitFail;
        return false;
    }

    bool bWaked = false;
    bool bOk = false;
    InferResultReason_E enReason = InferResultReason::IR_BACKEND_FAIL;
    {
        std::unique_lock<std::mutex> stLock(m_mtxInfer);
        bWaked = m_cvInfer.wait_for(stLock, std::chrono::milliseconds(m_stParam.nInferWaitMs),
                                    [this, u32Seq]
                                    { return m_bInferDone && u32Seq == m_u32InferSeq; });
        if (bWaked)
        {
            bOk = m_bInferOk;
            enReason = m_enInferReason;
            if (bOk)
            {
                rvecOut.swap(m_vecInferRaw);   /* 本线程独占，换一下省一次整幅拷贝 */
            }
        }
    }
    if (!bWaked)
    {
        /* 超时后**不清在途**：让迟到的回调自己写完那一格（下一次提交会重置 seq，它自然被丢掉）。
           清了下一次就可能把上一张图的结果当这一张用 */
        std::lock_guard<std::mutex> stLock(m_mtx);
        ++m_stBackendStat.u64InferTimeout;
        return false;
    }
    if (!bOk)
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (InferResultReason::IR_MODEL_UNREADY == enReason)
        {
            ++m_stBackendStat.u64InferUnready;
        }
        else
        {
            ++m_stBackendStat.u64InferFail;
        }
    }
    return bOk;
}

void CPPOcrBackend::workerLoop()
{
    /* 新线程入口第一句必须是它：comm 继承创建者，不设名就是在 144 条线程里认不出我们这条 */
    pthread_setname_np(pthread_self(), "ClsOcr");

    for (;;)
    {
        RgaBufPtr spRgb;
        int32_t nW = 0, nH = 0;
        int64_t llDueMs = 0;
        uint32_t u32Seq = 0;
        {
            std::unique_lock<std::mutex> stLock(m_mtx);
            m_cv.wait(stLock, [this] { return !m_bRunning || m_bJobIn; });
            if (!m_bRunning)
            {
                return;     /* 停止时不处理剩余作业：结果已经没人取了 */
            }
            /* 过期即丢（与调度器同一纪律）：黑板的内容会变，识别一张三分钟前的图
               等于往报表里写一条谁都对不上的记录 */
            if (classroomNowMs() - m_llJobDueMs > CLASSROOM_OCR_JOB_STALE_MS)
            {
                ++m_stBackendStat.u64JobStale;
                m_bJobIn = false;
                m_spJobRgb.reset();
                continue;
            }
            spRgb = m_spJobRgb;
            nW = m_nJobW;
            nH = m_nJobH;
            llDueMs = m_llJobDueMs;
            u32Seq = m_u32JobSeq;
            m_bJobIn = false;
            m_spJobRgb.reset();
        }

        COcrJobResult_S stRes;
        stRes.u32Seq = u32Seq;
        stRes.llDueMs = llDueMs;
        int64_t llCostMs = 0;
        /* recognizeBuf 自己持 m_mtxRun：与同步入口共用这一份实现，不会两线程同跑一张板子 */
        (void)recognizeBuf(spRgb, nW, nH, stRes, &llCostMs);

        std::lock_guard<std::mutex> stLock(m_mtx);
        if (!m_bRunning)
        {
            return;
        }
        m_stResult = std::move(stRes);
        m_bResultReady = true;
        ++m_stBackendStat.u64JobDone;
        if (m_stResult.bOk)
        {
            m_stBackendStat.llLastJobMs = llCostMs;
        }
    }
}
}
