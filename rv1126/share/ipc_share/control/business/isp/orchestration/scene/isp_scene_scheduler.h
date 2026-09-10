/**
 * @FilePath     : isp_scene_scheduler.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:47:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-16 11:30:00
 * @Description  : 共享ISP场景计划调度器，只提交计划意图
 */

#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

#include "isp_define.h"
#include "isp_capability_profile.h"

/**
 * @brief 场景计划单次本地时间快照。
 */
struct IspSchedulerTime_S
{
    /* 本地月份，范围1-12 */
    int nMonth{1};
    /* 自当天零点开始的秒数，范围0-86399 */
    int nDaySeconds{0};
};

/**
 * @brief 场景计划时钟函数集合，供平台注入本地时间快照。
 */
struct IspSchedulerClock_S
{
    /* 单次读取月份和当天秒数，避免跨月瞬间分别取值不一致。 */
    std::function<IspSchedulerTime_S()> fnGetCurrentTime;
};

/**
 * @brief 共享ISP场景计划调度器。
 * @note  只提交/清除计划意图到arbiter，不直接调用scene provider或持久化用户场景。
 *        仅使用一个秒级循环线程按单次本地时间快照重新裁决。
 */
class CIspSceneScheduler
{
public:
    /**
     * @brief 计划意图提交回调，返回OK成功，非OK触发退避重试
     */
    using ScheduleIntentCallback = std::function<int(ISP::SceneType_E enScene, bool bActive)>;

    /**
     * @brief   : 构造调度器
     * @param    {const IspSchedulerClock_S&} stClock：时钟函数
     * @param    {const ISP::IspCapabilityProfile_S&} stProfile：能力画像
     * @param    {ScheduleIntentCallback} fnCallback：意图提交回调
     */
    CIspSceneScheduler(const IspSchedulerClock_S &stClock,
                       const ISP::IspCapabilityProfile_S &stProfile,
                       ScheduleIntentCallback fnCallback);
    ~CIspSceneScheduler();

    /**
     * @brief   : 启动调度线程
     * @return   {int} OK：成功
     */
    int start();

    /**
     * @brief   : 停止调度线程并join
     * @return   {int} OK：成功
     */
    int stop();

    /**
     * @brief   : 更新计划配置
     * @param    {const ISP::SceneSchedule_S&} stSchedule：计划配置
     * @return   {int} OK：成功，ERR_PARAM/ERR_UNSUPPORT：校验失败
     */
    int update(const ISP::SceneSchedule_S &stSchedule);

private:
    /**
     * @brief   : 工作线程主循环
     */
    void worker_loop();

    /**
     * @brief   : 按计划快照解析当前应生效的场景
     * @param    {const ISP::SceneSchedule_S&} stSchedule：本轮计划快照
     * @param    {ISP::SceneType_E&} enScene：输出场景
     * @return   {bool} true：有命中，false：无命中或禁用
     */
    bool resolve_current_scene(const ISP::SceneSchedule_S &stSchedule, ISP::SceneType_E &enScene) const;

    /* 时钟函数 */
    IspSchedulerClock_S m_stClock;
    /* 能力画像 */
    ISP::IspCapabilityProfile_S m_stProfile;
    /* 意图提交回调 */
    ScheduleIntentCallback m_fnCallback;
    /* 工作线程 */
    std::thread m_stThread;
    /* lock: 保护计划配置，worker每轮仅持锁复制快照。 */
    std::mutex m_mtx;
    /* 是否运行中 */
    std::atomic<bool> m_bRunning;
    /* 当前计划配置 */
    ISP::SceneSchedule_S m_stSchedule;
    /* 上次提交的场景 */
    ISP::SceneType_E m_enLastSubmittedScene;
    /* 上次提交是否活跃 */
    bool m_bLastSubmittedActive;
    /* perf: 计划只需秒级精度；固定周期同时覆盖手动校时、跨日和跨月。 */
    static constexpr int SCHEDULE_CHECK_INTERVAL_SEC = 1;
};
