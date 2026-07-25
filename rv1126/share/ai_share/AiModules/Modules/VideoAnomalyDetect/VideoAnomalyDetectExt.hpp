/*
 * @FilePath     : VideoAnomalyDetectExt.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-09-29 19:19:35
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-09-29 19:19:35
 * @Description  :
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace VideoAnomalyDetect_NS
{

    /* 条纹检测算法 */
    typedef struct _StripesParam_
    {
        bool bEnable = false; /* 是否分析 */
        int nLineSize = 0;    /* 条纹直线大小阈值（默认为0，出现的所有直线） */
    } StripesParam_S;

    /* 亮暗检测算法 */
    typedef struct _LightDarkParam_
    {
        bool bEnable = false; /* 是否分析 */
        double fLDThres = 30; /* 亮暗阈值 */
    } LightDarkParam_S;

    /* 噪点检测算法 */
    typedef struct _NoiseParam_
    {
        bool bEnable = false;    /* 是否分析 */
        float fProportion = 0.1; /* 噪点个数占全部像素点的占比阈值 */
        float fSolidColorThes = 0.8; /* 纯色异常阈值 */
    } NoiseParam_S;

    /* 图片模糊度检测 */
    typedef struct _BlurrinessParam_
    {
        bool bEnable = false;   /* 是否分析 */
        double dBlurrThres = 3; /* 模糊阈值 */
    } BlurrinessParam_S;

    /* 视频异常检测分析参数 */
    typedef struct _AnalyseParam_
    {
        /* 定义背景颜色的范围 */
        cv::Scalar vLowerColor{0, 120, 70};
        cv::Scalar vUpperColor{10, 255, 255};

        StripesParam_S stStripesParam;
        LightDarkParam_S stLightDarkParam;
        NoiseParam_S stNoiseParam;
        BlurrinessParam_S stBlurrinessParam;

    } AnalyseParam_S;

    /* 边界检测初始化参数 */
    typedef struct _InParam_
    {
        /* 调试功能 */
        bool bDebug = false;             /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */

    } InParam_S;

    /* =========================================================================== */

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
        bool bStripesFlag = false;    /* 是否触发条纹事件 */
        bool bLightDarkFlag = false;  /* 是否触发亮暗事件 */
        bool bNoiseFlag = false;      /* 是否触发噪音事件 */
        bool bSolidColorFlag = false; /* 是否触发纯色异常事件 */
        bool bBlurrinessFlag = false; /* 是否触发模糊事件 */
    } Result_S;

} // namespace VideoAnomalyDetect_NS