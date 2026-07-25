///////////////////////////////////////////////////////////////////////////////
// KalmanTracker.cpp: KalmanTracker Class Implementation Declaration

#include "KalmanTracker.h"
#include<iostream>
#include <numeric> 

int KalmanTracker::kf_count = 0;


// initialize Kalman filter
void KalmanTracker::init_kf(StateType stateMat)
{
	int stateNum = 7;
	int measureNum = 4;
	kf = KalmanFilter(stateNum, measureNum, 0);

	measurement = Mat::zeros(measureNum, 1, CV_32F);
        //改过
	kf.transitionMatrix = (Mat_<float>(stateNum, stateNum) <<
		1, 0, 0, 0, 1, 0, 0,
		0, 1, 0, 0, 0, 1, 0,
		0, 0, 1, 0, 0, 0, 1,
		0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 1);

	// Identity只在对角线上置初值
	setIdentity(kf.measurementMatrix); 						 //!< measurement matrix (H)
	setIdentity(kf.processNoiseCov, Scalar::all(1e-2));		 //!< process noise covariance matrix (Q)
	setIdentity(kf.measurementNoiseCov, Scalar::all(1e-1));  //!< measurement noise covariance matrix (R)
	setIdentity(kf.errorCovPost, Scalar::all(1));			 //!< posteriori error estimate covariance matrix (P(k)): P(k)=(I-K(k)*H)*P'(k)		 
	
	// initialize state vector with bounding box in [cx,cy,s,r] style
	kf.statePost.at<float>(0, 0) = stateMat.x + stateMat.width / 2;
	kf.statePost.at<float>(1, 0) = stateMat.y + stateMat.height / 2;
	kf.statePost.at<float>(2, 0) = stateMat.area();
	kf.statePost.at<float>(3, 0) = stateMat.width / stateMat.height;
}


void KalmanTracker::initVector(int n){
	y.resize(n);
	dy.resize(n);  // 设置 std::vector 的大小
	for (int i = 0; i < n; i++) {
		y[i] = 1;  // 给每个元素赋值
		dy[i] = 1;  // 给每个元素赋值
	}
}


// Predict the estimated bounding box.
StateType KalmanTracker::predict()
{
	
	// for (auto i : kf.)
	// std::cout << kf.measurementMatrix.at<float>(0, 0) << kf.measurementMatrix.at<float>(0, 0);
	// std::cout << kf.controlMatrix.size();
	// std::cout << kf.processNoiseCov.size();
	
	// predict
	Mat p = kf.predict();
	m_age += 1;
	
	if (m_time_since_update > 0)
		m_hit_streak = 0;
	m_time_since_update += 1;

	StateType predictBox = get_rect_xysr(p.at<float>(0, 0), p.at<float>(1, 0), p.at<float>(2, 0), p.at<float>(3, 0));

	m_history.push_back(predictBox);
	return m_history.back();
}


// Update the state vector with observed bounding box.
void KalmanTracker::update(StateType stateMat)
{
	m_time_since_update = 0;
	m_history.clear();
	m_hits += 1;
	m_hit_streak += 1;

	// measurement
	measurement.at<float>(0, 0) = stateMat.x + stateMat.width / 2;
	measurement.at<float>(1, 0) = stateMat.y + stateMat.height / 2;
	measurement.at<float>(2, 0) = stateMat.area();
	measurement.at<float>(3, 0) = stateMat.width / stateMat.height;

	int i = m_age % n;
	y[i] = measurement.at<float>(1, 0);
	int prei = (i - 1 + n) % n;
	// cout << i << prei << endl;
	dy[i] = y[i] - y[prei];
	// cout << i << prei << ' ' << dy[i] << endl;

	// update
	kf.correct(measurement);
}

float calculateAverage(const std::vector<float>& numbers) {
    if (numbers.empty()) {
        return 0.0;
    }

    float sum = std::accumulate(numbers.begin(), numbers.end(), 0.0);
	return sum;
    // float average = sum / numbers.size();
    // return average;
}

float KalmanTracker::get_vector_state()
{
	// cout << calculateAverage(dy);
	// Mat s = kf.statePost;
	// return calculateAverage(dy) ;
	float h = sqrt(kf.statePost.at<float>(2, 0) * (1 / kf.statePost.at<float>(3, 0)) );
	return calculateAverage(dy) / h;

	// return accumulate(dy);
}


// Return the current state vector
StateType KalmanTracker::get_state()
{
	Mat s = kf.statePost;
	return get_rect_xysr(s.at<float>(0, 0), s.at<float>(1, 0), s.at<float>(2, 0), s.at<float>(3, 0));
}


// Convert bounding box from [cx,cy,s,r] to [x,y,w,h] style.
StateType KalmanTracker::get_rect_xysr(float cx, float cy, float s, float r)
{
	float w = sqrt(s * r);
	float h = s / w;
	float x = (cx - w / 2);
	float y = (cy - h / 2);

	if (x < 0 && cx > 0)
		x = 0;
	if (y < 0 && cy > 0)
		y = 0;

	return StateType(x, y, w, h);
}


