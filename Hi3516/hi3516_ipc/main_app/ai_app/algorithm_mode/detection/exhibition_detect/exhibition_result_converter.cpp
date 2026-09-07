/**
 * @FilePath     : exhibition_result_converter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 展馆模型结果到标准检测批次转换器实现
 */

#include "exhibition_result_converter.hpp"

#include <algorithm>
#include <cmath>

#include "IpcRet.h"
#include "dlog.h"
#include "normalized_geometry.hpp"
#include "video_define.h"

namespace
{
/* 厂商坐标舍入容差（像素），与 HVF Converter 一致 */
constexpr double ROUNDING_TOLERANCE = 1.0;
} // namespace

CExhibitionResultConverter::CExhibitionResultConverter()
{
    m_stModelInputSize.nWidth = PIXEL_WIDTH_1024;
    m_stModelInputSize.nHeight = PIXEL_HEIGHT_576;
    /* MODEL_INPUT 语义：YOLO 输出框属于模型输入坐标，原生结果尺寸固定为模型输入分辨率 */
    m_stNativeResultSize = m_stModelInputSize;
}

int CExhibitionResultConverter::convert(const std::vector<TrackResult_S> &vTrackedPersons,
                                        const std::vector<int> &vEndedTrackIds,
                                        const std::vector<TrackResult_S> &vTrackedHeads,
                                        const AiPipeline_NS::FrameMetadata_S &stMetaBase,
                                        AiPipeline_NS::DetectionBatch_S &stBatch)
{
    stBatch = AiPipeline_NS::DetectionBatch_S();

    /* 原生结果坐标分辨率非法时整批失败，不进入分发 */
    if (!AiPipeline_NS::Geometry_NS::is_valid_frame_size(m_stNativeResultSize))
    {
        dlog_warn("展馆结果转换失败：原生结果坐标分辨率非法 [%ux%u]",
                  m_stNativeResultSize.nWidth, m_stNativeResultSize.nHeight);
        return ERR_PARAM;
    }

    stBatch.stMetadata = stMetaBase;
    stBatch.stMetadata.stModelInputSize = m_stModelInputSize;
    stBatch.stMetadata.stNativeResultSize = m_stNativeResultSize;

    /* 能力位：目标检测 + 稳定 Track ID + 生命周期（ENDED 用于状态清理） */
    AiPipeline_NS::capability::set(stBatch.stMetadata.unCapabilities, AiPipeline_NS::DetectionCapability_E::OBJECT_DETECTION);
    AiPipeline_NS::capability::set(stBatch.stMetadata.unCapabilities, AiPipeline_NS::DetectionCapability_E::TRACK_ID);
    AiPipeline_NS::capability::set(stBatch.stMetadata.unCapabilities, AiPipeline_NS::DetectionCapability_E::TRACK_LIFECYCLE);

    /* 按结果容量预分配，每帧只拷贝紧凑值对象 */
    stBatch.vecObjects.reserve(vTrackedPersons.size() + vEndedTrackIds.size() + vTrackedHeads.size());

    /* 1. 跟踪中的 person：PERSON + TRACKED + Track ID，参与人流统计 */
    for (const auto &stTrack : vTrackedPersons)
    {
        AiPipeline_NS::DetectionObject_S stObject;
        if (!convert_tracked_object(stTrack, AiPipeline_NS::ObjectType_E::PERSON, stObject))
        {
            continue;
        }
        stBatch.vecObjects.emplace_back(std::move(stObject));
    }

    /* 2. 本帧消失的 person 跟踪目标：PERSON + ENDED + Track ID，供共享处理器立即清理轨迹 */
    for (const int nTrackId : vEndedTrackIds)
    {
        AiPipeline_NS::DetectionObject_S stObject;
        stObject.enType = AiPipeline_NS::ObjectType_E::PERSON;
        stObject.enTrackState = AiPipeline_NS::TrackState_E::ENDED;
        stObject.optTrackId = static_cast<uint64_t>(nTrackId);
        stObject.bHasGeometry = false;
        stBatch.vecObjects.emplace_back(std::move(stObject));
    }

    /* 3. 跟踪中的 head：HEAD + TRACKED + Track ID，参与人员密度统计；
     *    密度为瞬时计数无轨迹存储，无需 ENDED 生命周期 */
    for (const auto &stTrack : vTrackedHeads)
    {
        AiPipeline_NS::DetectionObject_S stObject;
        if (!convert_tracked_object(stTrack, AiPipeline_NS::ObjectType_E::HEAD, stObject))
        {
            continue;
        }
        stBatch.vecObjects.emplace_back(std::move(stObject));
    }

    /* 丢弃诊断：仅在本帧发生丢弃时输出，按原因分类计数，便于定位是
     * 置信度异常还是几何/越界异常；帧级热路径依赖 dlog 全局限流按调用点合并 */
    if (m_ullDroppedObjectCount != m_ullLastWarnDropCount)
    {
        m_ullLastWarnDropCount = m_ullDroppedObjectCount;
        dlog_warn("展馆结果转换丢弃目标，累计[%llu]（几何非法/越界[%llu] 置信度非法[%llu]）",
                  static_cast<unsigned long long>(m_ullDroppedObjectCount),
                  static_cast<unsigned long long>(m_ullDroppedGeometryCount),
                  static_cast<unsigned long long>(m_ullDroppedConfidenceCount));
    }

    return OK;
}

bool CExhibitionResultConverter::convert_tracked_object(const TrackResult_S &stTrack,
                                                        AiPipeline_NS::ObjectType_E enType,
                                                        AiPipeline_NS::DetectionObject_S &stObject)
{
    /* 置信度校验：跟踪框携带最近一次检测置信度 */
    if (!std::isfinite(stTrack.box.fConfidence) || stTrack.box.fConfidence < 0.0f || stTrack.box.fConfidence > 1.0f)
    {
        ++m_ullDroppedObjectCount;
        ++m_ullDroppedConfidenceCount;
        return false;
    }

    double dLeft = 0.0;
    double dTop = 0.0;
    double dRight = 0.0;
    double dBottom = 0.0;
    if (!convert_track_box_to_rect(stTrack.box, dLeft, dTop, dRight, dBottom))
    {
        ++m_ullDroppedObjectCount;
        ++m_ullDroppedGeometryCount;
        return false;
    }

    stObject.enType = enType;
    stObject.fConfidence = stTrack.box.fConfidence;
    stObject.enTrackState = AiPipeline_NS::TrackState_E::TRACKED;
    stObject.optTrackId = static_cast<uint64_t>(stTrack.nTrackId);
    stObject.bHasGeometry = true;
    stObject.stRect = AiPipeline_NS::Geometry_NS::normalize_rect(dLeft, dTop, dRight, dBottom, m_stNativeResultSize);
    return true;
}

const AiPipeline_NS::FrameSize_S &CExhibitionResultConverter::native_result_size() const
{
    return m_stNativeResultSize;
}

uint64_t CExhibitionResultConverter::dropped_object_count() const
{
    return m_ullDroppedObjectCount;
}

bool CExhibitionResultConverter::convert_track_box_to_rect(const DetectResult_S &stTrackBox,
                                                           double &dLeft, double &dTop,
                                                           double &dRight, double &dBottom) const
{
    /* IouTracker 的 DetectResult_S 语义为"中心点 (nX1,nY1) + 宽高 (nW,nH)"，
     * 与其 iou() 实现（x1 = nX1 - w/2）保持一致地还原左上角；
     * 右/下边界按 左+宽 计算，保持检测框原始宽高，允许 ±1 像素中心取整误差 */
    const int nW = stTrackBox.nW;
    const int nH = stTrackBox.nH;
    if (nW <= 0 || nH <= 0)
    {
        return false;
    }

    dLeft = static_cast<double>(stTrackBox.nX1 - nW / 2);
    dTop = static_cast<double>(stTrackBox.nY1 - nH / 2);
    dRight = dLeft + static_cast<double>(nW);
    dBottom = dTop + static_cast<double>(nH);

    if (!std::isfinite(dLeft) || !std::isfinite(dTop) || !std::isfinite(dRight) || !std::isfinite(dBottom))
    {
        return false;
    }

    /* 越界检查：允许 1 像素厂商舍入误差并裁剪到边界，超过容差丢弃 */
    const double dFrameW = static_cast<double>(m_stNativeResultSize.nWidth);
    const double dFrameH = static_cast<double>(m_stNativeResultSize.nHeight);
    if (dLeft < -ROUNDING_TOLERANCE || dTop < -ROUNDING_TOLERANCE ||
        dRight > dFrameW + ROUNDING_TOLERANCE || dBottom > dFrameH + ROUNDING_TOLERANCE)
    {
        return false;
    }
    dLeft = std::max(0.0, dLeft);
    dTop = std::max(0.0, dTop);
    dRight = std::min(dFrameW, dRight);
    dBottom = std::min(dFrameH, dBottom);
    return true;
}
