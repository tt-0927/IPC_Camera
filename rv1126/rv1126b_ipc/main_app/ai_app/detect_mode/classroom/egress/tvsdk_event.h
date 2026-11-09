/**
 * @FilePath     : tvsdk_event.h
 * @Author       : leiyy
 * @Date         : 2026-11-02 15:00:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 15:00:00
 * @Description  : 班级级即时事件 → TVSDK AI 目标告警
 */

#pragma once

#include <cstdint>

#include "NetTVSDKServer.h"
#include "rule_engine.h"

namespace Classroom
{
/* 课堂即时事件的 TVSDK 告警码（AI 段，见文件头）。平台侧契约的另一半 */
#define CLASSROOM_TVSDK_ALARM_GUIDING  (NET_ALARM_BASE_AI + 0x60)    /* 207 教师指导（学生站定） */
#define CLASSROOM_TVSDK_ALARM_INTERACT (NET_ALARM_BASE_AI + 0x61)    /* 209 师生互动（站立或举手） */

/**
 * @brief   : 该事件要不要即时上送
 * @param    {const CBehaviorEvent_S&} stEvent：规则引擎产出的事件
 * @return   {bool} true=207/209 的 BEGIN 且属于班级级哨兵行
 * @note    : 只送 BEGIN：END 不承载「现在该去问教师机」这个语义，且周期报表已有全程；
 *            只送哨兵行：逐人行的 207/209 是旧口径的残留，身份会随跟踪 ID 漂移
 */
bool isImmediateClassEvent(const CBehaviorEvent_S &stEvent);

/**
 * @brief   : 把一条即时事件填成 TVSDK AI 目标告警
 * @param    {const CBehaviorEvent_S&} stEvent：事件（u64EventSeq 已分配）
 * @param    {int64_t} llWallMs：墙钟毫秒（与报表同一锚点口径）
 * @param    {NET_AlarmAiObjectInfo_S&} rstInfo：OUT，先整体清零再按文件头填字段
 * @return   {bool} false=不该上送（与 isImmediateClassEvent 同一判据）
 * @note    : 结构体约 2MB（两块 1MB 图片缓冲），调用方在堆上分配再传进来；
 *            本函数不分配、不取锁、不读时钟——纯映射
 */
bool fillClassEventAlarm(const CBehaviorEvent_S &stEvent, int64_t llWallMs,
                         NET_AlarmAiObjectInfo_S &rstInfo);
}
