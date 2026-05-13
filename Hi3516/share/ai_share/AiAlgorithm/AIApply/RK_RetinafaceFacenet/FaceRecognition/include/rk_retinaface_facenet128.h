/*
*  File Name: rk_human_count_detect.h
*  Created on: 2023年7月13日
*  Author: wcp
*  description : 人数统计类接口定义
*  Modify date: 2023年7月18日
*/

#ifndef __RK_RETINAFACE_FACENET_H__
#define __RK_RETINAFACE_FACENET_H__

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include<fstream> 
#include <string>
#include <algorithm>
#include <numeric>
#include <functional>

#include "RgaUtils.h"
#include "im2d.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include "rknn_api.h"
#include "rk_mpi_mmz.h"

#include "rk_faces_detect.h"
#include "rk_face_feature.h"

#include <cmath>
#define M_PI 3.14159265358979323846


using namespace std;

struct PEOPLEFEATURES 
{
    float fBoxs[4];
    float fFeatures[128];
	char achName[256];
	int nResult;
};

/* 人脸识别 */
class RETINAFACE_FACENET
{
	private:
		/* 人脸检测和人脸特征提取图片 */
		cv::Mat aSrcImg;
		cv::Mat aFace;
		/* 输入图片的大小 */
		int nHeight = 1024;
		int nWidth  = 1920;

	public:
	    	
		/* 余弦相识度 */
		float CosineSimilarity(const float* vec1, const float* vec2, int size);
		/* 特征提取算法 
		@ pDataBuffer： 图片数据或视频流
		@ FaceDetect：   人脸检测网络初始化的对象
		@ Facenet128：   人脸特征提取网络初始化的对象
		@ vAllFeature： 网络特征提取的结果，包含多个容器———》fBoxs[4]:人脸的左上右下坐标；fFeatures[128]：128个人脸特征点
		*/
		int RetinafaceFacenetBgr(char *pDataBuffer,RK_FACES_DETECT &FaceDetect,RK_FACE_FEATURE &Facenet128,std::vector<PEOPLEFEATURES>& vAllFeature);
		int RetinafaceFacenetBgr(cv::Mat inMat, RK_FACES_DETECT& FaceDetect, RK_FACE_FEATURE& Facenet128, std::vector<PEOPLEFEATURES>& vAllFeature);

		/** 构造函数
		     * nInitImgWidth：模型需要图片的宽度
		     * nInitImgHeight：模型需要图片的高
		     * nInitImgChannel：模型需要图片的通道数
		* */
		RETINAFACE_FACENET();
		RETINAFACE_FACENET(int nInitImgWidth,int nInitImgHeight);

		/* 析构函数 */
		~RETINAFACE_FACENET();
};



#endif // __RK_RETINAFACE_FACENET_H__
