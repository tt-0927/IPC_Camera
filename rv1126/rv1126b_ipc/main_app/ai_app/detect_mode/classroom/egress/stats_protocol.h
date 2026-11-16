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

#include "NetTVSDKServer.h"

#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG

namespace Classroom
{
/* ---- 命令码 ----
   权威定义在NetTVSDKServer.h（NET_ALARM_CLASSROOM_STATISTICS_BASE/COUNTERS/SEGMENTS/ALERTS，由
   CAP_TV3881TJY_CLASSROOM_CONFIG 守卫）。这里取别名供产品代码旧称呼使用，
   别名不可能与 share 漂移——漂移的防线是下面那条钉死字面值的 static_assert：
   share 侧改了码值 ⇒ 产品构建当场失败（平台契约是 0x6200/0x6201/0x6202） */
#define CLASSROOM_STATS_TVSDK_BASE      NET_ALARM_CLASSROOM_STATISTICS_BASE
#define CLASSROOM_STATS_TVSDK_CMD_COUNTERS NET_ALARM_CLASSROOM_STATISTICS_COUNTERS
#define CLASSROOM_STATS_TVSDK_CMD_SEGMENTS NET_ALARM_CLASSROOM_STATISTICS_SEGMENTS
#define CLASSROOM_STATS_TVSDK_CMD_ALERTS   NET_ALARM_CLASSROOM_STATISTICS_ALERTS
static_assert(NET_ALARM_CLASSROOM_STATISTICS_BASE == 0x6200u, "classroom stats base must stay 0x6200");
static_assert(NET_ALARM_CLASSROOM_STATISTICS_COUNTERS == 0x6200u, "counters cmd must stay 0x6200");
static_assert(NET_ALARM_CLASSROOM_STATISTICS_SEGMENTS == 0x6201u, "segments cmd must stay 0x6201");
static_assert(NET_ALARM_CLASSROOM_STATISTICS_ALERTS == 0x6202u, "alerts cmd must stay 0x6202");

/* 协议常量别名（权威定义同样在 share 协议头） */
#define CLASSROOM_STATS_MAGIC   NET_CLASSROOM_STATS_MAGIC
#define CLASSROOM_STATS_VERSION NET_CLASSROOM_STATS_VERSION
#define CLASSROOM_STATS_SEG_MAX   NET_CLASSROOM_STATS_SEG_MAX   /* 单包段数上限；改它要同步改 stats.seg_batch_max 钳制 */
#define CLASSROOM_STATS_ALERT_MAX NET_CLASSROOM_STATS_ALERT_MAX /* 单包预警上限；改它要同步改 stats.alert_batch_max 钳制 */
#define CLASSROOM_STATS_ALERT_NUM NET_CLASSROOM_STATS_ALERT_NUM /* 预警类别数（顺序即报文数组下标，见 StatsAlert_E） */

/* 计数报文 u32Flags 位（每一位都是一个"这个数能不能这样读"的开关） */
#define CLASSROOM_STATS_FLAG_ATTEND_ENABLE    NET_CLASSROOM_STATS_FLAG_ATTEND_ENABLE
#define CLASSROOM_STATS_FLAG_ABSENT_KNOWN     NET_CLASSROOM_STATS_FLAG_ABSENT_KNOWN
#define CLASSROOM_STATS_FLAG_ATTEND_CLOSED    NET_CLASSROOM_STATS_FLAG_ATTEND_CLOSED
#define CLASSROOM_STATS_FLAG_CLASS_STARTED    NET_CLASSROOM_STATS_FLAG_CLASS_STARTED
#define CLASSROOM_STATS_FLAG_TEACHER_PRESENT  NET_CLASSROOM_STATS_FLAG_TEACHER_PRESENT
#define CLASSROOM_STATS_FLAG_TEACHER_SEEN     NET_CLASSROOM_STATS_FLAG_TEACHER_SEEN
#define CLASSROOM_STATS_FLAG_SESSION_FINAL    NET_CLASSROOM_STATS_FLAG_SESSION_FINAL
#define CLASSROOM_STATS_FLAG_ATTENTION_KNOWN  NET_CLASSROOM_STATS_FLAG_ATTENTION_KNOWN

/* 载荷结构体：协议头定义（平台侧与其它 NET_* 载荷同列可读），这里只取产品侧旧称呼 */
typedef NET_ClassroomStatsHead_S     CStatsHead_S;
typedef NET_ClassroomStatsCounters_S CStatsCountersMsg_S;
typedef NET_ClassroomStatsSegItem_S  CStatsSegItem_S;
typedef NET_ClassroomStatsSegments_S CStatsSegmentsMsg_S;
typedef NET_ClassroomStatsAlertItem_S CStatsAlertItem_S;
typedef NET_ClassroomStatsAlerts_S   CStatsAlertsMsg_S;

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

/* 平台 ABI：与 share 协议头那份同参数的双保险——任一侧布局变了都 BUILD-FAIL */
static_assert(sizeof(CStatsHead_S) == 64, "CStatsHead_S must stay 64 bytes");
static_assert(sizeof(CStatsCountersMsg_S) == 376, "counters payload is a platform ABI");
static_assert(sizeof(CStatsSegItem_S) == 56, "segment item is a platform ABI");
static_assert(sizeof(CStatsSegmentsMsg_S) == 1416, "segments payload is a platform ABI");
static_assert(sizeof(CStatsAlertItem_S) == 40, "alert item is a platform ABI");
static_assert(sizeof(CStatsAlertsMsg_S) == 392, "alerts payload is a platform ABI");
}

#else
#error "stats_protocol.h requires CAP_TV3881TJY_CLASSROOM_CONFIG=1 (classroom stats protocol is TV-3881TJY specific)"
#endif
