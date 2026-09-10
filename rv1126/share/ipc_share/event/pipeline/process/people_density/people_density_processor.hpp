/**
 * @FilePath     : people_density_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-26 15:00:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 共享人员密度事件处理器
 */

#pragma once

#if CAP_AI_PEOPLE_DENSITY_PIPELINE && !CAP_AI_PEOPLE_STATISTICS
#error "CAP_AI_PEOPLE_DENSITY_PIPELINE=1 要求 CAP_AI_PEOPLE_STATISTICS=1，请检查设备画像配置"
#endif

#if CAP_AI_PEOPLE_DENSITY_PIPELINE

#include <cstdint>

#include "people_density_types.hpp"
#include "result_processor.hpp"

namespace AiPipeline_NS
{
namespace PeopleDensity_NS
{
/* 共享人员密度事件处理器，只依赖标准检测结果、类型化配置和输出端口 */
class CPeopleDensityProcessor : public IResultProcessor
{
public:
    /**
     * @brief   : 构造人员密度处理器
     */
    CPeopleDensityProcessor() = default;

    /**
     * @brief   : 应用归一化配置
     * @param    {const PeopleDensityConfig_S &} stConfig：归一化配置
     * @return   {int} OK：成功
     * @note    : 使能从 true 变 false 时清空运行态，避免旧累计量跨新配置上报
     */
    int apply_config(const PeopleDensityConfig_S &stConfig);

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
     * @brief   : 获取处理器静态描述
     * @return   {const ProcessorDescriptor_S &} 处理器描述
     */
    const ProcessorDescriptor_S &descriptor() const override;

    /**
     * @brief   : 处理单帧标准检测结果
     * @param    {const DetectionBatch_S &} stBatch：标准检测批次，只读
     * @param    {ProcessorOutput_S &} stOutput：类型化输出收集器
     * @return   {int} OK：成功
     * @note    : 仅统计带 Track ID 的 HEAD 目标；低置信度与区域外目标不计数
     */
    int process(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput) override;

private:
    /**
     * @brief   : 根据人数计算命中的密度等级事件
     * @param    {uint32_t} nPeopleCount：当前区域人数
     * @return   {Event::Type_E} 命中的等级事件，未命中返回 PEOPLE_DENSITY_DETECTION 作无等级标识
     * @note    : 优先级 severe > medium > normal，与旧实现一致
     */
    Event::Type_E calculate_alarm_event_type(uint32_t nPeopleCount) const;

    /**
     * @brief   : 判断是否到达周期上报时间
     * @param    {int64_t} llNowMs：wall clock 毫秒时间戳
     * @return   {bool} true：需要上报
     * @note    : 首帧仅记录计时基线；到达间隔时推进基线，与旧实现一致
     */
    bool should_emit_periodic_report(int64_t llNowMs);

    /**
     * @brief   : 更新区间人时累计，用于平均停留时间估算
     * @param    {int64_t} llNowMs：wall clock 毫秒时间戳
     * @param    {uint32_t} nPeopleCount：当前帧区域人数
     * @return   {void}
     * @note    : 区间人时 occupancy_ms = Σ(c_i × Δt_i)，人数规模取区间内最大值；
     *            采样时间回退时重置区间并告警
     */
    void update_stay_accumulator(int64_t llNowMs, uint32_t nPeopleCount);

    /**
     * @brief   : 计算当前上报区间的平均停留时间
     * @return   {uint32_t} 平均停留秒数，无有效累计时返回 0
     */
    uint32_t average_stay_time_sec() const;

    /**
     * @brief   : 重置区间停留统计
     * @param    {uint32_t} nCurrentPeopleCount：重置后延续到下一区间的当前人数
     * @return   {void}
     */
    void reset_stay_accumulator(uint32_t nCurrentPeopleCount);

    /* 归一化内部配置 */
    PeopleDensityConfig_S m_stConfig;
    /* 配置是否已应用 */
    bool m_bConfigValid = false;
    /* 上一次统计上报时间戳，wall clock 毫秒 */
    int64_t m_llLastReportMs = 0;
    /* 统计报告递增序号 */
    uint32_t m_nReportSeq = 0;
    /* 上一次停留时间采样时间戳，wall clock 毫秒 */
    int64_t m_llStaySampleLastMs = 0;
    /* 当前上报区间累计人时，单位为 人*毫秒 */
    int64_t m_llStayOccupancyMs = 0;
    /* 上一次采样时区域内人数，代表相邻采样点之间的人数状态 */
    uint32_t m_nStayLastPeopleCount = 0;
    /* 当前上报区间内观测到的最大区域人数 */
    uint32_t m_nStayMaxPeopleCount = 0;
};
} // namespace PeopleDensity_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_DENSITY_PIPELINE
