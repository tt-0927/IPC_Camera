/**
 * @FilePath     : face_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 人脸侦测处理器，平台无关 IResultProcessor 实现
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <cstdint>

#include "face_config_adapter.hpp"
#include "result_processor.hpp"

namespace AiPipeline_NS
{
namespace Face_NS
{
/**
 * @brief   : 人脸侦测处理器，无状态每帧独立判定
 * @note    : 实现 IResultProcessor，单区域 + 置信度门限 + point_in_polygon，
 *            无 track 状态/驻留计时/索引管理器
 */
class CFaceProcessor : public IResultProcessor
{
public:
    /**
     * @brief   : 构造人脸侦测处理器
     */
    CFaceProcessor();

    /**
     * @brief   : 应用归一化配置
     * @param    {const FaceConfig_S &} stConfig：归一化配置
     * @return   {int} OK：成功
     */
    int apply_config(const FaceConfig_S &stConfig);

    /**
     * @brief   : 获取当前是否启用
     * @return   {bool} true：启用
     */
    bool is_enabled() const;

    /**
     * @brief   : 完全重置处理器
     * @return   {void}
     */
    void reset();

    /**
     * @brief   : 获取处理器静态描述
     * @return   {const ProcessorDescriptor_S &} 处理器描述
     */
    const ProcessorDescriptor_S &descriptor() const override;

    /**
     * @brief   : 处理单帧标准检测结果
     * @param    {const DetectionBatch_S &} stBatch：标准检测批次
     * @param    {ProcessorOutput_S &} stOutput：类型化输出收集器
     * @return   {int} OK：成功
     * @note    : 遍历 FACE 类别目标，置信度过滤 + 点在区域内判定，
     *            命中即输出 EventCondition_S(FACE_DETECT, bConditionMet=true)
     */
    int process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput) override;

private:
    /**
     * @brief   : 获取目标框几何中心点
     * @param    {const NormalizedRect_S &} stRect：归一化目标框
     * @return   {Geometry_NS::Point_S} 几何中心点
     * @note    : 与旧 is_in_region 取检测框中心语义一致
     */
    Geometry_NS::Point_S rect_center(const NormalizedRect_S &stRect) const;

    /* 归一化内部配置 */
    FaceConfig_S m_stConfig;
    /* 配置是否已应用 */
    bool m_bConfigValid = false;
};
} // namespace Face_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
