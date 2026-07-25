/*
 * @FilePath     : AudioAnomalyDetectExt.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-09-29 19:19:35
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-09-29 19:19:35
 * @Description  :
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"


namespace AudioAnomalyDetect_NS
{

    /* 静音检测算法 */
    typedef struct _SilentDetecte_
    {
        bool      bEnable=false;                /* 是否分析 */
        int       fAbilityThres = 25;         /* 静音能力变化阈值(分贝)  */
    } SilentDetecte_S;

    /* 声音大忽小检测算法 */
    typedef struct _FluctuateDetect_
    {
        bool                   bEnable=false;               /* 是否分析 */
        float                  fAbilityThres = 20;        /* 声音能力变化阈值(分贝) */
    } FluctuateDetect_S;

 
    /* 视频异常检测分析参数 */
    typedef struct _AnalyseParam_
    {
        SilentDetecte_S      stSilentParam;
        FluctuateDetect_S    stFluctuateParam;

    } AnalyseParam_S;

    /* 边界检测初始化参数 */
    typedef struct _InParam_
    {
        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        const char* strAnalyzeDataPath = nullptr;    /* 设置分析后数据的保存路径, 文件夹路径 */
        const char* strOriginalDataPath = nullptr;   /* 原始数据保存路径, 文件夹路径 */
    } InParam_S;

    /* =========================================================================== */

    /* 输入数据 */
    typedef struct _InData_
    {
        char*        pData   = nullptr;     /* PCM数据 */
        int          nLength = 0;           /* PCM数据长度 */
        AnalyseParam_S stParam;             /* 参数 */
    } InData_S;

    /* =========================================================================== */

    /* 结果 */
    typedef struct _Result_
    {
        bool bSilentFlag                = false; /* 是否触发静音事件 */
        bool bFluctuateHighFlag         = false; /* 是否触发声音忽大事件 */
        bool bFluctuateLowFlag          = false; /* 是否触发声音忽小事件 */
    } Result_S;

}    // namespace VideoAnomalyDetect_NS