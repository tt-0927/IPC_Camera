/**
 * @FilePath     : dwell_processor.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 驻留事件处理器实现（徘徊/停车）
 */

#include "dwell_processor.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_UNIFIED_EVENT_PIPELINE

namespace AiPipeline_NS
{
namespace Dwell_NS
{
namespace
{
/* 目标类型位掩码：接受 HUMAN + VEHICLE（覆盖徘徊/停车的目标范围） */
constexpr uint32_t DWELL_OBJECT_MASK = (1U << static_cast<uint32_t>(ObjectType_E::PERSON)) |
                                       (1U << static_cast<uint32_t>(ObjectType_E::VEHICLE));
/* 处理器必需能力：稳定 Track ID 与生命周期 */
constexpr uint32_t DWELL_REQUIRED_CAPABILITIES = static_cast<uint32_t>(DetectionCapability_E::TRACK_ID) |
                                                 static_cast<uint32_t>(DetectionCapability_E::TRACK_LIFECYCLE);
} // namespace

CDwellProcessor::CDwellProcessor() = default;

int CDwellProcessor::apply_config(const DwellConfig_S &stConfig)
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

bool CDwellProcessor::is_enabled() const
{
    return m_bConfigValid && m_stConfig.bEnabled;
}

void CDwellProcessor::reset()
{
    m_stConfig = DwellConfig_S();
    m_bConfigValid = false;
    m_trackStore.reset();
}

const ProcessorDescriptor_S &CDwellProcessor::descriptor() const
{
    static const ProcessorDescriptor_S stDescriptor = {
        "CDwellProcessor",
        DWELL_OBJECT_MASK,
        DWELL_REQUIRED_CAPABILITIES,
        20, /* 排在 TripLine 之后 */
    };
    return stDescriptor;
}

int CDwellProcessor::process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput)
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
        /* ENDED 目标：清理所有关联规则的状态
         * 等价旧徘徊手动丢失清理：对该 track 所有 (eventType, rule) 键的 erase
         * 停车旧代码无此清理导致状态残留，新链路统一 ENDED 清理是行为改善 */
        if (stObject.enTrackState == TrackState_E::ENDED)
        {
            if (stObject.optTrackId.has_value())
            {
                for (const auto &stRule : m_stConfig.vecRules)
                {
                    const DwellTrackKey_S stKey{ nChannelId, stRule.enEventType, stRule.nRuleId,
                                                 stObject.optTrackId.value() };
                    m_trackStore.erase(stKey);
                }
            }
            continue;
        }

        /* 无稳定 Track ID 的目标不参与驻留判定 */
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

            /* 目标类别过滤：徘徊只处理 PERSON、停车只处理 VEHICLE
             * 对齐旧行为：徘徊硬编码 HUMAN，停车硬编码 VEHICLE，
             * 不消费规则的 aDetectionTarget（旧徘徊该字段未生效） */
            if (!matches_detection_target(stObject.enType, stRule.nDetectionTargetMask))
            {
                continue;
            }

            /* 置信度阈值：灵敏度越高（值越大），阈值越低
             * 仅对 STARTED/TRACKED 生效（旧代码 is_confidence_matched 仅对 NEW/UPDATE 状态） */
            const float fConfidenceThreshold = 1.0f - static_cast<float>(stRule.nSensitivity) / 100.0f;
            if (stObject.fConfidence < fConfidenceThreshold)
            {
                continue;
            }

            /* 获取或创建轨迹状态 */
            const DwellTrackKey_S stKey{ nChannelId, stRule.enEventType, stRule.nRuleId,
                                         stObject.optTrackId.value() };
            DwellTrackState_S *pState = m_trackStore.get_or_create(stKey, llMonoMs);
            if (pState == nullptr)
            {
                /* 容量满载，跳过该目标 */
                continue;
            }

            /* 判断目标当前是否在区域内 */
            const bool bInRegion = Geometry_NS::point_in_polygon(stCenter, stRule.vecPolygon);

            /* step: 驻留判定（对齐旧 process_region_detection 语义）
             * 进入边沿：记录 enterMono
             * 离开边沿：重置状态
             * 驻留判定：(nowMono - enterMono)/1000 >= nTimeThreshold → 命中
             * 报警后重置 enterMono = nowMono（连续驻留非一次性触发） */
            bool bConditionMet = false;

            if (bInRegion)
            {
                if (!pState->bIsInRegion)
                {
                    /* 进入边沿：记录进入时刻 */
                    pState->bIsInRegion = true;
                    pState->llEnterMonotonicMs = llMonoMs;
                    pState->bAlarmed = false;
                }

                /* perf: 驻留计时使用 monotonic 时钟，避免 wall clock 回拨影响
                 * lock: pState 在 process() 调用期间不会被其他线程访问 */
                const int64_t llDwellMs = llMonoMs - pState->llEnterMonotonicMs;
                const int64_t llThresholdMs = static_cast<int64_t>(stRule.nTimeThresholdSec) * 1000;
                if (llDwellMs >= llThresholdMs)
                {
                    bConditionMet = true;
                    /* 报警后重置 enterMono（对齐旧 dEnterTime = dCurrentTime 语义）
                     * 连续驻留达阈值即触发，非一次性触发 */
                    pState->llEnterMonotonicMs = llMonoMs;
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

bool CDwellProcessor::matches_detection_target(ObjectType_E enType, uint32_t nMask) const
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
        return false;
    }
}

Geometry_NS::Point_S CDwellProcessor::rect_center(const NormalizedRect_S &stRect) const
{
    Geometry_NS::Point_S stPoint;
    stPoint.dX = (stRect.dLeft + stRect.dRight) / 2.0;
    stPoint.dY = (stRect.dTop + stRect.dBottom) / 2.0;
    return stPoint;
}
} // namespace Dwell_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
