/**
 * @FilePath     : trip_line_track_store.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 越界事件轨迹状态存储实现
 */

#include "trip_line_track_store.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_UNIFIED_EVENT_PIPELINE

namespace AiPipeline_NS
{
namespace TripLine_NS
{
CTripLineTrackStore::CTripLineTrackStore() = default;

TripLineTrackState_S *CTripLineTrackStore::find(const TripLineTrackKey_S &stKey)
{
    auto it = m_mapTracks.find(stKey);
    if (it == m_mapTracks.end())
    {
        return nullptr;
    }
    return &it->second;
}

TripLineTrackState_S *CTripLineTrackStore::create(const TripLineTrackKey_S &stKey,
                                                  const Geometry_NS::Point_S &stCenter,
                                                  int64_t llNowMs)
{
    /* 新轨迹：先尝试清理超时状态，再判断容量 */
    if (m_mapTracks.size() >= TRIP_LINE_TRACK_CAPACITY)
    {
        cleanup_expired(llNowMs);
        if (m_mapTracks.size() >= TRIP_LINE_TRACK_CAPACITY)
        {
            return nullptr;
        }
    }

    TripLineTrackState_S stState;
    /* 首帧：last=current=中心点，bIsTracking=false（由调用方翻 true + continue 不判跨线） */
    stState.stLastPosition = stCenter;
    stState.stCurrentPosition = stCenter;
    stState.bIsTracking = false;
    stState.bAlarmed = false;
    stState.llLastUpdateMs = llNowMs;

    auto result = m_mapTracks.emplace(stKey, stState);
    return &result.first->second;
}

void CTripLineTrackStore::erase(const TripLineTrackKey_S &stKey)
{
    m_mapTracks.erase(stKey);
}

size_t CTripLineTrackStore::cleanup_expired(int64_t llNowMs)
{
    size_t nRemoved = 0;
    for (auto it = m_mapTracks.begin(); it != m_mapTracks.end();)
    {
        /* 超过 TTL 未更新的轨迹视为目标丢失，立即清理（等价旧 5000ms 超时 reset） */
        if ((llNowMs - it->second.llLastUpdateMs) > TRIP_LINE_TRACK_TTL_MS)
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

void CTripLineTrackStore::reset()
{
    m_mapTracks.clear();
}

size_t CTripLineTrackStore::size() const
{
    return m_mapTracks.size();
}
} // namespace TripLine_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
