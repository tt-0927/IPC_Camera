/**
 * @FilePath     : isp_runtime_reconciler.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:43:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 09:46:56
 * @Description  : ISP硬件设置串行执行器
 */

#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include "isp_platform_adapters.h"
#include "isp_runtime_target.h"
#include "isp_transition_progress.h"
#include "isp_param_orchestrator.h"
#include "isp_fill_light_orchestrator.h"

/**
 * @brief ISP硬件设置串行执行器。
 * @note  按“关灯、切场景、重放参数、切 IR-CUT、开灯”的顺序执行。短时间内有多次更新时，只执行最后一次。
 *        调用平台接口前必须解锁，避免回调死锁。
 */
class CIspRuntimeReconciler
{
public:
    /**
     * @brief   : 构造硬件设置执行器
     * @param    {IIspSceneProvider&} stSceneProvider：场景端口
     * @param    {CIspParamOrchestrator&} stParamOrchestrator：参数应用对象
     * @param    {IIspPeripheralController&} stPeripheral：外设端口
     * @param    {const IspTransitionTiming_S&} stTiming：时序配置
     */
    CIspRuntimeReconciler(IIspSceneProvider &stSceneProvider,
                          CIspParamOrchestrator &stParamOrchestrator,
                          IIspPeripheralController &stPeripheral,
                          const IspTransitionTiming_S &stTiming);
    ~CIspRuntimeReconciler();

    /**
     * @brief   : 启动执行线程
     * @return   {int} OK：成功
     */
    int start();

    /**
     * @brief   : 停止并等待执行线程退出
     * @return   {int} OK：成功
     */
    int stop();

    /**
     * @brief   : 异步提交待应用设置
     * @param    {const ISP::IspRuntimeTarget_S&} stTarget：待应用设置
     */
    void request_reconcile(const ISP::IspRuntimeTarget_S &stTarget);

    /**
     * @brief   : 同步应用设置并等待结果
     * @param    {const ISP::IspRuntimeTarget_S&} stTarget：待应用设置
     * @return   {int} OK：成功，非OK：执行失败或超时
     */
    int reconcile_now(const ISP::IspRuntimeTarget_S &stTarget);

    /**
     * @brief   : 等待指定版本的设置处理结束
     * @param    {uint64_t} u64Generation：设置版本号
     * @param    {int64_t} nTimeoutMs：超时(ms)
     * @return   {int} OK：已执行，ERR：超时
     */
    int wait_for_generation(uint64_t u64Generation, int64_t nTimeoutMs);

    /**
     * @brief   : 获取当前执行进度
     * @return   {ISP::IspTransitionProgress_S} 进度
     */
    ISP::IspTransitionProgress_S get_progress() const;

    /**
     * @brief   : 请求协调目标并强制重放一次网页参数
     * @param    {const ISP::IspRuntimeTarget_S&} stTarget：待协调目标
     * @return   {void}
     * @note    : 用于恢复默认等配置场景未变但槽位内容整体变化的事务；普通场景切换不调用。
     */
    void request_full_reconcile(const ISP::IspRuntimeTarget_S &stTarget);

    /**
     * @brief   : 等待最近一次强制参数重放完成
     * @param    {int64_t} nTimeoutMs：超时毫秒数
     * @return   {int} OK：成功，其他：执行失败或超时
     */
    int wait_for_full_reconcile(int64_t nTimeoutMs);

private:
    /**
     * @brief 目标执行状态，区分已完成与被更新generation抢占。
     */
    enum class TargetExecutionStatus_E
    {
        COMPLETED,
        SUPERSEDED,
    };

    /**
     * @brief 单个外设目标的执行结果。
     */
    enum class PeripheralApplyStatus_E
    {
        SUCCESS,
        FAILED,
        SUPERSEDED,
    };

    /**
     * @brief   : 工作线程主循环
     */
    void worker_loop();

    /**
     * @brief   : 应用设置（不持锁）
     * @param    {const ISP::IspRuntimeTarget_S&} stTarget：目标
     * @param    {bool} bForceConfigReplay：是否强制重放当前配置场景参数
     * @return   {TargetExecutionStatus_E} COMPLETED：已完成（可成功或失败），SUPERSEDED：已被新目标抢占
     */
    TargetExecutionStatus_E execute_target(const ISP::IspRuntimeTarget_S &stTarget, bool bForceConfigReplay);

    /**
     * @brief   : 判断当前执行目标是否已被新generation取代
     * @param    {uint64_t} u64Generation：当前目标generation
     * @return   {bool} true：已停止或存在更新目标，false：可继续执行
     */
    bool is_target_superseded(uint64_t u64Generation) const;

    /**
     * @brief   : 等待外设重试间隔，新generation到达时提前结束
     * @param    {uint64_t} u64Generation：当前目标generation
     * @return   {bool} true：已停止或存在更新目标，false：重试间隔正常到期
     */
    bool wait_peripheral_retry(uint64_t u64Generation);

    /**
     * @brief   : 按实际成功态判断并有限重试IR-CUT目标
     * @param    {const ISP::IspRuntimeTarget_S&} stTarget：最终目标
     * @param    {bool&} bSwitched：本次是否实际切换成功
     * @param    {int&} nRet：最后一次adapter返回码
     * @return   {PeripheralApplyStatus_E} 外设执行状态
     */
    PeripheralApplyStatus_E apply_ircut_with_retry(const ISP::IspRuntimeTarget_S &stTarget, bool &bSwitched, int &nRet);

    /**
     * @brief   : 按实际成功态判断并有限重试灯光目标
     * @param    {const ISP::IspRuntimeTarget_S&} stTarget：最终目标
     * @param    {bool&} bApplied：本次是否实际应用成功
     * @param    {int&} nRet：最后一次adapter返回码
     * @return   {PeripheralApplyStatus_E} 外设执行状态
     */
    PeripheralApplyStatus_E apply_light_with_retry(const ISP::IspRuntimeTarget_S &stTarget, bool &bApplied, int &nRet);

    /**
     * @brief   : 记录当前目标执行错误
     * @param    {int} nRet：错误码
     * @param    {bool} bRuntimeSceneApplied：失败前是否已应用运行场景
     * @param    {bool} bIrCutSwitched：失败前是否已切换IR-CUT
     * @param    {bool} bLightApplied：失败前是否已应用灯光
     * @return   {void}
     */
    void record_execution_error(int nRet, bool bRuntimeSceneApplied, bool bIrCutSwitched, bool bLightApplied);

    /* 场景端口引用 */
    IIspSceneProvider &m_rstSceneProvider;
    /* 参数编排器引用 */
    CIspParamOrchestrator &m_rstParamOrchestrator;
    /* 外设端口引用 */
    IIspPeripheralController &m_rstPeripheral;
    /* 时序配置 */
    IspTransitionTiming_S m_stTiming;

    /* 工作线程 */
    std::thread m_stWorkerThread;
    /* lock: 保护pending target、progress和运行状态 */
    mutable std::mutex m_mtx;
    /* 唤醒工作线程和等待者 */
    std::condition_variable m_stCv;
    /* 是否运行中 */
    std::atomic<bool> m_bRunning;
    /* 待执行目标 */
    ISP::IspRuntimeTarget_S m_stPendingTarget;
    /* 执行进度 */
    ISP::IspTransitionProgress_S m_stProgress;
    /* 最后已处理完成generation，执行失败也会更新以唤醒同步等待方。 */
    uint64_t m_u64LastCompletedGeneration;
    /* 一次性全量参数重放请求；worker取走后清除，新请求不会被正在执行的旧目标覆盖。 */
    bool m_bForceConfigReplayPending;
    /* 最近一次全量重放是否已完成，与普通generation等待独立。 */
    bool m_bFullReconcileCompleted;
    /* worker最后实际完成参数重放的网页配置场景，SCENE_MAX表示尚未重放。 */
    ISP::SceneType_E m_enLastAppliedConfigScene;
    /* worker是否已经完成过内部运行场景应用。 */
    bool m_bHasAppliedRuntimeScene;
    /* worker最后实际完成应用的内部运行场景。 */
    ISP::IspRuntimeScene_E m_enLastAppliedRuntimeScene;
    /* worker是否已成功应用过IR-CUT目标。 */
    bool m_bHasAppliedIrCut;
    /* worker最后成功应用的IR-CUT目标。 */
    ISP::IspIrCutTarget_E m_enLastAppliedIrCutTarget;
    /* worker是否已成功应用过完整灯光目标。 */
    bool m_bHasAppliedLight;
    /* worker最后成功应用的完整灯光目标。 */
    ISP::IspLightTarget_S m_stLastAppliedLightTarget;
};
