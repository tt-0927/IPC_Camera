/**
 * @FilePath     : isp_runtime_decision.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:19:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : ISP日夜模式选择结果
 */

#pragma once

#include "isp_define.h"
#include "isp_runtime_scene.h"

namespace ISP
{

/**
 * @brief ISP日夜运行状态，用于表达初始化、切换中和错误等非布尔状态。
 */
enum class IspDayNightRuntimeState_E
{
    UNINITIALIZED,      /* 尚未初始化，不能切换日夜硬件 */
    DAY,                /* 当前处于白天模式 */
    NIGHT,              /* 当前处于夜间模式 */
    SWITCHING_TO_DAY,   /* 正在从夜间切换到白天模式 */
    SWITCHING_TO_NIGHT, /* 正在从白天切换到夜间模式 */
    ERROR,              /* 日夜状态机发生错误，需要上层恢复或重新初始化 */
};

/**
 * @brief IR-CUT目标方向。
 */
enum class IspIrCutTarget_E
{
    NONE,  /* 不需要执行IR-CUT切换 */
    DAY,   /* IR-CUT切换到白天滤光片状态 */
    NIGHT, /* IR-CUT切换到夜间透红外状态 */
};

/**
 * @brief 智能补光的第二次选择结果；当前固定返回 USE_RED，后续可改为动态选择。
 */
enum class IspSmartLightDecision_E
{
    USE_RED,      /* 智能补光选择红外灯 */
    USE_WHITE,    /* 智能补光选择白光灯 */
    KEEP_CURRENT, /* 保持当前补光配置，不触发额外切换 */
};

/**
 * @brief ISP日夜模式选择结果。
 */
struct IspRuntimeDecision_S
{
    /* 日夜状态机当前运行状态 */
    IspDayNightRuntimeState_E enRuntimeState;
    /* 要使用的内部场景，决定参数和补光。 */
    IspRuntimeScene_E enRuntimeScene;
    /* 最终允许使用的灯光类型 */
    LightType_E enEffectiveLightType;
    /* IR-CUT硬件切换目标 */
    IspIrCutTarget_E enIrCutTarget;
    /* 当前是否判定为夜间状态 */
    bool bIsNight;
    /* 是否需要执行IR-CUT切换 */
    bool bNeedIrCutSwitch;
    /* 是否需要同步补光状态 */
    bool bNeedFillLightSync;

    /**
     * @brief   : 构造默认日夜选择结果
     * @return  : 无
     * @note    : 默认保持未初始化状态，不触发IR-CUT和补光同步动作
     */
    IspRuntimeDecision_S()
        : enRuntimeState(IspDayNightRuntimeState_E::UNINITIALIZED), enRuntimeScene(IspRuntimeScene_E::DAY),
          enEffectiveLightType(LIGHT_TYPE_CLOSE), enIrCutTarget(IspIrCutTarget_E::NONE), bIsNight(false), bNeedIrCutSwitch(false),
          bNeedFillLightSync(false)
    {
    }
};

/**
 * @brief 自动日夜检测使用的最后一次成功结果。
 */
struct IspDayNightObservationContext_S
{
    /* 最后成功目标是否为夜间。 */
    bool bIsNight;
    /* 最后成功应用的内部日夜场景，用于选择夜转日阈值。 */
    IspRuntimeScene_E enRuntimeScene;
    /* 日夜策略在总开关前选出的灯型；SMART 当前已转换为红外通道。 */
    LightType_E enRequestedLightType;
    /* 经过外设总开关后实际下发的灯型；禁灯时为 CLOSE。 */
    LightType_E enActualLightType;

    IspDayNightObservationContext_S()
        : bIsNight(false), enRuntimeScene(IspRuntimeScene_E::DAY), enRequestedLightType(LIGHT_TYPE_CLOSE),
          enActualLightType(LIGHT_TYPE_CLOSE)
    {
    }
};

} // namespace ISP
