#include "smoke_fire_detect.hpp"

#if defined(SCENE_INTELLIGENCE) || CAP_AI_SMOKE_FIRE_DETECT

#include "video_frame_jpeg_encoder.hpp"
#include <chrono>
#include <unistd.h>

/* FirePerson model: 0 smoke, 1 flame, 2 pedestrian. */
static constexpr int kSmokeLabelId = 0;
static constexpr int kFireLabelId = 1;
/* 连续命中时也按事件周期结束，配合状态机冷却后允许再次触发联动。 */
static constexpr int kSmokeFireEventDurationSeconds = EVENT_END_TIME_THRESHOLD;

CSmokeFireDetect::CSmokeFireDetect()
{
    m_thread = std::thread(&CSmokeFireDetect::run, this);
}

CSmokeFireDetect::~CSmokeFireDetect()
{
    m_bRunning.store(false);
    m_dataQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dataQueue.clear();
    unInit();
}

void CSmokeFireDetect::recvMediaData(MediaData_S stMediaData)
{
    m_receivedFrames.fetch_add(1, std::memory_order_relaxed);
    /* 快照请求即使发生在详细开关切换期间，也必须允许下一帧进入队列。 */
    if ((m_bEnabled.load() || m_bSnapshotPending.load()) &&
        m_recvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        QueuedFrame_S stQueuedFrame;
        stQueuedFrame.stMediaData = stMediaData;
        if (m_bSnapshotPending.load(std::memory_order_acquire))
        {
            stQueuedFrame.ullSnapshotRequestId =
                m_activeSnapshotRequestId.load(std::memory_order_acquire);
        }
        m_queuedFrames.fetch_add(1, std::memory_order_relaxed);
        m_dataQueue.pushOrReplace(stQueuedFrame);
    }
}

void CSmokeFireDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_bEnabled.store(false);
    m_config.bEnable = stAlgoConfig.nEnSmokeFire;
    if (m_config.bEnable)
    {
        const int configResult = CEventConfigure::instance()->get_configure(m_config);
        if (configResult != 0)
        {
            dlog_error("烟火识别读取详细配置失败: ret=%d", configResult);
            m_config.bEnable = false;
        }
        dlog_info("烟火识别配置: 总开关=%d, 详细开关=%d, 灵敏度=%u, 置信度阈值=%.2f",
                  stAlgoConfig.nEnSmokeFire, m_config.bEnable, m_config.stRule.nSensitivity,
                  1.0f - m_config.stRule.nSensitivity / 100.0f);
    }
    else
    {
        dlog_info("烟火识别总开关关闭");
    }
    m_bEnabled.store(m_config.bEnable);
}

bool CSmokeFireDetect::init()
{
    m_pDetectHandle = new Inference_NS::CYoloUltralytics(AI_SMOKE_FIRE_DETECTION_CONFIG_FILE);
    if (!m_pDetectHandle->init())
    {
        delete m_pDetectHandle;
        m_pDetectHandle = nullptr;
        dlog_error("烟火识别模型初始化失败");
        return false;
    }
    if (TD_SUCCESS != mppVgs_create_video_frame_info(kWidth, kHeight,
                                                       OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                       &m_dstFrameInfo))
    {
        delete m_pDetectHandle;
        m_pDetectHandle = nullptr;
        dlog_error("烟火识别目标帧创建失败");
        return false;
    }
    dlog_info("烟火识别模型和目标帧初始化成功: %dx%d", kWidth, kHeight);
    return true;
}

void CSmokeFireDetect::unInit()
{
    if (m_pDetectHandle)
    {
        mppVgs_destroy_video_frame_info(&m_dstFrameInfo);
        delete m_pDetectHandle;
        m_pDetectHandle = nullptr;
    }
}

void CSmokeFireDetect::run()
{
    pthread_setname_np(pthread_self(), "SmokeFireDetect");
    QueuedFrame_S stQueuedFrame;
    while (m_bRunning.load())
    {
        logDiagnostics();
        
        /* 手动抓拍请求不受使能开关限制：置位后仍需取帧并推理 */
        if (!m_bEnabled.load() && !m_bSnapshotPending.load())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        if (!m_pDetectHandle && !init())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        if (!m_dataQueue.pop(stQueuedFrame, TIMEOUT_1000_MS) ||
            !stQueuedFrame.stMediaData.pVideoFrameInfo ||
            (!m_bEnabled.load() && !m_bSnapshotPending.load()))
        {
            ++m_stats.noFrame;
            continue;
        }

        MediaData_S &mediaData = stQueuedFrame.stMediaData;
        ot_video_frame_info *frame = mediaData.pVideoFrameInfo.get();
        const unsigned long long ullSnapshotRequestId = stQueuedFrame.ullSnapshotRequestId;
        const bool bSnapshotFrame = ullSnapshotRequestId != 0 &&
            ullSnapshotRequestId == m_activeSnapshotRequestId.load(std::memory_order_acquire);
        if (bSnapshotFrame)
        {
            dlog_info("烟火识别-快照: 取得命令后的实时帧，源尺寸[%dx%d] stride[%u,%u] 像素格式[%d]，模型输入[%dx%d]",
                      mediaData.stMediaParam.nVideoWidth, mediaData.stMediaParam.nVideoHeight,
                      frame->video_frame.stride[0], frame->video_frame.stride[1],
                      static_cast<int>(frame->video_frame.pixel_format), kWidth, kHeight);
        }
        if (mediaData.stMediaParam.nVideoWidth != kWidth || mediaData.stMediaParam.nVideoHeight != kHeight)
        {
            if (TD_SUCCESS != mppVgs_scale(frame, &m_dstFrameInfo))
            {
                ++m_stats.scaleFailed;
                continue;
            }
            frame = &m_dstFrameInfo;
        }

        Inference_NS::InputData_S input;
        input.pData = (float *)frame->video_frame.virt_addr[0];
        input.nDataSize = static_cast<int>(kWidth * kHeight * 1.5) * sizeof(float);
        std::vector<Inference_NS::BoxData_S> boxes;
        if (!m_pDetectHandle->inference(input, boxes))
        {
            ++m_stats.inferenceFailed;
            continue;
        }
        ++m_stats.inferenceOk;
        /* 快照请求优先回填结果：不受后续使能复检影响 */
        if (bSnapshotFrame &&
            m_bSnapshotPending.exchange(false, std::memory_order_acq_rel))
        {
            handleSnapshotRequest(frame, boxes, ullSnapshotRequestId);
        }
        if (!m_bEnabled.load())
        {
            continue;
        }

        SEventProcessContext context;
        context.nChnId = mediaData.stMediaParam.nChannel;
        context.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
        context.pFrameInfo = frame;
        processResult(boxes, context);
    }
}

void CSmokeFireDetect::logDiagnostics()
{
    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastDiagnosticLog < std::chrono::seconds(5))
    {
        return;
    }
    m_lastDiagnosticLog = now;
    const unsigned int received = m_receivedFrames.exchange(0, std::memory_order_relaxed);
    const unsigned int queued = m_queuedFrames.exchange(0, std::memory_order_relaxed);
    if (m_bEnabled.load())
    {
        dlog_info("烟火识别5秒统计: 收帧=%u 入队=%u 无有效帧=%u 缩放失败=%u 推理成功=%u 推理失败=%u "
                  "检测框=%u 烟雾框=%u 火焰框=%u 其他框=%u 过阈值=%u 触发帧=%u 持续事件帧=%u 冷却期帧=%u "
                  "目标最高置信度=%.3f 阈值=%.3f",
                  received, queued, m_stats.noFrame, m_stats.scaleFailed,
                  m_stats.inferenceOk, m_stats.inferenceFailed, m_stats.rawBoxes,
                  m_stats.smokeBoxes, m_stats.fireBoxes, m_stats.otherBoxes,
                  m_stats.passedBoxes, m_stats.triggeredFrames,
                  m_stats.ongoingFrames, m_stats.cooldownFrames,
                  m_stats.maxTargetConfidence, m_stats.threshold);
    }
    m_stats = DiagnosticStats{};
}

void CSmokeFireDetect::processResult(const std::vector<Inference_NS::BoxData_S> &boxes,
                                     const SEventProcessContext &context)
{
    bool triggered = false;
    std::vector<Common::RectInfo_S> rects;
    const float threshold = 1.0f - m_config.stRule.nSensitivity / 100.0f;
    m_stats.threshold = threshold;
    m_stats.rawBoxes += static_cast<unsigned int>(boxes.size());
    int bestLabel = -1;
    float bestConfidence = 0.0f;
    for (const auto &box : boxes)
    {
        if (box.nLabel == kSmokeLabelId)
        {
            ++m_stats.smokeBoxes;
        }
        else if (box.nLabel == kFireLabelId)
        {
            ++m_stats.fireBoxes;
        }
        else
        {
            ++m_stats.otherBoxes;
            continue;
        }
        if (box.fConfidence > m_stats.maxTargetConfidence)
        {
            m_stats.maxTargetConfidence = box.fConfidence;
        }
        if (box.fConfidence >= threshold)
        {
            triggered = true;
            ++m_stats.passedBoxes;
            if (box.fConfidence > bestConfidence)
            {
                bestConfidence = box.fConfidence;
                bestLabel = box.nLabel;
            }
            add_result_to_vector(box, rects);
        }
    }
    if (triggered)
    {
        ++m_stats.triggeredFrames;
    }
    if (!rects.empty())
    {
        send_detectionResult_to_osd(kWidth, kHeight, rects);
    }

    EventTriggerContext_S eventContext;
    eventContext.enEventType = Event::Type_E::SMOKE_FIRE;
    eventContext.nChnId = context.nChnId;
    eventContext.llTimestamp = context.llTimestamp;
    const bool canStartAlarm = m_alarmStateMachine.canStartAlarm();
    if (triggered && canStartAlarm)
    {
        dlog_info("烟火识别命中并提交事件: 类别=%d 置信度=%.3f 阈值=%.3f 通道=%d",
                  bestLabel, bestConfidence, threshold, context.nChnId);
    }
    else if (triggered)
    {
        if (m_alarmStateMachine.isInCooldown())
        {
            ++m_stats.cooldownFrames;
        }
        else
        {
            ++m_stats.ongoingFrames;
        }
    }
#ifdef ENABLE_TVSDK_SRC
    if (triggered && canStartAlarm && context.pFrameInfo != nullptr &&
        AiAppCommon::tvsdk_event_image_required())
    {
        auto payload = std::make_shared<EventTvSdkPayload_S>();
        payload->enType = get_tvsdk_payload_type(eventContext.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(context.pFrameInfo, payload->stPanoramaImage) == OK)
        {
            eventContext.pTvSdkPayload = payload;
        }
    }
#endif
    const bool wasReadyToStart = triggered && canStartAlarm;
    bool alarmActive = m_alarmStateMachine.handleAlarmState(triggered, eventContext);
    const auto now = std::chrono::steady_clock::now();

    if (wasReadyToStart && alarmActive)
    {
        m_eventStartTime = now;
        m_eventStartTimeValid = true;
        dlog_info("烟火事件已启动，将在持续%d秒后强制结束", kSmokeFireEventDurationSeconds);
    }

    if (alarmActive && m_eventStartTimeValid)
    {
        const auto activeSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - m_eventStartTime).count();
        if (activeSeconds >= kSmokeFireEventDurationSeconds)
        {
            dlog_info("烟火事件连续持续%lld秒，执行强制结束",
                      static_cast<long long>(activeSeconds));
            m_alarmStateMachine.endAlarmImmediately(eventContext);
            alarmActive = false;
        }
    }

    if (!alarmActive)
    {
        m_eventStartTimeValid = false;
    }
}

bool CSmokeFireDetect::detectOnce(SnapshotResult_S &stResult, int nTimeoutMs)
{
    /* 单个算法实例只有一个快照结果槽，多个请求必须串行处理。 */
    std::unique_lock<std::mutex> requestLock(m_snapshotRequestMutex);
    if (!m_bRunning.load())
    {
        dlog_error("烟火识别-快照: 算法线程未运行，拒绝抓拍");
        return false;
    }

    /* 丢弃请求前积压的帧，确保快照分析的是命令到达后的新画面。 */
    m_dataQueue.clear();

    /* 先复位上一轮结果，再置请求标志，避免与流式线程回填竞争。 */
    {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        m_bSnapshotDone = false;
        m_completedSnapshotRequestId = 0;
        m_stSnapshotResult = SnapshotResult_S{};
    }
    const unsigned long long ullRequestId =
        m_snapshotRequestSerial.fetch_add(1, std::memory_order_relaxed) + 1;
    m_activeSnapshotRequestId.store(ullRequestId, std::memory_order_release);
    m_bSnapshotPending.store(true, std::memory_order_release);
    dlog_info("烟火识别-快照: 请求[%llu]等待命令到达后的新视频帧", ullRequestId);

    std::unique_lock<std::mutex> lock(m_snapshotMutex);
    const bool bOk = m_snapshotCv.wait_for(lock, std::chrono::milliseconds(nTimeoutMs),
                                           [this, ullRequestId]() {
                                               return m_bSnapshotDone &&
                                                   m_completedSnapshotRequestId == ullRequestId;
                                           });
    m_bSnapshotPending.store(false, std::memory_order_release);
    if (!bOk)
    {
        dlog_error("烟火识别-快照: 等待检测结果超时[%d]ms", nTimeoutMs);
        return false;
    }

    stResult = m_stSnapshotResult;
    return true;
}

void CSmokeFireDetect::handleSnapshotRequest(ot_video_frame_info *pFrameInfo,
                                             const std::vector<Inference_NS::BoxData_S> &boxes,
                                             unsigned long long ullRequestId)
{
    SnapshotResult_S stResult;
    processSnapshotDetect(boxes, stResult);

    /* JPEG 编码复用与正常事件相同的接口 */
    if (pFrameInfo != nullptr)
    {
        EventTvSdkImage_S stImage;
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(pFrameInfo, stImage) == OK)
        {
            stResult.vecJpeg = stImage.vecJpeg;      /* 编码成功后再取成员回填 */
        }
        else
        {
            dlog_error("烟火识别-快照: JPEG 编码失败，仅返回检测结果");
            stResult.vecJpeg.clear();
        }
    }
    else
    {
        dlog_warn("烟火识别-快照: 帧信息为空，仅返回检测结果");
    }

    const int nSmoke = stResult.bSmokeDetected ? 1 : 0;
    const int nFire = stResult.bFireDetected ? 1 : 0;
    const size_t nRectCount = stResult.vstRectInfo.size();
    const size_t nJpegSize = stResult.vecJpeg.size();
    const unsigned int nRawBoxCount = stResult.nRawBoxCount;
    const float fMaxTargetConfidence = stResult.fMaxTargetConfidence;
    const float fThreshold = stResult.fThreshold;

    {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        m_stSnapshotResult = std::move(stResult);
        m_completedSnapshotRequestId = ullRequestId;
        m_bSnapshotDone = true;
    }
    m_snapshotCv.notify_all();

    dlog_info("烟火识别-快照: 请求[%llu]检测完成，原始框[%u] 烟雾[%d] 火焰[%d] 命中框[%zu] "
              "目标最高置信度[%.3f] 快照额外阈值[%.3f] 输入[%dx%d] JPEG[%zu]字节",
              ullRequestId, nRawBoxCount, nSmoke, nFire, nRectCount,
              fMaxTargetConfidence, fThreshold,
              kWidth, kHeight, nJpegSize);
}

void CSmokeFireDetect::processSnapshotDetect(const std::vector<Inference_NS::BoxData_S> &boxes,
                                             SnapshotResult_S &stResult)
{
    /*
     * boxes 已经经过模型配置中的 confidence 阈值筛选。手动抓拍与垃圾抓拍保持一致，
     * 不再叠加实时事件的灵敏度阈值，避免单帧候选框被重复过滤。
     */
    const float threshold = 0.0f;
    stResult.nRawBoxCount = static_cast<unsigned int>(boxes.size());
    stResult.fThreshold = threshold;
    for (const auto &box : boxes)
    {
        if (box.nLabel != kSmokeLabelId && box.nLabel != kFireLabelId)
        {
            continue;
        }
        if (box.fConfidence > stResult.fMaxTargetConfidence)
        {
            stResult.fMaxTargetConfidence = box.fConfidence;
        }
        if (box.nLabel == kSmokeLabelId)
        {
            stResult.bSmokeDetected = true;
        }
        else
        {
            stResult.bFireDetected = true;
        }
        add_result_to_vector(box, stResult.vstRectInfo);
    }
}

#endif
