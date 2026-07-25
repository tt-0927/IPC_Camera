/*
*  File Name: rk_facedetect_process.h
*  Created on: 2023年7月20日
*  Author: wcp
*  Modify date: 2023年7月21日
*/
 
 
#ifndef _RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_
#define _RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_

#include <stdint.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <set>
#include <iostream>
#include <cmath>
#include <vector>


#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

/******************************** 外部调用参数（可调） ******************************/
typedef struct _DETECTPARAM_{
    /* nms 的阈值 */
    float fNmsThreshold=0.25;
    /* 置信度阈值 */
    float fConfidentThreshold=0.7;
}DetectParam_S;
/**********************************************************************************/


/* 对输出的图片进行后处理 
 * pInput0： 神经网络的第一个输出
 * pInput1： 神经网络的第二个输出
 * pInput1： 神经网络的第三个输出
 * aSrc_img： 输入的视频流图片数据，用于绘制文字等
 * nHeight,nWidth：推理图片的宽高
*/
int ctrlNetoutputProcess(float *pInput0, float *pInput1,float *pInput2,int nHeight,
		int nWidth,std::vector<int> &vBox,DetectParam_S stDetectParam,int nModelNeed);

#endif //_RKNN_ZERO_COPY_DEMO_POSTPROCESS_H_
