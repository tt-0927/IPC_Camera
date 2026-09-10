/**
 * @FilePath     : people_flow_track_store.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 人流统计轨迹状态存储实现
 */

#include "people_flow_track_store.hpp"

#include "IpcRet.h"
#include "dlog.h"

#if CAP_AI_PEOPLE_STATISTICS

namespace AiPipeline_NS
{
namespace PeopleFlow_NS
{
CPeopleFlowTrackStore::CPeopleFlowTrackStore() = default;

int CPeopleFlowTrackStore::update(const TrackKey_S &stKey, const Geometry_NS::Point_S &stPosition, int64_t llNowMs)
{
    auto it = m_mapTracks.find(stKey);
    if (it != m_mapTracks.end())
    {
        /* 已存在轨迹：滚动上一帧位置，保留真实 Track 身份 */
        it->second.stLastPosition = it->second.stCurrentPosition;
        it->second.stCurrentPosition = stPosition;
        it->second.bHasLast = true;
        it->second.llLastUpdateMs = llNowMs;
        return OK;
    }

    /* 新轨迹：先尝试清理超时状态，再判断容量 */
    if (m_mapTracks.size() >= m_nCapacity)
    {
        cleanup_expired(llNowMs);
        if (m_mapTracks.size() >= m_nCapacity)
        {
            return ERR;
        }
    }

    TrackState_S stState;
    stState.stCurrentPosition = stPosition;
    stState.bHasLast = false;
    stState.llLastUpdateMs = llNowMs;
    m_mapTracks.emplace(stKey, stState);
    return OK;
}

void CPeopleFlowTrackStore::erase(const TrackKey_S &stKey)
{
    m_mapTracks.erase(stKey);
}

size_t CPeopleFlowTrackStore::cleanup_expired(int64_t llNowMs)
{
    size_t nRemoved = 0;
    for (auto it = m_mapTracks.begin(); it != m_mapTracks.end();)
    {
        /* 超过 TTL 未更新的轨迹状态视为目标丢失，立即清理，不继承给新 Track */
        if ((llNowMs - it->second.llLastUpdateMs) > PEOPLE_FLOW_TRACK_TTL_MS)
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

bool CPeopleFlowTrackStore::contains(const TrackKey_S &stKey) const
{
    return m_mapTracks.find(stKey) != m_mapTracks.end();
}

bool CPeopleFlowTrackStore::get(const TrackKey_S &stKey, TrackState_S &stOutState) const
{
    const auto it = m_mapTracks.find(stKey);
    if (it == m_mapTracks.end())
    {
        return false;
    }
    stOutState = it->second;
    return true;
}

void CPeopleFlowTrackStore::reset()
{
    m_mapTracks.clear();
}

size_t CPeopleFlowTrackStore::size() const
{
    return m_mapTracks.size();
}
} // namespace PeopleFlow_NS
} // namespace AiPipeline_NS

#endif // CAP_AI_PEOPLE_STATISTICS
