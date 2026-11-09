/**
 * @FilePath     : data_center.h
 * @Author       : leiyy
 * @Date         : 2026-10-22 14:47:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-22 14:48:00
 * @Description  : 课堂数据聚合中心
 */

#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "attendance.h"
#include "board_watcher.h"
#include "group_stats.h"
#include "scene_infer.h"
#include "seat_heat.h"
#include "stu_track.h"
#include "classroom_define.h"
#include "rule_engine.h"

namespace Classroom
{
/* 样本环容量 */
#define CLASSROOM_METRIC_RING (480)

/* 事件环容量 */
#define CLASSROOM_EVENT_RING (1024)

/**
 * @brief 聚合参数
 */
typedef struct _DataCenterParam_
{
    int32_t nSampleRingNum = CLASSROOM_METRIC_RING;   /* 样本环容量 */
    int32_t nEventRingNum = CLASSROOM_EVENT_RING;     /* 事件环容量 */
    int64_t llReportIntervalMs = 60000;               /* 上报节奏（与 egress.report_interval_sec 同源） */
    int32_t nReportBatchEvent = 192;                  /* 一次上报最多带几条事件 */
} CDataCenterParam_S;

/**
 * @brief 会话累计量
 */
typedef struct _SessionAccum_
{
    int64_t  llBeginMs = 0;        /* 会话起点（第一次收到样本的时刻） */
    int64_t  llLastMs = 0;         /* 上次积分到的时刻，0 表示还没开始 */
    int64_t  llLastPresentMs = 0;  /* 上一次「有人在场」的样本时刻，0 表示本节还没见过人。
                                       会话静默收尾（#95）拿它当静默起点：人走光多久了，
                                       与「样本多久没来」（llLastMs 管的）是两件事 */
    int64_t  llPresentMs = 0;      /* 有人在场的时间（分母） */
    int64_t  llActiveMs[CDurationGate::BEHAVIOR_COLUMN_NUM] = { 0 };
    /* 各行为的**开始**事件条数 */
    uint64_t u64BeginAt[CDurationGate::BEHAVIOR_COLUMN_NUM] = { 0 };
    /* 各情绪类别的**观测人次 */
    uint64_t u64ExprNum[CLASSROOM_EXPRESSION_CLASS_NUM] = { 0 };
    uint64_t u64BeginEvt = 0;      /* 本会话内的起止事件条数（不是环里的条数） */
    uint64_t u64EndEvt = 0;
    int32_t  nPersonMax = 0;       /* 峰值在场人数，"这节课最多多少人" */
    int32_t  nSampleNum = 0;       /* 已积分数（= 样本数），报表可信度：样本太少就别给占比 */
    bool     bTruncatedSeen = false; /* 会话期间出现过人数截断：所有百分比都要打上这个标记 */
} CSessionAccum_S;

/**
 * @brief 聚合中心观测
 */
typedef struct _DataCenterStat_
{
    uint64_t u64SampleIn = 0;
    uint64_t u64SampleOverwrite = 0;   /* 环满覆盖最旧的次数：非 0 就说明环容量或上报周期不匹配 */
    uint64_t u64EventIn = 0;
    uint64_t u64EventOverwrite = 0;
    uint64_t u64EventUnsent = 0;        /* 环里还没被取走的事件数 */
    uint64_t u64ReportBuild = 0;
    uint64_t u64ReportSkip = 0;         /* 到点但没样本可报（刚起线、或总开关刚开） */
    /* 断档跳过积分的次数 */
    uint64_t u64AccumGap = 0;
    int32_t  nSampleHold = 0;
    int32_t  nEventHold = 0;
} CDataCenterStat_S;

/**
 * @brief 一次上报要带的内容
 */
typedef struct _ReportBundle_
{
    CGroupMetric_S   stLatest;                       /* 最新一条样本 */
    std::vector<CGroupMetric_S> vecTimeline;         /* 自上次上报以来的样本（含上次那条，便于画折线） */
    std::vector<CBehaviorEvent_S> vecEvents;         /* 自上次上报以来的事件，容量受 nReportBatchEvent 限 */
    CSessionAccum_S  stSession;
    /* 两个"没报出去"必须分开 */
    int32_t  nEventDropped = 0;
    int32_t  nEventLost = 0;
    bool     bSessionReset = false;                  /* true=这是一次课的收尾（下课/关闭），报表要落最终值 */
    /* 考勤快照 */
    CAttendSnap_S stAttend;
    /* 座位热力快照。与 stAttend 同一来路：聚合中心不认识格子图，这一格由引擎在
       buildReport 之后填进来 */
    CSeatSnap_S   stSeat;
    /* 板书记录（「板书内容」那一列的设备侧来源）。与考勤快照、座位图同一来路 */
    std::vector<CBoardItem_S> vecBoard;
    /* 这一路开不开、以及最近一次采集的可用状态（`BackendState_E` 的整数值） */
    bool     bBoardEnable = false;
    int32_t  nBoardState = 0;
    /* 课堂场景时段。闭合段是**队列语义**（与 vecEvents 同：取走就不再出现在下一份里），
       而正在延长的那一段每次都带——平台要能在课中就知道"现在是什么场景"，
       而一段还没结束时就把它写进闭合列表会造出一个假的结束时刻 */
    std::vector<CSceneSegment_S> vecScenes;
    CSceneSegment_S stOpenScene;
    bool     bSceneEnable = false;
    /* 学生机位轨迹。与座位图同一来路、同一个量纲*/
    CStuTrackSnap_S stStuTrack;
} CReportBundle_S;

/**
 * @brief   : 数据聚合中心
 */
class CDataCenter
{
public:
    CDataCenter() = default;
    ~CDataCenter() = default;

    CDataCenter(const CDataCenter &) = delete;
    CDataCenter &operator=(const CDataCenter &) = delete;

    /**
     * @brief   : 定容并复位（可重复调；换容量时整表重建）
     * @return   {bool} 参数非法（容量为 0 或超上限）返回 false 并保持未就绪
     */
    bool init(const CDataCenterParam_S &stParam);
    void deinit();
    bool isReady() const;

    /**
     * @brief   : 收一条群体样本，并顺带把会话累计量积分到该时刻
     */
    void pushSample(const CGroupMetric_S &stSample);

    /* 收一条行为起止事件（来自 CRuleEngine::pollEvent，节拍线程逐条转进来） */
    /* 入库并分配事件唯一序号（1 起）。返回值给即时事件通道用（#95）：0 = 没进环
       （聚合中心没就绪），那份即时报文就带 0，平台对它没法做序号去重 */
    uint64_t pushEvent(const CBehaviorEvent_S &stEvent);

    /**
     * @brief   : 取一份上报内容并清掉已带走的部分
     * @param    {int64_t} llNowMs：本次取数时刻
     * @param    {CReportBundle_S&} rstOut：OUT=报文素材（内部 vector 会按需增长，容量稳定后不再扩）
     * @return   {bool} false 表示没有新内容可报（一条样本都没进来过），调用方不必上报也不该报空包
     */
    bool buildReport(int64_t llNowMs, CReportBundle_S &rstOut);

    /* 会话收尾（总开关关闭、或会话时长到上限）：把累计量结算并给出最后一份素材。
       内存降到应急档**不**收尾：应急档会回涨，一场课里来回几次就会把一节课切成多节"已结束" */
    bool closeSession(int64_t llNowMs, CReportBundle_S &rstOut);

    CDataCenterStat_S stat() const;
    void resetStat();
    CSessionAccum_S session() const;

    /**
     * @brief   : 最新一条群体样本（课程报告页用）
     * @return   {CGroupMetric_S} 还没样本时返回默认构造（各比率 -1 = 不知道）
     */
    CGroupMetric_S latest() const;
    CDataCenterParam_S param() const
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        return m_stParam;
    }

    /**
     * @brief   : 只换上报周期这一个数（配置热生效用）
     */
    void setReportIntervalMs(int64_t llReportMs);

    std::string statLine() const;     /* 心跳 data= 段 */
    std::string statJson() const;     /* statusJson 的 data 段 */

private:
    /* 锁内：把 [llLastMs, llToMs] 这段按当时的生效人数积进会话账本。调用方保证 llToMs>llLastMs */
    void accumulateLocked(const CGroupMetric_S &stSample, int64_t llToMs);

    /**
     * @brief   : 锁内：环里**还活着**且没被取走的事件条数（= 下次取数能带走的条数）
     */
    uint64_t eventUnsentLocked() const;

    mutable std::mutex m_mtx;
    CDataCenterParam_S m_stParam;
    bool m_bReady = false;

    std::vector<CGroupMetric_S> m_vecSample;    /* 定容环形 */
    /* 一律用**单调累计序号**判断"哪些还没被带走"，而不是环内下标：
       环满覆盖最旧之后，下标语义会变（同一个下标指向的内容换了），
       而序号语义不变 —— 少一处要跟着覆盖动作同步的状态，就少一处会算错的地方。
       序号从 1 起，环位 = (ullSeq - 1) % 容量，可读区间 (ullIn - nNum, ullIn] */
    uint64_t m_u64SampleIn = 0;                 /* 累计写入条数（也即最新一条的序号） */
    uint64_t m_u64SampleSent = 0;               /* 累计已随上报带走的条数 */
    size_t m_nSampleNum = 0;                    /* 环内当前条数（<= 容量） */

    std::vector<CBehaviorEvent_S> m_vecEvent;   /* 定容环形 */
    uint64_t m_u64EventIn = 0;
    uint64_t m_u64EventTaken = 0;
    /* 环满时被覆盖、且当时就还没被取走的事件：这笔账一旦丢了就永远补不回来，
       必须单独计数并在报表里给出去（与"本轮没带走、下次带走"的 nEventDropped 是两回事） */
    uint64_t m_u64EventLost = 0;
    uint64_t m_u64EventLostSent = 0;
    size_t m_nEventNum = 0;

    CSessionAccum_S m_stSession;
    CDataCenterStat_S m_stStat;
};
}
