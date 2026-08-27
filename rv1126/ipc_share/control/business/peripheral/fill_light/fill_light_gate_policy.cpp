/**
 * @FilePath     : fill_light_gate_policy.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 10:39:59
 * @Description  : 外设补光一级总控时间准入与物理功率限幅纯策略实现
 */

#include "fill_light_gate_policy.h"

#include <algorithm>
#include <cstdint>

namespace
{
/* 每天秒数。 */
constexpr int SECONDS_PER_DAY = 24 * 60 * 60;
/* 百分比最大值。 */
constexpr unsigned int PERCENT_MAX = 100U;
/* 四舍五入到整数百分比使用的半分母偏移。 */
constexpr std::uint32_t ROUND_HALF_UP_OFFSET = 50U;

/**
 * @brief   : 把时分秒转换为当天秒数
 * @param    {const Common::Time_S&} stTime：合法时间字段
 * @return   {int} 当天秒数，24:00:00返回86400
 */
int to_sec_of_day(const Common::Time_S &stTime)
{
    /* 允许结束时间表达24:00:00，供全天边界计算使用。 */
    return stTime.nHour * 60 * 60 + stTime.nMinute * 60 + stTime.nSecond;
}

/**
 * @brief   : 判断定时模式当前是否处于补光准入范围
 * @param    {const Common::SchedTime_S&} stRange：外设定时时间范围
 * @param    {int} nNowSecOfDay：当前当天秒数
 * @return   {bool} true：命中范围，false：未命中或为空范围
 */
bool is_in_time_range(const Common::SchedTime_S &stRange, int nNowSecOfDay)
{
    const int nBegin = to_sec_of_day(stRange.stStart);
    const int nEnd = to_sec_of_day(stRange.stStop);
    /* 规范化输入，避免校时或测试传入负秒数时破坏跨午夜判断。 */
    const int nNow = ((nNowSecOfDay % SECONDS_PER_DAY) + SECONDS_PER_DAY) % SECONDS_PER_DAY;

    if (nBegin == nEnd)
    {
        return false;
    }

    if (nBegin < nEnd)
    {
        return nNow >= nBegin && nNow < nEnd;
    }

    return nNow >= nBegin || nNow < nEnd;
}
} // namespace

namespace FillLightPolicy_NS
{

Peripheral_NS::FillLightGateState_S evaluate_gate(const Peripheral_NS::FillLightGlobalConfig_S &stConfig, int nNowSecOfDay)
{
    /* 上限先钳制到物理百分比范围；后续驱动只能使用该最终限制。 */
    Peripheral_NS::FillLightGateState_S stGate;
    stGate.nPowerLimitPercent = std::min(stConfig.nPowerLimitPercent, PERCENT_MAX);

    /* 总开关优先级最高：关闭时包括告警闪烁在内的任何灯光都不允许输出。 */
    if (!stConfig.bEnabled)
    {
        stGate.enBlockReason = Peripheral_NS::FillLightBlockReason_E::DISABLED;
        return stGate;
    }

    /* 0%上限等价于硬件无输出，即使场景请求100%也必须阻断。 */
    if (stGate.nPowerLimitPercent == 0U)
    {
        stGate.enBlockReason = Peripheral_NS::FillLightBlockReason_E::ZERO_POWER_LIMIT;
        return stGate;
    }

    /* 定时模式是一级总控时间门，不能被日夜场景或告警闪烁绕过。 */
    if (stConfig.enMode == Peripheral_NS::FillLightControlMode_E::TIME && !is_in_time_range(stConfig.stTimeRange, nNowSecOfDay))
    {
        stGate.enBlockReason = Peripheral_NS::FillLightBlockReason_E::OUTSIDE_TIME_RANGE;
        return stGate;
    }

    stGate.bAllowed = true;
    stGate.enBlockReason = Peripheral_NS::FillLightBlockReason_E::NONE;
    return stGate;
}

unsigned int calculate_output_level(unsigned int nRequestedLevel, unsigned int nPowerLimitPercent)
{
    /* 两个输入均来自配置/策略边界，重复钳制保证该纯函数可独立安全复用。 */
    const std::uint32_t nNormalizedRequest = std::min(nRequestedLevel, PERCENT_MAX);
    const std::uint32_t nNormalizedLimit = std::min(nPowerLimitPercent, PERCENT_MAX);
    return static_cast<unsigned int>((nNormalizedRequest * nNormalizedLimit + ROUND_HALF_UP_OFFSET) / PERCENT_MAX);
}

} // namespace FillLightPolicy_NS
