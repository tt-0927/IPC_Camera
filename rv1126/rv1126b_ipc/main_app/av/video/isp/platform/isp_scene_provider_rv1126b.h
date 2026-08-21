/**
 * @FilePath     : isp_scene_provider_rv1126b.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:39:13
 * @Description  : RV1126B ISP场景适配端口声明
 */

#pragma once

#include "isp_platform_adapters.h"

/**
 * @brief RV1126B ISP场景适配端口。
 * @note 适配器只转发共享场景生命周期；DAY、NIGHT_WHITE和NIGHT_LIGHT_OFF均真实调用
 *       RK normal/day IQ场景，保持全彩画面，不在此处控制白光或IR-CUT。
 */
class CIspSceneProviderRv1126b : public IIspSceneProvider
{
public:
    /**
     * @brief   : 构造RV1126B场景适配器
     * @return   {void}
     */
    CIspSceneProviderRv1126b() = default;
    /**
     * @brief   : 销毁RV1126B场景适配器
     * @return   {void}
     */
    ~CIspSceneProviderRv1126b() override = default;

    /**
     * @brief   : 初始化RK场景资源管理器
     * @return   {int} OK：成功，非OK：RK AIQ尚未就绪
     */
    int init() override;
    /**
     * @brief   : 释放RK场景资源管理器
     * @return   {int} OK：成功，非OK：释放失败
     */
    int deinit() override;
    /**
     * @brief   : 应用共享运行场景
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：DAY、NIGHT_WHITE或NIGHT_LIGHT_OFF
     * @return   {int} OK：成功，非OK：场景非法或RK IQ切换失败
     * @note    : 三种场景统一映射到normal/day全彩IQ。
     */
    int apply_scene(ISP::IspRuntimeScene_E enRuntimeScene) override;
};
