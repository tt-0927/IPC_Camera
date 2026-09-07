/**
 * @FilePath     : exhibition_density_output_executor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-26 15:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 人员密度新链路输出执行器，桥接共享草稿到现有 Reporter/联动
 */

#pragma once

#if CAP_AI_PEOPLE_DENSITY_PIPELINE && !CAP_AI_PEOPLE_STATISTICS
#error "CAP_AI_PEOPLE_DENSITY_PIPELINE=1 要求 CAP_AI_PEOPLE_STATISTICS=1，请检查设备画像配置"
#endif

#if CAP_AI_PEOPLE_DENSITY_PIPELINE

#include <memory>

#include "algorithm.hpp"
#include "detection_types.hpp"
#include "frame_image_provider.hpp"
#include "processor_output.hpp"

/* 人员密度输出执行器：把共享密度处理器草稿转换为现有上报/联动链路 */
class CExhibitionDensityOutputExecutor
{
public:
    /**
     * @brief   : 构造人员密度输出执行器
     */
    CExhibitionDensityOutputExecutor() = default;

    /**
     * @brief   : 设置统计上报器
     * @param    {const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &} pReporter：统计上报器
     * @return   {void}
     */
    void set_reporter(const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter);

    /**
     * @brief   : 设置帧图片 Provider
     * @param    {AiPipeline_NS::IFrameImageProvider *} pProvider：图片能力端口，可为空
     * @return   {void}
     */
    void set_image_provider(AiPipeline_NS::IFrameImageProvider *pProvider);

    /**
     * @brief   : 设置原生结果坐标尺寸
     * @param    {const AiPipeline_NS::FrameSize_S &} stSize：原生结果坐标尺寸
     * @return   {void}
     */
    void set_native_result_size(const AiPipeline_NS::FrameSize_S &stSize);

    /**
     * @brief   : 执行单帧输出适配
     * @param    {const AiPipeline_NS::ProcessorOutput_S &} stOutput：共享处理器输出
     * @return   {void}
     * @note    : 只消费 PEOPLE_DENSITY 类型的报告草稿与密度等级事件条件；
     *            OSD 叠加由人流统计输出执行器统一处理，此处不重复输出
     */
    void process(const AiPipeline_NS::ProcessorOutput_S &stOutput);

    /**
     * @brief   : 重置三级密度报警状态机
     * @return   {void}
     */
    void reset();

private:
    /**
     * @brief   : 构建人员密度统计上报结构
     * @param    {const AiPipeline_NS::StatisticsReportDraft_S &} stDraft：密度报告草稿
     * @return   {EventStatistics_NS::Report_S} 现有上报结构
     */
    EventStatistics_NS::Report_S build_report(const AiPipeline_NS::StatisticsReportDraft_S &stDraft);

    /**
     * @brief   : 构建密度联动上下文
     * @param    {const AiPipeline_NS::EventCondition_S &} stCondition：密度事件条件
     * @return   {EventTriggerContext_S} 联动上下文
     */
    EventTriggerContext_S build_event_context(const AiPipeline_NS::EventCondition_S &stCondition);

    /**
     * @brief   : 填充报告全景图负载
     * @param    {EventStatistics_NS::Report_S &} stReport：统计报告
     * @param    {const AiPipeline_NS::ProcessorOutput_S &} stOutput：共享处理器输出
     * @return   {void}
     */
    void fill_report_panorama(EventStatistics_NS::Report_S &stReport, const AiPipeline_NS::ProcessorOutput_S &stOutput);

    /* 统计上报器 */
    std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> m_pReporter;
    /* 帧图片能力端口 */
    AiPipeline_NS::IFrameImageProvider *m_pImageProvider = nullptr;
    /* 原生结果坐标尺寸 */
    AiPipeline_NS::FrameSize_S m_stNativeResultSize;
    /* 密度普通报警状态机 */
    CAlarmStateMachine m_normalAlarmStateMachine;
    /* 密度中度报警状态机 */
    CAlarmStateMachine m_mediumAlarmStateMachine;
    /* 密度严重报警状态机 */
    CAlarmStateMachine m_severeAlarmStateMachine;
};

#endif // CAP_AI_PEOPLE_DENSITY_PIPELINE
