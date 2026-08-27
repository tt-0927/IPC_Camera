/**
 * @FilePath     : fill_light_driver.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:56:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-18 09:28:44
 * @Description  : 摄像机白光红外补光统一设备驱动声明
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include "fill_light_hardware_target.h"
#include "fill_light_profile.h"

/**
 * @brief 摄像机白光、红外及告警闪烁唯一设备写入口。
 * @note  调用方传入的亮度必须已经过外设一级总控限幅，本类不读取业务配置。
 */
class CFillLightDriver
{
public:
    /**
     * @brief   : 构造补光灯驱动
     * @param    {const FillLightProfile_S&} stProfile：稳定设备画像
     * @return   {void}
     */
    explicit CFillLightDriver(const FillLightProfile_S &stProfile);
    /**
     * @brief   : 停止闪烁线程并安全关闭全部补光通道
     * @return   {void}
     */
    ~CFillLightDriver();

    CFillLightDriver(const CFillLightDriver &) = delete;
    CFillLightDriver &operator=(const CFillLightDriver &) = delete;

    /**
     * @brief   : 启动闪烁执行线程并建立安全关闭基线
     * @return   {int} OK：成功，非OK：硬件关闭失败
     */
    int init();
    /**
     * @brief   : 停止执行线程并关闭全部补光通道
     * @return   {int} OK：成功，非OK：硬件关闭失败
     */
    int deinit();
    /**
     * @brief   : 应用唯一补光硬件目标
     * @param    {const Peripheral_NS::FillLightHardwareTarget_S&} stTarget：最终通道、亮度和闪烁目标
     * @return   {int} OK：成功，ERR_PARAM：目标非法，其他：PWM失败
     */
    int apply_target(const Peripheral_NS::FillLightHardwareTarget_S &stTarget);
    /**
     * @brief   : 关闭指定物理补光通道
     * @param    {Peripheral_NS::LightChannel_E} enChannel：目标通道
     * @return   {int} OK：成功，ERR_PARAM：通道非法，其他：PWM失败
     */
    int turn_off(Peripheral_NS::LightChannel_E enChannel);

private:
    /**
     * @brief   : 校验最终硬件目标
     * @param    {const Peripheral_NS::FillLightHardwareTarget_S&} stTarget：待校验目标
     * @return   {int} OK：合法，ERR_PARAM/ERR_UNSUPPORT：非法
     */
    int validate_target(const Peripheral_NS::FillLightHardwareTarget_S &stTarget) const;
    /**
     * @brief   : 在设备锁保护下应用非闪烁目标
     * @param    {const Peripheral_NS::FillLightHardwareTarget_S&} stTarget：最终常亮或关闭目标
     * @return   {int} OK：成功，非OK：PWM失败
     */
    int apply_steady_target_locked(const Peripheral_NS::FillLightHardwareTarget_S &stTarget);
    /**
     * @brief   : 在设备锁保护下建立或更新闪烁目标
     * @param    {const Peripheral_NS::FillLightHardwareTarget_S&} stTarget：最终闪烁目标
     * @return   {int} OK：成功，非OK：PWM失败
     */
    int apply_flashing_target_locked(const Peripheral_NS::FillLightHardwareTarget_S &stTarget);
    /**
     * @brief   : 关闭全部受支持通道
     * @return   {int} OK：成功，非OK：首个PWM错误
     */
    int turn_off_all_locked();
    /**
     * @brief   : 关闭单一通道
     * @param    {Peripheral_NS::LightChannel_E} enChannel：待关闭物理通道
     * @return   {int} OK：成功，非OK：PWM失败
     */
    int turn_off_channel_locked(Peripheral_NS::LightChannel_E enChannel);
    /**
     * @brief   : 按最终亮度打开单一通道
     * @param    {Peripheral_NS::LightChannel_E} enChannel：待打开物理通道
     * @param    {unsigned int} nOutputLevel：经过总控限幅的最终亮度
     * @return   {int} OK：成功，非OK：PWM失败
     */
    int turn_on_channel_locked(Peripheral_NS::LightChannel_E enChannel, unsigned int nOutputLevel);
    /**
     * @brief   : 执行互斥通道关闭和稳定等待
     * @return   {int} OK：成功，非OK：PWM失败
     */
    int prepare_channel_locked();
    /**
     * @brief   : 闪烁线程主循环
     * @return   {void}
     */
    void flash_worker_loop();
    /**
     * @brief   : 获取闪烁亮灭间隔
     * @param    {Peripheral_NS::FillLightFlashFrequency_E} enFrequency：物理闪烁频率配置
     * @return   {std::chrono::milliseconds} 闪烁间隔
     */
    static std::chrono::milliseconds get_flash_interval(Peripheral_NS::FillLightFlashFrequency_E enFrequency);

    /* 稳定板级画像副本；初始化后不可变，避免硬件线程读取动态配置。 */
    FillLightProfile_S m_stProfile;
    /* lock: 保护线程生命周期、当前目标和全部PWM写入，保证白光红外互斥。 */
    std::mutex m_mtxDevice;
    /* 目标更新、停机和闪烁deadline到达时唤醒worker。 */
    std::condition_variable m_stCv;
    /* memory: 唯一闪烁执行线程；deinit必须join后才能销毁本对象。 */
    std::thread m_stFlashWorker;
    /* worker运行标志；false表示停止等待并退出。 */
    bool m_bRunning;
    /* 硬件安全基线和worker是否已建立。 */
    bool m_bInitialized;
    /* 当前物理通道是否已实际导通，决定下一次闪烁动作。 */
    bool m_bLightOn;
    /* 当前闪烁目标是否已到期，保留目标身份以防gate更新把同一告警重新计时。 */
    bool m_bFlashExpired;
    /* 目标取消版本；worker等待后必须用它识别过期闪烁任务。 */
    uint64_t m_u64TargetVersion;
    /* 已提交到硬件或由worker继续执行的唯一补光目标。 */
    Peripheral_NS::FillLightHardwareTarget_S m_stAppliedTarget;
    /* 告警闪烁到期时刻，使用单调时钟避免校时干扰。 */
    std::chrono::steady_clock::time_point m_stFlashDeadline;
    /* 下一次亮灭切换时刻，频率调整后从当前时刻重新计算。 */
    std::chrono::steady_clock::time_point m_stNextToggleTime;
};
