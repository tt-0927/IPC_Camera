/**
 * @file RubishDetectExt.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace RubishDetect_NS
{

    /*垃圾暴露检测参数 */
    typedef struct _GarbageExposureParam_
    {
        bool                   bEnable;     /* 是否分析 */
        float fConfidence = 0.5;            /* 置信度 */
    } GarbageExposureParam_S;

     /* 垃圾满溢检测参数 */
    typedef struct _GarbageOverflowParam_
    {
        bool                   bEnable;     /* 是否分析 */
        float fConfidence = 0.5;            /* 置信度 */
    } GarbageOverflowParam_S;

    /* 视频异常检测分析参数 */
    typedef struct _AnalyseParam_
    {
        GarbageExposureParam_S stGarbageExposureParam;
        GarbageOverflowParam_S stGarbageOverflowParam;

        float fConfidence = 0.3;    /* 置信度 */
        float fBoxThreshold = 0.25; /* 框的置信度阈值 */
        float fNmsThreshold = -1;   /* 两个框的重叠程度 */
    } AnalyseParam_S;

    /* =========================================================================== */

     /* 检测结果 */
    typedef struct _OutData_
    {
        int            nType;              /* 事件类型 */
        bool           validResult = false;     /* 标识有效结果 */
    } OutData_S;

    /* 初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath; /* 数字识别模型路径 */

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

    /* 结果 */
    typedef struct _Result_
    {
        bool           bGarbageExposure  = false;                   /* 是否垃圾暴露触发 */
        bool           bGarbageOverflow  = false;                   /* 是否垃圾满溢触发 */
    } Result_S;

} // namespace RubishDetect_NS