/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/tracker/tracker.cpp
 * @Date         : 2026-10-12
 * @Description  : 追踪算法封装实现
 */

#include "tracker.hpp"

#include <algorithm>
#include <unistd.h>

#include "dlog.h"
#include "BYTETracker.h"
#include <opencv2/opencv.hpp>

Tracker::Tracker()
{
}

Tracker::~Tracker()
{
    uninit();
}

bool Tracker::init(const TrackerConfig& config)
{
    uninit();

    if (config.nClassCount <= 0)
    {
        dlog_error("Tracker: nClassCount 必须大于 0");
        return false;
    }

    if (static_cast<int>(config.vecTrackThresh.size()) < config.nClassCount ||
        static_cast<int>(config.vecHighThresh.size()) < config.nClassCount ||
        static_cast<int>(config.vecMatchThresh.size()) < config.nClassCount ||
        static_cast<int>(config.vecFrameId.size()) < config.nClassCount)
    {
        dlog_error("Tracker: 参数 vector 大小不足，需要 >= %d", config.nClassCount);
        return false;
    }

    for (int i = 0; i < config.nClassCount; ++i)
    {
        Inference_NS::cBYTETracker* pTracker = new Inference_NS::cBYTETracker();
        if (pTracker == nullptr)
        {
            uninit();
            return false;
        }
        pTracker->setValue(config.vecTrackThresh[i],
                           config.vecHighThresh[i],
                           config.vecMatchThresh[i],
                           config.vecFrameId[i],
                           config.nMaxTimeLost);
        m_vecByteTrackers.push_back(pTracker);
    }

    dlog_info("Tracker: 初始化完成，共 %d 个追踪器", config.nClassCount);
    return true;
}

std::vector<std::vector<TrackedTarget_S>> Tracker::update(const std::vector<DetectedTarget_S>& vecDetections)
{
    std::vector<std::vector<TrackedTarget_S>> vecResults(m_vecByteTrackers.size());

    std::vector<std::vector<DetectResult_S>> vecClassDets(m_vecByteTrackers.size());
    for (const auto& stDet : vecDetections)
    {
        if (stDet.nLabel < 0 || stDet.nLabel >= static_cast<int>(m_vecByteTrackers.size()))
        {
            dlog_debug("Tracker: 丢弃越界类别检测框 label[%d]", stDet.nLabel);
            continue;
        }

        DetectResult_S stIn;
        stIn.nClassId    = stDet.nLabel;
        stIn.fConfidence = stDet.fConfidence;
        stIn.vfBox       = cv::Rect_<float>(stDet.fX1, stDet.fY1,
                                            stDet.fX2 - stDet.fX1, stDet.fY2 - stDet.fY1);
        vecClassDets[stDet.nLabel].push_back(stIn);
    }

    for (size_t i = 0; i < m_vecByteTrackers.size(); ++i)
    {
        if (m_vecByteTrackers[i] == nullptr)
        {
            continue;
        }

        // dlog_debug("Tracker: class[%d] input_det_count[%d]", (int)i, (int)vecClassDets[i].size());

        std::vector<cSTrack> vecTracks = m_vecByteTrackers[i]->update(vecClassDets[i]);

        for (const auto& stTrack : vecTracks)
        {
            if (stTrack.tlwh.size() < 4 || stTrack.track_id < 0)
            {
                continue;
            }

            TrackedTarget_S stTarget;
            stTarget.nTrackId    = stTrack.track_id;
            stTarget.fConfidence = stTrack.score;
            stTarget.fX1         = stTrack.tlwh[0];
            stTarget.fY1         = stTrack.tlwh[1];
            stTarget.fX2         = stTrack.tlwh[0] + stTrack.tlwh[2];
            stTarget.fY2         = stTrack.tlwh[1] + stTrack.tlwh[3];
            vecResults[i].push_back(stTarget);
        }

        // dlog_debug("Tracker: class[%d] output_track_count[%d]", (int)i, (int)vecResults[i].size());
    }

    if (!access("/printAlgoResults", F_OK))
    {
        int nTotal = 0;
        for (const auto &cls : vecResults) nTotal += (int)cls.size();
        dlog_info("Tracker: 跟踪输出 %d 条轨迹，共 %d 个类别", nTotal, (int)vecResults.size());
        for (size_t c = 0; c < vecResults.size(); c++)
        {
            for (size_t i = 0; i < vecResults[c].size(); i++)
            {
                const auto &t = vecResults[c][i];
                dlog_info("  class[%zu] id=%d conf=%.4f bbox=(%.1f, %.1f, %.1f, %.1f)",
                          c, t.nTrackId, t.fConfidence, t.fX1, t.fY1, t.fX2, t.fY2);
            }
        }
    }

    return vecResults;
}

bool Tracker::uninit()
{
    for(size_t i = 0; i < m_vecByteTrackers.size(); ++i)
    {
        if (m_vecByteTrackers[i] != nullptr)
        {
            delete m_vecByteTrackers[i];
            m_vecByteTrackers[i] = nullptr;
        }
    }
    m_vecByteTrackers.clear();
    return true;
}
