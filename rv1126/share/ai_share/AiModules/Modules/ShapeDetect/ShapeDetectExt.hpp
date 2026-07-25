/**
 * @file ShapeDetectExt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-25
 *
 * @brief
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace ShapeDetect_NS
{
    /* 分析参数 */
    typedef struct _AnalyseParam_
    {
        float dRCircularity = 0.85; /* 圆形的圆度阈值 */
        float dPCircularity = 0.5;  /* 多边形的圆度阈值 */

        float fEpsilonNum = 0.09; /* 原始轮廓与近似多边形之间的最大距离,越小会保留更多的细节 */
    } AnalyseParam_S;

    /* =========================================================================== */

    /* 初始化参数 */
    typedef struct _InParam_
    {
        /* 调试功能 */
        bool bDebug = false;             /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */
    } InParam_S;

    /* 输入数据 */
    typedef struct _InData_
    {
        cv::Mat inMat;          /* 图片 */
        AnalyseParam_S stParam; /* 参数 */
    } InData_S;

    /* =========================================================================== */

} // namespace ShapeDetect_NS