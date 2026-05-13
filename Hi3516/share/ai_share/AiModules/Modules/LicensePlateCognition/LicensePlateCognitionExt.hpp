/*
 * @FilePath     : LicensePlateKeyPointExt.hpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-10-09 13:59:59
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-10-09 13:59:59
 * @Description  :
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace LicensePlateCognition_NS
{
    /* 检测分析参数 */
    typedef struct _AnalyseParam_
    {
        float fBoxThreshold = 0.25; /* 框的置信度阈值 */
        float fNmsThreshold = -1;   /* 两个框的重叠程度 */
    } AnalyseParam_S;

    /* 车牌检测初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath1; /* 车牌检测模型路径 */
        std::string strModelPath2; /* 车牌识别模型路径 */

        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */

    } InParam_S;

    /* =========================================================================== */

    /* 检测输入数据 */
    typedef struct _InData_
    {
        cv::Mat inMat;          /* 图片 */
        AnalyseParam_S stParam; /* 参数 */
    } InData_S;

    /* =========================================================================== */

    /* 结果 */
    typedef struct _Result_
    {
        std::string licensePlateNumber;                /* 车牌号 */
        std::string licensePlateColor;                 /* 车牌颜色 */
        std::string licensePlateType;                  /* 车牌类型 */
        float fX      = 0;                             /* 左上角坐标 */
        float fY      = 0;                             /* 左上角坐标 */
        float fWidth  = 0;                             /* 宽 */
        float fHeight = 0;                             /* 高 */

    } Result_S;

}    // namespace LicensePlateCognition_NS