/**
 * @FilePath     : isp_business_service.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 15:01:25
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : 共享ISP业务服务声明
 */

#pragma once

#include <atomic>
#include <functional>

#include "isp_service_interface.h"
#include "isp_config_repository.h"
#include "isp_platform_adapters.h"
#include "isp_param_orchestrator.h"
#include "isp_scene_orchestrator.h"
#include "isp_runtime_arbiter.h"
#include "isp_runtime_reconciler.h"
#include "isp_scene_scheduler.h"
#include "isp_daynight_mode_controller.h"
#include "isp_fill_light_orchestrator.h"
#include "fill_light_gate_sink.h"

/**
 * @brief 共享 ISP 业务服务。
 * @note  创建并连接优先级选择、硬件执行、场景计划和日夜控制等组件。
 *        平台只需传入四个适配接口、时间要求、时钟和平台功能数据。
 */
class CIspBusinessService : public ISP::IIspBusinessService, public IFillLightGateSink
{
public:
    /**
     * @brief   : 构造共享 ISP 业务服务
     * @param    {const IspPlatformAdapters_S&} stAdapters：平台四类适配端口
     * @param    {const IspTransitionTiming_S&} stTiming：硬件切换时序约束
     * @param    {const IspSchedulerClock_S&} stSchedulerClock：场景计划时钟
     * @param    {const IspDayNightClock_S&} stDayNightClock：日夜模式时钟
     * @param    {const ISP::IspCapabilityProfile_S&} stProfile：当前机型支持的功能和参数范围
     * @return   {void}
     * @note    : 仅保存依赖并创建子服务；硬件资源在 init 中按顺序启动。
     */
    CIspBusinessService(const IspPlatformAdapters_S &stAdapters,
                        const IspTransitionTiming_S &stTiming,
                        const IspSchedulerClock_S &stSchedulerClock,
                        const IspDayNightClock_S &stDayNightClock,
                        const ISP::IspCapabilityProfile_S &stProfile);
    /**
     * @brief   : 析构共享 ISP 业务服务
     * @return   {void}
     * @note    : 析构时停止后台线程和平台资源；调用方仍应在销毁前显式调用 deinit。
     */
    ~CIspBusinessService();

    /**
     * @brief   : 初始化平台场景、硬件执行器、日夜检测和计划调度。
     * @return   {int} OK：成功，非OK：失败
     */
    int init() override;
    /**
     * @brief   : 按启动反序停止调度、检测、执行器和场景资源。
     * @return   {int} OK：成功，非OK：失败
     */
    int deinit() override;
    /**
     * @brief   : 从仓储读取并应用指定类别参数。
     * @param    {ISP::PicConfigureType_E} enType：配置类别
     * @return   {int} OK：成功，非OK：失败
     */
    int update_param(ISP::PicConfigureType_E enType) override;
    /**
     * @brief   : 将新的日夜配置交给模式控制器处理。
     * @param    {const ISP::DayNightAttr_S&} stOld：持久化前快照
     * @param    {const ISP::DayNightAttr_S&} stNew：已持久化新配置
     * @return   {int} OK：成功，非OK：失败
     */
    int update_daynight(const ISP::DayNightAttr_S &stOld, const ISP::DayNightAttr_S &stNew) override;
    /**
     * @brief   : 提交用户场景请求，并异步更新硬件状态。
     * @param    {ISP::SceneType_E} enScene：用户选定场景
     * @return   {int} OK：成功，非OK：失败
     */
    int apply_scene(ISP::SceneType_E enScene) override;
    /**
     * @brief   : 重新加载场景计划并唤醒调度线程。
     * @return   {int} OK：成功，非OK：失败
     */
    int on_schedule_changed() override;
    /**
     * @brief   : 按平台支持范围修正图像参数。
     * @param    {ISP::ImageParam_S&} stConfig：待校验配置
     * @return   {int} OK：成功，非OK：不支持或参数非法
     */
    int validate_image_param(ISP::ImageParam_S &stConfig) override;
    /**
     * @brief   : 按平台支持范围修正日夜和补光配置。
     * @param    {ISP::DayNightAttr_S&} stConfig：待校验配置
     * @return   {int} OK：成功，非OK：不支持或参数非法
     */
    int validate_daynight(ISP::DayNightAttr_S &stConfig) override;
    /**
     * @brief   : 返回平台功能和参数范围的副本。
     * @param    {ISP::IspCapabilityProfile_S&} stProfile：平台功能和参数范围输出
     * @return   {int} OK：成功
     */
    int get_capability_profile(ISP::IspCapabilityProfile_S &stProfile) const override;
    /**
     * @brief   : 将已持久化的 variant 配置分派到对应运行域。
     * @param    {const ISP::IspConfigValue_T&} stConfig：配置值
     * @return   {int} OK：成功，非OK：失败
     */
    int apply_config(const ISP::IspConfigValue_T &stConfig) override;
    /**
     * @brief   : 重新读取保存的配置并更新全部硬件设置。
     * @return   {int} OK：成功，非OK：失败
     */
    int reconcile_all() override;

    /**
     * @brief   : 申请临时灯光优先权。
     * @param    {const ISP::IspLightOverride_S&} stOverride：覆盖目标和截止时间
     * @param    {uint64_t&} u64Token：输出令牌
     * @return   {int} OK：成功，非OK：失败
     */
    int begin_light_override(const ISP::IspLightOverride_S &stOverride, uint64_t &u64Token) override;
    /**
     * @brief   : 使用令牌释放临时灯光优先权。
     * @param    {uint64_t} u64Token：申请时返回的令牌
     * @return   {int} OK：成功，非OK：令牌失效
     */
    int end_light_override(uint64_t u64Token) override;

    /**
     * @brief   : 接收外设补光一级总控状态并同步执行受影响的灯光目标
     * @param    {const Peripheral_NS::FillLightGateState_S&} stGate：准入与物理功率上限
     * @return   {int} OK：目标已执行或无需执行，非OK：硬件执行失败或超时
     */
    int update_fill_light_gate(const Peripheral_NS::FillLightGateState_S &stGate) override;

private:
    /**
     * @brief   : 应用用户选择并同步实际生效配置场景的日夜策略
     * @param    {ISP::SceneType_E} enScene：用户选择的网页配置场景
     * @return   {int} OK：成功，非OK：读取或日夜策略更新失败
     */
    int apply_user_config_scene(ISP::SceneType_E enScene);

    /**
     * @brief   : 应用或清除计划场景并同步实际生效配置场景的日夜策略
     * @param    {ISP::SceneType_E} enScene：计划命中的网页配置场景
     * @param    {bool} bActive：计划是否活跃
     * @return   {int} OK：成功，非OK：读取或日夜策略更新失败
     */
    int apply_schedule_config_scene(ISP::SceneType_E enScene, bool bActive);

    /**
     * @brief   : 配置场景变化后切换对应日夜配置，失败时恢复原请求
     * @param    {const ISP::IspRuntimeTarget_S&} stOldTarget：变化前目标
     * @param    {const std::function<void()>&} fnRollbackIntent：恢复请求的动作
     * @return   {int} OK：成功，非OK：读取或日夜配置更新失败
     */
    int apply_config_scene_transition(const ISP::IspRuntimeTarget_S &stOldTarget, const std::function<void()> &fnRollbackIntent);

    /**
     * @brief   : 按初始化逆序恢复日夜控制器、硬件执行器和场景资源
     * @return   {void}
     * @note    : 回滚错误仅记录，不覆盖触发回滚的原始初始化错误码。
     */
    void rollback_init();

    /**
     * @brief   : 将按优先级选出的最新设置交给单线程执行器
     * @return   {void}
     * @note    : 只提交更新序号（generation），不在业务调用线程直接操作平台硬件。
     */
    void submit_to_reconciler();

    /* 平台功能和参数范围副本 */
    ISP::IspCapabilityProfile_S m_stProfile;
    /* 时序配置 */
    IspTransitionTiming_S m_stTiming;
    /* ISP配置仓储 */
    CIspConfigRepository m_stIspRepository;
    /* 参数应用服务 */
    CIspParamOrchestrator m_stParamOrchestrator;
    /* 场景应用服务 */
    CIspSceneOrchestrator m_stSceneOrchestrator;
    /* 设置优先级选择器 */
    CIspRuntimeArbiter m_stArbiter;
    /* 单线程硬件执行器 */
    CIspRuntimeReconciler m_stReconciler;
    /* 日夜模式控制器 */
    CIspDayNightModeController m_stModeController;
    /* 场景计划调度器 */
    CIspSceneScheduler m_stScheduler;
    /* 初始化状态 */
    bool m_bInitialized;
    /* gate sink是否已注册，用于严格按逆序注销。 */
    bool m_bGateSinkRegistered;
    /* reconciler是否可接受同步gate请求。 */
    std::atomic<bool> m_bReconcilerStarted;
};
