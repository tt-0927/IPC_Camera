/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2026-11-04 09:34:05
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-11-05 10:27:53
 * @FilePath: /TV-3881TJY/rv1126b_ipc/main_app/ai_app/detect_mode/classroom/perception/pose_admit.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @FilePath     : pose_admit.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-19 14:26:24
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 17:26:24
 * @Description  : 姿态段准入判据的实现
 */

#include "pose_admit.h"
#include "feature_frame.h"

namespace Classroom
{
void poseAdmit(SPoseAdmitState_S &rstState, const SPoseAdmitIn_S &rstIn, SPoseAdmitOut_S &rstOut)
{
    rstOut.nWant = 0;
    rstOut.nLeftInWindow = 0;
    rstOut.bBusySkip = false;

    if (rstIn.nRowNum <= 0 || rstIn.nBudgetMs <= 0 || rstIn.nMaxPersons <= 0)
    {
        /* 配成 0 = 现场要把姿态整段关掉：关的是采样，下游随之落 UNKNOWN，
           不是把功能开着却悄悄不判。状态原地不动（含窗口账） */
        return;
    }

    /* ---- 第一级：本周期配额。窗口按 pose_interval_ms 滚动，与规则层的新鲜度同源 ---- */
    const int64_t llWindowMs = (rstIn.nWindowMs > 0) ? rstIn.nWindowMs
                                                     : CLASSROOM_POSE_TTL_MIN_MS;
    if (0 == rstState.nInWindow || rstIn.llNowMs - rstState.llWindowBeginMs >= llWindowMs)
    {
        rstState.llWindowBeginMs = rstIn.llNowMs;
        rstState.nInWindow = 0;
    }
    const int32_t nLeft = rstIn.nMaxPersons - rstState.nInWindow;
    if (nLeft <= 0)
    {
        return;    /* 配额用完：本拍整段让路，等窗口滚动（最多一个 pose_interval_ms） */
    }

    /* ---- 第二级：NPU 忙不忙。越过高线停、回到低线才恢复（迟滞，防反复启停） ---- */
    if (rstIn.dBusyPct >= 0.0)
    {
        if (!rstState.bThrottled && rstIn.dBusyPct >= static_cast<double>(rstIn.nBusyHighPct))
        {
            rstState.bThrottled = true;
        }
        else if (rstState.bThrottled && rstIn.dBusyPct <= static_cast<double>(rstIn.nBusyLowPct))
        {
            rstState.bThrottled = false;
        }

        if (rstState.bThrottled)
        {
            rstOut.bBusySkip = true;
            return;
        }
    }

    /* ---- 第三级：一拍能做几个人 = 预算 ÷ 实测单次耗时（EWMA，调度器按模型键统计） ---- */
    int32_t nByBudget = 1;
    if (rstIn.dCostMs >= 1.0)
    {
        nByBudget = static_cast<int32_t>(static_cast<double>(rstIn.nBudgetMs) / rstIn.dCostMs);
        if (nByBudget < 1)
        {
            nByBudget = 1;
        }
    }

    rstOut.nWant = (nByBudget < nLeft) ? nByBudget : nLeft;
    rstOut.nLeftInWindow = nLeft - rstOut.nWant;
}
}
