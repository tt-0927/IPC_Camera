/**
 * @FilePath     : isp_peripheral_controller.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:43:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 15:17:31
 * @Description  : 共享ISP外设控制适配端口实现
 */

#include "isp_peripheral_controller.h"

#include "IpcRet.h"
#include "dlog.h"
#include "alarm_define.h"
#include "fill_light_driver.h"
#include "ircut_driver.h"

namespace
{
/**
 * @brief   : 将ISP兼容灯型转换为单一物理补光通道
 * @param    {ISP::LightType_E} enLightType：ISP兼容灯型
 * @param    {Peripheral_NS::LightChannel_E&} enChannel：物理通道输出
 * @return   {int} OK：成功，ERR_PARAM：混合或未知灯型
 */
int to_light_channel(ISP::LightType_E enLightType, Peripheral_NS::LightChannel_E &enChannel)
{
    switch (enLightType)
    {
    case ISP::LIGHT_TYPE_CLOSE:
        enChannel = Peripheral_NS::LightChannel_E::NONE;
        return OK;
    case ISP::LIGHT_TYPE_WHITE:
    case ISP::LIGHT_TYPE_WHITE_ON_RED_OFF:
        enChannel = Peripheral_NS::LightChannel_E::WHITE;
        return OK;
    case ISP::LIGHT_TYPE_RED:
    case ISP::LIGHT_TYPE_RED_ON_WHITE_OFF:
        enChannel = Peripheral_NS::LightChannel_E::INFRARED;
        return OK;
    default:
        return ERR_PARAM;
    }
}

/**
 * @brief   : 获取物理补光通道的可读名称
 * @param    {Peripheral_NS::LightChannel_E} enChannel：物理补光通道
 * @return   {const char*} 用于日志的中文通道名称
 */
const char *get_light_channel_name(Peripheral_NS::LightChannel_E enChannel)
{
    switch (enChannel)
    {
    case Peripheral_NS::LightChannel_E::NONE:
        return "关闭";
    case Peripheral_NS::LightChannel_E::WHITE:
        return "白光";
    case Peripheral_NS::LightChannel_E::INFRARED:
        return "红外光";
    default:
        return "未知通道";
    }
}

/**
 * @brief   : 将ISP IR-CUT目标转换为物理位置
 * @param    {ISP::IspIrCutTarget_E} enIspTarget：ISP运行态目标
 * @param    {IrCutTarget_E&} enHardwareTarget：物理位置输出
 * @return   {int} OK：成功，ERR_PARAM：未知目标
 */
int to_ircut_target(ISP::IspIrCutTarget_E enIspTarget, IrCutTarget_E &enHardwareTarget)
{
    switch (enIspTarget)
    {
    case ISP::IspIrCutTarget_E::NONE:
        enHardwareTarget = IrCutTarget_E::NONE;
        return OK;
    case ISP::IspIrCutTarget_E::DAY:
        enHardwareTarget = IrCutTarget_E::DAY;
        return OK;
    case ISP::IspIrCutTarget_E::NIGHT:
        enHardwareTarget = IrCutTarget_E::NIGHT;
        return OK;
    default:
        return ERR_PARAM;
    }
}

/**
 * @brief   : 将事件告警闪烁频率转换为物理频率
 * @param    {Alarm::FlashFrequency_E} enAlarmFrequency：事件协议频率
 * @param    {Peripheral_NS::FillLightFlashFrequency_E&} enHardwareFrequency：物理频率输出
 * @return   {int} OK：成功，ERR_PARAM：未知频率
 */
int to_flash_frequency(Alarm::FlashFrequency_E enAlarmFrequency, Peripheral_NS::FillLightFlashFrequency_E &enHardwareFrequency)
{
    switch (enAlarmFrequency)
    {
    case Alarm::FlashFrequency_E::FLASH_STEADY_ON:
        enHardwareFrequency = Peripheral_NS::FillLightFlashFrequency_E::STEADY_ON;
        return OK;
    case Alarm::FlashFrequency_E::FLASH_LOW_FREQ:
        enHardwareFrequency = Peripheral_NS::FillLightFlashFrequency_E::LOW_FREQ;
        return OK;
    case Alarm::FlashFrequency_E::FLASH_MID_FREQ:
        enHardwareFrequency = Peripheral_NS::FillLightFlashFrequency_E::MID_FREQ;
        return OK;
    case Alarm::FlashFrequency_E::FLASH_HIGH_FREQ:
        enHardwareFrequency = Peripheral_NS::FillLightFlashFrequency_E::HIGH_FREQ;
        return OK;
    default:
        return ERR_PARAM;
    }
}
} // namespace platform

CIspPeripheralController::CIspPeripheralController(CFillLightDriver &stFillLightDriver, CIrCutDriver &stIrCutDriver)
    : m_rstFillLightDriver(stFillLightDriver), m_rstIrCutDriver(stIrCutDriver)
{
}

int CIspPeripheralController::turn_off_light(ISP::LightType_E enLightType)
{
    /* 将运行态兼容灯型收敛为唯一物理通道，避免硬件驱动理解 ISP 组合枚举。 */
    Peripheral_NS::LightChannel_E enChannel = Peripheral_NS::LightChannel_E::NONE;
    const int nRet = to_light_channel(enLightType, enChannel);
    if (nRet != OK || enChannel == Peripheral_NS::LightChannel_E::NONE)
    {
        return ERR_PARAM;
    }
    /* 关灯仍由共享驱动串行执行，不能绕过其闪烁取消和白红互斥状态。 */
    const int nTurnOffRet = m_rstFillLightDriver.turn_off(enChannel);
    if (nTurnOffRet == OK)
    {
        dlog_info("ISP补光物理通道已关闭, channel:%s", get_light_channel_name(enChannel));
    }
    return nTurnOffRet;
}

int CIspPeripheralController::switch_ircut(ISP::IspIrCutTarget_E enTarget)
{
    /* 物理驱动只接收光学位置，协议运行态枚举必须在边界处完成转换。 */
    IrCutTarget_E enHardwareTarget = IrCutTarget_E::NONE;
    const int nRet = to_ircut_target(enTarget, enHardwareTarget);
    /* 转换失败时禁止下发未知电平组合，保持IR-CUT最后成功位置。 */
    return nRet == OK ? m_rstIrCutDriver.switch_target(enHardwareTarget) : nRet;
}

int CIspPeripheralController::apply_light_target(const ISP::IspLightTarget_S &stTarget)
{
    /* 该局部目标剥离ISP/告警协议类型，作为共享硬件驱动唯一输入。 */
    Peripheral_NS::FillLightHardwareTarget_S stHardwareTarget;
    /* 先校验灯型映射，避免后续把未知策略类型解释为物理输出。 */
    int nRet = to_light_channel(stTarget.enLightType, stHardwareTarget.enChannel);
    if (nRet != OK)
    {
        return nRet;
    }
    /* arbiter已完成Gate限幅；adapter只搬运最终物理亮度，不得再次计算策略。 */
    stHardwareTarget.nOutputLevel = stTarget.nLightLevel;
    stHardwareTarget.bFlashing = stTarget.bFlashing;
    /* 告警协议频率不能泄漏至硬件层，统一转换为物理闪烁节奏。 */
    nRet = to_flash_frequency(stTarget.enFlashFrequency, stHardwareTarget.enFlashFrequency);
    if (nRet != OK)
    {
        return nRet;
    }
    /* 持续时间由驱动线程使用单调时钟计时，避免受系统校时影响。 */
    stHardwareTarget.nFlashTimeSec = stTarget.nFlashTimeSec;
    nRet = m_rstFillLightDriver.apply_target(stHardwareTarget);
    if (nRet == OK)
    {
        if (stHardwareTarget.enChannel == Peripheral_NS::LightChannel_E::NONE || stHardwareTarget.nOutputLevel == 0U)
        {
            dlog_info("ISP补光最终目标已关闭");
        }
        else
        {
            dlog_info("ISP补光最终目标已开启, channel:%s, level:%u, flashing:%d, flash_time:%us, frequency:%d",
                      get_light_channel_name(stHardwareTarget.enChannel),
                      stHardwareTarget.nOutputLevel,
                      stHardwareTarget.bFlashing,
                      stHardwareTarget.nFlashTimeSec,
                      static_cast<int>(stHardwareTarget.enFlashFrequency));
        }
    }
    return nRet;
}
