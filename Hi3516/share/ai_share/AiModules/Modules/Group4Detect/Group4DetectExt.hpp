/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 16:28:43
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-17 09:15:37
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group4Detect/Group4DetectExt.hpp
 * @Description: cigarette(香烟)、sleep(睡觉)、phone(玩手机)、fall(摔跤)、falling(摔跤中)
 */

#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace Group4Detect_NS {
/* 识别参数 */
typedef struct _Group4DetectParam_
{
    bool         bEnable      = false; /* 是否分析 */
    float        fConfidence  = 0.5;   /* 置信度 */
    unsigned int nDetectFrame = 1;     /* 检测多少帧才触发 */
} Group4DetectParam_S;

typedef struct _Group4DetectPersonParam_
{
    bool         bEnable         = false; /* 是否分析 */
    float        fConfidence     = 0.5;   /* 置信度 */
    unsigned int nDetectDuration = 1;     /* 检测多长时间才触发，默认1s */
} Group4DetectPersonParam;

/* 检测分析参数 */
typedef struct _AnalyseParam_
{
    Group4DetectPersonParam stCigaretteDetectParam; /* 香烟 识别 */
    Group4DetectParam_S     stSleepParam;           /* 睡觉 识别 */
    Group4DetectPersonParam stPhoneParam;           /* 玩手机 识别 */
    Group4DetectParam_S     stFallParam;            /* 摔倒 识别 */

    // float fConfidence = 0.3;    /* 置信度 */
    float fBoxThreshold = 0.25; /* 框的置信度阈值 */
    float fNmsThreshold = -1;   /* 两个框的重叠程度 */
} AnalyseParam_S;

/* =========================================================================== */

/* 检测结果 */
typedef struct _OutData_
{
    std::string savedFileName; /* 分析后图片路径 */
    
    bool bCigarette = false; /* 是否 香烟 触发 */
    bool bSleep     = false; /* 是否 睡觉 触发 */
    bool bPhone     = false; /* 是否 玩手机 触发 */
    bool bFall      = false; /* 是否 摔跤 触发 */
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

}  // namespace Group4Detect_NS