/**
 * @FilePath     : result_dispatcher.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 同步结果分发器，按固定顺序分发票批并隔离处理器失败
 */

#pragma once

#include <memory>
#include <vector>

#include "detection_types.hpp"
#include "processor_output.hpp"
#include "result_processor.hpp"

namespace AiPipeline_NS
{
/* 同步、确定性、可测试的结果分发器 */
class CResultDispatcher
{
public:
    /**
     * @brief   : 构造结果分发器
     */
    CResultDispatcher() = default;

    /**
     * @brief   : 析构结果分发器
     */
    ~CResultDispatcher() = default;

    /**
     * @brief   : 注册事件处理器
     * @param    {std::unique_ptr<IResultProcessor>} pProcessor：处理器所有权，空指针拒绝
     * @return   {int} OK：成功，ERR_PARAM_NULL：处理器为空
     * @note    : 注册完成后初始化固定顺序，运行期不修改
     */
    int register_processor(std::unique_ptr<IResultProcessor> pProcessor);

    /**
     * @brief   : 分发单帧标准检测批次
     * @param    {const DetectionBatch_S &} stBatch：标准检测批次，只读
     * @param    {ProcessorOutput_S &} stOutput：帧总输出收集器
     * @return   {int} OK：全部处理器成功或失败被隔离，ERR_PARAM：批次元数据非法
     * @note    : 单个处理器失败只丢弃其临时输出，不阻断其他处理器
     */
    int dispatch(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput);

    /**
     * @brief   : 获取已注册处理器数量
     * @return   {size_t} 处理器数量
     */
    size_t processor_count() const;

    /**
     * @brief   : 清空全部处理器，用于迁移模式切换重建
     * @return   {void}
     */
    void reset();

private:
    /* 已注册处理器，按 nOrder 升序固定执行 */
    std::vector<std::unique_ptr<IResultProcessor>> m_vecProcessors;
    /* 注册后是否已排序 */
    bool m_bSorted = false;
};
} // namespace AiPipeline_NS
