/**
 * @FilePath     : face_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 人脸侦测处理器实现（无状态，每帧独立判定）
 */

#include "face_processor.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_UNIFIED_EVENT_PIPELINE

namespace AiPipeline_NS
{
namespace Face_NS
{
namespace
{
/* 处理器必需能力：仅需几何信息（人脸无需稳定 track_id） */
constexpr uint32_t FACE_REQUIRED_CAPABILITIES = 0;
} // namespace

CFaceProcessor::CFaceProcessor() = default;

int CFaceProcessor::apply_config(const FaceConfig_S &stConfig)
{
    m_stConfig = stConfig;
    m_bConfigValid = true;
    return OK;
}

bool CFaceProcessor::is_enabled() const
{
    return m_bConfigValid && m_stConfig.bEnabled;
}

void CFaceProcessor::reset()
{
    m_stConfig = FaceConfig_S();
    m_bConfigValid = false;
}

const ProcessorDescriptor_S &CFaceProcessor::descriptor() const
{
    static const ProcessorDescriptor_S stDescriptor = {
        "CFaceProcessor",
        /* 人脸目标类别掩码：仅 FACE */
        (1U << static_cast<uint32_t>(ObjectType_E::FACE)),
        FACE_REQUIRED_CAPABILITIES,
        40, /* nOrder 排在 Dwell 之后 */
    };
    return stDescriptor;
}

int CFaceProcessor::process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput)
{
    if (!m_bConfigValid || !m_stConfig.bEnabled || !m_stConfig.bValid)
    {
        return OK;
    }

    const int64_t llMonoMs = stBatch.stMetadata.llMonotonicTimestampMs;
    const int64_t llWallMs = stBatch.stMetadata.llWallTimestampMs;
    const int nChannelId = stBatch.stMetadata.nChannelId;

    /* 置信度门限：灵敏度越高（值越大），门限越低；与旧 get_face_sensitivity_threshold 一致 */
    const float fConfidenceThreshold = 1.0f - static_cast<float>(m_stConfig.nSensitivity) / 100.0f;

    /* 本帧是否命中人脸（任一目标过置信度且在区域内即 true） */
    bool bConditionMet = false;

    for (const auto &stObject : stBatch.vecObjects)
    {
        /* 仅处理 FACE 类别 */
        if (stObject.enType != ObjectType_E::FACE)
        {
            continue;
        }

        /* 目标必须携带有效几何 */
        if (!stObject.bHasGeometry || !Geometry_NS::is_valid_normalized_rect(stObject.stRect))
        {
            continue;
        }

        /* 置信度过滤 */
        if (stObject.fConfidence < fConfidenceThreshold)
        {
            continue;
        }

        /* 目标位置：检测框几何中心，与旧 is_in_region 取中心语义一致 */
        const Geometry_NS::Point_S stCenter = rect_center(stObject.stRect);

        /* 点在单区域内判定 */
        if (!Geometry_NS::point_in_polygon(stCenter, m_stConfig.vecPolygon))
        {
            continue;
        }

        /* 命中人脸 */
        bConditionMet = true;

        /* 动态分析 OSD：bDynamicAnalysisEnable 时输出归一化框，
         * OutputExecutor 无条件 denormalize 追加到 vstRectInfo（统一 OSD 汇总，效果等价旧 send_detectionResult_to_osd）
         * bDynamicAnalysisEnable=false 时不输出 overlay */
        if (m_stConfig.bDynamicAnalysisEnable)
        {
            OverlayItem_S stOverlayItem;
            stOverlayItem.enType = stObject.enType;
            stOverlayItem.stRect = stObject.stRect;
            stOverlayItem.optTrackId = stObject.optTrackId;
            stOutput.vecOverlayItems.emplace_back(std::move(stOverlayItem));
        }
    }

    /* 输出事件条件（FACE_DETECT, bConditionMet, nRuleId=0）
     * 注意：不设 bHasTarget（保持 false），不填目标结构化字段
     * 严格对齐旧 CHVFFaceProcessor 行为：EventTriggerContext 仅填 enEventType/nChnId/llTimestamp */
    EventCondition_S stCondition;
    stCondition.enEventType = Event::Type_E::FACE_DETECT;
    stCondition.bConditionMet = bConditionMet;
    stCondition.nChannelId = nChannelId;
    stCondition.nRuleId = 0;
    stCondition.llWallTimestampMs = llWallMs;
    stCondition.llMonotonicTimestampMs = llMonoMs;
    /* bHasTarget 保持默认 false，不填 nObjectType/fConfidence/stTargetRect/ullTargetId */
    stOutput.vecEventConditions.emplace_back(std::move(stCondition));

    /* 抓拍意图：命中时输出全景 ImageRequest
     * OutputExecutor 三重门控（bConditionMet && canStartAlarm() && tvsdk_event_image_required()）
     * 通过后把全景编到 pTvSdkPayload->stPanoramaImage（而非 stPanoramaImage）
     * 严格对齐旧 pTvSdkPayload 路径，不裁剪特写图 */
    if (bConditionMet)
    {
        ImageRequest_S stPanoramaRequest;
        stPanoramaRequest.bPanorama = true;
        stPanoramaRequest.nOrder = 0;
        stOutput.vecImageRequests.emplace_back(std::move(stPanoramaRequest));
    }

    return OK;
}

Geometry_NS::Point_S CFaceProcessor::rect_center(const NormalizedRect_S &stRect) const
{
    Geometry_NS::Point_S stPoint;
    stPoint.dX = (stRect.dLeft + stRect.dRight) / 2.0;
    stPoint.dY = (stRect.dTop + stRect.dBottom) / 2.0;
    return stPoint;
}
} // namespace Face_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
