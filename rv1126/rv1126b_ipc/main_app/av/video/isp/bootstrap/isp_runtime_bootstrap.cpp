/**
 * @FilePath     : isp_runtime_bootstrap.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:51:17
 * @Description  : RV1126B ISP共享核心注册启动器实现
 */

#include "isp_runtime_bootstrap.h"

#include <ctime>
#include <functional>

#include "IpcRet.h"
#include "dlog.h"
#include "fill_light_driver.h"
#include "ircut_driver.h"
#include "isp_business_service.h"
#include "isp_capability_builder.h"
#include "isp_daynight_detector_rv1126b.h"
#include "isp_fill_light_orchestrator.h"
#include "isp_manage.h"
#include "isp_parameter_applier_rv1126b.h"
#include "isp_peripheral_controller.h"
#include "isp_scene_provider_rv1126b.h"
#include "peripheral_profile_builder.h"
#include "time_utils.h"

CRv1126bIspRuntimeBootstrap::CRv1126bIspRuntimeBootstrap() : m_bServiceRegistered(false), m_bPlatformCleanupPending(false)
{
}

CRv1126bIspRuntimeBootstrap::~CRv1126bIspRuntimeBootstrap()
{
    deinit();
}

int CRv1126bIspRuntimeBootstrap::init()
{
    /* step: 已注册时保持幂等；清理未完成时禁止覆盖仍可能被共享层借用的对象。 */
    if (m_bServiceRegistered)
    {
        return OK;
    }
    if (m_bPlatformCleanupPending)
    {
        /* ! 上一次退出仍有硬件资源未释放时，禁止重新注册共享service覆盖回滚状态。 */
        return ERR_UNINIT;
    }

    int nRet = Rv1126bIspCapabilityBuilder_NS::build_profile(m_stCapabilityProfile);
    if (nRet != OK)
    {
        dlog_error("RV1126B ISP能力画像构建失败: %d", nRet);
        return nRet;
    }

    nRet = Rv1126bPeripheralProfileBuilder_NS::build_profile(m_stFillLightProfile, m_stIrCutProfile);
    if (nRet != OK)
    {
        dlog_error("RV1126B补光板级画像构建失败: %d", nRet);
        return nRet;
    }

    /* step: 白光驱动是普通补光和告警闪烁唯一PWM写入口，必须早于共享service初始化。 */
    m_pFillLightDriver = std::make_unique<CFillLightDriver>(m_stFillLightProfile);
    nRet = m_pFillLightDriver->init();
    if (nRet != OK)
    {
        dlog_error("RV1126B白光驱动初始化失败: %d", nRet);
        const int nResetRet = reset_platform_objects();
        if (nResetRet != OK)
        {
            dlog_warn("RV1126B白光驱动初始化失败后释放资源失败: %d", nResetRet);
        }
        return nRet;
    }

    /* IR-CUT画像显式不支持；保留共享adapter统一接口，运行态不会产生IR-CUT目标。 */
    m_pIrCutDriver = std::make_unique<CIrCutDriver>(m_stIrCutProfile);
    m_pParameterApplier = std::make_unique<CIspParameterApplierRv1126b>();
    m_pSceneProvider = std::make_unique<CIspSceneProviderRv1126b>();
    m_pDetector = std::make_unique<CIspDayNightDetectorRv1126b>();
    m_pPeripheral = std::make_unique<CIspPeripheralController>(*m_pFillLightDriver, *m_pIrCutDriver);

    /* memory: bundle只保存四个adapter的引用，必须在shared service整个生命周期内有效。 */
    IspPlatformAdapters_S stAdapters{ *m_pParameterApplier, *m_pSceneProvider, *m_pDetector, *m_pPeripheral };

    /* info: 所有时序值均为毫秒；RV没有IR-CUT动作，但保留共享接口所需字段。 */
    IspTransitionTiming_S stTiming;
    stTiming.nLightOffSettleMs = 100U;
    stTiming.nSceneSettleBeforeIrCutMs = 0U;
    stTiming.nMinIrCutSwitchIntervalMs = 0U;
    stTiming.nPeripheralRetryIntervalMs = 200U;
    stTiming.nPeripheralMaxAttempts = 3U;
    stTiming.nReconcileWaitTimeoutMs = 5000U;

    /* info: 两类计划都使用同一时刻快照，避免跨秒读取造成月份和日内秒数不一致。 */
    auto fnGetSchedulerTime = []() -> IspSchedulerTime_S
    {
        const std::time_t stNow = std::time(nullptr);
        std::tm stLocalTime{};
        if (localtime_r(&stNow, &stLocalTime) == nullptr)
        {
            dlog_error("读取RV1126B场景计划本地时间失败");
            return IspSchedulerTime_S{};
        }
        return IspSchedulerTime_S{ stLocalTime.tm_mon + 1,
                                   stLocalTime.tm_hour * 3600 + stLocalTime.tm_min * 60 + stLocalTime.tm_sec };
    };
    auto fnGetDaySeconds = []() -> int
    {
        return TimeUtils_NS::getSecondsSinceStartOfDay();
    };
    IspSchedulerClock_S stSchedulerClock{ fnGetSchedulerTime };
    IspDayNightClock_S stDayNightClock{ fnGetDaySeconds };

    /* step: service构造后只借用adapter；注册失败必须在销毁service前回收这些依赖。 */
    m_pSharedService = std::make_unique<CIspBusinessService>(stAdapters,
                                                             stTiming,
                                                             stSchedulerClock,
                                                             stDayNightClock,
                                                             m_stCapabilityProfile);
    nRet = CIspManage::instance()->set_business_service(m_pSharedService.get());
    if (nRet != OK)
    {
        dlog_error("RV1126B注册共享ISP服务失败: %d", nRet);
        const int nResetRet = reset_platform_objects();
        if (nResetRet != OK)
        {
            dlog_warn("RV1126B注册共享ISP服务失败后释放资源失败: %d", nResetRet);
        }
        return nRet;
    }

    nRet = CIspManage::instance()->init();
    if (nRet != OK)
    {
        dlog_error("RV1126B共享ISP服务初始化失败: %d", nRet);
        const int nClearRet = CIspManage::instance()->clear_business_service(m_pSharedService.get());
        if (nClearRet != OK)
        {
            /* ! 门面仍引用service时不得销毁其依赖对象，保留给deinit重试清理。 */
            dlog_error("RV1126B撤销共享ISP服务注册失败: %d", nClearRet);
            m_bServiceRegistered = true;
            return nClearRet;
        }
        const int nResetRet = reset_platform_objects();
        if (nResetRet != OK)
        {
            dlog_warn("RV1126B共享ISP初始化失败后释放资源失败: %d", nResetRet);
        }
        return nRet;
    }

    /* done: 只有CIspManage::init成功后才标记已注册，保证失败路径可重复回滚。 */
    m_bServiceRegistered = true;
    dlog_info("RV1126B共享ISP服务注册完成");
    return OK;
}

int CRv1126bIspRuntimeBootstrap::deinit()
{
    /* step: 先停止共享线程，再清除入口指针，最后释放被借用的adapter和驱动。 */
    if (!m_bServiceRegistered)
    {
        return m_bPlatformCleanupPending ? reset_platform_objects() : OK;
    }

    const int nDeinitRet = CIspManage::instance()->deinit();
    if (nDeinitRet != OK)
    {
        dlog_warn("RV1126B共享ISP服务去初始化失败: %d", nDeinitRet);
    }

    const int nClearRet = CIspManage::instance()->clear_business_service(m_pSharedService.get());
    if (nClearRet != OK)
    {
        /* ! 清除失败时保留所有对象，避免CIspManage保存悬空非拥有指针。 */
        dlog_error("RV1126B清除共享ISP服务注册失败: %d", nClearRet);
        return nDeinitRet == OK ? nClearRet : nDeinitRet;
    }

    m_bServiceRegistered = false;
    const int nResetRet = reset_platform_objects();
    if (nDeinitRet != OK)
    {
        return nDeinitRet;
    }
    return nResetRet;
}

int CRv1126bIspRuntimeBootstrap::reset_platform_objects()
{
    /* memory: service析构后才能释放adapter；否则共享reconciler可能访问悬空引用。 */
    m_pSharedService.reset();

    /*
     * 共享service的析构会尝试停止AUTO detector，但共享层析构接口不返回该结果。
     * 在销毁detector适配器前再次核验停止结果，避免SmartIR释放失败后留下悬空SDK上下文。
     */
    if (m_pDetector)
    {
        const int nDetectorRet = m_pDetector->stop();
        if (nDetectorRet != OK)
        {
            dlog_error("RV1126B日夜detector释放失败: %d", nDetectorRet);
            m_bPlatformCleanupPending = true;
            return nDetectorRet;
        }
    }
    m_pPeripheral.reset();
    m_pDetector.reset();
    m_pSceneProvider.reset();
    m_pParameterApplier.reset();
    m_pIrCutDriver.reset();
    if (m_pFillLightDriver)
    {
        const int nRet = m_pFillLightDriver->deinit();
        if (nRet != OK)
        {
            dlog_warn("RV1126B白光驱动释放失败: %d", nRet);
            m_bPlatformCleanupPending = true;
            return nRet;
        }
        m_pFillLightDriver.reset();
    }
    m_bPlatformCleanupPending = false;
    return OK;
}
