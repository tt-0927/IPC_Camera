/**
 * @FilePath     : hvf_people_flow_migration_controller.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 人流统计迁移控制器实现
 */

#include "hvf_people_flow_migration_controller.hpp"

#include <chrono>

#include "IpcRet.h"
#include "dlog.h"
#include "hvf_detect_context.hpp"
#include "time_utils.h"

#if CAP_AI_PEOPLE_STATISTICS

namespace HVFDetectInternal
{
namespace
{
/**
 * @brief   : 获取当前 monotonic 毫秒时间戳
 * @return   {int64_t} monotonic 毫秒时间戳
 */
int64_t get_monotonic_timestamp_ms()
{
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}
} // namespace

CHVFPeopleFlowMigrationController::CHVFPeopleFlowMigrationController()
{
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    m_pOutputExecutor = std::make_unique<CHVFPeopleFlowOutputExecutor>();
    m_pOutputExecutor->set_image_provider(&m_imageProvider);

    /* 处理器所有权移交给 Dispatcher，本类保留访问指针 */
    auto pProcessor = std::make_unique<AiPipeline_NS::PeopleFlow_NS::CPeopleFlowProcessor>();
    m_pPeopleFlowProcessor = pProcessor.get();
    m_dispatcher.register_processor(std::move(pProcessor));
#endif
}

void CHVFPeopleFlowMigrationController::set_algo_resolution(int nWidth, int nHeight)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nAlgoWidth = nWidth;
    m_nAlgoHeight = nHeight;
}

void CHVFPeopleFlowMigrationController::set_model_input_size(const AiPipeline_NS::FrameSize_S &stModelInputSize)
{
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stModelInputSize = stModelInputSize;

    /* 模型尺寸非法时新链路保持不可用，Legacy 继续可运行 */
    if (m_stModelInputSize.nWidth == 0U || m_stModelInputSize.nHeight == 0U)
    {
        m_bNewPipelineReady = false;
        dlog_warn("人流统计新链路不可用：模型输入尺寸非法");
        return;
    }

    /* 模型重启后尺寸可能变化，重建 Converter 以刷新坐标合同 */
    m_pConverter = std::make_unique<CHisiHvfResultConverter>(m_enCoordinateSource, m_stModelInputSize);
    m_bNewPipelineReady = true;
    dlog_info("人流统计新链路就绪：模型输入[%ux%u] 原生结果坐标模式[%d]",
              m_stModelInputSize.nWidth,
              m_stModelInputSize.nHeight,
              static_cast<int>(m_enCoordinateSource));
#else
    /* 未启用新 Pipeline 时仅忽略输入，保持接口稳定 */
    (void) stModelInputSize;
#endif
}

void CHVFPeopleFlowMigrationController::apply_config(const Alarm::PeopleFlowStatistics_S &stConfig, bool bAlgoEnabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bAlgoEnabled = bAlgoEnabled;
    m_stConfig = stConfig;
    const bool bNowEnabled = bAlgoEnabled && stConfig.bEnable;
    m_bConfigValid = true;

    /* Legacy 链路：先设置使能，再按旧语义应用完整配置 */
    m_legacyProcessor.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyProcessor.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }
    else
    {
        m_legacyProcessor.clearStatisticsResult();
    }

#if CAP_AI_PEOPLE_FLOW_PIPELINE
    apply_new_pipeline_config(stConfig, bNowEnabled);
#endif

    m_bEnabled = bNowEnabled;
    dlog_info("人流统计迁移控制器应用配置：总开关[%d] 配置使能[%d] 生效[%d]",
              bAlgoEnabled ? 1 : 0,
              stConfig.bEnable ? 1 : 0,
              bNowEnabled ? 1 : 0);
}

void CHVFPeopleFlowMigrationController::update_parameters(const Alarm::PeopleFlowStatistics_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stConfig = stConfig;
    const bool bNowEnabled = stConfig.bEnable;
    m_bConfigValid = true;

    m_legacyProcessor.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyProcessor.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }
    else
    {
        m_legacyProcessor.clearStatisticsResult();
    }

#if CAP_AI_PEOPLE_FLOW_PIPELINE
    apply_new_pipeline_config(stConfig, bNowEnabled);
#endif

    m_bEnabled = bNowEnabled;
    dlog_info("人流统计迁移控制器更新参数：生效[%d]", bNowEnabled ? 1 : 0);
}

void CHVFPeopleFlowMigrationController::set_reporter(const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_legacyProcessor.setReporter(pReporter);
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    if (m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->set_reporter(pReporter);
    }
#endif
}

void CHVFPeopleFlowMigrationController::clear_statistics()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_legacyProcessor.clearStatisticsResult();
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    if (m_pPeopleFlowProcessor != nullptr)
    {
        m_pPeopleFlowProcessor->clear_statistics();
    }
    if (m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->reset();
    }
#endif
    dlog_info("人流统计运行态结果已清零");
}

bool CHVFPeopleFlowMigrationController::is_enabled() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bEnabled;
}

int CHVFPeopleFlowMigrationController::process(ot_aidetect_result_array &stResult,
                                               int nChnId,
                                               const std::shared_ptr<ot_video_frame_info> &pFrameInfo,
                                               const AiPipeline_NS::FrameSize_S &stSourceFrameSize,
                                               std::vector<Common::RectInfo_S> &vstRectInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_bEnabled)
    {
        return OK;
    }

    /* 同一帧内只采样一次时间戳与帧标识，全链路共用 */
    const uint64_t ullFrameId = ++m_ullFrameId;
    const int64_t llWallMs = static_cast<int64_t>(TimeUtils_NS::get_currentTimestampMs());
    const int64_t llMonoMs = get_monotonic_timestamp_ms();

    /* Legacy 处理上下文，旧处理器按算法分辨率消费原生结果 */
    const auto build_legacy_context = [&]()
    {
        HVFDetectInternal::SHVFProcessContext stContext{ stResult,
                                                         vstRectInfo,
                                                         m_nAlgoWidth,
                                                         m_nAlgoHeight
#if CAP_EXHIBITION_OSD_PANEL
                                                         ,
                                                         nullptr
#endif
        };
        stContext.nChnId = nChnId;
        stContext.llTimestamp = llWallMs;
        stContext.pFrameInfo = pFrameInfo.get();
        return stContext;
    };

#if CAP_AI_PEOPLE_FLOW_PIPELINE
    /* 新链路处理：执行标准转换、分发与全部外部输出 */
    if (m_bNewPipelineReady && m_pConverter != nullptr && m_pPeopleFlowProcessor != nullptr && m_pOutputExecutor != nullptr)
    {
        /* 帧元数据基础：源帧尺寸与能力位由 Converter 填充 */
        AiPipeline_NS::FrameMetadata_S stMetaBase;
        stMetaBase.nChannelId = nChnId;
        stMetaBase.ullFrameId = ullFrameId;
        stMetaBase.llWallTimestampMs = llWallMs;
        stMetaBase.llMonotonicTimestampMs = llMonoMs;
        stMetaBase.stSourceFrameSize = stSourceFrameSize;

        AiPipeline_NS::DetectionBatch_S stBatch;
        const int nConvertRet = m_pConverter->convert(stResult, stMetaBase, stBatch);
        if (nConvertRet != OK)
        {
            /* Converter 失败只跳过本帧输出，不回退为同帧 Legacy，避免双重或不确定副作用 */
            return nConvertRet;
        }

        AiPipeline_NS::ProcessorOutput_S stOutput;
        m_dispatcher.dispatch(stBatch, stOutput);

        /* 新链路执行外部输出，图片编码期间持有 Frame Lease */
        m_imageProvider.set_frame(pFrameInfo);
        m_pOutputExecutor->set_native_result_size(m_pConverter->native_result_size());
        m_pOutputExecutor->process(stOutput, vstRectInfo);
        m_imageProvider.clear_frame();
        return OK;
    }
#endif

    /* LEGACY 或新链路未就绪：只运行旧处理器 */
    HVFDetectInternal::SHVFProcessContext stLegacyContext = build_legacy_context();
    m_legacyProcessor.process(stLegacyContext);
    return OK;
}

void CHVFPeopleFlowMigrationController::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_legacyProcessor.clearStatisticsResult();
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    if (m_pPeopleFlowProcessor != nullptr)
    {
        m_pPeopleFlowProcessor->reset();
    }
    if (m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->reset();
    }
    m_imageProvider.clear_frame();
#endif
    m_bEnabled = false;
    m_bAlgoEnabled = false;
    m_bConfigValid = false;
    m_ullFrameId = 0;
}

#if CAP_AI_PEOPLE_FLOW_PIPELINE
void CHVFPeopleFlowMigrationController::apply_new_pipeline_config(const Alarm::PeopleFlowStatistics_S &stConfig, bool bEnabled)
{
    if (m_pPeopleFlowProcessor == nullptr || m_pOutputExecutor == nullptr)
    {
        return;
    }

    /* 配置只转换一次为归一化内部配置，非法配置由适配器保持禁用 */
    AiPipeline_NS::PeopleFlow_NS::PeopleFlowConfig_S stNewConfig;
    const int nAdaptRet = m_configAdapter.adapt(stConfig, stNewConfig);
    if (nAdaptRet != OK)
    {
        dlog_warn("人流统计新链路配置非法，处理器保持禁用");
    }
    m_pPeopleFlowProcessor->apply_config(stNewConfig);

    /* 禁用时重置新链路报警状态机 */
    if (!bEnabled)
    {
        m_pOutputExecutor->reset();
    }
}
#endif // CAP_AI_PEOPLE_FLOW_PIPELINE
} // namespace HVFDetectInternal

#endif // CAP_AI_PEOPLE_STATISTICS
