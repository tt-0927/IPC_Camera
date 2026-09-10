/**
 * @FilePath     : fill_light_hardware_target.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-18 09:28:44
 * @Description  : 摄像机补光物理通道与最终硬件目标模型
 */

#pragma once

namespace Peripheral_NS
{

/**
 * @brief 摄像机补光灯物理通道。
 */
enum class LightChannel_E
{
    NONE,     /* 无物理补光目标，表示关闭。 */
    WHITE,    /* 白光补光PWM通道。 */
    INFRARED, /* 红外补光PWM通道。 */
};

/**
 * @brief 摄像机补光物理闪烁频率。
 * @note  硬件层不依赖事件告警协议枚举，由上层适配器负责转换。
 */
enum class FillLightFlashFrequency_E
{
    STEADY_ON, /* 闪烁任务期间保持常亮，直到持续时间到期。 */
    LOW_FREQ,  /* 低频亮灭节拍。 */
    MID_FREQ,  /* 中频亮灭节拍。 */
    HIGH_FREQ, /* 高频亮灭节拍。 */
};

/**
 * @brief 已经过一级总控准入和功率限幅的补光硬件目标。
 */
struct FillLightHardwareTarget_S
{
    /* 唯一目标物理通道 */
    LightChannel_E enChannel;
    /* 最终PWM输出等级，范围为0到100 */
    unsigned int nOutputLevel;
    /* 是否执行告警闪烁 */
    bool bFlashing;
    /* 告警闪烁频率 */
    FillLightFlashFrequency_E enFlashFrequency;
    /* 告警闪烁持续时间，单位秒 */
    int nFlashTimeSec;

    /**
     * @brief   : 构造默认关闭全部补光的硬件目标
     * @return   {void}
     */
    FillLightHardwareTarget_S()
        : enChannel(LightChannel_E::NONE), nOutputLevel(0), bFlashing(false),
          enFlashFrequency(FillLightFlashFrequency_E::STEADY_ON), nFlashTimeSec(0)
    {
    }
};

} // namespace Peripheral_NS
