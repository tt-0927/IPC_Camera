/**
 * @FilePath     : event_linkage_action_direct.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 10:26:28
 * @Description  : 事件联动同步动作执行器模块
 */

#pragma once

#include "event_linkage_types.h"

class EventLinkageDirectAction
{
public:
    /**
     * @brief   : 处理报警录像联动
     * @param    {ResolvedLinkagePlan_S} &stPlan 联动计划
     * @param    {Event::EventState_S} &stEventState 当前事件状态
     * @return   {int} 0：成功 非0：失败
     */
    int deal_record(const ResolvedLinkagePlan_S &stPlan, const Event::EventState_S &stEventState);

    /**
     * @brief   : 处理抓图联动
     * @param    {ResolvedLinkagePlan_S} &stPlan 联动计划
     * @return   {int} 0：成功 非0：失败
     */
    int deal_capture_image(const ResolvedLinkagePlan_S &stPlan);

    /**
     * @brief   : 处理上传中心联动
     * @param    {ResolvedLinkagePlan_S} &stPlan 联动计划
     * @return   {int} 0：成功 非0：失败
     */
    int deal_upload(const ResolvedLinkagePlan_S &stPlan);

    /**
     * @brief   : 写入事件日志
     * @param    {Event::Info_S} stEventInfo 事件信息
     * @param    {bool} bEventEnded 是否结束
     * @return   {int} 0：成功 非0：失败
     */
    int write_log(const Event::Info_S &stEventInfo, bool bEventEnded);

private:
    /**
     * @brief   : 创建事件视频信息
     * @param    {Event::EventState_S} &stEventState 当前事件状态
     * @return   {int} 0：成功 非0：失败
     */
    int create_event_video(const Event::EventState_S &stEventState);
};
