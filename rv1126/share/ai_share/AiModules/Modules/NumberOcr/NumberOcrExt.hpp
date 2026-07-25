/*
 * @FilePath     : NumberOcrExt.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-09-29 19:19:35
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-09-29 19:19:35
 * @Description  :
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"


namespace NumberOcr_NS
{
    /* 视频异常检测分析参数 */
    typedef struct _AnalyseParam_
    {
        int nFrame  =   10; /* 前后两帧结果间隔多少帧合格 */
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
        cv::Mat        inMat;   /* 图片 */
        AnalyseParam_S stParam; /* 参数 */
    } InData_S;

    /* =========================================================================== */

}    // namespace NumberOcr_NS