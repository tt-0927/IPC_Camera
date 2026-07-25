/*
*  File Name: ocr_repvgg.h
*  Created on: 2023年12月18日
*  Author: wcp
*  description :
*	1、使用该模块，需要注意的是模型地址以及 cTextPath文本的地址
*	2、输入一个cv::Mat的图片，返回识别到的结果,如结构体OCRPRERESULTS（识别到的文字，每个字的置信度） 	
*  Modify date: 2023年12月18日
*/
#ifndef __RK_OCR_DETECT_H__
#define __RK_OCR_DETECT_H__

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <fstream> 
#include <string>
#include <vector>


#include "rknn_api.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#define STB_IMAGE_IMPLEMENTATION

using namespace std;

/* OCR返回结果结构体 */
struct OCRPRERESULTS
{
    std::string TextRest; // 识别到的文本
    std::vector<float> vScore; // 每个字的分数
};


class RK_OCR_REPVGG
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
		/* 读取文本数据 */
		char* cTextPath = "./weights/TextDict.txt";
		std::vector<std::string> vDictCharacter;
		void ReadText(char* cTextPath);
		/* 模型预处理 */
		void Precond(cv::Mat& aInputImg,cv::Size expected_size = cv::Size(448, 32));
		/******************************** 外部调用函数 ******************************/
		/**
		  * 输入rgb格式的图片数据进行识别：
		  * aInputImg：输入的cv::Mat 格式的图片rgb格式数据
		  * OcrData 返回识别到的结果,如结构体OCRPRERESULTS（识别到的文字，每个字的置信度） 	
		  * return 成功-> 0， 失败-> 1
		* */
		int DetectOcrRgb(cv::Mat aInputImg,OCRPRERESULTS &OcrData);

		/* 获取输入图片的大小 */
		void GetImgShape(int &nImgW,int &nImgH, int &nImgC){nImgW=nImgWidth;nImgH=nImgHeight;nImgC=nImgChannel;}
		/* 设置输入图片的大小 */
		//void SetImgShape(int nImgW,int nImgH, int nImgC){nImgWidth=nImgW;nImgHeight=nImgH;nImgChannel=nImgC;}
		/* 获取模型需要的图片大小 */
		void GetModelShape(int &nModelW,int &nModelH, int &nModelC){nModelW=nModelWidth;nModelH=nModelHeight;nModelC=nModelChannel;}

		/** 构造函数
		     * 三个不同参数的构造函数
		     * pModelPath：模型的路径
		     * cTextP：TextDict.txt文本，文字的解码器
		* */
		    RK_OCR_REPVGG();
		    RK_OCR_REPVGG(char* pModelPath);
		    RK_OCR_REPVGG(char* pModelPath,char* cTextP);

		    /* 析构函数 */
		    ~RK_OCR_REPVGG();
};

#endif // __RK_OCR_DETECT_H__
