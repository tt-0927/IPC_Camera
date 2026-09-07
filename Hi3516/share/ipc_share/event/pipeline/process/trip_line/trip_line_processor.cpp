/**
 * @FilePath     : trip_line_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 越界事件处理器实现（跨线方向判定 + 轨迹状态 + 首帧语义）
 */

#include "trip_line_processor.hpp"

#include "IpcRet.h"
#include "dlog.h"
#include "normalized_geometry.hpp"

#if CAP_UNIFIED_EVENT_PIPELINE

namespace AiPipeline_NS
{
namespace TripLine_NS
{
namespace
{
/* 目标类型位掩码：接受 HUMAN + VEHICLE（覆盖越界的目标范围） */
constexpr uint32_t TRIP_LINE_OBJECT_MASK = (1U << static_cast<uint32_t>(ObjectType_E::PERSON)) |
                                           (1U << static_cast<uint32_t>(ObjectType_E::VEHICLE));
/* 处理器必需能力：稳定 Track ID 与生命周期 */
constexpr uint32_t TRIP_LINE_REQUIRED_CAPABILITIES = static_cast<uint32_t>(DetectionCapability_E::TRACK_ID) |
                                                     static_cast<uint32_t>(DetectionCapability_E::TRACK_LIFECYCLE);
} // namespace

CTripLineProcessor::CTripLineProcessor() = default;

int CTripLineProcessor::apply_config(const TripLineConfig_S &stConfig)
{
    const bool bWasEnabled = m_bConfigValid && m_stConfig.bEnabled;
    const bool bNowEnabled = stConfig.bEnabled;

    /* 规则几何是否发生变化 */
    bool bGeometryChanged = false;
    if (m_bConfigValid && m_stConfig.vecRules.size() != stConfig.vecRules.size())
    {
        bGeometryChanged = true;
    }
    else if (m_bConfigValid)
    {
        for (size_t i = 0; i < stConfig.vecRules.size(); ++i)
        {
            const auto &stOld = m_stConfig.vecRules[i];
            const auto &stNew = stConfig.vecRules[i];
            if (stOld.nRuleId != stNew.nRuleId || stOld.enCrossDirection != stNew.enCrossDirection ||
                stOld.stLineStart != stNew.stLineStart || stOld.stLineEnd != stNew.stLineEnd)
            {
                bGeometryChanged = true;
                break;
            }
        }
    }

    m_stConfig = stConfig;
    m_bConfigValid = true;

    /* 禁用或规则变化时清空运行态，避免旧轨迹跨新规则产生误报 */
    if (!bNowEnabled || (bWasEnabled && bGeometryChanged))
    {
        m_trackStore.reset();
    }
    return OK;
}

bool CTripLineProcessor::is_enabled() const
{
    return m_bConfigValid && m_stConfig.bEnabled;
}

void CTripLineProcessor::reset()
{
    m_stConfig = TripLineConfig_S();
    m_bConfigValid = false;
    m_trackStore.reset();
}

const ProcessorDescriptor_S &CTripLineProcessor::descriptor() const
{
    static const ProcessorDescriptor_S stDescriptor = {
        "CTripLineProcessor",
        TRIP_LINE_OBJECT_MASK,
        TRIP_LINE_REQUIRED_CAPABILITIES,
        /* step: 越界排在 Region 之后，避免首帧误报对后续处理产生干扰 */
        20,
    };
    return stDescriptor;
}

int CTripLineProcessor::process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput)
{
    if (!m_bConfigValid || !m_stConfig.bEnabled)
    {
        return OK;
    }

    const int64_t llMonoMs = stBatch.stMetadata.llMonotonicTimestampMs;
    const int64_t llWallMs = stBatch.stMetadata.llWallTimestampMs;
    const int nChannelId = stBatch.stMetadata.nChannelId;

    for (const auto &stObject : stBatch.vecObjects)
    {
        /* ENDED 目标：清理所有关联规则的轨迹状态（等价旧 cleanupLostTargets 的第一层清理） */
        if (stObject.enTrackState == TrackState_E::ENDED)
        {
            if (stObject.optTrackId.has_value())
            {
                for (const auto &stRule : m_stConfig.vecRules)
                {
                    const TripLineTrackKey_S stKey{ nChannelId, stRule.nRuleId, stObject.optTrackId.value() };
                    m_trackStore.erase(stKey);
                }
            }
            continue;
        }

        /* 无稳定 Track ID 的目标不参与越界判定 */
        if (!stObject.optTrackId.has_value() || stObject.enTrackState == TrackState_E::UNAVAILABLE)
        {
            continue;
        }

        /* 目标必须携带有效几何 */
        if (!stObject.bHasGeometry || !Geometry_NS::is_valid_normalized_rect(stObject.stRect))
        {
            continue;
        }

        /* 目标位置：检测框几何中心，与旧 (x+w/2, y+h/2) 一致 */
        const Geometry_NS::Point_S stCenter = rect_center(stObject.stRect);

        /* 遍历所有规则，各规则独立判定 */
        for (const auto &stRule : m_stConfig.vecRules)
        {
            if (!stRule.bValid)
            {
                continue;
            }

            /* 目标类别过滤 */
            if (!matches_detection_target(stObject.enType, stRule.nDetectionTargetMask))
            {
                continue;
            }

            /* 轨迹状态查找或创建 */
            const TripLineTrackKey_S stKey{ nChannelId, stRule.nRuleId, stObject.optTrackId.value() };
            TripLineTrackState_S *pState = m_trackStore.find(stKey);

            if (pState == nullptr)
            {
                /* 首帧出现：创建轨迹，last=current=中心点，bIsTracking=false
                 * 等价旧 BoundaryTrackStatus_S 的 if (!stTrackStatus.bIsTracking) 分支：
                 *   stTrackStatus.bIsTracking = true;
                 *   stTrackStatus.stCurrentPosition = stCurrentPos;
                 *   stTrackStatus.stLastPosition = stCurrentPos;
                 *   continue; // 首帧不判跨线
                 */
                pState = m_trackStore.create(stKey, stCenter, llMonoMs);
                if (pState == nullptr)
                {
                    /* 容量满载，跳过该目标 */
                    continue;
                }
                pState->bIsTracking = true;
                /* ! 首帧不判定跨线：漏掉此 continue 会导致首次出现即误报 */
                continue;
            }

            /* 置信度阈值：灵敏度越高（值越大），阈值越低；
             * 仅对 STARTED/TRACKED（旧 NEW/UPDATE）状态生效，等价旧实现 */
            if (stObject.enTrackState == TrackState_E::STARTED || stObject.enTrackState == TrackState_E::TRACKED)
            {
                const float fConfidenceThreshold = 1.0f - static_cast<float>(stRule.nSensitivity) / 100.0f;
                if (stObject.fConfidence < fConfidenceThreshold)
                {
                    continue;
                }
            }

            /* 更新位置：last=上次 current，current=新中心点 */
            pState->stLastPosition = pState->stCurrentPosition;
            pState->stCurrentPosition = stCenter;
            pState->llLastUpdateMs = llMonoMs;

            /* 跨线判定：直接复用 detect_cross_direction，方向语义与旧 tripLineDetection 等价 */
            const Alarm::CrossDirection_E enCrossResult = Geometry_NS::detect_cross_direction(pState->stLastPosition,
                                                                                              pState->stCurrentPosition,
                                                                                              stRule.stLineStart,
                                                                                              stRule.stLineEnd);
            if (enCrossResult == Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID)
            {
                continue;
            }

            /* 方向匹配：等价旧 process_boundary_detection 的 switch 逻辑 */
            bool bShouldAlarm = false;
            switch (stRule.enCrossDirection)
            {
            case Alarm::CrossDirection_E::BOTH_WAYS:
                /* 双向：任何有效穿越都报警 */
                bShouldAlarm = true;
                break;
            case Alarm::CrossDirection_E::A_TO_B:
                bShouldAlarm = (enCrossResult == Alarm::CrossDirection_E::A_TO_B);
                break;
            case Alarm::CrossDirection_E::B_TO_A:
                bShouldAlarm = (enCrossResult == Alarm::CrossDirection_E::B_TO_A);
                break;
            case Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID:
            default:
                break;
            }

            if (!bShouldAlarm)
            {
                continue;
            }

            /* 命中：标记已报警 */
            pState->bAlarmed = true;

            /* OSD 叠加：通过过滤的目标进入叠加输出 */
            OverlayItem_S stOverlayItem;
            stOverlayItem.enType = stObject.enType;
            stOverlayItem.stRect = stObject.stRect;
            stOverlayItem.optTrackId = stObject.optTrackId;
            stOutput.vecOverlayItems.emplace_back(std::move(stOverlayItem));

            /* 输出事件条件：enEventType=LINE_CROSSING, bConditionMet=true */
            EventCondition_S stCondition;
            stCondition.enEventType = Event::Type_E::LINE_CROSSING;
            stCondition.bConditionMet = true;
            stCondition.nChannelId = nChannelId;
            stCondition.nRuleId = stRule.nRuleId;
            stCondition.llWallTimestampMs = llWallMs;
            stCondition.llMonotonicTimestampMs = llMonoMs;

            /* 填充触发目标信息（TVSDK 结构化字段回补）
             * 对齐旧 fill_hvf_tvsdk_event_context 行为：
             *   - 旧代码在 bIsAlarm && pAlarmObject != nullptr 时填充
             *   - 新链路每条 EventCondition 对应其触发目标（一个 track-rule 命中一条）
             * 平台无关中性语义，OutputExecutor 侧再映射到 TVSDK */
            stCondition.bHasTarget = true;
            stCondition.nObjectType = to_neutral_object_type(stObject.enType);
            stCondition.fConfidence = stObject.fConfidence;
            stCondition.stTargetRect = stObject.stRect;
            stCondition.ullTargetId = stObject.optTrackId.value_or(0U);

            stOutput.vecEventConditions.emplace_back(std::move(stCondition));

            /* 输出图片请求：全景图 + 目标特写图
             * 实际编码由 OutputExecutor 依三重门控决定
             * 共享层不碰 canStartAlarm() / tvsdk_event_image_required() */
            ImageRequest_S stPanoramaRequest;
            stPanoramaRequest.bPanorama = true;
            stPanoramaRequest.nOrder = 0;
            stOutput.vecImageRequests.emplace_back(std::move(stPanoramaRequest));

            /* 目标特写图请求：bPanorama=false + stCropRect=归一化目标框 */
            ImageRequest_S stTargetRequest;
            stTargetRequest.bPanorama = false;
            stTargetRequest.stCropRect = stObject.stRect;
            stTargetRequest.nOrder = 1;
            stOutput.vecImageRequests.emplace_back(std::move(stTargetRequest));
        }
    }

    /* 清理超过 TTL 未更新的轨迹状态（等价旧 5000ms 超时 reset，第二层清理） */
    m_trackStore.cleanup_expired(llMonoMs);

    return OK;
}

bool CTripLineProcessor::matches_detection_target(ObjectType_E enType, uint32_t nMask) const
{
    /* 空掩码表示未配置任何检测目标，与旧 is_target_match 语义一致：拒绝所有 */
    if (nMask == 0U)
    {
        return false;
    }

    switch (enType)
    {
    case ObjectType_E::PERSON:
        return (nMask & DETECTION_TARGET_HUMAN) != 0U;
    case ObjectType_E::VEHICLE:
        return (nMask & DETECTION_TARGET_VEHICLE) != 0U;
    default:
        return (nMask & DETECTION_TARGET_OTHER) != 0U;
    }
}

Geometry_NS::Point_S CTripLineProcessor::rect_center(const NormalizedRect_S &stRect) const
{
    Geometry_NS::Point_S stPoint;
    stPoint.dX = (stRect.dLeft + stRect.dRight) / 2.0;
    stPoint.dY = (stRect.dTop + stRect.dBottom) / 2.0;
    return stPoint;
}
} // namespace TripLine_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
