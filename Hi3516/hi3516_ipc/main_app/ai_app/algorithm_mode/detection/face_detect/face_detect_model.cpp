/**
 * @FilePath     : face_detect_model.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : 人脸检测统一检测层模型实现。
 */

#include "face_detect_model.hpp"

#include <string.h>

#include "dlog.h"
#include "IpcRet.h"
#include "cpu_sharpness_scorer.h"
#include "time_utils.h"

/* 检测分辨率（与模型输入一致，640x640）与异步帧池容量（对齐 Legacy） */
namespace
{
constexpr int FACE_DETECT_INPUT_WIDTH = 640;
constexpr int FACE_DETECT_INPUT_HEIGHT = 640;
#if CAP_IO_EXTERNAL_DDR_00S
constexpr size_t FACE_ASYNC_FRAME_POOL_SIZE = 5;
#else
constexpr size_t FACE_ASYNC_FRAME_POOL_SIZE = 3;
#endif
} // namespace

/* 模型描述符：检测输入 640x640，无裁剪区域，无启动延时 */
static const DetectionModelDescriptor_S s_faceDetectDescriptor = {
    "FaceDetect",
    FACE_DETECT_INPUT_WIDTH,
    FACE_DETECT_INPUT_HEIGHT,
    false, /* bNeedCrop */
    0,     /* nOrder：独立引擎实例，无同帧串行约束 */
    0,     /* nStartupDelayMs：无参考帧建立延时 */
};

CFaceDetectModel::~CFaceDetectModel()
{
    /* 引擎 shutdown 保证 unInit 已执行，此处兜底 */
    if (m_bAsyncFramePoolInitialized)
    {
        deinitAsyncFramePool();
    }
}

const DetectionModelDescriptor_S &CFaceDetectModel::descriptor() const
{
    return s_faceDetectDescriptor;
}

bool CFaceDetectModel::isEnabled() const
{
    return m_bAlgorithmEnabled.load();
}

bool CFaceDetectModel::shouldProcess(int nChannelId)
{
    /* 检测节流：固定 200ms（5fps，取优高频评分），抓拍间隔/落盘频率由事件状态机冷却控制 */
    return m_RecvManager.handleEvent(nChannelId);
}

int CFaceDetectModel::process(const ot_video_frame_info *pPreparedFrame, const MediaData_S &stMediaData)
{
    if (pPreparedFrame == nullptr)
    {
        return ERR_PARAM;
    }

    /*
     * 原始全分辨率帧：抓拍裁剪源（batch_crop 硬编码要求帧等于编码通道尺寸）
     * 与比对特征提取使用；异步回调期间经 stMediaData 的 shared_ptr 保活。
     */
    ot_video_frame_info *pSrcFrameInfo = stMediaData.pVideoFrameInfo.get();
    if (pSrcFrameInfo == nullptr)
    {
        return ERR_PARAM;
    }

    /* 引擎 prepare 帧缓冲跨帧复用，异步推理必须持有独立拷贝（对齐 Legacy 设计） */
    ot_video_frame_info *pAsyncFrame = acquireAsyncFrame();
    if (pAsyncFrame == nullptr)
    {
        dlog_warn("人脸异步VB帧池暂无空闲帧，丢弃当前输入帧");
        return ERR;
    }
    const size_t szFrameSize = static_cast<size_t>(FACE_DETECT_INPUT_WIDTH) * FACE_DETECT_INPUT_HEIGHT * 3 / 2;
    memcpy(pAsyncFrame->video_frame.virt_addr[0], pPreparedFrame->video_frame.virt_addr[0], szFrameSize);

    /*
     * 投递异步检测任务；回调在 Worker 线程执行（Legacy 编排原样迁移）：
     * 跟踪过滤 -> 域类型目标 -> 抓拍处理器 -> 比对处理器 -> OSD。
     */
    m_detectWorker.submitVideoFrame(
        pAsyncFrame,
        FACE_DETECT_INPUT_WIDTH,
        FACE_DETECT_INPUT_HEIGHT,
        [this, pAsyncFrame, stMediaData, pSrcFrameInfo](std::vector<Inference_NS::BoxData_S> vPointDatas)
        {
            /* 邮件附件列表 */
            std::vector<std::string> vecImageFile;
            /* OSD 角框（规则通过目标框，抓拍处理器输出） */
            std::vector<Common::RectInfo_S> vstRectInfo;

            /*
             * 目标跟踪：仅 TRACKING 稳定目标进入事件/取优/比对（NEW/PREDICTED 过滤），
             * nUserData 承载原始检测下标以取回完整检测数据。
             */
            std::vector<DetectionTracker_NS::DetectionInput_S> vecInputs;
            vecInputs.reserve(vPointDatas.size());
            for (size_t i = 0; i < vPointDatas.size(); i++)
            {
                const auto &stBox = vPointDatas[i].stBoxs;
                vecInputs.push_back({ stBox.nX1,
                                      stBox.nY1,
                                      stBox.nX2 - stBox.nX1,
                                      stBox.nY2 - stBox.nY1,
                                      0,
                                      vPointDatas[i].fConfidence,
                                      static_cast<int>(i) });
            }
            std::vector<Inference_NS::BoxData_S> vStableDatas;
            std::vector<int> vTrackIds;
            const auto vecTracked = m_tracker.update(vecInputs);
            for (const auto &stTarget : vecTracked)
            {
                if (stTarget.enState != DetectionTracker_NS::TrackState_E::TRACKING)
                {
                    continue;
                }
                vStableDatas.push_back(vPointDatas[stTarget.nUserData]);
                vTrackIds.push_back(stTarget.nTrackId);
            }

            /*
             * 离开的人脸交给抓拍处理器判断怎么处理：窗口进行中就把最佳图留到
             * 窗口结束一起出图，平时则立即清掉保存的最佳图。抓拍没开（只开
             * 比对）时没有窗口概念，直接清掉即可。
             */
            std::vector<int> vEndedTrackIds;
            for (const auto &stEnded : m_tracker.take_ended())
            {
                vEndedTrackIds.push_back(stEnded.nTrackId);
            }
            if (!vEndedTrackIds.empty())
            {
                if (m_captureProcessor.isEnabled())
                {
                    m_captureProcessor.onTracksEnded(vEndedTrackIds, m_bestshotHub);
                }
                else
                {
                    for (const int nTrackId : vEndedTrackIds)
                    {
                        m_bestshotHub.endTarget(nTrackId);
                    }
                }
            }

            /*
             * 域类型目标（face_chain_types.hpp）：稳定目标全集，跟踪 id 随目标流转。
             * 抓拍规则过滤在抓拍处理器内部（事件私有判定）；比对目标集不经过
             * 抓拍规则过滤（与 Legacy 语义一致）。
             */
            std::vector<FaceChain_NS::FaceTarget_S> vStableTargets;
            vStableTargets.reserve(vStableDatas.size());
            for (size_t i = 0; i < vStableDatas.size(); i++)
            {
                FaceChain_NS::FaceTarget_S stTarget;
                stTarget.stRect.nX1 = vStableDatas[i].stBoxs.nX1;
                stTarget.stRect.nY1 = vStableDatas[i].stBoxs.nY1;
                stTarget.stRect.nX2 = vStableDatas[i].stBoxs.nX2;
                stTarget.stRect.nY2 = vStableDatas[i].stBoxs.nY2;
                stTarget.fConfidence = vStableDatas[i].fConfidence;
                stTarget.nTrackId = (i < vTrackIds.size()) ? vTrackIds[i] : -1;
                vStableTargets.emplace_back(stTarget);
            }

            /*
             * 单帧上下文（纯数据）：原始全分辨率帧为裁剪/特征源，检测空间
             * 坐标由处理器在裁剪前按源帧分辨率做逆变换。
             */
            FaceChain_NS::FaceFrameContext_S stFrameCtx;
            stFrameCtx.pSrcFrameInfo = pSrcFrameInfo;
            stFrameCtx.nChnId = stMediaData.stMediaParam.nChannel;
            stFrameCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stFrameCtx.nDetectWidth = FACE_DETECT_INPUT_WIDTH;
            stFrameCtx.nDetectHeight = FACE_DETECT_INPUT_HEIGHT;

            /* 人脸抓拍（规则过滤与取优喂入在处理器内部，与既有语义一致） */
            if (m_captureProcessor.isEnabled())
            {
                m_captureProcessor.process(stFrameCtx, vStableTargets, m_bestshotHub, vstRectInfo, vecImageFile);
            }

#if CAP_AI_FACE_COMPARE
            /*
             * 抓拍关、只开比对：抓拍处理器不跑、没人喂帧，选优由编排侧驱动——
             * 按比对灵敏度阈值过滤后的稳定目标逐个喂入，裁剪框走抓拍同款
             * 检测空间→源帧映射；亮度按未知（-1）评选，选优退化为清晰度+
             * 置信度+面积三维评分（无 VGS 亮度查询）。抓拍开启时喂入仍由抓拍
             * 处理器内部完成（含亮度），此处不重复喂。
             */
            if (m_compareProcessor.isEnabled() && !m_captureProcessor.isEnabled() && pSrcFrameInfo != nullptr)
            {
                const int nFrameWidth = static_cast<int>(pSrcFrameInfo->video_frame.width);
                const int nFrameHeight = static_cast<int>(pSrcFrameInfo->video_frame.height);
                for (const auto &stTarget : vStableTargets)
                {
                    if (stTarget.fConfidence < m_compareProcessor.sensitivityThreshold())
                    {
                        continue;
                    }
                    Common::RectInfo_S stRect = stTarget.stRect;
                    ot_rect stCrop;
                    if (!FaceDetectInternal::CFaceCaptureProcessor::alignCropRect(stRect,
                                                                                  FACE_DETECT_INPUT_WIDTH,
                                                                                  FACE_DETECT_INPUT_HEIGHT,
                                                                                  nFrameWidth,
                                                                                  nFrameHeight,
                                                                                  stCrop))
                    {
                        /* 过小/出界目标无有效裁剪框，不参与选优（也无特征可提） */
                        continue;
                    }
                    m_bestshotHub.feed(stTarget.nTrackId, pSrcFrameInfo, stCrop, stTarget.fConfidence, stFrameCtx.llTimestamp);
                }
            }

            /* 人脸比对（原生实现）：目标为稳定目标全集，图片缓存与抓拍共享 */
            if (m_compareProcessor.isEnabled())
            {
                m_compareProcessor.process(stFrameCtx, vStableTargets, vecImageFile);
            }
#endif
            /* OSD 框显示：与 Legacy 一致（规则通过目标框、默认有效期） */
            if (m_captureProcessor.isEnabled() && !vstRectInfo.empty())
            {
                send_detectionResult_to_osd(FACE_DETECT_INPUT_WIDTH, FACE_DETECT_INPUT_HEIGHT, vstRectInfo);
            }
        },
        [this, pAsyncFrame]()
        {
            releaseAsyncFrame(pAsyncFrame);
        });
    return OK;
}

int CFaceDetectModel::init()
{
    /*
     * 取优枢纽装配（评分后端唯一注入点）：注释掉 m_bestshotHub.init 即整体
     * 关闭取优，抓拍自动回退当帧编码出图，事件/联动逻辑不受影响。
     * IVE 硬件后端通路见 capture/ive_sharpness_adapter（板端实测 IVE 与 NPU
     * 硬件级冲突未定位，默认不装配，仅保留切换通路）。
     */
    m_bestshotHub.registerListener(&m_captureProcessor);
#if CAP_AI_FACE_COMPARE
    /* 比对处理器订阅取优（新 best 时提特征缓存），注册在枢纽 init 之前 */
    m_bestshotHub.registerListener(&m_compareProcessor);
#endif
    Bestshot_NS::CCpuSharpnessScorer::Config_S stScorerCfg;
    stScorerCfg.nGradThreshold = 30;
    stScorerCfg.unMaxRoiSide = 512;
    m_bestshotHub.init(std::unique_ptr<Bestshot_NS::CCpuSharpnessScorer>(new Bestshot_NS::CCpuSharpnessScorer(stScorerCfg)));

    /* 异步 VB 帧池 */
    if (!initAsyncFramePool())
    {
        return ERR;
    }
#if CAP_AI_FACE_COMPARE
    /* 比对处理器：注入推理 Worker（特征提取执行器），时序早于首次 process */
    m_compareProcessor.attachWorker(&m_detectWorker);
    /* 特征模型（抓拍规则使能比对时已置位） */
    if (m_compareProcessor.isEnabled() && !m_compareProcessor.isInitialized() && !m_compareProcessor.init())
    {
        dlog_error("人脸特征模型初始化失败");
        return ERR;
    }
#endif
    /* 推理 Worker：检测与特征任务内部串行 */
    if (!m_detectWorker.start())
    {
        dlog_error("人脸检测 Worker 启动失败");
        deinitAsyncFramePool();
        return ERR;
    }
    return OK;
}

void CFaceDetectModel::unInit()
{
    /*
     * Worker 可能仍持有异步帧池中的帧：先停止并等待归还全部帧，
     * 再清空 VB 帧池（对齐 Legacy 关闭顺序）；最后释放取优枢纽
     * （扇出清理抓拍的 best 缓存）与比对处理器。
     */
    m_detectWorker.deinit();
    deinitAsyncFramePool();
    m_bestshotHub.deinit();
#if CAP_AI_FACE_COMPARE
    m_compareProcessor.deinit();
#endif
}

void CFaceDetectModel::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    const bool bEnableFaceCapture = stAlgoConfig.nEnFaceCapture;
    const bool bEnableFaceCompare = stAlgoConfig.nEnFaceCompare;

    if (bEnableFaceCapture || bEnableFaceCompare)
    {
        /* 人脸比对没有独立规则配置，始终加载抓拍规则作为前置过滤条件 */
        Alarm::FaceCapture_S stCaptureInfo;
        CEventConfigure::instance()->get_configure(stCaptureInfo);
        m_stAlgoFaceCapCfg = stCaptureInfo;
        m_captureProcessor.setAlgoParamCfg(stCaptureInfo, FACE_DETECT_INPUT_WIDTH, FACE_DETECT_INPUT_HEIGHT);
#if CAP_AI_FACE_COMPARE
        /* 比对目标入口灵敏度门限与抓拍同源（检测低阈放行低分框后，比对按抓拍灵敏度阈值过滤） */
        m_compareProcessor.setSensitivityThreshold(stCaptureInfo.stRule.nSensitivity);
#endif
    }
    else
    {
        m_stAlgoFaceCapCfg.bEnable = false;
    }
    m_stAlgoFaceCapCfg.bEnable = m_stAlgoFaceCapCfg.bEnable && bEnableFaceCapture;
    m_captureProcessor.setEnabled(m_stAlgoFaceCapCfg.bEnable);

#if CAP_AI_FACE_COMPARE
    if (bEnableFaceCompare)
    {
        Alarm::FaceCompare_S stCompareInfo;
        CEventConfigure::instance()->get_configure(stCompareInfo);
        m_compareProcessor.setAlgoParamCfg(stCompareInfo);
        m_compareProcessor.setEnabled(true);
    }
    else
    {
        m_compareProcessor.setEnabled(false);
    }
#endif
    /*
     * 任一人脸开关开启（抓拍/比对）即视为算法使能：引擎懒 init 时 Worker 启动，
     * 双模型（YOLO+ArcFace）无条件同时初始化（对齐旧链路），运行中后开比对
     * 无需补加载；比对画像固件必带双模型文件（入库也依赖），缺失属打包错误。
     */
    m_bAlgorithmEnabled.store(m_captureProcessor.isEnabled()
#if CAP_AI_FACE_COMPARE
                              || m_compareProcessor.isEnabled()
#endif
    );
}

bool CFaceDetectModel::initAsyncFramePool()
{
    std::lock_guard<std::mutex> lock(m_asyncFramePoolMutex);
    if (m_bAsyncFramePoolInitialized)
    {
        return true;
    }
    while (!m_availableAsyncFrames.empty())
    {
        m_availableAsyncFrames.pop();
    }
    size_t nCreated = 0;
    for (; nCreated < FACE_ASYNC_FRAME_POOL_SIZE; ++nCreated)
    {
        auto &stFrame = m_astAsyncFrames[nCreated];
        memset(&stFrame, 0, sizeof(stFrame));
        if (TD_SUCCESS !=
            mppVgs_create_video_frame_info(FACE_DETECT_INPUT_WIDTH, FACE_DETECT_INPUT_HEIGHT, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &stFrame))
        {
            dlog_error("初始化人脸异步VB帧池失败，已创建[%zu/%zu]", nCreated, FACE_ASYNC_FRAME_POOL_SIZE);
            for (size_t i = 0; i < nCreated; ++i)
            {
                mppVgs_destroy_video_frame_info(&m_astAsyncFrames[i]);
                memset(&m_astAsyncFrames[i], 0, sizeof(m_astAsyncFrames[i]));
            }
            return false;
        }
        m_availableAsyncFrames.push(&stFrame);
    }
    m_bAsyncFramePoolInitialized = true;
    dlog_info("人脸异步VB帧池初始化成功，帧数[%zu]，分辨率[%dx%d]",
              m_astAsyncFrames.size(),
              FACE_DETECT_INPUT_WIDTH,
              FACE_DETECT_INPUT_HEIGHT);
    return true;
}

void CFaceDetectModel::deinitAsyncFramePool()
{
    std::lock_guard<std::mutex> lock(m_asyncFramePoolMutex);
    if (!m_bAsyncFramePoolInitialized)
    {
        return;
    }
    while (!m_availableAsyncFrames.empty())
    {
        m_availableAsyncFrames.pop();
    }
    for (size_t i = 0; i < FACE_ASYNC_FRAME_POOL_SIZE; ++i)
    {
        mppVgs_destroy_video_frame_info(&m_astAsyncFrames[i]);
        memset(&m_astAsyncFrames[i], 0, sizeof(m_astAsyncFrames[i]));
    }
    m_bAsyncFramePoolInitialized = false;
    dlog_info("人脸异步VB帧池已释放");
}

ot_video_frame_info *CFaceDetectModel::acquireAsyncFrame()
{
    std::lock_guard<std::mutex> lock(m_asyncFramePoolMutex);
    if (!m_bAsyncFramePoolInitialized || m_availableAsyncFrames.empty())
    {
        return nullptr;
    }
    ot_video_frame_info *pFrameInfo = m_availableAsyncFrames.front();
    m_availableAsyncFrames.pop();
    return pFrameInfo;
}

void CFaceDetectModel::releaseAsyncFrame(ot_video_frame_info *pFrameInfo)
{
    if (pFrameInfo == nullptr)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_asyncFramePoolMutex);
    if (m_bAsyncFramePoolInitialized)
    {
        m_availableAsyncFrames.push(pFrameInfo);
    }
}
