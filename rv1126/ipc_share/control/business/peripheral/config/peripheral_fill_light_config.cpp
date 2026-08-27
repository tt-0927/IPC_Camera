/**
 * @FilePath     : peripheral_fill_light_config.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-23 09:17:09
 * @Description  : 外设补光全局配置协议校验与强类型转换实现
 */

#include "peripheral_fill_light_config.h"

#include "IpcRet.h"

namespace
{
/* 外设协议定时模式值。 */
constexpr int PERIPHERAL_LIGHT_MODE_TIME = 0;
/* 外设协议自动模式值。 */
constexpr int PERIPHERAL_LIGHT_MODE_AUTO = 1;
/* 补光物理功率百分比上限。 */
constexpr unsigned int POWER_LIMIT_MAX_PERCENT = 100U;
/* 每分钟秒数上限（开区间）。 */
constexpr unsigned int SECONDS_PER_MINUTE = 60U;
/* 每小时分钟数上限（开区间）。 */
constexpr unsigned int MINUTES_PER_HOUR = 60U;
/* 一天结束时刻，只有结束时间允许表达24:00:00。 */
constexpr unsigned int END_OF_DAY_HOUR = 24U;
/* 毫秒字段上限（开区间）。 */
constexpr unsigned int MILLISECONDS_PER_SECOND = 1000U;

/**
 * @brief   : 校验协议时间字段
 * @param    {const Common::Time_S&} stTime：待校验时间
 * @param    {bool} bAllowEndOfDay：是否允许24:00:00
 * @return   {bool} true：合法，false：非法
 */
bool is_valid_time(const Common::Time_S &stTime, bool bAllowEndOfDay)
{
    if (stTime.nMinute >= MINUTES_PER_HOUR || stTime.nSecond >= SECONDS_PER_MINUTE || stTime.nMilliSec >= MILLISECONDS_PER_SECOND)
    {
        return false;
    }

    if (stTime.nHour < END_OF_DAY_HOUR)
    {
        return true;
    }

    return bAllowEndOfDay && stTime.nHour == END_OF_DAY_HOUR && stTime.nMinute == 0 && stTime.nSecond == 0 &&
           stTime.nMilliSec == 0;
}

/**
 * @brief   : 判断定时区间是否严格递增
 * @param    {const Common::SchedTime_S&} stTimeRange：已校验字段范围的定时区间
 * @return   {bool} true：开始时间早于结束时间，false：开始时间不早于结束时间
 * @note    : 定时补光不支持跨零点，避免单日gate无法确定区间边界。
 */
bool is_time_range_increasing(const Common::SchedTime_S &stTimeRange)
{
    /* 从当天00:00开始计算的开始时间毫秒数。 */
    const unsigned long long u64StartTime = (((static_cast<unsigned long long>(stTimeRange.stStart.nHour) * MINUTES_PER_HOUR +
                                               stTimeRange.stStart.nMinute) *
                                                  SECONDS_PER_MINUTE +
                                              stTimeRange.stStart.nSecond) *
                                             MILLISECONDS_PER_SECOND) +
                                            stTimeRange.stStart.nMilliSec;
    /* 从当天00:00开始计算的结束时间毫秒数。 */
    const unsigned long long u64StopTime = (((static_cast<unsigned long long>(stTimeRange.stStop.nHour) * MINUTES_PER_HOUR +
                                              stTimeRange.stStop.nMinute) *
                                                 SECONDS_PER_MINUTE +
                                             stTimeRange.stStop.nSecond) *
                                            MILLISECONDS_PER_SECOND) +
                                           stTimeRange.stStop.nMilliSec;

    return u64StartTime < u64StopTime;
}
} // namespace

namespace Peripheral_NS
{

int decode_fill_light_config(const System::Peripheral_S &stProtocolConfig, FillLightGlobalConfig_S &stBusinessConfig)
{
    /* 协议模式仅允许两种已定义语义，未知值不能退化为自动模式。 */
    if (stProtocolConfig.nLightMode != PERIPHERAL_LIGHT_MODE_TIME && stProtocolConfig.nLightMode != PERIPHERAL_LIGHT_MODE_AUTO)
    {
        return ERR_PARAM;
    }

    if (stProtocolConfig.nLevel > POWER_LIMIT_MAX_PERCENT)
    {
        return ERR_PARAM;
    }

    if (!is_valid_time(stProtocolConfig.stLightTime.stStart, false) || !is_valid_time(stProtocolConfig.stLightTime.stStop, true))
    {
        return ERR_PARAM;
    }

    /* ! 定时补光的开始时间必须严格早于结束时间，拒绝空区间和跨零点区间。 */
    if (stProtocolConfig.nLightMode == PERIPHERAL_LIGHT_MODE_TIME && !is_time_range_increasing(stProtocolConfig.stLightTime))
    {
        return ERR_PARAM;
    }

    /* 使用临时强类型快照，确保任一字段失败时不污染调用方已有配置。 */
    FillLightGlobalConfig_S stDecoded;
    stDecoded.bEnabled = stProtocolConfig.bEnable;
    stDecoded.enMode = (stProtocolConfig.nLightMode == PERIPHERAL_LIGHT_MODE_TIME) ? FillLightControlMode_E::TIME
                                                                                   : FillLightControlMode_E::AUTO;
    stDecoded.stTimeRange = stProtocolConfig.stLightTime;
    stDecoded.nPowerLimitPercent = stProtocolConfig.nLevel;
    stBusinessConfig = stDecoded;
    return OK;
}

void encode_fill_light_config(const FillLightGlobalConfig_S &stBusinessConfig, System::Peripheral_S &stProtocolConfig)
{
    /* 仅做模型映射，不触发持久化或Gate更新，保持DTO转换无副作用。 */
    stProtocolConfig.bEnable = stBusinessConfig.bEnabled;
    stProtocolConfig.nLightMode = (stBusinessConfig.enMode == FillLightControlMode_E::TIME) ? PERIPHERAL_LIGHT_MODE_TIME
                                                                                            : PERIPHERAL_LIGHT_MODE_AUTO;
    stProtocolConfig.stLightTime = stBusinessConfig.stTimeRange;
    stProtocolConfig.nLevel = stBusinessConfig.nPowerLimitPercent;
}

} // namespace Peripheral_NS
