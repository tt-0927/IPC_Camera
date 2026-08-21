/**
 * @FilePath     : isp_runtime_bootstrap.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:39:13
 * @Description  : RV1126B ISP共享核心注册启动器声明
 */

#pragma once

#include <memory>

#include "Singleton.h"
#include "fill_light_profile.h"
#include "ircut_profile.h"
#include "isp_capability_profile.h"

class CIspParameterApplierRv1126b;
class CIspSceneProviderRv1126b;
class CIspDayNightDetectorRv1126b;
class CIspPeripheralController;
class CIspBusinessService;
class CFillLightDriver;
class CIrCutDriver;

/**
 * @brief RV1126B共享ISP核心启动器。
 * @note 负责把RK AIQ、SmartIR、白光驱动和四个业务适配端口注册到共享ISP服务；不实现
 *       日夜策略、定时过滤或补光仲裁。生命周期被CStreamVideo包围，RK AIQ上下文必须在
 *       共享服务、适配端口和SmartIR释放之后才允许销毁。
 */
class CRv1126bIspRuntimeBootstrap : public CSingleton<CRv1126bIspRuntimeBootstrap>
{
public:
    /**
     * @brief   : 构造RV1126B共享ISP启动器
     * @return   {void}
     */
    CRv1126bIspRuntimeBootstrap();
    /**
     * @brief   : 反序注销共享服务并释放业务侧资源
     * @return   {void}
     */
    ~CRv1126bIspRuntimeBootstrap();
    friend class CSingleton<CRv1126bIspRuntimeBootstrap>;

    /**
     * @brief   : 构建RV1126B画像、适配端口并注册共享ISP服务
     * @return   {int} OK：成功，非OK：画像、驱动或服务初始化失败
     */
    int init();
    /**
     * @brief   : 停止共享ISP服务并按反序释放RV1126B资源
     * @return   {int} OK：成功，非OK：服务注销或驱动释放失败
     */
    int deinit();

private:
    /**
     * @brief   : 按资源依赖反序释放服务、adapter和外设驱动
     * @return   {int} OK：资源已释放，非OK：外设驱动释放失败
     */
    int reset_platform_objects();

    /* memory: 共享service只借用参数适配器，service注销前不得释放。 */
    std::unique_ptr<CIspParameterApplierRv1126b> m_pParameterApplier;
    /* memory: 场景适配器借用RK AIQ上下文，必须先于CIspControl释放。 */
    std::unique_ptr<CIspSceneProviderRv1126b> m_pSceneProvider;
    /* memory: SmartIR适配器拥有回调桥接状态，释放前必须停止在途回调。 */
    std::unique_ptr<CIspDayNightDetectorRv1126b> m_pDetector;
    /* memory: 外设适配器只借用两个底层驱动，驱动生命周期必须覆盖service。 */
    std::unique_ptr<CIspPeripheralController> m_pPeripheral;
    /* memory: 白光驱动拥有PWM worker，deinit返回后才允许销毁。 */
    std::unique_ptr<CFillLightDriver> m_pFillLightDriver;
    /* memory: IR-CUT驱动由启动器独占；当前画像禁用其硬件输出。 */
    std::unique_ptr<CIrCutDriver> m_pIrCutDriver;
    /* memory: CIspManage只保存该对象的非拥有指针，clear成功后才能释放。 */
    std::unique_ptr<CIspBusinessService> m_pSharedService;
    /* 白光PWM和互斥等待参数；由启动阶段构建后保持不变。 */
    FillLightProfile_S m_stFillLightProfile;
    /* IR-CUT板级画像；当前RV产品固定为不支持。 */
    IrCutProfile_S m_stIrCutProfile;
    /* 共享层能力范围；仅由本业务仓按编译能力宏构建。 */
    ISP::IspCapabilityProfile_S m_stCapabilityProfile;
    /* service已注册到CIspManage且仍由本启动器持有。 */
    bool m_bServiceRegistered;
    /* 外部service已注销但适配器或硬件驱动释放失败，保留对象供下次deinit重试。 */
    bool m_bPlatformCleanupPending;
};
