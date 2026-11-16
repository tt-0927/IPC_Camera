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

#include "event_linkage_types.h"
#include "rule_engine.h"

namespace Classroom
{
/* 课堂即时事件的 TVSDK 告警码（AI 段，见文件头；平台侧契约的另一半）。
 * 权威定义在 NetTVSDKServer.h，由CAP_TV3881TJY_CLASSROOM_CONFIG 守卫；本文件只引用、不本地重定义（单一真源）。
 *   207 教师指导（学生机：学生站定）  → NET_ALARM_CLASSROOM_GUIDING   = 0x3060
 *   209 师生互动（学生机：站立或举手）→ NET_ALARM_CLASSROOM_INTERACT  = 0x3061
 * 事件类型 → 告警码的映射在 share 的 get_ai_object_alarm_type()（同守卫）。 */

/**
 * @brief   : 该事件要不要即时上送
 * @param    {const CBehaviorEvent_S&} stEvent：规则引擎产出的事件
 * @return   {bool} true=207/209 的 BEGIN 且属于班级级哨兵行
 * @note    : 只送 BEGIN：END 不承载「现在该去问教师机」这个语义，且周期报表已有全程；
 *            只送哨兵行：逐人行的 207/209 是旧口径的残留，身份会随跟踪 ID 漂移
 */
bool isImmediateClassEvent(const CBehaviorEvent_S &stEvent);

/**
 * @brief   : 课堂行为号 → share 事件类型（框架映射键）
 * @param    {Behavior_E} enBehavior：课堂行为号
 * @return   {Event::Type_E} 207/209 的两个课堂类型；其它返回 UNKNOWN（不进即时通道）
 */
Event::Type_E classEventType(Behavior_E enBehavior);

/**
 * @brief   : 把一条即时事件填成框架的触发上下文（**不填 SDK 结构体**，那是框架 pusher 的事）
 * @param    {const CBehaviorEvent_S&} stEvent：事件（u64EventSeq 已分配）
 * @param    {int64_t} llWallMs：墙钟毫秒（与报表同一锚点口径）
 * @param    {EventTriggerContext_S&} rstCtx：OUT，先整体重置再按文件头填字段
 * @return   {bool} false=不该上送（与 isImmediateClassEvent 同一判据）
 * @note    : 纯函数：不分配、不取锁、不读时钟、不碰 SDK——主机用例可直接调用。
 *            调用方拿上下文去调 EventLinkageDict::push_tvsdk_event_alarm 即完成推送
 */
bool classEventContext(const CBehaviorEvent_S &stEvent, int64_t llWallMs,
                       EventTriggerContext_S &rstCtx);
}
