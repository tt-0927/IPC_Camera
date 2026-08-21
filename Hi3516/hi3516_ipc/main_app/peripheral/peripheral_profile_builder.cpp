/**
 * @FilePath     : peripheral_profile_builder.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:56:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-21 15:17:12
 * @Description  : Hi3516外设板级画像构建实现
 */

#include "peripheral_profile_builder.h"

#include "IpcRet.h"

namespace
{
constexpr unsigned int GPIO_LOW_VALUE = 0U;
constexpr unsigned int GPIO_HIGH_VALUE = 1U;

/**
 * @brief   : 填充单路补光PWM通道和亮度映射
 * @param    {FillLightPwmChannelProfile_S&} stChannel：待填充的板级通道画像
 * @param    {unsigned int} nController：PWM控制器编号
 * @param    {unsigned int} nChannel：PWM通道编号
 * @param    {unsigned int} nDutyOffset：亮度映射占空比偏移
 * @param    {unsigned int} nDutyStep：亮度映射占空比步长
 * @param    {unsigned int} nDutyMax：灯珠允许的最大占空比
 * @return   {void}
 */
void set_pwm_channel(FillLightPwmChannelProfile_S &stChannel,
                     unsigned int nController,
                     unsigned int nChannel,
                     unsigned int nDutyOffset,
                     unsigned int nDutyStep,
                     unsigned int nDutyMax)
{
    stChannel.nController = nController;
    stChannel.nChannel = nChannel;
    stChannel.nPeriod = 20000U;
    stChannel.nDutyOffset = nDutyOffset;
    stChannel.nDutyStep = nDutyStep;
    stChannel.nDutyMax = nDutyMax;
}

/**
 * @brief   : 填充本机型白光、红外PWM接线和亮度映射
 * @param    {FillLightProfile_S&} stProfile：补光画像输出
 * @return   {void}
 * @note    : 通道号是板级数据，禁止迁移至共享驱动或运行态策略。
 */
void build_fill_light_profile(FillLightProfile_S &stProfile)
{
#if CAP_LIGHT_WHITE_ONLY
    /* 仅白光机型接线固定为PWM1_CH0；红外结构保持默认值，驱动会拒绝红外目标。 */
    stProfile.bInfraredSupported = false;
    set_pwm_channel(stProfile.stWhite, 1U, 0U, 0U, 200U, 20000U);
#else
    /* 双光机型通过不同通道完成白光与红外互斥输出。 */
    stProfile.bInfraredSupported = true;
#if CAP_IO_EXTERNAL_DDR_00S
    /* 外置DDR板复用PWM3，白光接CH1、红外接CH3。 */
    set_pwm_channel(stProfile.stWhite, 3U, 1U, 0U, 200U, 20000U);
    set_pwm_channel(stProfile.stInfrared, 3U, 3U, 1U, 104U, 10400U);
#else
    /* 标准板复用PWM0，白光接CH0、红外接CH2。 */
    set_pwm_channel(stProfile.stWhite, 0U, 0U, 0U, 200U, 20000U);
    set_pwm_channel(stProfile.stInfrared, 0U, 2U, 1U, 104U, 10400U);
#endif
#endif
    /* 切换通道前预留电气稳定窗口，防止灯珠驱动在交替导通时产生电源冲击。 */
    stProfile.nMutualExclusionSettleMs = 100U;
}

/**
 * @brief   : 填充本机型IR-CUT GPIO接线和方向脉冲
 * @param    {IrCutProfile_S&} stProfile：IR-CUT画像输出
 * @return   {void}
 * @note    : 产品接线差异仅在此处收敛，驱动只执行画像描述的安全脉冲。
 */
void build_ircut_profile(IrCutProfile_S &stProfile)
{
#if CAP_GPIO_LAYOUT_RV1126
    /* 当前Hi3516链路遇到RV1126布局时不允许误写GPIO，显式关闭IR-CUT能力。 */
    stProfile.bSupported = false;
    stProfile.nGpioPin1 = 0U;
    stProfile.nGpioPin2 = 0U;
#else
    /* 3852系列板级接线固定为47/46，两路组合脉冲由共享驱动串行执行。 */
    stProfile.bSupported = true;
    stProfile.nGpioPin1 = 47U;
    stProfile.nGpioPin2 = 46U;
#endif

#if CAP_GPIO_IR_CUT_JSON
    /* JSON控制布局的日夜方向与传统布局相反，只在画像层表达该差异。 */
    stProfile.nDayPin1Value = GPIO_LOW_VALUE;
    stProfile.nDayPin2Value = GPIO_HIGH_VALUE;
    stProfile.nNightPin1Value = GPIO_HIGH_VALUE;
    stProfile.nNightPin2Value = GPIO_LOW_VALUE;
#else
    /* 非JSON布局沿用传统线圈方向，驱动不需要理解产品宏。 */
    stProfile.nDayPin1Value = GPIO_HIGH_VALUE;
    stProfile.nDayPin2Value = GPIO_LOW_VALUE;
    stProfile.nNightPin1Value = GPIO_LOW_VALUE;
    stProfile.nNightPin2Value = GPIO_HIGH_VALUE;
#endif
    /* 脉冲结束统一回落到空闲电平，避免IR-CUT线圈被持续通电。 */
    stProfile.nIdleValue = GPIO_LOW_VALUE;
    stProfile.nPulseHoldMs = 1000U;
}
} // namespace

int PeripheralProfileBuilder_NS::build_profile(FillLightProfile_S &stFillLightProfile, IrCutProfile_S &stIrCutProfile)
{
    /* 先完成所有板级差异收敛，后续共享模块只消费不可变画像副本。 */
    build_fill_light_profile(stFillLightProfile);
    build_ircut_profile(stIrCutProfile);
    return OK;
}
