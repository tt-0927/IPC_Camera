/**
 * @file CameraObstructionExt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-04
 * 
 * @brief 摄像头遮挡算法参数配置
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace CameraObstruction_NS
{

    /* 检测分析参数 */
    typedef struct _AnalyseParam_
    {
        double dThres = 0.9;    /* 遮挡模糊程度阈值，越大越模糊 */
    } AnalyseParam_S;

    /* 检测初始化参数 */
    typedef struct _InParam_
    {
        /* 调试功能 */
        bool bDebug = false;             /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */

    } InParam_S;

    /* =========================================================================== */

    /* 输入数据 */
    typedef struct _InData_
    {
        cv::Mat inMat;          /* 图片 */
        AnalyseParam_S stParam; /* 参数 */
    } InData_S;

    /* =========================================================================== */

    /* 结果 */
    typedef struct _Result_
    {
        bool bBlockFlag = false;    /* 是否触发遮挡事件 */
    } Result_S;

} // namespace CameraObstruction_NS