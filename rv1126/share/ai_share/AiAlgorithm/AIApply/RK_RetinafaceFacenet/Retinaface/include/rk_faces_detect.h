/*
*  File Name: rk_faces_detect.h
*  Created on: 2023年7月20日
*  Author: wcp
*  description : 人脸检测类接口定义
*       1、通过类.DetectFaceBgr，可以获得处理后的视频流，返回人脸框+5个关键点int的Vetor容器
*  Modify date: 2023年7月21日
*/

#ifndef __RK_FACES_DETECT_H__
#define __RK_FACES_DETECT_H__

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include<fstream> 
#include <string>

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include "rk_facedetect_process.h"
#include "rknn_api.h"

#define STB_IMAGE_IMPLEMENTATION

using namespace std;

class RK_FACES_DETECT
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
	    /******************************** 外部可调参数 ******************************/
	    /* (动态变化的结构体)向图片绘制的相关可调参数，定义在postprocess.h中 */
	    DetectParam_S stDetectParam;

	   /******************************** 外部调用函数 ******************************/
	   
	    /**
	     * 输入bgr格式的图片数据进行识别：
	     * data_buffer：输入的char* 格式的图片bgr格式数据
	     * length：           图片数据的长度
	     * outData_buffer 识别完成后，输出的图片数据
	     * return 成功-> 0， 失败-> 非0
	     * */
	    int DetectFaceBgr(char* aInputImg, std::vector<int> &vBoxPoint);
		int DetectFaceBgr(cv::Mat InputImg, std::vector<int>& vBoxPoint);

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
	    RK_FACES_DETECT();
	    RK_FACES_DETECT(char* pModelPath);
	    RK_FACES_DETECT(char* pModelPath, int uInitImgWidth, int uInitImgHeight, int uInitImgChannel);

	    /* 析构函数 */
	    ~RK_FACES_DETECT();
};

#endif // __RK_FACES_DETECT_H__
