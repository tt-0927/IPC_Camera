/**
 * @FilePath     : classroom_warm_policy.h
 * @Author       : leiyy
 * @Date         : 2026-10-17 16:30:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-17 16:30:00
 * @Description  : 预热（模型驻留）那几条判据的纯函数层

 */

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "classroom_define.h"
#include "classroom_config.h"

namespace Classroom
{
/* 预热重试周期 */
#define CLASSROOM_WARM_RETRY_MS (2000)

/* 轮转武装多久之后，关一次、按原始目标向池子重问两组现在还装不装得下（复议窗口*/
#define CLASSROOM_ROTATE_RECONSIDER_MS (60000)

/* 单组时间片在配置层允许的上界 */
#define CLASSROOM_WARM_SLICE_MAX_MS (25000)

/* 动态轮转周期的 NPU 占用分档 */
#define CLASSROOM_SLICE_BUSY_HI_PCT (80.0)
#define CLASSROOM_SLICE_BUSY_LO_PCT (30.0)
#define CLASSROOM_SLICE_BUSY_LO_MS (10000)

/* 复核名单空了之后，G_OTHER 还替它保留多久（毫秒） */
#define CLASSROOM_PHONE_WARM_HOLD_MS (15000)

/* 板书 OCR 那两颗OcrDet + OcrRec 的保留期余量 */
#define CLASSROOM_BOARD_WARM_SLACK_MS (5000)

/* 两个重资源组（G_POSE/G_FACE）同时开启才有轮转问题，位运算是掩码级的判据，供池子与引擎用同一把尺 */
uint32_t warmHeavyMask(void);

/**
 * @brief   : 原始需求里两个重档组是不是都还要（轮转还有没有对象）
 * @return   {bool} false = 只剩一组要重资源了，轮转该解除、把机会交回给按原始目标走一次
 */
bool warmRotateHasTarget(const CConfigSnapshot_S &stSnap);

/**
 * @brief   : 一片窗口里的 NPU 占用
 * @param    {int64_t} llSeenBusyMs：本片攒下的忙毫秒增量（引擎逐拍从调度器累计量里攒，
 *            对 metrics() 每个心跳清一次调度器窗口账这件事免疫）
 * @param    {int64_t} llWinMs：本片实际时长
 * @return   {double} 0~100；<0 = 窗口无效（片长非正、或引擎根本没攒过账），调用方按「没量到」处理
 */
double heavySliceBusyPct(int64_t llSeenBusyMs, int64_t llWinMs);

/**
 * @brief   : 按这一组上一片的 NPU 占用，决定它下一片的长短
 * @param    {int64_t} llBaseMs：配置给的片长（slice_pose_ms / slice_face_ms，已被钳到 ≤25s）
 * @param    {double} dBusyPct：这一组上一片的占用（heavySliceBusyPct 的产出）；<0 = 没量过，原样放行
 * @return   {int64_t} 下一片时长
 */
int64_t heavyAdaptSliceMs(int64_t llBaseMs, double dBusyPct);

/**
 * @brief   : 一个模型组在掩码里的那一位
 * @return   {uint32_t} 只置一位
 */
uint32_t warmGroupBit(ModelGroup_E enGroup);

/**
 * @brief   : 纯按配置快照算"需要驻留哪些模型组"（不含事件触发的复核保留期、不含守护档位）
 * @param    {const CConfigSnapshot_S&} stSnap：已校验的那一份
 * @return   {uint32_t} 组位图，0 = 什么都不驻留
 */
uint32_t warmWantedGroupMask(const CConfigSnapshot_S &stSnap);

/**
 * @brief   : 人脸段（检测/配行/取特征）这一拍到底该不该跑
 * @param    {const CConfigSnapshot_S&} stSnap：已校验（含角色矩阵）的那一份
 * @return   {bool} true = 预热要驻留 G_FACE、管线也该按节拍提交
 */
bool faceStageWanted(const CConfigSnapshot_S &stSnap);

/**
 * @brief 只读的运行时状态入参：把引擎那几个成员显式化，好让"上一拍已经持着 POSE"能在主机上构造
 */
typedef struct _WarmNeed_S
{
    int64_t      llPhoneWarmUntilMs = 0;                   /* 复核保留期的到期时刻（单调毫秒），0 = 没有 */
    /* 板书侧最近一次"有活动"的时刻（有采集请求挂着 / 一次采集在途 / 刚发起过），0 = 开学至今没写过字 */
    int64_t      llBoardLastActMs = 0;
    GuardLevel_E enLevel = GuardLevel_E::GL_NORMAL;        /* 内存守护档位 */
    bool         bNeedRotate = false;                      /* 时间片轮转是否武装中 */
    ModelGroup_E enHeavyTurn = ModelGroup_E::GROUP_POSE;   /* 轮转中本轮持哪一组 */
} CWarmNeed_S;

/**
 * @brief   : 期望驻留集合 = 配置需求 ∪ 事件触发的复核保留期，再按守护档位与轮转裁剪
 * @param    {int64_t} llNowMs：本拍时刻，只用来判复核那份闩到没到期
 * @param    {const CWarmNeed_S&} rstNeed：见该结构体，全是入参不是状态
 * @return   {uint32_t} 组位图
 */
uint32_t warmEffectiveGroupMask(const CConfigSnapshot_S &stSnap, int64_t llNowMs,
                                const CWarmNeed_S &rstNeed);

/**
 * @brief   : 手机复核那颗本拍要不要驻留
 * @return   {bool} true = 要：要么教师玩手机这条行为本来就开着，要么复核名单刚非空过
 */
bool warmPhoneWanted(const CConfigSnapshot_S &stSnap, int64_t llNowMs, const CWarmNeed_S &rstNeed);

/**
 * @brief   : 板书 OCR 那两颗，本拍要不要驻留
 * @return   {bool} true = 要：板书开着，且"最近有过板书活动"还压在采集间隔 + 余量之内
 */
bool warmBoardOcrWanted(const CConfigSnapshot_S &stSnap, int64_t llNowMs, const CWarmNeed_S &rstNeed);

/**
 * @brief   : 板书边沿挂着而 OCR 从未驻留时的冷启动保持：这一拍该不该**不消费**这次采集机会
 * @param    {bool} bBoardEnable：配置快照里 board.enable（总开关先行，与 boardStep 的判序一致）
 * @param    {bool} bCaptureBusy：watcher 的 captureBusy()——"真有挂起请求/在途作业"；
 *            边沿模式下「没写过字不 load」的出厂语义就压在这个入参上，没有活动时不许触发
 * @param    {bool} bPeriodic：board.episode=false 的周期采集模式。该模式下没有边沿可挂，
 *            而在途作业当拍就被同步失败路径清掉（CAPTURE_MISS/MODEL_MISSING 都走 onOcrResult），
 *            bCaptureBusy 在暖组看得见之前恒 false —— 不认这个入参，周期模式的第一次采集
 *            必然被消费成 CAPTURE_MISS（没有裁剪档），之后活动闩永远置不上，OCR 永远不装载
 * @param    {BackendState_E} enOcrDet：池子报的 OcrDet 状态
 * @return   {bool} true = 挂住：调用方把活动闩推上去（boardStep 里 m_llBoardLastActMs = now）并整拍让路，
 *            机会留给下一拍，暖组下一拍按 warmBoardOcrWanted 装 OCR；false = 照旧走消费路径
 */
bool boardColdStartHold(bool bBoardEnable, bool bCaptureBusy, bool bPeriodic, BackendState_E enOcrDet);

/**
 * @brief   : 一颗模型本拍是否需要驻留（组位图之外的那一半判据）
 * @param    {uint32_t} u32Want：warmEffectiveGroupMask 给的组位图（已按档位与轮转裁剪过）
 * @param    {const CModelDesc_S&} stDesc：要问的那一颗（读它的 enGroup 与 strKey）
 * @return   {bool} true = 该持有；false = 该放掉（含"整组都不要"与"组要、但这颗不要"）
 */
bool warmModelWanted(const CConfigSnapshot_S &stSnap, int64_t llNowMs, const CWarmNeed_S &rstNeed,
                     uint32_t u32Want, const CModelDesc_S &stDesc);

/**
 * @brief   : 本拍该驻留的**模型**集合指纹（按 stSnap.vecModels 的下标置位）
 * @return   {uint32_t} 位图，位 i = vecModels[i] 该驻留
 */
uint32_t warmResidentModelMask(const CConfigSnapshot_S &stSnap, int64_t llNowMs,
                               const CWarmNeed_S &rstNeed, uint32_t u32Want);

/**
 * @brief   : 本拍该不该真去问池子要模型（重试门）
 * @param    {int64_t} llLastWarmMs：上次尝试的时刻，<=0 = 从没试过，或"目标刚变过"被调用方清成 0
 * @return   {bool} true = 该试。首次永远该试，之后每隔满 CLASSROOM_WARM_RETRY_MS 的那一拍放行
 */
bool warmRetryDue(int64_t llLastWarmMs, int64_t llNowMs);

/**
 * @brief   : 一次被拒是否构成"两个重档组此刻装不下、该武装轮转"的证据
 * @param    {BackendState_E} enRefused：池子给的拒绝原因
 * @param    {uint32_t} u32Want：本拍的期望驻留集合（**已裁剪过**的那一份，不是原始需求）
 * @param    {uint32_t} u32Bit：被拒那颗模型所在的组位
 * @param    {bool} bNeedRotate：轮转是否已经武装
 * @return   {bool} true = 武装轮转
 */
bool warmRefuseArmsRotate(BackendState_E enRefused, uint32_t u32Want, uint32_t u32Bit,
                          bool bNeedRotate);

/**
 * @brief   : 拒绝原因台账："这一颗上次是被什么原因挡住的"
 * @param    {std::map<std::string, BackendState_E>&} rmapLedger：调用方持有的那张表
 * @return   {bool} true = 与表里的旧值不同（含没见过），本次要打一行；表已被更新为本次的原因
 */
bool warmRefusalReport(std::map<std::string, BackendState_E> &rmapLedger,
                       const std::string &strKey, BackendState_E enRefused);

/**
 * @brief   : 拿到租约后台账改记 READY（与 warmRefusalReport 同一张表的另一侧）
 */
void warmReadyAdopt(std::map<std::string, BackendState_E> &rmapLedger, const std::string &strKey);
}
