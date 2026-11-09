/**
 * @FilePath     : peripheral_profile_builder.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:43:07
 * @Description  : RV1126B外设板级画像构建实现
 */

#include "peripheral_profile_builder.h"

#include "IpcRet.h"

namespace
{
constexpr unsigned int WHITE_LIGHT_PWM_CONTROLLER = 1U;
constexpr unsigned int WHITE_LIGHT_PWM_CHANNEL = 0U;
constexpr unsigned int PWM_PERIOD_NS = 20000U;
constexpr unsigned int WHITE_LIGHT_DUTY_OFFSET = 0U;
constexpr unsigned int WHITE_LIGHT_DUTY_STEP = 200U;
constexpr unsigned int WHITE_LIGHT_DUTY_MAX = 20000U;

/**
 * @brief   : 填充RV1126B白光PWM画像
 * @param    {FillLightProfile_S&} stProfile：补光画像输出
 * @return   {void}
 */
void build_fill_light_profile(FillLightProfile_S &stProfile)
{
    /*
     * info: 板级资料中的红外预留为controller 2/channel 0，但当前产品不支持红外；
     * 该预留只保留在文档事实中，不能写入stInfrared或触发任何PWM申请。
     */
    /* RV1126B只有白光输出，红外画像保持默认值以使共享驱动拒绝所有红外请求。 */
    /* step: 先恢复默认画像，确保预留红外通道不会继承其他机型的可用字段。 */
    stProfile = FillLightProfile_S();
    stProfile.bInfraredSupported = false;
    stProfile.stWhite.nController = WHITE_LIGHT_PWM_CONTROLLER;
    stProfile.stWhite.nChannel = WHITE_LIGHT_PWM_CHANNEL;
    stProfile.stWhite.nPeriod = PWM_PERIOD_NS;
    stProfile.stWhite.nDutyOffset = WHITE_LIGHT_DUTY_OFFSET;
    stProfile.stWhite.nDutyStep = WHITE_LIGHT_DUTY_STEP;
    stProfile.stWhite.nDutyMax = WHITE_LIGHT_DUTY_MAX;
    /* info: 切换白光前等待100毫秒，给PWM输出和灯珠驱动留下稳定时间。 */
    stProfile.nMutualExclusionSettleMs = 100U;
}
} // namespace

namespace Rv1126bPeripheralProfileBuilder_NS
{
int build_profile(FillLightProfile_S &stFillLightProfile, IrCutProfile_S &stIrCutProfile)
{
    /* 先清空输出，失败重试时不能沿用上一次可能更宽的硬件画像。 */
    stFillLightProfile = FillLightProfile_S();
    stIrCutProfile = IrCutProfile_S();
#if !CAP_LIGHT_WHITE_ONLY || CAP_ISP_IR_SWITCH
    /* ! 当前接线只允许白光；能力宏与板级画像不一致时禁止启动，避免误写红外或IR-CUT。 */
    return ERR_UNSUPPORT;
#else
    /* done: 画像已通过宏门禁，后续共享驱动只能看到白光controller 1/channel 0。 */
    build_fill_light_profile(stFillLightProfile);
    return OK;
#endif
}
} // namespace Rv1126bPeripheralProfileBuilder_NS
