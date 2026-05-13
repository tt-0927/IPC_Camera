/**
 * @FilePath     : event_linkage_dispatcher.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 10:25:14
 * @Description  : 事件联动调度器
 */

#pragma once

#include "event_linkage_action_direct.h"
#include "event_linkage_worker.h"

class EventLinkageDispatcher
{
public:
    EventLinkageDispatcher(EventLinkageDirectAction &stDirectAction, EventLinkageWorker &stWorker);

    /**
     * @brief   : 调度联动计划
     * @param    {ResolvedLinkagePlan_S} &stPlan 联动计划
     * @param    {Event::EventState_S} &stEventState 当前事件状态
     * @return   {int} 0：成功 非0：失败
     */
    int dispatch(const ResolvedLinkagePlan_S &stPlan, const Event::EventState_S &stEventState);

private:
    /**
     * @brief   : 构造联动异步任务
     * @param    {ResolvedLinkagePlan_S} &stPlan 联动计划
     * @param    {LinkageType_E} enLinkageType 联动类型
     * @return   {LinkageTask_S} 联动任务
     */
    LinkageTask_S build_task(const ResolvedLinkagePlan_S &stPlan, LinkageType_E enLinkageType) const;

private:
    /* 同步动作执行器引用 */
    EventLinkageDirectAction &m_directAction;
    /* 异步 worker 引用 */
    EventLinkageWorker &m_worker;
};
