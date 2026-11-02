/*
 * @Author: zhengxh
 * @Date: 2026-10-13 16:42:45
 * @LastEditors: zhengxh
 * @LastEditTime: 2026-10-13 16:51:20
 * @Description: 
 */
/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/algorithm_mode/tracker/tracker.hpp
 * @Date         : 2026-10-12
 * @Description  : 追踪算法封装
 */

#pragma once

#include <vector>

#include "algorithm_define.hpp"

namespace Inference_NS
{
    class cBYTETracker;
}

class Tracker
{
public:
    explicit Tracker();
    ~Tracker();

    struct TrackerConfig{
        int nClassCount;                    // 需要的追踪器数量
        std::vector<float> vecTrackThresh;  // 每个类别的追踪阈值
        std::vector<float> vecHighThresh;   // 每个类别的高置信度阈值
        std::vector<float> vecMatchThresh;  // 每个类别的匹配阈值
        std::vector<int>   vecFrameId;      // 每个类别的起始帧 ID
        int                nMaxTimeLost;    // 最大丢失时间（所有类别共用）
    };

    bool init(const TrackerConfig& config);

    /**
     * @brief   : 一帧检测（类别混杂，x1y1x2y2）→ 按类别分组的跟踪目标
     * @return   : 外层下标即类别，size == nClassCount；仅输出激活轨迹，异常数据丢弃
     */
    std::vector<std::vector<TrackedTarget_S>> update(const std::vector<DetectedTarget_S>& vecDetections);

    bool uninit();

private:
    std::vector<Inference_NS::cBYTETracker*> m_vecByteTrackers;
};
