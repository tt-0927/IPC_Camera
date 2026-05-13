/*
*  Created on: 2024年3月7日
*  Author: wcp
*  description : 输入图片，然后返回一个vector<float>的结果
*  Modify date: 2024年3月7日
*/

#ifndef __RK_SHIP_DETECT_H__
#define __RK_SHIP_DETECT_H__

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <fstream> 
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include "ship_detect_process.h"


/* rknn的api */
#include "rknn_api.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#define STB_IMAGE_IMPLEMENTATION

using namespace std;


// struct ShipTrack_S
// {
// 	/* 船矩形框左上角x坐标 */
// 	int nX1;
// 	/* 船矩形框左上角y坐标*/     
// 	int nY1;
// 	/* 船矩形框右下角x坐标*/ 
// 	int nX2;
// 	/* 船矩形框右下角y坐标*/ 
// 	int nY2;
// 	/* 船的id*/ 
// 	int nId;
// 	/* 船的高度*/ 
// 	int nHigh;
// 	/* 船的角度*/ 
// 	float fAngle;
// 	/* 船的状态，0是异常，1是正常*/ 
// 	int nState;
// };


namespace ShipDetect_NS
{
class cRkShipDetect
{
	private:
	    /******************************** 默认参数 ******************************/
	    /* 返回值 */
	    short m_nRet;
	    int m_nModelDataSize;
	    /* rk模型地址 */
	    const char* m_pModelName;
	    /* 定义网络相关的数据 */
	    rknn_context uCtx;
	    rknn_input_output_num stIoNum;
	    rknn_tensor_attr stInputAttrs[1];
	    rknn_tensor_attr stOutputAttrs[3];
	    rknn_input aInputs[1];
	    rknn_output aOutputs[3];
	    /* 模型需要数据的宽高和通道  */
	    int nModelChannel;
	    int nModelWidth;
	    int nModelHeight;
	    /* 存放模型的二进制文件  */
	    unsigned char *pModelData;
	    
	    /******************************** 可调参数 ******************************/
	    /* 输入视频流数据的宽高和通道
	     * 这里视频流数据的宽高和通道，应该和模型宽高（nModelChannel、model_width、model_height）对应相等
	     * 模型查看网址：https://netron.app/，可打开rknn即可看到输入和输出的尺寸
	    */
	    int nImgWidth;
	    int nImgHeight;
	    int nImgChannel;	    

	    /******************************** 内部函数的定义，无需操作 ******************************/
	    /* 载入模型  */
	    unsigned char* LoadModel(const char* filename, int* model_size);
	    /* rknn模型的定义以及销毁函数 */
	    int RknnDetectInit();
	    int RknnDetectQueryInoutIo();
	    int RknnDetectDestory();

		/******************************** 船只状态跟踪 ******************************/
	    /*追踪算法需要的参数*/
		std::map<int, std::pair<int, int>> centerPoints;
		std::unordered_map<int, std::tuple<int, int, int, int>> last_known_bbs;
    	std::unordered_map<int, int> frame_without_detection;
		int idCount = 0;
		int removeThreshold = 40;

		/*帧计数*/
		int cnt = 0;
		int startCnt = 0;
		/*初始框*/
		bool is_first_detection = true;
		std::vector<float> first_bbox;

		/*返回结构体*/
		// std::vector<ShipTrack_S> Shiptrack;


	public:
		/* yolo的阈值 */
	    	float fBoxThreshold=0.25;
	    	float fNmsThreshold=0.45;

		/******************************** 外部调用函数 ******************************/
		/**
		  * 输入bgr格式的图片数据进行识别：
		  * aInputImg：输入的cv::Mat 格式的图片rgb格式数据
		  * OutputVector 识别完成后，输出的是一个长度为128长度的flaot特征点容器
		  * return 成功-> 0， 失败-> 1
		* */
		int DetectShipRgb(cv::Mat aInputImg,std::vector<float> &vPoints);

		/* 获取输入图片的大小 */
		void GetImgShape(int &nImgW,int &nImgH, int &nImgC){nImgW=nImgWidth;nImgH=nImgHeight;nImgC=nImgChannel;}
		/* 设置输入图片的大小 */
		//void SetImgShape(int nImgW,int nImgH, int nImgC){nImgWidth=nImgW;nImgHeight=nImgH;nImgChannel=nImgC;}
		/* 获取模型需要的图片大小 */
		void GetModelShape(int &nModelW,int &nModelH, int &nModelC){nModelW=nModelWidth;nModelH=nModelHeight;nModelC=nModelChannel;}

		/* EuclideanDistTracker目标追踪算法 */
		void EuclideanDistTracker(std::vector<float> &vPoints);

		/* EuclideanDistTracker2目标追踪算法 */
		void Eoptimized(std::vector<float> &vPoints);

		/* 船只角度计算 */
		void ShipState(std::vector<float> &vPoints, std::vector<float> &first_bbox);

		/** 构造函数
		     * 三个不同参数的构造函数
		     * pModelPath：模型的路径
		     * uInitImgWidth：模型需要图片的宽度
		     * uInitImgHeight：模型需要图片的高
		     * uInitImgChannel：模型需要图片的通道数
		**/
	    cRkShipDetect();
	    cRkShipDetect(char* pModelPath);


	    /* 析构函数 */
	    ~cRkShipDetect();
};
}
#endif // __RK_SHIP_DETECT_H__CT_H__H___