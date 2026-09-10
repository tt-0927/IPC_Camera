/**
 * @FilePath     : people_flow_track_store.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 人流统计轨迹状态存储，以 Channel+Rule+Track ID 为真实 Key
 */

#pragma once

#if CAP_AI_PEOPLE_STATISTICS

#include <cstdint>
#include <unordered_map>

#include "people_flow_types.hpp"

namespace AiPipeline_NS
{
namespace PeopleFlow_NS
{
/* 人流统计轨迹状态存储 */
class CPeopleFlowTrackStore
{
public:
    /**
     * @brief   : 构造轨迹状态存储
     */
    CPeopleFlowTrackStore();

    /**
     * @brief   : 更新或创建轨迹状态
     * @param    {const TrackKey_S &} stKey：轨迹 Key
     * @param    {const Geometry_NS::Point_S &} stPosition：当前底边中点
     * @param    {int64_t} llNowMs：当前 monotonic 毫秒时间戳
     * @return   {int} OK：成功，ERR：容量满且超时清理后仍无空间
     * @note    : 首帧创建状态并返回成功，调用方通过 has_last_position 判断轨迹起点
     */
    int update(const TrackKey_S &stKey, const Geometry_NS::Point_S &stPosition, int64_t llNowMs);

    /**
     * @brief   : 立即删除轨迹状态，ENDED 目标调用
     * @param    {const TrackKey_S &} stKey：轨迹 Key
     * @return   {void}
     */
    void erase(const TrackKey_S &stKey);

    /**
     * @brief   : 清理超过 TTL 未更新的轨迹状态
     * @param    {int64_t} llNowMs：当前 monotonic 毫秒时间戳
     * @return   {size_t} 清理数量
     */
    size_t cleanup_expired(int64_t llNowMs);

    /**
     * @brief   : 查询轨迹状态是否存在
     * @param    {const TrackKey_S &} stKey：轨迹 Key
     * @return   {bool} true：存在
     */
    bool contains(const TrackKey_S &stKey) const;

    /**
     * @brief   : 读取轨迹状态
     * @param    {const TrackKey_S &} stKey：轨迹 Key
     * @param    {TrackState_S &} stOutState：输出轨迹状态
     * @return   {bool} true：存在并输出
     */
    bool get(const TrackKey_S &stKey, TrackState_S &stOutState) const;

    /**
     * @brief   : 清空全部轨迹状态
     * @return   {void}
     */
    void reset();

    /**
     * @brief   : 获取当前轨迹数量
     * @return   {size_t} 轨迹数量
     */
    size_t size() const;

private:
    /* 轨迹状态容器，Key 为 Channel+Rule+Track ID */
    std::unordered_map<TrackKey_S, TrackState_S, TrackKeyHash_S> m_mapTracks;
    /* 轨迹容量上限 */
    uint32_t m_nCapacity = PEOPLE_FLOW_TRACK_CAPACITY;
};
} // namespace PeopleFlow_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_STATISTICS
