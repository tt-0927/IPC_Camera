/**
 * @FilePath     : isp_parameter_applier_hi3516.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:11:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : Hi3516 ISP参数适配端口实现声明
 */

#pragma once

#include "isp_platform_adapters.h"

/**
 * @brief Hi3516 ISP参数适配端口。
 * @note  包装CIspControl的六类setter和Gamma后处理。
 */
class CIspParameterApplierHi3516 : public IIspParameterApplier
{
public:
    /**
     * @brief   : 构造参数下发适配器。
     * @return   {void}
     */
    CIspParameterApplierHi3516() = default;
    /**
     * @brief   : 销毁参数下发适配器。
     * @return   {void}
     */
    ~CIspParameterApplierHi3516() = default;

    /**
     * @brief   : 设置本次参数重放的内部运行场景
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：目标内部运行场景
     * @return   {int} OK：成功，ERR_PARAM：场景非法
     */
    int set_runtime_scene_context(ISP::IspRuntimeScene_E enRuntimeScene) override;

    /**
     * @brief   : 下发亮度、对比度、饱和度和锐度。
     * @param    {const ISP::ImageParam_S&} stConfig：已校验图像参数
     * @return   {int} OK：成功，非OK：底层失败
     */
    int apply_image(const ISP::ImageParam_S &stConfig) override;
    /**
     * @brief   : 下发曝光及防横纹配置。
     * @param    {const ISP::ExposureAttr_S&} stConfig：已校验曝光参数
     * @return   {int} OK：成功，非OK：底层失败
     */
    int apply_exposure(const ISP::ExposureAttr_S &stConfig) override;
    /**
     * @brief   : 下发背光补偿、WDR 或 HLC 配置。
     * @param    {const ISP::BackLightArrt_S&} stConfig：已校验背光参数
     * @return   {int} OK：成功，非OK：底层失败
     */
    int apply_backlight(const ISP::BackLightArrt_S &stConfig) override;
    /**
     * @brief   : 下发自动或手动白平衡配置。
     * @param    {const ISP::AwbAttr_S&} stConfig：已校验白平衡参数
     * @return   {int} OK：成功，非OK：底层失败
     */
    int apply_awb(const ISP::AwbAttr_S &stConfig) override;
    /**
     * @brief   : 下发空域/时域降噪配置。
     * @param    {const ISP::DnrAttr_S&} stConfig：已校验降噪参数
     * @return   {int} OK：成功，非OK：底层失败
     */
    int apply_nr(const ISP::DnrAttr_S &stConfig) override;
    /**
     * @brief   : 下发镜像翻转配置。
     * @param    {const ISP::VideoAdjust_S&} stConfig：已校验镜像参数
     * @return   {int} OK：成功，非OK：底层失败
     */
    int apply_mirror(const ISP::VideoAdjust_S &stConfig) override;
    /**
     * @brief   : 在场景与网页参数完成后应用 Gamma 后处理。
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：已生效内部运行场景
     * @return   {int} OK：成功，非OK：底层失败
     */
    int on_scene_applied(ISP::IspRuntimeScene_E enRuntimeScene) override;
};
