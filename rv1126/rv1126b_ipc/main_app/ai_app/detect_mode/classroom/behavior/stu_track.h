/**
 * @FilePath     : stu_track.h
 * @Author       : leiyy
 * @Date         : 2026-11-03 11:15:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 11:15:00
 * @Description  : 学生机位轨迹
 */

#pragma once

#include <cstdint>
#include <string>

#include "classroom_define.h"

namespace Classroom
{
/* 同时保留几条线。取 8 */
#define CLASSROOM_STU_TRACK_PERSON_MAX (8)

/* 一条线最多几个点。取 10 */
#define CLASSROOM_STU_TRACK_POINT_MAX (10)

/**
 * @brief 讲台门（需求那句「讲台区域人数为0」的三态化）
 */
enum StuTrackGate_E : int32_t
{
    STU_TRACK_GATE_NONE = 0,   /* 讲台角色没画成区域：不知道空不空，一律不记 */
    STU_TRACK_GATE_BUSY = 1,   /* 画了，本拍讲台里有人 */
    STU_TRACK_GATE_FREE = 2,   /* 画了，本拍讲台 0 人：只有这一态产点 */
};

/**
 * @brief 产点理由的位号（进报文时展开成文本，见 stuTrackWhyName）
 */
#define CLASSROOM_STU_WHY_STAND (1U << 0)   /* 101 站立 */
#define CLASSROOM_STU_WHY_BACK  (1U << 1)   /* 105 背身 */

/**
 * @brief 一拍的逐人观测（折样输入，与特征行同序）
 */
typedef struct _StuTrackObs_
{
    int32_t nPersonId = -1;
    /* 源帧归一化 0~1；-1 = 这一拍没有位置证据（几何未就绪或人头框退化） */
    float   fNormX = -1.0f;
    float   fNormY = -1.0f;
    /* 确认归属为教师的那一行不进本表：需求要的是"学生摄像头画面里那些学生"的位置，
       而教师已经有 track 段那一条自己的线。仲裁器从不发布 STUDENT（排除法），
       所以这里是"不是确认的教师即学生"，关掉归属仲裁时本层照样产点，不会静默失效 */
    bool    bStudent = true;
    bool    bStand = false;
    bool    bBack = false;
} CStuTrackObs_S;

/**
 * @brief 轨迹上的一个点
 */
typedef struct _StuTrackPoint_
{
    int64_t llMonoMs = 0;      /* 与特征帧同一单调时基，换算墙钟由报表层做（本层不认识墙钟） */
    float   fNormX = 0.0f;
    float   fNormY = 0.0f;
} CStuTrackPoint_S;

/**
 * @brief 一个人的那条线（快照与常驻用的是同一个形状）
 */
typedef struct _StuTrackPerson_
{
    int32_t  nPersonId = -1;
    uint32_t u32Why = 0;                    /* CLASSROOM_STU_WHY_* 的并集 */
    int32_t  nPtNum = 0;
    CStuTrackPoint_S astPt[CLASSROOM_STU_TRACK_POINT_MAX];
} CStuTrackPerson_S;

/**
 * @brief 一次报表要带的学生轨迹快照（由引擎在组包时取走）
 */
typedef struct _StuTrackSnap_
{
    /* 本层没有开关，于是"为什么这个人列表是空的"只有一个出口：
       最后一次折样看到的门。NONE = 讲台根本没画，此时 person[] 为空是"不知道"而不是"没人"
       （这两件事在设备侧靠心跳的 fold= 分开：一次都没折过样是没起线，折过而 gate=none 是没画区） */
    int32_t nGate = STU_TRACK_GATE_NONE;
    int32_t nPersonNum = 0;
    CStuTrackPerson_S astPerson[CLASSROOM_STU_TRACK_PERSON_MAX];
    /* 三个计数都是**本周期**的（快照即清），与 events 的 dropped/lost 同一队列语义 */
    int32_t nEvictNum = 0;   /* 整条线被让位的次数（同时超 8 个人） */
    int32_t nThinNum = 0;    /* 抽稀次数（保住起点、牺牲次旧点） */
    int32_t nNoPosNum = 0;   /* 该记但这一拍不知道他在哪 */
} CStuTrackSnap_S;

/**
 * @brief   : 理由位 → 报文文本
 */
const char *stuTrackWhyName(uint32_t u32WhyBit);
const char *stuTrackGateName(int32_t nGate);

/**
 * @brief 学生机位轨迹累加器
 */
class CStuTrack
{
public:
    CStuTrack() = default;

    /**
     * @brief   : 折一次样（与群体采样、座位热力同一拍）
     * @param    {const CStuTrackObs_S*} patObs：本拍逐人观测，与特征行同序
     * @param    {int32_t} nNum：行数。0 是有效折样（空场），只是不产点
     * @param    {int32_t} nGate：本拍的讲台门（三态，见 StuTrackGate_E）
     * @param    {int64_t} llNowMs：本拍单调毫秒，写进点里
     */
    void fold(const CStuTrackObs_S *patObs, int32_t nNum, int32_t nGate, int64_t llNowMs);

    /**
     * @brief   : 取一份报表快照：**拷出全部线并就地整表清空**（队列语义）
     */
    void snapshot(CStuTrackSnap_S &rsnapOut);

    /* 一节课收尾：整表作废（周期快照已经带走的部分与没来得及带走的部分在此一并了结）。
       与座位热力同一处调用；累计计数留到进程结束，心跳的 tot= 是"这台机器一辈子截断过多少" */
    void reset();

    /* 心跳一段（`strk=`）：给的是"这条链有没有在偷偷截断"，不含姓名、不含单个人 */
    std::string statText() const;

    /* 累计读数（进程级），给心跳用；周期内的计数在快照里 */
    uint64_t evictTotal() const { return m_u64EvictTotal; }
    uint64_t thinTotal() const { return m_u64ThinTotal; }

private:
    /* 找一个人在表里的下标，没有返回 -1 */
    int32_t indexOf(int32_t nPersonId) const;
    /* 取一个可用下标：空位→直接给，满→让最久没更新的那条线走（并计一次 evict） */
    int32_t rowFor(int32_t nPersonId);
    /* 当前占了几条线（含只有号还没落点的），只给心跳读 */
    int32_t rowCount() const;
    /* 追加一个点；到顶先抽稀 */
    void appendPoint(CStuTrackPerson_S &rstRow, int64_t llNowMs, float fX, float fY);

    CStuTrackPerson_S m_astRow[CLASSROOM_STU_TRACK_PERSON_MAX];
    bool     m_bGateKnown = false;
    int32_t  m_nGate = STU_TRACK_GATE_NONE;
    /* 折样拍数：只用于心跳里判断"这条链到底有没有在跑"（en=1 而 fold=0 是总开关或采样没起来） */
    uint64_t m_u64Folds = 0;
    uint64_t m_u64Points = 0;      /* 历史上真正落进表的点数（累计，只给心跳看"这条链在不在跑"） */

    /* 两套计数：per-period 随快照清（平台的"这一份有没有截断"），total 只增。
       只留一套的话，要么报表答不了当期问题，要么设备上看不出"这一节课比上一节截得多" */
    int32_t  m_nEvictPeriod = 0;
    int32_t  m_nThinPeriod = 0;
    int32_t  m_nNoPosPeriod = 0;
    uint64_t m_u64EvictTotal = 0;
    uint64_t m_u64ThinTotal = 0;
    uint64_t m_u64NoPosTotal = 0;
};

}/* namespace Classroom */
