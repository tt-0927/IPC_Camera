/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2025-12-29 10:05:53
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-01-15 15:40:55
 * @FilePath: /RV1126B/share/ipc_share/control/business/event/event_resource.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/**
 * @FilePath     : event_resource.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-04 19:47:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-26 16:02:47
 * @Description  : 事件资源管理
 */

#pragma once

#include <set>
#include <map>
#include <vector>
#include <iomanip>
#include <atomic>
#include <thread>
#include <mutex>

#include "alarm_define.h"
#include "event_configure.h"
#include "Singleton.h"

class CEventResource : public CSingleton<CEventResource>
{
public:
    CEventResource();
    ~CEventResource();
    friend class CSingleton<CEventResource>;

    /**
     * @brief   : 根据当前启用的智能事件，获取还可以启用的事件列表
     * @param    {SmartEventEnableStatus_S} &stStatus 传入当前智能事件的启用情况
     * @param    {vector<Event::Type_E>} &aCanEnableEvent [out] 返回还可以启用的事件类型列表
     * @return   {int} 0：成功 非0：失败
     */
    int get_canEventResource_rules(const Event::SmartEventEnableStatus_S &stStatus, std::vector<Event::Type_E> &aCanEnableEvent);

    /**
     * @brief   : 启用的智能事件，转换为类型数组
     * @param    {SmartEventEnableStatus_S} &stStatus 传入当前智能事件的启用情况
     * @param    {vector<Event::Type_E>} &aEventType [out] 返回当前启用的事件类型列表
     * @return   {*}
     */
    int enableStatus_convertArray(const Event::SmartEventEnableStatus_S &stStatus, std::vector<Event::Type_E> &aEventType);

    /**
     * @brief   : 根据智能事件启用状态的变化，自动禁用相关事件的配置
     * @note    : 当一个智能事件从启用变为禁用时，此函数会查找该事件的具体配置，并将其中的 bEnable 标志设为
     * false，同时更新总的算法配置并通知AI模块
     * @param    {Event::SmartEventEnableStatus_S} &oldStatus：变更前的启用状态
     * @param    {Event::SmartEventEnableStatus_S} &newStatus：变更后的启用状态
     */
    void update_event_configurations_on_disable(const Event::SmartEventEnableStatus_S &oldStatus,
                                                const Event::SmartEventEnableStatus_S &newStatus);

#ifdef SCENE_INTELLIGENT_ANALYSIS
    /**
    * @brief   : 根据智能事件启用状态的变化，自动开启相关事件的配置(现只用于大模型场景智能分析)
    * @note    : 当一个智能事件从禁用变为启用时，此函数会查找该事件的具体配置，并将其中的 bEnable 标志设为
    * true，同时更新总的算法配置并通知AI模块
    * @param    {Event::SmartEventEnableStatus_S} &oldStatus：变更前的启用状态
    * @param    {Event::SmartEventEnableStatus_S} &newStatus：变更后的启用状态
    * @return   {void}
    */
    void update_event_configurations_on_enable(const Event::SmartEventEnableStatus_S &oldStatus,
                                                                const Event::SmartEventEnableStatus_S &newStatus);
#endif

private:
    /**
     * @brief   : 初始化事件规则映射
     */
    // void init_rules();

    /**
     * @brief   : [模板辅助函数] 禁用指定类型的事件配置
     * @tparam   {ConfigType} 事件具体的配置结构体类型 (e.g., Alarm::BoundaryDetection_S)
     */
    template <typename ConfigType>
    void disable_specific_config();

private:
    /* 将bool状态映射到事件类型 */
    using BoolMemberPtr = bool Event::SmartEventEnableStatus_S::*;
    static const std::map<Event::Type_E, BoolMemberPtr> m_event_to_status_map;

    /* 事件到其所属互斥组的映射 */
    static const std::map<Event::Type_E, Event::SmartCategory_E> m_event_to_group_map;

    /* 存放每个组包含的所有事件 */
    static const std::map<Event::SmartCategory_E, std::set<Event::Type_E>> m_group_events;

    /* 独立事件，不受互斥规则影响 */
    static const std::set<Event::Type_E> m_independent_events;
};
