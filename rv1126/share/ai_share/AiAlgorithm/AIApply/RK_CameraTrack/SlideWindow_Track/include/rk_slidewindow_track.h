
#ifndef __RK_ClIDEWINDOW_TRACK_H__
#define __RK_ClIDEWINDOW_TRACK_H__

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <fstream> 
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <functional>

#include <opencv2/core/core.hpp> // OpenCV 核心功能的头文件
#include <opencv2/imgproc/imgproc.hpp> // 图像处理功能的头文件
#include <opencv2/highgui/highgui.hpp> // 图形用户界面功能的头文件
#include "deepsort_feature.h" // 特征提取网络

using namespace std;


struct BoundingBox {
    float x, y, w, h;
};

class RK_ClIDEWINDOW_TRACK
{
	private:
		std::vector<float> vNewPoints;
        /* 连续5帧的特征信息 */
		std::vector<float*> vFeatures;
	public:
		/* 相识度阈值 */
		float fSimilarityThreshold = 0.9;
		/* 定义一个特征提取网络 */
		RK_DEEPSORT_FEATURE feature_demo;
		/* 人体特征缓存人 */
		bool _flag = true; // 用于第一次特征提取
 		float* pMyFeatures = new float[512];
	
		/* 上一帧的位置信息 */
		BoundingBox lastBox;
		
		int nIndexFeature=0;

		/* 缓存多少帧特征 */
		int m_nNumFeatures = 2;
		
               /* 余弦相似度函数 */
               float cosine_similarity(const float *vec1, const float *vec2, int size);
		/* 获取人物距离滑动窗口中心点的距离 */
		float Point_instance(float Sx, float Sy);
		/* 获取距离框中心最近的框*/
		void get_id(std::vector<float> vPoints,std::vector<float>& vNewPoints);
		/* 使用特征来寻找最相似的人 */
		bool get_feature(cv::Mat aImg,std::vector<float> vPoints,std::vector<float>& vNewPoints);
		/* 更新滑动窗口的信息 */
		void nextImg(cv::Mat aFrame, std::vector<float> vPoints, std::vector<float>& vNewPoints);
		/* 从人头坐标获取上半身的512特征点 */
		void get_head2body_feature(cv::Mat aImg,float x1,float y1, float x2, float y2,float* pFeatures);
		/* 构造函数 */
		RK_ClIDEWINDOW_TRACK();
		RK_ClIDEWINDOW_TRACK(char* cFacenetPath);
		/* 析构函数 */
		~RK_ClIDEWINDOW_TRACK();

        /**
         * @brief 清空保存的特征点
         * @return [*] 无
         * @note 
         */
        void clear_features();
};

#endif // __RK_ClIDEWINDOW_TRACK_H__
