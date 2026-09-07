/**
 * @FilePath     : detection_engine.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 11:49:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 15:08:20
 * @Description  : 统一检测引擎实现 + 帧准备器实现
 */

#include "detection_engine.hpp"

#include <algorithm>
#include <pthread.h>
#include <unistd.h>

#include "share_data.h"

/* 数据队列容量（与 LEGACY 算法一致，latest-wins） */
#define QUEUE_MAX (2)

CUnifiedDetectionEngine::CUnifiedDetectionEngine()
    : m_dateQueue(QUEUE_MAX)
{
}

CUnifiedDetectionEngine::~CUnifiedDetectionEngine()
{
    shutdown();
}

void CUnifiedDetectionEngine::recvMediaData(MediaData_S stMediaData)
{
    /* 无模型使能时不入队（与 LEGACY 算法未使能时直接丢弃语义一致） */
    if (!has_enabled_model())
    {
        return;
    }

    if (m_dateQueue.size() >= QUEUE_MAX)
    {
        dlog_warn("统一检测层-数据队列满，latest-wins 替换 [%d]", m_dateQueue.size());
    }
    m_dateQueue.pushOrReplace(stMediaData);
}

void CUnifiedDetectionEngine::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    /* 配置线程调用，转发给全部模型（模型内部有互斥保护） */
    for (auto &entry : m_vModels)
    {
        entry.pModel->setAlgoEnCfg(stAlgoConfig);
    }
}

void CUnifiedDetectionEngine::setEventStatisticsReporter(
    const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
    /* 配置线程调用，转发给全部模型（MD/OD 暂不使用，保留扩展位） */
    for (auto &entry : m_vModels)
    {
        entry.pModel->setEventStatisticsReporter(pReporter);
    }
}

int CUnifiedDetectionEngine::handleRuntimeCommand(const RuntimeCommand_S &stCommand)
{
    int nRet = ERR;
    for (auto &entry : m_vModels)
    {
        if (entry.pModel->handleRuntimeCommand(stCommand) == OK)
        {
            nRet = OK;
        }
    }
    return nRet;
}

void CUnifiedDetectionEngine::register_model(std::unique_ptr<IDetectionModel> pModel)
{
    if (m_bStarted.load())
    {
        dlog_error("统一检测层：Worker 已启动，禁止注册模型");
        return;
    }
    if (pModel == nullptr)
    {
        dlog_error("统一检测层：注册空模型");
        return;
    }

    ModelEntry_S entry;
    entry.pModel = std::move(pModel);
    m_vModels.emplace_back(std::move(entry));
}

void CUnifiedDetectionEngine::start()
{
    if (m_bStarted.exchange(true))
    {
        return;
    }

    /* 按模型 nOrder 升序稳定排序，保证同帧串行执行顺序确定 */
    std::stable_sort(m_vModels.begin(), m_vModels.end(),
                     [](const ModelEntry_S &stA, const ModelEntry_S &stB) {
                         return stA.pModel->descriptor().nOrder < stB.pModel->descriptor().nOrder;
                     });

    m_bRunning.store(true);
    m_thread = std::thread(&CUnifiedDetectionEngine::run, this);
}

void CUnifiedDetectionEngine::shutdown()
{
    if (!m_bStarted.load())
    {
        /* 从未启动：无 Worker 竞争，直接释放模型与帧缓冲 */
        m_vModels.clear();
        m_framePreparer.destroy();
        return;
    }

    if (!m_bRunning.exchange(false))
    {
        /* 已停止过，避免重复 join/unInit */
        return;
    }

    /* 唤醒可能阻塞在 pop 的 Worker */
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();

    /* Worker 已退出，反初始化全部模型（无竞争） */
    for (auto &entry : m_vModels)
    {
        if (entry.bInitialized)
        {
            entry.pModel->unInit();
            entry.bInitialized = false;
        }
    }
    m_framePreparer.destroy();
}

void CUnifiedDetectionEngine::run()
{
    pthread_setname_np(pthread_self(), "UnifiedDetect");

    MediaData_S stMediaData;

    /* 沿用 LEGACY 启动延时语义（MD 4s / OD 2s，防首次检测误报），
     * 取已注册模型的最大值；LEGACY 各自线程从启动即延时，
     * NEW 引擎在首个算法使能时创建，语义一致 */
    uint32_t nStartupDelayMs = 0;
    for (const auto &entry : m_vModels)
    {
        nStartupDelayMs = std::max(nStartupDelayMs, entry.pModel->descriptor().nStartupDelayMs);
    }
    if (nStartupDelayMs > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(nStartupDelayMs));
    }

    while (m_bRunning.load())
    {
        /* 无任何模型使能时休眠，避免空转 */
        if (!has_enabled_model())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        /* 懒初始化 + 配置变更重初始化：未就绪模型本帧跳过，不阻断已就绪模型 */
        for (auto &entry : m_vModels)
        {
            if (!entry.pModel->isEnabled())
            {
                continue;
            }

            if (entry.bInitialized && entry.pModel->needsReinit())
            {
                /* 句柄需要重建（区域宽高变化等），先反初始化 */
                entry.pModel->unInit();
                entry.bInitialized = false;
            }
            if (!entry.bInitialized)
            {
                if (entry.pModel->init() == OK)
                {
                    entry.bInitialized = true;
                    dlog_info("统一检测模型[%s]初始化成功", entry.pModel->descriptor().pName);
                }
                else
                {
                    /* 沿用 LEGACY 模式：初始化失败 1 秒后重试，期间只跳过该模型 */
                    dlog_error("等待[%s]初始化完成", entry.pModel->descriptor().pName);
                }
            }
        }

        /* 阻塞获取最新帧 */
        if (!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS) || stMediaData.pVideoFrameInfo == nullptr)
        {
            continue;
        }

        /* 本帧周期开始：失效帧准备缓存，同帧内相同准备参数复用 */
        m_framePreparer.begin_frame();

        /* 按注册顺序（nOrder 升序）串行分发到已使能模型 */
        for (auto &entry : m_vModels)
        {
            if (!entry.pModel->isEnabled() || !entry.bInitialized)
            {
                continue;
            }

            const DetectionModelDescriptor_S &stDesc = entry.pModel->descriptor();
            /* 模型自带帧率控制，返回 false 跳过本帧 */
            if (!entry.pModel->shouldProcess(stMediaData.stMediaParam.nChannel))
            {
                continue;
            }

            /* 裁剪区域快照：模型内部状态由配置线程修改，快照保证 prepare 期间一致 */
            const Common::Rect_S *pCropRect = entry.pModel->crop_rect();
            Common::Rect_S stCropSnapshot;
            if (pCropRect != nullptr)
            {
                stCropSnapshot = *pCropRect;
                pCropRect = &stCropSnapshot;
            }

            ot_video_frame_info *pPreparedFrame = nullptr;
            if (m_framePreparer.prepare(stMediaData.pVideoFrameInfo.get(),
                                        stDesc.nInputWidth, stDesc.nInputHeight,
                                        pCropRect, &pPreparedFrame) != OK)
            {
                /* 帧准备失败：跳过该模型本帧，不影响其他模型；
                 * 依赖 dlog 全局限流防止帧级重复警告刷屏 */
                dlog_warn("统一检测模型[%s]帧准备失败，本帧跳过", stDesc.pName);
                continue;
            }

            if (entry.pModel->process(pPreparedFrame, stMediaData) != OK)
            {
                dlog_warn("统一检测模型[%s]处理失败，本帧跳过", stDesc.pName);
            }
        }
    }
}

bool CUnifiedDetectionEngine::has_enabled_model() const
{
    for (const auto &entry : m_vModels)
    {
        if (entry.pModel->isEnabled())
        {
            return true;
        }
    }
    return false;
}

// ==================== CFramePreparer 实现（合并） ====================

// merged: frame_preparer

#include "dlog.h"
#include "securec.h"

CFramePreparer::~CFramePreparer()
{
    destroy();
}

void CFramePreparer::begin_frame()
{
    /* 本帧缓存失效：同一帧周期内相同准备参数复用，帧周期结束自动失效 */
    m_bFrameCacheValid = false;
    m_nFrameCacheDstW = 0;
    m_nFrameCacheDstH = 0;
    m_bFrameCacheHasCrop = false;
    m_stFrameCacheCropRect = Common::Rect_S();
    m_pFrameCacheResult = nullptr;

    /* 缩放缓存仅对本帧源帧有效，帧周期结束失效 */
    m_bScaleCacheValid = false;
    m_pScaleCacheSrc = nullptr;
    m_nScaleCacheDstW = 0;
    m_nScaleCacheDstH = 0;
    m_pScaleCacheResult = nullptr;
}

int CFramePreparer::prepare(const ot_video_frame_info *pSrcFrame, uint32_t nDstW, uint32_t nDstH,
                            const Common::Rect_S *pCropRect, ot_video_frame_info **ppOut)
{
    if (pSrcFrame == nullptr || ppOut == nullptr || nDstW == 0 || nDstH == 0)
    {
        return ERR_PARAM;
    }

    /* 同帧同参数复用已准备结果 */
    if (m_bFrameCacheValid &&
        m_nFrameCacheDstW == nDstW && m_nFrameCacheDstH == nDstH &&
        m_bFrameCacheHasCrop == (pCropRect != nullptr) &&
        (!pCropRect ||
         (m_stFrameCacheCropRect.nX == pCropRect->nX &&
          m_stFrameCacheCropRect.nY == pCropRect->nY &&
          m_stFrameCacheCropRect.nWidth == pCropRect->nWidth &&
          m_stFrameCacheCropRect.nHeight == pCropRect->nHeight)))
    {
        *ppOut = m_pFrameCacheResult;
        return OK;
    }

    const int nSrcWidth = pSrcFrame->video_frame.width;
    const int nSrcHeight = pSrcFrame->video_frame.height;

    /* 先缩放：源帧尺寸与目标算法分辨率不一致时才执行（与 LEGACY 语义一致） */
    ot_video_frame_info *pPrepared = const_cast<ot_video_frame_info *>(pSrcFrame);
    if (static_cast<uint32_t>(nSrcWidth) != nDstW || static_cast<uint32_t>(nSrcHeight) != nDstH)
    {
        /* perf: 同帧内多个模型使用相同缩放参数时复用缩放结果，避免重复 VGS 缩放 */
        if (m_bScaleCacheValid && m_pScaleCacheSrc == pSrcFrame &&
            m_nScaleCacheDstW == nDstW && m_nScaleCacheDstH == nDstH)
        {
            pPrepared = m_pScaleCacheResult;
        }
        else
        {
            ot_video_frame_info *pScaleBuf = get_scale_buffer(nDstW, nDstH);
            if (pScaleBuf == nullptr)
            {
                dlog_error("统一检测层创建缩放帧失败 [%ux%u]", nDstW, nDstH);
                return ERR_CREATE;
            }
            if (TD_SUCCESS != mppVgs_scale(const_cast<ot_video_frame_info *>(pSrcFrame), pScaleBuf))
            {
                dlog_error("统一检测层缩放失败: [%dx%d] -> [%ux%u]", nSrcWidth, nSrcHeight, nDstW, nDstH);
                return ERR;
            }
            pPrepared = pScaleBuf;
            m_bScaleCacheValid = true;
            m_pScaleCacheSrc = pSrcFrame;
            m_nScaleCacheDstW = nDstW;
            m_nScaleCacheDstH = nDstH;
            m_pScaleCacheResult = pPrepared;
        }
    }

    /* 再裁剪：裁剪坐标基于缩放后的算法坐标系（与 LEGACY MD/OD 语义一致） */
    if (pCropRect != nullptr && !pCropRect->isEmpty())
    {
        ot_video_frame_info *pCropBuf =
            get_crop_buffer(static_cast<uint32_t>(pCropRect->nWidth),
                            static_cast<uint32_t>(pCropRect->nHeight));
        if (pCropBuf == nullptr)
        {
            dlog_error("统一检测层创建裁剪帧失败 [%dx%d]", pCropRect->nWidth, pCropRect->nHeight);
            return ERR_CREATE;
        }

        ot_rect stRect;
        stRect.x = pCropRect->nX;
        stRect.y = pCropRect->nY;
        stRect.width = pCropRect->nWidth;
        stRect.height = pCropRect->nHeight;
        if (TD_SUCCESS != mppVgs_crop(pPrepared, pCropBuf, &stRect))
        {
            dlog_error("统一检测层裁剪失败: rect[%d,%d,%d,%d]",
                       pCropRect->nX, pCropRect->nY, pCropRect->nWidth, pCropRect->nHeight);
            return ERR;
        }
        pPrepared = pCropBuf;
    }

    *ppOut = pPrepared;

    /* 记录本帧完整准备参数，供同帧复用 */
    m_bFrameCacheValid = true;
    m_nFrameCacheDstW = nDstW;
    m_nFrameCacheDstH = nDstH;
    m_bFrameCacheHasCrop = (pCropRect != nullptr);
    if (pCropRect != nullptr)
    {
        m_stFrameCacheCropRect = *pCropRect;
    }
    m_pFrameCacheResult = pPrepared;
    return OK;
}

void CFramePreparer::destroy()
{
    /* memory: 统一释放全部缩放/裁剪缓冲 */
    for (auto &entry : m_mapScaleBuffers)
    {
        if (entry.second != nullptr)
        {
            mppVgs_destroy_video_frame_info(entry.second.get());
        }
    }
    m_mapScaleBuffers.clear();

    for (auto &entry : m_mapCropBuffers)
    {
        if (entry.second != nullptr)
        {
            mppVgs_destroy_video_frame_info(entry.second.get());
        }
    }
    m_mapCropBuffers.clear();

    begin_frame();
}

ot_video_frame_info *CFramePreparer::get_scale_buffer(uint32_t nWidth, uint32_t nHeight)
{
    return acquire_buffer(m_mapScaleBuffers, nWidth, nHeight);
}

ot_video_frame_info *CFramePreparer::get_crop_buffer(uint32_t nWidth, uint32_t nHeight)
{
    return acquire_buffer(m_mapCropBuffers, nWidth, nHeight);
}

ot_video_frame_info *CFramePreparer::acquire_buffer(
    std::unordered_map<uint64_t, std::unique_ptr<ot_video_frame_info>> &mapBuffers,
    uint32_t nWidth, uint32_t nHeight)
{
    const uint64_t u64Key = (static_cast<uint64_t>(nWidth) << 32) | static_cast<uint64_t>(nHeight);

    auto it = mapBuffers.find(u64Key);
    if (it != mapBuffers.end())
    {
        return it->second.get();
    }

    /* 懒创建：缓冲按 (宽,高) 维度复用，避免 MD/OD 区域尺寸交替时每帧重建 VB */
    auto pFrameInfo = std::make_unique<ot_video_frame_info>();
    memset_s(pFrameInfo.get(), sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
    if (TD_SUCCESS != mppVgs_create_video_frame_info(nWidth, nHeight,
                                                     OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                     pFrameInfo.get()))
    {
        dlog_error("统一检测层创建视频帧失败 [%ux%u]", nWidth, nHeight);
        return nullptr;
    }

    ot_video_frame_info *pResult = pFrameInfo.get();
    mapBuffers.emplace(u64Key, std::move(pFrameInfo));
    return pResult;
}