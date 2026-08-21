/**
 * @FilePath     : isp_runtime_bootstrap.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:17:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : Hi3516 ISP共享核心注册启动器
 */

#pragma once

#include <memory>
#include "Singleton.h"
#include "isp_capability_profile.h"
#include "isp_tuning_profile.h"
#include "fill_light_profile.h"
#include "ircut_profile.h"

class CIspParameterApplierHi3516;
class CIspSceneProviderHi3516;
class CIspDayNightDetectorHi3516;
class CIspPeripheralController;
class CIspBusinessService;
class CFillLightDriver;
class CIrCutDriver;

class CHi3516IspRuntimeBootstrap : public CSingleton<CHi3516IspRuntimeBootstrap>
{
public:
    /**
     * @brief   : 构造 Hi3516 ISP 启动器。
     * @return   {void}
     */
    CHi3516IspRuntimeBootstrap();
    /**
     * @brief   : 销毁启动器并清理已注册的共享服务。
     * @return   {void}
     */
    ~CHi3516IspRuntimeBootstrap();
    friend class CSingleton<CHi3516IspRuntimeBootstrap>;

    /**
     * @brief   : 读取硬件配置，创建适配器并注册 ISP 服务。
     * @return   {int} OK：成功，非OK：构建或注册失败
     */
    int init();
    /**
     * @brief   : 先停止共享服务，再释放 Hi3516 对象。
     * @return   {int} OK：成功，非OK：停止失败
     */
    int deinit();

private:
    /**
     * @brief   : 按创建的相反顺序释放服务、适配器和外设驱动
     * @return   {void}
     */
    void reset_platform_objects();

    /* memory: 共享服务停止后才能释放以下适配器。 */
    /* 参数适配器 */
    std::unique_ptr<CIspParameterApplierHi3516> m_pParamApplier;
    /* 场景适配端口 */
    std::unique_ptr<CIspSceneProviderHi3516> m_pSceneProvider;
    /* 日夜检测器 */
    std::unique_ptr<CIspDayNightDetectorHi3516> m_pDetector;
    /* 外设控制器 */
    std::unique_ptr<CIspPeripheralController> m_pPeripheral;
    /* 白光、红外及闪烁唯一设备驱动。 */
    std::unique_ptr<CFillLightDriver> m_pFillLightDriver;
    /* IR-CUT唯一GPIO设备驱动。 */
    std::unique_ptr<CIrCutDriver> m_pIrCutDriver;
    /* 业务服务 */
    std::unique_ptr<CIspBusinessService> m_pSharedService;
    /* 调参数据必须在所有适配器销毁后才释放。 */
    Hi3516TuningProfile_S m_stTuningProfile;
    /* 摄像机补光灯硬件配置。 */
    FillLightProfile_S m_stFillLightProfile;
    /* 摄像机 IR-CUT 硬件配置。 */
    IrCutProfile_S m_stIrCutProfile;
    /* 当前机型支持的功能和参数范围。 */
    ISP::IspCapabilityProfile_S m_stProfile;
    /* 注册成功标记，同时作为 init/deinit 幂等边界。 */
    bool m_bServiceRegistered;
};
