/**
 * @FilePath     : fill_light_gate_controller.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 15:34:05
 * @Description  : 外设补光一级总控配置快照、定时检查与安全通知控制器声明
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include "fill_light_gate_sink.h"
#include "fill_light_gate_state.h"
#include "peripheral_fill_light_config.h"

/**
 * @brief 外设补光一级总控状态控制器。
 * @note  一个worker每秒复核本地时间，sink回调始终在状态锁外执行。
 */
class CFillLightGateController
{
public:
    /**
     * @brief   : 构造默认未初始化且禁止输出的gate控制器
     * @return   {void}
     */
    CFillLightGateController();
    /**
     * @brief   : 停止worker并清理非拥有sink
     * @return   {void}
     */
    ~CFillLightGateController();
    CFillLightGateController(const CFillLightGateController &) = delete;
    CFillLightGateController &operator=(const CFillLightGateController &) = delete;

    /**
     * @brief   : 加载配置快照并启动定时准入检查
     * @param    {const Peripheral_NS::FillLightGlobalConfig_S&} stConfig：初始配置
     * @return   {int} OK：成功，ERR：线程启动失败
     */
    int init(const Peripheral_NS::FillLightGlobalConfig_S &stConfig);

    /**
     * @brief   : 停止定时检查并清除sink
     * @return   {int} OK：成功
     */
    int deinit();

    /**
     * @brief   : 更新配置并立即通知变化后的gate
     * @param    {const Peripheral_NS::FillLightGlobalConfig_S&} stConfig：新配置
     * @return   {int} OK：成功，ERR_UNINIT：未初始化，其他：sink应用失败
     */
    int update_config(const Peripheral_NS::FillLightGlobalConfig_S &stConfig);

    /**
     * @brief   : 在系统校时或时区变化后立即重新计算补光gate
     * @param    {const char*} pszTrigger：触发来源，仅用于诊断日志
     * @return   {int} OK：无变化或同步成功，ERR_UNINIT：未初始化，其他：sink应用失败
     * @note    : 该入口与周期worker复用同一版本和通知机制，禁止直接操作PWM。
     */
    int refresh_after_time_change(const char *pszTrigger);

    /**
     * @brief   : 注册gate消费者并在已初始化时立即重放当前状态
     * @param    {IFillLightGateSink*} pSink：调用方持有的非拥有指针
     * @return   {int} OK：成功，ERR_PARAM_NULL：空指针，ERR：重复注册，其他：重放失败
     */
    int set_sink(IFillLightGateSink *pSink);

    /**
     * @brief   : 清除gate消费者并等待正在进行的回调结束
     * @param    {IFillLightGateSink*} pSink：待清除的非拥有指针
     * @return   {int} OK：成功，ERR_PARAM_NULL：空指针，ERR_PARAM：指针不匹配
     */
    int clear_sink(IFillLightGateSink *pSink);

    /**
     * @brief   : 获取当前gate快照
     * @param    {Peripheral_NS::FillLightGateState_S&} stGate：gate输出
     * @return   {int} OK：成功，ERR_UNINIT：未初始化
     */
    int get_gate(Peripheral_NS::FillLightGateState_S &stGate) const;

private:
    /**
     * @brief   : 定时复核本地时间并发布gate变化
     * @return   {void}
     */
    void worker_loop();

    /**
     * @brief   : 使用当前本地时间重新计算gate
     * @return   {int} OK：无变化或发布成功，其他：sink应用失败
     */
    int refresh_gate(const char *pszTrigger, bool bLogUnchanged);

    /**
     * @brief   : 仅在版本仍为最新时通知sink
     * @param    {std::uint64_t} u64ExpectedVersion：待通知gate版本
     * @return   {int} OK：成功、无sink或版本过期，其他：sink应用失败
     */
    int notify_gate(std::uint64_t u64ExpectedVersion);

    /* lock: 保护配置、gate、版本、worker状态和sink指针。 */
    mutable std::mutex m_mtxState;
    /* lock: 串行sink回调，并作为clear_sink的回调完成屏障。 */
    std::mutex m_mtxCallback;
    /* worker等待与配置更新唤醒条件。 */
    std::condition_variable m_stWakeCv;
    /* memory: controller独占worker线程并在deinit中join。 */
    std::thread m_stWorker;
    /* 当前强类型配置快照。 */
    Peripheral_NS::FillLightGlobalConfig_S m_stConfig;
    /* 当前gate快照。 */
    Peripheral_NS::FillLightGateState_S m_stGate;
    /* memory: sink由注册方持有，clear_sink返回后可安全析构。 */
    IFillLightGateSink *m_pstSink;
    /* 是否已初始化。 */
    bool m_bInitialized;
    /* worker停止标志。 */
    bool m_bStopRequested;
    /* gate变化版本，用于丢弃排队中的过期通知。 */
    std::uint64_t m_u64GateVersion;
    /* 最近一次sink确认成功的gate版本，硬件失败时worker会持续重试当前版本。 */
    std::uint64_t m_u64NotifiedGateVersion;
    /* worker最近一次通知错误，用于抑制每秒重复日志。 */
    int m_nLastNotifyError;
};
