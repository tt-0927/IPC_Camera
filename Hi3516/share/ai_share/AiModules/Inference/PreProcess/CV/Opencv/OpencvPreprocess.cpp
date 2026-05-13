/**
 * @file OpencvPreprocess.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-24
 * 
 * @brief 
 */

 #include "OpencvPreprocess.hpp"
#include <iostream>
 #include <algorithm>
 #include <string>
 #include <vector>
 #include <cmath>
 
 
 /* 数据预处理 */
 PreProcess_NS::COpencvPreProcess::COpencvPreProcess()
 {
 }
 PreProcess_NS::COpencvPreProcess::~COpencvPreProcess()
 {
 }
 
 bool PreProcess_NS::COpencvPreProcess::PreProcess(
    cv::Mat aInput,
    cv::Mat& aOutput,
    std::vector<float> vfMean,
    std::vector<float> vfStd,
    bool bRgb
)
{
    try 
    {
        /* 各通道的归一化倍数是否一样 */
        bool bNormal = std::adjacent_find(vfStd.begin(), vfStd.end(), std::not_equal_to<>()) == vfStd.end();
        /* 归一化 */
        if(!bNormal)
        {
            /* 1. 分离三个通道 */
            std::vector<cv::Mat> aChannels;
            cv::split(aInput, aChannels); 
            /* 2. 对每个通道分别进行缩放 */ 
            for(int nC=0; nC<aChannels.size(); nC++)
            {
                cv::multiply(aChannels[nC], 1.0 / vfStd[nC], aChannels[nC]); 
            }

            /* 3. 合并通道 */ 
            cv::merge(aChannels, aInput);
        }

        /* whc转为chw */
        /* 设置目标尺寸 */
        cv::Size stTargetSize = cv::Size();

        /* 设置方差 */
        float fSC = bNormal? (1.0/vfStd[0]) : 1.0;
        /* 设置均值 */
        cv::Scalar stvfMean = cv::Scalar(0, 0, 0);
        if(vfMean.size()==1)
        {
            stvfMean = cv::Scalar(vfMean[0]);
        }
        else if(vfMean.size()==3)
        {
            stvfMean = cv::Scalar(vfMean[0],vfMean[1],vfMean[2]);
        }

        /* 创建 4D blob，适用于神经网络输入 */
        aOutput = cv::dnn::blobFromImage(
            aInput,           /* 输入图像 */
            fSC,             /* 缩放因子 */
            stTargetSize,    /* 目标尺寸 */
            stvfMean,        /* 均值（减去） */
            bRgb,            /* 是否交换 BGR 和 RGB 通道 */
            false,           /* 是否裁剪图像 */
            CV_32F           /* 输出数据类型 */
        );
    }
    catch (const std::exception& e) 
    {
        std::cerr << "OpencvPreProcess处理报错： " << e.what() << std::endl;
        return false;
    }

     return true;
 }
 
