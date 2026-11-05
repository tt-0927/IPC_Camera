/**
 * @Author: xushou
 * @Date: 2026-11-03
 * @Description: 人员聚集检测子检测器（复用 pmnm_detect 跟踪结果）
 */

#pragma once

#include <vector>
#include <mutex>
#include <opencv2/opencv.hpp>
#include "algorithm.hpp"
#include "algorithm_define.hpp"
#include "crowd_gathering_processor.hpp"
#include "alarm_define.h"
#include "event_define.h"

class CCrowdGatheringSubDetector
{
public:
    CCrowdGatheringSubDetector();
    ~CCrowdGatheringSubDetector();

    void updateConfig();

    int process(const cv::Mat& rgbMat,
                const std::vector<TrackedTarget_S>& vecPersonTargets,
                const cv::Mat& fullRgbMat,
                int nChannel);

private:
    CCrowdGatheringProcessor m_processor;
    bool m_bEnable = false;
    std::mutex m_mutex;
    CAlarmStateMachine m_stateMachine;
};
