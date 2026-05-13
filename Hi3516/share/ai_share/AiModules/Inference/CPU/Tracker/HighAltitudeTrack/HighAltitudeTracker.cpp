
/*
 * @FilePath     : BYTETracker.cpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-09-23 19:48:15
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-09-23 19:48:15
 * @Description  : Bytetrack跟踪算法接口
 */


#include <Eigen/Dense>
#include <algorithm>
#include <limits>
#include <numeric>
#include "HighAltitudeTracker.hpp"

using namespace Inference_NS;

cHighAltitudeTracker::cHighAltitudeTracker(int max_age, int min_hits, float iou_threshold)
{
    m_nMaxAge = max_age;
    m_nMinHits = min_hits;
    m_fIouThreshold = iou_threshold;
	
}

cHighAltitudeTracker::~cHighAltitudeTracker()
{
}

std::vector<cv::Rect> Inference_NS::cHighAltitudeTracker::update(const std::vector<cv::Rect>& detections) {
    frame_count++;
    std::vector<cv::Rect> predictions;
    std::vector<int> to_delete;
    std::vector<cv::Rect> result;

    // std::cout << "1.trackers 数量: " << trackers.size() << std::endl;
    for (size_t i = 0; i < trackers.size(); ++i) {
        Eigen::Vector4f pos_vec = trackers[i].predict();
        int posX = static_cast<int>(pos_vec(0));
        int posY = static_cast<int>(pos_vec(1));
        int posWidth = static_cast<int>(pos_vec(2) - pos_vec(0));
        int posHeight = static_cast<int>(pos_vec(3) - pos_vec(1));
        cv::Rect pos(posX, posY, posWidth, posHeight);

        if (pos.width > 0 && pos.height > 0) {
            predictions.push_back(pos);
        } else {
            to_delete.push_back(i);
        }
    }

    // 删除无效的追踪器
    // std::cout << "2.删除无效的追踪器!!" << std::endl;
    for (auto it = to_delete.rbegin(); it != to_delete.rend(); ++it) {
        trackers.erase(trackers.begin() + *it);
    }

    // 匹配检测框与跟踪框
    // std::cout << "3.匹配检测框与跟踪框!!" << std::endl;
    auto [matches, unmatched_dets, unmatched_trks] = associateDetectionsToTrackers(detections, predictions);

    // 更新匹配的追踪器
    // std::cout << "4.更新匹配的追踪器!!" << std::endl;
    for (const auto& m : matches) {
        cv::Rect det1 = detections[m.first];
        Eigen::Vector4f det_vec1;
        det_vec1 << det1.x, det1.y, det1.x + det1.width, det1.y + det1.height;
        trackers[m.second].update(det_vec1);
    }

    // 为未匹配的检测框创建新的追踪器
    // std::cout << "5.为未匹配的检测框创建新的追踪器!!" << std::endl;
    for (int idx : unmatched_dets) {
        cv::Rect det2 = detections[idx];
        Eigen::Vector4f det_vec2;
        det_vec2 << det2.x, det2.y, det2.x + det2.width, det2.y + det2.height;
        KalmanBoxTracker tracker = KalmanBoxTracker(det_vec2);
        trackers.emplace_back(tracker);
    }

    // 收集结果
    // std::cout << "6.收集结果!!" << std::endl;
    std::cout << "----Trackers length: " << trackers.size() << std::endl;
    for (size_t i = 0; i < trackers.size(); ++i) {
        auto bbox = trackers[i].getState();
        if (bbox.second && trackers[i].getTimeSinceUpdate() < 1)
        {
            if (trackers[i].getHitStreak() >= m_nMinHits || frame_count <= m_nMinHits)
            {
                Eigen::Vector4f bbox_values1 = bbox.first;  // 取出 Eigen::Vector4f 部分
                cv::Rect rect1(bbox_values1[0], bbox_values1[1], bbox_values1[2]-bbox_values1[0], bbox_values1[3]-bbox_values1[1]);  // 转换为 cv::Rect
                result.push_back(rect1);  // 将 cv::Rect 添加到 result 中
            }
        }
        
        if (trackers[i].getTimeSinceUpdate() > m_nMaxAge) {
            trackers.erase(trackers.begin() + i);
        }
    }

    return result;
}

Eigen::MatrixXf Inference_NS::cHighAltitudeTracker::iouBatch(const std::vector<cv::Rect>& dets, const std::vector<cv::Rect>& trks) {
    int n_dets = dets.size();
    int n_trks = trks.size();
    Eigen::MatrixXf iou_matrix(n_dets, n_trks);

    std::vector<cv::Rect> enlargedDets = enlargeRects(dets, 1.5);
    std::vector<cv::Rect> enlargedTrks = enlargeRects(trks, 1.5);
    for (int i = 0; i < n_dets; ++i) {
        for (int j = 0; j < n_trks; ++j) {
            float inter_area = (enlargedDets[i] & enlargedTrks[j]).area();
            float union_area = enlargedDets[i].area() + enlargedTrks[j].area() - inter_area;
            iou_matrix(i, j) = (union_area > 0) ? inter_area / union_area : 0.0;
        }
    }

    return iou_matrix;
}

std::vector<std::pair<int, int>> Inference_NS::cHighAltitudeTracker::linearAssignment(const Eigen::MatrixXf& cost_matrix)
{
    int n = cost_matrix.rows();
    int m = cost_matrix.cols();

    // 创建一个矩阵的副本，以便操作
    // std::cout << "3.3.1.创建一个矩阵的副本，以便操作!!" << std::endl;
    Eigen::MatrixXf matrix = cost_matrix;

    // 步骤1：每行减去行最小值
    // std::cout << "3.3.2.每行减去行最小值!!" << std::endl;
    for (int i = 0; i < n; ++i) {
        float row_min = matrix.row(i).minCoeff();
        matrix.row(i) = matrix.row(i).array() - row_min;
    }

    // 步骤2：每列减去列最小值
    // std::cout << "3.3.3.每列减去列最小值!!" << std::endl;
    for (int j = 0; j < m; ++j) {
        float col_min = matrix.col(j).minCoeff();
        matrix.col(j) = matrix.col(j).array() - col_min;
    }

    // 步骤3：标记行列，寻找最优匹配
    // std::cout << "3.3.4.标记行列，寻找最优匹配!!" << std::endl;
    std::vector<bool> row_covered(n, false);
    std::vector<bool> col_covered(m, false);
    std::vector<std::pair<int, int>> zeros;

    // 寻找零元素并标记
    // std::cout << "3.3.5.寻找零元素并标记!!" << std::endl;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (matrix(i, j) == 0 && !row_covered[i] && !col_covered[j]) {
                zeros.push_back({i, j});
                row_covered[i] = true;
                col_covered[j] = true;
            }
        }
    }

    // 清除行列的覆盖
    // std::cout << "3.3.6.清除行列的覆盖!!" << std::endl;
    std::fill(row_covered.begin(), row_covered.end(), false);
    std::fill(col_covered.begin(), col_covered.end(), false);

    // 步骤4：寻找最优匹配
    // std::cout << "3.3.7.寻找最优匹配!!" << std::endl;
    std::vector<int> matching(n, -1); // 保存每个任务的匹配资源

    while (!zeros.empty()) {
        auto [i, j] = zeros.back();
        zeros.pop_back();

        if (matching[i] == -1) {
            matching[i] = j;
        }

        row_covered[i] = true;
        col_covered[j] = true;

        // 更新 zeros 列表，寻找未被覆盖的零
        std::vector<std::pair<int, int>> new_zeros;
        for (int ii = 0; ii < n; ++ii) {
            for (int jj = 0; jj < m; ++jj) {
                if (matrix(ii, jj) == 0 && !row_covered[ii] && !col_covered[jj]) {
                    new_zeros.push_back({ii, jj});
                }
            }
        }
        zeros = std::move(new_zeros);
    }

    // 将匹配结果转化为所需的输出格式
    // std::cout << "3.3.8.将匹配结果转化为所需的输出格式!!" << std::endl;
    std::vector<std::pair<int, int>> assignments;
    for (int i = 0; i < n; ++i) {
        if (matching[i] != -1) {
            assignments.push_back({i, matching[i]});
        }
    }

    return assignments;
}

bool Inference_NS::cHighAltitudeTracker::findMatch(int u,
                                   const Eigen::MatrixXf &cost_matrix, 
                                   std::vector<int> &lx, 
                                   std::vector<int> &ly, 
                                   std::vector<int> &match, 
                                   std::vector<bool> &s, 
                                   std::vector<bool> &t, 
                                   int slack[])
{
	int n = cost_matrix.rows();
    s[u] = true;
    for (int v = 0; v < n; ++v) {
        if (t[v]) continue;
        int gap = lx[u] + ly[v] - cost_matrix(u, v);
        if (gap == 0) {
            t[v] = true;
            if (match[v] == -1 || findMatch(match[v], cost_matrix, lx, ly, match, s, t, slack)) {
                match[v] = u;
                return true;
            }
        } else {
            slack[v] = std::min(slack[v], gap);
        }
    }
    return false;
}

std::tuple<std::vector<std::pair<int, int>>, std::vector<int>, std::vector<int>>
Inference_NS::cHighAltitudeTracker::associateDetectionsToTrackers(const std::vector<cv::Rect>& detections, const std::vector<cv::Rect>& trackers)
{
    // std::cout << "3.1.如果没有追踪到!!" << std::endl;
    if (trackers.empty()) {
        std::vector<int> unmatched_detections(detections.size());
        std::iota(unmatched_detections.begin(), unmatched_detections.end(), 0);
        return {{}, unmatched_detections, {}};
    }

    // std::cout << "3.2.获得追踪器和检测目标的iou!!" << std::endl;
    Eigen::MatrixXf iou_matrix = iouBatch(detections, trackers);
    // std::cout << "3.2.1.IOU Matrix: " << iou_matrix.rows() << "x" << iou_matrix.cols() << std::endl;
    // std::cout << iou_matrix << std::endl;
    // std::cout << -iou_matrix << std::endl;

    std::vector<std::pair<int, int>> matches;
    if (std::min(iou_matrix.rows(), iou_matrix.cols()) > 0)
    {
        // std::cout << "3.3.获得匹配索引!!" << std::endl;
        // 构造二值矩阵a
        Eigen::MatrixXi a = (iou_matrix.array() > m_fIouThreshold).cast<int>();
        Eigen::VectorXi row_sum = a.rowwise().sum();
        Eigen::VectorXi col_sum = a.colwise().sum();

        if (row_sum.maxCoeff() == 1 && col_sum.maxCoeff() == 1)
        {
            // 提取匹配的索引并存储在matched_indices中
            for (int i = 0; i < a.rows(); ++i)
            {
                for (int j = 0; j < a.cols(); ++j)
                {
                    if (a(i, j) == 1)
                    {
                        matches.emplace_back(i, j);
                    }
                }
            }
        }else{
            matches = linearAssignment(-iou_matrix);
        }
    }
    

    // std::cout << "3.4.未匹配的 新检测目标&追踪器!!" << std::endl;
    std::vector<int> unmatched_detections, unmatched_trackers;
    for (size_t i = 0; i < detections.size(); ++i) {
        if (std::none_of(matches.begin(), matches.end(), [i](const auto& match) { return match.first == i; })) {
            unmatched_detections.push_back(i);
        }
    }
    for (size_t j = 0; j < trackers.size(); ++j) {
        if (std::none_of(matches.begin(), matches.end(), [j](const auto& match) { return match.second == j; })) {
            unmatched_trackers.push_back(j);
        }
    }

    return {matches, unmatched_detections, unmatched_trackers};
}

std::vector<cv::Rect> Inference_NS::cHighAltitudeTracker::enlargeRects(const std::vector<cv::Rect> dets, float scaleFactor)
{
    std::vector<cv::Rect> enlargedRects;
    for (const auto& rect : dets) {
        // 获取中心点坐标
        cv::Point center = (rect.tl() + rect.br()) * 0.5;

        // 计算新的宽高
        int newWidth = static_cast<int>(rect.width * scaleFactor);
        int newHeight = static_cast<int>(rect.height * scaleFactor);

        // 计算新的左上角坐标，使得中心点不变
        cv::Rect enlargedRect;
        enlargedRect.x = center.x - newWidth / 2;
        enlargedRect.y = center.y - newHeight / 2;
        enlargedRect.width = newWidth;
        enlargedRect.height = newHeight;

        // 添加到结果列表
        enlargedRects.push_back(enlargedRect);
    }
    return enlargedRects;
}
