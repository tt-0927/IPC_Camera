/**
 * @FilePath     : isp_runtime_reconciler.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:43:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : ISP单线程硬件执行器实现
 */

#include "isp_runtime_reconciler.h"

#include <system_error>

#include "IpcRet.h"
#include "dlog.h"

namespace
{
/**
 * @brief   : 判断两个完整灯光目标是否相同
 * @param    {const ISP::IspLightTarget_S&} stLeft：左侧目标
 * @param    {const ISP::IspLightTarget_S&} stRight：右侧目标
 * @return   {bool} true：相同，false：不同
 */
bool light_target_equal(const ISP::IspLightTarget_S &stLeft, const ISP::IspLightTarget_S &stRight)
{
    return stLeft.enLightType == stRight.enLightType && stLeft.nLightLevel == stRight.nLightLevel &&
           stLeft.bFlashing == stRight.bFlashing && stLeft.nFlashTimeSec == stRight.nFlashTimeSec &&
           stLeft.enFlashFrequency == stRight.enFlashFrequency;
}
} // namespace

CIspRuntimeReconciler::CIspRuntimeReconciler(IIspSceneProvider &stSceneProvider,
                                             CIspParamOrchestrator &stParamOrchestrator,
                                             IIspPeripheralController &stPeripheral,
                                             const IspTransitionTiming_S &stTiming)
    : m_rstSceneProvider(stSceneProvider), m_rstParamOrchestrator(stParamOrchestrator), m_rstPeripheral(stPeripheral),
      m_stTiming(stTiming), m_bRunning(false), m_u64LastCompletedGeneration(0), m_bForceConfigReplayPending(false),
      m_bFullReconcileCompleted(false), m_enLastAppliedConfigScene(ISP::SCENE_MAX), m_bHasAppliedRuntimeScene(false),
      m_enLastAppliedRuntimeScene(ISP::IspRuntimeScene_E::DAY), m_bHasAppliedIrCut(false),
      m_enLastAppliedIrCutTarget(ISP::IspIrCutTarget_E::NONE), m_bHasAppliedLight(false)
{
}

CIspRuntimeReconciler::~CIspRuntimeReconciler()
{
    stop();
}

int CIspRuntimeReconciler::start()
{
    if (m_bRunning)
    {
        return OK;
    }
    /* 先置运行标志再创建线程，保证 worker 首次检查谓词时不会立即退出。 */
    m_bRunning = true;
    try
    {
        m_stWorkerThread = std::thread(&CIspRuntimeReconciler::worker_loop, this);
    }
    catch (const std::system_error &stError)
    {
        m_bRunning = false;
        dlog_error("reconciler线程启动失败, code:%d, message:%s", stError.code().value(), stError.what());
        return ERR;
    }
    return OK;
}

int CIspRuntimeReconciler::stop()
{
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        m_bRunning = false;
    }
    m_stCv.notify_all();
    if (m_stWorkerThread.joinable())
    {
        m_stWorkerThread.join();
    }
    return OK;
}

void CIspRuntimeReconciler::request_reconcile(const ISP::IspRuntimeTarget_S &stTarget)
{
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        /* 只保留最新更新序号（generation）；连续配置变化由工作线程合并，不逐个操作硬件。 */
        if (stTarget.u64Generation >= m_stPendingTarget.u64Generation)
        {
            m_stPendingTarget = stTarget;
        }
    }
    m_stCv.notify_all();
}

int CIspRuntimeReconciler::reconcile_now(const ISP::IspRuntimeTarget_S &stTarget)
{
    request_reconcile(stTarget);
    return wait_for_generation(stTarget.u64Generation, m_stTiming.nReconcileWaitTimeoutMs);
}

int CIspRuntimeReconciler::wait_for_generation(uint64_t u64Generation, int64_t nTimeoutMs)
{
    std::unique_lock<std::mutex> stLock(m_mtx);
    if (m_stCv.wait_for(stLock,
                        std::chrono::milliseconds(nTimeoutMs),
                        [&]
                        {
                            return m_u64LastCompletedGeneration >= u64Generation;
                        }))
    {
        return m_stProgress.nLastErrorCode;
    }
    return ERR;
}

ISP::IspTransitionProgress_S CIspRuntimeReconciler::get_progress() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stProgress;
}

void CIspRuntimeReconciler::request_full_reconcile(const ISP::IspRuntimeTarget_S &stTarget)
{
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        /* ! 强制标志由worker一次性取走，旧目标完成时不会覆盖并发提交的新请求。 */
        m_bForceConfigReplayPending = true;
        m_bFullReconcileCompleted = false;
        if (stTarget.u64Generation >= m_stPendingTarget.u64Generation)
        {
            m_stPendingTarget = stTarget;
        }
    }
    m_stCv.notify_all();
}

int CIspRuntimeReconciler::wait_for_full_reconcile(int64_t nTimeoutMs)
{
    std::unique_lock<std::mutex> stLock(m_mtx);
    if (m_stCv.wait_for(stLock,
                        std::chrono::milliseconds(nTimeoutMs),
                        [&]
                        {
                            return m_bFullReconcileCompleted;
                        }))
    {
        return m_stProgress.nLastErrorCode;
    }
    return ERR;
}

void CIspRuntimeReconciler::worker_loop()
{
    std::unique_lock<std::mutex> stLock(m_mtx);
    while (m_bRunning)
    {
        /* 等待目标或停止信号 */
        m_stCv.wait(stLock,
                    [&]
                    {
                        return !m_bRunning || m_bForceConfigReplayPending ||
                               m_stPendingTarget.u64Generation > m_u64LastCompletedGeneration;
                    });
        if (!m_bRunning)
        {
            break;
        }

        /* 取最新目标（合并） */
        /* 复制 pending 目标后解锁；adapter 可能回调业务层，持锁会形成锁反转。 */
        ISP::IspRuntimeTarget_S stTarget = m_stPendingTarget;
        const bool bForceConfigReplay = m_bForceConfigReplayPending;
        m_bForceConfigReplayPending = false;
        stLock.unlock();

        /* ! 执行不持锁，避免平台adapter回调死锁。 */
        const TargetExecutionStatus_E enExecutionStatus = execute_target(stTarget, bForceConfigReplay);

        stLock.lock();
        if (enExecutionStatus == TargetExecutionStatus_E::SUPERSEDED)
        {
            /* 强制重放若被新目标抢占，必须转移到最新目标，不能提前唤醒等待方。 */
            if (bForceConfigReplay)
            {
                m_bForceConfigReplayPending = true;
                m_bFullReconcileCompleted = false;
            }
            continue;
        }

        m_u64LastCompletedGeneration = stTarget.u64Generation;
        m_stProgress.u64Generation = stTarget.u64Generation;
        if (bForceConfigReplay && !m_bForceConfigReplayPending)
        {
            /* 无更新全量请求排队时，本次执行结果即为等待方所需结果。 */
            m_bFullReconcileCompleted = true;
        }
        m_stCv.notify_all();

        /* 在最小切换间隔内合并设置，只保留最新更新序号（generation）。 */
        if (m_bRunning && m_stTiming.nMinIrCutSwitchIntervalMs > 0)
        {
            m_stCv.wait_for(stLock,
                            std::chrono::milliseconds(m_stTiming.nMinIrCutSwitchIntervalMs),
                            [&]
                            {
                                return !m_bRunning || m_bForceConfigReplayPending ||
                                       m_stPendingTarget.u64Generation > m_u64LastCompletedGeneration;
                            });
        }
    }
}

bool CIspRuntimeReconciler::is_target_superseded(uint64_t u64Generation) const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return !m_bRunning || m_stPendingTarget.u64Generation > u64Generation;
}

bool CIspRuntimeReconciler::wait_peripheral_retry(uint64_t u64Generation)
{
    std::unique_lock<std::mutex> stLock(m_mtx);
    const int64_t nRetryIntervalMs = m_stTiming.nPeripheralRetryIntervalMs > 0 ? m_stTiming.nPeripheralRetryIntervalMs : 0;
    return m_stCv.wait_for(stLock,
                           std::chrono::milliseconds(nRetryIntervalMs),
                           [&]
                           {
                               return !m_bRunning || m_stPendingTarget.u64Generation > u64Generation;
                           });
}

CIspRuntimeReconciler::PeripheralApplyStatus_E
CIspRuntimeReconciler::apply_ircut_with_retry(const ISP::IspRuntimeTarget_S &stTarget, bool &bSwitched, int &nRet)
{
    bSwitched = false;
    nRet = OK;
    if (stTarget.enIrCutTarget == ISP::IspIrCutTarget_E::NONE ||
        (m_bHasAppliedIrCut && stTarget.enIrCutTarget == m_enLastAppliedIrCutTarget))
    {
        return PeripheralApplyStatus_E::SUCCESS;
    }

    const int nMaxAttempts = m_stTiming.nPeripheralMaxAttempts > 0 ? m_stTiming.nPeripheralMaxAttempts : 1;
    for (int nAttempt = 1; nAttempt <= nMaxAttempts; ++nAttempt)
    {
        if (is_target_superseded(stTarget.u64Generation))
        {
            return PeripheralApplyStatus_E::SUPERSEDED;
        }

        nRet = m_rstPeripheral.switch_ircut(stTarget.enIrCutTarget);
        if (nRet == OK)
        {
            m_enLastAppliedIrCutTarget = stTarget.enIrCutTarget;
            m_bHasAppliedIrCut = true;
            bSwitched = true;
            return PeripheralApplyStatus_E::SUCCESS;
        }

        if (nAttempt >= nMaxAttempts)
        {
            dlog_error("reconciler切换IR-CUT达到最大尝试次数, target:%d, attempts:%d, ret:%d",
                       static_cast<int>(stTarget.enIrCutTarget),
                       nMaxAttempts,
                       nRet);
            return PeripheralApplyStatus_E::FAILED;
        }

        dlog_warn("reconciler切换IR-CUT失败将重试, target:%d, attempt:%d/%d, ret:%d",
                  static_cast<int>(stTarget.enIrCutTarget),
                  nAttempt,
                  nMaxAttempts,
                  nRet);
        if (wait_peripheral_retry(stTarget.u64Generation))
        {
            return PeripheralApplyStatus_E::SUPERSEDED;
        }
    }

    return PeripheralApplyStatus_E::FAILED;
}

CIspRuntimeReconciler::PeripheralApplyStatus_E
CIspRuntimeReconciler::apply_light_with_retry(const ISP::IspRuntimeTarget_S &stTarget, bool &bApplied, int &nRet)
{
    bApplied = false;
    nRet = OK;
    if (m_bHasAppliedLight && light_target_equal(stTarget.stLight, m_stLastAppliedLightTarget))
    {
        return PeripheralApplyStatus_E::SUCCESS;
    }

    const int nMaxAttempts = m_stTiming.nPeripheralMaxAttempts > 0 ? m_stTiming.nPeripheralMaxAttempts : 1;
    for (int nAttempt = 1; nAttempt <= nMaxAttempts; ++nAttempt)
    {
        if (is_target_superseded(stTarget.u64Generation))
        {
            return PeripheralApplyStatus_E::SUPERSEDED;
        }

        nRet = m_rstPeripheral.apply_light_target(stTarget.stLight);
        if (nRet == OK)
        {
            m_stLastAppliedLightTarget = stTarget.stLight;
            m_bHasAppliedLight = true;
            bApplied = true;
            return PeripheralApplyStatus_E::SUCCESS;
        }

        if (nAttempt >= nMaxAttempts)
        {
            dlog_error("reconciler应用灯光达到最大尝试次数, type:%d, level:%u, attempts:%d, ret:%d",
                       static_cast<int>(stTarget.stLight.enLightType),
                       stTarget.stLight.nLightLevel,
                       nMaxAttempts,
                       nRet);
            return PeripheralApplyStatus_E::FAILED;
        }

        dlog_warn("reconciler应用灯光失败将重试, type:%d, level:%u, attempt:%d/%d, ret:%d",
                  static_cast<int>(stTarget.stLight.enLightType),
                  stTarget.stLight.nLightLevel,
                  nAttempt,
                  nMaxAttempts,
                  nRet);
        if (wait_peripheral_retry(stTarget.u64Generation))
        {
            return PeripheralApplyStatus_E::SUPERSEDED;
        }
    }

    return PeripheralApplyStatus_E::FAILED;
}

void CIspRuntimeReconciler::record_execution_error(int nRet, bool bRuntimeSceneApplied, bool bIrCutSwitched, bool bLightApplied)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stProgress.nLastErrorCode = nRet;
    m_stProgress.bRuntimeSceneApplied = bRuntimeSceneApplied;
    m_stProgress.bIrCutSwitched = bIrCutSwitched;
    m_stProgress.bLightApplied = bLightApplied;
}

CIspRuntimeReconciler::TargetExecutionStatus_E CIspRuntimeReconciler::execute_target(const ISP::IspRuntimeTarget_S &stTarget,
                                                                                     bool bForceConfigReplay)
{
    int nRet = OK;
    bool bRuntimeSceneApplied = false;
    bool bParamsReplayed = false;
    bool bIrCutSwitched = false;
    bool bLightApplied = false;
    const bool bNeedConfigReplay = bForceConfigReplay || stTarget.enConfigScene != m_enLastAppliedConfigScene;
    const bool bNeedRuntimeSceneApply = !m_bHasAppliedRuntimeScene || stTarget.enRuntimeScene != m_enLastAppliedRuntimeScene;

    if (is_target_superseded(stTarget.u64Generation))
    {
        return TargetExecutionStatus_E::SUPERSEDED;
    }

    /* step1: 日夜场景变化时先关闭冲突灯光，再切换平台调参场景。 */
    if (bNeedRuntimeSceneApply)
    {
        if (stTarget.enIrCutTarget == ISP::IspIrCutTarget_E::NIGHT)
        {
            /* ! 单独关灯会改变硬件状态，最后成功的灯光记录必须失效并在末尾重建。 */
            m_bHasAppliedLight = false;
            nRet = m_rstPeripheral.turn_off_light(ISP::LIGHT_TYPE_WHITE);
            if (nRet != OK)
            {
                dlog_error("reconciler切换夜间前关闭白光失败: %d", nRet);
                record_execution_error(nRet, bRuntimeSceneApplied, bIrCutSwitched, bLightApplied);
                return TargetExecutionStatus_E::COMPLETED;
            }
        }
        else if (stTarget.enIrCutTarget == ISP::IspIrCutTarget_E::DAY)
        {
            m_bHasAppliedLight = false;
            nRet = m_rstPeripheral.turn_off_light(ISP::LIGHT_TYPE_RED);
            if (nRet != OK)
            {
                dlog_error("reconciler切换白天前关闭红外光失败: %d", nRet);
                record_execution_error(nRet, bRuntimeSceneApplied, bIrCutSwitched, bLightApplied);
                return TargetExecutionStatus_E::COMPLETED;
            }
        }

        if (m_stTiming.nLightOffSettleMs > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_stTiming.nLightOffSettleMs));
        }
        if (is_target_superseded(stTarget.u64Generation))
        {
            return TargetExecutionStatus_E::SUPERSEDED;
        }

        nRet = m_rstSceneProvider.apply_scene(stTarget.enRuntimeScene);
        if (nRet != OK)
        {
            dlog_error("reconciler应用运行场景失败, runtime_scene:%d, ret:%d", static_cast<int>(stTarget.enRuntimeScene), nRet);
            record_execution_error(nRet, bRuntimeSceneApplied, bIrCutSwitched, bLightApplied);
            return TargetExecutionStatus_E::COMPLETED;
        }
        bRuntimeSceneApplied = true;
    }

    /* step2: 调参场景会覆盖网页参数；配置场景变化时也必须再次应用网页参数。 */
    if (bNeedRuntimeSceneApply || bNeedConfigReplay)
    {
        nRet = m_rstParamOrchestrator.replay_web_params(stTarget.enConfigScene, stTarget.enRuntimeScene);
        if (nRet != OK)
        {
            dlog_error("reconciler重放参数失败: %d", nRet);
            record_execution_error(nRet, bRuntimeSceneApplied, bIrCutSwitched, bLightApplied);
            return TargetExecutionStatus_E::COMPLETED;
        }
        m_enLastAppliedConfigScene = stTarget.enConfigScene;
        m_enLastAppliedRuntimeScene = stTarget.enRuntimeScene;
        m_bHasAppliedRuntimeScene = true;
        bParamsReplayed = true;

        if (bNeedRuntimeSceneApply && m_stTiming.nSceneSettleBeforeIrCutMs > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_stTiming.nSceneSettleBeforeIrCutMs));
        }
    }

    if (is_target_superseded(stTarget.u64Generation))
    {
        return TargetExecutionStatus_E::SUPERSEDED;
    }

    /* step3: IR-CUT 和灯光依据工作线程最后成功状态执行，不使用选择器中间设置的变化标记。 */
    PeripheralApplyStatus_E enApplyStatus = apply_ircut_with_retry(stTarget, bIrCutSwitched, nRet);
    if (enApplyStatus == PeripheralApplyStatus_E::SUPERSEDED)
    {
        return TargetExecutionStatus_E::SUPERSEDED;
    }
    if (enApplyStatus == PeripheralApplyStatus_E::FAILED)
    {
        record_execution_error(nRet, bRuntimeSceneApplied, bIrCutSwitched, bLightApplied);
        return TargetExecutionStatus_E::COMPLETED;
    }

    enApplyStatus = apply_light_with_retry(stTarget, bLightApplied, nRet);
    if (enApplyStatus == PeripheralApplyStatus_E::SUPERSEDED)
    {
        return TargetExecutionStatus_E::SUPERSEDED;
    }
    if (enApplyStatus == PeripheralApplyStatus_E::FAILED)
    {
        record_execution_error(nRet, bRuntimeSceneApplied, bIrCutSwitched, bLightApplied);
        return TargetExecutionStatus_E::COMPLETED;
    }

    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stProgress.nLastErrorCode = OK;
    m_stProgress.bRuntimeSceneApplied = bRuntimeSceneApplied;
    m_stProgress.bIrCutSwitched = bIrCutSwitched;
    m_stProgress.bLightApplied = bLightApplied;
    if (bRuntimeSceneApplied || bParamsReplayed || bIrCutSwitched || bLightApplied)
    {
        dlog_info("ISP运行态目标执行完成, generation:%llu, config_scene:%d, runtime_scene:%d, "
                  "ircut:%d, light:%d, runtime_scene_applied:%d, params_replayed:%d, "
                  "ircut_switched:%d, light_applied:%d",
                  static_cast<unsigned long long>(stTarget.u64Generation),
                  static_cast<int>(stTarget.enConfigScene),
                  static_cast<int>(stTarget.enRuntimeScene),
                  static_cast<int>(stTarget.enIrCutTarget),
                  static_cast<int>(stTarget.stLight.enLightType),
                  bRuntimeSceneApplied,
                  bParamsReplayed,
                  bIrCutSwitched,
                  bLightApplied);
    }
    return TargetExecutionStatus_E::COMPLETED;
}
