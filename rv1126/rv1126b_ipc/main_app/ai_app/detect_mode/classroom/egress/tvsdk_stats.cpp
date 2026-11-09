/**
 * @FilePath     : tvsdk_stats.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-02 15:26:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 15:26:00
 * @Description  : 班级统计推送状态机
 */

#include "tvsdk_stats.h"

#include <cstring>

#include "duration_gate.h"

namespace Classroom
{
namespace
{
/* hold 类预警的回线滞回：条件已不成立还要再持续这么久才结束。
   没有它时"参与度在 50% 上下抖"会变成一串 1~2 秒的预警，平台按 BEGIN 计数会把
   同一次分心算成十几次；取 15s 比采样周期（默认 5s）长三倍，抖动至少跨两次采样 */
const int64_t STATS_CLEAR_HOLD_MS = 15000;

/* 推送失败后的重试间隔：节拍线程 ~10Hz，不设间隔就是每秒百次地敲一个没有客户端的
   TVSDK 服务。2s 与段批量周期同量级：恢复了也最多晚一包 */
const int64_t STATS_RETRY_INTERVAL_MS = 2000;

/* 行为号 → 三类计数/段的下标；-1 = 不在这一类里 */
int teachIndex(Behavior_E enBehavior)
{
    switch (enBehavior)
    {
    case Behavior_E::TEA_TEACHING:    return 0; /* 讲授 */
    case Behavior_E::TEA_GUIDING:     return 1; /* 指导 */
    case Behavior_E::TEA_BOARD_WRITE: return 2; /* 书写板书 */
    case Behavior_E::TEA_INTERACT:    return 3; /* 师生互动 */
    case Behavior_E::TEA_PATROL:      return 4; /* 教室巡视 */
    default:                          return -1;
    }
}

int studyIndex(Behavior_E enBehavior)
{
    switch (enBehavior)
    {
    case Behavior_E::STU_LISTEN:     return 0; /* 听讲 */
    case Behavior_E::STU_ON_PODIUM:  return 1; /* 实践（上讲台动作，代理口径） */
    case Behavior_E::STU_PRESENT:    return 2; /* 演示 */
    case Behavior_E::STU_READING:    return 3; /* 阅读 */
    case Behavior_E::STU_DISCUSS:    return 4; /* 讨论 */
    default:                         return -1;
    }
}

int actionIndex(Behavior_E enBehavior)
{
    switch (enBehavior)
    {
    case Behavior_E::STU_HEAD_DOWN:   return 0; /* 低头 */
    case Behavior_E::STU_HEAD_UP:     return 1; /* 抬头 */
    case Behavior_E::STU_TURN_BACK:   return 2; /* 转头/背身 */
    case Behavior_E::STU_RAISE_HAND:  return 3; /* 举手 */
    case Behavior_E::STU_STAND_UP:    return 4; /* 站立 */
    case Behavior_E::STU_DISCUSS:     return 5; /* 讨论 */
    default:                          return -1;
    }
}

/* 表情类别 → 平台要求的顺序（愤怒/厌恶/恐惧/快乐/中性/悲伤/惊喜）。
   刻意不复用 Expression_E 的数值顺序：平台文档定的是上面这一列，设备内部枚举
   是训练集类别序，两者一旦混用，平台会把"愤怒"画成"中性"且没有任何报错 */
int exprIndex(int8_t nExprClass)
{
    switch (static_cast<Expression_E>(nExprClass))
    {
    case Expression_E::EXP_ANGRY:   return 0;
    case Expression_E::EXP_DISGUST: return 1;
    case Expression_E::EXP_FEAR:    return 2;
    case Expression_E::EXP_HAPPY:   return 3;
    case Expression_E::EXP_NEUTRAL: return 4;
    case Expression_E::EXP_SAD:     return 5;
    case Expression_E::EXP_SURPRISE:return 6;
    default:                        return -1;
    }
}

bool isOneShotAlert(StatsAlert_E enAlert)
{
    /* 这三条一节课只该有结论一次：未到达之后到了不再是"未到达"，
       拖堂/早退是一次事实，不是可反复触发的状态 */
    return StatsAlert_E::STATS_ALERT_TEA_NOT_ARRIVED == enAlert
        || StatsAlert_E::STATS_ALERT_TEA_OVERRUN == enAlert
        || StatsAlert_E::STATS_ALERT_TEA_EARLY_LEAVE == enAlert;
}

int32_t roundPct(float fRate)
{
    if (fRate < 0.0f)
    {
        return -1;
    }
    int32_t nPct = static_cast<int32_t>(fRate * 100.0f + 0.5f);
    if (nPct < 0)
    {
        nPct = 0;
    }
    if (nPct > 100)
    {
        nPct = 100;
    }
    return nPct;
}

const char *const AST_ALERT_NAME[CLASSROOM_STATS_ALERT_NUM] = {
    "tea_not_arrived", "tea_overrun", "tea_early_leave",
    "stu_absent", "stu_left_mid", "tea_no_patrol", "tea_phone",
    "stu_low_participation", "stu_discipline", "stu_gathering",
    "stu_lean_desk", "stu_phone",
};

const char *const AST_ALERT_CN[CLASSROOM_STATS_ALERT_NUM] = {
    "教师未到达课堂", "教师拖堂", "教师早退",
    "学生缺勤", "学生中途离课", "教师未巡视", "教师接打电话",
    "学生课堂参与度低", "学生课堂纪律差", "学生聚集",
    "学生趴桌", "学生玩手机",
};
}

const char *statsAlertName(StatsAlert_E enAlert)
{
    const int nIdx = static_cast<int>(enAlert);
    if (nIdx < 0 || nIdx >= CLASSROOM_STATS_ALERT_NUM)
    {
        return "unknown";
    }
    return AST_ALERT_NAME[nIdx];
}

const char *statsAlertCnName(StatsAlert_E enAlert)
{
    const int nIdx = static_cast<int>(enAlert);
    if (nIdx < 0 || nIdx >= CLASSROOM_STATS_ALERT_NUM)
    {
        return "未知";
    }
    return AST_ALERT_CN[nIdx];
}

int32_t CStatsHub::maxStandingCluster(const CStatsPerson_S *pstPersons, int32_t nNum,
                                      float fRadiusHeads)
{
    if (nullptr == pstPersons || nNum <= 0 || fRadiusHeads <= 0.0f)
    {
        return 0;
    }

    int32_t nBest = 0;
    for (int32_t i = 0; i < nNum; ++i)
    {
        const CStatsPerson_S &stA = pstPersons[i];
        if (!stA.bStudent || !stA.bStanding)
        {
            continue;
        }
        const float fWa = (stA.fHeadW > 1.0f) ? stA.fHeadW : 1.0f;
        int32_t nCount = 0;
        for (int32_t j = 0; j < nNum; ++j)
        {
            const CStatsPerson_S &stB = pstPersons[j];
            if (!stB.bStudent || !stB.bStanding)
            {
                continue;
            }
            const float fWb = (stB.fHeadW > 1.0f) ? stB.fHeadW : 1.0f;
            /* 半径按两人头宽的平均值缩放：远焦端两个人头都小，同一个"几个人头宽"
               对应的像素距离本来就该更短；用固定像素半径会把前排的一堆算成两堆 */
            const float fR = fRadiusHeads * 0.5f * (fWa + fWb);
            const float fDx = stA.fX - stB.fX;
            const float fDy = stA.fY - stB.fY;
            if (fDx * fDx + fDy * fDy <= fR * fR)
            {
                ++nCount;
            }
        }
        if (nCount > nBest)
        {
            nBest = nCount;
        }
    }
    return nBest;
}

CStatsHub::CStatsHub()
{
    m_stLastMetric = CGroupMetric_S();
    std::memset(&m_stPendingCounters, 0, sizeof(m_stPendingCounters));
}

CStatsHub::~CStatsHub() = default;

void CStatsHub::setSink(IStatsSink *pSink)
{
    m_pSink = (nullptr != pSink) ? pSink : &m_stFnSink;
}

void CStatsHub::setPushFn(CStatsPushFn fnPush)
{
    m_stFnSink.setPushFn(std::move(fnPush));
    if (nullptr == m_pSink)
    {
        m_pSink = &m_stFnSink;
    }
}

bool CStatsHub::sinkUsable() const
{
    return (nullptr != m_pSink) && m_pSink->ready();
}

bool CStatsHub::pushWired() const
{
    return sinkUsable();
}

void CStatsHub::configure(const CStatsConf_S &stConf)
{
    m_stConf = stConf;
    if (!m_stConf.bEnable)
    {
        /* 关掉推送时不能在这里补发"收尾包"：本函数拿不到墙钟，用单调轴当墙钟发出去
           会让平台的时间轴落到 1970。统一交给引擎在 enable_off 那次 closeSession 处理；
           这里只把未送出的记录计丢并清空，避免下一节把旧记录当本节发出去 */
        if (m_nSegCount > 0)
        {
            m_stStat.u64SegLost += static_cast<uint64_t>(m_nSegCount);
        }
        if (m_nAlertCount > 0)
        {
            m_stStat.u64AlertLost += static_cast<uint64_t>(m_nAlertCount);
        }
        m_nSegCount = 0;
        m_nSegHead = 0;
        m_nAlertCount = 0;
        m_nAlertHead = 0;
        m_bPendingCounters = false;
        resetSessionState();
    }
}

void CStatsHub::setDeviceRole(DeviceRole_E enRole, int64_t llMonoMs, int64_t llWallMs)
{
    const int nRole = static_cast<int>(enRole);
    if (nRole < static_cast<int>(DeviceRole_E::ROLE_TEACHER)
        || nRole > static_cast<int>(DeviceRole_E::ROLE_BOTH))
    {
        return;    /* 配置没解析出角色：不要拿一个越界值把在跑的会话封掉 */
    }
    if (m_bRoleKnown && m_enDeviceRole == enRole)
    {
        return;    /* 每拍都会调：同角色必须无副作用 */
    }

    if (m_bSessionOpen)
    {
        /* 累计值只对同一角色有意义（学生机的出勤/动作不该带进教师机的账）。
           先按旧值封口再换角色，收尾包的设备角色因此仍是旧角色，平台能把它归到旧会话 */
        CStatsSessionSummary_S stSummary;
        stSummary.bAttendEnable = m_bAttendEnable;
        stSummary.nPresentNum = m_nPresentNum;
        stSummary.nNamedNum = m_nNamedNum;
        stSummary.nExpectedNum = m_nExpectedNum;
        stSummary.nAbsentNum = m_nAbsentNum;
        stSummary.nLeftMid = m_nLeftMid;
        closeSessionInternal(llMonoMs, llWallMs, stSummary, false);
    }
    m_enDeviceRole = enRole;
    m_bRoleKnown = true;
}

void CStatsHub::resetSessionState()
{
    m_stLastMetric = CGroupMetric_S();
    m_bSessionOpen = false;
    m_u32MsgSeq = 0;
    m_llSessionId = 0;
    m_llSessionWallMs = 0;
    m_llClassStartMonoMs = 0;
    m_llClassStartWallMs = 0;
    m_llLastSampleMonoMs = 0;

    m_nPresentNum = -1;
    m_nNamedNum = -1;
    m_nExpectedNum = -1;
    m_nAbsentNum = -1;
    m_nLeftMid = 0;
    m_bAttendEnable = false;
    m_bTeacherPresent = false;
    m_bTeacherEverSeen = false;
    m_llTeacherAbsentSinceMonoMs = 0;
    m_bPatrolKnown = false;
    m_llLastPatrolMonoMs = 0;
    m_nGatherNum = 0;
    m_nAttentionPct = -1;
    m_nDisciplinePct = -1;
    m_nPersonMax = 0;

    std::memset(m_nActionBegin, 0, sizeof(m_nActionBegin));
    std::memset(m_nExprBegin, 0, sizeof(m_nExprBegin));
    std::memset(m_nTeachBegin, 0, sizeof(m_nTeachBegin));
    std::memset(m_llTeachActiveMs, 0, sizeof(m_llTeachActiveMs));
    std::memset(m_nStudyBegin, 0, sizeof(m_nStudyBegin));
    std::memset(m_llStudyActiveMs, 0, sizeof(m_llStudyActiveMs));
    std::memset(m_nAlertBegin, 0, sizeof(m_nAlertBegin));

    for (int i = 0; i < OPEN_SEG_MAX; ++i)
    {
        m_aOpenSeg[i] = OpenSeg();
    }
    for (int i = 0; i < CLASSROOM_MAX_PERSONS; ++i)
    {
        m_aLean[i] = LeanState();
    }
    for (int i = 0; i < CLASSROOM_STATS_ALERT_NUM; ++i)
    {
        m_aAlert[i] = AlertState();
    }

    m_llLastCounterPushMonoMs = 0;
    m_llLastSegPushMonoMs = 0;
    m_llLastAlertPushMonoMs = 0;
    m_llLastRetryMonoMs = 0;
}

void CStatsHub::beginSession(int64_t llMonoMs, int64_t llWallMs)
{
    resetSessionState();
    m_bSessionOpen = true;
    m_llSessionId = llMonoMs;
    m_llSessionWallMs = llWallMs;
    m_u32MsgSeq = 0;
    ++m_stStat.u64SessionNum;
    /* last=0 让第一包计数在下一个 tick 就出去：平台面板不该等满一个 10s 周期才出现 */
    m_llLastCounterPushMonoMs = 0;
    m_llLastSegPushMonoMs = llMonoMs;
    m_llLastAlertPushMonoMs = llMonoMs;
}

bool CStatsHub::sessionPresence(const CGroupMetric_S &stMetric) const
{
    /* 学生机/板机：学生到场才算一节课（教师早到、空教室都不算）。
       教师机：机位本来就只对讲台，教师到场就是开课；上台演示的学生仍按学生算 */
    if (stMetric.nStudentNum > 0)
    {
        return true;
    }
    return m_bRoleKnown && DeviceRole_E::ROLE_TEACHER == m_enDeviceRole
           && stMetric.nTeacherNum > 0;
}

int32_t CStatsHub::deviceRoleCode() const
{
    if (!m_bRoleKnown)
    {
        return 0;
    }
    switch (m_enDeviceRole)
    {
    case DeviceRole_E::ROLE_TEACHER:
        return 1;
    case DeviceRole_E::ROLE_STUDENT:
        return 2;
    case DeviceRole_E::ROLE_BOTH:
    default:
        return 0;
    }
}

void CStatsHub::fillHead(CStatsHead_S &rstHead, uint16_t u16Type, int64_t llMonoMs,
                         int64_t llWallMs, uint32_t u32Flags)
{
    std::memset(&rstHead, 0, sizeof(rstHead));
    rstHead.u32Magic = CLASSROOM_STATS_MAGIC;
    rstHead.u16Version = CLASSROOM_STATS_VERSION;
    rstHead.u16MsgType = u16Type;
    rstHead.u32MsgSeq = ++m_u32MsgSeq;
    rstHead.u32Flags = u32Flags;
    rstHead.llSessionId = m_llSessionId;
    rstHead.llSessionWallMs = m_llSessionWallMs;
    rstHead.llWallMs = llWallMs;
    rstHead.llMonoMs = llMonoMs;
    rstHead.nDeviceRole = deviceRoleCode();
}

bool CStatsHub::pushMsg(StatsTopic_E enTopic, const void *pData, uint32_t u32Len)
{
    if (!sinkUsable() || nullptr == pData || 0 == u32Len)
    {
        return false;
    }
    bool bOk = false;
    try
    {
        bOk = m_pSink->send(enTopic, pData, u32Len);
    }
    catch (...)
    {
        bOk = false;
    }
    if (!bOk)
    {
        ++m_stStat.u64PushFail;
        m_llLastRetryMonoMs = 0; /* 由调用方在失败后置为当前时刻（见 flush*） */
    }
    return bOk;
}

void CStatsHub::enqueueSeg(const CStatsSegItem_S &stItem)
{
    if (m_nSegCount >= SEG_QUEUE_MAX)
    {
        /* 丢最旧：新记录比旧记录更接近"平台现在要显示的状态"，而队列满了说明
           平台长时间不在线——这时保留最早的几段没有意义 */
        m_nSegHead = (m_nSegHead + 1) % SEG_QUEUE_MAX;
        --m_nSegCount;
        ++m_stStat.u64SegLost;
    }
    const int32_t nTail = (m_nSegHead + m_nSegCount) % SEG_QUEUE_MAX;
    m_aSegQueue[nTail].stItem = stItem;
    ++m_nSegCount;
    ++m_stStat.u64SegRecords;
}

void CStatsHub::enqueueAlert(StatsAlert_E enType, StatsPhase_E enPhase, int32_t nValue,
                             int64_t llMonoMs, int64_t llWallMs)
{
    (void)llMonoMs;
    if (m_nAlertCount >= ALERT_QUEUE_MAX)
    {
        m_nAlertHead = (m_nAlertHead + 1) % ALERT_QUEUE_MAX;
        --m_nAlertCount;
        ++m_stStat.u64AlertLost;
    }
    CStatsAlertItem_S stItem;
    std::memset(&stItem, 0, sizeof(stItem));
    stItem.u16Type = static_cast<uint16_t>(enType);
    stItem.u16Phase = static_cast<uint16_t>(enPhase);
    stItem.nValue = nValue;
    stItem.llWallMs = llWallMs;
    if (StatsPhase_E::STATS_PHASE_END == enPhase)
    {
        stItem.llBeginWallMs = m_aAlert[static_cast<int>(enType)].llBeginWallMs;
        stItem.llDurationMs = llWallMs - stItem.llBeginWallMs;
        if (stItem.llDurationMs < 0)
        {
            stItem.llDurationMs = 0;
        }
    }
    else
    {
        stItem.llBeginWallMs = llWallMs;
    }
    const int32_t nTail = (m_nAlertHead + m_nAlertCount) % ALERT_QUEUE_MAX;
    m_aAlertQueue[nTail].stItem = stItem;
    ++m_nAlertCount;
    ++m_stStat.u64AlertRecords;
}

CStatsHub::AlertState &CStatsHub::alertOf(StatsAlert_E enType)
{
    int nIdx = static_cast<int>(enType);
    if (nIdx < 0 || nIdx >= CLASSROOM_STATS_ALERT_NUM)
    {
        nIdx = 0;
    }
    return m_aAlert[nIdx];
}

void CStatsHub::raiseAlert(StatsAlert_E enType, int32_t nValue, int64_t llMonoMs, int64_t llWallMs)
{
    AlertState &rstState = alertOf(enType);
    if (rstState.bActive)
    {
        updateAlert(enType, nValue, llMonoMs, llWallMs);
        return;
    }
    if (rstState.bOnceRaised && isOneShotAlert(enType))
    {
        return;    /* 一次性预警本会话已经报过，教师回来又走不再报第二次 */
    }

    rstState.bActive = true;
    rstState.nValue = nValue;
    rstState.llBeginMonoMs = llMonoMs;
    rstState.llBeginWallMs = llWallMs;
    rstState.llHoldSinceMonoMs = 0;
    rstState.llClearSinceMonoMs = 0;
    if (isOneShotAlert(enType))
    {
        rstState.bOnceRaised = true;
    }

    const int nIdx = static_cast<int>(enType);
    if (nIdx >= 0 && nIdx < CLASSROOM_STATS_ALERT_NUM)
    {
        ++m_nAlertBegin[nIdx];
    }
    enqueueAlert(enType, StatsPhase_E::STATS_PHASE_BEGIN, nValue, llMonoMs, llWallMs);
}

void CStatsHub::updateAlert(StatsAlert_E enType, int32_t nValue, int64_t llMonoMs, int64_t llWallMs)
{
    AlertState &rstState = alertOf(enType);
    if (!rstState.bActive || rstState.nValue == nValue)
    {
        return;
    }
    rstState.nValue = nValue;
    enqueueAlert(enType, StatsPhase_E::STATS_PHASE_UPDATE, nValue, llMonoMs, llWallMs);
}

void CStatsHub::clearAlert(StatsAlert_E enType, int64_t llMonoMs, int64_t llWallMs)
{
    AlertState &rstState = alertOf(enType);
    if (!rstState.bActive)
    {
        return;
    }
    enqueueAlert(enType, StatsPhase_E::STATS_PHASE_END, rstState.nValue, llMonoMs, llWallMs);
    rstState.bActive = false;
    rstState.llHoldSinceMonoMs = 0;
    rstState.llClearSinceMonoMs = 0;
}

void CStatsHub::closeAllAlerts(int64_t llMonoMs, int64_t llWallMs)
{
    for (int i = 0; i < CLASSROOM_STATS_ALERT_NUM; ++i)
    {
        if (!m_aAlert[i].bActive)
        {
            continue;
        }
        const StatsAlert_E enType = static_cast<StatsAlert_E>(i);
        enqueueAlert(enType, StatsPhase_E::STATS_PHASE_END, m_aAlert[i].nValue, llMonoMs, llWallMs);
        m_aAlert[i].bActive = false;
    }
}

int32_t CStatsHub::findOpenSeg(uint8_t u8Kind, uint16_t u16Behavior, int32_t nPersonId) const
{
    for (int i = 0; i < OPEN_SEG_MAX; ++i)
    {
        const OpenSeg &rstSeg = m_aOpenSeg[i];
        if (rstSeg.bUsed && rstSeg.u8Kind == u8Kind && rstSeg.u16Behavior == u16Behavior
            && rstSeg.nPersonId == nPersonId)
        {
            return i;
        }
    }
    return -1;
}

void CStatsHub::addSegActive(uint8_t u8Kind, uint16_t u16Behavior, int64_t llDurationMs)
{
    if (llDurationMs < 0)
    {
        llDurationMs = 0;
    }
    const Behavior_E enBehavior = static_cast<Behavior_E>(static_cast<int>(u16Behavior));
    if (static_cast<uint8_t>(StatsSegKind_E::STATS_SEG_TEACHING) == u8Kind)
    {
        const int nIdx = teachIndex(enBehavior);
        if (nIdx >= 0)
        {
            m_llTeachActiveMs[nIdx] += llDurationMs;
        }
    }
    else if (static_cast<uint8_t>(StatsSegKind_E::STATS_SEG_STUDY) == u8Kind)
    {
        const int nIdx = studyIndex(enBehavior);
        if (nIdx >= 0)
        {
            m_llStudyActiveMs[nIdx] += llDurationMs;
        }
    }
}

void CStatsHub::openSegment(uint8_t u8Kind, uint16_t u16Behavior, const CBehaviorEvent_S &stEvent,
                            int64_t llEventMonoMs, int64_t llEventWallMs)
{
    /* 同一 (kind, behavior, person) 已经有开着的段：先按事实把它结束在本次 BEGIN 的时刻。
       正常门状态机不会连续两个 BEGIN，但合并器/热更/手搓输入都可能造出这种序列；
       不结束旧段就等于让平台看到一段永远不结束、且被新段覆盖的行为 */
    const int32_t nOld = findOpenSeg(u8Kind, u16Behavior, stEvent.nPersonId);
    if (nOld >= 0)
    {
        CStatsSegItem_S stEnd;
        std::memset(&stEnd, 0, sizeof(stEnd));
        stEnd.u8Kind = u8Kind;
        stEnd.u8Phase = static_cast<uint8_t>(StatsPhase_E::STATS_PHASE_END);
        stEnd.u16Behavior = u16Behavior;
        stEnd.nPersonId = stEvent.nPersonId;
        stEnd.nSeatId = m_aOpenSeg[nOld].nSeatId;
        stEnd.llBeginWallMs = m_aOpenSeg[nOld].llBeginWallMs;
        stEnd.llEndWallMs = llEventWallMs;
        stEnd.llDurationMs = llEventWallMs - m_aOpenSeg[nOld].llBeginWallMs;
        if (stEnd.llDurationMs < 0)
        {
            stEnd.llDurationMs = 0;
        }
        stEnd.llEventSeq = static_cast<int64_t>(m_aOpenSeg[nOld].u64EventSeq);
        addSegActive(u8Kind, u16Behavior, stEnd.llDurationMs);
        enqueueSeg(stEnd);
        m_aOpenSeg[nOld] = OpenSeg();
    }

    int32_t nSlot = -1;
    for (int i = 0; i < OPEN_SEG_MAX; ++i)
    {
        if (!m_aOpenSeg[i].bUsed)
        {
            nSlot = i;
            break;
        }
    }
    if (nSlot < 0)
    {
        /* 在途段表满：与队列满同一处置——丢最旧（结束它），保留最新的。
           在途段上限 64 远大于"同一时刻并发的行为数"（25 列 × 1 人），
           真撞上说明有人把行为门配成了全班同时成立，属于配置异常 */
        for (int i = 1; i < OPEN_SEG_MAX; ++i)
        {
            m_aOpenSeg[i - 1] = m_aOpenSeg[i];
        }
        nSlot = OPEN_SEG_MAX - 1;
        ++m_stStat.u64SegLost;
    }

    OpenSeg &rstSlot = m_aOpenSeg[nSlot];
    rstSlot = OpenSeg();
    rstSlot.bUsed = true;
    rstSlot.u8Kind = u8Kind;
    rstSlot.u16Behavior = u16Behavior;
    rstSlot.nPersonId = stEvent.nPersonId;
    rstSlot.nSeatId = stEvent.nSeatId;
    rstSlot.nExprClass = stEvent.nExprClass;
    rstSlot.llBeginMonoMs = llEventMonoMs;
    rstSlot.llBeginWallMs = llEventWallMs;
    rstSlot.u64EventSeq = stEvent.u64EventSeq;

    CStatsSegItem_S stBegin;
    std::memset(&stBegin, 0, sizeof(stBegin));
    stBegin.u8Kind = u8Kind;
    stBegin.u8Phase = static_cast<uint8_t>(StatsPhase_E::STATS_PHASE_BEGIN);
    stBegin.u16Behavior = u16Behavior;
    stBegin.nPersonId = stEvent.nPersonId;
    stBegin.nSeatId = stEvent.nSeatId;
    stBegin.nExprClass = stEvent.nExprClass;
    stBegin.llBeginWallMs = llEventWallMs;
    stBegin.llEventSeq = static_cast<int64_t>(stEvent.u64EventSeq);
    enqueueSeg(stBegin);
}

void CStatsHub::closeAllSegments(int64_t llWallMs)
{
    for (int i = 0; i < OPEN_SEG_MAX; ++i)
    {
        if (!m_aOpenSeg[i].bUsed)
        {
            continue;
        }
        CStatsSegItem_S stEnd;
        std::memset(&stEnd, 0, sizeof(stEnd));
        stEnd.u8Kind = m_aOpenSeg[i].u8Kind;
        stEnd.u8Phase = static_cast<uint8_t>(StatsPhase_E::STATS_PHASE_END);
        stEnd.u16Behavior = m_aOpenSeg[i].u16Behavior;
        stEnd.nPersonId = m_aOpenSeg[i].nPersonId;
        stEnd.nSeatId = m_aOpenSeg[i].nSeatId;
        stEnd.nExprClass = m_aOpenSeg[i].nExprClass;
        stEnd.llBeginWallMs = m_aOpenSeg[i].llBeginWallMs;
        stEnd.llEndWallMs = llWallMs;
        stEnd.llDurationMs = llWallMs - m_aOpenSeg[i].llBeginWallMs;
        if (stEnd.llDurationMs < 0)
        {
            stEnd.llDurationMs = 0;
        }
        stEnd.llEventSeq = static_cast<int64_t>(m_aOpenSeg[i].u64EventSeq);
        addSegActive(stEnd.u8Kind, stEnd.u16Behavior, stEnd.llDurationMs);
        enqueueSeg(stEnd);
        m_aOpenSeg[i] = OpenSeg();
    }
}

void CStatsHub::onLeanBegin(const CBehaviorEvent_S &stEvent, int64_t llEventMonoMs,
                            int64_t llEventWallMs)
{
    for (int i = 0; i < CLASSROOM_MAX_PERSONS; ++i)
    {
        if (m_aLean[i].bUsed && m_aLean[i].nPersonId == stEvent.nPersonId)
        {
            return;    /* 已在计时，重复 BEGIN 不重置起点（否则一直低头的人永远到不了阈值） */
        }
    }
    for (int i = 0; i < CLASSROOM_MAX_PERSONS; ++i)
    {
        if (!m_aLean[i].bUsed)
        {
            m_aLean[i].bUsed = true;
            m_aLean[i].bAlerted = false;
            m_aLean[i].nPersonId = stEvent.nPersonId;
            m_aLean[i].llBeginMonoMs = llEventMonoMs;
            m_aLean[i].llBeginWallMs = llEventWallMs;
            return;
        }
    }
}

void CStatsHub::onLeanEnd(const CBehaviorEvent_S &stEvent, int64_t llEventMonoMs,
                          int64_t llEventWallMs)
{
    (void)llEventMonoMs;
    (void)llEventWallMs;
    for (int i = 0; i < CLASSROOM_MAX_PERSONS; ++i)
    {
        if (m_aLean[i].bUsed && m_aLean[i].nPersonId == stEvent.nPersonId)
        {
            m_aLean[i] = LeanState();
            return;
        }
    }
}

int32_t CStatsHub::countLeanDesk(int64_t llMonoMs) const
{
    const int64_t llHoldMs = static_cast<int64_t>(m_stConf.nDeskHoldSec) * 1000;
    int32_t nCount = 0;
    for (int i = 0; i < CLASSROOM_MAX_PERSONS; ++i)
    {
        if (m_aLean[i].bUsed && llMonoMs - m_aLean[i].llBeginMonoMs >= llHoldMs)
        {
            ++nCount;
        }
    }
    return nCount;
}

void CStatsHub::onEvent(const CBehaviorEvent_S &stEvent, const CStatsEventObs_S &stObs,
                        int64_t llNowMonoMs, int64_t llWallMs)
{
    if (stObs.bSegment)
    {
        ingestSegment(stEvent, llNowMonoMs, llWallMs);
    }
    if (stObs.bCount)
    {
        ingestCount(stEvent);
    }
}

void CStatsHub::ingestSegment(const CBehaviorEvent_S &stEvent, int64_t llNowMonoMs,
                              int64_t llWallMs)
{
    if (!m_stConf.bEnable || !m_bSessionOpen)
    {
        return;
    }

    /* 事件时刻是单调轴；换到墙轴用同一锚点的差值。回推到会话之前也允许——
       平台要的是"这段行为从几点到几点"，不是"设备什么时候才开机" */
    int64_t llEventMonoMs = (stEvent.llMs > 0) ? stEvent.llMs : llNowMonoMs;
    if (llEventMonoMs > llNowMonoMs)
    {
        llEventMonoMs = llNowMonoMs;
    }
    int64_t llEventWallMs = llWallMs - (llNowMonoMs - llEventMonoMs);
    if (llEventWallMs > llWallMs)
    {
        llEventWallMs = llWallMs;
    }

    const int nTeach = teachIndex(stEvent.enBehavior);
    const int nStudy = studyIndex(stEvent.enBehavior);
    if (nTeach >= 0 || nStudy >= 0)
    {
        const uint8_t u8Kind = (nTeach >= 0)
                                   ? static_cast<uint8_t>(StatsSegKind_E::STATS_SEG_TEACHING)
                                   : static_cast<uint8_t>(StatsSegKind_E::STATS_SEG_STUDY);
        const uint16_t u16Behavior = static_cast<uint16_t>(static_cast<int>(stEvent.enBehavior));

        if (stEvent.bBegin)
        {
            openSegment(u8Kind, u16Behavior, stEvent, llEventMonoMs, llEventWallMs);
        }
        else
        {
            const int32_t nSlot = findOpenSeg(u8Kind, u16Behavior, stEvent.nPersonId);
            CStatsSegItem_S stEnd;
            std::memset(&stEnd, 0, sizeof(stEnd));
            stEnd.u8Kind = u8Kind;
            stEnd.u8Phase = static_cast<uint8_t>(StatsPhase_E::STATS_PHASE_END);
            stEnd.u16Behavior = u16Behavior;
            stEnd.nPersonId = stEvent.nPersonId;
            stEnd.nSeatId = (nSlot >= 0) ? m_aOpenSeg[nSlot].nSeatId : stEvent.nSeatId;
            stEnd.nExprClass = (nSlot >= 0) ? m_aOpenSeg[nSlot].nExprClass : stEvent.nExprClass;
            stEnd.llEndWallMs = llEventWallMs;
            if (stEvent.llHeldMs > 0)
            {
                stEnd.llDurationMs = stEvent.llHeldMs;
                stEnd.llBeginWallMs = llEventWallMs - stEnd.llDurationMs;
            }
            else if (nSlot >= 0)
            {
                stEnd.llBeginWallMs = m_aOpenSeg[nSlot].llBeginWallMs;
                stEnd.llDurationMs = llEventWallMs - stEnd.llBeginWallMs;
            }
            else
            {
                /* 没配到 BEGIN 的 END：自包含一条，起点=终点（时长 0），
                   宁可让平台看到一段 0 秒的记录，也不要让它等一个永远不来的 BEGIN */
                stEnd.llBeginWallMs = llEventWallMs;
                stEnd.llDurationMs = 0;
            }
            if (stEnd.llDurationMs < 0)
            {
                stEnd.llDurationMs = 0;
            }
            stEnd.llEventSeq = static_cast<int64_t>(
                (nSlot >= 0) ? m_aOpenSeg[nSlot].u64EventSeq : stEvent.u64EventSeq);
            addSegActive(u8Kind, u16Behavior, stEnd.llDurationMs);
            enqueueSeg(stEnd);
            if (nSlot >= 0)
            {
                m_aOpenSeg[nSlot] = OpenSeg();
            }
        }
    }

    switch (stEvent.enBehavior)
    {
    case Behavior_E::TEA_PATROL:
        if (stEvent.bBegin)
        {
            m_llLastPatrolMonoMs = llEventMonoMs;
            clearAlert(StatsAlert_E::STATS_ALERT_TEA_NO_PATROL, llNowMonoMs, llWallMs);
        }
        break;

    case Behavior_E::TEA_PHONE:
        if (stEvent.bBegin)
        {
            raiseAlert(StatsAlert_E::STATS_ALERT_TEA_PHONE, 1, llNowMonoMs, llWallMs);
        }
        else
        {
            clearAlert(StatsAlert_E::STATS_ALERT_TEA_PHONE, llNowMonoMs, llWallMs);
        }
        break;

    case Behavior_E::STU_PHONE:
        if (stEvent.bBegin)
        {
            raiseAlert(StatsAlert_E::STATS_ALERT_STU_PHONE, 1, llNowMonoMs, llWallMs);
        }
        else
        {
            clearAlert(StatsAlert_E::STATS_ALERT_STU_PHONE, llNowMonoMs, llWallMs);
        }
        break;

    case Behavior_E::STU_HEAD_DOWN:
        if (stEvent.bBegin)
        {
            onLeanBegin(stEvent, llEventMonoMs, llEventWallMs);
        }
        else
        {
            onLeanEnd(stEvent, llEventMonoMs, llEventWallMs);
        }
        break;

    default:
        break;
    }
}

void CStatsHub::ingestCount(const CBehaviorEvent_S &stEvent)
{
    if (!m_stConf.bEnable || !m_bSessionOpen || !stEvent.bBegin)
    {
        return;
    }

    const int nAction = actionIndex(stEvent.enBehavior);
    if (nAction >= 0)
    {
        ++m_nActionBegin[nAction];
    }
    if (Behavior_E::STU_EXPRESSION == stEvent.enBehavior)
    {
        const int nExpr = exprIndex(stEvent.nExprClass);
        if (nExpr >= 0)
        {
            ++m_nExprBegin[nExpr];
        }
    }
    const int nTeach = teachIndex(stEvent.enBehavior);
    if (nTeach >= 0)
    {
        ++m_nTeachBegin[nTeach];
    }
    const int nStudy = studyIndex(stEvent.enBehavior);
    if (nStudy >= 0)
    {
        ++m_nStudyBegin[nStudy];
    }
}

void CStatsHub::onSample(const CStatsSample_S &stIn)
{
    if (!m_stConf.bEnable || !sinkUsable() || nullptr == stIn.pstMetric)
    {
        return;
    }
    if (!m_bSessionOpen)
    {
        if (!sessionPresence(*stIn.pstMetric))
        {
            return;    /* 还没有开课条件：教师早到/空教室不算（教师机以教师到场为条件） */
        }
        beginSession(stIn.llMonoMs, stIn.llWallMs);
    }

    m_llLastSampleMonoMs = stIn.llMonoMs;
    m_stLastMetric = *stIn.pstMetric;

    if (0 == m_llClassStartMonoMs && sessionPresence(*stIn.pstMetric))
    {
        m_llClassStartMonoMs = stIn.llMonoMs;
        m_llClassStartWallMs = stIn.llWallMs;
    }

    m_bAttendEnable = stIn.bAttendEnable;
    m_nPresentNum = stIn.bAttendEnable ? stIn.nPresentNum : -1;
    m_nNamedNum = stIn.bAttendEnable ? stIn.nNamedNum : -1;
    m_nExpectedNum = stIn.bAttendEnable ? stIn.nExpectedNum : -1;
    m_nAbsentNum = stIn.bAttendEnable ? stIn.nAbsentNum : -1;
    m_nLeftMid = stIn.nLeftMid;
    m_bPatrolKnown = stIn.bPatrolKnown;

    if (stIn.pstMetric->nPersonNum > m_nPersonMax)
    {
        m_nPersonMax = stIn.pstMetric->nPersonNum;
    }

    const bool bPresent = (stIn.pstMetric->nTeacherNum > 0);
    if (bPresent)
    {
        m_bTeacherEverSeen = true;
        m_llTeacherAbsentSinceMonoMs = 0;
    }
    else if (m_bTeacherEverSeen && 0 == m_llTeacherAbsentSinceMonoMs)
    {
        m_llTeacherAbsentSinceMonoMs = stIn.llMonoMs;
    }
    m_bTeacherPresent = bPresent;

    m_nAttentionPct = roundPct(stIn.pstMetric->fStuAttention);

    if (stIn.pstMetric->nStuEvidence > 0)
    {
        const int nColDown = CDurationGate::behaviorColumn(Behavior_E::STU_HEAD_DOWN);
        const int nColTurn = CDurationGate::behaviorColumn(Behavior_E::STU_TURN_BACK);
        const int nColPhone = CDurationGate::behaviorColumn(Behavior_E::STU_PHONE);
        int32_t nBad = 0;
        if (nColDown >= 0)
        {
            nBad += stIn.pstMetric->nActive[nColDown];
        }
        if (nColTurn >= 0)
        {
            nBad += stIn.pstMetric->nActive[nColTurn];
        }
        if (nColPhone >= 0)
        {
            nBad += stIn.pstMetric->nActive[nColPhone];
        }
        if (nBad > stIn.pstMetric->nStuEvidence)
        {
            /* nActive 是全行口径（含容忍期内），三者之和可能超过证据人数；
               纪律占比的分母是"有证据的人"，不夹会把百分比顶到 100 以上 */
            nBad = stIn.pstMetric->nStuEvidence;
        }
        m_nDisciplinePct = (nBad * 100 + stIn.pstMetric->nStuEvidence / 2) / stIn.pstMetric->nStuEvidence;
    }
    else
    {
        m_nDisciplinePct = -1;
    }

    m_nGatherNum = maxStandingCluster(stIn.pstPersons, stIn.nPersonNum,
                                      m_stConf.fGatherRadiusHeads);

    evaluateHoldAlerts(*stIn.pstMetric, stIn.llMonoMs, stIn.llWallMs);
    evaluateTimeAlerts(stIn.llMonoMs, stIn.llWallMs);

    /* 缺勤与中途离课是"人数读数"而不是事件边沿：值变了就更新，回到 0 才结束。
       平台按 BEGIN 计数，因此一次缺勤只算一次，不会因为每 10s 报一次而翻倍 */
    if (m_nAbsentNum > 0)
    {
        raiseAlert(StatsAlert_E::STATS_ALERT_STU_ABSENT, m_nAbsentNum, stIn.llMonoMs, stIn.llWallMs);
    }
    else if (0 == m_nAbsentNum)
    {
        clearAlert(StatsAlert_E::STATS_ALERT_STU_ABSENT, stIn.llMonoMs, stIn.llWallMs);
    }
    if (m_nLeftMid > 0)
    {
        raiseAlert(StatsAlert_E::STATS_ALERT_STU_LEFT_MID, m_nLeftMid, stIn.llMonoMs, stIn.llWallMs);
    }
    else
    {
        clearAlert(StatsAlert_E::STATS_ALERT_STU_LEFT_MID, stIn.llMonoMs, stIn.llWallMs);
    }
}

void CStatsHub::evaluateHoldAlerts(const CGroupMetric_S &stMetric, int64_t llMonoMs,
                                   int64_t llWallMs)
{
    /* ---- 学生课堂参与度低 ---- */
    {
        AlertState &rstState = alertOf(StatsAlert_E::STATS_ALERT_STU_LOW_PARTICIPATION);
        const bool bKnown = (stMetric.fStuAttention >= 0.0f);
        const bool bLow = bKnown && (m_nAttentionPct >= 0)
                          && (m_nAttentionPct < m_stConf.nParticipateMinPct);
        if (bLow)
        {
            rstState.llClearSinceMonoMs = 0;
            if (rstState.bActive)
            {
                updateAlert(StatsAlert_E::STATS_ALERT_STU_LOW_PARTICIPATION, m_nAttentionPct,
                            llMonoMs, llWallMs);
            }
            else
            {
                if (0 == rstState.llHoldSinceMonoMs)
                {
                    rstState.llHoldSinceMonoMs = llMonoMs;
                }
                if (llMonoMs - rstState.llHoldSinceMonoMs
                    >= static_cast<int64_t>(m_stConf.nParticipateHoldSec) * 1000)
                {
                    raiseAlert(StatsAlert_E::STATS_ALERT_STU_LOW_PARTICIPATION, m_nAttentionPct,
                               llMonoMs, llWallMs);
                }
            }
        }
        else
        {
            rstState.llHoldSinceMonoMs = 0;
            if (rstState.bActive)
            {
                if (0 == rstState.llClearSinceMonoMs)
                {
                    rstState.llClearSinceMonoMs = llMonoMs;
                }
                if (llMonoMs - rstState.llClearSinceMonoMs >= STATS_CLEAR_HOLD_MS)
                {
                    clearAlert(StatsAlert_E::STATS_ALERT_STU_LOW_PARTICIPATION, llMonoMs, llWallMs);
                }
            }
        }
    }

    /* ---- 学生课堂纪律差 ---- */
    {
        AlertState &rstState = alertOf(StatsAlert_E::STATS_ALERT_STU_DISCIPLINE);
        const bool bKnown = (stMetric.nStuEvidence > 0) && (m_nDisciplinePct >= 0);
        const bool bBad = bKnown && (m_nDisciplinePct > m_stConf.nDisciplineBadPct);
        if (bBad)
        {
            rstState.llClearSinceMonoMs = 0;
            if (rstState.bActive)
            {
                updateAlert(StatsAlert_E::STATS_ALERT_STU_DISCIPLINE, m_nDisciplinePct,
                            llMonoMs, llWallMs);
            }
            else
            {
                if (0 == rstState.llHoldSinceMonoMs)
                {
                    rstState.llHoldSinceMonoMs = llMonoMs;
                }
                if (llMonoMs - rstState.llHoldSinceMonoMs
                    >= static_cast<int64_t>(m_stConf.nDisciplineHoldSec) * 1000)
                {
                    raiseAlert(StatsAlert_E::STATS_ALERT_STU_DISCIPLINE, m_nDisciplinePct,
                               llMonoMs, llWallMs);
                }
            }
        }
        else
        {
            rstState.llHoldSinceMonoMs = 0;
            if (rstState.bActive)
            {
                if (0 == rstState.llClearSinceMonoMs)
                {
                    rstState.llClearSinceMonoMs = llMonoMs;
                }
                if (llMonoMs - rstState.llClearSinceMonoMs >= STATS_CLEAR_HOLD_MS)
                {
                    clearAlert(StatsAlert_E::STATS_ALERT_STU_DISCIPLINE, llMonoMs, llWallMs);
                }
            }
        }
    }

    /* ---- 学生聚集 ---- */
    {
        AlertState &rstState = alertOf(StatsAlert_E::STATS_ALERT_STU_GATHERING);
        const bool bGather = (m_nGatherNum >= m_stConf.nGatherNum);
        if (bGather)
        {
            rstState.llClearSinceMonoMs = 0;
            if (rstState.bActive)
            {
                updateAlert(StatsAlert_E::STATS_ALERT_STU_GATHERING, m_nGatherNum, llMonoMs, llWallMs);
            }
            else
            {
                if (0 == rstState.llHoldSinceMonoMs)
                {
                    rstState.llHoldSinceMonoMs = llMonoMs;
                }
                if (llMonoMs - rstState.llHoldSinceMonoMs
                    >= static_cast<int64_t>(m_stConf.nGatherHoldSec) * 1000)
                {
                    raiseAlert(StatsAlert_E::STATS_ALERT_STU_GATHERING, m_nGatherNum, llMonoMs, llWallMs);
                }
            }
        }
        else
        {
            rstState.llHoldSinceMonoMs = 0;
            if (rstState.bActive)
            {
                if (0 == rstState.llClearSinceMonoMs)
                {
                    rstState.llClearSinceMonoMs = llMonoMs;
                }
                if (llMonoMs - rstState.llClearSinceMonoMs >= STATS_CLEAR_HOLD_MS)
                {
                    clearAlert(StatsAlert_E::STATS_ALERT_STU_GATHERING, llMonoMs, llWallMs);
                }
            }
        }
    }

    /* ---- 学生趴桌（按人头计：当前连续低头超过阈值的人数）---- */
    {
        AlertState &rstState = alertOf(StatsAlert_E::STATS_ALERT_STU_LEAN_DESK);
        const int32_t nLean = countLeanDesk(llMonoMs);
        if (nLean > 0)
        {
            rstState.llClearSinceMonoMs = 0;
            if (rstState.bActive)
            {
                updateAlert(StatsAlert_E::STATS_ALERT_STU_LEAN_DESK, nLean, llMonoMs, llWallMs);
            }
            else
            {
                raiseAlert(StatsAlert_E::STATS_ALERT_STU_LEAN_DESK, nLean, llMonoMs, llWallMs);
            }
        }
        else
        {
            clearAlert(StatsAlert_E::STATS_ALERT_STU_LEAN_DESK, llMonoMs, llWallMs);
        }
    }
}

void CStatsHub::evaluateTimeAlerts(int64_t llMonoMs, int64_t llWallMs)
{
    if (0 == m_llClassStartMonoMs)
    {
        return;    /* 还没开课：任何时间类判据都没有基准 */
    }

    const int64_t llElapsedMs = llMonoMs - m_llClassStartMonoMs;
    const int64_t llLessonMs = static_cast<int64_t>(m_stConf.nLessonMin) * 60000;
    const int64_t llGraceArriveMs = static_cast<int64_t>(m_stConf.nArriveGraceSec) * 1000;

    /* ---- 教师未到达：开课后宽限内一次教师都没被看见 ---- */
    {
        AlertState &rstState = alertOf(StatsAlert_E::STATS_ALERT_TEA_NOT_ARRIVED);
        if (!m_bTeacherEverSeen && !rstState.bOnceRaised && llElapsedMs >= llGraceArriveMs)
        {
            raiseAlert(StatsAlert_E::STATS_ALERT_TEA_NOT_ARRIVED,
                       static_cast<int32_t>(llGraceArriveMs / 1000), llMonoMs, llWallMs);
        }
        else if (m_bTeacherEverSeen && rstState.bActive)
        {
            clearAlert(StatsAlert_E::STATS_ALERT_TEA_NOT_ARRIVED, llMonoMs, llWallMs);
        }
    }

    /* ---- 教师早退：见过教师、现在离开超过阈值、且还没到下课时间 ---- */
    {
        AlertState &rstState = alertOf(StatsAlert_E::STATS_ALERT_TEA_EARLY_LEAVE);
        const bool bMaybeLeft = m_bTeacherEverSeen && !m_bTeacherPresent
                                && m_llTeacherAbsentSinceMonoMs > 0
                                && (llMonoMs - m_llTeacherAbsentSinceMonoMs
                                    >= static_cast<int64_t>(m_stConf.nLeaveAbsentSec) * 1000)
                                && llElapsedMs < llLessonMs;
        if (bMaybeLeft)
        {
            raiseAlert(StatsAlert_E::STATS_ALERT_TEA_EARLY_LEAVE, 1, llMonoMs, llWallMs);
        }
        else if (m_bTeacherPresent && rstState.bActive)
        {
            clearAlert(StatsAlert_E::STATS_ALERT_TEA_EARLY_LEAVE, llMonoMs, llWallMs);
        }
    }

    /* ---- 教师拖堂：课时 + 宽限之后仍未收尾 ---- */
    {
        AlertState &rstState = alertOf(StatsAlert_E::STATS_ALERT_TEA_OVERRUN);
        const int64_t llOverrunMs =
            llLessonMs + static_cast<int64_t>(m_stConf.nOverrunGraceMin) * 60000;
        if (!rstState.bActive && llElapsedMs >= llOverrunMs)
        {
            raiseAlert(StatsAlert_E::STATS_ALERT_TEA_OVERRUN,
                       static_cast<int32_t>(llElapsedMs / 60000), llMonoMs, llWallMs);
        }
    }

    /* ---- 教师未巡视：巡视这一路启用、教师在讲台且超过间隔没有 210 BEGIN ---- */
    {
        AlertState &rstState = alertOf(StatsAlert_E::STATS_ALERT_TEA_NO_PATROL);
        const int64_t llIntervalMs =
            static_cast<int64_t>(m_stConf.nPatrolIntervalMin) * 60000;
        if (!m_bPatrolKnown || !m_bTeacherPresent || llIntervalMs <= 0)
        {
            if (!m_bTeacherPresent)
            {
                /* 教师不在讲台时"没巡视"已经由"未到达/早退"覆盖，不重复报 */
                clearAlert(StatsAlert_E::STATS_ALERT_TEA_NO_PATROL, llMonoMs, llWallMs);
            }
            return;
        }
        const int64_t llRefMonoMs = (m_llLastPatrolMonoMs > 0)
                                        ? m_llLastPatrolMonoMs
                                        : m_llClassStartMonoMs;
        if (!rstState.bActive && llMonoMs - llRefMonoMs >= llIntervalMs)
        {
            raiseAlert(StatsAlert_E::STATS_ALERT_TEA_NO_PATROL,
                       static_cast<int32_t>(m_stConf.nPatrolIntervalMin), llMonoMs, llWallMs);
        }
    }
}

void CStatsHub::tick(int64_t llMonoMs, int64_t llWallMs)
{
    if (!m_stConf.bEnable || !sinkUsable())
    {
        return;
    }

    if (!m_bSessionOpen)
    {
        /* 收尾包的补发：会话已经结束，但上一包没送出去。新会话期间也允许重试，
           它带的是旧会话号，平台按 (会话, 序号) 仍能正确归档 */
        if (m_bPendingCounters && (0 == m_llLastRetryMonoMs
                                   || llMonoMs - m_llLastRetryMonoMs >= STATS_RETRY_INTERVAL_MS))
        {
            if (pushMsg(StatsTopic_E::STATS_TOPIC_COUNTERS, &m_stPendingCounters,
                        static_cast<uint32_t>(sizeof(m_stPendingCounters))))
            {
                m_bPendingCounters = false;
                ++m_stStat.u64CounterMsgs;
            }
            else
            {
                m_llLastRetryMonoMs = llMonoMs;
            }
        }
        return;
    }

    evaluateTimeAlerts(llMonoMs, llWallMs);

    flushAlerts(llMonoMs, llWallMs, false);
    flushSegments(llMonoMs, llWallMs, false);
    flushCounters(llMonoMs, llWallMs, false);
}

void CStatsHub::flushCounters(int64_t llMonoMs, int64_t llWallMs, bool bForceFinal)
{
    if (!sinkUsable())
    {
        return;
    }

    const int64_t llIntervalMs = static_cast<int64_t>(m_stConf.nPushIntervalMs);
    const bool bDue = bForceFinal || (0 == m_llLastCounterPushMonoMs)
                      || (llMonoMs - m_llLastCounterPushMonoMs >= llIntervalMs);
    if (!bDue)
    {
        return;
    }
    if (m_llLastRetryMonoMs > 0 && llMonoMs - m_llLastRetryMonoMs < STATS_RETRY_INTERVAL_MS
        && !bForceFinal)
    {
        return;
    }

    /* 先补上一包：周期包重复发送只是多一份同样的累计值（平台覆盖），
       收尾包则是这一节的最终值，不能因为后面新会话的周期包而被替换 */
    if (m_bPendingCounters)
    {
        const bool bPendingFinal =
            (0 != (m_stPendingCounters.stHead.u32Flags & CLASSROOM_STATS_FLAG_SESSION_FINAL));
        if (bPendingFinal || bForceFinal)
        {
            if (!pushMsg(StatsTopic_E::STATS_TOPIC_COUNTERS, &m_stPendingCounters,
                         static_cast<uint32_t>(sizeof(m_stPendingCounters))))
            {
                m_llLastRetryMonoMs = llMonoMs;
                return;
            }
            m_bPendingCounters = false;
            ++m_stStat.u64CounterMsgs;
            if (bPendingFinal)
            {
                /* 上一节的最终值已送到；本节的新包按正常周期重算 */
                m_llLastCounterPushMonoMs = llMonoMs;
                if (!m_bSessionOpen)
                {
                    return;
                }
            }
        }
        else
        {
            /* 旧的周期包：不单独发，直接用最新累计值覆盖（累计值没有增量语义，覆盖是等价的） */
            m_bPendingCounters = false;
        }
    }

    CStatsCountersMsg_S stMsg;
    std::memset(&stMsg, 0, sizeof(stMsg));

    uint32_t u32Flags = 0;
    if (m_bAttendEnable)
    {
        u32Flags |= CLASSROOM_STATS_FLAG_ATTEND_ENABLE;
    }
    if (m_nAbsentNum >= 0)
    {
        u32Flags |= CLASSROOM_STATS_FLAG_ABSENT_KNOWN;
    }
    if (0 != m_llClassStartMonoMs)
    {
        u32Flags |= CLASSROOM_STATS_FLAG_CLASS_STARTED;
    }
    if (m_bTeacherPresent)
    {
        u32Flags |= CLASSROOM_STATS_FLAG_TEACHER_PRESENT;
    }
    if (m_bTeacherEverSeen)
    {
        u32Flags |= CLASSROOM_STATS_FLAG_TEACHER_SEEN;
    }
    if (m_nAttentionPct >= 0)
    {
        u32Flags |= CLASSROOM_STATS_FLAG_ATTENTION_KNOWN;
    }
    if (bForceFinal)
    {
        u32Flags |= CLASSROOM_STATS_FLAG_SESSION_FINAL;
        if (m_bAttendEnable)
        {
            u32Flags |= CLASSROOM_STATS_FLAG_ATTEND_CLOSED;
        }
    }

    fillHead(stMsg.stHead, static_cast<uint16_t>(StatsMsgType_E::STATS_MSG_COUNTERS),
             llMonoMs, llWallMs, u32Flags);
    stMsg.stHead.u32PayloadLen =
        static_cast<uint32_t>(sizeof(stMsg) - sizeof(stMsg.stHead));
    /* 载荷里再放一份 flags：平台按结构体反序列化时不必先认头，头部那份留给
       只看统一头的转发层。两份必须同值，否则平台从两个入口读到不同结论 */
    stMsg.u32Flags = u32Flags;

    stMsg.nPresentNum = m_nPresentNum;
    stMsg.nNamedNum = m_nNamedNum;
    stMsg.nExpectedNum = m_nExpectedNum;
    stMsg.nAbsentNum = m_nAbsentNum;
    stMsg.llClassStartWallMs = m_llClassStartWallMs;

    stMsg.nPersonMax = m_nPersonMax;
    stMsg.nPersonNum = m_stLastMetric.nPersonNum;
    stMsg.nStudentNum = m_stLastMetric.nStudentNum;
    stMsg.nTeacherNum = m_stLastMetric.nTeacherNum;
    stMsg.nStuEvidence = m_stLastMetric.nStuEvidence;

    for (int i = 0; i < 6; ++i)
    {
        stMsg.nActionBegin[i] = m_nActionBegin[i];
    }
    for (int i = 0; i < 7; ++i)
    {
        stMsg.nExprBegin[i] = m_nExprBegin[i];
    }
    for (int i = 0; i < 5; ++i)
    {
        stMsg.nTeachBegin[i] = m_nTeachBegin[i];
        stMsg.llTeachActiveMs[i] = m_llTeachActiveMs[i];
        stMsg.nStudyBegin[i] = m_nStudyBegin[i];
        stMsg.llStudyActiveMs[i] = m_llStudyActiveMs[i];
    }
    for (int i = 0; i < CLASSROOM_STATS_ALERT_NUM; ++i)
    {
        stMsg.nAlertBegin[i] = m_nAlertBegin[i];
        if (m_aAlert[i].bActive)
        {
            stMsg.u32AlertActiveMask |= (1u << i);
        }
    }

    stMsg.nLeftMid = m_nLeftMid;
    stMsg.nStuAttentionPct = m_nAttentionPct;
    stMsg.nDisciplinePct = m_nDisciplinePct;
    stMsg.nGatherNum = m_nGatherNum;
    stMsg.nDeskLeanNum = countLeanDesk(llMonoMs);
    if (0 == m_llClassStartMonoMs)
    {
        stMsg.nPatrolAgeSec = -1;
    }
    else
    {
        const int64_t llRefMonoMs = (m_llLastPatrolMonoMs > 0)
                                        ? m_llLastPatrolMonoMs
                                        : m_llClassStartMonoMs;
        stMsg.nPatrolAgeSec = static_cast<int32_t>((llMonoMs - llRefMonoMs) / 1000);
    }

    m_stPendingCounters = stMsg;
    m_bPendingCounters = true;
    if (pushMsg(StatsTopic_E::STATS_TOPIC_COUNTERS, &m_stPendingCounters,
                static_cast<uint32_t>(sizeof(m_stPendingCounters))))
    {
        m_bPendingCounters = false;
        ++m_stStat.u64CounterMsgs;
        m_llLastCounterPushMonoMs = llMonoMs;
    }
    else
    {
        m_llLastRetryMonoMs = llMonoMs;
    }
}

void CStatsHub::flushSegments(int64_t llMonoMs, int64_t llWallMs, bool bForce)
{
    if (!sinkUsable() || m_nSegCount <= 0)
    {
        return;
    }
    const int64_t llBatchMs = static_cast<int64_t>(m_stConf.nSegBatchMs);
    const bool bDue = bForce || (m_nSegCount >= m_stConf.nSegBatchMax)
                      || (llMonoMs - m_llLastSegPushMonoMs >= llBatchMs);
    if (!bDue)
    {
        return;
    }
    if (m_llLastRetryMonoMs > 0 && llMonoMs - m_llLastRetryMonoMs < STATS_RETRY_INTERVAL_MS
        && !bForce)
    {
        return;
    }

    while (m_nSegCount > 0)
    {
        int32_t nTake = m_stConf.nSegBatchMax;
        if (nTake > CLASSROOM_STATS_SEG_MAX)
        {
            nTake = CLASSROOM_STATS_SEG_MAX;
        }
        if (nTake > m_nSegCount)
        {
            nTake = m_nSegCount;
        }

        CStatsSegmentsMsg_S stMsg;
        std::memset(&stMsg, 0, sizeof(stMsg));
        fillHead(stMsg.stHead, static_cast<uint16_t>(StatsMsgType_E::STATS_MSG_SEGMENTS),
                 llMonoMs, llWallMs, 0);
        stMsg.stHead.u32PayloadLen =
            static_cast<uint32_t>(sizeof(stMsg) - sizeof(stMsg.stHead));
        stMsg.u32Count = static_cast<uint32_t>(nTake);
        for (int32_t i = 0; i < nTake; ++i)
        {
            stMsg.arrItems[i] = m_aSegQueue[(m_nSegHead + i) % SEG_QUEUE_MAX].stItem;
        }

        if (!pushMsg(StatsTopic_E::STATS_TOPIC_SEGMENTS, &stMsg,
                     static_cast<uint32_t>(sizeof(stMsg))))
        {
            m_llLastRetryMonoMs = llMonoMs;
            return;
        }
        m_nSegHead = (m_nSegHead + nTake) % SEG_QUEUE_MAX;
        m_nSegCount -= nTake;
        ++m_stStat.u64SegMsgs;
        m_llLastSegPushMonoMs = llMonoMs;
    }
}

void CStatsHub::flushAlerts(int64_t llMonoMs, int64_t llWallMs, bool bForce)
{
    if (!sinkUsable() || m_nAlertCount <= 0)
    {
        return;
    }
    const int64_t llBatchMs = static_cast<int64_t>(m_stConf.nSegBatchMs);
    const bool bDue = bForce || (m_nAlertCount >= m_stConf.nAlertBatchMax)
                      || (llMonoMs - m_llLastAlertPushMonoMs >= llBatchMs);
    if (!bDue)
    {
        return;
    }
    if (m_llLastRetryMonoMs > 0 && llMonoMs - m_llLastRetryMonoMs < STATS_RETRY_INTERVAL_MS
        && !bForce)
    {
        return;
    }

    while (m_nAlertCount > 0)
    {
        int32_t nTake = m_stConf.nAlertBatchMax;
        if (nTake > CLASSROOM_STATS_ALERT_MAX)
        {
            nTake = CLASSROOM_STATS_ALERT_MAX;
        }
        if (nTake > m_nAlertCount)
        {
            nTake = m_nAlertCount;
        }

        CStatsAlertsMsg_S stMsg;
        std::memset(&stMsg, 0, sizeof(stMsg));
        fillHead(stMsg.stHead, static_cast<uint16_t>(StatsMsgType_E::STATS_MSG_ALERTS),
                 llMonoMs, llWallMs, 0);
        stMsg.stHead.u32PayloadLen =
            static_cast<uint32_t>(sizeof(stMsg) - sizeof(stMsg.stHead));
        stMsg.u32Count = static_cast<uint32_t>(nTake);
        for (int32_t i = 0; i < nTake; ++i)
        {
            stMsg.arrItems[i] = m_aAlertQueue[(m_nAlertHead + i) % ALERT_QUEUE_MAX].stItem;
        }

        if (!pushMsg(StatsTopic_E::STATS_TOPIC_ALERTS, &stMsg,
                     static_cast<uint32_t>(sizeof(stMsg))))
        {
            m_llLastRetryMonoMs = llMonoMs;
            return;
        }
        m_nAlertHead = (m_nAlertHead + nTake) % ALERT_QUEUE_MAX;
        m_nAlertCount -= nTake;
        ++m_stStat.u64AlertMsgs;
        m_llLastAlertPushMonoMs = llMonoMs;
    }
}

void CStatsHub::closeSession(int64_t llMonoMs, int64_t llWallMs,
                             const CStatsSessionSummary_S &stSummary)
{
    closeSessionInternal(llMonoMs, llWallMs, stSummary, true);
}

void CStatsHub::closeSessionInternal(int64_t llMonoMs, int64_t llWallMs,
                                     const CStatsSessionSummary_S &stSummary, bool bEvaluateTime)
{
    if (!m_bSessionOpen)
    {
        return;
    }

    /* 最终考勤读数先落进状态：收尾包必须带账本封账后的值，而不是上一份 5s 样本 */
    m_bAttendEnable = stSummary.bAttendEnable;
    m_nPresentNum = m_bAttendEnable ? stSummary.nPresentNum : -1;
    m_nNamedNum = m_bAttendEnable ? stSummary.nNamedNum : -1;
    m_nExpectedNum = m_bAttendEnable ? stSummary.nExpectedNum : -1;
    m_nAbsentNum = m_bAttendEnable ? stSummary.nAbsentNum : -1;
    m_nLeftMid = stSummary.nLeftMid;

    if (m_nAbsentNum > 0)
    {
        raiseAlert(StatsAlert_E::STATS_ALERT_STU_ABSENT, m_nAbsentNum, llMonoMs, llWallMs);
    }
    else if (0 == m_nAbsentNum)
    {
        clearAlert(StatsAlert_E::STATS_ALERT_STU_ABSENT, llMonoMs, llWallMs);
    }
    if (m_nLeftMid > 0)
    {
        raiseAlert(StatsAlert_E::STATS_ALERT_STU_LEFT_MID, m_nLeftMid, llMonoMs, llWallMs);
    }
    else
    {
        clearAlert(StatsAlert_E::STATS_ALERT_STU_LEFT_MID, llMonoMs, llWallMs);
    }

    if (bEvaluateTime)
    {
        evaluateTimeAlerts(llMonoMs, llWallMs);
    }
    closeAllSegments(llWallMs);
    closeAllAlerts(llMonoMs, llWallMs);

    /* 收尾这一拍允许"立即"发：force 绕过周期/重试门，但仍受队列上限约束 */
    flushAlerts(llMonoMs, llWallMs, true);
    flushSegments(llMonoMs, llWallMs, true);

    /* 平台不在线时队列会占着跨会话的旧记录，而头里的 (会话, 序号) 无法在新会话里复用。
       计丢并清空：报表流仍带着同一批事件，统计通道不做跨会话补传 */
    if (m_nAlertCount > 0)
    {
        m_stStat.u64AlertLost += static_cast<uint64_t>(m_nAlertCount);
        m_nAlertCount = 0;
        m_nAlertHead = 0;
    }
    if (m_nSegCount > 0)
    {
        m_stStat.u64SegLost += static_cast<uint64_t>(m_nSegCount);
        m_nSegCount = 0;
        m_nSegHead = 0;
    }

    flushCounters(llMonoMs, llWallMs, true);
    m_bSessionOpen = false;
}

CStatsHubStat_S CStatsHub::stat() const
{
    CStatsHubStat_S stOut = m_stStat;
    stOut.nSegQueued = m_nSegCount;
    stOut.nAlertQueued = m_nAlertCount;
    stOut.llSessionId = m_llSessionId;
    stOut.bSessionOpen = m_bSessionOpen;
    return stOut;
}
}
