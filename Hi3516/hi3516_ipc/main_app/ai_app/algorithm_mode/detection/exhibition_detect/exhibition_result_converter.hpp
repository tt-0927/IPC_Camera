/**
 * @FilePath     : exhibition_result_converter.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 16:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 展馆模型结果到标准检测批次转换器
 */

#pragma once

#include <cstdint>
#include <vector>

#include "IouTracker.hpp"
#include "detection_types.hpp"

/* 展馆模型类别：与 ai_exhibition.json post_precess.label_name 顺序一致 */
namespace ExhibitionDetect_NS
{
constexpr int LABEL_PERSON = 0; /* person：行人（人流统计目标） */
constexpr int LABEL_HEAD = 1;   /* head：人头（人员密度统计目标） */
} // namespace ExhibitionDetect_NS

/**
 * @brief   : 展馆 YOLO 结果转换器
 * @note    : 输入跟踪结果（TrackResult_S，中心点/宽高模型坐标），
 *            输出 AiPipeline_NS::DetectionBatch_S。
 *            - person（label 0）且通过跟踪器：PERSON + TRACKED + Track ID，参与人流统计
 *            - person 跟踪消失：PERSON + ENDED + Track ID，供共享处理器清理轨迹状态
 *            - head（label 1）且通过 head 跟踪器：HEAD + TRACKED + Track ID，参与人员密度统计；
 *              密度为瞬时计数无轨迹存储，head 不输出 ENDED
 *            三尺寸元数据：源帧 / 模型输入 1024×576 / 原生结果 1024×576（MODEL_INPUT 语义）。
 */
class CExhibitionResultConverter
{
public:
    /**
     * @brief   : 构造展馆结果转换器
     */
    CExhibitionResultConverter();

    /**
     * @brief   : 转换单帧展馆模型结果
     * @param    {const std::vector<TrackResult_S>} &vTrackedPersons：跟踪输出的 person 框
     * @param    {const std::vector<int>} &vEndedTrackIds：本帧消失的 person Track ID 集合
     * @param    {const std::vector<TrackResult_S>} &vTrackedHeads：head 跟踪器输出的 head 框
     * @param    {const AiPipeline_NS::FrameMetadata_S} &stMetaBase：帧元数据基础
     * @param    {AiPipeline_NS::DetectionBatch_S} &stBatch：输出标准检测批次
     * @return   {int} OK：成功，ERR_PARAM：元数据或原生结果尺寸非法
     * @note    : 单个坏目标只丢弃该目标，不丢整批；结果对象数组在本周期内完成值拷贝
     */
    int convert(const std::vector<TrackResult_S> &vTrackedPersons,
                const std::vector<int> &vEndedTrackIds,
                const std::vector<TrackResult_S> &vTrackedHeads,
                const AiPipeline_NS::FrameMetadata_S &stMetaBase,
                AiPipeline_NS::DetectionBatch_S &stBatch);

    /**
     * @brief   : 获取当前原生结果坐标尺寸
     * @return   {const AiPipeline_NS::FrameSize_S &} 原生结果坐标尺寸
     */
    const AiPipeline_NS::FrameSize_S &native_result_size() const;

    /**
     * @brief   : 获取累计丢弃目标数
     * @return   {uint64_t} 丢弃目标数
     */
    uint64_t dropped_object_count() const;

private:
    /**
     * @brief   : 将单个跟踪结果转换为标准检测目标
     * @param    {const TrackResult_S} &stTrack：跟踪器输出的目标
     * @param    {AiPipeline_NS::ObjectType_E} enType：语义目标类型（PERSON/HEAD）
     * @param    {AiPipeline_NS::DetectionObject_S} &stObject：输出检测目标
     * @return   {bool} true：转换成功 false：置信度或几何非法（计入丢弃统计）
     */
    bool convert_tracked_object(const TrackResult_S &stTrack,
                                AiPipeline_NS::ObjectType_E enType,
                                AiPipeline_NS::DetectionObject_S &stObject);

    /**
     * @brief   : 将跟踪框（中心点/宽高）转换并校验为像素矩形
     * @param    {const DetectResult_S} &stTrackBox：跟踪器输出的目标框
     * @param    {double} &dLeft：输出左边界
     * @param    {double} &dTop：输出上边界
     * @param    {double} &dRight：输出右边界
     * @param    {double} &dBottom：输出下边界
     * @return   {bool} true：矩形有效并已裁剪到边界
     * @note    : IouTracker 内部按"中心点 (nX1,nY1) + 宽高 (nW,nH)"语义计算 IOU
     *            （见 IouTracker.cpp iou()：x1 = nX1 - w/2），此处按同一语义还原左上角，
     *            宽度保持检测框原始宽高
     */
    bool convert_track_box_to_rect(const DetectResult_S &stTrackBox,
                                   double &dLeft, double &dTop, double &dRight, double &dBottom) const;

    /* 模型输入分辨率（与 ai_exhibition.json pre_process.size 一致） */
    AiPipeline_NS::FrameSize_S m_stModelInputSize;
    /* 原生结果坐标分辨率：MODEL_INPUT 语义，等于模型输入分辨率 */
    AiPipeline_NS::FrameSize_S m_stNativeResultSize;
    /* 累计丢弃目标数（总） */
    uint64_t m_ullDroppedObjectCount = 0;
    /* 累计丢弃：几何非法（宽高<=0/非有限/越界） */
    uint64_t m_ullDroppedGeometryCount = 0;
    /* 累计丢弃：置信度非法（非有限或超 [0,1]） */
    uint64_t m_ullDroppedConfidenceCount = 0;
    /* 上次诊断日志对应的丢弃数（丢弃数变化时才输出，dlog 全局限流兜底） */
    uint64_t m_ullLastWarnDropCount = 0;
};
