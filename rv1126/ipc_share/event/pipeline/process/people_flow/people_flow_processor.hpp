/**
 * @FilePath     : people_flow_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 共享人流统计事件处理器
 */

#pragma once

#if CAP_AI_PEOPLE_STATISTICS

#include <cstdint>

#include "people_flow_config_adapter.hpp"
#include "people_flow_statistics_state.hpp"
#include "people_flow_track_store.hpp"
#include "result_processor.hpp"

namespace AiPipeline_NS
{
namespace PeopleFlow_NS
{
/* 共享人流统计事件处理器，只依赖标准检测结果、类型化配置和输出端口 */
class CPeopleFlowProcessor : public IResultProcessor
{
public:
    /**
     * @brief   : 构造人流统计处理器
     */
    CPeopleFlowProcessor();

    /**
     * @brief   : 应用归一化配置
     * @param    {const PeopleFlowConfig_S &} stConfig：归一化配置
     * @return   {int} OK：成功
     * @note    : 使能从 true 变 false、规则几何/方向变化时清空运行态，
     *            避免旧轨迹跨新规则线产生伪跨线
     */
    int apply_config(const PeopleFlowConfig_S &stConfig);

    /**
     * @brief   : 手动清零统计与轨迹运行态
     * @return   {void}
     */
    void clear_statistics();

    /**
     * @brief   : 完全重置处理器（含配置与统计状态）
     * @return   {void}
     */
    void reset();

    /**
     * @brief   : 获取当前是否启用
     * @return   {bool} true：启用
     */
    bool is_enabled() const;

    /**
     * @brief   : 获取最近一帧处理摘要
     * @return   {const PeopleFlowFrameSummary_S &} 帧摘要
     */
    const PeopleFlowFrameSummary_S &last_frame_summary() const;

    /**
     * @brief   : 获取处理器静态描述
     * @return   {const ProcessorDescriptor_S &} 处理器描述
     */
    const ProcessorDescriptor_S &descriptor() const override;

    /**
     * @brief   : 处理单帧标准检测结果
     * @param    {const DetectionBatch_S &} stBatch：标准检测批次，只读
     * @param    {ProcessorOutput_S &} stOutput：类型化输出收集器
     * @return   {int} OK：成功
     * @note    : 低置信度、区域外、无 Track ID、UNAVAILABLE/ENDED 目标不会误计数
     */
    int process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput) override;

private:
    /**
     * @brief   : 判断是否到达周期上报时间
     * @param    {int64_t} llNowMs：wall clock 毫秒时间戳
     * @return   {bool} true：需要上报
     * @note    : 首帧仅记录计时基线；不在此更新时间戳，由调用方统一处理
     */
    bool should_emit_periodic_report(int64_t llNowMs);

    /* 归一化内部配置 */
    PeopleFlowConfig_S m_stConfig;
    /* 配置是否已应用 */
    bool m_bConfigValid = false;
    /* 轨迹状态存储 */
    CPeopleFlowTrackStore m_trackStore;
    /* 统计累计状态仓库 */
    CPeopleFlowStatisticsState m_stateStore;
    /* 上一次统计上报时间戳，wall clock 毫秒 */
    int64_t m_llLastReportMs = 0;
    /* 最近一帧处理摘要 */
    PeopleFlowFrameSummary_S m_stLastSummary;
    /* 容量满载降级告警限频时间戳 */
    int64_t m_llLastFullWarnMs = 0;
};
} // namespace PeopleFlow_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_STATISTICS
