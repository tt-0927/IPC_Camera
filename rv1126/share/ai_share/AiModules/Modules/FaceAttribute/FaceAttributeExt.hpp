/**
 * @file FaceAttributeExt.hpp
 * @author CaiShengJie (Caisj@kfb.cn)
 * @date 2024-10-10
 *
 * @brief
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace FaceAttribute_NS
{
    /* 视频异常检测分析参数 */
    typedef struct _AnalyseParam_
    {
        float fConfidence = 0.3; /* 置信度 */
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

    /* 年龄标签枚举 */
    typedef  enum class AgeLabel 
    {
        UNKNOWN = -1,
        CHILD   = 1,  // 小孩
        YOUTH   = 2,  // 青年
        MIDDLE  = 3,  // 中年
        OLD     = 4   // 老年
    }AgeLabel_E;

    /* 表情标签枚举 */
    typedef enum class EmotionLabel 
    {
        UNKNOWN   = -1,
        NEUTRAL   = 8,  // 中性
        ANGRY     = 9,  // 愤怒
        HAPPY     = 10, // 快乐
        SAD       = 11, // 悲伤
        SURPRISED = 12, // 惊讶
        FEAR      = 13, // 恐惧
        DISGUST   = 14  // 厌恶
    }EmotionLabel_E;

    /* 结果 */
    typedef struct _Result_
    {
        bool bIsMale;       /* 是否是男性 */
        int  nAgeLabel;     /* 年龄标签:1, 2, 3, 4 */
        bool bIsGlasses;    /* 是否戴眼镜 */
        bool bIsBeard;      /* 是否有胡子 */
        bool bIsMask;       /* 是否戴口罩 */
        int  nEmotionLabel; /* 年龄标签:8, 9, 10, 11, 12, 13, 14 */
    } Result_S;

} // namespace NumberOcr_NS