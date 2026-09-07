/**
 * @FilePath     : hvf_event_output_executor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : HVF 事件族新链路输出执行器实现（含越界三重门控图片编码）
 */

#include "hvf_event_output_executor.hpp"

#include <string>

#include "IpcRet.h"
#include "dlog.h"
#include "event_linkage_types.h"
#include "normalized_geometry.hpp"
#ifdef ENABLE_TVSDK_SRC
#include "video_frame_jpeg_encoder.hpp"
#endif

#if CAP_UNIFIED_EVENT_PIPELINE

namespace HVFDetectInternal
{
CHVFEventOutputExecutor::CHVFEventOutputExecutor() = default;

void CHVFEventOutputExecutor::set_image_provider(AiPipeline_NS::IFrameImageProvider *pProvider)
{
    m_pImageProvider = pProvider;
}

void CHVFEventOutputExecutor::set_native_result_size(const AiPipeline_NS::FrameSize_S &stSize)
{
    m_stNativeResultSize = stSize;
}

void CHVFEventOutputExecutor::process(const AiPipeline_NS::ProcessorOutput_S &stOutput,
                                      std::vector<Common::RectInfo_S> &vstRectInfo)
{
    /* 区域三事件 + 越界联动分派 */
    for (const auto &stCondition : stOutput.vecEventConditions)
    {
        switch (stCondition.enEventType)
        {
        case Event::Type_E::INTRUSION:
        {
            EventTriggerContext_S stContext = build_event_context(stCondition);
#ifdef ENABLE_TVSDK_SRC
            /* 区域事件无三重门控优化，事件触发即填充结构化字段 + 编码图片
             * 对齐旧 process_region_detection 行为：bIsAlarm && pAlarmObject != nullptr */
            if (stCondition.bConditionMet && stCondition.bHasTarget)
            {
                fill_tvsdk_structured_fields(stCondition, stContext);
                encode_event_images(stCondition, stContext);
            }
#endif
            m_intrusionAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        }
        case Event::Type_E::ENTER_REGION:
        {
            EventTriggerContext_S stContext = build_event_context(stCondition);
#ifdef ENABLE_TVSDK_SRC
            /* 进入区域事件：对齐旧 process_region_enter_exit_detection 行为 */
            if (stCondition.bConditionMet && stCondition.bHasTarget)
            {
                fill_tvsdk_structured_fields(stCondition, stContext);
                encode_event_images(stCondition, stContext);
            }
#endif
            m_enterAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        }
        case Event::Type_E::LEAVE_REGION:
        {
            EventTriggerContext_S stContext = build_event_context(stCondition);
#ifdef ENABLE_TVSDK_SRC
            /* 离开区域事件：对齐旧 process_region_enter_exit_detection 行为 */
            if (stCondition.bConditionMet && stCondition.bHasTarget)
            {
                fill_tvsdk_structured_fields(stCondition, stContext);
                encode_event_images(stCondition, stContext);
            }
#endif
            m_leaveAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        }
        case Event::Type_E::LOITERING_DETECT:
        {
            /* 徘徊侦测事件：区域类门控（bConditionMet && bHasTarget），无三重门控
             * 对齐旧 process_region_detection 行为：bIsAlarm && pAlarmObject != nullptr */
            EventTriggerContext_S stContext = build_event_context(stCondition);
#ifdef ENABLE_TVSDK_SRC
            if (stCondition.bConditionMet && stCondition.bHasTarget)
            {
                fill_tvsdk_structured_fields(stCondition, stContext);
                encode_event_images(stCondition, stContext);
            }
#endif
            m_loiteringAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        }
        case Event::Type_E::PARKING_DETECT:
        {
            /* 停车侦测事件：区域类门控（bConditionMet && bHasTarget），无三重门控
             * 对齐旧 process_region_detection 行为：bIsAlarm && pAlarmObject != nullptr */
            EventTriggerContext_S stContext = build_event_context(stCondition);
#ifdef ENABLE_TVSDK_SRC
            if (stCondition.bConditionMet && stCondition.bHasTarget)
            {
                fill_tvsdk_structured_fields(stCondition, stContext);
                encode_event_images(stCondition, stContext);
            }
#endif
            m_parkingAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        }
        case Event::Type_E::LINE_CROSSING:
        {
            /* step: 越界三重门控图片编码（等价旧 process_boundary_detection）
             * 门控三条件顺序与旧一致，缺一不可：
             *   1. bConditionMet (= bIsAlarm)：当前帧确实检测到越界（Processor 已保证为 true）
             *   2. canStartAlarm()：状态机不在活跃/冷却期，避免重复编码
             *   3. tvsdk_event_image_required()：有 TVSDK 客户端在订阅
             * 共享层 Processor 只输出 bIsAlarm + ImageRequest，不碰后两个门控 */
            EventTriggerContext_S stContext = build_event_context(stCondition);

#ifdef ENABLE_TVSDK_SRC
            /* ! 越界三重门控：结构化字段 + 图片编码都在门控内（对齐旧 hvf_boundary_processor 行为）
             * perf: 三重门控避免活跃/冷却期和无 TVSDK 订阅时的重复编码开销 */
            if (stCondition.bConditionMet && stCondition.bHasTarget &&
                m_lineCrossingAlarmStateMachine.canStartAlarm() &&
                AiAppCommon::tvsdk_event_image_required())
            {
                fill_tvsdk_structured_fields(stCondition, stContext);
                encode_event_images(stCondition, stContext);
            }
#endif

            m_lineCrossingAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        }
        case Event::Type_E::FACE_DETECT:
        {
            /* step: 人脸侦测三重门控图片编码（等价旧 CHVFFaceProcessor::process）
             * 门控三条件顺序与旧一致，缺一不可：
             *   1. bConditionMet (= bIsAlarm)：当前帧检测到区域内人脸（Processor 已保证为 true）
             *   2. canStartAlarm()：状态机不在活跃/冷却期，避免重复编码
             *   3. tvsdk_event_image_required()：有 TVSDK 客户端在订阅
             *
             * ! Face TVSDK 路径与区域/越界不同（等价关键，勿照搬 fill_tvsdk_structured_fields）：
             *   - 不填 EventTriggerContext 结构化字段（nTargetId/nObjectType/fConfidence/nLeft..nBottom 全默认值）
             *   - 不裁剪目标特写图（stTargetImage 空）
             *   - 只把全景图放进 pTvSdkPayload->stPanoramaImage（shared_ptr）
             *   - 下游 push_ai_object_alarm 通过 stPanoramaImage.vecJpeg.empty() && pTvSdkPayload 回退消费 */
            EventTriggerContext_S stContext = build_event_context(stCondition);

#ifdef ENABLE_TVSDK_SRC
            /* perf: 三重门控避免活跃/冷却期和无 TVSDK 订阅时的重复编码开销 */
            if (stCondition.bConditionMet && m_faceAlarmStateMachine.canStartAlarm() &&
                AiAppCommon::tvsdk_event_image_required())
            {
                /* 构建 pTvSdkPayload 并挂载到 stContext */
                auto pPayload = std::make_shared<EventTvSdkPayload_S>();
                pPayload->enType = get_tvsdk_payload_type(stCondition.enEventType);

                /* 编码全景图到 pTvSdkPayload->stPanoramaImage */
                if (m_pImageProvider != nullptr)
                {
                    EventStatistics_NS::ImagePayload_S stPanorama;
                    if (OK == m_pImageProvider->build_panorama(stPanorama))
                    {
                        /* memory: JPEG 数据所有权从 stPanorama 转移到 pPayload->stPanoramaImage */
                        pPayload->stPanoramaImage.vecJpeg = std::move(stPanorama.vecJpeg);
                        pPayload->stPanoramaImage.nWidth = stPanorama.nWidth;
                        pPayload->stPanoramaImage.nHeight = stPanorama.nHeight;
                        pPayload->stPanoramaImage.strTag = std::move(stPanorama.strTag);
                        stContext.pTvSdkPayload = pPayload;

                        dlog_info("HVF 人脸侦测 TVSDK payload 填充: panorama[%zu]",
                                  pPayload->stPanoramaImage.vecJpeg.size());
                    }
                    else
                    {
                        dlog_warn("HVF 人脸侦测全景图 JPEG 编码失败");
                    }
                }
                else
                {
                    dlog_warn("HVF 人脸侦测 TVSDK 上下文未填图: provider=null");
                }
            }
#endif

            m_faceAlarmStateMachine.handleAlarmState(stCondition.bConditionMet, stContext);
            break;
        }
        default:
            break;
        }
    }

    /* OSD 叠加：归一化框转回原生结果坐标后追加到本帧汇总框数组 */
    for (const auto &stOverlayItem : stOutput.vecOverlayItems)
    {
        if (!AiPipeline_NS::Geometry_NS::is_valid_frame_size(m_stNativeResultSize))
        {
            continue;
        }
        const Common::RectInfo_S stRectInfo = AiPipeline_NS::Geometry_NS::denormalize_rect(stOverlayItem.stRect, m_stNativeResultSize);
        if (stRectInfo.nX2 > stRectInfo.nX1 && stRectInfo.nY2 > stRectInfo.nY1)
        {
            vstRectInfo.emplace_back(stRectInfo);
        }
    }
}

EventTriggerContext_S CHVFEventOutputExecutor::build_event_context(const AiPipeline_NS::EventCondition_S &stCondition)
{
    /* 联动上下文，承载规则匹配所需的摘要属性 */
    EventTriggerContext_S stContext;
    stContext.enEventType = stCondition.enEventType;
    stContext.nChnId = stCondition.nChannelId;
    stContext.llTimestamp = stCondition.llWallTimestampMs;

    /* 严格对齐旧 CHVFFaceProcessor 行为：Face 不填 mapAttrs["rule_id"]
     * 旧 Face 的 EventTriggerContext 仅设 enEventType/nChnId/llTimestamp 三个字段
     * 下游 push_ai_object_alarm 不消费 rule_id（仅 push_rule_alarm 消费），
     * 但 resolver 的 match_attr 可能按 rule_id 过滤联动计划，多填会改变匹配行为 */
    if (stCondition.enEventType != Event::Type_E::FACE_DETECT)
    {
        stContext.mapAttrs["rule_id"] = std::to_string(stCondition.nRuleId);
    }
    return stContext;
}

#ifdef ENABLE_TVSDK_SRC
/**
 * @brief   : 将平台无关中性语义目标类型映射到 TVSDK 目标类型
 * @param    {int} nNeutralType：中性语义（0=未知 1=人 2=车 3=人脸）
 * @return   {int} TVSDK 目标类型（0=未知 1=人 2=车），人脸映射为 0（未知）
 * @note    : 对齐旧 to_tvsdk_object_type 行为：仅 HUMAN/VEHICLE 有明确映射
 */
static int neutral_to_tvsdk_object_type(int nNeutralType)
{
    switch (nNeutralType)
    {
    case 1: /* PERSON */
        return 1;
    case 2: /* VEHICLE */
        return 2;
    default:
        /* FACE(3) 及其他在 TVSDK 区域/越界事件中视为未知 */
        return 0;
    }
}

void CHVFEventOutputExecutor::fill_tvsdk_structured_fields(const AiPipeline_NS::EventCondition_S &stCondition,
                                                           EventTriggerContext_S &stContext)
{
    /* 填充 TVSDK 结构化字段，对齐旧 fill_hvf_tvsdk_event_context 行为 */
    stContext.nTargetId = static_cast<int>(stCondition.ullTargetId);
    stContext.nObjectType = neutral_to_tvsdk_object_type(stCondition.nObjectType);
    stContext.fConfidence = stCondition.fConfidence;

    /* 归一化目标框反归一化到像素坐标 */
    if (AiPipeline_NS::Geometry_NS::is_valid_frame_size(m_stNativeResultSize))
    {
        const Common::RectInfo_S stPixelRect = AiPipeline_NS::Geometry_NS::denormalize_rect(
            stCondition.stTargetRect, m_stNativeResultSize);
        stContext.nLeft = stPixelRect.nX1;
        stContext.nTop = stPixelRect.nY1;
        stContext.nRight = stPixelRect.nX2;
        stContext.nBottom = stPixelRect.nY2;
    }
}

void CHVFEventOutputExecutor::encode_event_images(const AiPipeline_NS::EventCondition_S &stCondition,
                                                  EventTriggerContext_S &stContext)
{
    if (m_pImageProvider == nullptr)
    {
        dlog_warn("HVF TVSDK上下文未填图: event[%d], rule[%d], target[%llu], provider=null",
                  static_cast<int>(stCondition.enEventType),
                  stCondition.nRuleId,
                  static_cast<unsigned long long>(stCondition.ullTargetId));
        return;
    }

    /* perf: 全景图 JPEG 编码，由 IFrameImageProvider 封装 VGS/JPEG */
    EventStatistics_NS::ImagePayload_S stPanorama;
    if (OK == m_pImageProvider->build_panorama(stPanorama))
    {
        /* memory: JPEG 数据所有权从 stPanorama 转移到 stContext.stPanoramaImage */
        stContext.stPanoramaImage.vecJpeg = std::move(stPanorama.vecJpeg);
        stContext.stPanoramaImage.nWidth = stPanorama.nWidth;
        stContext.stPanoramaImage.nHeight = stPanorama.nHeight;
        stContext.stPanoramaImage.strTag = std::move(stPanorama.strTag);
    }
    else
    {
        dlog_warn("HVF事件全景图 JPEG 编码失败，事件类型[%d]", static_cast<int>(stCondition.enEventType));
    }

    /* perf: 目标特写图 JPEG 编码，使用归一化目标框裁剪 */
    EventStatistics_NS::ImagePayload_S stTarget;
    if (OK == m_pImageProvider->build_target(stCondition.stTargetRect, stTarget))
    {
        /* memory: JPEG 数据所有权从 stTarget 转移到 stContext.stTargetImage */
        stContext.stTargetImage.vecJpeg = std::move(stTarget.vecJpeg);
        stContext.stTargetImage.nWidth = stTarget.nWidth;
        stContext.stTargetImage.nHeight = stTarget.nHeight;
        stContext.stTargetImage.strTag = std::move(stTarget.strTag);
    }
    else
    {
        dlog_warn("HVF事件目标特写图 JPEG 编码失败，事件类型[%d]", static_cast<int>(stCondition.enEventType));
    }

    dlog_info("HVF TVSDK上下文填充: event[%d], rule[%d], target[%llu], objType[%d], rect[%d,%d,%d,%d], "
              "panorama[%zu], targetImg[%zu]",
              static_cast<int>(stCondition.enEventType),
              stCondition.nRuleId,
              static_cast<unsigned long long>(stCondition.ullTargetId),
              stContext.nObjectType,
              stContext.nLeft,
              stContext.nTop,
              stContext.nRight,
              stContext.nBottom,
              stContext.stPanoramaImage.vecJpeg.size(),
              stContext.stTargetImage.vecJpeg.size());
}
#endif /* ENABLE_TVSDK_SRC */

void CHVFEventOutputExecutor::reset()
{
    m_intrusionAlarmStateMachine.reset();
    m_enterAlarmStateMachine.reset();
    m_leaveAlarmStateMachine.reset();
    m_lineCrossingAlarmStateMachine.reset();
    m_loiteringAlarmStateMachine.reset();
    m_parkingAlarmStateMachine.reset();
    m_faceAlarmStateMachine.reset();
}

void CHVFEventOutputExecutor::reset_region()
{
    m_intrusionAlarmStateMachine.reset();
    m_enterAlarmStateMachine.reset();
    m_leaveAlarmStateMachine.reset();
}

void CHVFEventOutputExecutor::reset_trip_line()
{
    m_lineCrossingAlarmStateMachine.reset();
}

void CHVFEventOutputExecutor::reset_dwell()
{
    m_loiteringAlarmStateMachine.reset();
    m_parkingAlarmStateMachine.reset();
}

void CHVFEventOutputExecutor::reset_face()
{
    m_faceAlarmStateMachine.reset();
}
} // namespace HVFDetectInternal

#endif // CAP_UNIFIED_EVENT_PIPELINE
