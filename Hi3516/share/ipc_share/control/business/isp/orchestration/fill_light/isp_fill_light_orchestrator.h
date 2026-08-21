/**
 * @FilePath     : isp_fill_light_orchestrator.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:43:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-15 17:30:00
 * @Description  : ISP补光时序配置
 */

#pragma once

#include <cstdint>

/**
 * @brief ISP切换时序配置。
 * @note  由平台/机型builder提供，reconciler按此时序执行关灯→场景→IR-CUT→灯光。
 */
struct IspTransitionTiming_S
{
    /* 关灯后等待稳定时间(ms) */
    int64_t nLightOffSettleMs;
    /* 场景应用后等待IR-CUT稳定时间(ms) */
    int64_t nSceneSettleBeforeIrCutMs;
    /* 最小IR-CUT切换间隔(ms)，用于generation合并窗口 */
    int64_t nMinIrCutSwitchIntervalMs;
    /* IR-CUT和补光硬件失败后的重试间隔(ms) */
    int64_t nPeripheralRetryIntervalMs;
    /* IR-CUT和补光硬件的最大尝试次数，包含首次执行 */
    int nPeripheralMaxAttempts;
    /* reconcile同步等待超时(ms) */
    int64_t nReconcileWaitTimeoutMs;

    IspTransitionTiming_S()
        : nLightOffSettleMs(0), nSceneSettleBeforeIrCutMs(0), nMinIrCutSwitchIntervalMs(0),
          nPeripheralRetryIntervalMs(200), nPeripheralMaxAttempts(3), nReconcileWaitTimeoutMs(5000)
    {
    }
};
