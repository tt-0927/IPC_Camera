/**
 * @FilePath     : fill_light_profile.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:56:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 16:05:53
 * @Description  : 摄像机补光灯设备能力画像声明
 */

#pragma once

/**
 * @brief 摄像机单路补光PWM设备与用户亮度映射画像。
 */
struct FillLightPwmChannelProfile_S
{
    /* PWM控制器编号。 */
    unsigned int nController;
    /* PWM通道编号。 */
    unsigned int nChannel;
    /* PWM周期。 */
    unsigned int nPeriod;
    /* 用户亮度映射的占空比偏移。 */
    unsigned int nDutyOffset;
    /* 每一级用户亮度对应的占空比步长。 */
    unsigned int nDutyStep;
    /* 当前灯珠允许的最大原始占空比。 */
    unsigned int nDutyMax;

    /**
     * @brief   : 构造安全的默认PWM通道画像
     * @return   {void}
     */
    FillLightPwmChannelProfile_S() : nController(0), nChannel(0), nPeriod(20000), nDutyOffset(0), nDutyStep(0), nDutyMax(20000)
    {
    }
};

/**
 * @brief 摄像机补光灯稳定设备画像。
 */
struct FillLightProfile_S
{
    /* 当前机型是否具备红外补光通道。 */
    bool bInfraredSupported;
    /* 白光PWM通道和亮度映射。 */
    FillLightPwmChannelProfile_S stWhite;
    /* 红外PWM通道和亮度映射。 */
    FillLightPwmChannelProfile_S stInfrared;
    /* 互斥通道关闭后的电气稳定等待时间。 */
    unsigned int nMutualExclusionSettleMs;

    /**
     * @brief   : 构造默认禁止输出的补光画像
     * @return   {void}
     */
    FillLightProfile_S() : bInfraredSupported(false), nMutualExclusionSettleMs(100)
    {
    }
};
