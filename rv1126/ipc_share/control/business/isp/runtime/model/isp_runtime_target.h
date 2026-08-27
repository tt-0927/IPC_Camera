/**
 * @FilePath     : isp_runtime_target.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:36:24
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 09:46:56
 * @Description  : ISP待应用硬件设置
 */

#pragma once

#include <cstdint>
#include "isp_define.h"
#include "isp_runtime_decision.h"
#include "isp_light_target.h"

namespace ISP
{

/**
 * @brief 下一次要应用的硬件设置。
 */
struct IspRuntimeTarget_S
{
    /* 设置版本号；每次有效修改加一。 */
    uint64_t u64Generation;
    /* 当前生效的网页配置场景，用于选择参数槽位 */
    SceneType_E enConfigScene;
    /* 日夜模式选出的内部场景，用于选择 ISP 参数。 */
    IspRuntimeScene_E enRuntimeScene;
    /* IR-CUT目标 */
    IspIrCutTarget_E enIrCutTarget;
    /* 灯光目标 */
    IspLightTarget_S stLight;

    IspRuntimeTarget_S()
        : u64Generation(0), enConfigScene(SCENE_MAX), enRuntimeScene(IspRuntimeScene_E::DAY),
          enIrCutTarget(IspIrCutTarget_E::NONE)
    {
    }
};

} // namespace ISP
