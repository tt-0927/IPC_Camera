/**
 * @file VehiclePlateDetectExt.hpp
 * @author songww
 * @date 2025-10-29
 * 
 * @brief 车牌识别
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
// #include "CVInferenceMOL.hpp"
#include "OutputDataEXT.hpp"


using namespace Inference_NS;
namespace VehiclePlateDetect_NS
{
    /* 车牌识别初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath; /* 行人分析模型路径 */
        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */
    } InParam_S;

    /* =========================================================================== */
    /* 传入参数 */
    typedef struct _InData_
    {
        cv::Mat        inMat;           /* 图片 */
    } InData_S;
    /* 检测结果 */
    typedef struct _OutData_
    {
        std::vector<Inference_NS::ClsData_S> vClsDatas;             /* 车牌号码、颜色 */
    } OutData_S;

}    // namespace VehicleDetect_NS