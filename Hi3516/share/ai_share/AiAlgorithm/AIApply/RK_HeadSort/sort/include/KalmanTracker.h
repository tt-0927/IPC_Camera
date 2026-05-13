///////////////////////////////////////////////////////////////////////////////
// KalmanTracker.h: KalmanTracker Class Declaration

#ifndef KALMAN_H
#define KALMAN_H 2

#include <opencv2/video/tracking.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

#define StateType Rect_<float>


// This class represents the internel state of individual tracked objects observed as bounding box.
class KalmanTracker
{
public:
	KalmanTracker()
	{
		init_kf(StateType());
		n = 10;
		m_time_since_update = 0;
		m_hits = 0;
		m_hit_streak = 0;
		m_age = 0;
		m_id = kf_count;
		//kf_count++;
	}
	KalmanTracker(StateType initRect)
	{
		n = 10;
		init_kf(initRect); // Kalman Filter Init
		initVector(n);
		m_time_since_update = 0;
		m_hits = 0;
		m_hit_streak = 0;
		m_age = 0;
		m_id = kf_count;
		kf_count++;
	}

	~KalmanTracker()
	{
		m_history.clear();
	}

	void initVector(int n);
	float get_vector_state();

	StateType predict();
	void update(StateType stateMat);
	
	StateType get_state();
	StateType get_rect_xysr(float cx, float cy, float s, float r);

	static int kf_count; // 类的静态成员与类本身直接相关 
	int m_time_since_update;
	int m_hits;
	int m_hit_streak;
	int m_age;
	int m_id;
	int n;

	std::vector<float> y;
	std::vector<float> dy;


private:
	void init_kf(StateType stateMat);

	cv::KalmanFilter kf;
	cv::Mat measurement;
	
	std::vector<StateType> m_history;
};

#endif
