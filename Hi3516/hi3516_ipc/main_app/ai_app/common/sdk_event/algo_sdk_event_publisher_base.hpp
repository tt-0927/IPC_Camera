/**
 * @FilePath     : algo_sdk_event_publisher_base.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 19:29:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:21:58
 * @Description  : AI 算法 SDK 事件推送公共基类
 */

#pragma once

#include <set>

namespace AiAppCommon
{
class CAlgoSdkEventPublisherBase
{
public:
    /**
     * @brief   : 析构 SDK 事件推送基类
     * @return   {void}
     */
    virtual ~CAlgoSdkEventPublisherBase() = default;

    /**
     * @brief   : 判断当前是否存在 TVSDK 客户端
     * @return   {bool} true：存在客户端 false：不存在客户端或未启用 TVSDK
     */
    bool hasClient() const;

    /**
     * @brief   : 重置 SDK 事件活跃状态
     * @return   {void}
     */
    void resetEvent();

    /**
     * @brief   : 重置指定通道的 SDK 事件活跃状态
     * @param    {int} nChnId：通道号
     * @return   {void}
     */
    void resetEvent(int nChnId);

protected:
    /**
     * @brief   : 将算法侧通道号标准化为 TVSDK 协议通道号
     * @param    {int} nChnId：算法侧通道号
     * @return   {int} 标准化后的通道号
     */
    int normalizeChannel(int nChnId) const;

    /**
     * @brief   : 获取当前 SDK 事件是否处于活跃阶段
     * @return   {bool} true：活跃 false：未活跃
     */
    bool isEventActive(int nChnId) const;

    /**
     * @brief   : 设置 SDK 事件活跃状态
     * @param    {int} nChnId：通道号
     * @param    {bool} bActive：true：活跃 false：未活跃
     * @return   {void}
     */
    void setEventActive(int nChnId, bool bActive);

private:
    /* 按通道记录 SDK 事件活跃状态，避免多通道共用实例时互相影响首帧全景图 */
    std::set<int> m_setActiveChannels;
};
} // namespace AiAppCommon
