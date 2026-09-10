/**
 * @FilePath     : result_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : AI 事件处理器抽象合同定义
 */

#pragma once

#include <cstdint>

#include "detection_types.hpp"
#include "processor_output.hpp"

namespace AiPipeline_NS
{
/* 处理器静态描述，用于分发器筛选与排序 */
struct ProcessorDescriptor_S
{
    const char *pName = nullptr;         /* 处理器名称，仅用于诊断 */
    uint32_t unAcceptedObjectMask = 0;   /* 可接受的目标类型位掩码，0 表示不限制 */
    uint32_t unRequiredCapabilities = 0; /* 必需能力位集合 */
    int nOrder = 0;                      /* 固定分发顺序，越小越先执行 */
};

/* AI 事件处理器抽象接口，所有事件后处理统一实现该合同 */
class IResultProcessor
{
public:
    /**
     * @brief   : 析构事件处理器
     * @return   {void}
     */
    virtual ~IResultProcessor() = default;

    /**
     * @brief   : 获取处理器静态描述
     * @return   {const ProcessorDescriptor_S &} 处理器描述
     */
    virtual const ProcessorDescriptor_S &descriptor() const = 0;

    /**
     * @brief   : 处理单帧标准检测结果
     * @param    {const DetectionBatch_S &} stBatch：标准检测批次，只读
     * @param    {ProcessorOutput_S &} stOutput：类型化输出收集器
     * @return   {int} OK：成功，非 OK：失败（输出不会被提交）
     * @note    : 处理器必须遵守“先校验、后修改状态”，失败返回不得留下半更新状态
     */
    virtual int process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput) = 0;
};
} // namespace AiPipeline_NS
