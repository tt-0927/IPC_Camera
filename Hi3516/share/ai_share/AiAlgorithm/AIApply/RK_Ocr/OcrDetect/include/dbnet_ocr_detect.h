/*
*  File Name: dbnet_ocr_detect.h
*  Created on: 2023年12月16日
*  Author: wcp
*  description : 获取输入CV::Mat格式的视频流中的文字位置	
*  Modify date: 2023年12月18日
*/
#ifndef _RKNN_DBOCR_DETECT_H_
#define _RKNN_DBOCR_DETECT_H_

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <fstream> 
#include <string>
#include <vector>


#include "rknn_api.h"
#include "dbnet_ocr_process.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#define STB_IMAGE_IMPLEMENTATION

using namespace std;

class RK_OCR_DETECT
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
	    rknn_tensor_attr stOutputAttrs[1];
	    rknn_input aInputs[1];
	    rknn_output aOutputs[1];
	    /* 模型需要数据的宽高和通道  */
	    int nModelChannel;
	    int nModelWidth;
	    int nModelHeight;
	    /* 存放模型的二进制文件  */
	    unsigned char *pModelData;
	    
	    /******************************** 可调参数 ******************************/
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

	public:
		/* 识别可调参数 */
		int DestWidth;
		int DestHeight;
		float fThreshold=0.1;
		float fBoxThresh=0.1;
		int nMaxCandidates=1000;
		float fUnclipRatio=2;

		/* 模型预处理 */
		void Precond(cv::Mat& aInputImg,cv::Size expected_size = cv::Size(640, 640));
		void PerspectiveTransform(cv::Mat aImg, std::vector<cv::Point2f>& vBoxes,cv::Mat& aRepImg);
		/******************************** 外部调用函数 ******************************/
		/**
		  * 输入rgb格式的图片数据进行识别：
		  * aInputImg：输入的cv::Mat 格式的图片rgb格式数据
		  * vBoxes 识别完成后，文字框坐标，坐标存储格式如下：
		  	- 第一层std::vector，表示是检测框个数
		  		- 第二次std::vector，有4个cv::Point2f（即4个坐标）
		  			- cv::Point2f P 表示坐标点，获取数据方式 x坐标：P.x ; y坐标：P.y
  		  * vScores ：对应每个识别框的得分
		  * return 成功-> 0， 失败-> 1
		* */
		int DetectOcrRgb(cv::Mat aInputImg,std::vector<std::vector<cv::Point2f>>& vBoxes,std::vector<float>& vScores);

		/* 获取输入图片的大小 */
		void GetImgShape(int &nImgW,int &nImgH, int &nImgC){nImgW=nImgWidth;nImgH=nImgHeight;nImgC=nImgChannel;}
		/* 设置输入图片的大小 */
		//void SetImgShape(int nImgW,int nImgH, int nImgC){nImgWidth=nImgW;nImgHeight=nImgH;nImgChannel=nImgC;}
		/* 获取模型需要的图片大小 */
		void GetModelShape(int &nModelW,int &nModelH, int &nModelC){nModelW=nModelWidth;nModelH=nModelHeight;nModelC=nModelChannel;}

		/** 构造函数
		     * 三个不同参数的构造函数
		     * pModelPath：模型的路径
		     * uInitImgWidth：模型需要图片的宽度
		     * uInitImgHeight：模型需要图片的高
		     * uInitImgChannel：模型需要图片的通道数
		* */
		    RK_OCR_DETECT();
		    RK_OCR_DETECT(char* pModelPath);


		    /* 析构函数 */
		    ~RK_OCR_DETECT();
};

#endif // _RKNN_DBOCR_DETECT_H_
