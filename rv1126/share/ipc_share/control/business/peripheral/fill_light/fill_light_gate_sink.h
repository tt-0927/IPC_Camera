/**
 * @FilePath     : fill_light_gate_sink.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 10:39:59
 * @Description  : 外设补光一级总控运行态消费者契约
 */

#pragma once

#include "fill_light_gate_state.h"

/**
 * @brief 外设补光一级总控状态消费者。
 */
class IFillLightGateSink
{
public:
    /**
     * @brief   : 析构外设补光gate消费者接口
     * @return   {void}
     */
    virtual ~IFillLightGateSink() = default;

    /**
     * @brief   : 提交外设补光一级总控的最新状态
     * @param    {const Peripheral_NS::FillLightGateState_S&} stGate：准入及物理功率上限
     * @return   {int} OK：成功，非OK：运行态拒绝或未初始化
     * @note    : 实现方只能更新运行态意图，不得回写外设或日夜配置。
     */
    virtual int update_fill_light_gate(const Peripheral_NS::FillLightGateState_S &stGate) = 0;
};
