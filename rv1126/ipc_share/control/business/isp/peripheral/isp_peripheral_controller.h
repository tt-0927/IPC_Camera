/**
 * @FilePath     : isp_peripheral_controller.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:43:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-18 09:28:44
 * @Description  : 共享ISP外设控制适配端口实现声明
 */

#pragma once

#include "isp_platform_adapters.h"

class CFillLightDriver;
class CIrCutDriver;

/**
 * @brief 共享ISP外设控制适配端口。
 * @note  仅做共享运行态模型到独立补光、IR-CUT设备驱动的转换，不包含平台SDK调用。
 */
class CIspPeripheralController : public IIspPeripheralController
{
public:
    /**
     * @brief   : 构造外设控制适配器。
     * @return   {void}
     */
    CIspPeripheralController(CFillLightDriver &stFillLightDriver, CIrCutDriver &stIrCutDriver);
    /**
     * @brief   : 销毁外设控制适配器。
     * @return   {void}
     */
    ~CIspPeripheralController() override = default;

    /**
     * @brief   : 关闭指定补光通道。
     * @param    {ISP::LightType_E} enLightType：白光或红外灯
     * @return   {int} OK：成功，非OK：PWM 操作失败
     */
    int turn_off_light(ISP::LightType_E enLightType) override;
    /**
     * @brief   : 将 IR-CUT 切至白天或夜间位置。
     * @param    {ISP::IspIrCutTarget_E} enTarget：目标位置
     * @return   {int} OK：成功，非OK：GPIO 操作失败
     */
    int switch_ircut(ISP::IspIrCutTarget_E enTarget) override;
    /**
     * @brief   : 执行仲裁后的补光目标并消除冲突灯光。
     * @param    {const ISP::IspLightTarget_S&} stTarget：目标灯型、亮度或闪烁参数
     * @return   {int} OK：成功，非OK：底层失败
     */
    int apply_light_target(const ISP::IspLightTarget_S &stTarget) override;

private:
    /* memory: 非拥有补光驱动引用；外设驱动生命周期必须覆盖本适配器。 */
    CFillLightDriver &m_rstFillLightDriver;
    /* memory: 非拥有IR-CUT驱动引用；调用期间由启动编排保证其有效。 */
    CIrCutDriver &m_rstIrCutDriver;
};
