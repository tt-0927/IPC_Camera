/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2026-10-19 15:57:27
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-11-05 10:28:16
 * @FilePath: /TV-3881TJY/rv1126b_ipc/main_app/ai_app/detect_mode/classroom/perception/pose_admit.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#pragma once

#include <cstdint>

#include "classroom_define.h"

namespace Classroom
{
/**
 * @brief 准入判据的跨拍状态（poseWantNum 原有的三个成员，搬进来集中）
 */
typedef struct _PoseAdmitState_
{
    int64_t llWindowBeginMs = 0;   /* 配额窗口起点（单调毫秒） */
    int32_t nInWindow = 0;         /* 本窗口已做人次（含失败的尝试，调用方累加） */
    bool    bThrottled = false;    /* NPU 忙迟滞闸的当前态 */
} SPoseAdmitState_S;

/**
 * @brief 一次准入判据的输入（全部值拷贝，本层不碰调度器与配置对象）
 */
typedef struct _PoseAdmitIn_
{
    int32_t nRowNum = 0;           /* 本拍特征行数；<=0 = 屏上没人，整段让路 */
    int32_t nBudgetMs = 0;         /* 一拍给姿态的毫秒预算；<=0 = 现场整段关掉采样 */
    int32_t nWindowMs = 0;         /* pose_interval_ms 原值（<=0 由本层回退到 TTL 下限） */
    int32_t nMaxPersons = 0;       /* 一个窗口最多做几人；<=0 = 同上整段关 */
    int32_t nBusyHighPct = 0;      /* NPU 忙迟滞：越过即停 */
    int32_t nBusyLowPct = 0;       /* NPU 忙迟滞：回到此线才恢复 */
    double  dBusyPct = -1.0;       /* 调度器读数；<0 = 本拍没有调度器可问（跳过迟滞） */
    double  dCostMs = 0.0;         /* 单次实测耗时（EWMA）；<1.0 = 没有样本 */
    int64_t llNowMs = 0;           /* 本拍时刻（单调毫秒） */
} SPoseAdmitIn_S;

/**
 * @brief 一次准入判据的输出
 * @note  nLeftInWindow 只在「真走到配额分配」那一步才有数：忙让路/配额耗尽/整段关的
 *        返回里它是 0——与搬出前的行为逐行同义（心跳的 nLastPoseLeft 同口径）
 */
typedef struct _PoseAdmitOut_
{
    int32_t nWant = 0;             /* 本拍允许提交的人数（0 = 整段让路） */
    int32_t nLeftInWindow = 0;     /* 分配完本拍后窗口还剩的配额 */
    bool    bBusySkip = false;     /* 本拍因 NPU 忙让路（调用方记 u64PoseBusySkip 并打日志） */
} SPoseAdmitOut_S;

/**
 * @brief   : 姿态段本拍准入判据
 * @param    {SPoseAdmitState_S&} rstState：调用方持有的跨拍状态（窗口/迟滞），本函数原地更新
 * @param    {const SPoseAdmitIn_S&} rstIn：本拍的配置、读数与时刻
 * @param    {SPoseAdmitOut_S&} rstOut：OUT=判定结果
 * @note    : 「整段关」与「忙让路」都不动状态里的窗口账——关着的时候窗口起点跟着漂移
 *            会让重新打开后的第一拍白滚一次窗口（行为与搬出前一致，刻意保留）
 */
void poseAdmit(SPoseAdmitState_S &rstState, const SPoseAdmitIn_S &rstIn, SPoseAdmitOut_S &rstOut);
}
