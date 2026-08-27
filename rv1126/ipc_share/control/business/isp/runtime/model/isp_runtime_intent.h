/**
 * @FilePath     : isp_runtime_intent.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:36:24
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 09:46:56
 * @Description  : ISP设置请求模型
 */

#pragma once

#include <cstdint>
#include "isp_define.h"
#include "isp_runtime_decision.h"
#include "isp_light_target.h"
#include "fill_light_gate_state.h"

namespace ISP
{

/**
 * @brief 用户选择的网页场景。
 */
struct IspUserSceneIntent_S
{
    /* 用户选择的网页配置场景 */
    SceneType_E enConfigScene;
    /* 是否活跃 */
    bool bActive;

    IspUserSceneIntent_S() : enConfigScene(SCENE_NORMAL), bActive(false)
    {
    }
};

/**
 * @brief 计划选择的场景；启用后覆盖用户场景。
 */
struct IspScheduleSceneIntent_S
{
    /* 计划命中的网页配置场景 */
    SceneType_E enConfigScene;
    /* 是否活跃 */
    bool bActive;

    IspScheduleSceneIntent_S() : enConfigScene(SCENE_NORMAL), bActive(false)
    {
    }
};

/**
 * @brief 日夜设置，包含内部场景、IR-CUT 和正常灯光，不修改网页场景。
 */
struct IspDayNightIntent_S
{
    /* 当前是否夜间 */
    bool bIsNight;
    /* 日夜模式选出的内部场景 */
    IspRuntimeScene_E enRuntimeScene;
    /* IR-CUT目标 */
    IspIrCutTarget_E enIrCutTarget;
    /* 正常灯光目标 */
    IspLightTarget_S stLight;
    /* 是否活跃 */
    bool bActive;

    IspDayNightIntent_S()
        : bIsNight(false), enRuntimeScene(IspRuntimeScene_E::DAY), enIrCutTarget(IspIrCutTarget_E::NONE), bActive(false)
    {
    }
};

/**
 * @brief 外设补光总控设置。
 * @note  只表达灯光准入和物理功率上限，不选择白光或红外，不改变IR-CUT。
 */
struct IspFillLightGateIntent_S
{
    /* 外设补光一级总控状态 */
    Peripheral_NS::FillLightGateState_S stGate;
    /* 是否活跃 */
    bool bActive;

    IspFillLightGateIntent_S() : bActive(false)
    {
    }
};

/**
 * @brief 临时灯光请求（只改灯光，带令牌和截止时间）。
 */
struct IspLightOverride_S
{
    /* 唯一标识，0表示无效 */
    uint64_t u64Token;
    /* 灯光目标 */
    IspLightTarget_S stLight;
    /* 单调时钟截止时间(ms)，0表示无期限 */
    int64_t nDeadlineMs;
    /* 是否活跃 */
    bool bActive;

    IspLightOverride_S() : u64Token(0), nDeadlineMs(0), bActive(false)
    {
    }
};

} // namespace ISP
