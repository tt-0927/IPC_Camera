/**
 * @FilePath     : event_merger.h
 * @Author       : leiyy
 * @Date         : 2026-10-20 10:20:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 10:20:00
 * @Description  : 行为事件合并器
 */

#pragma once

#include <cstdint>
#include <string>

#include "rule_engine.h"

namespace Classroom
{
/* 文档给的合并窗口：翻转重取一般在 1~2s 内完成，3s 覆盖「重新攒满最短时长」的路径 */
#define CLASSROOM_MERGE_WINDOW_MS   (3000)
/* 挂起表容量：满座 60 人在同一窗口内各自结束一段行为也放得下，余量给教师与双重翻转 */
#define CLASSROOM_MERGE_PENDING_MAX (64)
/* 输出环容量：单次 tick/flush 的产出远小于这个数（挂起表全满放行也就 64 条） */
#define CLASSROOM_MERGE_OUT_RING    (64)

/**
 * @brief 合并参数
 */
typedef struct _EventMergerParam_
{
    int32_t nWindowMs = CLASSROOM_MERGE_WINDOW_MS;  /* END 挂起等待翻转的窗口 */
    float   fDistRatio = 0.8f;                      /* 中心距离 ≤ 头框对角线均值 × 此值判「同一位置」 */
} CEventMergerParam_S;

/**
 * @brief 合并器观测
 */
typedef struct _EventMergerStat_
{
    uint64_t u64Fed = 0;          /* 进来的事件总数（含被吞的） */
    uint64_t u64BeginPass = 0;    /* 原样放行的 BEGIN */
    uint64_t u64BeginSwallow = 0; /* 被吞掉的 BEGIN（翻转接手）条数 */
    uint64_t u64EndIn = 0;        /* 进入挂起的 END 条数 */
    uint64_t u64EndOut = 0;       /* 放行的 END 条数（到期原样与合并形都算） */
    uint64_t u64Evict = 0;        /* 挂起表压力：放行最旧腾位 / 开链无处挂丢弃 */
    uint64_t u64SealMark = 0;     /* 被 sealPending 事后封口的挂起 END 条数 */
    uint64_t u64SealBlock = 0;    /* 因「翻转收尾」拒绝并段的 BEGIN 条数
                                     每个教师交接应看到 1 条 212 结束 + 1 条新的 212 开始 */
    int32_t  nPendingNum = 0;     /* 当前挂起条数 */
} CEventMergerStat_S;

/**
 * @brief   : 两个头框算不算「同一物理位置」（纯函数，主机可测）
 * @param    {const CRectf_S&} stA/stB：两次事件各自携带的头框（分析帧坐标）
 * @param    {float} fDistRatio：中心距离相对头框尺寸的允许倍数
 * @return   {bool} true=两框大概率是同一颗头
 */
bool eventBoxNear(const CRectf_S &stA, const CRectf_S &stB, float fDistRatio);

/**
 * @brief   : 行为事件合并器
 */
class CEventMerger
{
public:
    CEventMerger() = default;
    ~CEventMerger() = default;

    CEventMerger(const CEventMerger &) = delete;
    CEventMerger &operator=(const CEventMerger &) = delete;

    /* 定容并复位（固定容量，不会失败；可重复调） */
    void init(const CEventMergerParam_S &stParam);
    void reset();

    /**
     * @brief   : 把某人挂在窗口里的 END 全部封口（主体翻转时调用）
     * @param    {int32_t} nPersonId：刚翻转的人
     */
    void sealPending(int32_t nPersonId);
    bool isReady() const { return m_bReady; }

    /**
     * @brief   : 喂一条事件。产出的可上报事件从 pollEvent 取
     */
    void feed(const CBehaviorEvent_S &stEvent);

    /* 每拍调：把超过窗口还没等到翻转 BEGIN 的挂起 END 放行 */
    void tick(int64_t llNowMs);

    /* 会话收尾前调：不等窗口，所有已挂起的 END 立刻放行。
       开着链（已吞 BEGIN、还在等它那条 END）不动——那条 END 会在之后的节拍里来，
       提前凭空造一条结束时刻是把「还没结束」报成「已结束」 */
    void flushAll();

    bool pollEvent(CBehaviorEvent_S &stOut);

    CEventMergerStat_S stat() const;
    CEventMergerParam_S param() const { return m_stParam; }
    std::string statLine() const;

private:
    /* 一段（可能已多次换手的）行为的合并账本。
       首 ID 是平台看到的那条 BEGIN 的 id：整段 END 挂它名下，
       BEGIN(first)…END(first) 的配对形状与无翻转时完全一致 */
    typedef struct _MergeEntry_
    {
        bool       bUsed = false;
        Behavior_E enBehavior = Behavior_E::STU_STAND_UP;
        int32_t    nFirstId = -1;    /* 平台看到的 id（BEGIN 的） */
        int32_t    nLastId = -1;     /* 链上最新的跟踪 id：认领下一条 END 用 */
        int64_t    llSumHeldMs = 0;  /* 已吞掉的各段时长之和（不含正在挂起的那段） */
        CBehaviorEvent_S stPendingEnd;
        bool       bEndValid = false; /* false=链开着，正在等新 id 的 END */
    } CMergeEntry_S;

    int32_t findPairable(const CBehaviorEvent_S &stBegin);
    int32_t findChainEnd(const CBehaviorEvent_S &stEnd);
    int32_t oldestSlot() const;
    void emit(const CBehaviorEvent_S &stEvent);
    /* 放行一条合并形 END 并释放槽位。只在 bEndValid 的槽上调 */
    void emitMerged(CMergeEntry_S &rstEntry);

    CEventMergerParam_S m_stParam;
    bool m_bReady = false;

    CMergeEntry_S m_astPending[CLASSROOM_MERGE_PENDING_MAX];
    CBehaviorEvent_S m_astOut[CLASSROOM_MERGE_OUT_RING];
    uint32_t m_u32OutHead = 0;
    uint32_t m_u32OutNum = 0;

    CEventMergerStat_S m_stStat;
};
}
