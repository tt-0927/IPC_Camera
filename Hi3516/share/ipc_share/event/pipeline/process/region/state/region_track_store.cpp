/**
 * @FilePath     : region_track_store.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 区域事件轨迹驻留状态存储实现
 */

#include "region_track_store.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_UNIFIED_EVENT_PIPELINE

namespace AiPipeline_NS
{
namespace Region_NS
{
CRegionTrackStore::CRegionTrackStore() = default;

RegionTrackState_S *CRegionTrackStore::get_or_create(const RegionTrackKey_S &stKey, int64_t llNowMs)
{
    auto it = m_mapTracks.find(stKey);
    if (it != m_mapTracks.end())
    {
        /* 已有轨迹：更新最后刷新时间 */
        it->second.llLastUpdateMs = llNowMs;
        return &it->second;
    }

    /* 新轨迹：先尝试清理超时状态，再判断容量 */
    if (m_mapTracks.size() >= REGION_TRACK_CAPACITY)
    {
        cleanup_expired(llNowMs);
        if (m_mapTracks.size() >= REGION_TRACK_CAPACITY)
        {
            return nullptr;
        }
    }

    RegionTrackState_S stState;
    stState.llLastUpdateMs = llNowMs;
    auto result = m_mapTracks.emplace(stKey, stState);
    return &result.first->second;
}

void CRegionTrackStore::erase(const RegionTrackKey_S &stKey)
{
    m_mapTracks.erase(stKey);
}

size_t CRegionTrackStore::cleanup_expired(int64_t llNowMs)
{
    size_t nRemoved = 0;
    for (auto it = m_mapTracks.begin(); it != m_mapTracks.end();)
    {
        /* 超过 TTL 未更新的轨迹视为目标丢失，立即清理 */
        if ((llNowMs - it->second.llLastUpdateMs) > REGION_TRACK_TTL_MS)
        {
            it = m_mapTracks.erase(it);
            ++nRemoved;
        }
        else
        {
            ++it;
        }
    }
    return nRemoved;
}

void CRegionTrackStore::reset()
{
    m_mapTracks.clear();
}

size_t CRegionTrackStore::size() const
{
    return m_mapTracks.size();
}
} // namespace Region_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
