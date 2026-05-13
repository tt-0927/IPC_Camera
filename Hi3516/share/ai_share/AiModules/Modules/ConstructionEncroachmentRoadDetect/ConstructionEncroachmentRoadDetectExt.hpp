/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-17 10:07:33
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-11-18 08:54:52
 * @FilePath: /1126/share/ai_share/AiModules/Modules/ConstructionEncroachmentRoadDetect/ConstructionEncroachmentRoadDetectExt.hpp
 * @Description: 施工占道检测
 */

#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace ConstructionEncroachmentRoadDetect_NS
{

    /* 施工占道检测参数 */
    typedef struct _ConstructionEncroachmentRoadParam_
    {
        bool                   bEnable;             /* 是否分析 */
        float                  fConfidence = 0.5;   /* 置信度 */
        unsigned int           nDetectFrame = 1;    /* 检测多少帧才触发 */
    } ConstructionEncroachmentRoadParam_S;

    /* 视频异常检测分析参数 */
    typedef struct _AnalyseParam_
    {
        ConstructionEncroachmentRoadParam_S stConstructionEncroachmentRoadParam;

        float fConfidence = 0.3;    /* 置信度 */
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

    /* 检测结果 */
    typedef struct _OutData_
    {
        int nChnId = 0;                            /* 通道 */

        bool bConstructionEncroachmentRoad = false;   /* 施工占道是否触发 */

        std::string savedFileName;            /* 分析后图片路径 */
    } OutData_S;


    /* 结果 */
    typedef struct _Result_
    {
        int   nType = 0;           /* 哪种侦测事件类别的结果 */ 
        int   nId     = 0;         /* 框ID */
        float fX      = 0;         /* 左上角坐标 */
        float fY      = 0;         /* 左上角坐标 */
        float fWidth  = 0;         /* 宽 */
        float fHeight = 0;         /* 高 */
    } Result_S;

} // namespace ConstructionEncroachmentRoadDetect_NS