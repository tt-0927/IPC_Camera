/**
 * @FilePath     : isp_transition_progress.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:36:24
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-15 17:52:00
 * @Description  : ISP事务执行进度和错误
 */

#pragma once

#include <cstdint>

namespace ISP
{

/**
 * @brief 事务执行进度，由reconciler维护。
 */
struct IspTransitionProgress_S
{
    /* 最后已处理完成的generation，成功或失败都会更新 */
    uint64_t u64Generation;
    /* 最后一次错误码，OK表示无错误 */
    int nLastErrorCode;
    /* 内部运行场景是否已应用 */
    bool bRuntimeSceneApplied;
    /* IR-CUT是否已切换 */
    bool bIrCutSwitched;
    /* 灯光是否已应用 */
    bool bLightApplied;

    IspTransitionProgress_S()
        : u64Generation(0), nLastErrorCode(0),
          bRuntimeSceneApplied(false), bIrCutSwitched(false), bLightApplied(false) {}
};

} // namespace ISP
