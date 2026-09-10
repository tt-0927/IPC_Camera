/**
 * @FilePath     : event_linkage_dict.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 14:20:33
 * @Description  : 事件联动字典与协议映射基础模块
 */

#pragma once

#include <string>

#include "event_define.h"
#include "event_linkage_types.h"

namespace EventLinkageDict
{
/**
 * @brief   : 获取事件名称
 * @param    {Event::Type_E} enType 事件类型
 * @return   {std::string} 中文事件名称
 */
std::string get_event_name(Event::Type_E enType);

/**
 * @brief   : 使用完整事件上下文推送 TVSDK 事件
 * @param    {EventTriggerContext_S} &stContext 事件触发上下文
 * @return   {void}
 */
void push_tvsdk_event_alarm(const EventTriggerContext_S &stContext);

/**
 * @brief   : 推送 TVSDK 事件
 * @param    {Event::Type_E} enEventType 事件类型
 * @param    {bool} bEventEnded 是否结束
 * @param    {int} nChnId 通道号
 */
void push_tvsdk_event_alarm(Event::Type_E enEventType, bool bEventEnded, int nChnId);
}
