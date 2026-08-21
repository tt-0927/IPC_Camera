/**
 * @FilePath     : isp_parameter_applier_rv1126b.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:39:13
 * @Description  : RV1126B ISP参数适配端口声明
 */

#pragma once

#include "isp_platform_adapters.h"

/**
 * @brief RV1126B ISP参数适配端口。
 * @note 将共享配置模型映射为既有CIspControl的RK AIQ调用；不保存业务配置，也不直接
 *       切换IQ、日夜状态或补光硬件。
 */
class CIspParameterApplierRv1126b : public IIspParameterApplier
{
public:
    /**
     * @brief   : 构造RV1126B参数适配器
     * @return   {void}
     */
    CIspParameterApplierRv1126b() = default;
    /**
     * @brief   : 销毁RV1126B参数适配器
     * @return   {void}
     */
    ~CIspParameterApplierRv1126b() override = default;

    /**
     * @brief   : 保存本次参数重放关联的运行场景
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：共享层裁决的目标运行场景
     * @return   {int} OK：支持，ERR_UNSUPPORT：RV1126B不支持的场景
     */
    int set_runtime_scene_context(ISP::IspRuntimeScene_E enRuntimeScene) override;
    /**
     * @brief   : 应用图像基础参数
     * @param    {const ISP::ImageParam_S&} stConfig：已由共享层校验的图像配置
     * @return   {int} OK：成功，非OK：RK AIQ下发失败
     */
    int apply_image(const ISP::ImageParam_S &stConfig) override;
    /**
     * @brief   : 应用曝光参数
     * @param    {const ISP::ExposureAttr_S&} stConfig：已校验的曝光配置
     * @return   {int} OK：成功，非OK：RK AIQ下发失败
     */
    int apply_exposure(const ISP::ExposureAttr_S &stConfig) override;
    /**
     * @brief   : 应用背光参数
     * @param    {const ISP::BackLightArrt_S&} stConfig：已校验的背光配置
     * @return   {int} OK：成功，非OK：RK AIQ下发失败
     */
    int apply_backlight(const ISP::BackLightArrt_S &stConfig) override;
    /**
     * @brief   : 应用白平衡参数
     * @param    {const ISP::AwbAttr_S&} stConfig：已校验的白平衡配置
     * @return   {int} OK：成功，非OK：RK AIQ下发失败
     */
    int apply_awb(const ISP::AwbAttr_S &stConfig) override;
    /**
     * @brief   : 应用降噪参数
     * @param    {const ISP::DnrAttr_S&} stConfig：已校验的降噪配置
     * @return   {int} OK：成功，非OK：RK AIQ下发失败
     */
    int apply_nr(const ISP::DnrAttr_S &stConfig) override;
    /**
     * @brief   : 应用镜像参数
     * @param    {const ISP::VideoAdjust_S&} stConfig：已校验的镜像配置
     * @return   {int} OK：成功，非OK：RK AIQ下发失败
     */
    int apply_mirror(const ISP::VideoAdjust_S &stConfig) override;
    /**
     * @brief   : 完成场景后的RV1126B专有处理
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：已应用的运行场景
     * @return   {int} OK：成功，ERR_PARAM：场景上下文不匹配
     * @note    : 本平台无独立Gamma等后处理，仍校验时序契约防止错误的场景重放。
     */
    int on_scene_applied(ISP::IspRuntimeScene_E enRuntimeScene) override;

private:
    /* 当前参数重放关联的运行场景；RV1126B参数映射不区分IQ场景，但必须保留接口语义。 */
    ISP::IspRuntimeScene_E m_enRuntimeScene{ ISP::IspRuntimeScene_E::DAY };
};
