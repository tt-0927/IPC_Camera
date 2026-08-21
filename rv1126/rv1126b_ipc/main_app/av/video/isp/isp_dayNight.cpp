/**
 * @FilePath     : isp_dayNight.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:52:37
 * @Description  : RV1126B SmartIR自动日夜观测控制实现
 */

#include "isp_dayNight.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>

#include "IpcRet.h"
#include "dlog.h"
#include "isp_control.h"

namespace
{
constexpr unsigned int SMART_IR_SENSITIVITY_MIN = 1U;
constexpr unsigned int SMART_IR_SENSITIVITY_MAX = 10U;
constexpr unsigned int SMART_IR_DEFAULT_SENSITIVITY = 5U;
constexpr unsigned int SMART_IR_SAMPLE_RATE = 30U;
constexpr float SMART_IR_D2N_ENVL_THRESHOLD = 0.01F;
constexpr float SMART_IR_N2D_ENVL_THRESHOLD = 0.20F;
constexpr float SMART_IR_RGGAIN_BASE = 1.00F;
constexpr float SMART_IR_BGGAIN_BASE = 1.00F;
constexpr float SMART_IR_AWBGAIN_RADIUS = 0.10F;
constexpr float SMART_IR_AWBGAIN_DISPERSION = 0.20F;

/**
 * @brief   : 将共享灵敏度转换为SmartIR稳定样本数
 * @param    {unsigned int} nLevel：灵敏度等级[1,10]
 * @return   {uint16_t} SmartIR稳定样本数
 * @note    : 保持旧产品的等级映射；共享过滤时间不写入该字段，避免双重过滤。
 */
uint16_t to_switch_count_threshold(unsigned int nLevel)
{
    const unsigned int nClampedLevel = std::max(SMART_IR_SENSITIVITY_MIN, std::min(nLevel, SMART_IR_SENSITIVITY_MAX));
    return static_cast<uint16_t>(nClampedLevel * SMART_IR_SAMPLE_RATE);
}
} // namespace

CDayNightController::CDayNightController()
    : m_pstAiqContext(nullptr), m_pstSmartIrContext(nullptr), m_bAcceptedNight(false),
      m_nSensitivity(SMART_IR_DEFAULT_SENSITIVITY), m_bRunning(false), m_nActiveObservationCallbacks(0U)
{
    std::memset(&m_stSmartIrAttr, 0, sizeof(m_stSmartIrAttr));
}

CDayNightController::~CDayNightController()
{
    stop();
}

int CDayNightController::start()
{
    /* lock: 启动、属性配置和SmartIR句柄创建必须串行，避免stop并发释放同一context。 */
    std::lock_guard<std::mutex> stLock(m_mtxObserver);
    if (m_bRunning)
    {
        return OK;
    }
    if (m_pstSmartIrContext != nullptr)
    {
        /* ! 上一次SmartIR释放失败时保留context，必须先由stop重试，禁止覆盖仍可能运行的SDK对象。 */
        return ERR_UNINIT;
    }

    m_pstAiqContext = CIspControl::instance()->get_aiq_ctx();
    if (m_pstAiqContext == nullptr)
    {
        return ERR_UNINIT;
    }

    m_pstSmartIrContext = rk_smart_ir_init(m_pstAiqContext);
    if (m_pstSmartIrContext == nullptr)
    {
        dlog_error("RV1126B SmartIR初始化失败");
        return ERR;
    }

    XCamReturn nRet = rk_smart_ir_getAttr(m_pstSmartIrContext, &m_stSmartIrAttr);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog_error("RV1126B读取SmartIR属性失败: %d", nRet);
        const XCamReturn enDeinitRet = rk_smart_ir_deInit(m_pstSmartIrContext);
        if (enDeinitRet == XCAM_RETURN_NO_ERROR)
        {
            m_pstSmartIrContext = nullptr;
            m_pstAiqContext = nullptr;
        }
        else
        {
            dlog_error("RV1126B读取SmartIR属性失败后释放context失败: %d", enDeinitRet);
        }
        return static_cast<int>(nRet);
    }

    /* note: SmartIR只输出环境候选；固定AUTO和可见光观测，禁止其接管白光PWM。 */
    m_stSmartIrAttr.init_status = m_bAcceptedNight.load() ? RK_SMART_IR_STATUS_NIGHT : RK_SMART_IR_STATUS_DAY;
    m_stSmartIrAttr.switch_mode = RK_SMART_IR_SWITCH_MODE_AUTO;
    m_stSmartIrAttr.light_mode = RK_SMART_IR_LIGHT_MODE_MANUAL;
    m_stSmartIrAttr.light_type = RK_SMART_IR_LIGHT_TYPE_VIS;
    m_stSmartIrAttr.light_value = 0U;
    /* 保留历史SmartIR环境阈值与AWB标定值；仅过滤时间改由共享控制器统一处理。 */
    m_stSmartIrAttr.params.d2n_envL_th = SMART_IR_D2N_ENVL_THRESHOLD;
    m_stSmartIrAttr.params.n2d_envL_th = SMART_IR_N2D_ENVL_THRESHOLD;
    m_stSmartIrAttr.params.rggain_base = SMART_IR_RGGAIN_BASE;
    m_stSmartIrAttr.params.bggain_base = SMART_IR_BGGAIN_BASE;
    m_stSmartIrAttr.params.awbgain_rad = SMART_IR_AWBGAIN_RADIUS;
    m_stSmartIrAttr.params.awbgain_dis = SMART_IR_AWBGAIN_DISPERSION;
    m_stSmartIrAttr.params.switch_cnts_th = to_switch_count_threshold(m_nSensitivity);
    m_stSmartIrAttr.en_quick_switch = false;
    m_stSmartIrAttr.en_grid_weight = false;
    m_stSmartIrAttr.en_auto_n2dth = true;

    nRet = rk_smart_ir_setAttr(m_pstSmartIrContext, &m_stSmartIrAttr);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog_error("RV1126B设置SmartIR观测属性失败: %d", nRet);
        const XCamReturn enDeinitRet = rk_smart_ir_deInit(m_pstSmartIrContext);
        if (enDeinitRet == XCAM_RETURN_NO_ERROR)
        {
            m_pstSmartIrContext = nullptr;
            m_pstAiqContext = nullptr;
        }
        else
        {
            dlog_error("RV1126B设置SmartIR属性失败后释放context失败: %d", enDeinitRet);
        }
        return static_cast<int>(nRet);
    }

    nRet = rk_smart_ir_runCb(m_pstSmartIrContext, false, CDayNightController::smart_ir_callback);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog_error("RV1126B注册SmartIR回调失败: %d", nRet);
        const XCamReturn enDeinitRet = rk_smart_ir_deInit(m_pstSmartIrContext);
        if (enDeinitRet == XCAM_RETURN_NO_ERROR)
        {
            m_pstSmartIrContext = nullptr;
            m_pstAiqContext = nullptr;
        }
        else
        {
            dlog_error("RV1126B注册SmartIR回调失败后释放context失败: %d", enDeinitRet);
        }
        return static_cast<int>(nRet);
    }

    m_bRunning = true;
    return OK;
}

int CDayNightController::stop()
{
    /* step: 先摘除运行标志，再在锁外释放SDK，避免底层回调反向等待本锁。 */
    rk_smart_ir_ctx_t *pstSmartIrContext = nullptr;
    {
        std::lock_guard<std::mutex> stLock(m_mtxObserver);
        if (!m_bRunning && m_pstSmartIrContext == nullptr)
        {
            m_pstAiqContext = nullptr;
            return OK;
        }

        /* lock: 先禁止回调上送，再在锁外等待SDK停止，避免SDK反调本类时发生锁等待。 */
        m_bRunning = false;
        pstSmartIrContext = m_pstSmartIrContext;
    }

    int nRet = OK;
    /* memory: 释放失败时保留context和AIQ借用关系，下一次stop必须继续重试。 */
    if (pstSmartIrContext != nullptr)
    {
        const XCamReturn enRet = rk_smart_ir_deInit(pstSmartIrContext);
        if (enRet != XCAM_RETURN_NO_ERROR)
        {
            dlog_error("RV1126B释放SmartIR失败: %d", enRet);
            nRet = static_cast<int>(enRet);
        }
    }

    {
        std::unique_lock<std::mutex> stLock(m_mtxObserver);
        /* memory: stop返回前等待已取出回调完成，适配器才能释放其回调桥接对象。 */
        m_stObservationCallbackIdleCv.wait(stLock,
                                           [this]
                                           {
                                               return m_nActiveObservationCallbacks == 0U;
                                           });
        if (nRet == OK)
        {
            m_pstSmartIrContext = nullptr;
            m_pstAiqContext = nullptr;
        }
    }
    return nRet;
}

int CDayNightController::sync_runtime_context(const ISP::IspDayNightObservationContext_S &stContext)
{
    std::lock_guard<std::mutex> stLock(m_mtxObserver);
    m_bAcceptedNight.store(stContext.bIsNight);
    if (!m_bRunning)
    {
        return OK;
    }

    m_stSmartIrAttr.init_status = m_bAcceptedNight.load() ? RK_SMART_IR_STATUS_NIGHT : RK_SMART_IR_STATUS_DAY;
    return apply_attr_locked();
}

int CDayNightController::set_sensitivity(unsigned int nLevel)
{
    std::lock_guard<std::mutex> stLock(m_mtxObserver);
    m_nSensitivity = std::max(SMART_IR_SENSITIVITY_MIN, std::min(nLevel, SMART_IR_SENSITIVITY_MAX));
    m_stSmartIrAttr.params.switch_cnts_th = to_switch_count_threshold(m_nSensitivity);
    if (!m_bRunning)
    {
        return OK;
    }
    return apply_attr_locked();
}

bool CDayNightController::is_night_mode() const
{
    return m_bAcceptedNight.load();
}

void CDayNightController::set_observation_callback(ObservationCallback stCallback)
{
    std::lock_guard<std::mutex> stLock(m_mtxObserver);
    m_stObservationCallback = std::move(stCallback);
}

void CDayNightController::smart_ir_callback(rk_smart_ir_result_t stResult)
{
    CDayNightController::instance()->handle_smart_ir_result(stResult);
}

void CDayNightController::handle_smart_ir_result(rk_smart_ir_result_t stResult)
{
    ObservationCallback stCallback;
    bool bSuggestedNight = false;
    {
        std::lock_guard<std::mutex> stLock(m_mtxObserver);
        if (!m_bRunning || !m_stObservationCallback)
        {
            return;
        }

        /* SmartIR异常结果按最后成功态回报，用于取消共享层正在等待的反向候选。 */
        if (stResult.status == RK_SMART_IR_STATUS_NIGHT)
        {
            bSuggestedNight = true;
        }
        else if (stResult.status == RK_SMART_IR_STATUS_DAY)
        {
            bSuggestedNight = false;
        }
        else
        {
            bSuggestedNight = m_bAcceptedNight.load();
        }
        stCallback = m_stObservationCallback;
        ++m_nActiveObservationCallbacks;
    }

    /* lock: 回调在锁外执行，避免共享reconciler同步运行态时反向等待SmartIR锁。 */
    stCallback(bSuggestedNight);
    release_observation_callback();
}

void CDayNightController::release_observation_callback()
{
    std::lock_guard<std::mutex> stLock(m_mtxObserver);
    if (m_nActiveObservationCallbacks == 0U)
    {
        return;
    }
    --m_nActiveObservationCallbacks;
    if (m_nActiveObservationCallbacks == 0U)
    {
        m_stObservationCallbackIdleCv.notify_all();
    }
}

int CDayNightController::apply_attr_locked()
{
    if (m_pstSmartIrContext == nullptr)
    {
        return ERR_UNINIT;
    }
    const XCamReturn nRet = rk_smart_ir_setAttr(m_pstSmartIrContext, &m_stSmartIrAttr);
    if (nRet != XCAM_RETURN_NO_ERROR)
    {
        dlog_error("RV1126B更新SmartIR观测属性失败: %d", nRet);
    }
    return static_cast<int>(nRet);
}
