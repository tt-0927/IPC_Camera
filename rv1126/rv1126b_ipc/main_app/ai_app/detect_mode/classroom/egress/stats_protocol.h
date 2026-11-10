/**
 * @FilePath     : stats_protocol.h
 * @Author       : leiyy
 * @Date         : 2026-11-02 10:00:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 10:00:00
 * @Description  : 班级统计的线格式，零依赖纯数据层
 */

#pragma once

#include <cstdint>

namespace Classroom
{
/* ---- 命令码 ----
   取 STATISTICS 段（0x6000）内的 0x62xx 子段：现网 0x6000 只声明了客流结构，
   0x62xx 在共享头里没有对应结构体，平台按本文件的 payload 结构反序列化即可 */
#define CLASSROOM_STATS_TVSDK_BASE      0x6000u
#define CLASSROOM_STATS_TVSDK_CMD_COUNTERS (CLASSROOM_STATS_TVSDK_BASE + 0x200) /* 0x6200 */
#define CLASSROOM_STATS_TVSDK_CMD_SEGMENTS (CLASSROOM_STATS_TVSDK_BASE + 0x201) /* 0x6201 */
#define CLASSROOM_STATS_TVSDK_CMD_ALERTS   (CLASSROOM_STATS_TVSDK_BASE + 0x202) /* 0x6202 */

/* 报文头魔数 "CST1"（小端读作 0x31545343）。平台按它判"这是课堂统计而不是别的 0x62xx" */
#define CLASSROOM_STATS_MAGIC   0x31545343u
#define CLASSROOM_STATS_VERSION 1u

#define CLASSROOM_STATS_SEG_MAX   24   /* 单包段数上限；改它要同步改 stats.seg_batch_max 钳制 */
#define CLASSROOM_STATS_ALERT_MAX 8    /* 单包预警上限；改它要同步改 stats.alert_batch_max 钳制 */
#define CLASSROOM_STATS_ALERT_NUM 12   /* 预警类别数（顺序即报文数组下标，见 StatsAlert_E） */

/* 计数报文 u32Flags 位（每一位都是一个"这个数能不能这样读"的开关） */
#define CLASSROOM_STATS_FLAG_ATTEND_ENABLE    0x00000001u
#define CLASSROOM_STATS_FLAG_ABSENT_KNOWN     0x00000002u
#define CLASSROOM_STATS_FLAG_ATTEND_CLOSED    0x00000004u
#define CLASSROOM_STATS_FLAG_CLASS_STARTED    0x00000008u
#define CLASSROOM_STATS_FLAG_TEACHER_PRESENT  0x00000010u
#define CLASSROOM_STATS_FLAG_TEACHER_SEEN     0x00000020u
#define CLASSROOM_STATS_FLAG_SESSION_FINAL    0x00000040u
#define CLASSROOM_STATS_FLAG_ATTENTION_KNOWN  0x00000080u

/* 出口话题：核心只说"这是什么"，命令码由传输适配层映射一次 */
typedef enum class StatsTopic : uint8_t
{
    STATS_TOPIC_COUNTERS = 0,
    STATS_TOPIC_SEGMENTS = 1,
    STATS_TOPIC_ALERTS   = 2,
} StatsTopic_E;

inline uint32_t statsTopicCmd(StatsTopic_E enTopic)
{
    switch (enTopic)
    {
    case StatsTopic_E::STATS_TOPIC_COUNTERS: return CLASSROOM_STATS_TVSDK_CMD_COUNTERS;
    case StatsTopic_E::STATS_TOPIC_SEGMENTS: return CLASSROOM_STATS_TVSDK_CMD_SEGMENTS;
    case StatsTopic_E::STATS_TOPIC_ALERTS:   return CLASSROOM_STATS_TVSDK_CMD_ALERTS;
    default:                                 return 0;
    }
}

/* 报文类型（头里的 u16MsgType） */
typedef enum class StatsMsgType : uint16_t
{
    STATS_MSG_COUNTERS = 1,
    STATS_MSG_SEGMENTS = 2,
    STATS_MSG_ALERTS   = 3,
} StatsMsgType_E;

/* 段类型（kind）与阶段（phase） */
typedef enum class StatsSegKind : uint8_t
{
    STATS_SEG_TEACHING = 1, /* 教学行为：206 讲授 / 207 指导 / 208 板书 / 209 互动 / 210 巡视 */
    STATS_SEG_STUDY    = 2, /* 学生行为：107 听讲 / 110 实践 / 111 演示 / 108 阅读 / 109 讨论 */
} StatsSegKind_E;

typedef enum class StatsPhase : uint8_t
{
    STATS_PHASE_BEGIN  = 1, /* 记录开始；平台每收到一条 BEGIN 计一次数 */
    STATS_PHASE_UPDATE = 2, /* 进行中的值变化（预警人数/比例），**不计次** */
    STATS_PHASE_END    = 3, /* 记录结束，带时长 */
} StatsPhase_E;

/* 12 类预警。顺序 = 计数报文 nAlertBegin[] 的下标，也是平台文档里的编号 */
typedef enum class StatsAlert : int32_t
{
    STATS_ALERT_TEA_NOT_ARRIVED       = 0,  /* 教师未到达课堂 */
    STATS_ALERT_TEA_OVERRUN           = 1,  /* 教师拖堂 */
    STATS_ALERT_TEA_EARLY_LEAVE       = 2,  /* 教师早退 */
    STATS_ALERT_STU_ABSENT            = 3,  /* 学生缺勤（value=缺勤人数） */
    STATS_ALERT_STU_LEFT_MID          = 4,  /* 学生中途离课（value=人数） */
    STATS_ALERT_TEA_NO_PATROL         = 5,  /* 教师未巡视 */
    STATS_ALERT_TEA_PHONE             = 6,  /* 教师接打电话 */
    STATS_ALERT_STU_LOW_PARTICIPATION = 7,  /* 学生课堂参与度低（value=专注度百分比） */
    STATS_ALERT_STU_DISCIPLINE        = 8,  /* 学生课堂纪律差（value=违纪占比百分比） */
    STATS_ALERT_STU_GATHERING         = 9,  /* 学生聚集（value=最大聚集人数） */
    STATS_ALERT_STU_LEAN_DESK         = 10, /* 学生趴桌（value=趴桌人数） */
    STATS_ALERT_STU_PHONE             = 11, /* 学生玩手机 */
} StatsAlert_E;

const char *statsAlertName(StatsAlert_E enAlert);   /* 英文短名（日志/文档/statusJson） */
const char *statsAlertCnName(StatsAlert_E enAlert); /* 中文名（只给人看） */

/**
 * @brief 报文头（三条命令码共用，64 字节定长）
 * @note  llSessionId 是**会话幂等键**的一半：新会话从 1 重新计 u32MsgSeq，
 *        平台把 (llSessionId, u32MsgSeq) 当联合主键；同一 PK 重复到达直接丢弃。
 *        会话号取设备单调毫秒（会话起点），跨重启不保证唯一——重启后会话也重新开始，
 *        平台按 (会话墙钟, 会话号) 仍可区分。u32MsgSeq 在一次会话内跨三类报文单调递增。
 */
typedef struct _StatsHead_
{
    uint32_t u32Magic;        /* CLASSROOM_STATS_MAGIC */
    uint16_t u16Version;      /* CLASSROOM_STATS_VERSION */
    uint16_t u16MsgType;      /* StatsMsgType_E */
    uint32_t u32MsgSeq;       /* 会话内 1 起单调递增，三类报文共用一条序列 */
    uint32_t u32PayloadLen;   /* 头之后的字节数 */
    uint32_t u32Flags;        /* 计数报文的位定义见 CLASSROOM_STATS_FLAG_*；段/ 预警报文填 0 */
    uint32_t u32Channel;      /* 通道号；单通道 IPC 填 0（占用原有对齐填充位，头长度不变） */
    int64_t  llSessionId;     /* 会话起点（单调毫秒） */
    int64_t  llSessionWallMs; /* 会话起点（墙钟毫秒） */
    int64_t  llWallMs;        /* 本包生成时刻（墙钟毫秒） */
    int64_t  llMonoMs;        /* 本包生成时刻（单调毫秒） */
    int32_t  nDeviceRole;     /* 0=板机 / 1=教师机 / 2=学生机（按部署配置 role 自动填） */
    int32_t  nReserved;       /* 0 */
} CStatsHead_S;

/**
 * @brief 累计计数报文（0x6200）
 * @note  所有计数字段都是**本会话累计值**，不是增量。「规避相同累加次数」靠
 *        (会话, 序号) 幂等 + 覆盖写实现：平台收到新包直接替换旧值即可。
 *        人数类 -1 = 设备不知道（没有下发应到人数/底库未交付），不是 0。
 *        动作/表情/行为次数按 BEGIN 计：一次持续行为只算一次，不按采样拍累加。
 */
typedef struct _StatsCountersMsg_
{
    CStatsHead_S stHead;

    /* ---- 出勤 ---- */
    int32_t  nPresentNum;       /* 到课人数（已过时长门）；-1 = 未启用/未知 */
    int32_t  nNamedNum;         /* 其中已识别身份的人数 */
    int32_t  nExpectedNum;      /* 应到人数；-1 = 未知 */
    int32_t  nAbsentNum;        /* 应到−实到；>0 缺勤，<0 超员；-1 时看 FLAG_ABSENT_KNOWN */
    uint32_t u32Flags;          /* CLASSROOM_STATS_FLAG_*（与头里那份同值） */
    int32_t  nReserved0;
    int64_t  llClassStartWallMs;/* 「本课第一份有学生的样本」的墙钟；0 = 还没开始 */

    /* ---- 当前画面（最近一份群体样本）---- */
    int32_t  nPersonMax;        /* 本会话屏上人数峰值 */
    int32_t  nPersonNum;        /* 当前屏上人数（含无证据行） */
    int32_t  nStudentNum;
    int32_t  nTeacherNum;
    int32_t  nStuEvidence;      /* 当前有骨骼证据的学生数（比率分母） */

    /* ---- 学生动作次数（顺序：低头/抬头/转头/举手/站立/讨论）---- */
    int32_t  nActionBegin[6];
    /* ---- 学生表情次数（顺序：愤怒/厌恶/恐惧/快乐/中性/悲伤/惊喜）---- */
    int32_t  nExprBegin[7];
    /* ---- 教学行为（顺序：讲授/指导/板书/互动/巡视）---- */
    int32_t  nTeachBegin[5];
    int64_t  llTeachActiveMs[5];/* 该类行为累计时长（段 END 时结算，会话结束封口） */
    /* ---- 学生行为（顺序：听讲/实践/演示/阅读/讨论）---- */
    int32_t  nStudyBegin[5];
    int64_t  llStudyActiveMs[5];

    /* ---- AI 预警（顺序 = StatsAlert_E）---- */
    int32_t  nAlertBegin[CLASSROOM_STATS_ALERT_NUM]; /* 每类预警发生次数（BEGIN 才计） */

    /* ---- 当前读数 ---- */
    int32_t  nLeftMid;          /* 当前判定为中途离课的人数 */
    int32_t  nStuAttentionPct;  /* 专注度百分比 0~100；-1 = 无证据（flags 的 ATTENTION_KNOWN） */
    int32_t  nDisciplinePct;    /* 违纪占比（低头+背身+手机）/有证据学生；-1 = 无证据 */
    int32_t  nGatherNum;        /* 当前最大站立聚集人数；0 = 无 */
    int32_t  nDeskLeanNum;      /* 当前连续低头超过 stats.desk_hold_sec 的人数 */
    int32_t  nPatrolAgeSec;     /* 距上次巡视的秒数；本课还没巡过时从上课起算；-1 = 还没上课 */
    uint32_t u32AlertActiveMask;/* 当前处于激活态的预警位（位号 = StatsAlert_E）*/
    uint32_t u32Reserved1;
} CStatsCountersMsg_S;

/**
 * @brief 一条行为段（自包含：起止/时长都在里面，不依赖前后包配对）
 * @note  END 的 llBeginWallMs 可能早于本会话第一份样本：那说明行为在会话开始前
 *        就发生了，设备用 llEndWallMs - llDurationMs 反推起点，宁可起点略早也不丢段。
 */
typedef struct _StatsSegItem_
{
    uint8_t  u8Kind;          /* StatsSegKind_E */
    uint8_t  u8Phase;         /* StatsPhase_E（本结构目前只发 BEGIN/END）*/
    uint16_t u16Behavior;     /* 行为号 107/108/109/110/111 或 206~210 */
    int32_t  nPersonId;       /* 跟踪 ID；班级级哨兵为 0 */
    int32_t  nSeatId;         /* 座位号；未分配 -1 */
    int32_t  nExprClass;      /* 非表情段恒 -1（预留，表情段不走本通道）*/
    int32_t  nReserved;       /* 0 */
    int64_t  llBeginWallMs;   /* 段起点（墙钟毫秒）*/
    int64_t  llEndWallMs;     /* 段终点；BEGIN 行填 0 */
    int64_t  llDurationMs;    /* BEGIN 行填 0；END 行 = 终点−起点 */
    int64_t  llEventSeq;      /* 与报表 events[] 对账用的事件序号；0 = 没进过事件环 */
} CStatsSegItem_S;

typedef struct _StatsSegmentsMsg_
{
    CStatsHead_S   stHead;
    uint32_t       u32Count;     /* 1..CLASSROOM_STATS_SEG_MAX */
    uint32_t       u32Reserved;
    CStatsSegItem_S arrItems[CLASSROOM_STATS_SEG_MAX];
} CStatsSegmentsMsg_S;

/**
 * @brief 一条预警记录
 * @note  平台只对 phase==BEGIN 计数；UPDATE 只更新当前值，END 关闭这一段。
 *        value 的含义随类型变（人数/百分比/秒），见枚举注释与对接文档。
 */
typedef struct _StatsAlertItem_
{
    uint16_t u16Type;        /* StatsAlert_E */
    uint16_t u16Phase;       /* StatsPhase_E */
    int32_t  nValue;         /* BEGIN/UPDATE/END 时的读数；无读数填 1 */
    int64_t  llWallMs;       /* 本记录生成时刻（墙钟）*/
    int64_t  llBeginWallMs;  /* 这一段预警的起点（BEGIN 行 = llWallMs）*/
    int64_t  llDurationMs;   /* END 行 = llWallMs − llBeginWallMs；BEGIN/UPDATE 填 0 */
    int64_t  llEventSeq;     /* 预留：与事件对账；当前恒 0 */
} CStatsAlertItem_S;

typedef struct _StatsAlertsMsg_
{
    CStatsHead_S     stHead;
    uint32_t         u32Count;   /* 1..CLASSROOM_STATS_ALERT_MAX */
    uint32_t         u32Reserved;
    CStatsAlertItem_S arrItems[CLASSROOM_STATS_ALERT_MAX];
} CStatsAlertsMsg_S;

/* 平台 ABI：尺寸一改就 BUILD-FAIL，而不是让平台读到错位字段 */
static_assert(sizeof(CStatsHead_S) == 64, "CStatsHead_S must stay 64 bytes");
static_assert(sizeof(CStatsCountersMsg_S) == 376, "counters payload is a platform ABI");
static_assert(sizeof(CStatsSegItem_S) == 56, "segment item is a platform ABI");
static_assert(sizeof(CStatsSegmentsMsg_S) == 1416, "segments payload is a platform ABI");
static_assert(sizeof(CStatsAlertItem_S) == 40, "alert item is a platform ABI");
static_assert(sizeof(CStatsAlertsMsg_S) == 392, "alerts payload is a platform ABI");
}
