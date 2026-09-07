/**
 * @FilePath     : exhibition_detect_model.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 16:30:00
 * @Description  : 展馆人流统计/人员密度检测模型适配器实现
 */

#include "exhibition_detect_model.hpp"

#include "action_code.h"
#include "common_process.h"
#include "dlog.h"
#include "path_define.h"
#include <string>
#include "time_utils.h"
#include "video_define.h"
#include "yolo_inference_engine.hpp"

#if CAP_AI_EXHIBITION_PEOPLE_FLOW

namespace
{
/**
 * @brief   : 获取当前 monotonic 毫秒时间戳（Track TTL 与报警生命周期基准）
 * @return   {int64_t} monotonic 毫秒时间戳
 */
int64_t get_monotonic_timestamp_ms()
{
    return static_cast<int64_t>(TimeUtils_NS::get_monotonicTimestampMs());
}
} // namespace

const DetectionModelDescriptor_S CExhibitionDetectModel::ms_stDescriptor = {
    "ExhibitionDetect",      /* 模型名 */
    PIXEL_WIDTH_1024,        /* 模型输入分辨率宽（ai_exhibition.json size） */
    PIXEL_HEIGHT_576,        /* 模型输入分辨率高 */
    false,                   /* 全帧检测，无需按区域裁剪 */
    2,                       /* 同帧执行顺序（在 MD/OD 之后） */
    4000                     /* 启动延时：与 MD 一致，防首次检测误报 */
};

CExhibitionDetectModel::CExhibitionDetectModel()
{
    /* 组装新链路：输出执行器 + 帧图片 Provider + 共享人流统计处理器（所有权归 Dispatcher） */
    m_pOutputExecutor = std::make_unique<HVFDetectInternal::CHVFPeopleFlowOutputExecutor>();
    m_pOutputExecutor->set_image_provider(&m_imageProvider);

    auto pProcessor = std::make_unique<AiPipeline_NS::PeopleFlow_NS::CPeopleFlowProcessor>();
    m_pPeopleFlowProcessor = pProcessor.get();
    m_dispatcher.register_processor(std::move(pProcessor));

#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    /* 人员密度：共享密度处理器（所有权归 Dispatcher）+ 密度输出执行器（共用帧图片 Provider） */
    auto pDensityProcessor = std::make_unique<AiPipeline_NS::PeopleDensity_NS::CPeopleDensityProcessor>();
    m_pDensityProcessor = pDensityProcessor.get();
    m_dispatcher.register_processor(std::move(pDensityProcessor));

    m_pDensityExecutor = std::make_unique<CExhibitionDensityOutputExecutor>();
    m_pDensityExecutor->set_image_provider(&m_imageProvider);
#endif

    /* 推理引擎：展馆模型用 YOLO；海思 HiAiDetect 引擎（CHiAiDetectInferenceEngine）
     * 预留，迁移时切换 */
    m_pInferenceEngine = std::make_unique<CYoloInferenceEngine>();
}

CExhibitionDetectModel::~CExhibitionDetectModel()
{
    unInit();
}

const DetectionModelDescriptor_S &CExhibitionDetectModel::descriptor() const
{
    return ms_stDescriptor;
}

bool CExhibitionDetectModel::isEnabled() const
{
    /* 引擎懒初始化门控：算法总开关（nEnPeopleFlowStatistics） */
    return m_logic.is_enabled();
}

bool CExhibitionDetectModel::shouldProcess(int nChannelId)
{
    /* 帧率控制：每 200ms 放行一帧（与 HVF 人脸车检测节奏一致），
     * 展馆 YOLO 推理较重，全帧率推理会挤占引擎 Worker */
    if (!m_recvManager.handleEvent(nChannelId))
    {
        return false;
    }
    /* 重建请求未处理完时跳过本帧，避免新规则送入旧跟踪器 */
    return !m_bNeedReinit.load();
}

int CExhibitionDetectModel::process(const ot_video_frame_info *pPreparedFrame,
                                    const MediaData_S &stMediaData)
{
    /* 仅 Worker 线程调用 */
    if (pPreparedFrame == nullptr || m_pInferenceEngine == nullptr || m_pTracker == nullptr
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
        || m_pHeadTracker == nullptr
#endif
    )
    {
        return ERR_PARAM;
    }
    /* 配置在分发间隙更新：本帧跳过，等待引擎下一轮重建句柄 */
    if (m_bNeedReinit.load())
    {
        return OK;
    }

    /* lock: 与配置线程的处理器配置应用/清零串行，保证 processor 状态一致 */
    std::lock_guard<std::mutex> lock(m_pipelineMutex);
    const bool bFlowActive = m_logic.is_flow_active();
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    const bool bDensityActive = m_logic.is_density_active();
#else
    const bool bDensityActive = false;
#endif
    if ((!bFlowActive && !bDensityActive) || !m_bPipelineConfigured)
    {
        return OK;
    }

    /* 1. 推理：统一接口输出 DetectionBox_S（模型输入坐标，左上角/右下角），
     *    帧数据与模型输入分辨率由引擎内部按配置构造，适配器不再接触 YOLO SDK */
    std::vector<DetectionBox_S> vOutBoxes;
    const int nInferRet = m_pInferenceEngine->infer(pPreparedFrame, vOutBoxes);
    if (nInferRet != OK)
    {
        dlog_warn("展馆模型推理失败，本帧跳过");
        return nInferRet;
    }

    /* 2. 按类别过滤并换算为跟踪器输入（中心点/宽高语义）：
     *    person 供人流统计，head 供人员密度；业务未生效的类别不进跟踪器；
     *    非法框（宽高<=0）在进入跟踪器前丢弃，避免负面积破坏 IOU 匹配 */
    std::vector<DetectResult_S> vPersonDetections;
    std::vector<DetectResult_S> vHeadDetections;
    vPersonDetections.reserve(vOutBoxes.size());
    for (const auto &stBox : vOutBoxes)
    {
        if (stBox.nX2 <= stBox.nX1 || stBox.nY2 <= stBox.nY1)
        {
            continue;
        }
        DetectResult_S stDetect;
        if (stBox.nClassId == ExhibitionDetect_NS::LABEL_PERSON)
        {
            if (!bFlowActive)
            {
                continue;
            }
            box_to_detect_result(stBox, stDetect);
            vPersonDetections.emplace_back(stDetect);
        }
        else if (stBox.nClassId == ExhibitionDetect_NS::LABEL_HEAD)
        {
            if (!bDensityActive)
            {
                continue;
            }
            box_to_detect_result(stBox, stDetect);
            vHeadDetections.emplace_back(stDetect);
        }
    }

    /* 3. 跟踪：person/head 使用相互独立的跟踪器，输出稳定 Track ID */
    std::vector<TrackResult_S> vTrackedPersons;
    std::vector<int> vEndedTrackIds;
    if (bFlowActive)
    {
        vTrackedPersons = m_pTracker->update(vPersonDetections);

        /* 4. 生成 ENDED 生命周期：上一帧存在、本帧跟踪器已删除的 Track（丢失超过 max_age） */
        std::unordered_set<int> stCurrentTrackIds;
        stCurrentTrackIds.reserve(vTrackedPersons.size());
        for (const auto &stTrack : vTrackedPersons)
        {
            stCurrentTrackIds.insert(stTrack.nTrackId);
        }
        for (const int nLastTrackId : m_setLastTrackIds)
        {
            if (stCurrentTrackIds.find(nLastTrackId) == stCurrentTrackIds.end())
            {
                vEndedTrackIds.emplace_back(nLastTrackId);
            }
        }
        m_setLastTrackIds = std::move(stCurrentTrackIds);
    }

    std::vector<TrackResult_S> vTrackedHeads;
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    if (bDensityActive)
    {
        vTrackedHeads = m_pHeadTracker->update(vHeadDetections);
    }
#endif

    if (bFlowActive)
    {
        /* 跟踪诊断：依赖 dlog 全局限流按调用点合并（1 秒一次），
         * 展示检测框/跟踪输出/ID 生命周期摘要，便于定位跟踪是否正常 */
        std::string strTrackIds;
        size_t nMaxShow = vTrackedPersons.size() > 8U ? 8U : vTrackedPersons.size();
        for (size_t i = 0; i < nMaxShow; ++i)
        {
            if (i > 0) { strTrackIds += ' '; }
            strTrackIds += std::to_string(vTrackedPersons[i].nTrackId);
        }
        if (vTrackedPersons.size() > 8U) { strTrackIds += "..."; }

        std::string strEndedIds;
        nMaxShow = vEndedTrackIds.size() > 8U ? 8U : vEndedTrackIds.size();
        for (size_t i = 0; i < nMaxShow; ++i)
        {
            if (i > 0) { strEndedIds += ' '; }
            strEndedIds += std::to_string(vEndedTrackIds[i]);
        }
        if (vEndedTrackIds.size() > 8U) { strEndedIds += "..."; }

        dlog_info("展馆跟踪: 检测框[%zu] 人体[%zu] 跟踪输出[%zu->{%s}] 结束[%zu->{%s}] 活跃跟踪[%d]",
                  vOutBoxes.size(),
                  vPersonDetections.size(),
                  vTrackedPersons.size(), strTrackIds.c_str(),
                  vEndedTrackIds.size(), strEndedIds.c_str(),
                  m_pTracker->active_track_count());

        /* 参考 svpAiDetect_printResult 风格，逐目标打印人体检测/跟踪详细结果；
         * 依赖 dlog 全局限流防止帧级刷屏；最多展示 20 个目标 */
        std::string strDetail;
        const size_t nDetailMax = vTrackedPersons.size() > 20U ? 20U : vTrackedPersons.size();
        for (size_t i = 0; i < nDetailMax; ++i)
        {
            const auto &stBox = vTrackedPersons[i].box;
            const int nW = stBox.nW;
            const int nH = stBox.nH;
            char buf[64];
            snprintf(buf, sizeof(buf), " [#%d %d%% %dx%d]", vTrackedPersons[i].nTrackId,
                     static_cast<int>(stBox.fConfidence * 100.0f), nW, nH);
            strDetail += buf;
        }
        if (vTrackedPersons.size() > 20U) { strDetail += " ..."; }
        if (!strDetail.empty())
        {
            dlog_info("展馆检测结果:%s", strDetail.c_str());
        }
    }

#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    if (bDensityActive)
    {
        dlog_info("展馆人头跟踪: 人头框[%zu] 跟踪输出[%zu] 活跃跟踪[%d]",
                  vHeadDetections.size(),
                  vTrackedHeads.size(),
                  m_pHeadTracker->active_track_count());
    }
#endif

    /* 5. 帧元数据基础：同一帧只采样一次时间戳与帧标识，全链路共用 */
    AiPipeline_NS::FrameMetadata_S stMetaBase;
    stMetaBase.nChannelId = stMediaData.stMediaParam.nChannel;
    stMetaBase.ullFrameId = ++m_ullFrameId;
    stMetaBase.llWallTimestampMs = static_cast<int64_t>(TimeUtils_NS::get_currentTimestampMs());
    stMetaBase.llMonotonicTimestampMs = get_monotonic_timestamp_ms();
    stMetaBase.stSourceFrameSize.nWidth = static_cast<uint32_t>(stMediaData.stMediaParam.nVideoWidth);
    stMetaBase.stSourceFrameSize.nHeight = static_cast<uint32_t>(stMediaData.stMediaParam.nVideoHeight);

    /* 6. 转换：person/head 跟踪结果 -> 标准检测批次（未生效业务的跟踪集合为空） */
    AiPipeline_NS::DetectionBatch_S stBatch;
    const int nConvertRet = m_converter.convert(vTrackedPersons, vEndedTrackIds, vTrackedHeads, stMetaBase, stBatch);
    if (nConvertRet != OK)
    {
        /* Converter 失败只跳过本帧输出，不产生副作用 */
        return nConvertRet;
    }

    /* 7. 分发 + 外部输出（人流统计复用 HVF 输出适配，人员密度由密度执行器消费，
     *    图片编码期间持有 Frame Lease） */
    AiPipeline_NS::ProcessorOutput_S stOutput;
    m_dispatcher.dispatch(stBatch, stOutput);

    std::vector<Common::RectInfo_S> vstRectInfo;
    m_imageProvider.set_frame(stMediaData.pVideoFrameInfo);
    m_pOutputExecutor->set_native_result_size(m_converter.native_result_size());
    m_pOutputExecutor->process(stOutput, vstRectInfo);
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    m_pDensityExecutor->set_native_result_size(m_converter.native_result_size());
    m_pDensityExecutor->process(stOutput);
#endif
    m_imageProvider.clear_frame();

    /* OSD 叠加：转换回模型坐标（1024×576）后送入现有 OSD 链路 */
    if (!vstRectInfo.empty())
    {
        send_detectionResult_to_osd(PIXEL_WIDTH_1024, PIXEL_HEIGHT_576, vstRectInfo);
    }
    return OK;
}

int CExhibitionDetectModel::init()
{
    /* 仅 Worker 线程调用（引擎保证 init/unInit/process 同线程串行） */
    if (m_pInferenceEngine == nullptr)
    {
        return ERR;
    }

    /* 消费重建请求：初始化期间到达的新配置会重新置位，由引擎下一轮再次重建 */
    m_bNeedReinit.store(false);
    m_logic.consume_tracker_rebuild_request();

    /* 引擎内部按模型配置加载 YOLO 句柄；失败时引擎侧已释放句柄，
     * 走引擎 1s 重试路径 */
    const int nEngineRet = m_pInferenceEngine->init(AI_EXHIBITION_DETECTION_CONFIG_FILE);
    if (nEngineRet != OK)
    {
        dlog_error("展馆模型初始化失败 [%s]", AI_EXHIBITION_DETECTION_CONFIG_FILE);
        return nEngineRet;
    }

    /* 跟踪器参数由逻辑配置提供（默认 max_age=5/min_hits=3/iou_threshold=0.3/max_tracks=20） */
    m_pTracker = new (std::nothrow) Inference_NS::cIouTracker(m_logic.tracker_max_age(),
                                                              m_logic.tracker_min_hits(),
                                                              m_logic.tracker_iou_threshold(),
                                                              m_logic.tracker_max_tracks());
    if (m_pTracker == nullptr)
    {
        /* 跟踪器创建失败：释放引擎句柄，下次重试从引擎初始化开始 */
        m_pInferenceEngine->unInit();
        dlog_error("展馆模型初始化失败-创建跟踪器失败");
        return ERR;
    }

#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    /* head 跟踪器：密度瞬时计数要求出现即统计，min_hits 默认 1 */
    m_pHeadTracker = new (std::nothrow) Inference_NS::cIouTracker(m_logic.head_tracker_max_age(),
                                                                  m_logic.head_tracker_min_hits(),
                                                                  m_logic.head_tracker_iou_threshold(),
                                                                  m_logic.tracker_max_tracks());
    if (m_pHeadTracker == nullptr)
    {
        /* memory: 释放已创建的 person 跟踪器与引擎句柄，下次重试从引擎初始化开始 */
        delete m_pTracker;
        m_pTracker = nullptr;
        m_pInferenceEngine->unInit();
        dlog_error("展馆模型初始化失败-创建head跟踪器失败");
        return ERR;
    }
#endif
    /* 重建后旧 Track 状态失效，清空生命周期差集基准 */
    m_setLastTrackIds.clear();

    dlog_info("展馆人流统计模型初始化成功 [%s]", AI_EXHIBITION_DETECTION_CONFIG_FILE);
    return OK;
}

void CExhibitionDetectModel::unInit()
{
    /* 仅 Worker 线程调用 */
    if (m_pTracker != nullptr)
    {
        delete m_pTracker;
        m_pTracker = nullptr;
    }
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    if (m_pHeadTracker != nullptr)
    {
        delete m_pHeadTracker;
        m_pHeadTracker = nullptr;
    }
#endif
    if (m_pInferenceEngine != nullptr)
    {
        /* memory: 引擎内部释放 YOLO 模型句柄与模型资源 */
        m_pInferenceEngine->unInit();
    }
    m_setLastTrackIds.clear();
}

void CExhibitionDetectModel::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    /* 配置线程调用；与 process 共用同一把锁，保证逻辑状态与共享处理器配置原子切换，
     * 避免"新逻辑使能 + 旧处理器配置"的中间态进入逐帧处理 */
    std::lock_guard<std::mutex> lock(m_pipelineMutex);

    m_logic.apply_algo_config(stAlgoConfig);
    if (m_logic.consume_tracker_rebuild_request())
    {
        /* 规则变化或使能切换：重建跟踪器（清空旧 Track 状态） */
        m_bNeedReinit.store(true);
    }

    const bool bFlowActive = m_logic.is_flow_active();
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    const bool bDensityActive = m_logic.is_density_active();
#else
    const bool bDensityActive = false;
#endif

    if (!bFlowActive && !bDensityActive)
    {
        /* 全部业务关闭：处理器保持禁用并清空运行态 */
        if (m_pPeopleFlowProcessor != nullptr)
        {
            m_pPeopleFlowProcessor->reset();
        }
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
        if (m_pDensityProcessor != nullptr)
        {
            m_pDensityProcessor->reset();
        }
        if (m_pDensityExecutor != nullptr)
        {
            m_pDensityExecutor->reset();
        }
#endif
        if (m_pOutputExecutor != nullptr)
        {
            m_pOutputExecutor->reset();
        }
        m_bPipelineConfigured = false;
        return;
    }

    /* 人流统计配置应用：只转换一次为归一化内部配置，非法配置由适配器保持禁用 */
    if (bFlowActive)
    {
        Alarm::PeopleFlowStatistics_S stConfig;
        m_logic.snapshot_config(stConfig);
        AiPipeline_NS::PeopleFlow_NS::PeopleFlowConfig_S stNewConfig;
        const int nAdaptRet = m_configAdapter.adapt(stConfig, stNewConfig);
        if (nAdaptRet != OK)
        {
            dlog_warn("展馆人流统计配置非法，处理器保持禁用（检测线或区域无效）");
        }
        dlog_info("展馆人流统计适配结果: 适配[%s] 处理器使能[%d] 灵敏度[%u] 规则线[(%.1f,%.1f)->(%.1f,%.1f) 方向[%d]] 区域顶点数[%zu]",
                  (nAdaptRet == OK) ? "OK" : "ERR_PARAM",
                  stNewConfig.bEnabled ? 1 : 0,
                  stNewConfig.nSensitivity,
                  stNewConfig.stRule.stLineStart.dX, stNewConfig.stRule.stLineStart.dY,
                  stNewConfig.stRule.stLineEnd.dX, stNewConfig.stRule.stLineEnd.dY,
                  static_cast<int>(stNewConfig.stRule.enEnterDirection),
                  stNewConfig.stRule.vecRegion.size());
        m_pPeopleFlowProcessor->apply_config(stNewConfig);
        dlog_info("展馆人流统计模型应用配置：总开关[%d] 配置使能[%d]",
                  stAlgoConfig.nEnPeopleFlowStatistics != 0 ? 1 : 0,
                  stConfig.bEnable ? 1 : 0);
    }
    else if (m_pPeopleFlowProcessor != nullptr)
    {
        m_pPeopleFlowProcessor->reset();
    }

#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    /* 人员密度配置应用：归一化区域 + 三级阈值，非法配置由适配器保持禁用 */
    if (bDensityActive)
    {
        Alarm::PeopleDensityDetection_S stDensityConfig;
        m_logic.snapshot_density_config(stDensityConfig);
        AiPipeline_NS::PeopleDensity_NS::PeopleDensityConfig_S stNewDensityConfig;
        const int nDensityAdaptRet = m_densityConfigAdapter.adapt(stDensityConfig, stNewDensityConfig);
        if (nDensityAdaptRet != OK)
        {
            dlog_warn("展馆人员密度配置非法，处理器保持禁用（区域或灵敏度无效）");
        }
        dlog_info("展馆人员密度适配结果: 适配[%s] 处理器使能[%d] 灵敏度[%u] 上报间隔[%u]s 区域顶点数[%zu]",
                  (nDensityAdaptRet == OK) ? "OK" : "ERR_PARAM",
                  stNewDensityConfig.bEnabled ? 1 : 0,
                  stNewDensityConfig.nSensitivity,
                  stNewDensityConfig.nReportIntervalSec,
                  stNewDensityConfig.vecRegion.size());
        m_pDensityProcessor->apply_config(stNewDensityConfig);
    }
    else
    {
        if (m_pDensityProcessor != nullptr)
        {
            m_pDensityProcessor->reset();
        }
        /* 密度禁用时同步清空三级报警状态机，与旧 V2 setEnabled(false) 复位语义对齐：
         * 处理器禁用后不再输出事件条件，若状态机残留激活态，报警将永远无法结束 */
        if (m_pDensityExecutor != nullptr)
        {
            m_pDensityExecutor->reset();
        }
    }
#endif

    m_bPipelineConfigured = true;
}

void CExhibitionDetectModel::setEventStatisticsReporter(
    const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
    if (m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->set_reporter(pReporter);
    }
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    if (m_pDensityExecutor != nullptr)
    {
        m_pDensityExecutor->set_reporter(pReporter);
    }
#endif
}

int CExhibitionDetectModel::handleRuntimeCommand(const RuntimeCommand_S &stCommand)
{
    if (stCommand.nCode != AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT)
    {
        return ERR;
    }

    /* lock: 与逐帧处理互斥，避免清零与统计更新竞争 */
    std::lock_guard<std::mutex> lock(m_pipelineMutex);
    if (m_pPeopleFlowProcessor != nullptr)
    {
        m_pPeopleFlowProcessor->clear_statistics();
    }
    if (m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->reset();
    }
    dlog_info("展馆人流统计运行态结果已清零");
    return OK;
}

bool CExhibitionDetectModel::needsReinit() const
{
    return m_bNeedReinit.load();
}

const Common::Rect_S *CExhibitionDetectModel::crop_rect() const
{
    /* 展馆模型全帧检测，无裁剪区域 */
    return nullptr;
}

void CExhibitionDetectModel::box_to_detect_result(const DetectionBox_S &stBox,
                                                  DetectResult_S &stOut)
{
    /* DetectionBox_S 为左上角 (nX1,nY1) + 右下角 (nX2,nY2)；
     * IouTracker 的 DetectResult_S 为"中心点 (nX1,nY1) + 宽高 (nW,nH)"语义
     * （见 IouTracker.cpp iou()：x1 = nX1 - w/2），两者必须换算，否则 IOU 匹配错位 */
    const int nW = stBox.nX2 - stBox.nX1;
    const int nH = stBox.nY2 - stBox.nY1;
    stOut.nClassId = stBox.nClassId;
    stOut.fConfidence = stBox.fConfidence;
    stOut.nX1 = stBox.nX1 + nW / 2;
    stOut.nY1 = stBox.nY1 + nH / 2;
    stOut.nW = nW;
    stOut.nH = nH;
}

#endif // CAP_AI_EXHIBITION_PEOPLE_FLOW
