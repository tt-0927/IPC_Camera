/**
 * @FilePath     : hvf_event_migration_controller.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-31 17:28:17
 * @Description  : HVF 事件族统一迁移控制器实现
 */

#include "hvf_event_migration_controller.hpp"

#include <chrono>

#include "IpcRet.h"
#include "dlog.h"
#include "hvf_detect_context.hpp"
#include "time_utils.h"

#if CAP_UNIFIED_EVENT_PIPELINE

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

CHVFEventMigrationController::CHVFEventMigrationController()
{
    m_pOutputExecutor = std::make_unique<CHVFEventOutputExecutor>();
    m_pOutputExecutor->set_image_provider(&m_imageProvider);

    /* 处理器所有权移交给 Dispatcher，本类保留访问指针 */
    auto pRegionProcessor = std::make_unique<AiPipeline_NS::Region_NS::CRegionProcessor>();
    m_pRegionProcessor = pRegionProcessor.get();
    m_dispatcher.register_processor(std::move(pRegionProcessor));

    /* 越界处理器注册，nOrder 排在 Region 之后 */
    auto pTripLineProcessor = std::make_unique<AiPipeline_NS::TripLine_NS::CTripLineProcessor>();
    m_pTripLineProcessor = pTripLineProcessor.get();
    m_dispatcher.register_processor(std::move(pTripLineProcessor));

    /* 驻留处理器注册（徘徊/停车），nOrder 排在 TripLine 之后 */
    auto pDwellProcessor = std::make_unique<AiPipeline_NS::Dwell_NS::CDwellProcessor>();
    m_pDwellProcessor = pDwellProcessor.get();
    m_dispatcher.register_processor(std::move(pDwellProcessor));

    /* 人脸侦测处理器注册，nOrder 排在 Dwell 之后 */
    auto pFaceProcessor = std::make_unique<AiPipeline_NS::Face_NS::CFaceProcessor>();
    m_pFaceProcessor = pFaceProcessor.get();
    m_dispatcher.register_processor(std::move(pFaceProcessor));
}

void CHVFEventMigrationController::set_algo_resolution(int nWidth, int nHeight)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nAlgoWidth = nWidth;
    m_nAlgoHeight = nHeight;
}

void CHVFEventMigrationController::set_model_input_size(const AiPipeline_NS::FrameSize_S &stModelInputSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stModelInputSize = stModelInputSize;

    /* 模型尺寸非法时新链路保持不可用，Legacy 继续可运行 */
    if (m_stModelInputSize.nWidth == 0U || m_stModelInputSize.nHeight == 0U)
    {
        m_bNewPipelineReady = false;
        dlog_warn("HVF 事件新链路不可用：模型输入尺寸非法");
        return;
    }

    /* 模型重启后尺寸可能变化，重建 Converter 以刷新坐标合同 */
    m_pConverter = std::make_unique<CHisiHvfResultConverter>(m_enCoordinateSource, m_stModelInputSize);
    m_bNewPipelineReady = true;
    dlog_info("HVF 事件新链路就绪：模型输入[%ux%u] 原生结果坐标模式[%d]",
              m_stModelInputSize.nWidth,
              m_stModelInputSize.nHeight,
              static_cast<int>(m_enCoordinateSource));
}

void CHVFEventMigrationController::apply_intrusion_config(const Alarm::FieldDetection_S &stConfig, bool bAlgoEnabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bIntrusionAlgoEnabled = bAlgoEnabled;
    m_stIntrusionConfig = stConfig;

    /* Legacy 链路：先设置使能，再按旧语义应用完整配置 */
    const bool bNowEnabled = bAlgoEnabled && stConfig.bEnable;
    m_legacyIntrusion.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyIntrusion.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_region_config();

    /* 更新总使能（含越界、徘徊、停车） */
    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在本事件使能态翻转时打 info，其他情况静默 */
    if (bNowEnabled != m_bPrevIntrusionEnabled)
    {
        dlog_info("HVF 事件控制器：区域入侵事件%s", bNowEnabled ? "已启用" : "已禁用");
        m_bPrevIntrusionEnabled = bNowEnabled;
    }
}

void CHVFEventMigrationController::apply_entrance_config(const Alarm::EntranceDetection_S &stConfig, bool bAlgoEnabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bEntranceAlgoEnabled = bAlgoEnabled;
    m_stEntranceConfig = stConfig;

    const bool bNowEnabled = bAlgoEnabled && stConfig.bEnable;
    m_legacyEnterExit.setEntranceEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyEnterExit.setEntranceAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_region_config();

    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在本事件使能态翻转时打 info，其他情况静默 */
    if (bNowEnabled != m_bPrevEntranceEnabled)
    {
        dlog_info("HVF 事件控制器：进入区域事件%s", bNowEnabled ? "已启用" : "已禁用");
        m_bPrevEntranceEnabled = bNowEnabled;
    }
}

void CHVFEventMigrationController::apply_exit_config(const Alarm::ExitingDetection_S &stConfig, bool bAlgoEnabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bExitAlgoEnabled = bAlgoEnabled;
    m_stExitConfig = stConfig;
    const bool bNowEnabled = bAlgoEnabled && stConfig.bEnable;
    m_legacyEnterExit.setExitEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyEnterExit.setExitAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_region_config();

    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在本事件使能态翻转时打 info，其他情况静默 */
    if (bNowEnabled != m_bPrevExitEnabled)
    {
        dlog_info("HVF 事件控制器：离开区域事件%s", bNowEnabled ? "已启用" : "已禁用");
        m_bPrevExitEnabled = bNowEnabled;
    }
}

void CHVFEventMigrationController::update_intrusion_parameters(const Alarm::FieldDetection_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stIntrusionConfig = stConfig;
    const bool bNowEnabled = stConfig.bEnable;
    m_legacyIntrusion.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyIntrusion.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    dlog_debug("====================");
    rebuild_and_apply_region_config();

    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在使能态翻转时打 info，其他情况静默 */
    const bool bEffectiveEnabled = m_bIntrusionAlgoEnabled && stConfig.bEnable;
    if (bEffectiveEnabled != m_bPrevIntrusionEnabled)
    {
        dlog_info("HVF 事件控制器：区域入侵事件%s（参数更新）", bEffectiveEnabled ? "已启用" : "已禁用");
        m_bPrevIntrusionEnabled = bEffectiveEnabled;
    }
}

void CHVFEventMigrationController::update_entrance_parameters(const Alarm::EntranceDetection_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stEntranceConfig = stConfig;
    const bool bNowEnabled = stConfig.bEnable;
    m_legacyEnterExit.setEntranceEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyEnterExit.setEntranceAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_region_config();

    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在使能态翻转时打 info，其他情况静默 */
    const bool bEffectiveEnabled = m_bEntranceAlgoEnabled && stConfig.bEnable;
    if (bEffectiveEnabled != m_bPrevEntranceEnabled)
    {
        dlog_info("HVF 事件控制器：进入区域事件%s（参数更新）", bEffectiveEnabled ? "已启用" : "已禁用");
        m_bPrevEntranceEnabled = bEffectiveEnabled;
    }
}

void CHVFEventMigrationController::update_exit_parameters(const Alarm::ExitingDetection_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stExitConfig = stConfig;
    const bool bNowEnabled = stConfig.bEnable;
    m_legacyEnterExit.setExitEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyEnterExit.setExitAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_region_config();

    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在使能态翻转时打 info，其他情况静默 */
    const bool bEffectiveEnabled = m_bExitAlgoEnabled && stConfig.bEnable;
    if (bEffectiveEnabled != m_bPrevExitEnabled)
    {
        dlog_info("HVF 事件控制器：离开区域事件%s（参数更新）", bEffectiveEnabled ? "已启用" : "已禁用");
        m_bPrevExitEnabled = bEffectiveEnabled;
    }
}

void CHVFEventMigrationController::apply_boundary_config(const Alarm::BoundaryDetection_S &stConfig, bool bAlgoEnabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bBoundaryAlgoEnabled = bAlgoEnabled;
    m_stBoundaryConfig = stConfig;
    /* Legacy 链路：先设置使能，再按旧语义应用完整配置 */
    const bool bNowEnabled = bAlgoEnabled && stConfig.bEnable;
    m_legacyBoundary.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyBoundary.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_trip_line_config();

    /* 更新总使能 */
    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在本事件使能态翻转时打 info，其他情况静默 */
    if (bNowEnabled != m_bPrevBoundaryEnabled)
    {
        dlog_info("HVF 事件控制器：越界侦测事件%s", bNowEnabled ? "已启用" : "已禁用");
        m_bPrevBoundaryEnabled = bNowEnabled;
    }
}

void CHVFEventMigrationController::update_boundary_parameters(const Alarm::BoundaryDetection_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stBoundaryConfig = stConfig;
    const bool bNowEnabled = stConfig.bEnable;
    m_legacyBoundary.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyBoundary.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_trip_line_config();

    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在使能态翻转时打 info，其他情况静默 */
    const bool bEffectiveEnabled = m_bBoundaryAlgoEnabled && stConfig.bEnable;
    if (bEffectiveEnabled != m_bPrevBoundaryEnabled)
    {
        dlog_info("HVF 事件控制器：越界侦测事件%s（参数更新）", bEffectiveEnabled ? "已启用" : "已禁用");
        m_bPrevBoundaryEnabled = bEffectiveEnabled;
    }
}

bool CHVFEventMigrationController::is_enabled() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bEnabled;
}

int CHVFEventMigrationController::process(ot_aidetect_result_array &stResult,
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

    /* 同一帧内只采样一次时间戳与帧标识 */
    const uint64_t ullFrameId = ++m_ullFrameId;
    const int64_t llWallMs = static_cast<int64_t>(TimeUtils_NS::get_currentTimestampMs());
    const int64_t llMonoMs = get_monotonic_timestamp_ms();

    /* Legacy 处理上下文构建器 */
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

    /* perf: 新链路处理：单次转换 + 单次分发 */
    if (m_bNewPipelineReady && m_pConverter != nullptr && m_pRegionProcessor != nullptr &&
        m_pTripLineProcessor != nullptr && m_pDwellProcessor != nullptr && m_pFaceProcessor != nullptr &&
        m_pOutputExecutor != nullptr)
    {
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
            /* Converter 失败只跳过本帧输出，不回退为同帧 Legacy */
            return nConvertRet;
        }

        AiPipeline_NS::ProcessorOutput_S stOutput;
        m_dispatcher.dispatch(stBatch, stOutput);

        /* 新链路执行外部输出，图片编码期间持有 Frame Lease */
        /* memory: set_frame 持有帧引用，clear_frame 释放 */
        m_imageProvider.set_frame(pFrameInfo);
        m_pOutputExecutor->set_native_result_size(m_pConverter->native_result_size());
        m_pOutputExecutor->process(stOutput, vstRectInfo);
        m_imageProvider.clear_frame();
        return OK;
    }

    /* Legacy 或新链路未就绪：运行旧处理器 */
    HVFDetectInternal::SHVFProcessContext stLegacyContext = build_legacy_context();

    if (m_legacyIntrusion.isEnabled())
    {
        m_legacyIntrusion.process(stLegacyContext);
    }
    if (m_legacyEnterExit.isEntranceEnabled())
    {
        m_legacyEnterExit.processEntrance(stLegacyContext);
    }
    if (m_legacyEnterExit.isExitEnabled())
    {
        m_legacyEnterExit.processExit(stLegacyContext);
    }
    if (m_legacyBoundary.isEnabled())
    {
        m_legacyBoundary.process(stLegacyContext);
    }
    if (m_legacyLoitering.isEnabled())
    {
        m_legacyLoitering.process(stLegacyContext);
    }
    if (m_legacyParking.isEnabled())
    {
        m_legacyParking.process(stLegacyContext);
    }
    if (m_legacyFace.isEnabled())
    {
        m_legacyFace.process(stLegacyContext);
    }

    return OK;
}

void CHVFEventMigrationController::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pRegionProcessor != nullptr)
    {
        m_pRegionProcessor->reset();
    }
    if (m_pTripLineProcessor != nullptr)
    {
        m_pTripLineProcessor->reset();
    }
    if (m_pDwellProcessor != nullptr)
    {
        m_pDwellProcessor->reset();
    }
    if (m_pFaceProcessor != nullptr)
    {
        m_pFaceProcessor->reset();
    }
    if (m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->reset();
    }
    m_imageProvider.clear_frame();
    m_bEnabled = false;
    m_bIntrusionAlgoEnabled = false;
    m_bEntranceAlgoEnabled = false;
    m_bExitAlgoEnabled = false;
    m_bBoundaryAlgoEnabled = false;
    m_bLoiteringAlgoEnabled = false;
    m_bParkingAlgoEnabled = false;
    m_bFaceAlgoEnabled = false;
    m_bPrevIntrusionEnabled = false;
    m_bPrevEntranceEnabled = false;
    m_bPrevExitEnabled = false;
    m_bPrevBoundaryEnabled = false;
    m_bPrevLoiteringEnabled = false;
    m_bPrevParkingEnabled = false;
    m_bPrevFaceEnabled = false;
    m_ullFrameId = 0;
}

void CHVFEventMigrationController::rebuild_and_apply_region_config()
{
    if (m_pRegionProcessor == nullptr)
    {
        return;
    }

    /* 合并三类配置为平台无关原始配置 */
    AiPipeline_NS::Region_NS::RawRegionConfig_S stRawConfig;

    /* 仅当某事件既使能又配置了至少一条非占位规则时,才视为有效启用
     * 全 (0,0) 占位规则归类为"未配置"而非"无效",避免误报"配置无效"warn */
    const bool bIntrusionEffective = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable &&
                                      !is_all_placeholder(m_stIntrusionConfig));
    const bool bEntranceEffective = (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable &&
                                     !is_all_placeholder(m_stEntranceConfig));
    const bool bExitEffective = (m_bExitAlgoEnabled && m_stExitConfig.bEnable &&
                                 !is_all_placeholder(m_stExitConfig));
    stRawConfig.bEnabled = bIntrusionEffective || bEntranceEffective || bExitEffective;

    if (bIntrusionEffective)
    {
        convert_intrusion_rules(m_stIntrusionConfig, stRawConfig.vecRules);
    }
    if (bEntranceEffective)
    {
        convert_entrance_rules(m_stEntranceConfig, stRawConfig.vecRules);
    }
    if (bExitEffective)
    {
        convert_exit_rules(m_stExitConfig, stRawConfig.vecRules);
    }

    /* 配置适配器归一化 */
    AiPipeline_NS::Region_NS::RegionConfig_S stNewConfig;
    const int nAdaptRet = m_regionConfigAdapter.adapt(stRawConfig, stNewConfig);
    /* 仅在事件已使能但配置无效时 warn；正常禁用（无规则导致 adapt 返回 ERR_PARAM）静默 */
    if (nAdaptRet != OK && stRawConfig.bEnabled)
    {
        dlog_warn("HVF 事件新链路区域配置适配失败：事件已使能但配置无效，处理器保持禁用");
    }
    m_pRegionProcessor->apply_config(stNewConfig);

    /* 禁用时重置区域事件族报警状态机（入侵/进入/离开），不影响其他事件族 */
    if (!stNewConfig.bEnabled && m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->reset_region();
    }
}

void CHVFEventMigrationController::convert_intrusion_rules(const Alarm::FieldDetection_S &stSrc,
                                                           std::vector<AiPipeline_NS::Region_NS::RawRegionRule_S> &vecOut) const
{
    int nRuleId = 0;
    for (const auto &stRule : stSrc.aRule)
    {
        AiPipeline_NS::Region_NS::RawRegionRule_S stRaw;
        stRaw.enEventType = Event::Type_E::INTRUSION;
        stRaw.nTimeThresholdSec = stRule.nTimeThreshold;
        stRaw.nSensitivity = stRule.nSensitivity;
        stRaw.nDetectionTargetMask = convert_detection_targets(stRule.aDetectionTarget);
        stRaw.nRuleId = nRuleId++;

        if (!convert_region_polygon(stRule.stRegion, stRaw.vecPolygon))
        {
            continue;
        }
        vecOut.emplace_back(std::move(stRaw));
    }
}

void CHVFEventMigrationController::convert_entrance_rules(const Alarm::EntranceDetection_S &stSrc,
                                                          std::vector<AiPipeline_NS::Region_NS::RawRegionRule_S> &vecOut) const
{
    /* 进入区域规则 ID 从 0 起，与旧链路数组下标对齐；
     * 状态隔离由 RegionTrackKey_S 的事件类型维度保证，无需人为偏移 */
    int nRuleId = 0;
    for (const auto &stRule : stSrc.aRule)
    {
        AiPipeline_NS::Region_NS::RawRegionRule_S stRaw;
        stRaw.enEventType = Event::Type_E::ENTER_REGION;
        stRaw.nTimeThresholdSec = 0;
        stRaw.nSensitivity = stRule.nSensitivity;
        stRaw.nDetectionTargetMask = convert_detection_targets(stRule.aDetectionTarget);
        stRaw.nRuleId = nRuleId++;

        if (!convert_region_polygon(stRule.stRegion, stRaw.vecPolygon))
        {
            continue;
        }
        vecOut.emplace_back(std::move(stRaw));
    }
}

void CHVFEventMigrationController::convert_exit_rules(const Alarm::ExitingDetection_S &stSrc,
                                                      std::vector<AiPipeline_NS::Region_NS::RawRegionRule_S> &vecOut) const
{
    /* 离开区域规则 ID 从 0 起，与旧链路数组下标对齐；
     * 状态隔离由 RegionTrackKey_S 的事件类型维度保证，无需人为偏移 */
    int nRuleId = 0;
    for (const auto &stRule : stSrc.aRule)
    {
        AiPipeline_NS::Region_NS::RawRegionRule_S stRaw;
        stRaw.enEventType = Event::Type_E::LEAVE_REGION;
        stRaw.nTimeThresholdSec = 0;
        stRaw.nSensitivity = stRule.nSensitivity;
        stRaw.nDetectionTargetMask = convert_detection_targets(stRule.aDetectionTarget);
        stRaw.nRuleId = nRuleId++;

        if (!convert_region_polygon(stRule.stRegion, stRaw.vecPolygon))
        {
            continue;
        }
        vecOut.emplace_back(std::move(stRaw));
    }
}

bool CHVFEventMigrationController::convert_region_polygon(const Alarm::Region_S &stRegion,
                                                          std::vector<AiPipeline_NS::Geometry_NS::Point_S> &vecOut) const
{
    if (!stRegion.IsValid())
    {
        return false;
    }

    /* 占位空规则识别：Web 下发的规则数组长度固定（如区域 4 条），未配置的规则顶点全为 (0,0)，
     * 构成零面积退化多边形。这类规则在旧链路中因检测目标掩码为空恒不触发，属惰性占位。
     * 此处按包围盒宽高判退化并静默跳过，避免退化多边形误报“多边形自交/规则无效”噪音日志 */
    float fMinX = stRegion.aPoint[0].fX;
    float fMaxX = stRegion.aPoint[0].fX;
    float fMinY = stRegion.aPoint[0].fY;
    float fMaxY = stRegion.aPoint[0].fY;
    for (unsigned int i = 1; i < stRegion.nPointNum; ++i)
    {
        const float fX = stRegion.aPoint[i].fX;
        const float fY = stRegion.aPoint[i].fY;
        fMinX = (fX < fMinX) ? fX : fMinX;
        fMaxX = (fX > fMaxX) ? fX : fMaxX;
        fMinY = (fY < fMinY) ? fY : fMinY;
        fMaxY = (fY > fMaxY) ? fY : fMaxY;
    }
    /* 包围盒宽或高不足 1 像素视为退化占位规则 */
    if ((fMaxX - fMinX) < 1.0F || (fMaxY - fMinY) < 1.0F)
    {
        return false;
    }

    vecOut.clear();
    vecOut.reserve(stRegion.nPointNum);
    for (unsigned int i = 0; i < stRegion.nPointNum; ++i)
    {
        /* 像素坐标直接使用，归一化由 config_adapter 完成 */
        AiPipeline_NS::Geometry_NS::Point_S stPoint;
        stPoint.dX = static_cast<double>(stRegion.aPoint[i].fX);
        stPoint.dY = static_cast<double>(stRegion.aPoint[i].fY);
        vecOut.emplace_back(stPoint);
    }
    return true;
}

bool CHVFEventMigrationController::is_region_placeholder(const Alarm::Region_S &stRegion) const
{
    /* 全零顶点视为占位（Web 下发规则数组中未配置的规则默认顶点全为 (0,0)） */
    for (unsigned int i = 0; i < stRegion.nPointNum && i < stRegion.aPoint.size(); ++i)
    {
        if (stRegion.aPoint[i].fX != 0.0F || stRegion.aPoint[i].fY != 0.0F)
        {
            return false;
        }
    }
    return true;
}

bool CHVFEventMigrationController::is_all_placeholder(const Alarm::FieldDetection_S &stConfig) const
{
    for (const auto &stRule : stConfig.aRule)
    {
        if (!is_region_placeholder(stRule.stRegion))
        {
            return false;
        }
    }
    return true;
}

bool CHVFEventMigrationController::is_all_placeholder(const Alarm::EntranceDetection_S &stConfig) const
{
    for (const auto &stRule : stConfig.aRule)
    {
        if (!is_region_placeholder(stRule.stRegion))
        {
            return false;
        }
    }
    return true;
}

bool CHVFEventMigrationController::is_all_placeholder(const Alarm::ExitingDetection_S &stConfig) const
{
    for (const auto &stRule : stConfig.aRule)
    {
        if (!is_region_placeholder(stRule.stRegion))
        {
            return false;
        }
    }
    return true;
}

bool CHVFEventMigrationController::is_all_placeholder(const Alarm::BoundaryDetection_S &stConfig) const
{
    for (const auto &stRule : stConfig.aRule)
    {
        if (stRule.stStartPos.fX != 0.0F || stRule.stStartPos.fY != 0.0F ||
            stRule.stEndPos.fX != 0.0F || stRule.stEndPos.fY != 0.0F)
        {
            return false;
        }
    }
    return true;
}

bool CHVFEventMigrationController::is_all_placeholder(const Alarm::LoiteringDetection_S &stConfig) const
{
    for (const auto &stRule : stConfig.aRule)
    {
        if (!is_region_placeholder(stRule.stRegion))
        {
            return false;
        }
    }
    return true;
}

bool CHVFEventMigrationController::is_all_placeholder(const Alarm::ParkingDetection_S &stConfig) const
{
    for (const auto &stRule : stConfig.aRule)
    {
        if (!is_region_placeholder(stRule.stRegion))
        {
            return false;
        }
    }
    return true;
}

void CHVFEventMigrationController::rebuild_and_apply_trip_line_config()
{
    if (m_pTripLineProcessor == nullptr)
    {
        return;
    }

    /* 构造平台无关原始配置 */
    AiPipeline_NS::TripLine_NS::RawTripLineConfig_S stRawConfig;

    /* 仅当越界事件既使能又配置了至少一条非占位规则时,才视为有效启用 */
    const bool bBoundaryEffective = (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable &&
                                     !is_all_placeholder(m_stBoundaryConfig));
    stRawConfig.bEnabled = bBoundaryEffective;

    if (bBoundaryEffective)
    {
        convert_boundary_rules(m_stBoundaryConfig, stRawConfig.vecRules);
    }

    /* 配置适配器归一化 */
    AiPipeline_NS::TripLine_NS::TripLineConfig_S stNewConfig;
    const int nAdaptRet = m_tripLineConfigAdapter.adapt(stRawConfig, stNewConfig);
    /* 仅在事件已使能但配置无效时 warn；正常禁用（无规则导致 adapt 返回 ERR_PARAM）静默 */
    if (nAdaptRet != OK && stRawConfig.bEnabled)
    {
        dlog_warn("HVF 事件新链路越界配置适配失败：事件已使能但配置无效，处理器保持禁用");
    }
    m_pTripLineProcessor->apply_config(stNewConfig);

    /* 禁用时重置越界报警状态机，不影响其他事件族 */
    if (!stNewConfig.bEnabled && m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->reset_trip_line();
    }
}

void CHVFEventMigrationController::convert_boundary_rules(const Alarm::BoundaryDetection_S &stSrc,
                                                          std::vector<AiPipeline_NS::TripLine_NS::RawTripLineRule_S> &vecOut) const
{
    int nRuleId = 0;
    for (const auto &stRule : stSrc.aRule)
    {
        AiPipeline_NS::TripLine_NS::RawTripLineRule_S stRaw;
        /* 像素坐标直接使用，归一化由 config_adapter 完成；
         * 旧 BoundaryPlane_S.stStartPos/stEndPos 已是 1920×1080 基准像素坐标 */
        stRaw.stLineStart.dX = static_cast<double>(stRule.stStartPos.fX);
        stRaw.stLineStart.dY = static_cast<double>(stRule.stStartPos.fY);
        stRaw.stLineEnd.dX = static_cast<double>(stRule.stEndPos.fX);
        stRaw.stLineEnd.dY = static_cast<double>(stRule.stEndPos.fY);
        stRaw.enCrossDirection = stRule.enCrossDirection;
        stRaw.nSensitivity = stRule.nSensitivity;
        stRaw.nDetectionTargetMask = convert_detection_targets(stRule.aDetectionTarget);
        stRaw.nRuleId = nRuleId++;
        vecOut.emplace_back(std::move(stRaw));
    }
}

uint32_t CHVFEventMigrationController::convert_detection_targets(const std::vector<int> &vecTargets) const
{
    if (vecTargets.empty())
    {
        /* 空列表表示未配置检测目标，对应旧 is_target_match 返回 false（拒绝所有） */
        return 0U;
    }

    uint32_t nMask = 0U;
    for (int nTarget : vecTargets)
    {
        switch (static_cast<Alarm::DetectionTarget_E>(nTarget))
        {
        case Alarm::HUMAN_DETECTION:
            nMask |= AiPipeline_NS::TripLine_NS::DETECTION_TARGET_HUMAN;
            break;
        case Alarm::CAR_DETECTION:
            nMask |= AiPipeline_NS::TripLine_NS::DETECTION_TARGET_VEHICLE;
            break;
        default:
            nMask |= AiPipeline_NS::TripLine_NS::DETECTION_TARGET_OTHER;
            break;
        }
    }
    return nMask;
}

void CHVFEventMigrationController::apply_loitering_config(const Alarm::LoiteringDetection_S &stConfig, bool bAlgoEnabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bLoiteringAlgoEnabled = bAlgoEnabled;
    m_stLoiteringConfig = stConfig;
    /* Legacy 链路：先设置使能，再按旧语义应用完整配置 */
    const bool bNowEnabled = bAlgoEnabled && stConfig.bEnable;
    m_legacyLoitering.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyLoitering.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_dwell_config();

    /* 更新总使能 */
    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在本事件使能态翻转时打 info，其他情况静默 */
    if (bNowEnabled != m_bPrevLoiteringEnabled)
    {
        dlog_info("HVF 事件控制器：徘徊侦测事件%s", bNowEnabled ? "已启用" : "已禁用");
        m_bPrevLoiteringEnabled = bNowEnabled;
    }
}

void CHVFEventMigrationController::apply_parking_config(const Alarm::ParkingDetection_S &stConfig, bool bAlgoEnabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bParkingAlgoEnabled = bAlgoEnabled;
    m_stParkingConfig = stConfig;
    /* Legacy 链路：先设置使能，再按旧语义应用完整配置 */
    const bool bNowEnabled = bAlgoEnabled && stConfig.bEnable;
    m_legacyParking.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyParking.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_dwell_config();

    /* 更新总使能 */
    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在本事件使能态翻转时打 info，其他情况静默 */
    if (bNowEnabled != m_bPrevParkingEnabled)
    {
        dlog_info("HVF 事件控制器：停车侦测事件%s", bNowEnabled ? "已启用" : "已禁用");
        m_bPrevParkingEnabled = bNowEnabled;
    }
}

void CHVFEventMigrationController::update_loitering_parameters(const Alarm::LoiteringDetection_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stLoiteringConfig = stConfig;
    const bool bNowEnabled = stConfig.bEnable;
    m_legacyLoitering.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyLoitering.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_dwell_config();

    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在使能态翻转时打 info，其他情况静默 */
    const bool bEffectiveEnabled = m_bLoiteringAlgoEnabled && stConfig.bEnable;
    if (bEffectiveEnabled != m_bPrevLoiteringEnabled)
    {
        dlog_info("HVF 事件控制器：徘徊侦测事件%s（参数更新）", bEffectiveEnabled ? "已启用" : "已禁用");
        m_bPrevLoiteringEnabled = bEffectiveEnabled;
    }
}

void CHVFEventMigrationController::update_parking_parameters(const Alarm::ParkingDetection_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stParkingConfig = stConfig;
    const bool bNowEnabled = stConfig.bEnable;
    m_legacyParking.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyParking.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_dwell_config();

    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在使能态翻转时打 info，其他情况静默 */
    const bool bEffectiveEnabled = m_bParkingAlgoEnabled && stConfig.bEnable;
    if (bEffectiveEnabled != m_bPrevParkingEnabled)
    {
        dlog_info("HVF 事件控制器：停车侦测事件%s（参数更新）", bEffectiveEnabled ? "已启用" : "已禁用");
        m_bPrevParkingEnabled = bEffectiveEnabled;
    }
}

void CHVFEventMigrationController::rebuild_and_apply_dwell_config()
{
    if (m_pDwellProcessor == nullptr)
    {
        return;
    }

    /* 合并徘徊/停车配置为平台无关原始配置 */
    AiPipeline_NS::Dwell_NS::RawDwellConfig_S stRawConfig;

    /* 仅当某事件既使能又配置了至少一条非占位规则时,才视为有效启用 */
    const bool bLoiteringEffective = (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable &&
                                      !is_all_placeholder(m_stLoiteringConfig));
    const bool bParkingEffective = (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable &&
                                    !is_all_placeholder(m_stParkingConfig));
    stRawConfig.bEnabled = bLoiteringEffective || bParkingEffective;

    if (bLoiteringEffective)
    {
        convert_loitering_rules(m_stLoiteringConfig, stRawConfig.vecRules);
    }
    if (bParkingEffective)
    {
        convert_parking_rules(m_stParkingConfig, stRawConfig.vecRules);
    }

    /* 配置适配器归一化 */
    AiPipeline_NS::Dwell_NS::DwellConfig_S stNewConfig;
    const int nAdaptRet = m_dwellConfigAdapter.adapt(stRawConfig, stNewConfig);
    /* 仅在事件已使能但配置无效时 warn；正常禁用（无规则导致 adapt 返回 ERR_PARAM）静默 */
    if (nAdaptRet != OK && stRawConfig.bEnabled)
    {
        dlog_warn("HVF 事件新链路驻留配置适配失败：事件已使能但配置无效，处理器保持禁用");
    }
    m_pDwellProcessor->apply_config(stNewConfig);

    /* 禁用时重置驻留事件族报警状态机（徘徊/停车），不影响其他事件族 */
    if (!stNewConfig.bEnabled && m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->reset_dwell();
    }
}

void CHVFEventMigrationController::convert_loitering_rules(const Alarm::LoiteringDetection_S &stSrc,
                                                          std::vector<AiPipeline_NS::Dwell_NS::RawDwellRule_S> &vecOut) const
{
    int nRuleId = 0;
    for (const auto &stRule : stSrc.aRule)
    {
        AiPipeline_NS::Dwell_NS::RawDwellRule_S stRaw;
        stRaw.enEventType = Event::Type_E::LOITERING_DETECT;
        stRaw.nTimeThresholdSec = stRule.nTimeThreshold;
        stRaw.nSensitivity = stRule.nSensitivity;
        /* 徘徊固定 HUMAN mask，对齐旧行为（旧徘徊硬编码 HUMAN，aDetectionTarget 未生效） */
        stRaw.nDetectionTargetMask = AiPipeline_NS::Dwell_NS::DETECTION_TARGET_HUMAN;
        stRaw.nRuleId = nRuleId++;

        if (!convert_region_polygon(stRule.stRegion, stRaw.vecPolygon))
        {
            continue;
        }
        vecOut.emplace_back(std::move(stRaw));
    }
}

void CHVFEventMigrationController::convert_parking_rules(const Alarm::ParkingDetection_S &stSrc,
                                                        std::vector<AiPipeline_NS::Dwell_NS::RawDwellRule_S> &vecOut) const
{
    int nRuleId = 0;
    for (const auto &stRule : stSrc.aRule)
    {
        AiPipeline_NS::Dwell_NS::RawDwellRule_S stRaw;
        stRaw.enEventType = Event::Type_E::PARKING_DETECT;
        stRaw.nTimeThresholdSec = stRule.nTimeThreshold;
        stRaw.nSensitivity = stRule.nSensitivity;
        /* 停车固定 VEHICLE mask，对齐旧行为（旧停车硬编码 VEHICLE） */
        stRaw.nDetectionTargetMask = AiPipeline_NS::Dwell_NS::DETECTION_TARGET_VEHICLE;
        stRaw.nRuleId = nRuleId++;

        if (!convert_region_polygon(stRule.stRegion, stRaw.vecPolygon))
        {
            continue;
        }
        vecOut.emplace_back(std::move(stRaw));
    }
}

void CHVFEventMigrationController::apply_face_config(const Alarm::FaceDetection_S &stConfig, bool bAlgoEnabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bFaceAlgoEnabled = bAlgoEnabled;
    m_stFaceConfig = stConfig;
    /* Legacy 链路：先设置使能，再按旧语义应用完整配置 */
    const bool bNowEnabled = bAlgoEnabled && stConfig.bEnable;
    m_legacyFace.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyFace.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_face_config();

    /* 更新总使能（含所有事件） */
    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在本事件使能态翻转时打 info，其他情况静默 */
    if (bNowEnabled != m_bPrevFaceEnabled)
    {
        dlog_info("HVF 事件控制器：人脸侦测事件%s", bNowEnabled ? "已启用" : "已禁用");
        m_bPrevFaceEnabled = bNowEnabled;
    }
}

void CHVFEventMigrationController::update_face_parameters(const Alarm::FaceDetection_S &stConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stFaceConfig = stConfig;
    const bool bNowEnabled = stConfig.bEnable;
    m_legacyFace.setEnabled(bNowEnabled);
    if (bNowEnabled)
    {
        m_legacyFace.setAlgoParamCfg(stConfig, m_nAlgoWidth, m_nAlgoHeight);
    }

    rebuild_and_apply_face_config();

    m_bEnabled = (m_bIntrusionAlgoEnabled && m_stIntrusionConfig.bEnable) ||
                 (m_bEntranceAlgoEnabled && m_stEntranceConfig.bEnable) ||
                 (m_bExitAlgoEnabled && m_stExitConfig.bEnable) ||
                 (m_bBoundaryAlgoEnabled && m_stBoundaryConfig.bEnable) ||
                 (m_bLoiteringAlgoEnabled && m_stLoiteringConfig.bEnable) ||
                 (m_bParkingAlgoEnabled && m_stParkingConfig.bEnable) ||
                 (m_bFaceAlgoEnabled && m_stFaceConfig.bEnable);

    /* 仅在使能态翻转时打 info，其他情况静默 */
    const bool bEffectiveEnabled = m_bFaceAlgoEnabled && stConfig.bEnable;
    if (bEffectiveEnabled != m_bPrevFaceEnabled)
    {
        dlog_info("HVF 事件控制器：人脸侦测事件%s（参数更新）", bEffectiveEnabled ? "已启用" : "已禁用");
        m_bPrevFaceEnabled = bEffectiveEnabled;
    }
}

void CHVFEventMigrationController::rebuild_and_apply_face_config()
{
    if (m_pFaceProcessor == nullptr)
    {
        return;
    }

    /* 构建平台无关原始配置 */
    AiPipeline_NS::Face_NS::RawFaceConfig_S stRawConfig;
    stRawConfig.bEnabled = m_bFaceAlgoEnabled && m_stFaceConfig.bEnable;
    stRawConfig.bDynamicAnalysisEnable = m_stFaceConfig.bDynamicAnalysisEnable;
    stRawConfig.nSensitivity = m_stFaceConfig.nSensitivity;

    /* 单区域多边形转换；全 (0,0) 占位区域归类为"未配置",静默禁用,不报"转换失败" */
    if (stRawConfig.bEnabled)
    {
        if (is_region_placeholder(m_stFaceConfig.stRegion))
        {
            stRawConfig.bEnabled = false;
        }
        else if (!convert_region_polygon(m_stFaceConfig.stRegion, stRawConfig.vecPolygon))
        {
            dlog_warn("人脸配置区域多边形转换失败");
            stRawConfig.bEnabled = false;
        }
    }

    /* 配置适配器归一化 */
    AiPipeline_NS::Face_NS::FaceConfig_S stNewConfig;
    const int nAdaptRet = m_faceConfigAdapter.adapt(stRawConfig, stNewConfig);
    if (nAdaptRet != OK && stRawConfig.bEnabled)
    {
        dlog_warn("人脸配置归一化失败，处理器保持禁用");
    }

    /* 应用归一化配置到共享层处理器 */
    const int nApplyRet = m_pFaceProcessor->apply_config(stNewConfig);
    if (nApplyRet != OK)
    {
        dlog_warn("人脸处理器应用配置失败");
    }

    /* 禁用时重置人脸侦测报警状态机，不影响其他事件族 */
    if (!stNewConfig.bEnabled && m_pOutputExecutor != nullptr)
    {
        m_pOutputExecutor->reset_face();
    }

}

} // namespace HVFDetectInternal

#endif // CAP_UNIFIED_EVENT_PIPELINE
