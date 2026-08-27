/**
 * @FilePath     : event_linkage_resolver.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 10:24:54
 * @Description  : 事件联动规则解析器
 */

#pragma once

#include <mutex>

#include "event_linkage_types.h"

class EventLinkageResolver
{
public:
    /**
     * @brief   : 解析事件联动计划
     * @param    {EventTriggerContext_S} &stContext 触发上下文
     * @param    {Event::Info_S} &stEventInfo 事件信息
     * @param    {ResolvedLinkagePlan_S} &stPlan 联动计划
     * @return   {int} 0：成功 非0：失败
     */
    int resolve(const EventTriggerContext_S &stContext,
                const Event::Info_S &stEventInfo,
                ResolvedLinkagePlan_S &stPlan);

private:
    /**
     * @brief   : 从策略集中查找当前上下文命中的最优扩展规则
     * @param    {LinkagePolicySet_S} &stPolicySet 扩展策略集
     * @param    {EventTriggerContext_S} &stContext 触发上下文
     * @return   {const LinkageRule_S *} 命中的最优规则；未命中时返回 nullptr
     */
    const LinkageRule_S *find_best_rule(const LinkagePolicySet_S &stPolicySet,
                                        const EventTriggerContext_S &stContext) const;

    /**
     * @brief   : 应用命中的扩展联动规则到当前联动计划
     * @param    {LinkageRule_S} &stRule 命中的扩展规则
     * @param    {ResolvedLinkagePlan_S} &stPlan 联动计划
     * @return   {void}
     */
    void apply_extended_rule(const LinkageRule_S &stRule, ResolvedLinkagePlan_S &stPlan) const;

    /**
     * @brief   : 加载默认联动配置
     * @param    {Event::Type_E} enEventType 事件类型
     * @param    {Alarm::LinkageList_S} &stLinkageList 联动列表
     * @return   {int} 0：成功 非0：失败
     */
    int load_default_linkage_list(Event::Type_E enEventType, Alarm::LinkageList_S &stLinkageList);

    /**
     * @brief   : 加载扩展联动策略集
     * @param    {LinkagePolicySet_S} &stPolicySet 策略集
     * @return   {int} 0：成功 非0：失败
     */
    int load_policy_set(LinkagePolicySet_S &stPolicySet) const;

    /**
     * @brief   : 匹配单条联动规则
     * @param    {LinkageRule_S} &stRule 联动规则
     * @param    {EventTriggerContext_S} &stContext 触发上下文
     * @return   {bool} true：命中 false：未命中
     */
    bool match_rule(const LinkageRule_S &stRule, const EventTriggerContext_S &stContext) const;

    /**
     * @brief   : 匹配单个属性条件
     * @param    {LinkageAttrMatch_S} &stAttrMatch 属性匹配条件
     * @param    {EventTriggerContext_S} &stContext 触发上下文
     * @return   {bool} true：命中 false：未命中
     */
    bool match_attr(const LinkageAttrMatch_S &stAttrMatch, const EventTriggerContext_S &stContext) const;

    /**
     * @brief   : 填充联动计划中的派生字段
     * @param    {ResolvedLinkagePlan_S} &stPlan 联动计划
     */
    void fill_plan_flags(ResolvedLinkagePlan_S &stPlan) const;

private:
    /* 策略缓存互斥锁，保护策略文件的懒加载缓存 */
    mutable std::mutex m_policyMutex;
    /* 策略是否已完成加载 */
    mutable bool m_bPolicyLoaded = false;
    /* 已缓存的联动策略集合 */
    mutable LinkagePolicySet_S m_cachedPolicySet;
};
