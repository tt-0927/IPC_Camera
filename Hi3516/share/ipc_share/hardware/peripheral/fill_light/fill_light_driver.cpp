/**
 * @FilePath     : fill_light_driver.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:56:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-18 09:28:44
 * @Description  : 摄像机白光红外补光统一设备驱动实现
 */

#include "fill_light_driver.h"

#include <algorithm>
#include <pthread.h>
#include <system_error>

#include "IpcRet.h"
#include "dlog.h"
#include "pwm_utils.h"

namespace
{
/**
 * @brief   : 从稳定画像中取得物理通道配置
 * @param    {Peripheral_NS::LightChannel_E} enChannel：物理通道
 * @param    {const FillLightProfile_S&} stProfile：补光设备画像
 * @return   {const FillLightPwmChannelProfile_S*} 通道画像，非法通道返回nullptr
 */
const FillLightPwmChannelProfile_S *get_pwm_profile(Peripheral_NS::LightChannel_E enChannel, const FillLightProfile_S &stProfile)
{
    if (enChannel == Peripheral_NS::LightChannel_E::WHITE)
    {
        return &stProfile.stWhite;
    }
    if (enChannel == Peripheral_NS::LightChannel_E::INFRARED)
    {
        return &stProfile.stInfrared;
    }
    return nullptr;
}

/**
 * @brief   : 将最终用户亮度转换为目标通道PWM占空比
 * @param    {const FillLightPwmChannelProfile_S&} stProfile：通道映射画像
 * @param    {unsigned int} nOutputLevel：最终亮度[1,100]
 * @return   {unsigned int} PWM占空比
 */
unsigned int calculate_duty_cycle(const FillLightPwmChannelProfile_S &stProfile, unsigned int nOutputLevel)
{
    const unsigned int nLevel = std::min(nOutputLevel, 100U);
    const unsigned int nMappedDuty = stProfile.nDutyOffset + stProfile.nDutyStep * nLevel;
    return std::min(nMappedDuty, stProfile.nDutyMax);
}

/**
 * @brief   : 释放PWM句柄并记录失败上下文
 * @param    {PwmHandle_S*} pHandle：PWM句柄
 * @param    {const FillLightPwmChannelProfile_S&} stProfile：通道画像
 * @return   {int} OK：成功，非OK：释放失败
 */
int release_pwm_handle(PwmHandle_S *pHandle, const FillLightPwmChannelProfile_S &stProfile)
{
    const int nRet = pwm_release(pHandle);
    if (nRet != OK)
    {
        dlog_error("释放补光PWM句柄失败, pwm:%u, channel:%u, ret:%d", stProfile.nController, stProfile.nChannel, nRet);
    }
    return nRet;
}

/**
 * @brief   : 设置单路PWM最终占空比和启用状态
 * @param    {const FillLightPwmChannelProfile_S&} stProfile：通道画像
 * @param    {unsigned int} nDutyCycle：最终占空比
 * @param    {bool} bEnable：是否启用输出
 * @return   {int} OK：成功，非OK：PWM操作失败
 */
int set_pwm_output(const FillLightPwmChannelProfile_S &stProfile, unsigned int nDutyCycle, bool bEnable)
{
    /* 特殊局部变量封装本次PWM申请参数；每次操作独占短生命周期句柄，避免跨板级配置复用。 */
    PwmNeedParam_S stNeedParam = { stProfile.nController, stProfile.nChannel, stProfile.nPeriod, nDutyCycle, 0U };
    /* memory: PWM句柄由本函数申请并在所有返回路径释放。 */
    PwmHandle_S *pHandle = pwm_alloc(stNeedParam);
    if (pHandle == nullptr)
    {
        dlog_error("申请补光PWM句柄失败, pwm:%u, channel:%u", stProfile.nController, stProfile.nChannel);
        return ERR;
    }

    /* 初始化后才允许改写输出；关闭路径保留降级动作以优先消除残留亮灯。 */
    const int nInitRet = pHandle->pwm_init(pHandle);
    if (nInitRet != OK && bEnable)
    {
        dlog_error("初始化补光PWM失败, pwm:%u, channel:%u, ret:%d", stProfile.nController, stProfile.nChannel, nInitRet);
        const int nReleaseRet = release_pwm_handle(pHandle, stProfile);
        return nReleaseRet == OK ? ERR : nReleaseRet;
    }
    if (nInitRet != OK)
    {
        /* ! 关闭是独立安全动作，初始化失败仍必须继续尝试disable。 */
        dlog_warn("关闭补光前初始化PWM失败，继续禁用输出, pwm:%u, channel:%u, ret:%d",
                  stProfile.nController,
                  stProfile.nChannel,
                  nInitRet);
    }

    /* PWM占空比在申请参数中设置，此调用只提交最终的导通/关断状态。 */
    const int nEnableRet = pHandle->set_enable(pHandle, bEnable ? 1U : 0U);
    if (nEnableRet != OK)
    {
        dlog_error("设置补光PWM启用状态失败, pwm:%u, channel:%u, enable:%d, ret:%d",
                   stProfile.nController,
                   stProfile.nChannel,
                   bEnable,
                   nEnableRet);
    }
    const int nReleaseRet = release_pwm_handle(pHandle, stProfile);
    return (nEnableRet == OK && nReleaseRet == OK) ? OK : ERR;
}
} // namespace

CFillLightDriver::CFillLightDriver(const FillLightProfile_S &stProfile)
    : m_stProfile(stProfile), m_bRunning(false), m_bInitialized(false), m_bLightOn(false), m_bFlashExpired(false),
      m_u64TargetVersion(0)
{
}

CFillLightDriver::~CFillLightDriver()
{
    deinit();
}

int CFillLightDriver::init()
{
    std::lock_guard<std::mutex> stLock(m_mtxDevice);
    if (m_bInitialized)
    {
        return OK;
    }

    /* 初始化先强制关闭全部物理通道，清理重启或上次异常遗留的导通状态。 */
    int nRet = turn_off_all_locked();
    if (nRet != OK)
    {
        return nRet;
    }

    /* 状态完成初始化后再启动worker，worker只会观察到完整且安全的初始快照。 */
    m_bRunning = true;
    m_bInitialized = true;
    try
    {
        m_stFlashWorker = std::thread(&CFillLightDriver::flash_worker_loop, this);
    }
    catch (const std::system_error &stError)
    {
        m_bRunning = false;
        m_bInitialized = false;
        dlog_error("补光灯闪烁线程启动失败, code:%d, message:%s", stError.code().value(), stError.what());
        return ERR;
    }
    return OK;
}

int CFillLightDriver::deinit()
{
    {
        std::lock_guard<std::mutex> stLock(m_mtxDevice);
        if (!m_bInitialized)
        {
            return OK;
        }
        /* 递增版本并唤醒等待线程，使其放弃旧闪烁目标后退出。 */
        m_bRunning = false;
        ++m_u64TargetVersion;
        m_stCv.notify_all();
    }

    if (m_stFlashWorker.joinable())
    {
        m_stFlashWorker.join();
    }

    /* join建立worker退出屏障，随后才能安全执行最终硬件关灯。 */
    std::lock_guard<std::mutex> stLock(m_mtxDevice);
    const int nRet = turn_off_all_locked();
    m_bInitialized = false;
    return nRet;
}

int CFillLightDriver::apply_target(const Peripheral_NS::FillLightHardwareTarget_S &stTarget)
{
    /* 锁外完成纯参数校验，缩短硬件状态锁持有时间。 */
    int nRet = validate_target(stTarget);
    if (nRet != OK)
    {
        return nRet;
    }

    std::lock_guard<std::mutex> stLock(m_mtxDevice);
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    /* 闪烁与常亮拥有不同状态机：前者由worker续驱动，后者立即提交硬件。 */
    if (stTarget.bFlashing)
    {
        nRet = apply_flashing_target_locked(stTarget);
    }
    else
    {
        nRet = apply_steady_target_locked(stTarget);
    }

    if (nRet == OK)
    {
        /* 新版本是worker的取消令牌，确保旧目标不会在唤醒后再次切换PWM。 */
        ++m_u64TargetVersion;
        m_stCv.notify_all();
    }
    return nRet;
}

int CFillLightDriver::turn_off(Peripheral_NS::LightChannel_E enChannel)
{
    if (enChannel == Peripheral_NS::LightChannel_E::NONE)
    {
        return ERR_PARAM;
    }

    std::lock_guard<std::mutex> stLock(m_mtxDevice);
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }

    /* 手动关闭同时取消该通道上的告警闪烁，不能只修改硬件不修改状态机。 */
    const int nRet = turn_off_channel_locked(enChannel);
    if (nRet == OK && m_stAppliedTarget.enChannel == enChannel)
    {
        m_stAppliedTarget = Peripheral_NS::FillLightHardwareTarget_S{};
        m_bLightOn = false;
        m_bFlashExpired = false;
        ++m_u64TargetVersion;
        m_stCv.notify_all();
    }
    return nRet;
}

int CFillLightDriver::validate_target(const Peripheral_NS::FillLightHardwareTarget_S &stTarget) const
{
    /* 硬件亮度协议固定为百分比，拒绝越界以免画像映射得到不可预测占空比。 */
    if (stTarget.nOutputLevel > 100U)
    {
        return ERR_PARAM;
    }
    /* 板级画像未声明红外时必须显式拒绝，禁止误驱动复用PWM/GPIO。 */
    if (stTarget.enChannel == Peripheral_NS::LightChannel_E::INFRARED && !m_stProfile.bInfraredSupported)
    {
        return ERR_UNSUPPORT;
    }
    if (stTarget.enChannel == Peripheral_NS::LightChannel_E::NONE || stTarget.nOutputLevel == 0U)
    {
        return stTarget.bFlashing ? ERR_PARAM : OK;
    }
    if (stTarget.bFlashing && (stTarget.nFlashTimeSec < 1 || stTarget.nFlashTimeSec > 300))
    {
        return ERR_PARAM;
    }
    return OK;
}

int CFillLightDriver::apply_steady_target_locked(const Peripheral_NS::FillLightHardwareTarget_S &stTarget)
{
    if (stTarget.enChannel == Peripheral_NS::LightChannel_E::NONE || stTarget.nOutputLevel == 0U)
    {
        const int nRet = turn_off_all_locked();
        if (nRet == OK)
        {
            m_stAppliedTarget = Peripheral_NS::FillLightHardwareTarget_S{};
            m_bLightOn = false;
        }
        return nRet;
    }

    /* 同通道常亮只更新占空比，避免总控上限变化造成不必要的灭灯窗口。 */
    if (!m_stAppliedTarget.bFlashing && m_stAppliedTarget.enChannel == stTarget.enChannel)
    {
        const int nRet = turn_on_channel_locked(stTarget.enChannel, stTarget.nOutputLevel);
        if (nRet == OK)
        {
            m_stAppliedTarget = stTarget;
            m_bLightOn = true;
        }
        return nRet;
    }

    /* 通道切换必须先执行互斥关断和稳定等待，防止白光、红外短时同时导通。 */
    int nRet = prepare_channel_locked();
    if (nRet == OK)
    {
        nRet = turn_on_channel_locked(stTarget.enChannel, stTarget.nOutputLevel);
    }
    if (nRet == OK)
    {
        m_stAppliedTarget = stTarget;
        m_bLightOn = true;
    }
    return nRet;
}

int CFillLightDriver::apply_flashing_target_locked(const Peripheral_NS::FillLightHardwareTarget_S &stTarget)
{
    /* 特殊局部变量区分“更新同一告警”与“切换新通道”，避免重复关灯造成闪烁窗口。 */
    const bool bUpdateActiveFlash = m_stAppliedTarget.bFlashing && m_stAppliedTarget.enChannel == stTarget.enChannel;
    if (bUpdateActiveFlash)
    {
        const Peripheral_NS::FillLightFlashFrequency_E enOldFrequency = m_stAppliedTarget.enFlashFrequency;
        /* 告警已到期时只更新下次重启参数，不能因配置刷新而重新点亮。 */
        if (m_bFlashExpired)
        {
            m_stAppliedTarget.nOutputLevel = stTarget.nOutputLevel;
            m_stAppliedTarget.enFlashFrequency = stTarget.enFlashFrequency;
            return OK;
        }
        if (stTarget.enFlashFrequency == Peripheral_NS::FillLightFlashFrequency_E::STEADY_ON)
        {
            const int nRet = turn_on_channel_locked(stTarget.enChannel, stTarget.nOutputLevel);
            if (nRet == OK)
            {
                m_stAppliedTarget.nOutputLevel = stTarget.nOutputLevel;
                m_stAppliedTarget.enFlashFrequency = stTarget.enFlashFrequency;
                m_bLightOn = true;
            }
            return nRet;
        }
        if (m_bLightOn)
        {
            const int nRet = turn_on_channel_locked(stTarget.enChannel, stTarget.nOutputLevel);
            if (nRet != OK)
            {
                return nRet;
            }
        }
        m_stAppliedTarget.nOutputLevel = stTarget.nOutputLevel;
        m_stAppliedTarget.enFlashFrequency = stTarget.enFlashFrequency;
        /* 修改频率从当前时刻重新计时，避免沿用旧节拍产生突发短脉冲。 */
        if (enOldFrequency != stTarget.enFlashFrequency)
        {
            m_stNextToggleTime = std::chrono::steady_clock::now() + get_flash_interval(stTarget.enFlashFrequency);
        }
        return OK;
    }

    /* 新告警开始前先终止任何旧通道及其状态，后续由worker负责周期切换。 */
    int nRet = prepare_channel_locked();
    if (nRet != OK)
    {
        return nRet;
    }

    if (stTarget.enFlashFrequency == Peripheral_NS::FillLightFlashFrequency_E::STEADY_ON)
    {
        nRet = turn_on_channel_locked(stTarget.enChannel, stTarget.nOutputLevel);
        if (nRet != OK)
        {
            return nRet;
        }
    }

    m_stAppliedTarget = stTarget;
    /* 使用steady_clock避免系统校时改变告警闪烁的持续时长。 */
    m_stFlashDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(stTarget.nFlashTimeSec);
    m_stNextToggleTime = std::chrono::steady_clock::now();
    m_bLightOn = stTarget.enFlashFrequency == Peripheral_NS::FillLightFlashFrequency_E::STEADY_ON;
    m_bFlashExpired = false;
    return OK;
}

int CFillLightDriver::turn_off_all_locked()
{
    /* 即使白光关断失败也继续尝试红外关断，优先消除另一通道的潜在风险。 */
    int nFirstRet = turn_off_channel_locked(Peripheral_NS::LightChannel_E::WHITE);
    if (m_stProfile.bInfraredSupported)
    {
        const int nInfraredRet = turn_off_channel_locked(Peripheral_NS::LightChannel_E::INFRARED);
        if (nFirstRet == OK)
        {
            nFirstRet = nInfraredRet;
        }
    }
    if (nFirstRet == OK)
    {
        m_stAppliedTarget = Peripheral_NS::FillLightHardwareTarget_S{};
        m_bLightOn = false;
        m_bFlashExpired = false;
    }
    return nFirstRet;
}

int CFillLightDriver::turn_off_channel_locked(Peripheral_NS::LightChannel_E enChannel)
{
    if (enChannel == Peripheral_NS::LightChannel_E::INFRARED && !m_stProfile.bInfraredSupported)
    {
        return OK;
    }

    /* 非拥有画像指针只在本调用栈使用，画像随driver生命周期保持稳定。 */
    const FillLightPwmChannelProfile_S *pProfile = get_pwm_profile(enChannel, m_stProfile);
    if (pProfile == nullptr)
    {
        return ERR_PARAM;
    }
    return set_pwm_output(*pProfile, 0U, false);
}

int CFillLightDriver::turn_on_channel_locked(Peripheral_NS::LightChannel_E enChannel, unsigned int nOutputLevel)
{
    const FillLightPwmChannelProfile_S *pProfile = get_pwm_profile(enChannel, m_stProfile);
    if (pProfile == nullptr)
    {
        return ERR_PARAM;
    }
    /* 将经总控限幅后的亮度映射为板级PWM占空比，不直接把百分比写入硬件。 */
    const unsigned int nDutyCycle = calculate_duty_cycle(*pProfile, nOutputLevel);
    return set_pwm_output(*pProfile, nDutyCycle, true);
}

int CFillLightDriver::prepare_channel_locked()
{
    /* ! 切换前先关闭两路通道，确保任何旧闪烁或未知上电状态都不会与目标通道并发导通。 */
    const int nRet = turn_off_all_locked();
    if (nRet != OK)
    {
        return nRet;
    }
    if (m_stProfile.nMutualExclusionSettleMs > 0U)
    {
        /* 等待板级要求的电气稳定时间，约束是本函数必须在设备锁内调用。 */
        std::this_thread::sleep_for(std::chrono::milliseconds(m_stProfile.nMutualExclusionSettleMs));
    }
    return OK;
}

void CFillLightDriver::flash_worker_loop()
{
    /* trace: 独立线程只操作已加锁的driver状态，名称用于现场线程诊断。 */
    pthread_setname_np(pthread_self(), "FillLightFlash");
    std::unique_lock<std::mutex> stLock(m_mtxDevice);
    while (m_bRunning)
    {
        /* 无告警目标时无限等待版本/目标变更，避免轮询占用CPU。 */
        if (!m_stAppliedTarget.bFlashing)
        {
            m_stCv.wait(stLock,
                        [this]()
                        {
                            return !m_bRunning || m_stAppliedTarget.bFlashing;
                        });
            continue;
        }

        /* 特殊局部变量记录目标版本；所有等待谓词以它判断旧任务是否已失效。 */
        const uint64_t u64Version = m_u64TargetVersion;
        if (m_bFlashExpired)
        {
            m_stCv.wait(stLock,
                        [this, u64Version]()
                        {
                            return !m_bRunning || m_u64TargetVersion != u64Version;
                        });
            continue;
        }
        /* 统一使用单调时间比较deadline和下次翻转点，避免墙钟跳变影响时序。 */
        const auto stNow = std::chrono::steady_clock::now();
        if (stNow >= m_stFlashDeadline)
        {
            /* 到期时必须物理关灯，不能只清空逻辑目标。 */
            const int nRet = turn_off_channel_locked(m_stAppliedTarget.enChannel);
            if (nRet != OK)
            {
                dlog_error("告警闪烁到期关闭补光失败: %d", nRet);
                /* ! 到期关灯失败不得标记完成，否则灯可能永久保持导通；短暂退避后继续重试。 */
                m_stCv.wait_for(stLock,
                                std::chrono::milliseconds(200),
                                [this, u64Version]()
                                {
                                    return !m_bRunning || m_u64TargetVersion != u64Version;
                                });
                continue;
            }
            m_bLightOn = false;
            m_bFlashExpired = true;
            continue;
        }

        /* STEADY_ON只等待到期；普通闪烁等待下一个亮灭边界。 */
        const auto stWakeTime = m_stAppliedTarget.enFlashFrequency == Peripheral_NS::FillLightFlashFrequency_E::STEADY_ON
                                    ? m_stFlashDeadline
                                    : m_stNextToggleTime;
        if (stWakeTime > stNow)
        {
            m_stCv.wait_until(stLock,
                              stWakeTime,
                              [this, u64Version]()
                              {
                                  return !m_bRunning || m_u64TargetVersion != u64Version;
                              });
            continue;
        }

        /* perf: 闪烁亮灭不输出常规日志，仅在PWM失败时记录一次错误上下文。 */
        int nRet = OK;
        if (m_bLightOn)
        {
            nRet = turn_off_channel_locked(m_stAppliedTarget.enChannel);
        }
        else
        {
            nRet = turn_on_channel_locked(m_stAppliedTarget.enChannel, m_stAppliedTarget.nOutputLevel);
        }
        if (nRet != OK)
        {
            dlog_error("告警闪烁PWM切换失败, channel:%d, ret:%d", static_cast<int>(m_stAppliedTarget.enChannel), nRet);
        }
        else
        {
            m_bLightOn = !m_bLightOn;
        }
        /* 无论本轮PWM是否失败都推进节拍，避免错误时形成忙循环。 */
        m_stNextToggleTime = std::chrono::steady_clock::now() + get_flash_interval(m_stAppliedTarget.enFlashFrequency);
    }
}

std::chrono::milliseconds CFillLightDriver::get_flash_interval(Peripheral_NS::FillLightFlashFrequency_E enFrequency)
{
    switch (enFrequency)
    {
    case Peripheral_NS::FillLightFlashFrequency_E::LOW_FREQ:
        return std::chrono::milliseconds(1000);
    case Peripheral_NS::FillLightFlashFrequency_E::HIGH_FREQ:
        return std::chrono::milliseconds(200);
    case Peripheral_NS::FillLightFlashFrequency_E::MID_FREQ:
    default:
        return std::chrono::milliseconds(500);
    }
}
