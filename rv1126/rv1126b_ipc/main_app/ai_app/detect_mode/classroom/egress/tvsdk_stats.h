/**
 * @FilePath     : tvsdk_stats.h
 * @Author       : leiyy
 * @Date         : 2026-11-02 15:26:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 15:26:00
 * @Description  : 班级统计聚合状态机
 */

#pragma once

#include <cstdint>

#include "classroom_config.h"
#include "group_stats.h"
#include "rule_engine.h"
#include "stats_protocol.h"
#include "stats_sink.h"

namespace Classroom
{
/**
 * @brief 聚集判定用的一行（分析帧像素坐标，与上报坐标系无关）
 * @note 只收**站立的学生**。聚集的定义是"多人站在一起"，坐着的一排不算；
 *       头宽用同一帧的头框宽度，远近焦的人因此可比（不用固定像素半径）
 */
typedef struct _StatsPerson_
{
    int32_t nPersonId = -1;
    float   fX = 0.0f;       /* 头框中心 x（分析帧像素）*/
    float   fY = 0.0f;
    float   fHeadW = 0.0f;   /* 头框宽度（分析帧像素）*/
    bool    bStudent = true;
    bool    bStanding = false;
} CStatsPerson_S;

/**
 * @brief 一拍统计输入（引擎在群体采样点填）
 * @note pstMetric/pstPersons 只在调用期间有效，本层**不保存指针**，只拷数字。
 */
typedef struct _StatsSample_
{
    int64_t  llMonoMs = 0;
    int64_t  llWallMs = 0;
    const CGroupMetric_S *pstMetric = nullptr;
    const CStatsPerson_S *pstPersons = nullptr;
    int32_t  nPersonNum = 0;       /* pstPersons 条数 */
    bool     bAttendEnable = false;
    int32_t  nPresentNum = -1;
    int32_t  nNamedNum = -1;
    int32_t  nExpectedNum = -1;
    int32_t  nAbsentNum = -1;
    int32_t  nLeftMid = 0;         /* 引擎按账本 llLastMs 算出的人数 */
    bool     bPatrolKnown = false; /* 210 这一路是否启用（决定"未巡视"判不判）*/
} CStatsSample_S;

/**
 * @brief 一次事件输入的角色（同一条事实在链上出现两次，各取所需）
 * @note  bSegment：**合并器之前**的原始边沿，进段状态机——旧 ID 的 END 与新 ID 的 BEGIN
 *        都必须被平台看到；bCount：**合并器之后**的事件，只认 BEGIN 做次数累计。
 *        两者可以同时为假（上游统一循环时跳过），调用无副作用。
 */
typedef struct _StatsEventObs_
{
    bool bSegment = false;
    bool bCount = false;
} CStatsEventObs_S;

/**
 * @brief 会话收尾的全部读数（位置参数换结构体：字段名写错当场编不过，加字段不改调用点顺序）
 * @note  bAttendEnable=false 时四个考勤数一律按 -1 上报（"没配"不是"零个人"）
 */
typedef struct _StatsSessionSummary_
{
    int32_t nPresentNum = -1;
    int32_t nNamedNum = -1;
    int32_t nExpectedNum = -1;
    int32_t nAbsentNum = -1;
    int32_t nLeftMid = 0;
    bool    bAttendEnable = false;
} CStatsSessionSummary_S;

/**
 * @brief 本层观测（心跳/statusJson 用）
 */
typedef struct _StatsHubStat_
{
    uint64_t u64CounterMsgs = 0;
    uint64_t u64SegMsgs = 0;
    uint64_t u64AlertMsgs = 0;
    uint64_t u64SegRecords = 0;
    uint64_t u64AlertRecords = 0;
    uint64_t u64PushFail = 0;
    uint64_t u64SegLost = 0;       /* 队列溢出丢最旧 */
    uint64_t u64AlertLost = 0;
    uint64_t u64SessionNum = 0;
    int32_t  nSegQueued = 0;
    int32_t  nAlertQueued = 0;
    int64_t  llSessionId = 0;
    bool     bSessionOpen = false;
} CStatsHubStat_S;

/**
 * @brief 班级统计推送状态机（无锁、只在节拍线程使用、不读时钟、不分配）
 * @note  输入：引擎喂的三类事实（事件边沿、群体样本、会话收尾）；输出：通过 IStatsSink
 *        发出三型报文。判据（阈值/hold）全在 CStatsConf_S，状态机本身只决定
 *        "什么时候越线/回线"。
 */
class CStatsHub
{
public:
    CStatsHub();
    ~CStatsHub();

    CStatsHub(const CStatsHub &) = delete;
    CStatsHub &operator=(const CStatsHub &) = delete;

    /* 换传输实现（网关装 SDK sink；不装时用下面的函数适配器）。
       传 nullptr 退回内嵌的函数 sink（即"没有通道"） */
    void setSink(IStatsSink *pSink);
    /* 函数式出口：主机用例与轻量网关的最小用法 */
    void setPushFn(CStatsPushFn fnPush);
    /* 换参数；关掉时清队列与当前会话（平台收到的是"这一段没数据"，而不是半截旧账） */
    void configure(const CStatsConf_S &stConf);

    /**
     * @brief   : 设备角色（教师机/学生机/板机）热切换
     * @param    {DeviceRole_E} enRole：本版配置的部署角色；越界值（配置没解析出来）忽略
     * @param    {int64_t} llMonoMs / llWallMs：换角色的时刻（封口旧会话用）
     * @note    : 统计计数的累计值只对**同一角色**有意义（学生机的出勤/动作带到教师机上
     *            就是一段不属于它的账），因此角色一变就把在跑的会话封口发最终包，下一次
     *            样本按新角色的开课条件重开新会话——不需要重启进程来清账。
     *            同角色重复调用是无副作用的（每拍都会调一次）。
     */
    void setDeviceRole(DeviceRole_E enRole, int64_t llMonoMs, int64_t llWallMs);
    /* 线格式头里的通道号：单通道 IPC 保持默认 0，多通道设备由调用方显式设置 */
    void setChannel(uint32_t u32Channel);

    /**
     * @brief   : 一条行为事件的统一入口
     * @param    {CStatsEventObs_S} stObs：本调用扮演的角色（段边沿 / 计数），见该结构体注释
     * @param    {int64_t} llNowMonoMs / llWallMs：事件时刻的单调与墙钟；bSegment=false 时不看
     * @note    : 分成"角色"而不是两个入口，是因为同一条事件在链上确实出现两次：
     *            合并器之前取段（旧 ID 结束、新 ID 开始），之后取次数（只认 BEGIN）。
     *            一次调用只做一件事，两个标志都假时直接返回
     */
    void onEvent(const CBehaviorEvent_S &stEvent, const CStatsEventObs_S &stObs,
                 int64_t llNowMonoMs = 0, int64_t llWallMs = 0);

    /* 一拍群体样本：更新当前读数、驱动 hold 类预警 */
    void onSample(const CStatsSample_S &stIn);

    /* 每个引擎节拍调：超时类预警 + 按周期/批量上限出包 */
    void tick(int64_t llMonoMs, int64_t llWallMs);

    /* 会话收尾：封口在途段与预警，发最终计数（带 SESSION_FINAL），之后等下一会话。
       stSummary 是本课最终考勤读数：引擎在账本 closeSession 之后再收统计会话，
       不能让最后一包还停在上一份 5s 样本上 */
    void closeSession(int64_t llMonoMs, int64_t llWallMs,
                      const CStatsSessionSummary_S &stSummary = CStatsSessionSummary_S());

    CStatsHubStat_S stat() const;

    /* 出口是否可用（statusJson 用来区分"没接 SDK"与"判据没触发"） */
    bool pushWired() const;

    /**
     * @brief   : 站立学生最大聚集人数（纯函数，主机用例直接调）
     * @return  {int32_t} 以某人为中心、彼此头距 ≤ radius×平均头宽 的最大同簇人数；无人返回 0
     */
    static int32_t maxStandingCluster(const CStatsPerson_S *pstPersons, int32_t nNum,
                                      float fRadiusHeads);

private:
    /* ---- 在途段：键 = (kind, behavior, personId)，线性扫（上界 64，事件级频率）---- */
    struct OpenSeg
    {
        bool     bUsed = false;
        uint8_t  u8Kind = 0;
        uint16_t u16Behavior = 0;
        int32_t  nPersonId = -1;
        int32_t  nSeatId = -1;
        int32_t  nExprClass = -1;
        int64_t  llBeginMonoMs = 0;
        int64_t  llBeginWallMs = 0;
        uint64_t u64EventSeq = 0;
    };
    static const int OPEN_SEG_MAX = 64;

    /* 104 低头在途（趴桌判定用） */
    struct LeanState
    {
        bool    bUsed = false;
        bool    bAlerted = false;
        int32_t nPersonId = -1;
        int64_t llBeginMonoMs = 0;
        int64_t llBeginWallMs = 0;
    };

    struct AlertState
    {
        bool    bActive = false;
        bool    bOnceRaised = false;  /* 一次性预警（未到达/拖堂）：本会话不再重复 */
        int32_t nValue = 1;
        int64_t llBeginMonoMs = 0;
        int64_t llBeginWallMs = 0;
        int64_t llHoldSinceMonoMs = 0; /* hold 类判据从何时开始连续成立 */
        int64_t llClearSinceMonoMs = 0;/* 回线计时（滞回）*/
    };

    /* 队列项只存记录本身：队列在会话收尾时要么全发出去、要么计丢最旧并清空，
       不跨会话滞留——跨会话的旧记录若在下一节重发，头里的 (会话, 序号) 会与
       新会话撞键，平台按幂等键会把它当重复丢掉，反而更难查 */
    struct QueuedSeg
    {
        CStatsSegItem_S stItem;
    };
    struct QueuedAlert
    {
        CStatsAlertItem_S stItem;
    };

private:
    /* onEvent 的两条腿：一次调用最多走一条 */
    void ingestSegment(const CBehaviorEvent_S &stEvent, int64_t llNowMonoMs, int64_t llWallMs);
    void ingestCount(const CBehaviorEvent_S &stEvent);

    void beginSession(int64_t llMonoMs, int64_t llWallMs);
    void resetSessionState();
    /* 封口公共体：bEvaluateTime=false 用于角色切换——那不是下课，不该凭空触发
       "教师未到达/拖堂"这类以开课时刻为基准的判据（学生机本来就拍不到教师） */
    void closeSessionInternal(int64_t llMonoMs, int64_t llWallMs,
                              const CStatsSessionSummary_S &stSummary, bool bEvaluateTime);
    /* 本拍是否具备开课条件：学生到场；或教师机视野里教师/学生到场
       （讲台机位本来就只有教师，最多加一个上台的学生） */
    bool sessionPresence(const CGroupMetric_S &stMetric) const;
    /* 线格式头里的设备角色码：0=板机 / 1=教师机 / 2=学生机 */
    int32_t deviceRoleCode() const;
    void fillHead(CStatsHead_S &rstHead, uint16_t u16Type, int64_t llMonoMs, int64_t llWallMs,
                  uint32_t u32Flags);
    bool sinkUsable() const;
    bool pushMsg(StatsTopic_E enTopic, const void *pData, uint32_t u32Len);
    void flushCounters(int64_t llMonoMs, int64_t llWallMs, bool bForceFinal);
    void flushSegments(int64_t llMonoMs, int64_t llWallMs, bool bForce);
    void flushAlerts(int64_t llMonoMs, int64_t llWallMs, bool bForce);

    void enqueueSeg(const CStatsSegItem_S &stItem);
    void enqueueAlert(StatsAlert_E enType, StatsPhase_E enPhase, int32_t nValue,
                      int64_t llMonoMs, int64_t llWallMs);
    void raiseAlert(StatsAlert_E enType, int32_t nValue, int64_t llMonoMs, int64_t llWallMs);
    void updateAlert(StatsAlert_E enType, int32_t nValue, int64_t llMonoMs, int64_t llWallMs);
    void clearAlert(StatsAlert_E enType, int64_t llMonoMs, int64_t llWallMs);
    void closeAllAlerts(int64_t llMonoMs, int64_t llWallMs);

    int32_t findOpenSeg(uint8_t u8Kind, uint16_t u16Behavior, int32_t nPersonId) const;
    void openSegment(uint8_t u8Kind, uint16_t u16Behavior, const CBehaviorEvent_S &stEvent,
                     int64_t llEventMonoMs, int64_t llEventWallMs);
    void closeSegmentAt(uint8_t u8Kind, uint16_t u16Behavior, int32_t nPersonId,
                        int64_t llEndWallMs, int64_t llDurationMs, uint64_t u64EventSeq);
    void closeAllSegments(int64_t llWallMs);
    void addSegActive(uint8_t u8Kind, uint16_t u16Behavior, int64_t llDurationMs);

    void onLeanBegin(const CBehaviorEvent_S &stEvent, int64_t llEventMonoMs, int64_t llEventWallMs);
    void onLeanEnd(const CBehaviorEvent_S &stEvent, int64_t llEventMonoMs, int64_t llEventWallMs);
    void evaluateHoldAlerts(const CGroupMetric_S &stMetric, int64_t llMonoMs, int64_t llWallMs);
    void evaluateTimeAlerts(int64_t llMonoMs, int64_t llWallMs);
    int32_t countLeanDesk(int64_t llMonoMs) const;

    AlertState &alertOf(StatsAlert_E enType);

private:
    CStatsConf_S m_stConf;
    CFnStatsSink m_stFnSink;            /* setPushFn 的落点（函数式传输） */
    IStatsSink  *m_pSink = &m_stFnSink; /* 当前传输实现；非空，可用性由 ready() 判 */

    /* ---- 会话 ---- */
    bool     m_bSessionOpen = false;
    /* 部署角色：不随会话清零（会话是"这一段课"，角色是"这台机"）。m_bRoleKnown=false
       表示配置还没给过角色，此时按板机（ROLE_BOTH）的老口径判开课 */
    DeviceRole_E m_enDeviceRole = DeviceRole_E::ROLE_BOTH;
    bool     m_bRoleKnown = false;
    uint32_t m_u32MsgSeq = 0;
    uint32_t m_u32Channel = 0;      /* 线格式头里的通道号，默认 0 */
    int64_t  m_llSessionId = 0;
    int64_t  m_llSessionWallMs = 0;
    int64_t  m_llClassStartMonoMs = 0;
    int64_t  m_llClassStartWallMs = 0;
    int64_t  m_llLastSampleMonoMs = 0;

    /* ---- 最近一拍的事实 ---- */
    CGroupMetric_S m_stLastMetric;
    int32_t  m_nPresentNum = -1;
    int32_t  m_nNamedNum = -1;
    int32_t  m_nExpectedNum = -1;
    int32_t  m_nAbsentNum = -1;
    int32_t  m_nLeftMid = 0;
    bool     m_bAttendEnable = false;
    bool     m_bTeacherPresent = false;
    bool     m_bTeacherEverSeen = false;
    int64_t  m_llTeacherAbsentSinceMonoMs = 0;
    bool     m_bPatrolKnown = false;
    int64_t  m_llLastPatrolMonoMs = 0;
    int32_t  m_nGatherNum = 0;
    int32_t  m_nAttentionPct = -1;
    int32_t  m_nDisciplinePct = -1;

    /* ---- 计数 ---- */
    int32_t  m_nPersonMax = 0;
    int32_t  m_nActionBegin[6] = { 0 };
    int32_t  m_nExprBegin[7] = { 0 };
    int32_t  m_nTeachBegin[5] = { 0 };
    int64_t  m_llTeachActiveMs[5] = { 0 };
    int32_t  m_nStudyBegin[5] = { 0 };
    int64_t  m_llStudyActiveMs[5] = { 0 };
    int32_t  m_nAlertBegin[CLASSROOM_STATS_ALERT_NUM] = { 0 };

    /* ---- 在途与队列 ---- */
    OpenSeg  m_aOpenSeg[OPEN_SEG_MAX];
    LeanState m_aLean[CLASSROOM_MAX_PERSONS];
    AlertState m_aAlert[CLASSROOM_STATS_ALERT_NUM];

    static const int SEG_QUEUE_MAX = 256;
    static const int ALERT_QUEUE_MAX = 128;
    QueuedSeg   m_aSegQueue[SEG_QUEUE_MAX];
    int32_t     m_nSegHead = 0;
    int32_t     m_nSegCount = 0;
    QueuedAlert m_aAlertQueue[ALERT_QUEUE_MAX];
    int32_t     m_nAlertHead = 0;
    int32_t     m_nAlertCount = 0;

    /* 计数包：同一时刻最多挂一包未送达（周期包会被更新的累计值替换；
       收尾包带 SESSION_FINAL，未送达前不替换——它是这一节的最终值） */
    CStatsCountersMsg_S m_stPendingCounters;
    bool m_bPendingCounters = false;

    int64_t m_llLastCounterPushMonoMs = 0;
    int64_t m_llLastSegPushMonoMs = 0;
    int64_t m_llLastAlertPushMonoMs = 0;
    int64_t m_llLastRetryMonoMs = 0;

    CStatsHubStat_S m_stStat;
};
}
