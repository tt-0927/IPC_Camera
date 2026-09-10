/**
 * @FilePath     : region_track_store.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 区域事件轨迹驻留状态存储，以 Channel+EventType+Rule+TrackId 为 Key
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <cstdint>
#include <unordered_map>

#include "region_types.hpp"

namespace AiPipeline_NS
{
namespace Region_NS
{
/**
 * @brief   : 区域事件轨迹驻留状态存储
 * @note    : 一份存储服务三事件（侵入/进入/离开），键含事件类型 + ruleId 保证多规则不互相污染
 */
class CRegionTrackStore
{
public:
    /**
     * @brief   : 构造区域状态存储
     */
    CRegionTrackStore();

    /**
     * @brief   : 获取或创建轨迹驻留状态（可写引用）
     * @param    {const RegionTrackKey_S &} stKey：轨迹 Key
     * @param    {int64_t} llNowMs：当前 monotonic 毫秒时间戳
     * @return   {RegionTrackState_S *} 状态指针，容量满时返回 nullptr
     * @note    : 新轨迹自动创建，调用方通过返回值判断是否成功
     */
    RegionTrackState_S *get_or_create(const RegionTrackKey_S &stKey, int64_t llNowMs);

    /**
     * @brief   : 立即删除轨迹状态，ENDED 目标调用
     * @param    {const RegionTrackKey_S &} stKey：轨迹 Key
     * @return   {void}
     */
    void erase(const RegionTrackKey_S &stKey);

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
    /* 轨迹状态容器，Key 为 Channel+EventType+Rule+TrackId */
    std::unordered_map<RegionTrackKey_S, RegionTrackState_S, RegionTrackKeyHash_S> m_mapTracks;
};
} // namespace Region_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
