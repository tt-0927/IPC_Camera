/**
 * @FilePath     : ircut_profile.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:56:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-18 09:28:44
 * @Description  : 摄像机IR-CUT GPIO与脉冲方向画像声明
 */

#pragma once

/**
 * @brief 摄像机IR-CUT物理位置。
 * @note  硬件层不依赖ISP运行态枚举，由上层适配器负责转换。
 */
enum class IrCutTarget_E
{
    NONE,  /* 不执行IR-CUT动作。 */
    DAY,   /* 切至白天滤光片位置。 */
    NIGHT, /* 切至夜晚滤光片位置。 */
};

/**
 * @brief 摄像机IR-CUT稳定设备画像。
 */
struct IrCutProfile_S
{
    /* 当前机型是否支持IR-CUT。 */
    bool bSupported;
    /* IR-CUT第一路GPIO编号。 */
    unsigned int nGpioPin1;
    /* IR-CUT第二路GPIO编号。 */
    unsigned int nGpioPin2;
    /* 白天方向第一路脉冲电平。 */
    unsigned int nDayPin1Value;
    /* 白天方向第二路脉冲电平。 */
    unsigned int nDayPin2Value;
    /* 夜晚方向第一路脉冲电平。 */
    unsigned int nNightPin1Value;
    /* 夜晚方向第二路脉冲电平。 */
    unsigned int nNightPin2Value;
    /* 脉冲结束后的安全空闲电平。 */
    unsigned int nIdleValue;
    /* 动作脉冲保持时间，单位毫秒。 */
    unsigned int nPulseHoldMs;

    /**
     * @brief   : 构造默认不支持IR-CUT的安全画像
     * @return   {void}
     */
    IrCutProfile_S()
        : bSupported(false), nGpioPin1(0), nGpioPin2(0), nDayPin1Value(0), nDayPin2Value(0), nNightPin1Value(0),
          nNightPin2Value(0), nIdleValue(0), nPulseHoldMs(0)
    {
    }
};
