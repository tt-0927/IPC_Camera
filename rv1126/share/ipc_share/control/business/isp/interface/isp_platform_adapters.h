/**
 * @FilePath     : isp_platform_adapters.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:05:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : ISP四个平台适配接口契约，平台只实现这四个端口
 */

#pragma once

#include <functional>

#include "isp_define.h"
#include "isp_runtime_decision.h"
#include "isp_light_target.h"

/**
 * @brief ISP平台参数适配端口。
 * @note  平台实现六类基础参数下发和场景后处理钩子。
 */
class IIspParameterApplier
{
public:
    virtual ~IIspParameterApplier() = default;

    /**
     * @brief   : 在参数重放前设置本次明确的内部运行场景
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：待重放目标场景
     * @return   {int} OK：成功，非OK：场景非法
     * @note    : 只更新参数映射上下文，不操作场景、IR-CUT或补光硬件。
     */
    virtual int set_runtime_scene_context(ISP::IspRuntimeScene_E enRuntimeScene) = 0;

    /**
     * @brief   : 应用图像基础参数
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_image(const ISP::ImageParam_S &stConfig) = 0;

    /**
     * @brief   : 应用曝光参数
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_exposure(const ISP::ExposureAttr_S &stConfig) = 0;

    /**
     * @brief   : 应用背光参数
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_backlight(const ISP::BackLightArrt_S &stConfig) = 0;

    /**
     * @brief   : 应用白平衡参数
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_awb(const ISP::AwbAttr_S &stConfig) = 0;

    /**
     * @brief   : 应用降噪参数
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_nr(const ISP::DnrAttr_S &stConfig) = 0;

    /**
     * @brief   : 应用镜像参数
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_mirror(const ISP::VideoAdjust_S &stConfig) = 0;

    /**
     * @brief   : 场景应用后平台后处理钩子（如Gamma）
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：已应用的内部运行场景
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int on_scene_applied(ISP::IspRuntimeScene_E enRuntimeScene) = 0;
};

/**
 * @brief ISP平台场景适配端口。
 * @note  平台实现场景资源init/deinit和场景切换。
 */
class IIspSceneProvider
{
public:
    virtual ~IIspSceneProvider() = default;

    /**
     * @brief   : 初始化场景资源
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int init() = 0;

    /**
     * @brief   : 释放场景资源
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int deinit() = 0;

    /**
     * @brief   : 应用指定场景
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：目标内部运行场景
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_scene(ISP::IspRuntimeScene_E enRuntimeScene) = 0;
};

/**
 * @brief ISP平台日夜检测适配端口。
 * @note  只提供自动环境光/SmartIR观测，不处理mode/time/filter。
 */
class IIspDayNightDetector
{
public:
    /** 自动观测回调类型，参数为建议是否夜间 */
    using ObservationCallback = std::function<void(bool bSuggestedNight)>;

    virtual ~IIspDayNightDetector() = default;

    /**
     * @brief   : 同步共享层最后成功日夜运行态，供detector更新观测基线
     * @param    {const ISP::IspDayNightObservationContext_S&} stContext：运行场景、请求和实际灯型
     * @return   {int} OK：成功，非OK：失败
     * @note    : 只同步观测状态，不得在此接口中操作场景、IR-CUT或灯光硬件。
     */
    virtual int sync_runtime_context(const ISP::IspDayNightObservationContext_S &stContext) = 0;

    /**
     * @brief   : 设置检测灵敏度
     * @param    {unsigned int} nLevel：灵敏度等级
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int set_sensitivity(unsigned int nLevel) = 0;

    /**
     * @brief   : 启动自动观测并注册回调
     * @param    {const ObservationCallback&} stCallback：观测回调
     * @return   {int} OK：成功，非OK：失败
     * @note    : 恢复采样时必须保留sync_runtime_context写入的已接受状态，以及stop前尚未失效的连续候选。
     */
    virtual int start(const ObservationCallback &stCallback) = 0;

    /**
     * @brief   : 停止自动观测
     * @return   {int} OK：成功，非OK：失败
     * @note    : 仅暂停采样和回调，不得自行重置已接受日夜状态或连续候选；候选失效由共享控制器同步新状态。
     */
    virtual int stop() = 0;
};

/**
 * @brief ISP平台外设控制适配端口。
 * @note  包含关灯、IR-CUT切换和灯光目标执行。
 */
class IIspPeripheralController
{
public:
    virtual ~IIspPeripheralController() = default;

    /**
     * @brief   : 关闭指定类型灯光
     * @param    {ISP::LightType_E} enLightType：灯光类型
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int turn_off_light(ISP::LightType_E enLightType) = 0;

    /**
     * @brief   : 切换IR-CUT
     * @param    {ISP::IspIrCutTarget_E} enTarget：IR-CUT目标
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int switch_ircut(ISP::IspIrCutTarget_E enTarget) = 0;

    /**
     * @brief   : 应用灯光目标
     * @param    {const ISP::IspLightTarget_S&} stTarget：灯光目标
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_light_target(const ISP::IspLightTarget_S &stTarget) = 0;
};

/**
 * @brief 四端口bundle，一次性构造注入共享service。
 */
struct IspPlatformAdapters_S
{
    IIspParameterApplier &stParameter;
    IIspSceneProvider &stScene;
    IIspDayNightDetector &stDetector;
    IIspPeripheralController &stPeripheral;
};
