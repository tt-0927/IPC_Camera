/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 16:28:43
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-08 20:15:56
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group3Detect/Group3DetectExt.hpp
 * @Description: smoke(烟雾)、fire(火焰)、Overflow(垃圾满溢)、expose(垃圾暴露)、Complete(井盖完好)、Damaged(井盖破损)、Lost(井盖丢失)、Uncovered(未盖井盖)、BreakoutOfOuterEdge(井盖外边沿破损)、WaterAccumulation(道路积水)
 */

#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace Group3Detect_NS {
/* 识别参数 */
typedef struct _Group3DetectParam_
{
    bool         bEnable      = false; /* 是否分析 */
    float        fConfidence  = 0.5;   /* 置信度 */
    unsigned int nDetectFrame = 1;     /* 检测多少帧才触发 */
} Group3DetectParam_S;

/* 检测分析参数 */
typedef struct _AnalyseParam_
{
    Group3DetectParam_S stSmokeFireDetectParam;      /* 烟雾 识别 */
    Group3DetectParam_S stOpenFlameParam;            /* 火焰 识别 */
    Group3DetectParam_S stGarbageExposureParam;      /* 垃圾暴露 识别 */
    Group3DetectParam_S stGarbageOverParam;          /* 垃圾满溢 识别 */
    Group3DetectParam_S stManholeCoverAbnormalParam; /* 井盖异常 识别 */
    Group3DetectParam_S stRoadPondingParam;          /* 道路积水 识别 */

    // float fConfidence = 0.3;    /* 置信度 */
    float fBoxThreshold = 0.25; /* 框的置信度阈值 */
    float fNmsThreshold = -1;   /* 两个框的重叠程度 */
} AnalyseParam_S;

/* =========================================================================== */

/* 检测结果 */
typedef struct _OutData_
{
    std::string savedFileName; /* 分析后图片路径 */
    
    bool bSmoke                = false; /* 是否 烟雾 触发 */
    bool bOpenFire             = false; /* 是否 火焰 触发 */
    bool bGarbageExposure      = false; /* 是否 垃圾暴露 触发 */
    bool bGarbageOver          = false; /* 是否 垃圾满溢 触发 */
    bool bManholeCoverAbnormal = false; /* 是否 井盖异常 触发 */
    bool bRoadPonding          = false; /* 是否 道路积水 触发 */
    // bool validResult           = false; /* 标识有效结果 */
} OutData_S;

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
    int            nChnId;  /* 通道 */
    cv::Mat        inMat;   /* 图片 */
    AnalyseParam_S stParam; /* 参数 */
} InData_S;

/* =========================================================================== */

/* 结果 */
typedef struct _Result_
{
    float fX1 = 0;        /* 左上角x坐标 */
    float fY1 = 0;        /* 左上角y坐标 */
    float fX2 = 0;        /* 右下角x坐标 */
    float fY2 = 0;        /* 右下角y坐标 */
    float fBoxConfidence; /* 置信度 */
    int   nClassId = -1;  /* 种类ID */
} Result_S;

}  // namespace Group3Detect_NS