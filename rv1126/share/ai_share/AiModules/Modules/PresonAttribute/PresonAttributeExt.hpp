/**
 * @file PresonAttributeExt.hpp
 * @author CaiShengJie (Caisj@kfb.cn)
 * @date 2024-10-10
 *
 * @brief
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace PresonAttribute_NS
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

    /* 结果 */
    typedef struct _Result_
    {
        char *strName;
        float fConfidence;
        bool bIsMale;             /* 是否是男性 */
        bool bBackPack   = false; /* 双肩包 */
        bool bHat        = false; /* 帽子 */
        bool bPostManBag = false; /* 邮差包 */
        bool bHandBag    = false; /* 手提袋 */
        bool bUmbrella   = false; /* 雨伞 */
        int  nAgeLabel;           /* 年龄标签 */
        int  nTopTypeLabel;       /* 上装类型标签 */
        int  nBottomTypeLabel;    /* 下装类型标签 */
        int  nTopColorLabel;      /* 上身颜色标签 */
        int  nBottomColorLabel;   /* 下身颜色标签 */
        // int  nItemLabel;        /* 物品标签 */
    } Result_S;

} // namespace NumberOcr_NS