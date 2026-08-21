/**
 * @FilePath     : isp_runtime_bootstrap.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-09 13:49:39
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 19:26:20
 * @Description  : Hi3516 ISP共享核心注册启动器实现
 */

#include "isp_runtime_bootstrap.h"

#include <ctime>
#include <functional>

#include "IpcRet.h"
#include "dlog.h"
#include "isp_manage.h"
#include "isp_configure.h"
#include "time_utils.h"

#include "isp_parameter_applier_hi3516.h"
#include "isp_scene_provider_hi3516.h"
#include "isp_daynight_detector_hi3516.h"
#include "isp_peripheral_controller.h"
#include "isp_capability_builder.h"
#include "isp_tuning_builder.h"
#include "path_define.h"
#include "isp_control.h"
#include "isp_dayNight.h"
#include "peripheral_profile_builder.h"
#include "fill_light_driver.h"
#include "ircut_driver.h"

#include "isp_business_service.h"
#include "isp_fill_light_orchestrator.h"

CHi3516IspRuntimeBootstrap::CHi3516IspRuntimeBootstrap() : m_bServiceRegistered(false)
{
}

CHi3516IspRuntimeBootstrap::~CHi3516IspRuntimeBootstrap()
{
    deinit();
}

int CHi3516IspRuntimeBootstrap::init()
{
    if (m_bServiceRegistered)
    {
        return OK;
    }

    /* step1: 先构建平台功能和参数范围；共享层据此拒绝本机型不支持的命令。 */
    int nRet = CapabilityBuilder_NS::build_profile(m_stProfile);
    if (nRet != OK)
    {
        dlog_error("ISP能力画像构建失败: %d", nRet);
        return nRet;
    }

    /* 调参数据保存机型和焦距相关的硬件数值，与平台功能分开，避免共享层读取平台宏。 */
    nRet = IspTuningBuilder_NS::build_tuning_profile(ISP_CONFIG_PATH, m_stTuningProfile);
    if (nRet != OK)
    {
        dlog_error("ISP调参画像构建失败: %d", nRet);
        return nRet;
    }
    nRet = PeripheralProfileBuilder_NS::build_profile(m_stFillLightProfile, m_stIrCutProfile);
    if (nRet != OK)
    {
        dlog_error("摄像机光学外设板级画像构建失败: %d", nRet);
        return nRet;
    }

    /* step2: 在适配器启动前设置调参数据；CIspControl 和日夜检测后续只读取该对象。 */
    nRet = CIspControl::instance()->set_tuning_profile(&m_stTuningProfile);
    if (nRet != OK)
    {
        return nRet;
    }
    /* 日夜检测阈值同样来自调参数据；其接口无返回码，沿用既有底层约定。 */
    CDayNightController::instance()->set_tuning_profile(&m_stTuningProfile);

    /* step3: 先创建独立设备驱动，再将非拥有引用传给平台适配器。 */
    m_pFillLightDriver = std::make_unique<CFillLightDriver>(m_stFillLightProfile);
    nRet = m_pFillLightDriver->init();
    if (nRet != OK)
    {
        dlog_error("Hi3516补光灯驱动初始化失败: %d", nRet);
        reset_platform_objects();
        return nRet;
    }
    m_pIrCutDriver = std::make_unique<CIrCutDriver>(m_stIrCutProfile);

    m_pParamApplier = std::make_unique<CIspParameterApplierHi3516>();
    m_pSceneProvider = std::make_unique<CIspSceneProviderHi3516>(m_stTuningProfile);
    m_pDetector = std::make_unique<CIspDayNightDetectorHi3516>();
    m_pPeripheral = std::make_unique<CIspPeripheralController>(*m_pFillLightDriver, *m_pIrCutDriver);

    /* 将四个非拥有引用传给共享层；启动器必须在共享服务销毁后才能销毁它们。 */
    IspPlatformAdapters_S stAdapters{ *m_pParamApplier, *m_pSceneProvider, *m_pDetector, *m_pPeripheral };

    /* step4: 构造时序和时钟 */
    /* 硬件时序为本平台约束：关灯稳定后切场景，IR-CUT 切换需要最小间隔。 */
    IspTransitionTiming_S stTiming;
    stTiming.nLightOffSettleMs = 100;
    stTiming.nSceneSettleBeforeIrCutMs = 1;
    stTiming.nMinIrCutSwitchIntervalMs = 500;
    stTiming.nPeripheralRetryIntervalMs = 200;
    stTiming.nPeripheralMaxAttempts = 3;
    stTiming.nReconcileWaitTimeoutMs = 5000;

    /* 场景计划一次读取完整本地时间，跨月瞬间不会混用两次读取的月份和秒数。 */
    auto fnGetSchedulerTime = []() -> IspSchedulerTime_S
    {
        const std::time_t stNow = std::time(nullptr);
        std::tm stLocalTime{};
        if (localtime_r(&stNow, &stLocalTime) == nullptr)
        {
            dlog_error("读取场景计划本地时间失败");
            return IspSchedulerTime_S{};
        }
        return IspSchedulerTime_S{ stLocalTime.tm_mon + 1,
                                   stLocalTime.tm_hour * 3600 + stLocalTime.tm_min * 60 + stLocalTime.tm_sec };
    };
    /* TIME 日夜模式只需要当天秒数，继续使用公共时间工具。 */
    auto fnGetDaySeconds = []() -> int
    {
        return TimeUtils_NS::getSecondsSinceStartOfDay();
    };

    /* 两类时钟结构只保存回调副本，不捕获启动器状态。 */
    IspSchedulerClock_S stSchedulerClock{ fnGetSchedulerTime };
    IspDayNightClock_S stDayNightClock{ fnGetDaySeconds };

    /* step5: 构造共享业务服务 */
    m_pSharedService = std::make_unique<CIspBusinessService>(stAdapters,
                                                             stTiming,
                                                             stSchedulerClock,
                                                             stDayNightClock,
                                                             m_stProfile);

    /* step6: 先注册非拥有服务指针，CIspManage 才能创建依赖该服务的命令事务对象。 */
    nRet = CIspManage::instance()->set_business_service(m_pSharedService.get());
    if (nRet != OK)
    {
        dlog_error("注册共享ISP业务服务失败: %d", nRet);
        reset_platform_objects();
        return nRet;
    }

    /* 注册后再启动服务；启动失败时立即清除指针，避免留下无效指针。 */
    nRet = CIspManage::instance()->init();
    if (nRet != OK)
    {
        dlog_error("共享ISP业务服务初始化失败: %d", nRet);
        const int nClearRet = CIspManage::instance()->clear_business_service(m_pSharedService.get());
        if (nClearRet != OK)
        {
            /* ! 入口仍保存服务指针时不能销毁服务，留给 deinit 重试清理。 */
            dlog_error("共享ISP初始化失败后撤销服务注册失败: %d", nClearRet);
            m_bServiceRegistered = true;
            return nClearRet;
        }
        reset_platform_objects();
        return nRet;
    }

    m_bServiceRegistered = true;
    dlog_info("共享ISP业务服务注册完成");
    return OK;
}

int CHi3516IspRuntimeBootstrap::deinit()
{
    if (!m_bServiceRegistered)
    {
        return OK;
    }

    int nRet = CIspManage::instance()->deinit();
    if (nRet != OK)
    {
        dlog_warn("共享ISP业务服务去初始化失败: %d", nRet);
    }

    /* 服务还在时先清除入口中的指针，再释放对象。 */
    const int nClearRet = CIspManage::instance()->clear_business_service(m_pSharedService.get());
    if (nClearRet != OK)
    {
        /* ! 清除服务失败时保留对象，避免入口留下无效指针。 */
        dlog_error("清除共享ISP业务服务注册失败: %d", nClearRet);
        return nRet == OK ? nClearRet : nRet;
    }

    /* step: 先销毁service(停止线程)，再销毁adapters和设备驱动。 */
    reset_platform_objects();

    m_bServiceRegistered = false;
    dlog_info("共享ISP业务服务清理完成");
    return nRet;
}

void CHi3516IspRuntimeBootstrap::reset_platform_objects()
{
    m_pSharedService.reset();
    m_pPeripheral.reset();
    m_pDetector.reset();
    m_pSceneProvider.reset();
    m_pParamApplier.reset();
    m_pIrCutDriver.reset();
    if (m_pFillLightDriver)
    {
        const int nRet = m_pFillLightDriver->deinit();
        if (nRet != OK)
        {
            dlog_warn("Hi3516补光灯驱动释放失败: %d", nRet);
        }
    }
    m_pFillLightDriver.reset();
}
