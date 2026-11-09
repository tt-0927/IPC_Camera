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
/* 行为号 → 告警码；0 = 这条事件不进即时通道 */
int classEventAlarmCode(Behavior_E enBehavior)
{
    switch (enBehavior)
    {
    case Behavior_E::TEA_GUIDING:  return CLASSROOM_TVSDK_ALARM_GUIDING;
    case Behavior_E::TEA_INTERACT: return CLASSROOM_TVSDK_ALARM_INTERACT;
    default:                       return 0;
    }
}
}

bool isImmediateClassEvent(const CBehaviorEvent_S &stEvent)
{
    /* 三个条件缺一不可，少一个都会把「触发器」变成别的东西：
       - bBegin：END 问的是「结束了吗」，平台不需要它去查教师机；
       - 哨兵行：逐人行的 207/209 会随跟踪 ID 漂移，同一次站定被报成两个人；
       - 行为号：只有指导/互动两条有「去问教师机」的下游动作，210 由教师机自己答 */
    return stEvent.bBegin
        && CLASSROOM_CLASS_PERSON_ID == stEvent.nPersonId
        && 0 != classEventAlarmCode(stEvent.enBehavior);
}

bool fillClassEventAlarm(const CBehaviorEvent_S &stEvent, int64_t llWallMs,
                         NET_AlarmAiObjectInfo_S &rstInfo)
{
    const int nCode = classEventAlarmCode(stEvent.enBehavior);
    if (!stEvent.bBegin || CLASSROOM_CLASS_PERSON_ID != stEvent.nPersonId || 0 == nCode)
    {
        return false;
    }

    /* 整体清零：图片缓冲区各 1MB，不归零就会把上一次的残留（如果有）当图发出去。
       传进来的对象通常本来就是 new 出来值初始化的，这里仍清一遍——判据不依赖调用方 */
    std::memset(&rstInfo, 0, sizeof(rstInfo));

    rstInfo.uAlarmType = static_cast<UINT32>(nCode);
    rstInfo.uChannel = 0;
    rstInfo.uObjectType = static_cast<UINT32>(static_cast<int>(stEvent.enBehavior));
    rstInfo.fConfidence = 0.0f;
    rstInfo.llTimestampMs = static_cast<INT64>(llWallMs);

    /* ev_seq 是设备侧对账键：同一事件在周期报表的 events[] 里也带这一格。
       没进过环（0）就照发——平台按 0 知道这份没法去重，而不是收到一个假序号 */
    std::snprintf(rstInfo.strObjectID, sizeof(rstInfo.strObjectID), "%llu",
                  static_cast<unsigned long long>(stEvent.u64EventSeq));
    return true;
}
}
