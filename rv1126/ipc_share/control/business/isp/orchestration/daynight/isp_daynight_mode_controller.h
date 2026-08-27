/**
 * @FilePath     : isp_daynight_mode_controller.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:50:59
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : 共享ISP日夜模式控制器
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>

#include "isp_define.h"
#include "isp_capability_profile.h"
#include "isp_platform_adapters.h"
#include "isp_runtime_intent.h"

/**
 * @brief 日夜控制器时钟函数集合。
 */
struct IspDayNightClock_S
{
    /* api: 返回当天已过秒数，TIME 模式据此判断日夜区间；调用方必须提供线程安全、无副作用的时钟实现。 */
    std::function<int()> fnGetDaySeconds;
};

/**
 * @brief 共享ISP日夜模式控制器。
 * @note  DAY/NIGHT立即提交，TIME使用共享时间policy，AUTO启动detector并执行filter time。
 *        只提交IspDayNightIntent_S到arbiter，不直接操作scene/IR-CUT/light。
 */
class CIspDayNightModeController
{
public:
    /**
     * @brief 日夜设置请求提交回调
     */
    using DayNightIntentCallback = std::function<int(const ISP::IspDayNightIntent_S &, ISP::IspDayNightObservationContext_S &)>;

    /**
     * @brief   : 构造日夜模式控制器
     * @param    {IIspDayNightDetector&} stDetector：平台环境光检测端口
     * @param    {const IspDayNightClock_S&} stClock：时间模式所需时钟
     * @param    {const ISP::IspCapabilityProfile_S&} stProfile：补光能力约束
     * @param    {DayNightIntentCallback} fnCallback：向优先级选择器提交设置请求的回调
     * @return   {void}
     */
    CIspDayNightModeController(IIspDayNightDetector &stDetector,
                               const IspDayNightClock_S &stClock,
                               const ISP::IspCapabilityProfile_S &stProfile,
                               DayNightIntentCallback fnCallback);
    /**
     * @brief   : 停止可能仍在运行的检测器和定时线程。
     * @return   {void}
     */
    ~CIspDayNightModeController();

    /**
     * @brief   : 使用启动时配置建立当前日夜设置请求。
     * @param    {const ISP::DayNightAttr_S&} stConfig：已持久化配置
     * @return   {int} OK：成功，非OK：检测器或回调失败
     */
    int init(const ISP::DayNightAttr_S &stConfig);
    /**
     * @brief   : 切换日夜策略并使旧检测/定时回调失效。
     * @param    {const ISP::DayNightAttr_S&} stConfig：新的已校验配置
     * @return   {int} OK：成功，非OK：失败
     */
    int update_config(const ISP::DayNightAttr_S &stConfig);
    /**
     * @brief   : 停止控制器拥有的后台活动。
     * @return   {int} OK：成功，非OK：失败
     */
    int deinit();

private:
    /**
     * @brief   : 热更新AUTO参数并保持当前已接受的日夜状态
     * @param    {const ISP::DayNightAttr_S&} stConfig：新的已校验配置
     * @return   {int} OK：成功，非OK：检测器或请求提交失败
     * @note    : 灵敏度变化会重新建立检测候选；过滤时间变化保留候选首次成立时间。
     */
    int update_auto_config(const ISP::DayNightAttr_S &stConfig);
    /**
     * @brief   : 依据配置模式启动 AUTO、TIME 或固定日夜策略
     * @return   {int} OK：成功，非OK：策略、回调或 detector 启动失败
     */
    int apply_current_mode();
    /**
     * @brief   : 下发灵敏度、注册当前配置代次回调并启动AUTO检测器
     * @param    {const ISP::DayNightAttr_S&} stConfig：AUTO配置快照
     * @param    {uint32_t} u32Epoch：回调所属配置代次
     * @return   {int} OK：成功，非OK：检测器配置或启动失败
     */
    int start_auto_detector(const ISP::DayNightAttr_S &stConfig, uint32_t u32Epoch);
    /**
     * @brief   : 不提交硬件设置，只将最后成功状态重新同步给检测器
     * @return   {int} OK：成功，非OK：策略或检测器同步失败
     */
    int sync_detector_current_state();
    /**
     * @brief   : 生成包含场景、IR-CUT 与补光设置的日夜请求。
     * @param    {bool} bIsNight：目标是否为夜间
     * @param    {uint32_t} u32Epoch：产生该请求的配置更新序号
     * @return   {int} OK：成功，非OK：策略或请求提交失败
     */
    int submit_intent(bool bIsNight, uint32_t u32Epoch);
    /**
     * @brief   : 启动统一的时间边界与滤波等待工作线程
     * @return   {int} OK：已运行或启动成功，ERR：线程创建失败
     */
    int start_worker();
    /**
     * @brief   : 停止并回收统一工作线程
     * @return   {void}
     */
    void stop_worker();
    /**
     * @brief   : 轮询 TIME 模式边界或 AUTO 滤波截止时间，并提交有效请求
     * @return   {void}
     * @note    : Hi3516运行库的条件变量绝对超时不可靠，因此使用固定周期轮询。
     */
    void worker_loop();
    /**
     * @brief   : 停止已启动的环境光检测器
     * @return   {int} OK：已停止或未运行，非OK：detector 停止失败
     */
    int stop_detector();
    /**
     * @brief   : 过滤检测器抖动后提交观测结果。
     * @param    {uint32_t} u32Epoch：配置代次
     * @param    {bool} bSuggestedNight：检测建议
     * @return   {void}
     */
    void on_detector_observation(uint32_t u32Epoch, bool bSuggestedNight);
    /* memory: 平台检测器由业务服务持有；控制器只在 init 到 deinit 期间借用，检测器必须晚于控制器销毁。 */
    IIspDayNightDetector &m_rstDetector;
    /* TIME 模式的时钟回调副本；不捕获控制器状态，避免工作线程访问已释放对象。 */
    IspDayNightClock_S m_stClock;
    /* 构造时复制的平台功能和参数范围；日夜策略据此选择允许的日夜场景、IR-CUT 和灯光类型。 */
    ISP::IspCapabilityProfile_S m_stProfile;
    /* 将日夜请求提交给优先级选择器的回调；调用时不持有 m_mtx，避免业务回调重入死锁。 */
    DayNightIntentCallback m_fnCallback;

    /* lock: 串行化初始化、配置切换和去初始化，避免同时操作同一工作线程。 */
    std::mutex m_mtxLifecycle;
    /* lock: 串行执行“校验配置更新序号→提交请求”和配置失效，禁止旧请求在新配置后进入选择器。 */
    std::mutex m_mtxIntentSubmit;
    /* lock: 保护配置、初始化状态、检测器状态、worker状态和待处理的滤波请求；回调不在持锁状态下调用。 */
    std::mutex m_mtx;
    /* memory: 控制器独占的统一工作线程；deinit 必须 join 后才能释放其读取的成员状态。 */
    std::thread m_stWorkerThread;
    /* lock: 受 m_mtx 保护的总开关；置为 false 后检测器回调和工作线程都不能再提交请求。 */
    bool m_bRunning;
    /* lock: 受 m_mtx 保护；模式、创建停止阶段或观测条件变化时递增，丢弃已经不适用的检测、TIME 和滤波请求。 */
    uint32_t m_u32Epoch;

    /* lock: 受 m_mtx 保护的已校验日夜配置副本；所有策略判断均基于同一份配置。 */
    ISP::DayNightAttr_S m_stConfig;
    /* lock: 受 m_mtx 保护；标记 init 已成功建立初始请求，禁止未初始化时热更新或提交观测。 */
    bool m_bInitialized;
    /* lock: 受 m_mtx 保护；记录 detector 是否已成功启动，避免重复调用平台 stop。 */
    bool m_bDetectorRunning;
    /* lock: 受 m_mtx 保护；worker 已创建且未退出，不能用 std::thread::joinable() 推断其仍在执行。 */
    bool m_bWorkerRunning;
    /* lock: 受 m_mtx 保护的最后成功日夜状态；TIME 边界和 AUTO 观测均以它作为去重基线。 */
    bool m_bLastIsNight;
    /* lock: 受 m_mtx 保护；表示 AUTO 观测已产生与当前状态相反、尚待过滤的候选。 */
    bool m_bPendingNight;
    /* lock: 受 m_mtx 保护；保存 AUTO 模式等待确认的日夜结果，仅在 m_bPendingNight 为 true 时有效。 */
    bool m_bPendingSuggestedNight;
    /* lock: 受 m_mtx 保护；最后一次硬件成功状态由提交回调返回，供检测器重启后作为观测基准。 */
    ISP::IspDayNightObservationContext_S m_stLastObservationContext;
    /* lock: 受 m_mtx 保护；记录候选首次连续成立时间，使过滤时间热更新不会重新开始计时。 */
    std::chrono::steady_clock::time_point m_stFilterStartTime;
    /* lock: 受 m_mtx 保护；detector 回调更新滤波截止时间，worker 最多在 200ms 后读取并消费。 */
    std::chrono::steady_clock::time_point m_stFilterDeadline;
};
