/**
 * @FilePath     : trip_line_track_store.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 越界事件轨迹状态存储，以 Channel+Rule+TrackId 为 Key
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <cstdint>
#include <unordered_map>

#include "trip_line_types.hpp"

namespace AiPipeline_NS
{
namespace TripLine_NS
{
/**
 * @brief   : 越界事件轨迹状态存储
 * @note    : 键含 ruleId 保证多规则线不互相污染；
 *            两层清理：ENDED 主动 erase + cleanup_expired 超时清理（5000ms）
 */
class CTripLineTrackStore
{
public:
    /**
     * @brief   : 构造越界状态存储
     */
    CTripLineTrackStore();

    /**
     * @brief   : 查找已有轨迹状态（可写引用）
     * @param    {const TripLineTrackKey_S &} stKey：轨迹 Key
     * @return   {TripLineTrackState_S *} 状态指针，不存在返回 nullptr
     */
    TripLineTrackState_S *find(const TripLineTrackKey_S &stKey);

    /**
     * @brief   : 创建新轨迹状态（首帧初始化）
     * @param    {const TripLineTrackKey_S &} stKey：轨迹 Key
     * @param    {const Geometry_NS::Point_S &} stCenter：首帧中心点
     * @param    {int64_t} llNowMs：当前 monotonic 毫秒时间戳
     * @return   {TripLineTrackState_S *} 状态指针，容量满时返回 nullptr
     * @note    : 新轨迹 bIsTracking=false，调用方须翻 true 并 continue 不判跨线
     */
    TripLineTrackState_S *create(const TripLineTrackKey_S &stKey,
                                 const Geometry_NS::Point_S &stCenter,
                                 int64_t llNowMs);

    /**
     * @brief   : 立即删除轨迹状态，ENDED 目标调用
     * @param    {const TripLineTrackKey_S &} stKey：轨迹 Key
     * @return   {void}
     */
    void erase(const TripLineTrackKey_S &stKey);

    /**
     * @brief   : 清理超过 TTL 未更新的轨迹状态
     * @param    {int64_t} llNowMs：当前 monotonic 毫秒时间戳
     * @return   {size_t} 清理数量
     */
    size_t cleanup_expired(int64_t llNowMs);

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
    /* 轨迹状态容器，Key 为 Channel+Rule+TrackId */
    std::unordered_map<TripLineTrackKey_S, TripLineTrackState_S, TripLineTrackKeyHash_S> m_mapTracks;
};
} // namespace TripLine_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
