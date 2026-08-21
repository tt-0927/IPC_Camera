/**
 * @FilePath     : isp_daynight_mode_controller.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 16:33:21
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : 共享ISP日夜模式控制器实现
 */

#include "isp_daynight_mode_controller.h"

#include <system_error>

#include "IpcRet.h"
#include "dlog.h"
#include "isp_daynight_policy.h"

namespace
{
/* perf: Hi3516运行库的condition_variable绝对deadline超时不可靠；200ms轮询保证滤波截止时间可被消费。 */
constexpr std::chrono::milliseconds WORKER_POLL_INTERVAL(200);
} // namespace

CIspDayNightModeController::CIspDayNightModeController(IIspDayNightDetector &stDetector,
                                                       const IspDayNightClock_S &stClock,
                                                       const ISP::IspCapabilityProfile_S &stProfile,
                                                       DayNightIntentCallback fnCallback)
    : m_rstDetector(stDetector), m_stClock(stClock), m_stProfile(stProfile), m_fnCallback(fnCallback), m_bRunning(false),
      m_u32Epoch(0), m_bInitialized(false), m_bDetectorRunning(false), m_bWorkerRunning(false), m_bLastIsNight(false),
      m_bPendingNight(false), m_bPendingSuggestedNight(false), m_stFilterStartTime(), m_stFilterDeadline()
{
}

CIspDayNightModeController::~CIspDayNightModeController()
{
    deinit();
}

int CIspDayNightModeController::init(const ISP::DayNightAttr_S &stConfig)
{
    /* lock: 生命周期锁使初始化、配置切换和去初始化不会并发操作同一工作线程。 */
    std::lock_guard<std::mutex> stLifecycleLock(m_mtxLifecycle);
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (m_bInitialized)
        {
            return OK;
        }

        m_stConfig = stConfig;
        m_bRunning = true;
        m_bInitialized = true;
        ++m_u32Epoch;
        m_bPendingNight = false;
    }

    /* 先提交当前模式的初始请求，再启动等待线程，避免首个边界被当作状态切换。 */
    int nRet = apply_current_mode();
    if (nRet != OK)
    {
        /* 初始模式建立失败时撤销生命周期状态，禁止业务服务进入半初始化。 */
        std::lock_guard<std::mutex> stLock(m_mtx);
        m_bRunning = false;
        m_bInitialized = false;
        ++m_u32Epoch;
        m_bPendingNight = false;
        return nRet;
    }
    nRet = start_worker();
    if (nRet == OK)
    {
        return OK;
    }

    const int nStopRet = stop_detector();
    if (nStopRet != OK)
    {
        dlog_warn("日夜worker启动失败后停止detector失败: %d", nStopRet);
    }
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        m_bRunning = false;
        m_bInitialized = false;
        ++m_u32Epoch;
        m_bPendingNight = false;
    }
    return nRet;
}

int CIspDayNightModeController::update_config(const ISP::DayNightAttr_S &stConfig)
{
    std::lock_guard<std::mutex> stLifecycleLock(m_mtxLifecycle);
    bool bAutoHotUpdate = false;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (!m_bInitialized)
        {
            return ERR_UNINIT;
        }
        bAutoHotUpdate = m_stConfig.enDayNightMode == ISP::AUTO_MODE && stConfig.enDayNightMode == ISP::AUTO_MODE;
    }

    if (bAutoHotUpdate)
    {
        int nRet = update_auto_config(stConfig);
        const int nWorkerRet = start_worker();
        return nRet == OK ? nWorkerRet : nRet;
    }

    {
        /* lock: 先与正在提交的旧请求串行；配置更新序号变化后，旧事件会因 epoch 不匹配被丢弃。 */
        std::lock_guard<std::mutex> stIntentSubmitLock(m_mtxIntentSubmit);
        std::lock_guard<std::mutex> stLock(m_mtx);

        /* 模式切换废弃旧TIME/Filter等待；同AUTO参数更新由热更新路径保留当前状态。 */
        ++m_u32Epoch;
        m_stConfig = stConfig;
        m_bPendingNight = false;
    }

    int nRet = apply_current_mode();
    if (nRet != OK)
    {
        return nRet;
    }

    /* update_config也确保worker仍存活，避免遗留的joinable线程对象阻断filter超时处理。 */
    return start_worker();
}

int CIspDayNightModeController::update_auto_config(const ISP::DayNightAttr_S &stConfig)
{
    /* ! 停止采样线程后再替换配置和回调，避免热更新期间产生无法归属的观测事件。 */
    int nRet = stop_detector();
    if (nRet != OK)
    {
        return nRet;
    }

    ISP::DayNightAttr_S stOldConfig;
    bool bCurrentIsNight = false;
    bool bSensitivityChanged = false;
    bool bNightRuntimeChanged = false;
    uint32_t u32CurrentEpoch = 0;
    {
        /* lock: 配置替换与旧请求提交串行，热更新期间始终保留已接受的日夜状态。 */
        std::lock_guard<std::mutex> stIntentSubmitLock(m_mtxIntentSubmit);
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (!m_bInitialized || m_stConfig.enDayNightMode != ISP::AUTO_MODE || stConfig.enDayNightMode != ISP::AUTO_MODE)
        {
            return ERR_UNINIT;
        }

        stOldConfig = m_stConfig;
        bCurrentIsNight = m_bLastIsNight;
        bSensitivityChanged = stOldConfig.nSensitivityLevel != stConfig.nSensitivityLevel;
        bNightRuntimeChanged = IspDayNightPolicy_NS::has_night_light_runtime_changed(stOldConfig, stConfig);

        if (bSensitivityChanged || (bCurrentIsNight && bNightRuntimeChanged))
        {
            /* 新阈值或已生效补光条件会改变观测结果，递增代次使worker已取出的旧候选也无法提交。 */
            ++m_u32Epoch;
            m_bPendingNight = false;
        }
        u32CurrentEpoch = m_u32Epoch;
        m_stConfig = stConfig;

        if (m_bPendingNight && stOldConfig.nFilterTime != stConfig.nFilterTime)
        {
            /* 纯过滤时间更新沿用原代次，worker已取出的到期候选可基于新配置正常提交。 */
            m_stFilterDeadline = m_stFilterStartTime + std::chrono::seconds(stConfig.nFilterTime);
        }
    }

    int nSetupRet = OK;
    if (bCurrentIsNight && bNightRuntimeChanged)
    {
        /* 夜间补光配置立即重裁决；白天不提交夜间灯型，配置留待下一次检测切夜时生效。 */
        nSetupRet = submit_intent(true, u32CurrentEpoch);
    }
    else if (bSensitivityChanged)
    {
        /* 灵敏度变化后清空底层旧阈值候选，但保持当前日夜硬件目标不变。 */
        nSetupRet = sync_detector_current_state();
    }

    /* 即使硬件设置重新同步失败也尝试恢复检测线程，避免一次配置错误永久关闭 AUTO 检测。 */
    int nStartRet = start_auto_detector(stConfig, u32CurrentEpoch);
    if (nSetupRet != OK)
    {
        return nSetupRet;
    }
    return nStartRet;
}

int CIspDayNightModeController::deinit()
{
    std::lock_guard<std::mutex> stLifecycleLock(m_mtxLifecycle);
    {
        /* lock: 等待正在提交的请求结束后再停止控制器，避免析构期间仍有回调访问业务状态。 */
        std::lock_guard<std::mutex> stIntentSubmitLock(m_mtxIntentSubmit);
        std::lock_guard<std::mutex> stLock(m_mtx);
        m_bRunning = false;
        m_bInitialized = false;
        ++m_u32Epoch;
        m_bPendingNight = false;
    }

    /* ! 先停止 detector 再等待 worker 退出，禁止对象销毁后仍有回调写入控制器成员。 */
    int nDetectorRet = stop_detector();
    stop_worker();
    return nDetectorRet;
}

int CIspDayNightModeController::apply_current_mode()
{
    /* AUTO 切换或参数更新前先撤销旧 detector，避免旧回调与新配置混用。 */
    int nRet = stop_detector();
    if (nRet != OK)
    {
        return nRet;
    }

    ISP::DayNightAttr_S stConfig;
    uint32_t u32CurrentEpoch = 0;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (!m_bRunning || !m_bInitialized)
        {
            return ERR_UNINIT;
        }
        stConfig = m_stConfig;
        u32CurrentEpoch = m_u32Epoch;
    }

    switch (stConfig.enDayNightMode)
    {
    case ISP::DAY_MODE:
        /* DAY 立即提交白天请求，不启动检测器。 */
        return submit_intent(false, u32CurrentEpoch);

    case ISP::NIGHT_MODE:
        /* NIGHT 立即提交夜间请求，不启动检测器。 */
        return submit_intent(true, u32CurrentEpoch);

    case ISP::TIME_MODE:
    {
        /* TIME 使用注入时钟和共享策略判定当前状态，边界续期由单一 worker 循环完成。 */
        int nNowSec = m_stClock.fnGetDaySeconds();
        bool bIsNight = IspDayNightPolicy_NS::is_night_by_time_range(stConfig, nNowSec);
        return submit_intent(bIsNight, u32CurrentEpoch);
    }

    case ISP::AUTO_MODE:
    {
        /* ! AUTO 启动时硬件可能仍停留在断电前的红外位置；先明确提交白天目标，
         * 防止 detector 首个“白天”观测被去重后遗留旧 IR-CUT/补光状态。
         */
        nRet = submit_intent(false, u32CurrentEpoch);
        if (nRet != OK)
        {
            return nRet;
        }

        return start_auto_detector(stConfig, u32CurrentEpoch);
    }

    default:
        dlog_error("未知日夜模式: %d", static_cast<int>(stConfig.enDayNightMode));
        return ERR_PARAM;
    }
}

int CIspDayNightModeController::start_auto_detector(const ISP::DayNightAttr_S &stConfig, uint32_t u32Epoch)
{
    /* 先下发灵敏度再注册回调，避免 detector 启动初期使用旧阈值。 */
    int nRet = m_rstDetector.set_sensitivity(stConfig.nSensitivityLevel);
    if (nRet != OK)
    {
        dlog_error("设置日夜 detector 灵敏度失败: %d", nRet);
        return nRet;
    }

    nRet = m_rstDetector.start(
        [this, u32Epoch](bool bSuggestedNight)
        {
            on_detector_observation(u32Epoch, bSuggestedNight);
        });
    if (nRet != OK)
    {
        dlog_error("启动日夜 detector 失败: %d", nRet);
        return nRet;
    }

    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (m_bRunning && m_u32Epoch == u32Epoch && m_stConfig.enDayNightMode == ISP::AUTO_MODE)
        {
            m_bDetectorRunning = true;
            dlog_info("AUTO日夜检测已启动, state:%s, sensitivity:%u, filter_time:%us",
                      m_bLastIsNight ? "夜间" : "白天",
                      stConfig.nSensitivityLevel,
                      stConfig.nFilterTime);
            return OK;
        }
    }

    /* 生命周期已变化时立即回收刚启动的 detector，不能遗留无主回调。 */
    nRet = m_rstDetector.stop();
    if (nRet != OK)
    {
        dlog_error("回收过期日夜 detector 失败: %d", nRet);
        return nRet;
    }
    return ERR;
}

int CIspDayNightModeController::sync_detector_current_state()
{
    ISP::IspDayNightObservationContext_S stContext;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        stContext = m_stLastObservationContext;
    }
    const int nRet = m_rstDetector.sync_runtime_context(stContext);
    if (nRet != OK)
    {
        dlog_error("同步日夜detector当前状态失败: %d", nRet);
    }
    return nRet;
}

int CIspDayNightModeController::submit_intent(bool bIsNight, uint32_t u32Epoch)
{
    /* lock: 持有至回调完成，使配置切换不能发生在epoch校验与arbiter提交之间。 */
    std::lock_guard<std::mutex> stIntentSubmitLock(m_mtxIntentSubmit);
    ISP::DayNightAttr_S stConfig;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (!m_bRunning || m_u32Epoch != u32Epoch)
        {
            return ERR;
        }
        stConfig = m_stConfig;
    }

    /* 使用共享策略计算能力裁决后的场景、IR-CUT 和有效灯型，避免本类复制业务规则。 */
    ISP::IspRuntimeDecision_S stDecision;
    int nRet = IspDayNightPolicy_NS::decide_runtime_scene(bIsNight, stConfig, m_stProfile, stDecision);
    if (nRet != OK)
    {
        dlog_error("日夜策略决策失败: %d", nRet);
        return nRet;
    }

    ISP::IspDayNightIntent_S stIntent;
    stIntent.bActive = true;
    stIntent.bIsNight = bIsNight;

    /* 日夜请求直接携带内部日夜场景，不能再转换为网页 SceneType_E。 */
    stIntent.enRuntimeScene = stDecision.enRuntimeScene;
    stIntent.enIrCutTarget = stDecision.enIrCutTarget;
    stIntent.stLight.enLightType = stDecision.enEffectiveLightType;
    stIntent.stLight.bFlashing = false;

    if (stDecision.enEffectiveLightType == ISP::LIGHT_TYPE_WHITE ||
        stDecision.enEffectiveLightType == ISP::LIGHT_TYPE_WHITE_ON_RED_OFF)
    {
        stIntent.stLight.nLightLevel = stConfig.stFillLight.stWhiteAttr.nLightLevel;
    }
    else if (stDecision.enEffectiveLightType == ISP::LIGHT_TYPE_RED ||
             stDecision.enEffectiveLightType == ISP::LIGHT_TYPE_RED_ON_WHITE_OFF)
    {
        stIntent.stLight.nLightLevel = stConfig.stFillLight.stRedAttr.nLightLevel;
    }

    /* 回调可能同步进入业务层，因此不持有状态锁；提交串行锁仍阻止并发配置切换。 */
    ISP::IspDayNightObservationContext_S stObservationContext;
    nRet = m_fnCallback(stIntent, stObservationContext);
    if (nRet != OK)
    {
        dlog_error("提交日夜运行态意图失败: %d", nRet);
        return nRet;
    }

    dlog_info("日夜运行态意图已提交, state:%s, runtime_scene:%d, ircut:%d, light:%d",
              bIsNight ? "夜间" : "白天",
              static_cast<int>(stIntent.enRuntimeScene),
              static_cast<int>(stIntent.enIrCutTarget),
              static_cast<int>(stIntent.stLight.enLightType));

    /* 检测器状态也被运动检测等模块读取，因此所有模式都同步最后成功的硬件状态。 */
    nRet = m_rstDetector.sync_runtime_context(stObservationContext);
    if (nRet != OK)
    {
        dlog_error("同步日夜 detector 运行态失败: %d", nRet);
        return nRet;
    }

    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (m_bRunning && m_u32Epoch == u32Epoch)
        {
            /* 仅在硬件与detector均同步成功后记录状态，失败时允许worker重试同一边界。 */
            m_bLastIsNight = bIsNight;
            m_stLastObservationContext = stObservationContext;
        }
    }
    return OK;
}

int CIspDayNightModeController::start_worker()
{
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (m_bWorkerRunning)
        {
            return OK;
        }
    }

    /* worker已退出但thread对象仍可join时先回收，joinable不能代表线程仍在执行。 */
    if (m_stWorkerThread.joinable())
    {
        m_stWorkerThread.join();
    }

    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        m_bWorkerRunning = true;
    }
    try
    {
        m_stWorkerThread = std::thread(&CIspDayNightModeController::worker_loop, this);
    }
    catch (const std::system_error &stError)
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        m_bWorkerRunning = false;
        dlog_error("日夜worker线程启动失败, code:%d, message:%s", stError.code().value(), stError.what());
        return ERR;
    }
    return OK;
}

void CIspDayNightModeController::stop_worker()
{
    if (m_stWorkerThread.joinable())
    {
        /* lock: 调用方未持有 m_mtx；worker最多等待200ms即可读取停止状态并安全退出。 */
        m_stWorkerThread.join();
    }
}

void CIspDayNightModeController::worker_loop()
{
    dlog_info("AUTO日夜filter worker已启动");
    while (true)
    {
        bool bNeedSubmit = false;
        bool bSuggestedNight = false;
        bool bFilterExpired = false;
        uint32_t u32SubmitEpoch = 0;
        {
            std::lock_guard<std::mutex> stLock(m_mtx);
            if (!m_bRunning)
            {
                m_bWorkerRunning = false;
                break;
            }

            if (m_bInitialized)
            {
                const auto stNow = std::chrono::steady_clock::now();
                if (m_bPendingNight && stNow >= m_stFilterDeadline)
                {
                    bNeedSubmit = true;
                    bFilterExpired = true;
                    bSuggestedNight = m_bPendingSuggestedNight;
                    u32SubmitEpoch = m_u32Epoch;
                    m_bPendingNight = false;
                }
                else if (m_stConfig.enDayNightMode == ISP::TIME_MODE)
                {
                    int nNowSec = m_stClock.fnGetDaySeconds();
                    bool bIsNight = IspDayNightPolicy_NS::is_night_by_time_range(m_stConfig, nNowSec);
                    if (bIsNight != m_bLastIsNight)
                    {
                        bNeedSubmit = true;
                        bSuggestedNight = bIsNight;
                        u32SubmitEpoch = m_u32Epoch;
                    }
                }
            }
        }

        if (bNeedSubmit)
        {
            if (bFilterExpired)
            {
                dlog_info("AUTO日夜检测过滤完成, 提交目标:%s", bSuggestedNight ? "夜间" : "白天");
            }

            int nRet = submit_intent(bSuggestedNight, u32SubmitEpoch);
            if (nRet != OK)
            {
                dlog_error("日夜检测目标提交失败, target:%s, ret:%d", bSuggestedNight ? "夜间" : "白天", nRet);
            }
            continue;
        }

        /* perf: 仅读取内存状态，200ms轮询避免使用目标平台不可靠的绝对deadline条件变量等待。 */
        std::this_thread::sleep_for(WORKER_POLL_INTERVAL);
    }

    dlog_info("AUTO日夜filter worker已退出");
}

int CIspDayNightModeController::stop_detector()
{
    bool bNeedStop = false;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        bNeedStop = m_bDetectorRunning;
    }

    if (!bNeedStop)
    {
        return OK;
    }

    int nRet = m_rstDetector.stop();
    if (nRet != OK)
    {
        dlog_error("停止AUTO日夜 detector 失败: %d", nRet);
        return nRet;
    }

    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        m_bDetectorRunning = false;
    }
    dlog_info("AUTO日夜检测已停止");
    return OK;
}

void CIspDayNightModeController::on_detector_observation(uint32_t u32Epoch, bool bSuggestedNight)
{
    bool bCancelledPending = false;
    bool bStartedPending = false;
    unsigned int nFilterTime = 0;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        if (!m_bRunning || m_u32Epoch != u32Epoch || m_stConfig.enDayNightMode != ISP::AUTO_MODE)
        {
            return;
        }

        if (bSuggestedNight == m_bLastIsNight)
        {
            /* 与当前状态一致的观测取消未完成的反向滤波，避免旧建议在稍后误提交。 */
            bCancelledPending = m_bPendingNight;
            m_bPendingNight = false;
        }
        else
        {
            /* 连续收到相同目标只表示条件仍成立，只有新候选才建立filter deadline。 */
            if (!m_bPendingNight || m_bPendingSuggestedNight != bSuggestedNight)
            {
                m_bPendingNight = true;
                m_bPendingSuggestedNight = bSuggestedNight;
                nFilterTime = m_stConfig.nFilterTime;
                bStartedPending = true;
                /* memory: 截止时间由统一 worker 消费，控制器析构前会 join 该 worker。 */
                m_stFilterStartTime = std::chrono::steady_clock::now();
                m_stFilterDeadline = m_stFilterStartTime + std::chrono::seconds(nFilterTime);
            }
        }
    }

    if (bCancelledPending)
    {
        dlog_info("AUTO日夜检测恢复当前状态, 取消待处理切换");
    }
    else if (bStartedPending)
    {
        dlog_info("AUTO日夜检测收到新建议, target:%s, filter_time:%us, filter已挂起",
                  bSuggestedNight ? "夜间" : "白天",
                  nFilterTime);
    }
}
