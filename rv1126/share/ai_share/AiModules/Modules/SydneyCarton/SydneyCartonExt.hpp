/**
 * @file SydneyCartonExt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-25
 * 
 * @brief 
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"


namespace SydneyCarton_NS
{
    /* 分析参数 */
    typedef struct _AnalyseParam_
    {
        int nShapeNum  =   3; /* 一个位置的图像，可以变化多少种格式 */
    } AnalyseParam_S;

    /* =========================================================================== */

    /* 初始化参数 */
    typedef struct _InParam_
    {
        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */
    } InParam_S;


    /* 输入数据 */
    typedef struct _InData_
    {
        cv::Mat        inMat;   /* 图片 */
        AnalyseParam_S stParam; /* 参数 */
    } InData_S;

    /* =========================================================================== */

}    // namespace SydneyCarton_NS