#ifndef KALMAN_BOX_TRACKER_HPP
#define KALMAN_BOX_TRACKER_HPP

#include "KalmanFilter.hpp"
#include <Eigen/Dense>
#include <cmath>
#include <vector>

class KalmanBoxTracker {
public:
    KalmanBoxTracker(const Eigen::Vector4f& bbox);
    void update(const Eigen::Vector4f& bbox);
    Eigen::Vector4f predict();
    std::pair<Eigen::Vector4f, bool> getState();
	int getHitStreak();
	int getTimeSinceUpdate();

private:
    static int count;
    int id;
    int time_since_update;
    int hits;
    int hit_streak;
    int age;
    bool is_throw;
    Eigen::Vector4f org_box;
    KalmanFilter kf;
    std::vector<Eigen::Vector4f> history;

    Eigen::Vector4f convertXToBbox(const Eigen::VectorXd& x, float score = -1.0);
    Eigen::VectorXd convertBboxToZ(const Eigen::Vector4f& bbox);
};

#endif // KALMAN_BOX_TRACKER_HPP
