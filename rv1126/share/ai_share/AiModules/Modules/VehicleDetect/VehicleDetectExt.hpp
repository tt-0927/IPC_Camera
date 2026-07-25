/**
 * @file VehicleDetectExt.hpp
 * @author songww
 * @date 2025-10-29
 * 
 * @brief 车辆检测
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace VehicleDetect_NS
{
    /*车辆检测参数*/
    typedef struct _VehicleParam_
    {
        bool                   bEnable;   /* 是否分析 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } VehicleParam_S;

    /* 越界类型枚举 */
    typedef enum _TripLineType_
    {
        OVERFLOW_NONE = -1, /* 空 */
        OVERFLOW_A_B_BOTH,  /* A<->B */
        OVERFLOW_A_TO_B,    /* A->B */
        OVERFLOW_B_TO_A,    /* B->A */
    } TripLineType_E;

    /* 违规变道检测参数 */
    typedef struct _IllegalLaneChangeParam_
    {
        bool                   bEnable;   /* 是否分析 */
        cv::Point              alertLine1;/* 警戒线点1 */
        float                  fIllegalLaneChangeBoxThreshold = 0.25;   /* 触发违规变道事件框的阈值 */
        cv::Point              alertLine2;/* 警戒线点2 */
    } IllegalLaneChangeParam_S;

    /* 逆行检测参数 */
    typedef struct _DrivingAgainstTrafficParam_
    {
        bool                   bEnable;       /* 是否分析 */
        cv::Point              alertLine1;    /* 警戒线点1 */
        cv::Point              alertLine2;    /* 警戒线点2 */
        float                  fDrivingAgainstTrafficBoxThreshold = 0.25;   /* 触发逆行事件框的阈值 */
        TripLineType_E         eTripLineType = OVERFLOW_NONE; /* 检测逆行方向 */
    } DrivingAgainstTrafficParam_S;

    /* 停车检测参数 */
    typedef struct _ParkingParam_
    {
        bool                   bEnable;   /* 是否分析 */
        int                    nParkingTimeThreshold;   /* 触发违停事件时长阈值 */
        float                  fParkingBoxThreshold = 0.25;    /* 触发违停事件框的阈值 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } ParkingParam_S;

    /* 拥堵检测参数 */
    typedef struct _CongestionParam_
    {
        bool                   bEnable;   /* 是否分析 */
        float                  fCongestionBoxThreshold = 0.25; /* 框的置信度阈值 */
        int                    nCongestionThreshold = 10;      /* 车辆数量拥堵触发阈值 */
        // int                    nCongestionTimeThreshold;
    } CongestionParam_S;

    /* 检测分析参数 */
    typedef struct _AnalyseParam_
    {
        /* 逆行检测区域 */
        std::vector<DrivingAgainstTrafficParam_S>     vstDrivingAgainstTrafficParam;
        /* 违规变道检测区域 */
        std::vector<IllegalLaneChangeParam_S>         vstIllegalLaneChangeParam;
        /* 停车检测区域 */
        std::vector<ParkingParam_S>                   vstParkingParam;
        /* 车辆检测区域 */
        std::vector<VehicleParam_S>                   vsVehicleParam;
        /* 拥堵检测 */
        CongestionParam_S                             stCongestionParam;
        float fBoxThreshold = 0.25; /* 框的置信度阈值 */
        float fNmsThreshold = -1;   /* 两个框的重叠程度 */
        unsigned int detectionType = 0b0110; /* 检测类型默认 非机动/机动车(低->高) */
    } AnalyseParam_S;

    /* 车牌检测初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath; /* 行人分析模型路径 */
        /* 调试功能 */
        bool        bDebug = false;      /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */

    } InParam_S;

    /* =========================================================================== */

    /* 传入数据 */
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

    /* 结果 */
    typedef struct _Result_
    {

        bool           bIllegalLaneChangeFlag = false;         /* 是否违规变道触发 */
        bool           bDrivingAgainstTrafficFlag = false;     /* 是否逆行触发 */
        bool           bParkingFlag = false;                   /* 是否停车触发 */
        bool           bCongestionFlag = false;                /* 是否拥堵触发 */

        int   nId     = 0;                             /* 框ID */
        float fX      = 0;                             /* 左上角坐标 */
        float fY      = 0;                             /* 左上角坐标 */
        float fWidth  = 0;                             /* 宽 */
        float fHeight = 0;                             /* 高 */
    } Result_S;

}    // namespace VehicleDetect_NS