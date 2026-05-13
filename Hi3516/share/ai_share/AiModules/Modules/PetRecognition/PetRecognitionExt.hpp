/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-24 14:38:38
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-11-25 08:40:27
 * @FilePath: /1126/share/ai_share/AiModules/Modules/PetRecognition/PetRecognitionExt.hpp
 * @Description: 宠物识别
 */

#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace PetRecognition_NS
{
    /* 宠物识别参数 */
    typedef struct _PetRecognitionParam_
    {
        bool bEnable;                     /* 是否分析 */
        float fConfidence = 0.3;          /* 置信度 */
        unsigned int nDetectFrame = 1;    /* 检测多少帧才触发 */
    } PetRecognitionParam_S;

    /* 视频异常检测分析参数 */
    typedef struct _AnalyseParam_
    {
        PetRecognitionParam_S stPetRecognitionParam;

        float fBoxThreshold = 0.25; /* 框的置信度阈值 */
        float fNmsThreshold = -1;   /* 两个框的重叠程度 */
    } AnalyseParam_S;

    /* =========================================================================== */

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
        int   nClassId = -1;                   /* 宠物种类ID */
        // flozt fConfidence;
        float fX1      = 0;                    /* 左上角坐标 */
        float fY1      = 0;                    /* 左上角坐标 */
        float fX2      = 0;                    /* 右下角坐标 */
        float fY2      = 0;                    /* 右下角坐标 */
    } Result_S;

} // namespace PetRecognition_NS