/*
 * @FilePath     : HumanAreaDetectExt.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-26 13:38:35
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-11-13 20:05:50
 * @Description  :
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace HumanAreaDetect_NS
{
    
    /* 越界检测参数 */
    typedef struct _TripLineParam_
    {
        bool      bEnable;    /* 是否分析 */
        cv::Point alertLine1; /* 警戒线点1 */
        cv::Point alertLine2; /* 警戒线点2 */
    } TripLineParam_S;

    /* 入侵检测参数 */
    typedef struct _IntrusionParam_
    {
        bool                   bEnable;   /* 是否分析 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } IntrusionParam_S;

    /* 进入检测参数 */
    typedef struct _EntryParam_
    {
        bool                   bEnable;   /* 是否分析 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } EntryParam_S;

    /* 离开检测参数 */
    typedef struct _LeaveParam_
    {
        bool                   bEnable;   /* 是否分析 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } LeaveParam_S;

    /* 徘徊检测参数 */
    typedef struct _LoiteringParam_
    {
        bool                   bEnable;   /* 是否分析 */
        unsigned int           nTimeThreshold = 0; /* 时间阈值触发 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } LoiteringParam_S;

    /* 翻越围栏检测参数 */
    typedef struct _FenceClimbingParam_
    {
        bool                   bEnable;   /* 是否分析 */
        unsigned int           nDetectFrame = 0;    /* 检测多少帧才触发 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } FenceClimbingParam_S;

     /* 离岗检测参数 */
    typedef struct _LeavePostParam_
    {
        bool                   bEnable;   /* 是否分析 */
        unsigned int           nTimeThreshold = 0; /* 时间阈值触发 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } LeavePostParam_S;

    /* 行人闯入检测参数 */
    typedef struct _PedestrianIntrusionParam_
    {
        bool                   bEnable;   /* 是否分析 */
        unsigned int           nTimeThreshold = 0; /* 时间阈值触发 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } PedestrianIntrusionParam_S;

    /* 人员聚集检测参数 */
    typedef struct _CrowdGatheringDetParam_
    {
        bool                   bEnable;   /* 是否分析 */
        unsigned int           nProportionThreshold = 0; /* 占比阈值触发 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } CrowdGatheringDetParam_S;

    /* 人员倒地检测参数 */
    typedef struct _PersonFallDownParam_
    {
        bool                   bEnable;   /* 是否分析 */
    } PersonFallDownParam_S;

    /* 检测分析参数 */
    typedef struct _AnalyseParam_
    {
        TripLineParam_S  stTripLineParam;
        IntrusionParam_S stIntrusionParam;
        LeaveParam_S     stLeaveParam;
        EntryParam_S     stEntryParam;
        PersonFallDownParam_S stPersonFallDownParam;

        /* 多个区域使用 */
        bool                                bVecEnable = false;
        std::vector<TripLineParam_S>        vstTripLineParam;
        std::vector<IntrusionParam_S>       vstIntrusionParam;
        std::vector<LeaveParam_S>           vstLeaveParam;
        std::vector<EntryParam_S>           vstEntryParam;
        std::vector<LoiteringParam_S>       vsLoiteringParam;
        std::vector<FenceClimbingParam_S>   vstFenceClimbingParam;
        std::vector<LeavePostParam_S>       vstLeavePostParam;
        std::vector<PedestrianIntrusionParam_S>       vstPedestrianIntrusionParam;
        std::vector<CrowdGatheringDetParam_S>         vstCrowdGatheringDetParam;

        float fBoxThreshold = 0.25; /* 框的置信度阈值 */
        float fNmsThreshold = -1;   /* 两个框的重叠程度 */
    } AnalyseParam_S;

    /* 边界检测初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath; /* 行人分析模型路径 */

        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */

    } InParam_S;

    /* =========================================================================== */

    /* 检测结果 */
    typedef struct _InData_
    {
        int            nChnId;          /* 通道 */
        cv::Mat        inMat;           /* 图片 */
        AnalyseParam_S stParam;         /* 参数 */
    } InData_S;

    /* 检测结果 */
    typedef struct _OutData_
    {
        int            nChnId;          /* 通道 */
        int            nType;           /* 事件类型 */
        bool           validResult;     /* 标识有效结果 */
        std::string    savedFileName;   /* 分析后图片路径 */
    } OutData_S;

    /* =========================================================================== */

    /* 越界类型枚举 */
    typedef enum _TripLineType_
    {
        OVERFLOW_NONE = 0, /* 空 */
        OVERFLOW_A_TO_B,   /* A->B */
        OVERFLOW_B_TO_A,   /* B->A */
        OVERFLOW_A_B_BOTH  /* A<->B */
    } TripLineType_E;

    /* 结果 */
    typedef struct _Result_
    {
        TripLineType_E enTripLineType = OVERFLOW_NONE; /* 越界类型 */
        bool           bIntrusionFlag = false;         /* 是否入侵触发 */
        bool           bEntryFlag     = false;         /* 是否进入触发 */
        bool           bLeaveFlag     = false;         /* 是否离开触发 */
        bool           bLoiteringFlag     = false;     /* 是否徘徊触发 */
        bool           bFenceClimbFlag     = false;    /* 是否翻越围栏触发 */
        bool           bLeavePostFlag = false;         /* 是否离岗触发 */
        bool           bPedestrianIntrusionFlag = false;         /* 是否闯入触发 */
        bool           bCrowdGatheringDetParamFlag = false;      /* 是否人员聚集触发 */
        bool           bPersonFalldownFlag = false;     /* 是否人员倒地触发 */

        int   nId     = 0;                             /* 框ID */
        float fX      = 0;                             /* 左上角坐标 */
        float fY      = 0;                             /* 左上角坐标 */
        float fWidth  = 0;                             /* 宽 */
        float fHeight = 0;                             /* 高 */
    } Result_S;

}    // namespace HumanAreaDetect_NS