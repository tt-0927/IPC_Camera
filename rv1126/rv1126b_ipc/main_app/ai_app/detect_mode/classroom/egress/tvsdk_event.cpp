/**
 * @FilePath     : tvsdk_event.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-02 15:00:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-02 15:00:00
 * @Description  : 即时事件告警载荷
 */

#include "tvsdk_event.h"

#include <cstdio>
#include <cstring>

#include "classroom_define.h"

namespace Classroom
{
namespace
{
/* 行为号 → share 事件类型；UNKNOWN = 这条事件不进即时通道。
   映射的另一半（事件类型 → 告警码 0x3060/0x3061）在 share 的 get_ai_object_alarm_type() */
Event::Type_E classEventTypeOf(Behavior_E enBehavior)
{
    switch (enBehavior)
    {
    case Behavior_E::TEA_GUIDING:  return Event::Type_E::CLASSROOM_GUIDING;
    case Behavior_E::TEA_INTERACT: return Event::Type_E::CLASSROOM_INTERACT;
    default:                       return Event::Type_E::UNKNOWN;
    }
}
}

Event::Type_E classEventType(Behavior_E enBehavior)
{
    return classEventTypeOf(enBehavior);
}

bool isImmediateClassEvent(const CBehaviorEvent_S &stEvent)
{
    /* 三个条件缺一不可，少一个都会把「触发器」变成别的东西：
       - bBegin：END 问的是「结束了吗」，平台不需要它去查教师机；
       - 哨兵行：逐人行的 207/209 会随跟踪 ID 漂移，同一次站定被报成两个人；
       - 行为号：只有指导/互动两条有「去问教师机」的下游动作，210 由教师机自己答 */
    return stEvent.bBegin
        && CLASSROOM_CLASS_PERSON_ID == stEvent.nPersonId
        && Event::Type_E::UNKNOWN != classEventTypeOf(stEvent.enBehavior);
}

bool classEventContext(const CBehaviorEvent_S &stEvent, int64_t llWallMs,
                       EventTriggerContext_S &rstCtx)
{
    const Event::Type_E enType = classEventTypeOf(stEvent.enBehavior);
    if (!stEvent.bBegin || CLASSROOM_CLASS_PERSON_ID != stEvent.nPersonId ||
        Event::Type_E::UNKNOWN == enType)
    {
        return false;
    }

    rstCtx = EventTriggerContext_S();

    /* 207/209 分流：框架按这一格去查告警码（get_ai_object_alarm_type） */
    rstCtx.enEventType = enType;
    rstCtx.bEventEnded = false;     /* 只送 BEGIN，见 isImmediateClassEvent */
    rstCtx.nChnId = 0;              /* 课堂子系统只有一路画面 */

    /* 墙钟档口由调用方（引擎）取，与 deliverReport 同一口径；通道只认秒级偏移，
       但上下文里的 llTimestamp 最终进结构体的 llTimestampMs，必须是墙钟 */
    rstCtx.llTimestamp = llWallMs;

    /* 行为号进 uObjectType：平台不查第二份报文就知道这是 207 还是 209 */
    rstCtx.nObjectType = static_cast<int>(stEvent.enBehavior);
    rstCtx.nTargetId = -1;          /* 班级级事实没有具体目标，不假造一个 */
    rstCtx.fConfidence = 0.0f;      /* 同上：没有"这一次判得多准"可填 */
    /* bbox 零值是默认：班级级事件填一个像坐标的假框，平台事件图上就会画出一个点 */

    /* ev_seq 是设备侧对账键：同一事件在周期报表的 events[] 里也带这一格。
       框架的 push_ai_object_alarm 优先用这一格填 strObjectID（默认行为才是 to_string(nTargetId)）。
       没进过环（0）就照发——平台按 0 知道这份没法去重，而不是收到一个假序号 */
    char szSeq[32] = {0};
    std::snprintf(szSeq, sizeof(szSeq), "%llu",
                  static_cast<unsigned long long>(stEvent.u64EventSeq));
    rstCtx.strObjectId = szSeq;
    return true;
}
}
