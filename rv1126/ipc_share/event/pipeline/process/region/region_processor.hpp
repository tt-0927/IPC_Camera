/**
 * @FilePath     : region_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 区域事件处理器（入侵/进入/离开），平台无关 IResultProcessor 实现
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <cstdint>

#include "region_config_adapter.hpp"
#include "region_track_store.hpp"
#include "result_processor.hpp"

namespace AiPipeline_NS
{
namespace Region_NS
{
/**
 * @brief   : 区域事件处理器，合并侵入/进入/离开三事件
 * @note    : 实现 IResultProcessor，一次 process() 内遍历所有规则，
 *            按规则事件类型分派触发时机，共用一份状态存储
 */
class CRegionProcessor : public IResultProcessor
{
public:
    /**
     * @brief   : 构造区域事件处理器
     */
    CRegionProcessor();

    /**
     * @brief   : 应用归一化配置
     * @param    {const RegionConfig_S &} stConfig：归一化配置
     * @return   {int} OK：成功
     * @note    : 禁用或规则变化时清空运行态
     */
    int apply_config(const RegionConfig_S &stConfig);

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
     * @note    : 遍历侵入/进入/离开规则集，各规则独立判定并输出 EventCondition_S
     */
    int process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput) override;

private:
    /**
     * @brief   : 检查目标类别是否匹配规则的检测目标掩码
     * @param    {ObjectType_E} enType：目标类别
     * @param    {uint32_t} nMask：检测目标位掩码
     * @return   {bool} true：匹配
     */
    bool matches_detection_target(ObjectType_E enType, uint32_t nMask) const;

    /**
     * @brief   : 获取目标框几何中心点
     * @param    {const NormalizedRect_S &} stRect：归一化目标框
     * @return   {Geometry_NS::Point_S} 几何中心点
     * @note    : 与旧 is_in_region 取检测框中心语义一致
     */
    Geometry_NS::Point_S rect_center(const NormalizedRect_S &stRect) const;

    /* 归一化内部配置 */
    RegionConfig_S m_stConfig;
    /* 配置是否已应用 */
    bool m_bConfigValid = false;
    /* 区域轨迹驻留状态存储，三事件共用 */
    CRegionTrackStore m_trackStore;
};
} // namespace Region_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
