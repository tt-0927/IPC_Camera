#pragma once

#include <Eigen/Dense>
#include <vector>
#include <opencv2/opencv.hpp>
#include "KalmanBoxTracker.hpp"


namespace Inference_NS
{
    class cHighAltitudeTracker {
    public:
        cHighAltitudeTracker(int max_age = 1, int min_hits = 3, float iou_threshold = 0.05);
        ~cHighAltitudeTracker();
        std::vector<cv::Rect> update(const std::vector<cv::Rect>& detections);

    private:
        int m_nMaxAge = 1;
        int m_nMinHits = 3;
        float m_fIouThreshold = 0.3;
        std::vector<KalmanBoxTracker> trackers;
        int frame_count;

        Eigen::MatrixXf iouBatch(const std::vector<cv::Rect>& dets, const std::vector<cv::Rect>& trks);
        std::vector<std::pair<int, int>> linearAssignment(const Eigen::MatrixXf& cost_matrix);
        bool findMatch(int u, const Eigen::MatrixXf& cost_matrix, std::vector<int>& lx, std::vector<int>& ly,
                            std::vector<int>& match, std::vector<bool>& s, std::vector<bool>& t, int slack[]);
        std::tuple<std::vector<std::pair<int, int>>, std::vector<int>, std::vector<int>> 
        associateDetectionsToTrackers(const std::vector<cv::Rect>& detections, const std::vector<cv::Rect>& trackers);
        std::vector<cv::Rect> enlargeRects(const std::vector<cv::Rect> dets, float scaleFactor);
    };
}
