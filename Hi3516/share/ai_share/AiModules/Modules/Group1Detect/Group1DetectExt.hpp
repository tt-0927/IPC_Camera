/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-12-24 16:22:37
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-08 20:17:08
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group1Detect/Group1DetectExt.hpp
 * @Description: notHelmet(未戴安全帽)、helmet(安全帽)、reflective(反光衣)、safetyRope(安全绳)、exposedSoil(泥土裸露)、person(人)
 */

#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace Group1Detect_NS {
/* 识别参数 */
typedef struct _Group1DetectParam_
{
    bool         bEnable      = false; /* 是否分析 */
    float        fConfidence  = 0.5;   /* 置信度 */
    unsigned int nDetectFrame = 1;     /* 检测多少帧才触发 */
} Group1DetectParam_S;

/* 检测分析参数 */
typedef struct _AnalyseParam_
{
    Group1DetectParam_S stSafetyHelmetDetectParam;   /* 安全帽识别 */
    Group1DetectParam_S stReflectiveClothingParam;   /* 反光衣识别 */
    Group1DetectParam_S stHighAltitudeSeatbeltParam; /* 高空安全带识别 */
    Group1DetectParam_S stBareSoiletParam;           /* 泥土裸露 */

    // float fConfidence = 0.3;    /* 置信度 */
    float fBoxThreshold = 0.25; /* 框的置信度阈值 */
    float fNmsThreshold = -1;   /* 两个框的重叠程度 */
} AnalyseParam_S;

/* =========================================================================== */

/* 检测结果 */
typedef struct _OutData_
{
    bool bSafetyHelmet         = false; /* 是否 安全帽识别 触发 */
    bool bReflectiveClothing   = false; /* 是否 反光衣识别 触发 */
    bool bHighAltitudeSeatbelt = false; /* 是否 高空安全带识别 触发 */
    bool bBareSoilet           = false; /* 是否 泥土裸露 触发 */
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

}  // namespace Group1Detect_NS