/**
 * @FilePath     : event_linkage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-30 14:18:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-01 14:34:30
 * @Description  : 事件联动门面层实现
 */

#include "event_linkage.h"

#include "event_linkage_action_async.h"
#include "event_linkage_action_direct.h"
#include "event_linkage_dict.h"
#include "event_linkage_dispatcher.h"
#include "event_linkage_resolver.h"
#include "event_linkage_worker.h"
#include "time_utils.h"
#include "dlog.h"
#include "IpcRet.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace
{
struct EventTimeParts_S
{
    std::string strDateCompact;
    std::string strDateDash;
    std::string strTimeCompactMs;
    std::string strTimeColon;
    std::string strDateTimeDash;
};

EventTimeParts_S build_event_time_parts(long long llTimestamp)
{
    if (llTimestamp <= 0)
    {
        llTimestamp = TimeUtils_NS::get_currentTimestampMs();
    }

    const std::time_t seconds = static_cast<std::time_t>(llTimestamp / 1000);
    const int millis = static_cast<int>(llTimestamp % 1000);
    struct tm tmValue;
    localtime_r(&seconds, &tmValue);

    EventTimeParts_S stParts;
    {
        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%Y%m%d");
        stParts.strDateCompact = oss.str();
    }
    {
        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%Y-%m-%d");
        stParts.strDateDash = oss.str();
    }
    {
        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%H%M%S") << std::setw(3) << std::setfill('0') << millis;
        stParts.strTimeCompactMs = oss.str();
    }
    {
        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%H:%M:%S");
        stParts.strTimeColon = oss.str();
    }
    stParts.strDateTimeDash = stParts.strDateDash + " " + stParts.strTimeColon;
    return stParts;
}
} // namespace

CEventLinkage::CEventLinkage()
{
    init();
}

CEventLinkage::~CEventLinkage()
{
    deinit();
}

int CEventLinkage::init()
{
    if (m_bInited)
    {
        return OK;
    }

    /* 按职责分别创建规则解析、同步执行、异步执行、调度与工作线程对象 */
    m_resolver.reset(new EventLinkageResolver());
    m_directAction.reset(new EventLinkageDirectAction());
    m_asyncAction.reset(new EventLinkageAsyncAction());
    m_worker.reset(new EventLinkageWorker(*m_asyncAction));
    m_dispatcher.reset(new EventLinkageDispatcher(*m_directAction, *m_worker));

    m_worker->init();
    m_bInited = true;
    dlog_info("事件联动模块初始化成功");
    return OK;
}

int CEventLinkage::deinit()
{
    if (!m_bInited)
    {
        return OK;
    }

    if (m_worker)
    {
        m_worker->deinit();
    }

    m_dispatcher.reset();
    m_worker.reset();
    m_asyncAction.reset();
    m_directAction.reset();
    m_resolver.reset();
    m_bInited = false;

    dlog_info("事件联动模块反初始化成功");
    return OK;
}

bool CEventLinkage::handleEvent(Event::Type_E enEventType, bool bEventEnded)
{
    EventTriggerContext_S stContext;
    stContext.enEventType = enEventType;
    stContext.bEventEnded = bEventEnded;
    stContext.nChnId = 0;
    stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
    return handleEvent(stContext);
}

bool CEventLinkage::handleEvent(const EventTriggerContext_S &stInputContext)
{
    if (!m_bInited && init() != OK)
    {
        return false;
    }

    /* 补齐运行时上下文，确保后续链路拿到完整时间信息 */
    EventTriggerContext_S stContext = stInputContext;
    if (stContext.llTimestamp <= 0)
    {
        stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
    }

    /* 先生成一份事件快照，后续写日志和数据库都基于这份数据 */
    Event::Info_S stCreatedEventInfo = create_eventInfo(stContext);
    Event::EventState_S stEventState;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        dlog_info("事件 %s[%d] %s",
                  EventLinkageDict::get_event_name(stContext.enEventType).c_str(),
                  static_cast<int>(stContext.enEventType),
                  stContext.bEventEnded ? "结束" : "触发");

        if (!stContext.bEventEnded)
        {
            stEventState.stEventInfo = stCreatedEventInfo;
            stEventState.nEventStatus = 1;
        }
        else
        {
            /* 事件结束时尽量复用开始阶段留下的事件信息，补齐结束时间 */
            auto eventIt = m_eventInfoMap.find(stContext.enEventType);
            if (eventIt != m_eventInfoMap.end())
            {
                stEventState = eventIt->second;
                stEventState.nEventStatus = 0;
                stEventState.stEventInfo.strEndTime = stCreatedEventInfo.strEndTime;
                stEventState.stEventInfo.strDate = stCreatedEventInfo.strDate;
                // stEventState.stEventInfo.strTime = stCreatedEventInfo.strTime;
            }
            else
            {
                dlog_warn("事件结束时未找到对应事件记录，事件类型: %d", static_cast<int>(stContext.enEventType));
                stEventState.stEventInfo = stCreatedEventInfo;
                stEventState.nEventStatus = 0;
            }
        }

        m_stEventInfo = stEventState.stEventInfo;
        m_eventInfoMap[stContext.enEventType] = stEventState;
    }

    /* 同步执行日志写入和TVSDK通知，这两类动作需要立刻完成 */
    m_directAction->write_log(stEventState.stEventInfo, stContext.bEventEnded);
    EventLinkageDict::push_tvsdk_event_alarm(stContext);

    /* 解析本次事件最终应该触发的联动，再交给调度器拆分执行 */
    ResolvedLinkagePlan_S stPlan;
    if (m_resolver->resolve(stContext, stEventState.stEventInfo, stPlan) == OK)
    {
        if(stContext.enEventType == Event::Type_E::FACE_COMPARE_SUCCESS)
        {
            stPlan.bSound = true;
        }
        m_dispatcher->dispatch(stPlan, stEventState);
    }

    if (stContext.bEventEnded)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        /* 结束事件处理完成后，从缓存表中移除已结束记录 */
        remove_ended_events_locked();
    }

    return true;
}

void CEventLinkage::get_event(Event::Info_S &stEvent)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    stEvent = m_stEventInfo;
}

void CEventLinkage::play_audio(std::string strAudioPath, int nTimes)
{
    if (!m_bInited && init() != OK)
    {
        return;
    }

    m_asyncAction->play_audio(strAudioPath, nTimes, m_worker->getRunningFlag(LinkageType_E::SOUND));
}

bool CEventLinkage::stop_play_audio()
{
    if (!m_worker)
    {
        return false;
    }

    dlog_info("停止声音联动");
    return m_worker->stopTask(LinkageType_E::SOUND);
}

void CEventLinkage::remove_EndedEvents()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    remove_ended_events_locked();
}

int CEventLinkage::get_EventInfoMapSize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_eventInfoMap.size());
}

Event::Info_S CEventLinkage::create_eventInfo(const EventTriggerContext_S &stContext) const
{
    Event::Info_S stEventInfo;
    /* 统一生成事件时间字段，确保各联动动作读取到一致的时间快照 */
    const EventTimeParts_S stTimeParts = build_event_time_parts(stContext.llTimestamp);
    stEventInfo.enType = stContext.enEventType;
    stEventInfo.nChnId = stContext.nChnId;
    stEventInfo.strDate = stTimeParts.strDateCompact;
    stEventInfo.strTime = stTimeParts.strTimeCompactMs;
    stEventInfo.lTimestamp = stContext.llTimestamp;
    stEventInfo.strStartTime = stTimeParts.strDateTimeDash;
    stEventInfo.strEndTime = stEventInfo.strStartTime;
    return stEventInfo;
}

void CEventLinkage::remove_ended_events_locked()
{
    for (auto it = m_eventInfoMap.begin(); it != m_eventInfoMap.end();)
    {
        /* 仅保留仍处于进行中的事件，结束态事件在本轮统一清掉 */
        if (it->second.nEventStatus == 0)
        {
            it = m_eventInfoMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
