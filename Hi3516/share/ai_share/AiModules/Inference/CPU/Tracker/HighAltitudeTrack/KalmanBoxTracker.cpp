#include "KalmanBoxTracker.hpp"
#include <iostream>

int KalmanBoxTracker::count = 0;

KalmanBoxTracker::KalmanBoxTracker(const Eigen::Vector4f& bbox)
    : kf(9, 4), time_since_update(0), hits(0), hit_streak(0), age(0), is_throw(false), org_box(bbox) {
    
    kf.F << 1, 0, 0, 0, 1, 0, 0, 0.5, 0,
            0, 1, 0, 0, 0, 1, 0, 0, 0.5,
            0, 0, 1, 0, 0, 0, 1, 0, 0,
            0, 0, 0, 1, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 1, 0, 0, 1, 0,
            0, 0, 0, 0, 0, 1, 0, 0, 1,
            0, 0, 0, 0, 0, 0, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 1, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 1;
    
    kf.H << 1, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 1, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 1, 0, 0, 0, 0, 0;

    kf.R.block<2, 2>(2, 2) *= 10.0;
    kf.P.block<5, 5>(4, 4) *= 1000.0;
    kf.P *= 10.0;
    kf.Q(8, 8) *= 0.01;
    kf.Q.block<5, 5>(4, 4) *= 0.01;

    kf.x.head<4>() = convertBboxToZ(bbox);
    id = count++;
}

void KalmanBoxTracker::update(const Eigen::Vector4f& bbox) {
    time_since_update = 0;
    history.clear();
    hits++;
    hit_streak++;
    kf.update(convertBboxToZ(bbox));
}

Eigen::Vector4f KalmanBoxTracker::predict() {
    if (kf.x(6) + kf.x(2) <= 0) {
        kf.x(6) = 0.0;
    }
    kf.predict();
    age++;
    if (time_since_update > 0) {
        hit_streak = 0;
    }
    time_since_update++;
    Eigen::Vector4f predicted_bbox = convertXToBbox(kf.x);
    history.push_back(predicted_bbox);
    return predicted_bbox;
}

std::pair<Eigen::Vector4f, bool> KalmanBoxTracker::getState() {
    Eigen::Vector4f bbox = convertXToBbox(kf.x);
    float x_offset = (bbox(0) + bbox(2)) / 2 - (org_box(0) + org_box(2)) / 2;
    float y_offset = (bbox(1) + bbox(3)) / 2 - (org_box(1) + org_box(3)) / 2;
    float distance = std::hypot(x_offset, y_offset);

    float vecM = std::sqrt(x_offset * x_offset + y_offset * y_offset);
    float cosTheta = y_offset / vecM;


    // std::cout << "原始框: x1=" << org_box(0)
    //             << ", y1=" << org_box(1)
    //             << ", x2=" << org_box(2)
    //             << ", y2=" << org_box(3) << std::endl;

    // std::cout << "预测框: x1=" << bbox(0)
    //             << ", y1=" << bbox(1)
    //             << ", x2=" << bbox(2)
    //             << ", y2=" << bbox(3) << std::endl;

    // std::cout << "计算的距离: " << distance << std::endl;

    if (distance > 2 * (org_box(2) - org_box(0) + bbox(2) - bbox(0)) &&
        distance > (org_box(3) - org_box(1) + bbox(3) - bbox(1)) && cosTheta >= 0.5)
    {
        is_throw = true;
    }else{
        is_throw = false;
    }

    // std::cout << "判断结果: " << is_throw << std::endl; 

    // std::cout << "----------------------" << std::endl; 

    return {bbox, is_throw};
}

int KalmanBoxTracker::getHitStreak()
{
    return hit_streak;
}

int KalmanBoxTracker::getTimeSinceUpdate()
{
    return time_since_update;
}

Eigen::Vector4f KalmanBoxTracker::convertXToBbox(const Eigen::VectorXd& x, float score) {
    float w = std::sqrt(x(2) * x(3));
    float h = x(2) / w;
    Eigen::Vector4f bbox;
    bbox << x(0) - w / 2, x(1) - h / 2, x(0) + w / 2, x(1) + h / 2;
    return bbox;
}

Eigen::VectorXd KalmanBoxTracker::convertBboxToZ(const Eigen::Vector4f& bbox) {
    Eigen::VectorXd z(4);
    float w = bbox(2) - bbox(0);
    float h = bbox(3) - bbox(1);
    z << (bbox(0) + bbox(2)) / 2, (bbox(1) + bbox(3)) / 2, w * h, w / h;
    return z;
}
