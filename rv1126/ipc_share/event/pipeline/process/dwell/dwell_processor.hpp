/**
 * @FilePath     : dwell_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 驻留事件处理器（徘徊/停车），平台无关 IResultProcessor 实现
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <cstdint>

#include "dwell_config_adapter.hpp"
#include "dwell_track_store.hpp"
#include "result_processor.hpp"

namespace AiPipeline_NS
{
namespace Dwell_NS
{
/**
 * @brief   : 驻留事件处理器，合并徘徊/停车两事件
 * @note    : 实现 IResultProcessor，一次 process() 内遍历所有规则，
 *            徘徊规则集(HUMAN)与停车规则集(VEHICLE)按目标类别+事件类型参数化，
 *            共用一份状态存储
 */
class CDwellProcessor : public IResultProcessor
{
public:
    /**
     * @brief   : 构造驻留事件处理器
     */
    CDwellProcessor();

    /**
     * @brief   : 应用归一化配置
     * @param    {const DwellConfig_S &} stConfig：归一化配置
     * @return   {int} OK：成功
     * @note    : 禁用或规则变化时清空运行态
     */
    int apply_config(const DwellConfig_S &stConfig);

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
     * @note    : 遍历徘徊(HUMAN)+停车(VEHICLE)规则集，各规则独立判定并输出 EventCondition_S
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
    DwellConfig_S m_stConfig;
    /* 配置是否已应用 */
    bool m_bConfigValid = false;
    /* 驻留轨迹状态存储，徘徊+停车共用 */
    CDwellTrackStore m_trackStore;
};
} // namespace Dwell_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
