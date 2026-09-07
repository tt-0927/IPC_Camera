/**
 * @FilePath     : exhibition_detect_model.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 展馆人流统计/人员密度检测模型适配器（YOLO 推理 + 双跟踪 + 转换 + 分发）
 */

#pragma once

#if CAP_AI_EXHIBITION_PEOPLE_FLOW && !CAP_AI_PEOPLE_STATISTICS
#error "CAP_AI_EXHIBITION_PEOPLE_FLOW=1 要求 CAP_AI_PEOPLE_STATISTICS=1，请检查设备画像配置"
#endif

#if CAP_AI_PEOPLE_DENSITY_PIPELINE && !CAP_AI_EXHIBITION_PEOPLE_FLOW
#error "CAP_AI_PEOPLE_DENSITY_PIPELINE=1 要求 CAP_AI_EXHIBITION_PEOPLE_FLOW=1，请检查设备画像配置"
#endif

#if CAP_AI_EXHIBITION_PEOPLE_FLOW

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

#include "detection_engine.hpp"
#include "i_inference_engine.hpp"
#include "event_manager.hpp"
#include "exhibition_detect_logic.hpp"
#include "exhibition_result_converter.hpp"
#include "hisi_frame_image_provider.hpp"
#include "hvf_people_flow_output_executor.hpp"
#include "people_flow_config_adapter.hpp"
#include "people_flow_processor.hpp"
#include "result_dispatcher.hpp"
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
#include "exhibition_density_output_executor.hpp"
#include "people_density_config_adapter.hpp"
#include "people_density_processor.hpp"
#endif

/**
 * @brief   : 展馆人流统计/人员密度检测模型适配器
 * @note    : 实现 IDetectionModel 接入统一检测引擎；在 process 内完成
 *            推理引擎推理 -> cIouTracker 跟踪（person/head 双跟踪器）-> Converter 转换
 *            -> Dispatcher 分发 -> 输出执行器（人流统计复用 CHVFPeopleFlowOutputExecutor，
 *            人员密度使用 CExhibitionDensityOutputExecutor，共用 CHisiFrameImageProvider）。
 *            SDK 句柄（IInferenceEngine/cIouTracker）生命周期仅由引擎 Worker 线程访问；
 *            配置线程只通过 setAlgoEnCfg 更新逻辑与共享处理器状态（内部互斥保护）。
 */
class CExhibitionDetectModel : public IDetectionModel
{
public:
    CExhibitionDetectModel();
    ~CExhibitionDetectModel() override;

    /* 禁止拷贝 */
    CExhibitionDetectModel(const CExhibitionDetectModel &) = delete;
    CExhibitionDetectModel &operator=(const CExhibitionDetectModel &) = delete;

    const DetectionModelDescriptor_S &descriptor() const override;
    bool isEnabled() const override;
    bool shouldProcess(int nChannelId) override;
    int process(const ot_video_frame_info *pPreparedFrame,
                const MediaData_S &stMediaData) override;
    int init() override;
    void unInit() override;

    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;
    void setEventStatisticsReporter(
        const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter) override;
    int handleRuntimeCommand(const RuntimeCommand_S &stCommand) override;
    bool needsReinit() const override;
    const Common::Rect_S *crop_rect() const override;

private:
    /**
     * @brief   : 将统一检测框（左上角/右下角）转换为跟踪器输入（中心点/宽高）
     * @param    {const DetectionBox_S} &stBox：统一检测框
     * @param    {DetectResult_S} &stOut：跟踪器输入框
     * @note    : IouTracker 内部按中心点语义计算 IOU（IouTracker.cpp iou()：
     *            x1 = nX1 - w/2），与 DetectionBox_S 的左上角语义不同，必须换算
     */
    static void box_to_detect_result(const DetectionBox_S &stBox,
                                     DetectResult_S &stOut);

    /* 模型描述（名称/输入分辨率/裁剪需求/执行顺序/启动延时） */
    static const DetectionModelDescriptor_S ms_stDescriptor;

    /* 配置/规则/跟踪参数逻辑 */
    CExhibitionDetectLogic m_logic;
    /* 检测频率控制（每 200ms 放行一帧，与 HVF 人脸车检测节奏一致） */
    EventManager m_recvManager{200};
    /* 配置变更后是否需要重建句柄（配置线程置位，Worker 线程 init 时消费） */
    std::atomic<bool> m_bNeedReinit{false};

    /* 推理引擎（当前为 YOLO；海思 HiAiDetect 类模型迁移到统一检测层时替换为
     * CHiAiDetectInferenceEngine，仅 Worker 线程访问） */
    std::unique_ptr<IInferenceEngine> m_pInferenceEngine;
    /* person IOU 跟踪器（人流统计，仅 Worker 线程访问） */
    Inference_NS::cIouTracker *m_pTracker = nullptr;
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    /* head IOU 跟踪器（人员密度，独立于 person 跟踪器避免人/头框互相 IOU 匹配，
     * 仅 Worker 线程访问） */
    Inference_NS::cIouTracker *m_pHeadTracker = nullptr;
#endif

    /* lock: 串行化共享处理器配置应用（配置线程）与逐帧处理（Worker 线程），
     * 模式参考 CHVFPeopleFlowMigrationController 的单锁串行方案 */
    mutable std::mutex m_pipelineMutex;
    /* 新链路结果分发器 */
    AiPipeline_NS::CResultDispatcher m_dispatcher;
    /* 共享人流统计处理器，所有权归属 Dispatcher，本类只保留访问指针 */
    AiPipeline_NS::PeopleFlow_NS::CPeopleFlowProcessor *m_pPeopleFlowProcessor = nullptr;
#if CAP_AI_PEOPLE_DENSITY_PIPELINE
    /* 共享人员密度处理器，所有权归属 Dispatcher，本类只保留访问指针 */
    AiPipeline_NS::PeopleDensity_NS::CPeopleDensityProcessor *m_pDensityProcessor = nullptr;
    /* 人员密度输出执行器 */
    std::unique_ptr<CExhibitionDensityOutputExecutor> m_pDensityExecutor;
    /* 人员密度配置适配器（1920×1080 配置 -> 归一化内部配置） */
    AiPipeline_NS::PeopleDensity_NS::CPeopleDensityConfigAdapter m_densityConfigAdapter;
#endif
    /* 新链路输出执行器 */
    std::unique_ptr<HVFDetectInternal::CHVFPeopleFlowOutputExecutor> m_pOutputExecutor;
    /* 新链路帧图片 Provider */
    HVFDetectInternal::CHisiFrameImageProvider m_imageProvider;
    /* 配置适配器（1920×1080 配置 -> 归一化内部配置） */
    AiPipeline_NS::PeopleFlow_NS::CPeopleFlowConfigAdapter m_configAdapter;
    /* 结果转换器 */
    CExhibitionResultConverter m_converter;
    /* 共享处理器是否已应用过配置（首次 setAlgoEnCfg 前不处理帧） */
    bool m_bPipelineConfigured = false;

    /* 单调递增帧标识（仅 Worker 线程访问） */
    uint64_t m_ullFrameId = 0;
    /* 上一帧已输出的 Track ID 集合（仅 Worker 线程访问），用于生成 ENDED 生命周期 */
    std::unordered_set<int> m_setLastTrackIds;
};

#endif // CAP_AI_EXHIBITION_PEOPLE_FLOW
