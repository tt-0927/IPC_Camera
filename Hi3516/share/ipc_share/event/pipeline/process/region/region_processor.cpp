/**
 * @FilePath     : region_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 区域事件处理器实现（入侵/进入/离开）
 */

#include "region_processor.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_UNIFIED_EVENT_PIPELINE

namespace AiPipeline_NS
{
namespace Region_NS
{
namespace
{
/* 目标类型位掩码：接受 HUMAN + VEHICLE（覆盖入侵/进入/离开的目标范围） */
constexpr uint32_t REGION_OBJECT_MASK = (1U << static_cast<uint32_t>(ObjectType_E::PERSON)) |
                                        (1U << static_cast<uint32_t>(ObjectType_E::VEHICLE));
/* 处理器必需能力：稳定 Track ID 与生命周期 */
constexpr uint32_t REGION_REQUIRED_CAPABILITIES = static_cast<uint32_t>(DetectionCapability_E::TRACK_ID) |
                                                  static_cast<uint32_t>(DetectionCapability_E::TRACK_LIFECYCLE);
} // namespace

CRegionProcessor::CRegionProcessor() = default;

int CRegionProcessor::apply_config(const RegionConfig_S &stConfig)
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
            if (stOld.enEventType != stNew.enEventType || stOld.nRuleId != stNew.nRuleId ||
                stOld.vecPolygon.size() != stNew.vecPolygon.size())
            {
                bGeometryChanged = true;
                break;
            }
            for (size_t j = 0; j < stOld.vecPolygon.size(); ++j)
            {
                if (stOld.vecPolygon[j].dX != stNew.vecPolygon[j].dX ||
                    stOld.vecPolygon[j].dY != stNew.vecPolygon[j].dY)
                {
                    bGeometryChanged = true;
                    break;
                }
            }
            if (bGeometryChanged)
            {
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

bool CRegionProcessor::is_enabled() const
{
    return m_bConfigValid && m_stConfig.bEnabled;
}

void CRegionProcessor::reset()
{
    m_stConfig = RegionConfig_S();
    m_bConfigValid = false;
    m_trackStore.reset();
}

const ProcessorDescriptor_S &CRegionProcessor::descriptor() const
{
    static const ProcessorDescriptor_S stDescriptor = {
        "CRegionProcessor",
        REGION_OBJECT_MASK,
        REGION_REQUIRED_CAPABILITIES,
        10, /* 排在人流统计之后 */
    };
    return stDescriptor;
}

int CRegionProcessor::process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput)
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
        /* ENDED 目标：清理所有关联规则的状态 */
        if (stObject.enTrackState == TrackState_E::ENDED)
        {
            if (stObject.optTrackId.has_value())
            {
                for (const auto &stRule : m_stConfig.vecRules)
                {
                    const RegionTrackKey_S stKey{ nChannelId, stRule.enEventType, stRule.nRuleId,
                                                   stObject.optTrackId.value() };
                    m_trackStore.erase(stKey);
                }
            }
            continue;
        }

        /* 无稳定 Track ID 的目标不参与区域判定 */
        if (!stObject.optTrackId.has_value() || stObject.enTrackState == TrackState_E::UNAVAILABLE)
        {
            continue;
        }

        /* 目标必须携带有效几何 */
        if (!stObject.bHasGeometry || !Geometry_NS::is_valid_normalized_rect(stObject.stRect))
        {
            continue;
        }

        /* 目标位置：检测框几何中心，与旧 is_in_region 取中心语义一致 */
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

            /* 置信度阈值：灵敏度越高（值越大），阈值越低 */
            const float fConfidenceThreshold = 1.0f - static_cast<float>(stRule.nSensitivity) / 100.0f;
            if (stObject.fConfidence < fConfidenceThreshold)
            {
                continue;
            }

            /* 获取或创建轨迹状态 */
            const RegionTrackKey_S stKey{ nChannelId, stRule.enEventType, stRule.nRuleId,
                                          stObject.optTrackId.value() };
            RegionTrackState_S *pState = m_trackStore.get_or_create(stKey, llMonoMs);
            if (pState == nullptr)
            {
                /* 容量满载，跳过该目标 */
                continue;
            }

            /* 判断目标当前是否在区域内 */
            const bool bInRegion = Geometry_NS::point_in_polygon(stCenter, stRule.vecPolygon);

            /* 按事件类型分派触发时机 */
            bool bConditionMet = false;

            switch (stRule.enEventType)
            {
            case Event::Type_E::INTRUSION:
            {
                /* 入侵：点在多边形 + 驻留满时长 */
                if (bInRegion)
                {
                    if (!pState->bIsInRegion)
                    {
                        /* 进入边沿：记录进入时刻 */
                        pState->bIsInRegion = true;
                        pState->llEnterMonotonicMs = llMonoMs;
                        pState->bAlarmed = false;
                    }

                    /* 驻留计时 */
                    const int64_t llDwellMs = llMonoMs - pState->llEnterMonotonicMs;
                    const int64_t llThresholdMs = static_cast<int64_t>(stRule.nTimeThresholdSec) * 1000;
                    if (llDwellMs >= llThresholdMs && !pState->bAlarmed)
                    {
                        bConditionMet = true;
                        pState->bAlarmed = true;
                    }
                }
                else
                {
                    /* 离开边沿：重置驻留状态 */
                    if (pState->bIsInRegion)
                    {
                        pState->bIsInRegion = false;
                        pState->llEnterMonotonicMs = 0;
                        pState->bAlarmed = false;
                    }
                }
                break;
            }

            case Event::Type_E::ENTER_REGION:
            {
                /* 进入区域：入区边沿触发 */
                if (bInRegion && !pState->bIsInRegion)
                {
                    bConditionMet = true;
                    pState->bIsInRegion = true;
                    pState->llEnterMonotonicMs = llMonoMs;
                }
                else if (!bInRegion && pState->bIsInRegion)
                {
                    /* 离开区域，重置状态以便再次进入 */
                    pState->bIsInRegion = false;
                    pState->llEnterMonotonicMs = 0;
                }
                break;
            }

            case Event::Type_E::LEAVE_REGION:
            {
                /* 离开区域：出区边沿触发 */
                if (bInRegion && !pState->bIsInRegion)
                {
                    /* 进入区域，记录状态 */
                    pState->bIsInRegion = true;
                    pState->llEnterMonotonicMs = llMonoMs;
                }
                else if (!bInRegion && pState->bIsInRegion)
                {
                    /* 离开边沿：触发事件 */
                    bConditionMet = true;
                    pState->bIsInRegion = false;
                    pState->llEnterMonotonicMs = 0;
                }
                break;
            }

            default:
                break;
            }

            /* OSD 叠加：通过过滤的目标进入叠加输出 */
            OverlayItem_S stOverlayItem;
            stOverlayItem.enType = stObject.enType;
            stOverlayItem.stRect = stObject.stRect;
            stOverlayItem.optTrackId = stObject.optTrackId;
            stOutput.vecOverlayItems.emplace_back(std::move(stOverlayItem));

            /* 输出事件条件 */
            EventCondition_S stCondition;
            stCondition.enEventType = stRule.enEventType;
            stCondition.bConditionMet = bConditionMet;
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

            /* 事件触发时追加图片请求（对齐旧 fill_hvf_tvsdk_event_context 的全景+特写图编码）
             * 旧代码区域事件无三重门控优化，事件触发即编码两张图
             * 共享层只输出 ImageRequest，实际编码由 OutputExecutor 依门控决定 */
            if (bConditionMet)
            {
                /* 全景图请求 */
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
    }

    /* 清理超过 TTL 未更新的轨迹状态 */
    m_trackStore.cleanup_expired(llMonoMs);

    return OK;
}

bool CRegionProcessor::matches_detection_target(ObjectType_E enType, uint32_t nMask) const
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

Geometry_NS::Point_S CRegionProcessor::rect_center(const NormalizedRect_S &stRect) const
{
    Geometry_NS::Point_S stPoint;
    stPoint.dX = (stRect.dLeft + stRect.dRight) / 2.0;
    stPoint.dY = (stRect.dTop + stRect.dBottom) / 2.0;
    return stPoint;
}
} // namespace Region_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
