/**
 * @FilePath     : hvf_result_converter.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 海思 HVF 检测结果到标准检测批次转换器
 */

#pragma once

#include <cstdint>

#include "detection_types.hpp"
#include "svp_ai_detect.h"

namespace HVFDetectInternal
{
/* 原生结果坐标来源，海思未明确 detect_rect 坐标系时显式配置 */
enum class NativeResultCoordinateSource_E
{
    MODEL_INPUT = 0,  /* detect_rect 属于模型输入坐标 */
    SOURCE_FRAME = 1, /* detect_rect 属于源帧坐标 */
};

/* 海思 HVF 检测结果转换器，新框架内唯一接触 ot_aidetect_result_array 的组件 */
class CHisiHvfResultConverter
{
public:
    /**
     * @brief   : 构造 HVF 结果转换器
     * @param    {NativeResultCoordinateSource_E} enCoordinateSource：原生结果坐标来源
     * @param    {const AiPipeline_NS::FrameSize_S &} stModelInputSize：模型输入分辨率
     * @return   {void}
     */
    CHisiHvfResultConverter(NativeResultCoordinateSource_E enCoordinateSource, const AiPipeline_NS::FrameSize_S &stModelInputSize);

    /**
     * @brief   : 转换单帧 HVF 结果
     * @param    {const ot_aidetect_result_array &} stResult：HVF 原生结果
     * @param    {const AiPipeline_NS::FrameMetadata_S &} stMetaBase：帧元数据基础（通道/帧号/时间戳/源帧尺寸）
     * @param    {AiPipeline_NS::DetectionBatch_S &} stBatch：输出标准检测批次
     * @return   {int} OK：成功，ERR_PARAM：元数据或结果数组非法
     * @note    : 单个坏目标只丢弃该目标，元数据/数组损坏才丢整批；结果对象数组必须在本周期内完成值拷贝
     */
    int convert(const ot_aidetect_result_array &stResult,
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
    /* 原生结果坐标来源 */
    NativeResultCoordinateSource_E m_enCoordinateSource;
    /* 模型输入分辨率 */
    AiPipeline_NS::FrameSize_S m_stModelInputSize;
    /* 原生结果坐标分辨率 */
    AiPipeline_NS::FrameSize_S m_stNativeResultSize;
    /* 累计丢弃目标数 */
    uint64_t m_ullDroppedObjectCount = 0;
    /* 上次丢弃诊断时间戳 */
    int64_t m_llLastDropWarnMs = 0;
};
} // namespace HVFDetectInternal
