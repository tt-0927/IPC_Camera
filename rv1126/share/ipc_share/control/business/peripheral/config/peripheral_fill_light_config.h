/**
 * @FilePath     : peripheral_fill_light_config.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 10:39:59
 * @Description  : 外设补光全局配置强类型模型与协议转换声明
 */

#pragma once

#include "common_define.h"
#include "system_define.h"

namespace Peripheral_NS
{

/**
 * @brief 外设补光一级总控模式。
 */
enum class FillLightControlMode_E
{
    TIME, /* 仅在一级总控时间范围内允许任何补光输出。 */
    AUTO, /* 不施加时间限制，由显示场景继续裁决补光。 */
};

/**
 * @brief 外设补光一级总控配置。
 * @note  本结构只表达业务语义，不直接承担JSON序列化。
 */
struct FillLightGlobalConfig_S
{
    /* 是否允许任何补光灯输出 */
    bool bEnabled;
    /* 总控时间准入模式 */
    FillLightControlMode_E enMode;
    /* 定时模式使用的全局准入时间范围 */
    Common::SchedTime_S stTimeRange;
    /* 白光、红外和告警闪光共同遵守的物理功率上限 */
    unsigned int nPowerLimitPercent;

    /**
     * @brief   : 构造兼容现有协议默认值的外设补光配置
     * @return   {void}
     */
    FillLightGlobalConfig_S() : bEnabled(true), enMode(FillLightControlMode_E::AUTO), nPowerLimitPercent(50)
    {
    }
};

/**
 * @brief   : 校验协议配置并转换为外设补光强类型模型
 * @param    {const System::Peripheral_S&} stProtocolConfig：网页协议外设配置
 * @param    {FillLightGlobalConfig_S&} stBusinessConfig：强类型配置输出
 * @return   {int} OK：成功，ERR_PARAM：模式、强度或时间非法
 */
int decode_fill_light_config(const System::Peripheral_S &stProtocolConfig, FillLightGlobalConfig_S &stBusinessConfig);

/**
 * @brief   : 将外设补光强类型模型转换为现有网页协议配置
 * @param    {const FillLightGlobalConfig_S&} stBusinessConfig：强类型配置
 * @param    {System::Peripheral_S&} stProtocolConfig：网页协议配置输出
 * @return   {void}
 */
void encode_fill_light_config(const FillLightGlobalConfig_S &stBusinessConfig, System::Peripheral_S &stProtocolConfig);

} // namespace Peripheral_NS
