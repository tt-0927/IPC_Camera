/**
 * @FilePath     : detection_tracker.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : 统一检测层通用目标跟踪模块实现。
 */

#include "detection_tracker.hpp"

#include <algorithm>
#include <memory>
#include <unistd.h>

#include "dlog.h"

namespace
{
/* 跟踪调试开关：板端 touch /opt/cam/bin/testPrint 后输出 NEW/END 轨迹日志（绝对路径，进程 CWD 可能在 /opt/cam/bin） */
constexpr const char *TRACKER_DEBUG_FLAG = "/opt/cam/bin/testPrint";
}

namespace DetectionTracker_NS
{

CDetectionTracker::CDetectionTracker(const Config &stCfg) : m_stCfg(stCfg)
{
}

Inference_NS::cIouTracker &CDetectionTracker::ensure_class_tracker(int nClassId)
{
    auto it = m_mapTrackers.find(nClassId);
    if (it == m_mapTrackers.end())
    {
        /* 内部 minHits=1：TENTATIVE/STABLE 判定由本层负责，便于暴露完整状态机 */
        it = m_mapTrackers
                 .emplace(nClassId,
                          std::unique_ptr<Inference_NS::cIouTracker>(
                              new Inference_NS::cIouTracker(m_stCfg.nMaxAge, 1, m_stCfg.fIouThreshold, m_stCfg.nMaxTargets)))
                 .first;
    }
    return *it->second;
}

std::vector<TrackedTarget_S> CDetectionTracker::update(const std::vector<DetectionInput_S> &vecDetections)
{
    /* 按类别分流（左上角+宽高 -> 中心点+宽高，cIouTracker 内部语义） */
    std::map<int, std::vector<DetectResult_S>> mapClassDetections;
    for (const auto &stDet : vecDetections)
    {
        if (stDet.nWidth <= 0 || stDet.nHeight <= 0)
        {
            continue;
        }
        DetectResult_S stResult;
        stResult.nClassId = stDet.nClassId;
        stResult.fConfidence = stDet.fConfidence;
        stResult.nW = stDet.nWidth;
        stResult.nH = stDet.nHeight;
        stResult.nX1 = stDet.nLeft + stDet.nWidth / 2;
        stResult.nY1 = stDet.nTop + stDet.nHeight / 2;
        stResult.nUserData = stDet.nUserData;
        mapClassDetections[stDet.nClassId].emplace_back(stResult);
    }

    std::vector<TrackedTarget_S> vecTargets;
    std::map<int, std::set<int>> mapNowSeenIds;
    for (const auto &stPair : mapClassDetections)
    {
        const int nClassId = stPair.first;
        /*
         * 先在"本帧出现过的目标"表里登记该类别，再喂给跟踪器。这样能区分
         * "本帧没有检测结果"（表里没有该类别）和"喂过但目标都不在了"（表里
         * 有该类别、内容为空），下面判定目标离开时要靠这个区分。
         */
        std::set<int> &setSeenIds = mapNowSeenIds[nClassId];
        auto vecTracked = ensure_class_tracker(nClassId).update(stPair.second);
        for (const auto &stTrack : vecTracked)
        {
            setSeenIds.insert(stTrack.nTrackId);

            TrackedTarget_S stTarget;
            stTarget.nTrackId = stTrack.nTrackId;
            stTarget.nClassId = nClassId;
            /* 中心点+宽高 -> 左上角+宽高 */
            stTarget.nLeft = stTrack.box.nX1 - stTrack.box.nW / 2;
            stTarget.nTop = stTrack.box.nY1 - stTrack.box.nH / 2;
            stTarget.nWidth = stTrack.box.nW;
            stTarget.nHeight = stTrack.box.nH;
            stTarget.fConfidence = stTrack.box.fConfidence;
            stTarget.nHits = stTrack.nHits;
            stTarget.nLost = stTrack.nLost;
            stTarget.nUserData = stTrack.nUserData;
            if (stTrack.nLost > 0)
            {
                stTarget.enState = TrackState_E::PREDICTED;
            }
            else if (stTrack.nHits >= m_stCfg.nMinHits)
            {
                stTarget.enState = TrackState_E::TRACKING;
            }
            else
            {
                stTarget.enState = TrackState_E::NEW;
                if (access(TRACKER_DEBUG_FLAG, F_OK) == 0)
                {
                    dlog_info("[目标跟踪] 目标%d 出现 置信度%.2f 大小%dx%d 位置(%d,%d) 已检出%d次",
                              stTarget.nTrackId,
                              stTarget.fConfidence,
                              stTarget.nWidth,
                              stTarget.nHeight,
                              stTarget.nLeft,
                              stTarget.nTop,
                              stTarget.nHits);
                }
            }
            vecTargets.emplace_back(stTarget);
        }
    }

    /*
     * 判定哪些目标已离开：上一帧还在、这一帧消失的，就是结束。
     * 整帧一个目标都没检出时不算（板端实测教训：曾有目标被反复判定离开）——
     * 那种帧没有喂给跟踪器，目标记录原封不动，不能当作离开；只有真正喂过
     * 检测结果、目标却不在其中，才算离开。没喂过的类别把记录原样带到下帧。
     */
    for (auto it = m_mapLastSeenIds.begin(); it != m_mapLastSeenIds.end(); ++it)
    {
        const auto itNow = mapNowSeenIds.find(it->first);
        if (itNow == mapNowSeenIds.end())
        {
            /* 本帧没有检出输入：不判定离开，记录原样带到下一帧 */
            mapNowSeenIds[it->first] = it->second;
            continue;
        }
        for (const int nTrackId : it->second)
        {
            if (itNow->second.count(nTrackId) == 0)
            {
                m_vecEnded.push_back({ nTrackId, it->first });
                if (access(TRACKER_DEBUG_FLAG, F_OK) == 0)
                {
                    dlog_info("[目标跟踪] 目标%d 离开（连续 %d 次未检出，结束跟踪）", nTrackId, m_stCfg.nMaxAge);
                }
            }
        }
    }
    m_mapLastSeenIds = std::move(mapNowSeenIds);
    return vecTargets;
}

std::vector<EndedTrack_S> CDetectionTracker::take_ended()
{
    std::vector<EndedTrack_S> vecEnded;
    vecEnded.swap(m_vecEnded);
    return vecEnded;
}

void CDetectionTracker::reset()
{
    m_mapTrackers.clear();
    m_mapLastSeenIds.clear();
    m_vecEnded.clear();
}

} // namespace DetectionTracker_NS
