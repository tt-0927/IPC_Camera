/**
 * @FilePath     : garbage_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-12 09:07:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-15 10:45:51
 * @Description  : 垃圾暴露/垃圾满溢检测
 */

#include "garbage_detect.hpp"
#include "video_frame_jpeg_encoder.hpp"

#if CAP_AI_GARBAGE_DETECT

#include <chrono>
#include <unistd.h>

/* 数据队列 */
#define QUEUE_MAX (2)
/* 垃圾满溢标签 */
#define GARBAGE_OVERFLOW_LABEL_ID (0)
/* 垃圾暴露标签 */
#define GARBAGE_EXPOSURE_LABEL_ID (1)

CGarbageDetect::CGarbageDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CGarbageDetect::run, this);
}

CGarbageDetect::~CGarbageDetect()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    /* 调用 shutdown() 来唤醒可能阻塞在 pop() 的线程 */
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    unInit();
}

void CGarbageDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!isEnabled())
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("垃圾检测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CGarbageDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoGarbageExposureCfg.bEnable = stAlgoConfig.nEnGarbageExposure;
    m_stAlgoGarbageOverflowCfg.bEnable = stAlgoConfig.nEnGarbageOverflow;

    if (m_stAlgoGarbageExposureCfg.bEnable)
    {
        Alarm::GarbageExposureDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoGarbageOverflowCfg.bEnable)
    {
        Alarm::GarbageOverflowDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CGarbageDetect::setAlgoParamCfg(const Alarm::GarbageExposureDetection_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置垃圾暴露检测参数");
    m_stAlgoGarbageExposureCfg = stAlgoCfg;

    auto &region = m_stAlgoGarbageExposureCfg.stRule.stRegion;
    region.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
    if (!region.IsValid())
    {
        dlog_warn("垃圾暴露检测-检测区域无效，关闭该算法分析");
        m_stAlgoGarbageExposureCfg.bEnable = false;
    }
}

void CGarbageDetect::setAlgoParamCfg(const Alarm::GarbageOverflowDetection_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置垃圾满溢检测参数");
    m_stAlgoGarbageOverflowCfg = stAlgoCfg;

    auto &region = m_stAlgoGarbageOverflowCfg.stRule.stRegion;
    region.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
    if (!region.IsValid())
    {
        dlog_warn("垃圾满溢检测-检测区域无效，关闭该算法分析");
        m_stAlgoGarbageOverflowCfg.bEnable = false;
    }
}

bool CGarbageDetect::isEnabled() const
{
    return m_stAlgoGarbageExposureCfg.bEnable || m_stAlgoGarbageOverflowCfg.bEnable;
}

bool CGarbageDetect::waitForHandle()
{
    if (m_pGarbageDetHandle)
    {
        return true;
    }

    if (!isEnabled())
    {
        sleep(1);
        return false;
    }

    if (init())
    {
        return true;
    }

    dlog_error("等待垃圾检测初始化");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return false;
}

bool CGarbageDetect::init()
{
    if (!m_pGarbageDetHandle)
    {
        std::string strModelPath = AI_GARBAGE_DETECTION_CONFIG_FILE;
        m_pGarbageDetHandle = new Inference_NS::CYoloUltralytics(strModelPath);
        if (m_pGarbageDetHandle)
        {
            if (m_pGarbageDetHandle->init())
            {
                dlog_info("垃圾检测初始化成功, %s", strModelPath.c_str());
                memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
                /* 创建目标视频帧 */
                if (TD_SUCCESS != mppVgs_create_video_frame_info(m_nWidth,
                                                                 m_nHeight,
                                                                 OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                                 &m_stDstFrameInfo))
                {
                    delete m_pGarbageDetHandle;
                    m_pGarbageDetHandle = nullptr;
                    dlog_error("垃圾检测初始化失败-创建目标视频帧失败");
                    return false;
                }
                return true;
            }

            delete m_pGarbageDetHandle;
            m_pGarbageDetHandle = nullptr;
            dlog_error("垃圾检测初始化失败");
        }
    }

    return false;
}

bool CGarbageDetect::unInit()
{
    /* 销毁目标视频帧 */
    mppVgs_destroy_video_frame_info(&m_stDstFrameInfo);

    if (m_pGarbageDetHandle)
    {
        delete m_pGarbageDetHandle;
        m_pGarbageDetHandle = nullptr;
    }

    return true;
}

ot_video_frame_info *CGarbageDetect::getFrameForInference(MediaData_S &stMediaData)
{
    /* 直接使用原始视频帧，避免不必要的内存拷贝 */
    ot_video_frame_info *pSrcFrameInfo = stMediaData.pVideoFrameInfo.get();
    if (!pSrcFrameInfo)
    {
        dlog_error("原始数据帧为空");
        return nullptr;
    }

    if (m_nWidth == stMediaData.stMediaParam.nVideoWidth && m_nHeight == stMediaData.stMediaParam.nVideoHeight)
    {
        return pSrcFrameInfo;
    }

    /* VGS scale 缩放 */
    if (TD_SUCCESS != mppVgs_scale(pSrcFrameInfo, &m_stDstFrameInfo))
    {
        return nullptr;
    }

    return &m_stDstFrameInfo;
}

bool CGarbageDetect::inferFrame(ot_video_frame_info *pFrameInfo, std::vector<Inference_NS::BoxData_S> &vBoxDatas)
{
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float *)pFrameInfo->video_frame.virt_addr[0];
    stInputData.nDataSize = static_cast<int>(m_nWidth * m_nHeight * 1.5) * sizeof(float);
    return m_pGarbageDetHandle && m_pGarbageDetHandle->inference(stInputData, vBoxDatas);
}

void CGarbageDetect::run()
{
    MediaData_S stMediaData;
    pthread_setname_np(pthread_self(), "GarbageDetect");

    while (m_bRunning.load())
    {
        if (!waitForHandle() || !m_bRunning.load())
        {
            continue;
        }

        /* 阻塞获取 */
        if (!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS) || stMediaData.pVideoFrameInfo == nullptr)
        {
            continue;
        }

        ot_video_frame_info *pFrameInfo = getFrameForInference(stMediaData);
        if (!pFrameInfo)
        {
            continue;
        }

        std::vector<Inference_NS::BoxData_S> vBoxDatas;
        const bool bInferOk = inferFrame(pFrameInfo, vBoxDatas);

        /* 手动抓拍请求优先处理：不受事件使能判断影响，处理完再走正常事件流程。 */
        if (m_bSnapshotPending.load())
        {
            dlog_info("垃圾站抓图识别-算法层: 流式线程收到快照请求，开始处理该帧 推理[%s]",
                      bInferOk ? "成功" : "失败");
            handleSnapshotRequest(bInferOk ? pFrameInfo : nullptr, vBoxDatas);
        }

        if (!bInferOk || !isEnabled())
        {
            continue;
        }

        /* OSD 动态分析显示数组 */
        std::vector<Common::RectInfo_S> vstRectInfo;
        SEventProcessContext stCtx;
        stCtx.nChnId = stMediaData.stMediaParam.nChannel;
        stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
        stCtx.pFrameInfo = pFrameInfo;
        /* 垃圾检测后处理函数 */
        processGarbageDetect(vBoxDatas, vstRectInfo, stCtx);
        if (!vstRectInfo.empty())
        {
            /* 发送结果至 OSD 模块，进行框选显示 */
            send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
        }
    }
}

void CGarbageDetect::processGarbageDetect(std::vector<Inference_NS::BoxData_S> &vBoxDatas,
                                          std::vector<Common::RectInfo_S> &vstRectInfo,
                                          const SEventProcessContext &stCtx)
{
    bool bGarbageOverflow = false;
    bool bGarbageExposure = false;

    printResult(vBoxDatas);
    /* 检测到对象的数量: 1
    对象: 1:
    类型ID: 0
    置信度: 0.915527
    边界框: (x1 = 231, y1 = 82, x2 = 344, y2 = 274)

    检测到对象的数量: 1
    对象: 1:
    类型ID: 0
    置信度: 0.884766
    边界框: (x1 = 201, y1 = 64, x2 = 314, y2 = 241)

    检测到对象的数量: 1
    对象: 1:
    类型ID: 0
    置信度: 0.915527
    边界框: (x1 = 192, y1 = 58, x2 = 302, y2 = 253)

    检测到对象的数量: 1
    对象: 1:
    类型ID: 0
    置信度: 0.884766
    边界框: (x1 = 219, y1 = 58, x2 = 329, y2 = 231) */

    for (const auto &boxData : vBoxDatas)
    {
        /* 垃圾满溢 */
        if (boxData.nLabel == GARBAGE_OVERFLOW_LABEL_ID && m_stAlgoGarbageOverflowCfg.bEnable)
        {
            /* 垃圾满溢目标框中心点在检测区域内，才认为本轮命中 */
            if (is_in_region(m_stAlgoGarbageOverflowCfg.stRule.stRegion, boxData.stBoxs))
            {
                bGarbageOverflow = true;
                add_result_to_vector(boxData, vstRectInfo);
            }
            continue;
        }

        /* 垃圾暴露 */
        if (boxData.nLabel == GARBAGE_EXPOSURE_LABEL_ID && m_stAlgoGarbageExposureCfg.bEnable)
        {
            /* 垃圾暴露目标框中心点在检测区域内，才认为本轮命中 */
            if (is_in_region(m_stAlgoGarbageExposureCfg.stRule.stRegion, boxData.stBoxs))
            {
                bGarbageExposure = true;
                add_result_to_vector(boxData, vstRectInfo);
            }
        }
    }

    EventTriggerContext_S stOverflowContext;
    stOverflowContext.enEventType = Event::Type_E::GARBAGE_OVERFLOW;
    stOverflowContext.nChnId = stCtx.nChnId;
    stOverflowContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    /* perf: 有TVSDK客户端订阅时才软件编码全景图，无订阅者或冷却期跳过编码 */
    if (bGarbageOverflow && m_garbageOverflowAlarmStateMachine.canStartAlarm() && stCtx.pFrameInfo != nullptr &&
        AiAppCommon::tvsdk_event_image_required())
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stOverflowContext.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stCtx.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stOverflowContext.pTvSdkPayload = pPayload;
        }
    }
#endif
    m_garbageOverflowAlarmStateMachine.handleAlarmState(bGarbageOverflow, stOverflowContext);

    EventTriggerContext_S stExposureContext;
    stExposureContext.enEventType = Event::Type_E::GARBAGE_EXPOSURE;
    stExposureContext.nChnId = stCtx.nChnId;
    stExposureContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    /* perf: 有TVSDK客户端订阅时才软件编码全景图，无订阅者或冷却期跳过编码 */
    if (bGarbageExposure && m_garbageExposureAlarmStateMachine.canStartAlarm() && stCtx.pFrameInfo != nullptr &&
        AiAppCommon::tvsdk_event_image_required())
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stExposureContext.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stCtx.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stExposureContext.pTvSdkPayload = pPayload;
        }
    }
#endif
    m_garbageExposureAlarmStateMachine.handleAlarmState(bGarbageExposure, stExposureContext);
}

void CGarbageDetect::processSnapshotDetect(std::vector<Inference_NS::BoxData_S> &vBoxDatas, SnapshotResult_S &stResult)
{
    /* 手动抓拍仅复用区域限制：不判断事件使能，也不经过报警状态机。 */
    for (const auto &boxData : vBoxDatas)
    {
        if (boxData.nLabel == GARBAGE_OVERFLOW_LABEL_ID)
        {
            if (is_in_region(m_stAlgoGarbageOverflowCfg.stRule.stRegion, boxData.stBoxs))
            {
                stResult.bGarbageOverflow = true;
                add_result_to_vector(boxData, stResult.vstRectInfo);
            }
            continue;
        }

        if (boxData.nLabel == GARBAGE_EXPOSURE_LABEL_ID)
        {
            if (is_in_region(m_stAlgoGarbageExposureCfg.stRule.stRegion, boxData.stBoxs))
            {
                stResult.bGarbageExposure = true;
                add_result_to_vector(boxData, stResult.vstRectInfo);
            }
            continue;
        }
    }

    dlog_info("垃圾站抓图识别-算法层: 区域过滤完成，推理框[%d] 命中框[%d] 满溢[%d] 暴露[%d]",
              static_cast<int>(vBoxDatas.size()), static_cast<int>(stResult.vstRectInfo.size()),
              stResult.bGarbageOverflow ? 1 : 0, stResult.bGarbageExposure ? 1 : 0);
}

void CGarbageDetect::handleSnapshotRequest(ot_video_frame_info *pFrameInfo,
                                           const std::vector<Inference_NS::BoxData_S> &vBoxDatas)
{
    SnapshotResult_S stResult;

    if (pFrameInfo != nullptr)
    {
        /* 推理结果由调用方复用，避免重复推理。 */
        std::vector<Inference_NS::BoxData_S> vBoxes = vBoxDatas;
        processSnapshotDetect(vBoxes, stResult);

        /* 全景图按正常事件流程同样的方式在内存中编码为 JPEG。 */
        const auto tpJpegBegin = std::chrono::steady_clock::now();
        EventTvSdkImage_S stImage;
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(pFrameInfo, stImage) == OK)
        {
            stResult.vecJpeg = stImage.vecJpeg;
            const auto nJpegCostMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - tpJpegBegin)
                                         .count();
            dlog_info("垃圾站抓图识别-算法层: JPEG 编码完成，大小[%d]字节 耗时[%lld]ms",
                      static_cast<int>(stResult.vecJpeg.size()), static_cast<long long>(nJpegCostMs));
        }
        else
        {
            dlog_error("垃圾站抓图识别-算法层: JPEG 编码失败");
        }
    }
    else
    {
        dlog_warn("垃圾站抓图识别-算法层: 帧数据为空，仅返回空结果");
    }

    {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        m_stSnapshotResult = std::move(stResult);
        m_bSnapshotDone = true;
    }
    m_snapshotCv.notify_all();
}

bool CGarbageDetect::detectOnce(SnapshotResult_S &stResult, int nTimeoutMs)
{
    if (!m_pGarbageDetHandle)
    {
        dlog_error("垃圾站抓图识别-算法层: 算法句柄未初始化，无法执行单帧检测");
        return false;
    }

    dlog_info("垃圾站抓图识别-算法层: 投递单帧检测请求，等待流式线程执行，超时[%d]ms", nTimeoutMs);
    const auto tpBegin = std::chrono::steady_clock::now();

    {
        std::unique_lock<std::mutex> lock(m_snapshotMutex);
        m_bSnapshotDone = false;
        m_stSnapshotResult = SnapshotResult_S();
    }
    m_bSnapshotPending.store(true);

    std::unique_lock<std::mutex> lock(m_snapshotMutex);
    const bool bCompleted = m_snapshotCv.wait_for(lock, std::chrono::milliseconds(nTimeoutMs),
                                                  [this]() { return m_bSnapshotDone; });
    m_bSnapshotPending.store(false);
    if (!bCompleted)
    {
        dlog_error("垃圾站抓图识别-算法层: 单帧检测超时[%d]ms，可能流式线程未收到帧", nTimeoutMs);
        return false;
    }

    stResult = std::move(m_stSnapshotResult);
    const auto nTotalCostMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::steady_clock::now() - tpBegin)
                                  .count();
    dlog_info("垃圾站抓图识别-算法层: 单帧检测完成，总等待[%lld]ms 命中框[%d] JPEG[%d]字节",
              static_cast<long long>(nTotalCostMs), static_cast<int>(stResult.vstRectInfo.size()),
              static_cast<int>(stResult.vecJpeg.size()));
    return true;
}

void CGarbageDetect::handleDetectResult(const std::vector<Inference_NS::BoxData_S> &vBoxDatas, ot_video_frame_info *pFrameInfo)
{
    std::vector<Inference_NS::BoxData_S> tmp(vBoxDatas);

    std::vector<Common::RectInfo_S> vstRectInfo;

    SEventProcessContext stCtx;
    stCtx.nChnId = 0;
    stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
    stCtx.pFrameInfo = pFrameInfo;

    processGarbageDetect(tmp, vstRectInfo, stCtx);

    if (!vstRectInfo.empty())
    {
        send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
    }
}

#endif
