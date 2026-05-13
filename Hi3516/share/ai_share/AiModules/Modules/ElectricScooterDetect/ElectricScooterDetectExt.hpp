/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-19 20:25:15
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-11-20 10:38:33
 * @FilePath: /1126/share/ai_share/AiModules/Modules/ElectricScooterDetect/ElectricScooterDetectExt.hpp
 * @Description: 电瓶车检测配置参数
 */

#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace ElectricScooterDetect_NS
{

    /*电瓶车识别参数 */
    typedef struct _ElectricScooterParam_
    {
        bool                   bEnable;     /* 是否分析 */
        float fConfidence = 0.5;            /* 置信度 */
        unsigned int           nDetectFrame = 1;    /* 检测多少帧才触发 */
    } ElectricScooterParam_S;


    /* 视频异常检测分析参数 */
    typedef struct _AnalyseParam_
    {
        ElectricScooterParam_S stElectricScooterParam;

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
        bool           bElectricScooter  = false;                   /* 是否电瓶车事件触发 */
    } Result_S;

} // namespace ElectricScooterDetect_NS