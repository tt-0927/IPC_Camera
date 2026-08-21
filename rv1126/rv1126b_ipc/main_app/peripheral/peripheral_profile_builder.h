/**
 * @FilePath     : peripheral_profile_builder.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:39:13
 * @Description  : RV1126B外设板级画像构建声明
 */

#pragma once

#include "fill_light_profile.h"
#include "ircut_profile.h"

namespace Rv1126bPeripheralProfileBuilder_NS
{
/**
 * @brief   : 构建RV1126B白光PWM和IR-CUT外设画像
 * @param    {FillLightProfile_S&} stFillLightProfile：白光PWM画像输出
 * @param    {IrCutProfile_S&} stIrCutProfile：IR-CUT画像输出
 * @return   {int} OK：成功，非OK：当前产品能力不兼容
 * @note    : 两款RV1126B产品共用controller 1/channel 0白光接线；controller 2/channel 0
 *            只作为板级预留资料，不进入画像，不创建红外或IR-CUT物理输出。
 */
int build_profile(FillLightProfile_S &stFillLightProfile, IrCutProfile_S &stIrCutProfile);
} // namespace Rv1126bPeripheralProfileBuilder_NS
