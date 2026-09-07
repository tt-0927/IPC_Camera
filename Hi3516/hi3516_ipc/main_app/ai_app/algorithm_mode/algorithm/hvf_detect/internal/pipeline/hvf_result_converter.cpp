/**
 * @FilePath     : hvf_result_converter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 海思 HVF 检测结果到标准检测批次转换器实现
 */

#include "hvf_result_converter.hpp"

#include <cmath>

#include "IpcRet.h"
#include "dlog.h"
#include "normalized_geometry.hpp"

namespace
{
/**
 * @brief   : 映射海思类别到标准语义类型
 * @param    {ot_aidetect_class} enClass：海思类别
 * @return   {AiPipeline_NS::ObjectType_E} 标准语义类型
 */
AiPipeline_NS::ObjectType_E map_object_class(ot_aidetect_class enClass)
{
    switch (enClass)
    {
    case OT_AIDETECT_CLASS_HUMAN:
        return AiPipeline_NS::ObjectType_E::PERSON;
    case OT_AIDETECT_CLASS_FACE:
        return AiPipeline_NS::ObjectType_E::FACE;
    case OT_AIDETECT_CLASS_VEHICLE:
        return AiPipeline_NS::ObjectType_E::VEHICLE;
    case OT_AIDETECT_CLASS_PET:
        return AiPipeline_NS::ObjectType_E::PET;
    case OT_AIDETECT_CLASS_GARBAGE:
        return AiPipeline_NS::ObjectType_E::GARBAGE;
    case OT_AIDETECT_CLASS_BAG:
        return AiPipeline_NS::ObjectType_E::BAG;
    case OT_AIDETECT_CLASS_WALLET:
        return AiPipeline_NS::ObjectType_E::WALLET;
    case OT_AIDETECT_CLASS_PHONE:
        return AiPipeline_NS::ObjectType_E::PHONE;
    default:
        return AiPipeline_NS::ObjectType_E::UNKNOWN;
    }
}

/**
 * @brief   : 映射海思跟踪状态到标准跟踪状态
 * @param    {ot_aidetect_track_status} enStatus：海思跟踪状态
 * @return   {AiPipeline_NS::TrackState_E} 标准跟踪状态
 */
AiPipeline_NS::TrackState_E map_track_status(ot_aidetect_track_status enStatus)
{
    switch (enStatus)
    {
    case OT_AIDETECT_TRACK_STATUS_NEW:
        return AiPipeline_NS::TrackState_E::STARTED;
    case OT_AIDETECT_TRACK_STATUS_UPDATE:
        return AiPipeline_NS::TrackState_E::TRACKED;
    case OT_AIDETECT_TRACK_STATUS_DIE:
        return AiPipeline_NS::TrackState_E::ENDED;
    case OT_AIDETECT_TRACK_STATUS_VALID:
    default:
        return AiPipeline_NS::TrackState_E::UNAVAILABLE;
    }
}
} // namespace

namespace HVFDetectInternal
{
CHisiHvfResultConverter::CHisiHvfResultConverter(NativeResultCoordinateSource_E enCoordinateSource,
                                                 const AiPipeline_NS::FrameSize_S &stModelInputSize)
    : m_enCoordinateSource(enCoordinateSource), m_stModelInputSize(stModelInputSize)
{
    /* MODEL_INPUT 模式下原生结果坐标固定为模型输入分辨率 */
    if (m_enCoordinateSource == NativeResultCoordinateSource_E::MODEL_INPUT)
    {
        m_stNativeResultSize = m_stModelInputSize;
    }
}

int CHisiHvfResultConverter::convert(const ot_aidetect_result_array &stResult,
                                     const AiPipeline_NS::FrameMetadata_S &stMetaBase,
                                     AiPipeline_NS::DetectionBatch_S &stBatch)
{
    stBatch = AiPipeline_NS::DetectionBatch_S();

    /* SOURCE_FRAME 模式下原生结果坐标随当前帧尺寸更新 */
    if (m_enCoordinateSource == NativeResultCoordinateSource_E::SOURCE_FRAME)
    {
        m_stNativeResultSize = stMetaBase.stSourceFrameSize;
    }

    /* 原生结果坐标分辨率非法时整批失败，不进入分发 */
    if (!AiPipeline_NS::Geometry_NS::is_valid_frame_size(m_stNativeResultSize))
    {
        dlog_warn("HVF 结果转换失败：原生结果坐标分辨率非法 [%ux%u]", m_stNativeResultSize.nWidth, m_stNativeResultSize.nHeight);
        return ERR_PARAM;
    }

    /* 结果数组结构损坏时整批失败 */
    if (stResult.class_num > OT_AIDETECT_CLASS_BUTT)
    {
        dlog_warn("HVF 结果转换失败：类别数量非法 [%u]", stResult.class_num);
        return ERR_PARAM;
    }

    stBatch.stMetadata = stMetaBase;
    stBatch.stMetadata.stNativeResultSize = m_stNativeResultSize;

    /* 能力位：HVF 始终提供目标检测、Track ID 与生命周期 */
    AiPipeline_NS::capability::set(stBatch.stMetadata.unCapabilities, AiPipeline_NS::DetectionCapability_E::OBJECT_DETECTION);
    AiPipeline_NS::capability::set(stBatch.stMetadata.unCapabilities, AiPipeline_NS::DetectionCapability_E::TRACK_ID);
    AiPipeline_NS::capability::set(stBatch.stMetadata.unCapabilities, AiPipeline_NS::DetectionCapability_E::TRACK_LIFECYCLE);

    /* 按海思结果容量预分配，每帧只拷贝紧凑值对象 */
    uint32_t unTotalObjectNum = 0;
    for (uint32_t i = 0; i < stResult.class_num; ++i)
    {
        unTotalObjectNum += stResult.object_class[i].object_num;
    }
    stBatch.vecObjects.reserve(unTotalObjectNum);

    for (uint32_t i = 0; i < stResult.class_num; ++i)
    {
        const ot_aidetect_object_of_one_class &stOneClass = stResult.object_class[i];
        if (stOneClass.objects == nullptr || stOneClass.object_num == 0U)
        {
            continue;
        }

        const AiPipeline_NS::ObjectType_E enObjectType = map_object_class(stOneClass.class_type);
        for (uint32_t j = 0; j < stOneClass.object_num; ++j)
        {
            const ot_aidetect_object &stObject = stOneClass.objects[j];

            AiPipeline_NS::DetectionObject_S stDstObject;
            stDstObject.enType = enObjectType;
            stDstObject.fConfidence = stObject.detect_confidence;
            stDstObject.enTrackState = map_track_status(stObject.track_status);
            stDstObject.optTrackId = stObject.track_id;

            /* DIE 目标坐标退化为零矩形，不参与几何计算，但保留 Track ID 供状态清理 */
            if (stDstObject.enTrackState == AiPipeline_NS::TrackState_E::ENDED)
            {
                stBatch.vecObjects.emplace_back(std::move(stDstObject));
                continue;
            }

            /* 普通目标必须携带有效几何：有限坐标、正面积、置信度合法 */
            const float fRectX = static_cast<float>(stObject.detect_rect.x);
            const float fRectY = static_cast<float>(stObject.detect_rect.y);
            const float fRectW = static_cast<float>(stObject.detect_rect.width);
            const float fRectH = static_cast<float>(stObject.detect_rect.height);

            if (!std::isfinite(fRectX) || !std::isfinite(fRectY) || !std::isfinite(fRectW) || !std::isfinite(fRectH) || fRectW <= 0.0f ||
                fRectH <= 0.0f)
            {
                ++m_ullDroppedObjectCount;
                continue;
            }

            if (!std::isfinite(stObject.detect_confidence) || stObject.detect_confidence < 0.0f || stObject.detect_confidence > 1.0f)
            {
                ++m_ullDroppedObjectCount;
                continue;
            }

            /* 越界检查：允许 1 像素厂商舍入误差并裁剪到边界，超过容差丢弃 */
            const double dFrameW = static_cast<double>(m_stNativeResultSize.nWidth);
            const double dFrameH = static_cast<double>(m_stNativeResultSize.nHeight);
            constexpr double ROUNDING_TOLERANCE = 1.0;
            double dLeft = static_cast<double>(fRectX);
            double dTop = static_cast<double>(fRectY);
            double dRight = static_cast<double>(fRectX) + static_cast<double>(fRectW);
            double dBottom = static_cast<double>(fRectY) + static_cast<double>(fRectH);

            if (dLeft < -ROUNDING_TOLERANCE || dTop < -ROUNDING_TOLERANCE || dRight > dFrameW + ROUNDING_TOLERANCE ||
                dBottom > dFrameH + ROUNDING_TOLERANCE)
            {
                ++m_ullDroppedObjectCount;
                continue;
            }

            dLeft = std::max(0.0, dLeft);
            dTop = std::max(0.0, dTop);
            dRight = std::min(dFrameW, dRight);
            dBottom = std::min(dFrameH, dBottom);

            /* 坐标统一归一化到原生结果坐标分辨率 */
            stDstObject.bHasGeometry = true;
            stDstObject.stRect = AiPipeline_NS::Geometry_NS::normalize_rect(dLeft, dTop, dRight, dBottom, m_stNativeResultSize);
            stBatch.vecObjects.emplace_back(std::move(stDstObject));
        }
    }

    return OK;
}

const AiPipeline_NS::FrameSize_S &CHisiHvfResultConverter::native_result_size() const
{
    return m_stNativeResultSize;
}

uint64_t CHisiHvfResultConverter::dropped_object_count() const
{
    return m_ullDroppedObjectCount;
}
} // namespace HVFDetectInternal
