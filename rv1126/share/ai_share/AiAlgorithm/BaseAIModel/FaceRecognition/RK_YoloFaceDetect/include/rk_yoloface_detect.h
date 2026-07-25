/*
 * @FilePath     : rk_yoloface_detect
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-03-21
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-03-21
 * @Description  : 人脸检测代码，输入图片，然后返回一个vector<float>的结果
 */

#ifndef __RK_YOLOFACE_DETECT_H__
#define __RK_YOLOFACE_DETECT_H__

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
#include "yoloface_detect_process.h"


/* rknn的api */
#include "rknn_api.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#define STB_IMAGE_IMPLEMENTATION

using namespace std;

namespace YoloFaceDetect_NS
{
class CRkYoloFaceDetect
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


	public:
		/* yolo的阈值 */
    	float fBoxThreshold=0.7;
    	float fNmsThreshold=0.45;

		/******************************** 外部调用函数 ******************************/
		
		/**
		 * @brief 人脸马赛克功能-均值滤波实现马赛克效果
		 * @param [cv::Mat] aInputImg: 输入的 格式的图片rgb格式数据
		 * @param [int] nX1,nY1,nX2,nY2: 人脸的左上坐标(X1，Y1)，右下坐标(X2,Y2)
		 * @param [int] nMosaicSize: 用于均值滤波的卷积核大小，默认选择10
		 * @note
		 */
		void MosaicFace(cv::Mat& aFace, int nX1, int nY1, int nX2, int nY2, int nMosaicSize);
		
		/**
		 * @brief 输入rgb格式的图片数据进行识别
		 * @param [cv::Mat] aInputImg: 输入的 格式的图片rgb格式数据
		 * @param [std::vector<float>] vPoints: 识别完成后，输出的是一个长度为128长度的flaot特征点容器
		 * @return [*] 成功-> 0， 失败-> 1
		 * @note
		 */
		int DetecFaceRgb(cv::Mat aInputImg,std::vector<float> &vPoints);

		/* 获取输入图片的大小 */
		void GetImgShape(int &nImgW,int &nImgH, int &nImgC){nImgW=nImgWidth;nImgH=nImgHeight;nImgC=nImgChannel;}
		/* 获取模型需要的图片大小 */
		void GetModelShape(int &nModelW,int &nModelH, int &nModelC){nModelW=nModelWidth;nModelH=nModelHeight;nModelC=nModelChannel;}

		/** 构造函数
		     * pModelPath：模型的路径
		**/
	    CRkYoloFaceDetect();
	    CRkYoloFaceDetect(char* pModelPath);


	    /* 析构函数 */
	    ~CRkYoloFaceDetect();
};
}
#endif // __RK_YOLOFACE_DETECT_H__
