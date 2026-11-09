/**
 * @FilePath     : classroom_warm_policy.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-17 16:30:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-17 16:30:00
 * @Description  : 见 classroom_warm_policy.h
 */

#include "classroom_warm_policy.h"

#include <algorithm>
#include <map>

#include "duration_gate.h"

namespace Classroom
{
namespace
{
/* 组位图：一条行为需要哪几组模型，全靠它表达 */
constexpr uint32_t warmMaskBit(ModelGroup_E enGroup)
{
    return 1u << static_cast<uint32_t>(enGroup);
}

constexpr uint32_t MASK_BASE  = warmMaskBit(ModelGroup_E::GROUP_BASE);
constexpr uint32_t MASK_POSE  = warmMaskBit(ModelGroup_E::GROUP_POSE);
constexpr uint32_t MASK_FACE  = warmMaskBit(ModelGroup_E::GROUP_FACE);
constexpr uint32_t MASK_OTHER = warmMaskBit(ModelGroup_E::GROUP_OTHER);

/* 两个重资源组同时开启才有轮转问题 */
constexpr uint32_t MASK_HEAVY = MASK_POSE | MASK_FACE;

/*
 * 行为 → 所需模型组
 * 下标口径与 CDurationGate 的列号严格一致
 * 加一个 Behavior_E 忘了补一行的后果是「功能开了但模型永远不预热」，
 * 因此下面用 static_assert 把行数钉死在列数上，编译期就炸，不给运行期留悬念。
 */
constexpr uint32_t AST_BEHAVIOR_GROUPS[] =
{
    /* ---------- 学生 101~112 ---------- */
    MASK_BASE | MASK_POSE,                    /* STU_STAND_UP    */
    MASK_BASE | MASK_POSE,                    /* STU_RAISE_HAND  */
    MASK_BASE | MASK_POSE,                    /* STU_HEAD_UP     */
    MASK_BASE | MASK_POSE,                    /* STU_HEAD_DOWN   */
    MASK_BASE | MASK_POSE,                    /* STU_TURN_BACK   */
    MASK_BASE | MASK_POSE,                    /* STU_PHONE：判据是低头×手在身前，两半都只吃骨骼读数 */
    MASK_BASE | MASK_POSE,                    /* STU_LISTEN      朝向读鼻与双眼，吃骨骼 */
    MASK_BASE | MASK_POSE,                    /* STU_READING     与 STU_PHONE 同一个形状 */
    MASK_BASE | MASK_POSE,                    /* STU_DISCUSS     邻座朝向，同一批面部点 */
    MASK_BASE,                                /* STU_ON_PODIUM   只用头框与区域，刻意不吃 POSE：
                                                  姿态在时间片外时"谁站在讲台上"这件事照样要报 */
    MASK_BASE | MASK_POSE,                    /* STU_PRESENT     站立 + 头朝全班 */
    MASK_BASE | MASK_FACE,                    /* STU_EXPRESSION  */
    /* ---------- 教师 201~213 ---------- */
    MASK_BASE | MASK_POSE,                    /* TEA_STAND_UP     */
    MASK_BASE | MASK_POSE,                    /* TEA_HEAD_UP      */
    MASK_BASE | MASK_POSE,                    /* TEA_HEAD_DOWN    */
    MASK_BASE | MASK_POSE,                    /* TEA_TURN_BACK    */
    MASK_BASE | MASK_POSE | MASK_OTHER,       /* TEA_PHONE        */
    MASK_BASE,                                /* TEA_TEACHING     */
    MASK_BASE,                                /* TEA_GUIDING      */
    MASK_BASE,                                /* TEA_BOARD_WRITE  */
    MASK_BASE,                                /* TEA_INTERACT     */
    MASK_BASE,                                /* TEA_PATROL       */
    MASK_BASE,                                /* TEA_ENTER_PODIUM */
    MASK_BASE,                                /* TEA_TRACK        */
    MASK_BASE | MASK_FACE,                    /* TEA_IDENTITY     */
};

constexpr uint32_t BEHAVIOR_GROUP_NUM =
    static_cast<uint32_t>(sizeof(AST_BEHAVIOR_GROUPS) / sizeof(AST_BEHAVIOR_GROUPS[0]));
}

static_assert(BEHAVIOR_GROUP_NUM == static_cast<uint32_t>(CDurationGate::BEHAVIOR_COLUMN_NUM),
              "行为→模型组映射表行数必须等于时长门行为列数（新增 Behavior_E 要同步补行）");

uint32_t warmGroupBit(ModelGroup_E enGroup)
{
    return warmMaskBit(enGroup);
}

uint32_t warmHeavyMask(void)
{
    return MASK_HEAVY;
}

bool faceStageWanted(const CConfigSnapshot_S &stSnap)
{
    /* 三个条件与 warmWantedGroupMask 的 FACE 位一字不差 */
    return stSnap.bEnable && stSnap.stAttend.bEnable && stSnap.stSample.nFaceIntervalMs > 0;
}

bool warmRotateHasTarget(const CConfigSnapshot_S &stSnap)
{
    return (warmWantedGroupMask(stSnap) & MASK_HEAVY) == MASK_HEAVY;
}

uint32_t warmWantedGroupMask(const CConfigSnapshot_S &stSnap)
{
    if (!stSnap.bEnable)
    {
        return 0;
    }

    /* vecGates 的下标就是行为列号，与 AST_BEHAVIOR_GROUPS 同一套序，故直接按列或起来*/
    const uint32_t u32Num = static_cast<uint32_t>(
        std::min(stSnap.vecGates.size(), static_cast<size_t>(BEHAVIOR_GROUP_NUM)));
    uint32_t u32Mask = 0;
    for (uint32_t i = 0; i < u32Num; ++i)
    {
        if (stSnap.vecGates[i].bEnable)
        {
            u32Mask |= AST_BEHAVIOR_GROUPS[i];
        }
    }

    /* 考勤不是一条"行为列"：上面那张表按 Behavior_E 的列号索引，而到课账本是会话级的，
       在列表里没有位置 */
    if (faceStageWanted(stSnap))
    {
        u32Mask |= MASK_FACE;
    }
    return u32Mask;
}

uint32_t warmEffectiveGroupMask(const CConfigSnapshot_S &stSnap, int64_t llNowMs,
                                const CWarmNeed_S &rstNeed)
{
    uint32_t u32Want = warmWantedGroupMask(stSnap);

    /* 事件触发的那一份需求  */
    if (warmPhoneWanted(stSnap, llNowMs, rstNeed))
    {
        u32Want |= MASK_OTHER;
    }

    /* 板书 OCR 那两颗的需求同样落在 G_OTHER 上 */
    if (warmBoardOcrWanted(stSnap, llNowMs, rstNeed))
    {
        u32Want |= MASK_OTHER;
    }

    /* 守护档位直接改写目标集合 */
    if (rstNeed.enLevel == GuardLevel_E::GL_HARD)
    {
        u32Want &= ~(MASK_OTHER | MASK_FACE);
    }
    else if (rstNeed.enLevel >= GuardLevel_E::GL_EMERGENCY)
    {
        return 0;
    }

    if (rstNeed.bNeedRotate)
    {
        /* 两组放不下：这一拍只持其中一组，另一组由引擎的换片判据到点换入 */
        u32Want &= (ModelGroup_E::GROUP_POSE == rstNeed.enHeavyTurn) ? ~MASK_FACE : ~MASK_POSE;
    }
    return u32Want;
}

bool warmPhoneWanted(const CConfigSnapshot_S &stSnap, int64_t llNowMs, const CWarmNeed_S &rstNeed)
{
    if (!stSnap.bEnable)
    {
        return false;
    }
    if (llNowMs < rstNeed.llPhoneWarmUntilMs)
    {
        return true;
    }
    /* 教师玩手机那条行为本来就是这颗模型的直接消费者（不是"复核"，是常驻需求 */
    const int nCol = CDurationGate::behaviorColumn(Behavior_E::TEA_PHONE);
    return nCol >= 0 && static_cast<size_t>(nCol) < stSnap.vecGates.size() &&
           stSnap.vecGates[static_cast<size_t>(nCol)].bEnable;
}

bool warmBoardOcrWanted(const CConfigSnapshot_S &stSnap, int64_t llNowMs, const CWarmNeed_S &rstNeed)
{
    if (!stSnap.bEnable || !stSnap.stBoard.bEnable)
    {
        return false;
    }
    /* 与 watcher 的 wantCapture 同一条：间隔 <=0 的语义是"本功能不参与节拍"，不是"每拍都采"。
       判在这里而不是只靠调用方不刷新那份闩*/
    const int64_t llIntervalMs = stSnap.stBoard.llIntervalMs;
    if (llIntervalMs <= 0)
    {
        return false;
    }
    if (rstNeed.llBoardLastActMs <= 0)
    {
        return false;   /* 开机到现在没写过字：这颗模型一次都不该 load */
    }
    return (llNowMs - rstNeed.llBoardLastActMs) <= (llIntervalMs + CLASSROOM_BOARD_WARM_SLACK_MS);
}

bool boardColdStartHold(bool bBoardEnable, bool bCaptureBusy, bool bPeriodic,
                        BackendState_E enOcrDet)
{
    /* 边沿模式靠"挂起边沿本身让 captureBusy 为真"完成自举；周期模式（episode=false）
       没有边沿，在途作业又被同步失败路径当拍清掉，captureBusy 在暖组看得见之前恒 false
       ——没有 bPeriodic 这一格，周期模式的第一次采集必然被消费成 CAPTURE_MISS，
       活动闩永远置不上，OCR 永远不装载 */
    if (!bBoardEnable || !(bCaptureBusy || bPeriodic))
    {
        return false;
    }
    return BackendState_E::UNLOADED == enOcrDet;
}

bool warmModelWanted(const CConfigSnapshot_S &stSnap, int64_t llNowMs, const CWarmNeed_S &rstNeed,
                     uint32_t u32Want, const CModelDesc_S &stDesc)
{
    if (0 == (u32Want & warmMaskBit(stDesc.enGroup)))
    {
        return false;
    }
    if (ModelGroup_E::GROUP_OTHER != stDesc.enGroup)
    {
        return true;
    }
    if (CLASSROOM_MODEL_KEY_OCR_DET == stDesc.strKey ||
        CLASSROOM_MODEL_KEY_OCR_REC == stDesc.strKey)
    {
        return warmBoardOcrWanted(stSnap, llNowMs, rstNeed);
    }
    if (CLASSROOM_MODEL_KEY_PHONE == stDesc.strKey)
    {
        return warmPhoneWanted(stSnap, llNowMs, rstNeed);
    }
    return true;
}

uint32_t warmResidentModelMask(const CConfigSnapshot_S &stSnap, int64_t llNowMs,
                               const CWarmNeed_S &rstNeed, uint32_t u32Want)
{
    const size_t nNum = std::min(stSnap.vecModels.size(), static_cast<size_t>(32));
    uint32_t u32Set = 0;
    for (size_t i = 0; i < nNum; ++i)
    {
        if (warmModelWanted(stSnap, llNowMs, rstNeed, u32Want, stSnap.vecModels[i]))
        {
            u32Set |= (1u << i);
        }
    }
    return u32Set;
}

bool warmRetryDue(int64_t llLastWarmMs, int64_t llNowMs)
{
    if (llLastWarmMs <= 0)
    {
        return true;
    }
    return (llNowMs - llLastWarmMs) >= CLASSROOM_WARM_RETRY_MS;
}

bool warmRefuseArmsRotate(BackendState_E enRefused, uint32_t u32Want, uint32_t u32Bit,
                          bool bNeedRotate)
{
    /* 两种拒绝都算"这两组此刻装不下" */
    if (BackendState_E::OUT_OF_BUDGET != enRefused && BackendState_E::GROUP_BUSY != enRefused)
    {
        return false;
    }
    return (0 != (u32Bit & MASK_HEAVY)) && !bNeedRotate &&
           (u32Want & MASK_HEAVY) == MASK_HEAVY;
}


double heavySliceBusyPct(int64_t llSeenBusyMs, int64_t llWinMs)
{
    if (llWinMs <= 0 || llSeenBusyMs < 0)
    {
        return -1.0;    /* 没攒过账 / 窗口非正：调用方按「没量到」放行，不许拿坏读数改行为 */
    }
    const double dPct = static_cast<double>(llSeenBusyMs) * 100.0 / static_cast<double>(llWinMs);
    return (dPct > 100.0) ? 100.0 : dPct;
}

int64_t heavyAdaptSliceMs(int64_t llBaseMs, double dBusyPct)
{
    if (dBusyPct < 0.0)
    {
        return llBaseMs;
    }
    if (dBusyPct > CLASSROOM_SLICE_BUSY_HI_PCT)
    {
        /* 忙：顶到片长上界 */
        return CLASSROOM_WARM_SLICE_MAX_MS;
    }
    if (dBusyPct < CLASSROOM_SLICE_BUSY_LO_PCT)
    {
        /* 闲：只往下砍。配置本身配得比 10s 短是现场自己的快切选择，不反向改长 */
        return (llBaseMs > CLASSROOM_SLICE_BUSY_LO_MS) ? CLASSROOM_SLICE_BUSY_LO_MS : llBaseMs;
    }
    return llBaseMs;
}

bool warmRefusalReport(std::map<std::string, BackendState_E> &rmapLedger,
                       const std::string &strKey, BackendState_E enRefused)
{
    /* 只在状态变化时打日志：缺件是长期状态，每 2s 刷一条会把日志冲干净 */
    const auto itOld = rmapLedger.find(strKey);
    if (itOld != rmapLedger.end() && itOld->second == enRefused)
    {
        return false;
    }
    rmapLedger[strKey] = enRefused;
    return true;
}

void warmReadyAdopt(std::map<std::string, BackendState_E> &rmapLedger, const std::string &strKey)
{
    rmapLedger[strKey] = BackendState_E::READY;
}
}
