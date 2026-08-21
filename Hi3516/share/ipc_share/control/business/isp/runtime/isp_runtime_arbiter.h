/**
 * @FilePath     : isp_runtime_arbiter.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:36:24
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 09:46:56
 * @Description  : ISP设置优先级选择器
 */

#pragma once

#include <cstdint>
#include <mutex>

#include "isp_runtime_intent.h"
#include "isp_runtime_target.h"

/**
 * @brief ISP设置优先级选择器。
 * @note  网页场景和日夜场景分别选择。计划场景优先于用户场景；临时灯光请求仍受外设总控限制。
 */
class CIspRuntimeArbiter
{
public:
    CIspRuntimeArbiter();

    /**
     * @brief   : 更新用户选择的场景
     * @param    {ISP::SceneType_E} enScene：用户选择的场景
     */
    void update_user_scene(ISP::SceneType_E enScene);

    /**
     * @brief   : 更新计划选择的场景
     * @param    {ISP::SceneType_E} enScene：计划命中的场景
     * @param    {bool} bActive：是否活跃
     */
    void update_schedule_scene(ISP::SceneType_E enScene, bool bActive);

    /**
     * @brief   : 清除计划选择的场景
     */
    void clear_schedule_scene();

    /**
     * @brief   : 读取计划选择的场景
     * @param    {ISP::SceneType_E&} enScene：计划场景输出
     * @param    {bool&} bActive：计划是否活跃
     * @return   {void}
     */
    void get_schedule_scene(ISP::SceneType_E &enScene, bool &bActive) const;

    /**
     * @brief   : 更新日夜切换请求
     * @param    {const ISP::IspDayNightIntent_S&} stIntent：日夜设置
     */
    void update_daynight(const ISP::IspDayNightIntent_S &stIntent);

    /**
     * @brief   : 清除日夜切换请求
     */
    void clear_daynight();

    /**
     * @brief   : 更新补光总控设置
     * @param    {const Peripheral_NS::FillLightGateState_S&} stGate：最新准入与物理功率上限
     * @return   {void}
     */
    void update_fill_light_gate(const Peripheral_NS::FillLightGateState_S &stGate);

    /**
     * @brief   : 开始临时灯光抢占
     * @param    {const ISP::IspLightOverride_S&} stOverride：抢占配置（不含token，由arbiter分配）
     * @param    {uint64_t&} u64Token：输出抢占token，失败时为0
     * @return   {int} OK：成功，ERR_NOT_ENABLED：gate禁止，ERR：已有抢占活跃
     */
    int begin_light_override(const ISP::IspLightOverride_S &stOverride, uint64_t &u64Token);

    /**
     * @brief   : 结束临时灯光抢占
     * @param    {uint64_t} u64Token：抢占token
     * @return   {bool} true：成功清除，false：token不匹配
     */
    bool end_light_override(uint64_t u64Token);

    /**
     * @brief   : 查询指定临时灯光token是否仍处于活跃状态
     * @param    {uint64_t} u64Token：抢占token
     * @return   {bool} true：仍活跃，false：已结束、被gate撤销或token不匹配
     */
    bool is_light_override_active(uint64_t u64Token) const;

    /**
     * @brief   : 清除过期抢占
     * @param    {int64_t} nCurrentTimeMs：当前单调时钟时间(ms)
     * @return   {bool} true：清除了过期抢占
     */
    bool clear_expired_overrides(int64_t nCurrentTimeMs);

    /**
     * @brief   : 获取下一次要应用的设置
     * @return   {ISP::IspRuntimeTarget_S} 待应用设置
     */
    ISP::IspRuntimeTarget_S get_current_target() const;

private:
    /**
     * @brief   : 按优先级计算待应用设置（调用方已加锁）
     */
    void resolve_target_locked();

    /* lock: 保护所有请求和待应用设置。 */
    mutable std::mutex m_mtx;
    /* 用户选择的场景 */
    ISP::IspUserSceneIntent_S m_stUserScene;
    /* 计划选择的场景 */
    ISP::IspScheduleSceneIntent_S m_stSchedule;
    /* 日夜切换请求 */
    ISP::IspDayNightIntent_S m_stDayNight;
    /* 补光总控设置 */
    ISP::IspFillLightGateIntent_S m_stFillLightGate;
    /* 临时灯光抢占 */
    ISP::IspLightOverride_S m_stOverride;
    /* 下一次要应用的设置 */
    ISP::IspRuntimeTarget_S m_stCurrentTarget;
    /* 下一个token */
    uint64_t m_u64NextToken;
};
