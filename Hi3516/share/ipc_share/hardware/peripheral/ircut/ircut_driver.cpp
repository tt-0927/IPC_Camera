/**
 * @FilePath     : ircut_driver.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:56:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-18 09:28:44
 * @Description  : 摄像机IR-CUT独立设备驱动实现
 */

#include "ircut_driver.h"

#include <chrono>
#include <thread>

#include "IpcRet.h"
#include "dlog.h"

extern "C"
{
#include "gpio_utils.h"
}

namespace
{
/**
 * @brief GPIO句柄RAII包装，统一处理部分初始化失败的释放路径。
 */
class CGpioHandleGuard
{
public:
    /**
     * @brief   : 申请并初始化输出GPIO句柄
     * @param    {unsigned int} nGpio：GPIO编号
     * @param    {unsigned int} nInitialValue：初始化电平
     * @return   {void}
     */
    CGpioHandleGuard(unsigned int nGpio, unsigned int nInitialValue) : m_nGpio(nGpio), m_pHandle(nullptr), m_bInitialized(false)
    {
        /* 特殊局部变量描述输出GPIO的方向和安全初始空闲电平。 */
        GpioNeedParam_S stNeedParam = { nGpio, true, false, nInitialValue };
        /* memory: 句柄由本RAII对象独占，析构时覆盖申请成功但初始化失败的路径。 */
        m_pHandle = gpio_alloc(stNeedParam);
        if (m_pHandle != nullptr && m_pHandle->gpio_init(m_pHandle) == OK)
        {
            m_bInitialized = true;
        }
    }

    /**
     * @brief   : 反初始化并释放GPIO句柄
     * @return   {void}
     */
    ~CGpioHandleGuard()
    {
        if (m_pHandle == nullptr)
        {
            return;
        }
        /* 仅对成功初始化的句柄执行反初始化，避免向底层传递半初始化资源。 */
        if (m_bInitialized)
        {
            const int nRet = m_pHandle->gpio_uninit(m_pHandle);
            if (nRet != OK)
            {
                dlog_warn("IR-CUT GPIO反初始化失败, gpio:%u, ret:%d", m_nGpio, nRet);
            }
        }
        const int nReleaseRet = gpio_release(m_pHandle);
        if (nReleaseRet != OK)
        {
            dlog_warn("释放IR-CUT GPIO句柄失败, gpio:%u, ret:%d", m_nGpio, nReleaseRet);
        }
    }

    CGpioHandleGuard(const CGpioHandleGuard &) = delete;
    CGpioHandleGuard &operator=(const CGpioHandleGuard &) = delete;

    /**
     * @brief   : 查询句柄是否完成初始化
     * @return   {bool} true：可用，false：申请或初始化失败
     */
    bool valid() const
    {
        return m_bInitialized;
    }

    /**
     * @brief   : 设置当前GPIO电平
     * @param    {unsigned int} nValue：目标电平
     * @return   {int} OK：成功，非OK：失败
     */
    int set_value(unsigned int nValue)
    {
        if (!m_bInitialized)
        {
            return ERR_UNINIT;
        }
        return m_pHandle->set_value(m_nGpio, nValue);
    }

private:
    unsigned int m_nGpio;
    GpioHandle_S *m_pHandle;
    bool m_bInitialized;
};
} // namespace

CIrCutDriver::CIrCutDriver(const IrCutProfile_S &stProfile)
    : m_stProfile(stProfile), m_bHasLastTarget(false), m_enLastTarget(IrCutTarget_E::NONE)
{
}

int CIrCutDriver::switch_target(IrCutTarget_E enTarget)
{
    /* NONE表示调用方无需IR-CUT动作，是无副作用的合法目标。 */
    if (enTarget == IrCutTarget_E::NONE)
    {
        return OK;
    }
    if (!m_stProfile.bSupported)
    {
        return ERR_UNSUPPORT;
    }

    /* lock: IR-CUT为双GPIO脉冲动作，必须串行防止两次切换交叉驱动线圈。 */
    std::lock_guard<std::mutex> stLock(m_mtxDevice);
    if (m_bHasLastTarget && m_enLastTarget == enTarget)
    {
        return OK;
    }

    /* 特殊局部变量保留动作结果，非法枚举不会污染上一次成功目标。 */
    int nRet = ERR_PARAM;
    if (enTarget == IrCutTarget_E::DAY)
    {
        nRet = pulse_locked(m_stProfile.nDayPin1Value, m_stProfile.nDayPin2Value, "白天");
    }
    else if (enTarget == IrCutTarget_E::NIGHT)
    {
        nRet = pulse_locked(m_stProfile.nNightPin1Value, m_stProfile.nNightPin2Value, "夜晚");
    }

    /* 只有脉冲及复位完整成功后才缓存目标，失败请求必须允许后续重试。 */
    if (nRet == OK)
    {
        m_bHasLastTarget = true;
        m_enLastTarget = enTarget;
    }
    return nRet;
}

int CIrCutDriver::pulse_locked(unsigned int nPin1Value, unsigned int nPin2Value, const char *pTargetName)
{
    /* 两个局部RAII守卫以空闲电平创建GPIO，保证所有失败分支均释放资源。 */
    CGpioHandleGuard stPin1(m_stProfile.nGpioPin1, m_stProfile.nIdleValue);
    CGpioHandleGuard stPin2(m_stProfile.nGpioPin2, m_stProfile.nIdleValue);
    if (!stPin1.valid() || !stPin2.valid())
    {
        dlog_error("IR-CUT GPIO申请或初始化失败, target:%s, gpio:%u/%u",
                   pTargetName,
                   m_stProfile.nGpioPin1,
                   m_stProfile.nGpioPin2);
        return ERR;
    }

    /* 按画像组合写入双GPIO电平，形成指定方向的线圈驱动脉冲。 */
    int nActionRet = stPin1.set_value(nPin1Value);
    if (nActionRet == OK)
    {
        nActionRet = stPin2.set_value(nPin2Value);
    }
    if (nActionRet == OK)
    {
        /* 保持画像规定的脉冲宽度，调用方持有设备锁以禁止期间插入反向动作。 */
        std::this_thread::sleep_for(std::chrono::milliseconds(m_stProfile.nPulseHoldMs));
    }
    else
    {
        dlog_error("IR-CUT动作脉冲设置失败, target:%s, ret:%d", pTargetName, nActionRet);
    }

    /* ! 无论第一路复位是否成功，都必须继续尝试复位第二路，避免线圈长期带电。 */
    const int nPin1Ret = stPin1.set_value(m_stProfile.nIdleValue);
    const int nPin2Ret = stPin2.set_value(m_stProfile.nIdleValue);
    if (nPin1Ret != OK || nPin2Ret != OK)
    {
        dlog_error("IR-CUT脉冲复位失败, target:%s, pin1_ret:%d, pin2_ret:%d", pTargetName, nPin1Ret, nPin2Ret);
        return ERR;
    }
    return nActionRet == OK ? OK : ERR;
}
