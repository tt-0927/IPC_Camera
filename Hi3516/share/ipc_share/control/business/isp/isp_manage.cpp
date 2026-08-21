/**
 * @FilePath     : isp_manage.cpp
 * @Author       : cyc
 * @Date         : 2025-08-27 09:51:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-12 14:37:03
 * @Description  : ISP共享层调用入口实现
 */

#include "isp_manage.h"

#include "IpcRet.h"
#include "dlog.h"

CIspManage::CIspManage() : m_pstService(nullptr)
{
}

CIspManage::~CIspManage()
{
}

CIspManage::CServiceCallGuard::CServiceCallGuard(const CIspManage &rstManage) : m_rstManage(rstManage), m_pstService(nullptr)
{
    std::lock_guard<std::mutex> stLock(m_rstManage.m_mtxService);
    m_pstService = m_rstManage.m_pstService;
    if (m_pstService != nullptr)
    {
        ++m_rstManage.m_nActiveServiceCalls;
    }
}

CIspManage::CServiceCallGuard::~CServiceCallGuard()
{
    if (m_pstService != nullptr)
    {
        m_rstManage.release_service_call();
    }
}

ISP::IIspBusinessService *CIspManage::CServiceCallGuard::get() const
{
    return m_pstService;
}

void CIspManage::release_service_call() const
{
    std::lock_guard<std::mutex> stLock(m_mtxService);
    --m_nActiveServiceCalls;
    if (m_nActiveServiceCalls == 0)
    {
        m_stServiceIdleCv.notify_all();
    }
}

int CIspManage::set_business_service(ISP::IIspBusinessService *pService)
{
    if (pService == nullptr)
    {
        return ERR_PARAM_NULL;
    }

    std::lock_guard<std::mutex> stLock(m_mtxService);

    if (m_bServiceClearing)
    {
        dlog_warn("ISP业务服务正在注销，拒绝新的注册请求");
        return ERR;
    }

    if (m_pstService != nullptr && m_pstService != pService)
    {
        dlog_error("ISP业务服务重复注册");
        return ERR;
    }

    /* 只保存服务接口，不保存平台实现，方便其他芯片复用。 */
    m_pstService = pService;
    /* service注册成功后构造命令服务，typed API通过命令服务统一执行事务。 */
    if (!m_pstCommandService)
    {
        m_pstCommandService = std::make_unique<CIspConfigCommandService>(m_stIspRepository, *pService);
    }
    return OK;
}

int CIspManage::clear_business_service(ISP::IIspBusinessService *pService)
{
    if (pService == nullptr)
    {
        return ERR_PARAM_NULL;
    }

    std::unique_lock<std::mutex> stLock(m_mtxService);

    if (m_pstService != pService)
    {
        dlog_error("清理ISP业务服务失败: 指针不匹配");
        return ERR_PARAM;
    }

    /* ! 先拒绝新服务句柄和新注册，再等待已发出的调用返回，外部方可在本函数返回后销毁业务对象。 */
    m_bServiceClearing = true;
    m_pstService = nullptr;
    /* 命令服务在同一锁域内执行，取得本锁说明其不会继续借用已注销服务。 */
    m_pstCommandService.reset();
    m_stServiceIdleCv.wait(stLock,
                           [this]
                           {
                               return m_nActiveServiceCalls == 0;
                           });
    m_bServiceClearing = false;
    return OK;
}

int CIspManage::init()
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        dlog_warn("ISP业务服务未注册，跳过共享入口初始化");
        return OK;
    }

    /* 初始化由业务实现完成；这里不保存平台状态。 */
    return pService->init();
}

int CIspManage::deinit()
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        return OK;
    }

    /* 去初始化必须在解除服务注册前完成，以便业务实现仍可完成自己的清理链路。 */
    return pService->deinit();
}

int CIspManage::reconcile_all()
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        dlog_error("ISP全量配置重放失败: 业务服务未注册");
        return ERR_UNINIT;
    }

    /* 通过受保护的服务句柄转发，保证全量重放期间业务服务不会被注销。 */
    return pService->reconcile_all();
}

int CIspManage::update_config(const ISP::PicConfigureType_E &enConfigType)
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        dlog_error("ISP配置更新失败: 业务服务未注册, type:%d", static_cast<int>(enConfigType));
        return ERR_UNINIT;
    }

    /* 具体参数类型的校验和硬件下发均由业务服务实现，避免共享层依赖平台代码。 */
    return pService->update_param(enConfigType);
}

int CIspManage::update_daynight(const ISP::DayNightAttr_S &stOldDayNightAttr, const ISP::DayNightAttr_S &stNewDayNightAttr)
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        dlog_error("ISP日夜配置更新失败: 业务服务未注册");
        return ERR_UNINIT;
    }

    /* 同时转发旧、新快照，使业务层可仅在运行态确实变化时触发重同步。 */
    return pService->update_daynight(stOldDayNightAttr, stNewDayNightAttr);
}

int CIspManage::apply_scene_all_params(ISP::SceneType_E enSceneType)
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        dlog_error("ISP场景应用失败: 业务服务未注册, scene:%d", static_cast<int>(enSceneType));
        return ERR_UNINIT;
    }

    /* 共享层只表达场景请求，场景资源和参数重放留给业务实现编排。 */
    return pService->apply_scene(enSceneType);
}

int CIspManage::on_schedule_changed()
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        dlog_error("ISP场景计划更新失败: 业务服务未注册");
        return ERR_UNINIT;
    }

    return pService->on_schedule_changed();
}

int CIspManage::validate_image_param_config(ISP::ImageParam_S &stConfig) const
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        dlog_error("ISP图像参数校验失败: 业务服务未注册");
        return ERR_UNINIT;
    }

    return pService->validate_image_param(stConfig);
}

int CIspManage::validate_daynight_config(ISP::DayNightAttr_S &stConfig) const
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        dlog_error("ISP日夜参数校验失败: 业务服务未注册");
        return ERR_UNINIT;
    }

    return pService->validate_daynight(stConfig);
}

int CIspManage::set_image_config(const ISP::ImageParam_S &stConfig)
{
    /* lock: 命令事务期间保持 service/command service 生命周期稳定，禁止并发注销。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->set_image_config(stConfig);
}

int CIspManage::set_exposure_config(const ISP::ExposureAttr_S &stConfig)
{
    /* lock: 命令事务期间保持 service/command service 生命周期稳定，禁止并发注销。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->set_exposure_config(stConfig);
}

int CIspManage::set_daynight_config(const ISP::DayNightAttr_S &stConfig)
{
    /* lock: 命令事务期间保持 service/command service 生命周期稳定，禁止并发注销。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->set_daynight_config(stConfig);
}

int CIspManage::set_backlight_config(const ISP::BackLightArrt_S &stConfig)
{
    /* lock: 命令事务期间保持 service/command service 生命周期稳定，禁止并发注销。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->set_backlight_config(stConfig);
}

int CIspManage::set_awb_config(const ISP::AwbAttr_S &stConfig)
{
    /* lock: 命令事务期间保持 service/command service 生命周期稳定，禁止并发注销。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->set_awb_config(stConfig);
}

int CIspManage::set_nr_config(const ISP::DnrAttr_S &stConfig)
{
    /* lock: 命令事务期间保持 service/command service 生命周期稳定，禁止并发注销。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->set_nr_config(stConfig);
}

int CIspManage::set_mirror_config(const ISP::VideoAdjust_S &stConfig)
{
    /* lock: 命令事务期间保持 service/command service 生命周期稳定，禁止并发注销。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->set_mirror_config(stConfig);
}

int CIspManage::set_scene_schedule(const ISP::SceneSchedule_S &stConfig)
{
    /* lock: 命令事务期间保持 service/command service 生命周期稳定，禁止并发注销。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->set_scene_schedule(stConfig);
}

int CIspManage::set_user_scene(ISP::SceneType_E enScene)
{
    /* lock: 命令事务期间保持 service/command service 生命周期稳定，禁止并发注销。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->set_user_scene(enScene);
}

int CIspManage::restore_default_config()
{
    /* lock: 恢复事务会读写多份配置，期间不允许清理其依赖的业务服务。 */
    std::lock_guard<std::mutex> stLock(m_mtxService);
    if (!m_pstCommandService)
    {
        return ERR_UNINIT;
    }
    return m_pstCommandService->restore_default_config();
}

int CIspManage::begin_light_override(const ISP::IspLightOverride_S &stOverride, uint64_t &u64Token)
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        return ERR_UNINIT;
    }
    return pService->begin_light_override(stOverride, u64Token);
}

int CIspManage::end_light_override(uint64_t u64Token)
{
    CServiceCallGuard stServiceCall(*this);
    ISP::IIspBusinessService *pService = stServiceCall.get();
    if (pService == nullptr)
    {
        return ERR_UNINIT;
    }
    return pService->end_light_override(u64Token);
}
