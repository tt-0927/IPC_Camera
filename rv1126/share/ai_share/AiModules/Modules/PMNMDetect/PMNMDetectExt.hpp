/**
 * @file PMNMDetectExt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-22
 *
 * @brief 行人、机动车、非机动车检测
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace PMNMDetect_NS
{
    /* 越界类型枚举 */
    typedef enum _TripLineType_
    {
        OVERFLOW_NONE = 0, /* 空 */
        OVERFLOW_A_TO_B,   /* A->B */
        OVERFLOW_B_TO_A,   /* B->A */
        OVERFLOW_A_B_BOTH  /* A<->B */
    } TripLineType_E;

    /* 越界检测参数 */
    typedef struct _TripLineParam_
    {
        bool      bEnable = false;    /* 是否分析 */
        std::vector<int> veDetectionTargetTypes; /* 检测目标类型 */
        cv::Point alertLine1; /* 警戒线点1 */
        cv::Point alertLine2; /* 警戒线点2 */
    } TripLineParam_S;

    /* 入侵检测参数 */
    typedef struct _IntrusionParam_
    {
        bool                   bEnable = false;  /* 是否分析 */
        std::vector<int> veDetectionTargetTypes; /* 检测目标类型 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } IntrusionParam_S;

    /* 进入检测参数 */
    typedef struct _EntryParam_
    {
        bool                   bEnable = false;  /* 是否分析 */
        std::vector<int> veDetectionTargetTypes; /* 检测目标类型 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } EntryParam_S;

    /* 离开检测参数 */
    typedef struct _LeaveParam_
    {
        bool                   bEnable = false;  /* 是否分析 */
        std::vector<int> veDetectionTargetTypes; /* 检测目标类型 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } LeaveParam_S;

    /* 应急车道占用检测参数 */
    typedef struct _EmergencyLaneOccupancyParam_
    {
        bool                   bEnable = false;   /* 是否分析 */
        float                  fEmergencyLaneOccupancyThreshold = 0.5;  /* 触发应急车道占用事件的置信度阈值 */
        int                    nEmergencyLaneOccupancyTimeThreshold;    /* 触发应急车道占用事件时长阈值 */
        std::vector<int> veDetectionTargetTypes; /* 检测目标类型 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } EmergencyLaneOccupancyParam_S;

        /* 非机动车闯入检测参数 */
    typedef struct _NonMotorVehicleIntrusionParam_
    {
        bool                   bEnable = false;      /* 是否分析 */
        float                  fNonMotorVehicleIntrusionThreshold = 0.5;  /* 非机动车闯入的置信度阈值 */
        int                    nNonMotorVehicleIntrusionTimeThreshold;    /* 触发非机动车闯入事件时长阈值 */
        std::vector<int>       veDetectionTargetTypes; /* 检测目标类型 */
        std::vector<cv::Point> vecPoints; /* 警戒多边形参数 */
    } NonMotorVehicleIntrusionParam_S;

    /* 检测分析参数 */
    typedef struct _AnalyseParam_
    {
        TripLineParam_S  stTripLineParam;
        IntrusionParam_S stIntrusionParam;
        LeaveParam_S     stLeaveParam;
        EntryParam_S     stEntryParam;
        EmergencyLaneOccupancyParam_S stEmergencyLaneOccupancyParam;

        /* 多个区域使用 */
        bool                                            bVecEnable = false;
        std::vector<TripLineParam_S>                    vstTripLineParam;
        std::vector<IntrusionParam_S>                   vstIntrusionParam;
        std::vector<LeaveParam_S>                       vstLeaveParam;
        std::vector<EntryParam_S>                       vstEntryParam;
        std::vector<EmergencyLaneOccupancyParam_S>      vstEmergencyLaneOccupancyParam;
        std::vector<NonMotorVehicleIntrusionParam_S>    vstNonMotorVehicleIntrusionParam;

        float fBoxThreshold = 0.25; /* 框的置信度阈值 */
        float fNmsThreshold = -1;   /* 两个框的重叠程度 */
    } AnalyseParam_S;

    /* =========================================================================== */

    /* 初始化参数 */
    typedef struct _InParam_
    {
        std::string strModelPath; /* 模型路径 */

        /* 调试功能 */
        bool bDebug = false;             /* 是否开启调试功能 */
        std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
        std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */
    } InParam_S;

    /* 输入数据 */
    typedef struct _InData_
    {
        int nChnId;             /* 通道 */
        cv::Mat inMat;          /* 图片 */
        AnalyseParam_S stParam; /* 参数 */
    } InData_S;

    /* =========================================================================== */

    /* 检测结果 */
    typedef struct _OutData_
    {
        int            nChnId;          /* 通道 */
        int            nType;           /* 事件类型 */
        bool           validResult;     /* 标识有效结果 */
        std::string    savedFileName;   /* 分析后图片路径 */
    } OutData_S;

    /* 结果 */
    typedef struct _Result_
    {
        TripLineType_E enTripLineType = OVERFLOW_NONE; /* 越界类型 */
        bool bIntrusionFlag              = false; /* 是否入侵触发 */
        bool bEntryFlag                  = false; /* 是否进入触发 */
        bool bLeaveFlag                  = false; /* 是否离开触发 */
        bool bEmergencyLaneOccupancyFlag = false; /* 是否应急车道占用触发 */
        bool bNonMotorVehicleIntrusionFlag = false; /* 是否非机动车闯入触发 */

        float fX1 = 0;          /* 左上角x坐标 */
        float fY1 = 0;          /* 左上角y坐标 */
        float fX2 = 0;          /* 右下角x坐标 */
        float fY2 = 0;          /* 右下角y坐标 */
        float fBoxConfidence;   /* 置信度 */
        int nId     = 0;        /* 框ID */
        int nID = -1;           /* 种类ID */
        std::string sClassName; /* 种类名 */

    } Result_S;

} // namespace PMNMDetect_NS