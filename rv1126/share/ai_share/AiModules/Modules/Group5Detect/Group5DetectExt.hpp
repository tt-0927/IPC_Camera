/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-12-24 16:22:37
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-08 20:18:08
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group5Detect/Group5DetectExt.hpp
 * @Description: metalFence(金属栅栏)、ConeTank(锥形桶)、CrashBarrels(防撞桶)、fence(防护栏)
 */

#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace Group5Detect_NS
{
    /* 识别参数 */
    typedef struct _Group5DetectParam_
    {
        bool bEnable = false;             /* 是否分析 */
        float fConfidence = 0.5;          /* 置信度 */
        unsigned int nDetectFrame = 1;    /* 检测多少帧才触发 */
    } Group5DetectParam_S;

    /* 检测分析参数 */
    typedef struct _AnalyseParam_
    {
        Group5DetectParam_S stHoleProtectionBarDetectParam;   /* 检测洞口防护栏参数 */
        Group5DetectParam_S stConstructionEncroachmentRoad;   /* 检测施工占道参数 */

        // float fConfidence = 0.3;    /* 置信度 */
        float fBoxThreshold = 0.25; /* 框的置信度阈值 */
        float fNmsThreshold = -1;   /* 两个框的重叠程度 */
    } AnalyseParam_S;

    /* =========================================================================== */

    /* 检测结果 */
    typedef struct _OutData_
    {
        std::string savedFileName; /* 分析后图片路径 */
        
        bool           bHoleProtectionBar = false;              /* 是否洞口防护栏触发 */
        bool           bConstructionEncroachmentRoad = false;   /* 是否施工占道触发 */ 
        // bool           validResult = false;                  /* 标识有效结果 */
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
        int            nChnId;  /* 通道 */
        cv::Mat inMat;          /* 图片 */
        AnalyseParam_S stParam; /* 参数 */
    } InData_S;

    /* =========================================================================== */

    /* 结果 */
    typedef struct _Result_
    {
        float fX1 = 0;          /* 左上角x坐标 */
        float fY1 = 0;          /* 左上角y坐标 */
        float fX2 = 0;          /* 右下角x坐标 */
        float fY2 = 0;          /* 右下角y坐标 */
        float fBoxConfidence;   /* 置信度 */
        int   nClassId = -1;    /* 种类ID */
    } Result_S;

} // namespace Group5Detect_NS