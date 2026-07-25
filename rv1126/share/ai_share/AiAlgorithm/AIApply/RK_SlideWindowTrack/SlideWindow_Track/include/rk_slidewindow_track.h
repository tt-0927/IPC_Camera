
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
		/******************************** 默认参数 ******************************/
		BoundingBox MainBox={640,640,160,320};
	public:
		/* 相识度阈值 */
		float fSimilarityThreshold = 0.9;
		/* 定义一个特征提取网络 */
		RK_DEEPSORT_FEATURE feature_demo;
		/* 人体特征缓存人 */
		bool _flag = true; // 用于第一次特征提取
 		float* pMyFeatures = new float[512];
	
	       /* 上一帧的位置信息 */
               //BoundingBox lastBox;
               
                /* 获取人物距离滑动窗口中心点的距离 */
		float Point_instance(float Sx, float Sy);
		/* 获取距离框中心最近的框*/
		void get_id(std::vector<float> vPoints,std::vector<float>& vNewPoints);
		/* 使用特征来寻找最相似的人 */
		bool get_feature(cv::Mat aImg,std::vector<float> vPoints,std::vector<float>& vNewPoints);
		/* 截图框调整 */
		void changeBox(cv::Mat aFrame);
		void changeBox(cv::Mat aFrame,int& x1_, int& y_1, int& x2_, int& y2_,
	int& top, int& left, int& bottom, int& right);
		/* 更换下一个截图框 */
		void get_newPoint(cv::Mat aFrame, std::vector<float> vPoints);
		/* 输入原图获取滑动窗口图 */
		void getImg(cv::Mat aFrame,cv::Mat& aSlideFrame);
		/* 更新滑动窗口的信息 */
		void nextImg(cv::Mat aFrame, std::vector<float> vPoints, cv::Mat& aNewFrame);
		/* 获取窗口的位置 */
		void getClide(float& x, float& y, float& w, float& h){x=MainBox.x; y=MainBox.y; w=MainBox.w; h=MainBox.h;};
		/* 从人头坐标获取上半身的512特征点 */
		void get_head2body_feature(cv::Mat aImg,float x1,float y1, float x2, float y2,float* pFeatures);
		/* 构造函数 */
		RK_ClIDEWINDOW_TRACK();
		RK_ClIDEWINDOW_TRACK(char* cFacenetPath);
		RK_ClIDEWINDOW_TRACK(float fXc,float fYc, float fWc, float fHc, char* cFacenetPath);
		/* 析构函数 */
		~RK_ClIDEWINDOW_TRACK();
};

#endif // __RK_ClIDEWINDOW_TRACK_H__
