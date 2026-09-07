/**
 * @FilePath     : hvf_people_flow_migration_controller.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 人流统计迁移控制器，集中编排 Legacy/New 两态链路
 */

#pragma once

#if CAP_AI_PEOPLE_FLOW_PIPELINE && !CAP_AI_PEOPLE_STATISTICS
#error "CAP_AI_PEOPLE_FLOW_PIPELINE=1 要求 CAP_AI_PEOPLE_STATISTICS=1，请检查设备画像配置"
#endif

#if CAP_AI_PEOPLE_STATISTICS

#include <cstdint>
#include <memory>
#include <mutex>

#include "alarm_define.h"
#include "detection_types.hpp"
#include "hisi_frame_image_provider.hpp"
#include "hvf_people_flow_output_executor.hpp"
#include "hvf_people_flow_processor.hpp"
#include "hvf_result_converter.hpp"
#include "people_flow_config_adapter.hpp"
#include "people_flow_processor.hpp"
#include "result_dispatcher.hpp"
#include "svp_ai_detect.h"

namespace HVFDetectInternal
{
/* 人流统计迁移控制器：Composition Root 与迁移门面 */
class CHVFPeopleFlowMigrationController
{
public:
    /**
     * @brief   : 构造迁移控制器
     */
    CHVFPeopleFlowMigrationController();

    /**
     * @brief   : 析构迁移控制器
     */
    ~CHVFPeopleFlowMigrationController() = default;

    /**
     * @brief   : 设置算法分辨率
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     * @note    : Legacy 链路按算法分辨率消费配置坐标，保持旧行为
     */
    void set_algo_resolution(int nWidth, int nHeight);

    /**
     * @brief   : 设置模型输入尺寸，创建新链路 Converter
     * @param    {const AiPipeline_NS::FrameSize_S &} stModelInputSize：模型输入分辨率
     * @return   {void}
     * @note    : 获取 model_info 失败时不得调用，新链路保持不可用
     */
    void set_model_input_size(const AiPipeline_NS::FrameSize_S &stModelInputSize);

    /**
     * @brief   : 原子应用算法总开关与完整配置
     * @param    {const Alarm::PeopleFlowStatistics_S &} stConfig：人流统计配置
     * @param    {bool} bAlgoEnabled：算法总开关
     * @return   {void}
     * @note    : lock: 同一锁内完成校验与 Legacy/New 配置应用
     */
    void apply_config(const Alarm::PeopleFlowStatistics_S &stConfig, bool bAlgoEnabled);

    /**
     * @brief   : 后续完整参数更新
     * @param    {const Alarm::PeopleFlowStatistics_S &} stConfig：人流统计配置
     * @return   {void}
     * @note    : lock: 只按完整配置应用，不单独更新半份规则
     */
    void update_parameters(const Alarm::PeopleFlowStatistics_S &stConfig);

    /**
     * @brief   : 设置统计上报器
     * @param    {const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &} pReporter：统计上报器
     * @return   {void}
     */
    void set_reporter(const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter);

    /**
     * @brief   : 同步清理当前模式涉及的全部状态
     * @return   {void}
     */
    void clear_statistics();

    /**
     * @brief   : 获取当前是否启用
     * @return   {bool} true：启用
     */
    bool is_enabled() const;

    /**
     * @brief   : 处理单帧 HVF 结果
     * @param    {const ot_aidetect_result_array &} stResult：HVF 原生结果
     * @param    {int} nChnId：通道号
     * @param    {const std::shared_ptr<ot_video_frame_info> &} pFrameInfo：当前帧共享引用
     * @param    {const AiPipeline_NS::FrameSize_S &} stSourceFrameSize：实际源帧分辨率
     * @param    {std::vector<Common::RectInfo_S> &} vstRectInfo：本帧 OSD 汇总框数组
     * @return   {int} OK：成功，ERR：新链路失败
     * @note    : lock: 与配置/清零/启停共用同一把锁串行化
     */
    int process(ot_aidetect_result_array &stResult,
                int nChnId,
                const std::shared_ptr<ot_video_frame_info> &pFrameInfo,
                const AiPipeline_NS::FrameSize_S &stSourceFrameSize,
                std::vector<Common::RectInfo_S> &vstRectInfo);

    /**
     * @brief   : 完全重置控制器
     * @return   {void}
     */
    void reset();

private:
    /**
     * @brief   : 应用归一化配置到新链路
     * @param    {const Alarm::PeopleFlowStatistics_S &} stConfig：原始配置
     * @param    {bool} bEnabled：是否启用
     * @return   {void}
     */
    void apply_new_pipeline_config(const Alarm::PeopleFlowStatistics_S &stConfig, bool bEnabled);

    /* lock: 串行化配置、清零、启停与逐帧处理 */
    mutable std::mutex m_mutex;

    /* 最近一次完整配置 */
    Alarm::PeopleFlowStatistics_S m_stConfig;
    /* 算法总开关 */
    bool m_bAlgoEnabled = false;
    /* 当前是否启用 */
    bool m_bEnabled = false;
    /* 配置是否已应用 */
    bool m_bConfigValid = false;
    /* 单调递增帧标识 */
    uint64_t m_ullFrameId = 0;
    /* 算法分辨率 */
    int m_nAlgoWidth = 0;
    int m_nAlgoHeight = 0;

    /* Legacy 处理器 */
    CHVFPeopleFlowProcessor m_legacyProcessor;

#if CAP_AI_PEOPLE_FLOW_PIPELINE
    /* 新链路结果转换器 */
    std::unique_ptr<CHisiHvfResultConverter> m_pConverter;
    /* 新链路结果分发器 */
    AiPipeline_NS::CResultDispatcher m_dispatcher;
    /* 新链路人流统计处理器，所有权归属 Dispatcher，本类只保留访问指针 */
    AiPipeline_NS::PeopleFlow_NS::CPeopleFlowProcessor *m_pPeopleFlowProcessor = nullptr;
    /* 新链路输出执行器 */
    std::unique_ptr<CHVFPeopleFlowOutputExecutor> m_pOutputExecutor;
    /* 新链路帧图片 Provider */
    CHisiFrameImageProvider m_imageProvider;
    /* 配置适配器 */
    AiPipeline_NS::PeopleFlow_NS::CPeopleFlowConfigAdapter m_configAdapter;
    /* 原生结果坐标来源 */
    NativeResultCoordinateSource_E m_enCoordinateSource = NativeResultCoordinateSource_E::MODEL_INPUT;
    /* 模型输入尺寸 */
    AiPipeline_NS::FrameSize_S m_stModelInputSize;
    /* 新链路是否就绪 */
    bool m_bNewPipelineReady = false;
#endif
};
} // namespace HVFDetectInternal

#endif // CAP_AI_PEOPLE_STATISTICS
