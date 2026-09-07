/**
 * @FilePath     : hvf_event_output_executor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : HVF 事件族新链路输出执行器，按事件类型分派到对应 CAlarmStateMachine
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <memory>

#include "algorithm.hpp"
#include "detection_types.hpp"
#include "frame_image_provider.hpp"
#include "processor_output.hpp"

namespace HVFDetectInternal
{
/**
 * @brief   : HVF 事件族输出执行器
 * @note    : 消费 ProcessorOutput_S 的 vecEventConditions，按 enEventType 分派到
 *            INTRUSION/ENTER_REGION/LEAVE_REGION/LINE_CROSSING 四个独立 CAlarmStateMachine，
 *            复用 CEventLinkage 联动喉点。vecOverlayItems 反归一化后追加到 OSD 汇总框。
 *            LINE_CROSSING 走三重门控图片编码（bIsAlarm→canStartAlarm→tvsdk_required）。
 */
class CHVFEventOutputExecutor
{
public:
    /**
     * @brief   : 构造事件输出执行器
     */
    CHVFEventOutputExecutor();

    /**
     * @brief   : 设置帧图片 Provider
     * @param    {AiPipeline_NS::IFrameImageProvider *} pProvider：图片能力端口
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
     */
    void process(const AiPipeline_NS::ProcessorOutput_S &stOutput, std::vector<Common::RectInfo_S> &vstRectInfo);

    /**
     * @brief   : 重置所有报警状态机（全量重置）
     * @return   {void}
     * @note    : 供 MigrationController::reset() 完全重置控制器用
     */
    void reset();

    /**
     * @brief   : 重置区域事件族报警状态机（入侵/进入/离开）
     * @return   {void}
     * @note    : 仅重置 m_intrusionAlarmStateMachine + m_enterAlarmStateMachine + m_leaveAlarmStateMachine，
     *            不影响其他事件族状态机
     */
    void reset_region();

    /**
     * @brief   : 重置越界事件报警状态机
     * @return   {void}
     * @note    : 仅重置 m_lineCrossingAlarmStateMachine，不影响其他事件族状态机
     */
    void reset_trip_line();

    /**
     * @brief   : 重置驻留事件族报警状态机（徘徊/停车）
     * @return   {void}
     * @note    : 仅重置 m_loiteringAlarmStateMachine + m_parkingAlarmStateMachine，
     *            不影响其他事件族状态机
     */
    void reset_dwell();

    /**
     * @brief   : 重置人脸侦测报警状态机
     * @return   {void}
     * @note    : 仅重置 m_faceAlarmStateMachine，不影响其他事件族状态机
     */
    void reset_face();

private:
    /**
     * @brief   : 构建事件联动上下文
     * @param    {const AiPipeline_NS::EventCondition_S &} stCondition：事件条件
     * @return   {EventTriggerContext_S} 联动上下文
     */
    EventTriggerContext_S build_event_context(const AiPipeline_NS::EventCondition_S &stCondition);

#ifdef ENABLE_TVSDK_SRC
    /**
     * @brief   : 填充 TVSDK 结构化字段（nTargetId/nObjectType/fConfidence/nLeft..nBottom）
     * @param    {const AiPipeline_NS::EventCondition_S &} stCondition：事件条件（含触发目标信息）
     * @param    {EventTriggerContext_S &} stContext：输出联动上下文
     * @return   {void}
     * @note    : 对齐旧 fill_hvf_tvsdk_event_context 的结构化字段填充行为
     */
    void fill_tvsdk_structured_fields(const AiPipeline_NS::EventCondition_S &stCondition,
                                      EventTriggerContext_S &stContext);

    /**
     * @brief   : 编码事件图片（全景图 + 目标特写图）
     * @param    {const AiPipeline_NS::EventCondition_S &} stCondition：事件条件（含目标框）
     * @param    {EventTriggerContext_S &} stContext：输出联动上下文
     * @return   {void}
     * @note    : 对齐旧 fill_hvf_tvsdk_event_context 的图片编码行为
     */
    void encode_event_images(const AiPipeline_NS::EventCondition_S &stCondition,
                             EventTriggerContext_S &stContext);
#endif

    /* 帧图片能力端口 */
    AiPipeline_NS::IFrameImageProvider *m_pImageProvider = nullptr;
    /* 原生结果坐标尺寸 */
    AiPipeline_NS::FrameSize_S m_stNativeResultSize;
    /* 区域入侵报警状态机 */
    CAlarmStateMachine m_intrusionAlarmStateMachine;
    /* 进入区域报警状态机 */
    CAlarmStateMachine m_enterAlarmStateMachine;
    /* 离开区域报警状态机 */
    CAlarmStateMachine m_leaveAlarmStateMachine;
    /* 越界报警状态机（三重门控图片编码依赖其 canStartAlarm） */
    CAlarmStateMachine m_lineCrossingAlarmStateMachine;
    /* 徘徊侦测报警状态机 */
    CAlarmStateMachine m_loiteringAlarmStateMachine;
    /* 停车侦测报警状态机 */
    CAlarmStateMachine m_parkingAlarmStateMachine;
    /* 人脸侦测报警状态机（三重门控图片编码依赖其 canStartAlarm） */
    CAlarmStateMachine m_faceAlarmStateMachine;
};
} // namespace HVFDetectInternal

#endif // CAP_UNIFIED_EVENT_PIPELINE
