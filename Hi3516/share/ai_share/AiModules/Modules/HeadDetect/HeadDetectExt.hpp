/**
 * @file HeadDetectExt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-02-20
 * 
 * @brief 人头检测
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace HeadDetect_NS
{
    /* 检测分析参数 */
    typedef struct _AnalyseParam_
    {
        float fBoxThreshold = 0.25; /* 框的置信度阈值 */
        float fNmsThreshold = -1;   /* 两个框的重叠程度 */
    } AnalyseParam_S;

    /* =========================================================================== */

    /* 初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath; /* 模型路径 */

        /* 调试功能 */
        bool bDebug = false;             /* 是否开启调试功能 */
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

    /* 结果 */
    typedef struct _Result_
    {
        float fX1 = 0;             /* 左上角x坐标 */
        float fY1 = 0;             /* 左上角y坐标 */
        float fX2 = 0;             /* 右下角x坐标 */
        float fY2 = 0;             /* 右下角y坐标 */
        float fBoxConfidence = 0.0f;  /* 置信度 */
        int nID = -1;              /* 种类ID */
        std::string sClassName;    /* 种类名 */
    } Result_S;

} // namespace HeadDetect_NS