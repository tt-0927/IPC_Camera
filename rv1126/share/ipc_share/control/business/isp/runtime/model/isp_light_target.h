/**
 * @FilePath     : isp_light_target.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:05:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : ISP灯光设置模型，供硬件执行器下发给外设控制器
 */

#pragma once

#include "alarm_define.h"
#include "isp_define.h"

namespace ISP
{

/**
 * @brief 待应用的灯光设置，由硬件执行器交给外设控制器执行。
 */
struct IspLightTarget_S
{
    /* 灯光类型 */
    LightType_E enLightType;
    /* 灯光亮度等级 [0,100] */
    unsigned int nLightLevel;
    /* 是否闪烁（临时灯光生效时为 true） */
    bool bFlashing;
    /* 闪烁持续时间(秒)，bFlashing为true时有效 */
    int nFlashTimeSec;
    /* 闪烁频率，bFlashing为true时有效 */
    Alarm::FlashFrequency_E enFlashFrequency;

    IspLightTarget_S()
        : enLightType(LIGHT_TYPE_CLOSE), nLightLevel(0), bFlashing(false), nFlashTimeSec(0),
          enFlashFrequency(Alarm::FlashFrequency_E::FLASH_STEADY_ON)
    {
    }
};

} // namespace ISP
