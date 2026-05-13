/**
 * @FilePath     : event_linkage.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-30 14:18:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 10:39:02
 * @Description  : 事件联动门面层
 */

#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "Singleton.h"
#include "event_linkage_types.h"

class EventLinkageResolver;
class EventLinkageDispatcher;
class EventLinkageWorker;
class EventLinkageDirectAction;
class EventLinkageAsyncAction;

class CEventLinkage : public CSingleton<CEventLinkage>
{
public:
    CEventLinkage();
    ~CEventLinkage();
    friend class CSingleton<CEventLinkage>;

    /**
     * @brief   : 初始化联动管理器
     * @return   {int} 0：成功 非0：失败
     */
    int init();

    /**
     * @brief   : 反初始化联动管理器
     * @return   {int} 0：成功 非0：失败
     */
    int deinit();

    /**
     * @brief   : 兼容旧入口处理事件联动
     * @param    {Event::Type_E} enEventType 事件类型
     * @param    {bool} bEventEnded 是否结束
     * @return   {bool} true：处理成功 false：被时间窗过滤或处理失败
     */
    bool handleEvent(Event::Type_E enEventType, bool bEventEnded = false);

    /**
     * @brief   : 新上下文入口处理事件联动
     * @param    {EventTriggerContext_S} &stContext 事件触发上下文
     * @return   {bool} true：处理成功 false：被时间窗过滤或处理失败
     */
    bool handleEvent(const EventTriggerContext_S &stContext);

    /**
     * @brief   : 获取最近一次事件信息
     * @param    {Event::Info_S} &stEvent 事件信息
     */
    void get_event(Event::Info_S &stEvent);

    /**
     * @brief   : 直接播放音频
     * @param    {std::string} strAudioPath 音频路径
     * @param    {int} nTimes 播放次数
     */
    void play_audio(std::string strAudioPath, int nTimes);

    /**
     * @brief   : 停止正在播放的音频
     * @return   {bool} true：已停止或原本未播放 false：worker 未初始化
     */
    bool stop_play_audio();

    /**
     * @brief   : 清理已经结束的事件
     */
    void remove_EndedEvents();

    /**
     * @brief   : 获取当前事件个数
     * @return   {int} 当前事件个数
     */
    int get_EventInfoMapSize();

private:
    /**
     * @brief   : 构造事件信息
     * @param    {EventTriggerContext_S} &stContext 事件触发上下文
     * @return   {Event::Info_S} 事件信息快照
     */
    Event::Info_S create_eventInfo(const EventTriggerContext_S &stContext) const;

    /**
     * @brief   : 在持锁状态下清理结束事件
     */
    void remove_ended_events_locked();

private:
    /* 联动规则解析器，负责规则命中与默认回退 */
    std::unique_ptr<EventLinkageResolver> m_resolver;
    /* 同步动作执行器，负责录像/抓图/日志等即时动作 */
    std::unique_ptr<EventLinkageDirectAction> m_directAction;
    /* 异步动作执行器，负责邮件/音频/灯光/IO 等异步动作 */
    std::unique_ptr<EventLinkageAsyncAction> m_asyncAction;
    /* 异步 worker，负责优先级队列与并发治理 */
    std::unique_ptr<EventLinkageWorker> m_worker;
    /* 联动调度器，负责 plan 拆分为同步与异步动作 */
    std::unique_ptr<EventLinkageDispatcher> m_dispatcher;

    /* 事件时间窗时间戳表，用于抑制时间窗内重复触发 */
    std::map<Event::Type_E, long long> m_eventTimeStampsMap;
    /* 当前活跃事件状态表，用于事件开始/结束生命周期管理 */
    std::map<Event::Type_E, Event::EventState_S> m_eventInfoMap;
    /* 事件状态互斥锁，保护事件信息和时间窗状态 */
    mutable std::mutex m_mutex;

    /* 最近一次处理的事件信息快照 */
    Event::Info_S m_stEventInfo;
    /* 报警时间窗，单位毫秒 同步海康事件触发过滤时间 10s */
    long long m_llTimeWindow = 10 * 1000;
    /* 初始化状态标志 */
    bool m_bInited = false;
};
