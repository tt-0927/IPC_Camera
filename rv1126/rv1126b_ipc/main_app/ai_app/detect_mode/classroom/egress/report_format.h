/**
 * @FilePath     : report_format.h
 * @Author       : leiyy
 * @Date         : 2026-10-19 16:40:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 16:40:00
 * @Description  : 报表 → 报文文本
 */

#pragma once

#include <cstdint>
#include <string>

#include "data_center.h"

namespace Classroom
{
/* 时间轴最多带几个点上限 */
#define CLASSROOM_REPORT_TIMELINE_MAX (64)

/* 一份报表最多带几条板书记录 */
#define CLASSROOM_BOARD_REPORT_MAX (8)

/* 轨迹打点数组的点数上限 */
#define CLASSROOM_REPORT_TRACK_MAX (120)

/* 考勤名单一条报文最多列几个人 */
#define CLASSROOM_REPORT_ROSTER_MAX (40)

/**
 * @brief   : 把一份报表素材转成报文文本
 * @param    {const CReportBundle_S&} stBundle：素材（data_center 产出）
 * @param    {int64_t} llWallMs：OUT 锚点，墙钟毫秒（CLOCK_REALTIME）
 * @param    {int64_t} llMonoMs：与上一行同一时刻的单调毫秒，用来换算各点的墙钟值
 * @return   {std::string} JSON 文本；kind 字段区分周期报表与课堂收尾报
 */
std::string reportBundleJson(const CReportBundle_S &stBundle, int64_t llWallMs, int64_t llMonoMs);

/**
 * @brief   : 即时事件推送报文：单事件一条，kind=event。
 *            周期报表仍是权威全集，两边同带 ev_seq，平台先到为准去重
 * @param    {const CBehaviorEvent_S&} stEvent：要推送的事件（u64EventSeq 已分配）
 * @param    {int64_t} llWallMs / llMonoMs：同一时刻的墙钟/单调锚点（与报表同一口径）
 * @return   {std::string} JSON 文本
 */
std::string eventPushJson(const CBehaviorEvent_S &stEvent, int64_t llWallMs, int64_t llMonoMs);

/**
 * @brief   : 教师机讲台状态段
 * @param    {const CClassPodiumState_S&} stPodium：规则引擎的发布件（见 rule_engine.h）
 * @param    {int64_t} llAgeMs：判定那一拍到现在的墙钟差（调用方算，本函数不碰时钟）；
 *            没有判过（bValid=false）时传 -1
 * @return   {std::string} `{"state":"empty|occupied|unknown","reason":"…","age_ms":N}`
 */
std::string podiumJson(const CClassPodiumState_S &stPodium, int64_t llAgeMs);

/**
 * @brief   : 课程报告页
 */
typedef struct _PanelReport_
{
    int32_t  nExpectedNum = -1;   /* 应到：-1 = 还没下发名单，网页显示"--"而不是 0 */
    int32_t  nPresentNum = -1;    /* 实到：-1 = 没在考勤（考勤关着/没底库） */
    /* 下标 = CDurationGate::behaviorColumn()，列序与门表/配置同源 */
    uint64_t au64BehaviorBegin[CDurationGate::BEHAVIOR_COLUMN_NUM] = { 0 };
    /* 下标 = Expression_E 0..6（平静..厌恶） */
    uint64_t au64ExprNum[CLASSROOM_EXPRESSION_CLASS_NUM] = { 0 };
} CPanelReport_S;

/**
 * @brief   : 六组实时读数 → JSON
 * @param    {const CPanelReport_S&} stPanel：素材（引擎从三份无损快照里取）
 * @return   {std::string} 菜单：attendance / teacher / student_basic / student_learning / emotion
 */
std::string panelReportJson(const CPanelReport_S &stPanel);

/**
 * @brief   : 上报用的墙钟时刻
 * @return   {int64_t} CLOCK_REALTIME 毫秒；未校时时落在 1970 附近，
 *            报文里的 clock_sync 字段负责承认这一点（不假装可用）
 */
int64_t reportWallMs();
}
