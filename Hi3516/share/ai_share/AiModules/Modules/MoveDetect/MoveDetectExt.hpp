
#pragma once

#include "opencv2/opencv.hpp"

namespace MoveDetect_NS
{

    /* 检测初始化参数 */
    typedef struct _AnalyseParam_
    {
        int erode_size = 1;   /* 遮挡模糊程度阈值，越大越模糊 */
        int dilate_size = 20;  /* 遮挡模糊程度阈值，越大越模糊 */
        int nSkipFrames = 1; /* 检测间隔帧数 */

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

} // namespace MoveDetect_NS