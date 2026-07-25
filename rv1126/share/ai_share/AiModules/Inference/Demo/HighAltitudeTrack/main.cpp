/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-26 16:32:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 21:01:29
 * @Description  :
 */

// #include "opencv2/core/core.hpp"
// #include "opencv2/imgcodecs.hpp"
// #include "opencv2/imgproc.hpp"
#include <chrono>

#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include "HighAltitudeTracker.hpp"
using namespace Inference_NS;

// 模拟一些检测框
std::vector<cv::Rect> generateDetections(int frame) {
    std::vector<cv::Rect> detections;
    int num_detections = 5 + (frame % 3); // 随着帧数改变检测框数量
    for (int i = 0; i < num_detections; ++i) {
        int x = 50 * (i + 1) + (frame % 20); // x 坐标随帧数波动
        int y = 60 * (i + 1) + (frame % 15); // y 坐标随帧数波动
        int width = 30 + (frame % 5);
        int height = 30 + (frame % 5);
        detections.emplace_back(x, y, width, height);
        std::cout << "original Object " << i + 1 << ": x=" << x << ", y=" << y
                      << ", width=" << width << ", height=" << height << std::endl;
    }
    return detections;
}

int main() {
    // Sort HAtracker(2, 3, 0.3); // 初始化 Sort 跟踪器，参数分别为 max_age, min_hits, iou_threshold
    cHighAltitudeTracker* demo = new cHighAltitudeTracker();

    for (int frame = 0; frame < 10; ++frame) {
        std::cout << "Frame: " << frame << std::endl;
        
        // 生成当前帧的检测框
        std::vector<cv::Rect> detections = generateDetections(frame);

        // 调用 Sort 的 update 方法进行跟踪
        std::vector<cv::Rect> tracked_objects = demo->update(detections);

        // 输出跟踪结果
        for (size_t i = 0; i < tracked_objects.size(); ++i) {
            const auto& bbox = tracked_objects[i];
            std::cout << "predict Object " << i + 1 << ": x=" << bbox.x << ", y=" << bbox.y
                      << ", width=" << bbox.width << ", height=" << bbox.height << std::endl;
        }

        std::cout << "-------------------" << std::endl;
    }

    return 0;
}

