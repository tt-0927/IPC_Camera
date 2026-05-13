/**
 * @file ParkingDetectExt.hpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2025-03-05
 * 
 * @brief 停车检测
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace ParkingDetect_NS
{
    /* 停车检测参数 */
    typedef struct _ParkingParam_
    {
        bool                   bEnable;   /* 是否分析 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } ParkingParam_S;

    /* 检测分析参数 */
    typedef struct _AnalyseParam_
    {
        ParkingParam_S stParkingParam;
        float fBoxThreshold = 0.25; /* 框的置信度阈值 */
        float fNmsThreshold = -1;   /* 两个框的重叠程度 */
        int nParkingTimeThreshold = 100; /* 停车时间阈值(单位：帧) */
        int nMaxLostFrames = 20; /* 最大丢失帧数 */
    } AnalyseParam_S;

    /* 停车检测初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath; /* 行人分析模型路径 */

        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */

    } InParam_S;

    /* =========================================================================== */

    /* 检测结果 */
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
        int   nId     = 0;                             /* 框ID */
        float fX      = 0;                             /* 左上角坐标 */
        float fY      = 0;                             /* 左上角坐标 */
        float fWidth  = 0;                             /* 宽 */
        float fHeight = 0;                             /* 高 */
    } Result_S;

}    // namespace ParkingDetect_NS