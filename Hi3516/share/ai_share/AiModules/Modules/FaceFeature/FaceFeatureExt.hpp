/**
 * @file FaceFeatureExt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-01
 * 
 * @brief 
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"


namespace FaceFeature_NS 
{
    /* 分析参数 */
    typedef struct _AnalyseParam_
    {
    } AnalyseParam_S;

    /* =========================================================================== */

    /* 初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath; /* 数字识别模型路径 */

        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */
    } InParam_S;


    /* 输入数据 */
    typedef struct _InData_
    {
        int            nChnId;          /* 通道 */
        cv::Mat        inMat;           /* 图片 */
        AnalyseParam_S stParam;         /* 参数 */
    } InData_S;

    /* 检测结果 */
    typedef struct _OutData_
    {
        int            nChnId;          /* 通道 */
        int            nType;           /* 事件类型 */
        bool           validResult;     /* 标识有效结果 */
        std::string    savedFileName;   /* 分析后图片路径 */
    } OutData_S;

    /* =========================================================================== */

}    // namespace FaceFeature_NS