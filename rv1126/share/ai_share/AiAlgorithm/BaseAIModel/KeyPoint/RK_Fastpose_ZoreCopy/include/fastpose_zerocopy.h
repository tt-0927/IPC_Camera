/*
*  File Name: fastpose_zerocopy.h
*  Created on: 2024年7月18日
*  Author: wcp
*  description : 人体26个关键点识别
*  Modify date: 2024年7月18日
*/

#ifndef __RK_FASTPOSE_ZEROCOPY_H__
#define __RK_FASTPOSE_ZEROCOPY_H__

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <fstream> 
#include <string>

#include "rknn_api.h"
/* 自定义的预处理头文件 */
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"


using namespace std;

namespace Fastpose_NS{
class cFastpose
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
		/* 申请输入和输出数据内存 */
		rknn_tensor_mem *pInputMen[1];
		rknn_tensor_mem *pOutputMem[1];
		
		/* 设置输入数据的格式和数据类型 */
		unsigned char*     pInputData   = NULL; // 输入数据
		rknn_tensor_type   InputType   = RKNN_TENSOR_UINT8; // 输入数据类型
		rknn_tensor_type   OutputType   = RKNN_TENSOR_FLOAT32; // 输出数据类型
		rknn_tensor_format InputLayout = RKNN_TENSOR_NHWC; // 输入数据格式
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
		/* 零拷贝-将输入数据拷贝到输入张量内存 
		@ pDataBuffer 输入的图片数据
		*/ 
		void setInputDatas(unsigned char *pDataBuffer, int nInputIdex);

	public:
		/******************************** 外部调用函数 ******************************/
		/**
		  * 输入bgr格式的图片数据进行识别：
		  * aInputImg：输入的cv::Mat 格式的图片rgb格式数据
		  * vPoints:  26个人体关键点[x,y,置信度]
		  * return 成功-> 0， 失败-> 1
		* */
		int Infer(cv::Mat aInputImg,std::vector<float>& vPoints);
		
		/* 获取输入图片的大小 */
		void GetImgShape(int &nImgW,int &nImgH, int &nImgC){nImgW=nImgWidth;nImgH=nImgHeight;nImgC=nImgChannel;}
		/* 获取模型需要的图片大小 */
		void GetModelShape(int &nModelW,int &nModelH, int &nModelC){nModelW=nModelWidth;nModelH=nModelHeight;nModelC=nModelChannel;}

		/** 构造函数
		* 三个不同参数的构造函数
		* pModelPath：模型的路径
		* */
		cFastpose();
		cFastpose(char* pModelPath);

		/* 析构函数 */
		~cFastpose();
};
};
#endif // __RK_FASTPOSE_ZEROCOPY_H__
