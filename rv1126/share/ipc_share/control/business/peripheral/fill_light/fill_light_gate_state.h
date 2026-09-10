/**
 * @FilePath     : fill_light_gate_state.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 10:39:59
 * @Description  : 外设补光一级总控准入状态模型
 */

#pragma once

namespace Peripheral_NS
{

/**
 * @brief 外设补光一级总控禁止原因。
 */
enum class FillLightBlockReason_E
{
    NONE,               /* 总控已允许输出。 */
    DISABLED,           /* 外设补光总开关关闭。 */
    OUTSIDE_TIME_RANGE, /* 定时总控未命中当前时间。 */
    ZERO_POWER_LIMIT,   /* 物理功率上限为0%。 */
};

/**
 * @brief 外设补光一级总控计算结果。
 */
struct FillLightGateState_S
{
    /* 是否允许灯光请求继续进入硬件目标 */
    bool bAllowed;
    /* 当前生效的全局物理功率上限 */
    unsigned int nPowerLimitPercent;
    /* 禁止原因，允许时为NONE */
    FillLightBlockReason_E enBlockReason;

    /**
     * @brief   : 构造默认禁止输出的安全gate状态
     * @return   {void}
     */
    FillLightGateState_S() : bAllowed(false), nPowerLimitPercent(0), enBlockReason(FillLightBlockReason_E::DISABLED)
    {
    }
};

} // namespace Peripheral_NS
