/**
 * @FilePath     : isp_business_service.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 15:01:25
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : 共享ISP业务服务实现
 */

#include "isp_business_service.h"

#include <variant>
#include <type_traits>

#include "IpcRet.h"
#include "dlog.h"
#include "isp_param_policy.h"
#include "peripheral_manage.h"

namespace
{
/**
 * @brief   : 校验临时灯光抢占目标
 * @param    {const ISP::IspLightOverride_S&} stOverride：待校验抢占请求
 * @return   {int} OK：合法，ERR_PARAM：灯型、亮度、时间或频率非法
 */
int validate_light_override(const ISP::IspLightOverride_S &stOverride)
{
    if (stOverride.stLight.enLightType != ISP::LIGHT_TYPE_WHITE && stOverride.stLight.enLightType != ISP::LIGHT_TYPE_RED)
    {
        return ERR_PARAM;
    }
    if (stOverride.stLight.nLightLevel == 0U || stOverride.stLight.nLightLevel > 100U)
    {
        return ERR_PARAM;
    }
    if (!stOverride.stLight.bFlashing)
    {
        return OK;
    }
    if (stOverride.stLight.nFlashTimeSec < 1 || stOverride.stLight.nFlashTimeSec > 300)
    {
        return ERR_PARAM;
    }
    switch (stOverride.stLight.enFlashFrequency)
    {
    case Alarm::FlashFrequency_E::FLASH_STEADY_ON:
    case Alarm::FlashFrequency_E::FLASH_LOW_FREQ:
    case Alarm::FlashFrequency_E::FLASH_MID_FREQ:
    case Alarm::FlashFrequency_E::FLASH_HIGH_FREQ:
        return OK;
    default:
        return ERR_PARAM;
    }
}
} // namespace

CIspBusinessService::CIspBusinessService(const IspPlatformAdapters_S &stAdapters,
                                         const IspTransitionTiming_S &stTiming,
                                         const IspSchedulerClock_S &stSchedulerClock,
                                         const IspDayNightClock_S &stDayNightClock,
                                         const ISP::IspCapabilityProfile_S &stProfile)
    : m_stProfile(stProfile), m_stTiming(stTiming), m_stIspRepository(),
      m_stParamOrchestrator(m_stIspRepository, stAdapters.stParameter), m_stSceneOrchestrator(stAdapters.stScene), m_stArbiter(),
      m_stReconciler(stAdapters.stScene, m_stParamOrchestrator, stAdapters.stPeripheral, m_stTiming),
      m_stModeController(stAdapters.stDetector,
                         stDayNightClock,
                         m_stProfile,
                         [this](const ISP::IspDayNightIntent_S &stIntent, ISP::IspDayNightObservationContext_S &stContext) -> int
                         {
                             /* 日夜控制器只提交请求；服务等待本次硬件成功后再返回检测器所需状态。 */
                             m_stArbiter.update_daynight(stIntent);
                             const ISP::IspRuntimeTarget_S stSubmittedTarget = m_stArbiter.get_current_target();
                             int nRet = m_stReconciler.reconcile_now(stSubmittedTarget);
                             if (nRet != OK)
                             {
                                 return nRet;
                             }
                             /* gate可与日夜提交并发；以协调完成后的最新目标再次确认实际灯型。 */
                             const ISP::IspRuntimeTarget_S stTarget = m_stArbiter.get_current_target();
                             if (stTarget.u64Generation != stSubmittedTarget.u64Generation)
                             {
                                 nRet = m_stReconciler.reconcile_now(stTarget);
                                 if (nRet != OK)
                                 {
                                     return nRet;
                                 }
                             }
                             stContext.bIsNight = stIntent.bIsNight;
                             stContext.enRuntimeScene = stTarget.enRuntimeScene;
                             stContext.enRequestedLightType = stIntent.stLight.enLightType;
                             stContext.enActualLightType = stTarget.stLight.enLightType;
                             return OK;
                         }),
      m_stScheduler(stSchedulerClock,
                    m_stProfile,
                    [this](ISP::SceneType_E enScene, bool bActive) -> int
                    {
                        return apply_schedule_config_scene(enScene, bActive);
                    }),
      m_bInitialized(false), m_bGateSinkRegistered(false), m_bReconcilerStarted(false)
{
}

CIspBusinessService::~CIspBusinessService()
{
    deinit();
}

void CIspBusinessService::rollback_init()
{
    if (m_bGateSinkRegistered)
    {
        const int nGateRet = CPeripheralManage::instance()->clear_fill_light_gate_sink(this);
        if (nGateRet != OK)
        {
            dlog_warn("共享ISP初始化回滚补光gate sink失败: %d", nGateRet);
        }
        m_bGateSinkRegistered = false;
    }

    int nRet = m_stModeController.deinit();
    if (nRet != OK)
    {
        dlog_warn("共享ISP初始化回滚日夜控制器失败: %d", nRet);
    }

    nRet = m_stReconciler.stop();
    m_bReconcilerStarted = false;
    if (nRet != OK)
    {
        dlog_warn("共享ISP初始化回滚reconciler失败: %d", nRet);
    }

    nRet = m_stSceneOrchestrator.deinit();
    if (nRet != OK)
    {
        dlog_warn("共享ISP初始化回滚场景资源失败: %d", nRet);
    }
}

int CIspBusinessService::init()
{
    if (m_bInitialized)
    {
        return OK;
    }

    /* step: 场景provider初始化 */
    int nRet = m_stSceneOrchestrator.init();
    if (nRet != OK)
    {
        dlog_error("共享ISP场景初始化失败: %d", nRet);
        return nRet;
    }

    /* step: reconciler启动 */
    nRet = m_stReconciler.start();
    if (nRet != OK)
    {
        dlog_error("共享ISP reconciler启动失败: %d", nRet);
        rollback_init();
        return nRet;
    }
    m_bReconcilerStarted = true;

    /* step: 必须先注册总开关，确保所有初始灯光设置都经过功率和开关检查。 */
    nRet = CPeripheralManage::instance()->set_fill_light_gate_sink(this);
    if (nRet != OK)
    {
        dlog_error("共享ISP注册补光gate sink失败: %d", nRet);
        rollback_init();
        return nRet;
    }
    m_bGateSinkRegistered = true;

    /* step: 读取当前场景并提交用户场景请求。 */
    ISP::AllSceneParams_S stAllParams;
    nRet = m_stIspRepository.load_all_scene_params(stAllParams);
    if (nRet == OK)
    {
        m_stArbiter.update_user_scene(stAllParams.enCurrentScene);
    }

    /* step: 用 DayNightAttr_S 初始化 variant，load 通过当前备选类型选择底层重载。 */
    ISP::IspConfigValue_T stDayNightValue = ISP::DayNightAttr_S{};
    nRet = m_stIspRepository.load(stDayNightValue);
    if (nRet != OK)
    {
        dlog_error("共享ISP日夜配置加载失败: %d", nRet);
        rollback_init();
        return nRet;
    }

    nRet = m_stModeController.init(std::get<ISP::DayNightAttr_S>(stDayNightValue));
    if (nRet != OK)
    {
        dlog_error("共享ISP日夜控制器初始化失败: %d", nRet);
        rollback_init();
        return nRet;
    }

    /* step: 提交初始硬件设置。 */
    submit_to_reconciler();

    /* step: 用 SceneSchedule_S 作为读取类型标记，恢复持久化的计划后再启动调度线程。 */
    ISP::IspConfigValue_T stScheduleValue = ISP::SceneSchedule_S{};
    nRet = m_stIspRepository.load(stScheduleValue);
    if (nRet == OK)
    {
        m_stScheduler.update(std::get<ISP::SceneSchedule_S>(stScheduleValue));
    }
    m_stScheduler.start();

    m_bInitialized = true;
    return OK;
}

int CIspBusinessService::deinit()
{
    if (!m_bInitialized)
    {
        return OK;
    }

    m_bInitialized = false;
    if (m_bGateSinkRegistered)
    {
        const int nGateRet = CPeripheralManage::instance()->clear_fill_light_gate_sink(this);
        if (nGateRet != OK)
        {
            dlog_warn("共享ISP清除补光gate sink失败: %d", nGateRet);
        }
        m_bGateSinkRegistered = false;
    }
    m_stScheduler.stop();
    m_stModeController.deinit();
    m_stReconciler.stop();
    m_bReconcilerStarted = false;
    m_stSceneOrchestrator.deinit();
    return OK;
}

int CIspBusinessService::update_param(ISP::PicConfigureType_E enType)
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    if (enType == ISP::PicConfigureType_E::DAYNIGHT)
    {
        /* variant 的默认日夜对象仅用于指定仓储读取类型，不参与策略计算。 */
        ISP::IspConfigValue_T stValue = ISP::DayNightAttr_S{};
        int nRet = m_stIspRepository.load(stValue);
        if (nRet != OK)
            return nRet;
        return m_stModeController.update_config(std::get<ISP::DayNightAttr_S>(stValue));
    }

    if (enType == ISP::PicConfigureType_E::SCENE)
    {
        /* 全量快照包含当前用户场景，避免从单个参数配置推断场景来源。 */
        ISP::AllSceneParams_S stAllParams;
        int nRet = m_stIspRepository.load_all_scene_params(stAllParams);
        if (nRet != OK)
            return nRet;
        return apply_user_config_scene(stAllParams.enCurrentScene);
    }

    return m_stParamOrchestrator.apply_by_type(enType);
}

int CIspBusinessService::update_daynight(const ISP::DayNightAttr_S &stOld, const ISP::DayNightAttr_S &stNew)
{
    (void) stOld;
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }
    return m_stModeController.update_config(stNew);
}

int CIspBusinessService::apply_scene(ISP::SceneType_E enScene)
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }
    return apply_user_config_scene(enScene);
}

int CIspBusinessService::on_schedule_changed()
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }
    ISP::IspConfigValue_T stValue = ISP::SceneSchedule_S{};
    int nRet = m_stIspRepository.load(stValue);
    if (nRet != OK)
        return nRet;
    return m_stScheduler.update(std::get<ISP::SceneSchedule_S>(stValue));
}

int CIspBusinessService::validate_image_param(ISP::ImageParam_S &stConfig)
{
    return IspParamPolicy_NS::normalize_image_param(stConfig, m_stProfile);
}

int CIspBusinessService::validate_daynight(ISP::DayNightAttr_S &stConfig)
{
    return IspParamPolicy_NS::normalize_daynight(stConfig, m_stProfile);
}

int CIspBusinessService::get_capability_profile(ISP::IspCapabilityProfile_S &stProfile) const
{
    stProfile = m_stProfile;
    return OK;
}

int CIspBusinessService::apply_config(const ISP::IspConfigValue_T &stConfig)
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    /* variant 类型本身就是命令域；此处只负责路由，不重复读取调用方刚持久化的值。 */
    return std::visit(
        [this](const auto &stValue) -> int
        {
            /* T 保留编译期实际类型，使 if constexpr 不会实例化无关分支。 */
            using T = std::decay_t<decltype(stValue)>;
            if constexpr (std::is_same_v<T, ISP::ImageParam_S>)
            {
                return m_stParamOrchestrator.apply_by_type(ISP::PicConfigureType_E::IAMGE);
            }
            else if constexpr (std::is_same_v<T, ISP::ExposureAttr_S>)
            {
                return m_stParamOrchestrator.apply_by_type(ISP::PicConfigureType_E::EXPOSURE);
            }
            else if constexpr (std::is_same_v<T, ISP::BackLightArrt_S>)
            {
                return m_stParamOrchestrator.apply_by_type(ISP::PicConfigureType_E::BACKLIGHT);
            }
            else if constexpr (std::is_same_v<T, ISP::AwbAttr_S>)
            {
                return m_stParamOrchestrator.apply_by_type(ISP::PicConfigureType_E::AWB);
            }
            else if constexpr (std::is_same_v<T, ISP::DnrAttr_S>)
            {
                return m_stParamOrchestrator.apply_by_type(ISP::PicConfigureType_E::NR);
            }
            else if constexpr (std::is_same_v<T, ISP::VideoAdjust_S>)
            {
                return m_stParamOrchestrator.apply_by_type(ISP::PicConfigureType_E::MIRROR);
            }
            else if constexpr (std::is_same_v<T, ISP::DayNightAttr_S>)
            {
                return m_stModeController.update_config(stValue);
            }
            else if constexpr (std::is_same_v<T, ISP::SceneType_E>)
            {
                return apply_user_config_scene(stValue);
            }
            else if constexpr (std::is_same_v<T, ISP::SceneSchedule_S>)
            {
                return m_stScheduler.update(stValue);
            }
        },
        stConfig);
}

int CIspBusinessService::reconcile_all()
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    /* 读取当前场景并提交 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_stIspRepository.load_all_scene_params(stAllParams);
    if (nRet != OK)
    {
        dlog_error("全量协调读取场景快照失败: %d", nRet);
        return nRet;
    }
    m_stArbiter.update_user_scene(stAllParams.enCurrentScene);

    /* 用 DayNightAttr_S 指定仓储读取类型，再由模式控制器重新生成日夜请求。 */
    ISP::IspConfigValue_T stDayNightValue = ISP::DayNightAttr_S{};
    nRet = m_stIspRepository.load(stDayNightValue);
    if (nRet == OK)
    {
        nRet = m_stModeController.update_config(std::get<ISP::DayNightAttr_S>(stDayNightValue));
        if (nRet != OK)
        {
            return nRet;
        }
    }
    else
    {
        return nRet;
    }

    /* 恢复默认可能不改变场景枚举，使用独立的一次性全量重放请求。 */
    const ISP::IspRuntimeTarget_S stTarget = m_stArbiter.get_current_target();
    m_stReconciler.request_full_reconcile(stTarget);
    return m_stReconciler.wait_for_full_reconcile(m_stTiming.nReconcileWaitTimeoutMs);
}

int CIspBusinessService::begin_light_override(const ISP::IspLightOverride_S &stOverride, uint64_t &u64Token)
{
    if (!m_bInitialized)
    {
        u64Token = 0;
        return ERR_UNINIT;
    }
    const int nValidateRet = validate_light_override(stOverride);
    if (nValidateRet != OK)
    {
        u64Token = 0;
        return nValidateRet;
    }
    const int nBeginRet = m_stArbiter.begin_light_override(stOverride, u64Token);
    if (nBeginRet != OK)
    {
        return nBeginRet;
    }
    const ISP::IspRuntimeTarget_S stTarget = m_stArbiter.get_current_target();
    const int nRet = m_stReconciler.reconcile_now(stTarget);
    if (nRet != OK)
    {
        if (!m_stArbiter.end_light_override(u64Token))
        {
            dlog_warn("灯光抢占应用失败后清理token失败, token:%llu", static_cast<unsigned long long>(u64Token));
        }
        submit_to_reconciler();
        u64Token = 0;
        return nRet;
    }
    if (!m_stArbiter.is_light_override_active(u64Token))
    {
        u64Token = 0;
        return ERR_NOT_ENABLED;
    }
    return OK;
}

int CIspBusinessService::end_light_override(uint64_t u64Token)
{
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }
    if (!m_stArbiter.end_light_override(u64Token))
    {
        return ERR_PARAM;
    }
    submit_to_reconciler();
    return OK;
}

int CIspBusinessService::update_fill_light_gate(const Peripheral_NS::FillLightGateState_S &stGate)
{
    const ISP::IspRuntimeTarget_S stOldTarget = m_stArbiter.get_current_target();
    m_stArbiter.update_fill_light_gate(stGate);
    const ISP::IspRuntimeTarget_S stNewTarget = m_stArbiter.get_current_target();

    if (!m_bReconcilerStarted.load())
    {
        return OK;
    }

    if (stNewTarget.u64Generation == stOldTarget.u64Generation)
    {
        const ISP::IspTransitionProgress_S stProgress = m_stReconciler.get_progress();
        if (stProgress.u64Generation != stNewTarget.u64Generation || stProgress.nLastErrorCode == OK)
        {
            return OK;
        }

        /* ! 时间边界首次执行失败时gate版本不会再次变化，必须显式重试同一generation。 */
        m_stReconciler.request_full_reconcile(stNewTarget);
        return m_stReconciler.wait_for_full_reconcile(m_stTiming.nReconcileWaitTimeoutMs);
    }

    /* gate配置事务需要得到真实硬件结果，失败时外设配置服务才能恢复旧文件和旧gate。 */
    return m_stReconciler.reconcile_now(stNewTarget);
}

void CIspBusinessService::submit_to_reconciler()
{
    /* 选择器返回加锁复制的副本；执行线程按更新序号（generation）合并过期请求。 */
    m_stReconciler.request_reconcile(m_stArbiter.get_current_target());
}

int CIspBusinessService::apply_user_config_scene(ISP::SceneType_E enScene)
{
    const ISP::IspRuntimeTarget_S stOldTarget = m_stArbiter.get_current_target();
    m_stArbiter.update_user_scene(enScene);

    /* 计划生效时，用户选择只更新基础请求；实际配置场景不变，因此不切换日夜策略。 */
    return apply_config_scene_transition(stOldTarget,
                                         [this, stOldTarget]()
                                         {
                                             m_stArbiter.update_user_scene(stOldTarget.enConfigScene);
                                         });
}

int CIspBusinessService::apply_schedule_config_scene(ISP::SceneType_E enScene, bool bActive)
{
    ISP::SceneType_E enOldScheduleScene = ISP::SCENE_NORMAL;
    bool bOldScheduleActive = false;
    m_stArbiter.get_schedule_scene(enOldScheduleScene, bOldScheduleActive);

    const ISP::IspRuntimeTarget_S stOldTarget = m_stArbiter.get_current_target();
    if (bActive)
    {
        m_stArbiter.update_schedule_scene(enScene, true);
    }
    else
    {
        m_stArbiter.clear_schedule_scene();
    }

    return apply_config_scene_transition(stOldTarget,
                                         [this, enOldScheduleScene, bOldScheduleActive]()
                                         {
                                             if (bOldScheduleActive)
                                             {
                                                 m_stArbiter.update_schedule_scene(enOldScheduleScene, true);
                                             }
                                             else
                                             {
                                                 m_stArbiter.clear_schedule_scene();
                                             }
                                         });
}

int CIspBusinessService::apply_config_scene_transition(const ISP::IspRuntimeTarget_S &stOldTarget,
                                                       const std::function<void()> &fnRollbackIntent)
{
    const ISP::IspRuntimeTarget_S stNewTarget = m_stArbiter.get_current_target();
    if (stNewTarget.enConfigScene == stOldTarget.enConfigScene)
    {
        submit_to_reconciler();
        return OK;
    }

    /* 先读取新旧完整参数，确保日夜更新失败时能恢复原来的设置。 */
    ISP::SceneParams_S stOldSceneParams;
    int nRet = m_stIspRepository.load_scene_params(stOldTarget.enConfigScene, stOldSceneParams);
    if (nRet != OK)
    {
        dlog_error("读取原配置场景失败, config_scene:%d, ret:%d", static_cast<int>(stOldTarget.enConfigScene), nRet);
        fnRollbackIntent();
        submit_to_reconciler();
        return nRet;
    }

    ISP::SceneParams_S stNewSceneParams;
    nRet = m_stIspRepository.load_scene_params(stNewTarget.enConfigScene, stNewSceneParams);
    if (nRet != OK)
    {
        dlog_error("读取目标配置场景失败, config_scene:%d, ret:%d", static_cast<int>(stNewTarget.enConfigScene), nRet);
        fnRollbackIntent();
        submit_to_reconciler();
        return nRet;
    }

    nRet = m_stModeController.update_config(stNewSceneParams.stDayNightAttr);
    if (nRet == OK)
    {
        submit_to_reconciler();
        return OK;
    }

    /* ! 日夜控制器可能已提交部分新请求；先恢复场景来源，再恢复旧日夜设置，确保回调使用旧设置。 */
    dlog_error("配置场景日夜策略更新失败, config_scene:%d, ret:%d, 恢复原场景",
               static_cast<int>(stNewTarget.enConfigScene),
               nRet);
    fnRollbackIntent();
    int nRestoreRet = m_stModeController.update_config(stOldSceneParams.stDayNightAttr);
    if (nRestoreRet != OK)
    {
        dlog_error("恢复原配置场景日夜策略失败, config_scene:%d, ret:%d",
                   static_cast<int>(stOldTarget.enConfigScene),
                   nRestoreRet);
    }
    submit_to_reconciler();
    return nRet;
}
