/**
 * @FilePath     : peripheral_profile_builder.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 16:05:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 11:15:38
 * @Description  : Hi3516外设板级画像构建声明
 */

#pragma once

#include "fill_light_profile.h"
#include "ircut_profile.h"

namespace PeripheralProfileBuilder_NS
{
/**
 * @brief   : 按当前Hi3516产品能力宏填充外设板级画像
 * @param    {FillLightProfile_S&} stFillLightProfile：补光PWM与亮度映射画像输出
 * @param    {IrCutProfile_S&} stIrCutProfile：IR-CUT GPIO与脉冲画像输出
 * @return   {int} OK：成功
 * @note    : 仅在本业务仓消费产品宏；共享驱动只读取完成后的稳定画像。
 */
int build_profile(FillLightProfile_S &stFillLightProfile, IrCutProfile_S &stIrCutProfile);
} // namespace PeripheralProfileBuilder_NS
