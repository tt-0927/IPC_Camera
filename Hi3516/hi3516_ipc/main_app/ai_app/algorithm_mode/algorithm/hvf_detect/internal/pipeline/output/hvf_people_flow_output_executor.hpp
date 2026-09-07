/**
 * @FilePath     : hvf_people_flow_output_executor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 人流统计新链路输出执行器，桥接共享草稿到现有 Reporter/联动/OSD
 */

#pragma once

#if CAP_AI_PEOPLE_STATISTICS

#include <memory>

#include "algorithm.hpp"
#include "detection_types.hpp"
#include "frame_image_provider.hpp"
#include "processor_output.hpp"

namespace HVFDetectInternal
{
/* 人流统计输出执行器：把共享处理器草稿转换为现有输出链路 */
class CHVFPeopleFlowOutputExecutor
{
public:
    /**
     * @brief   : 构造输出执行器
     */
    CHVFPeopleFlowOutputExecutor();

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
     * @param    {std::vector<Common::RectInfo_S> &} vstRectInfo：本帧 OSD 汇总框数组
     * @return   {void}
     * @note    : 图片失败只记录 WARN，统计报告仍提交；不捕获异常
     */
    void process(const AiPipeline_NS::ProcessorOutput_S &stOutput, std::vector<Common::RectInfo_S> &vstRectInfo);

    /**
     * @brief   : 重置三个滞留报警状态机与报告上下文
     * @return   {void}
     */
    void reset();

private:
    /**
     * @brief   : 构建统计上报结构
     * @param    {const AiPipeline_NS::StatisticsReportDraft_S &} stDraft：统计报告草稿
     * @return   {EventStatistics_NS::Report_S} 现有上报结构
     */
    EventStatistics_NS::Report_S build_report(const AiPipeline_NS::StatisticsReportDraft_S &stDraft);

    /**
     * @brief   : 构建事件联动上下文
     * @param    {const AiPipeline_NS::EventCondition_S &} stCondition：事件条件
     * @return   {EventTriggerContext_S} 联动上下文
     */
    EventTriggerContext_S build_event_context(const AiPipeline_NS::EventCondition_S &stCondition);

    /**
     * @brief   : 填充报告图片负载
     * @param    {EventStatistics_NS::Report_S &} stReport：统计报告
     * @param    {const AiPipeline_NS::ProcessorOutput_S &} stOutput：共享处理器输出
     * @return   {void}
     */
    void fill_report_images(EventStatistics_NS::Report_S &stReport, const AiPipeline_NS::ProcessorOutput_S &stOutput);

    /* 统计上报器 */
    std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> m_pReporter;
    /* 帧图片能力端口 */
    AiPipeline_NS::IFrameImageProvider *m_pImageProvider = nullptr;
    /* 原生结果坐标尺寸 */
    AiPipeline_NS::FrameSize_S m_stNativeResultSize;
    /* 滞留普通报警状态机 */
    CAlarmStateMachine m_normalAlarmStateMachine;
    /* 滞留中度报警状态机 */
    CAlarmStateMachine m_mediumAlarmStateMachine;
    /* 滞留严重报警状态机 */
    CAlarmStateMachine m_severeAlarmStateMachine;
};
} // namespace HVFDetectInternal

#endif // CAP_AI_PEOPLE_STATISTICS
