/**
 * @file FaceQualityAssessmentExt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-12-05
 * 
 * @brief 
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"


namespace FaceQualityAssessment_NS 
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

    
    /* 结果 */
    typedef struct _Result_
    {
        float fX1 = 0.0f;            /* 左上角x坐标 */
        float fY1 = 0.0f;            /* 左上角y坐标 */
        float fX2 = 0.0f;            /* 右下角x坐标 */
        float fY2 = 0.0f;            /* 右下角y坐标 */
        std::vector<float> vPoint;   /* 人脸5个特征点 */
        float fBoxConfidence = 0.0f; /* 置信度 */
        float fFqaSouce = 0.0f;      /* 人脸质量评分*/
    } Result_S;

    /* =========================================================================== */

}    // namespace FaceQualityAssessment_NS