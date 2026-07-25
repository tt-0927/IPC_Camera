/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-09 11:34:24
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-02-02 15:10:21
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group2Detect/Group2DetectExt.hpp
 * @Description: 人、车、非相关事件检测
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace Group2Detect_NS {
/* 越界类型枚举 */
typedef enum _TripLineType_
{
    OVERFLOW_NONE = -1, /* 空 */
    OVERFLOW_A_B_BOTH,  /* A<->B */
    OVERFLOW_A_TO_B,    /* A->B */
    OVERFLOW_B_TO_A     /* B->A */
} TripLineType_E;

typedef enum _LibrarySeatType_
{
    NONE = 0, /* 空 */
    ENTRY_SEAT,     /* 占用座位 */
    LEAVE_SEAT,     /* 离开座位 */
 }LibrarySeatType_E;

/* 越界检测参数 */
typedef struct _TripLineParam_
{
    bool             bEnable            = false;    /* 是否分析 */
    float            fTripLineThreshold = 0.5;      /* 触发越界检测事件的置信度阈值 */
    std::vector<int> veDetectionTargetTypes;        /* 检测目标类型 0-人 1-机动车 2-非机动车 */
    TripLineType_E   eTripLineType = OVERFLOW_NONE; /* 检测越界方向 */
    cv::Point        alertLine1;                    /* 警戒线点1 */
    cv::Point        alertLine2;                    /* 警戒线点2 */
} TripLineParam_S;

/* 入侵检测参数 */
typedef struct _IntrusionParam_
{
    bool                   bEnable                 = false; /* 是否分析 */
    float                  fIntrusionThreshold     = 0.5;   /* 触发入侵检测事件的置信度阈值 */
    int                    nIntrusionTimeThreshold = 10;    /* 触发入侵检测事件时长阈值 */
    std::vector<int>       veDetectionTargetTypes;          /* 检测目标类型 */
    std::vector<cv::Point> vecPoints;                       /* 警戒多边形参数 */
} IntrusionParam_S;

/* 进入检测参数 */
typedef struct _EntryParam_
{
    bool                   bEnable         = false; /* 是否分析 */
    float                  fEntryThreshold = 0.5;   /* 触发进入检测事件的置信度阈值 */
    std::vector<int>       veDetectionTargetTypes;  /* 检测目标类型 */
    std::vector<cv::Point> vecPoints;               /* 警戒多边形参数 */
} EntryParam_S;

/* 人流统计参数 */
typedef struct _HeadCountParam_
{
    bool                   bEnable         = false; /* 是否分析 */
    float                  fThreshold = 0.5;   /* 触发进入检测事件的置信度阈值 */
    std::vector<int>       veDetectionTargetTypes;  /* 检测目标类型 */
    TripLineType_E         eTripLineType = OVERFLOW_NONE; /* 检测越界方向 */
    cv::Point              alertLine1;                    /* 警戒线点1 */
    cv::Point              alertLine2;                    /* 警戒线点2 */
} HeadCountParam_S;

/* 离开检测参数 */
typedef struct _LeaveParam_
{
    bool                   bEnable         = false; /* 是否分析 */
    float                  fLeaveThreshold = 0.5;   /* 触发离开检测事件的置信度阈值 */
    std::vector<int>       veDetectionTargetTypes;  /* 检测目标类型 */
    std::vector<cv::Point> vecPoints;               /* 警戒多边形参数 */
} LeaveParam_S;

/* 应急车道占用检测参数 */
typedef struct _EmergencyLaneOccupancyParam_
{
    bool                   bEnable                          = false; /* 是否分析 */
    float                  fEmergencyLaneOccupancyThreshold = 0.5;   /* 触发应急车道占用事件的置信度阈值 */
    int                    nEmergencyLaneOccupancyTimeThreshold;     /* 触发应急车道占用事件时长阈值 */
    std::vector<int>       veDetectionTargetTypes;                   /* 检测目标类型 */
    std::vector<cv::Point> vecPoints;                                /* 警戒多边形参数 */
} EmergencyLaneOccupancyParam_S;

/* 非机动车闯入检测参数 */
typedef struct _NonMotorVehicleIntrusionParam_
{
    bool                   bEnable                            = false; /* 是否分析 */
    float                  fNonMotorVehicleIntrusionThreshold = 0.5;   /* 非机动车闯入的置信度阈值 */
    int                    nNonMotorVehicleIntrusionTimeThreshold;     /* 触发非机动车闯入事件时长阈值 */
    std::vector<int>       veDetectionTargetTypes;                     /* 检测目标类型 */
    std::vector<cv::Point> vecPoints;                                  /* 警戒多边形参数 */
} NonMotorVehicleIntrusionParam_S;

/* 非机动车车道检测参数 */
typedef struct _NonVehicleLaneDetParam_
{
    bool                   bEnable                            = false; /* 是否分析 */
    float                  fThreshold = 0.5;                           /* 非机动车的置信度阈值 */
    std::vector<int>       veDetectionTargetTypes;                     /* 检测目标类型 */
    std::vector<cv::Point> vecPoints;                                  /* 警戒多边形参数 */
} NonVehicleLaneDetParam_S;


/*电瓶车识别参数 */
typedef struct _ElectricScooterParam_
{
    bool         bEnable      = false; /* 是否分析 */
    float        fConfidence  = 0.5;   /* 置信度 */
    unsigned int nDetectFrame = 1;     /* 检测多少帧才触发 */
} ElectricScooterParam_S;

/* 徘徊检测参数 */
typedef struct _LoiteringParam_
{
    bool                   bEnable             = false; /* 是否分析 */
    float                  fLoiteringThreshold = 0.5;   /* 徘徊检测的置信度阈值 */
    unsigned int           nTimeThreshold      = 0;     /* 时间阈值触发 */
    std::vector<cv::Point> vecPoints;                   /* 警戒多边形参数 */
} LoiteringParam_S;

/* 翻越围栏检测参数 */
typedef struct _FenceClimbingParam_
{
    bool                   bEnable                 = false; /* 是否分析 */
    float                  fFenceClimbingThreshold = 0.5;   /* 翻越围栏的置信度阈值 */
    unsigned int           nDetectFrame            = 0;     /* 检测多少帧才触发 */
    std::vector<cv::Point> vecPoints;                       /* 警戒多边形参数 */
} FenceClimbingParam_S;

/* 离岗检测参数 */
typedef struct _LeavePostParam_
{
    bool                   bEnable             = false; /* 是否分析 */
    float                  fLeavePostThreshold = 0.5;   /* 离岗检测的置信度阈值 */
    unsigned int           nTimeThreshold      = 0;     /* 时间阈值触发 */
    std::vector<cv::Point> vecPoints;                   /* 警戒多边形参数 */
} LeavePostParam_S;

/* 图书馆空位参数 */
typedef struct _LibraryVacanciesParam_
{
    bool                   bEnable             = false; /* 是否分析 */
    float                  fThreshold = 0.5;   /* 图书馆空位检测的置信度阈值 */
    unsigned int           nEntryTimeThreshold      = 5;     /* 占用座位时间阈值触发 */
    unsigned int           nLeaveTimeThreshold      = 5;     /* 离开座位时间阈值触发 */
    std::vector<cv::Point> vecPoints;                   /* 警戒多边形参数 */
} LibraryVacanciesParam_S;

/* 行人闯入检测参数 */
typedef struct _PedestrianIntrusionParam_
{
    bool                   bEnable                       = false; /* 是否分析 */
    float                  fPedestrianIntrusionThreshold = 0.5;   /* 行人闯入的置信度阈值 */
    unsigned int           nTimeThreshold                = 0;     /* 时间阈值触发 */
    std::vector<cv::Point> vecPoints;                             /* 警戒多边形参数 */
} PedestrianIntrusionParam_S;

/* 人员聚集检测参数 */
typedef struct _CrowdGatheringDetParam_
{
    bool                   bEnable              = false; /* 是否分析 */
    unsigned int           nProportionThreshold = 0;     /* 占比阈值触发 */
    std::vector<cv::Point> vecPoints;                    /* 警戒多边形参数 */
} CrowdGatheringDetParam_S;

/* 楼道拥挤检测参数 */
typedef struct _StairwellDetParam_
{
    bool                   bEnable              = false; /* 是否分析 */
    unsigned int           nProportionThreshold = 0;     /* 占比阈值触发 */
    std::vector<cv::Point> vecPoints;                    /* 警戒多边形参数 */
} StairwellDetParam_S;

/* 人员倒地检测参数 */
typedef struct _PersonFallDownParam_
{
    bool         bEnable                  = false; /* 是否分析 */
    float        fPersonFallDownThreshold = 0.5;   /* 人员倒地的置信度阈值 */
    unsigned int nTimeThreshold           = 0;     /* 时间阈值触发 */
} PersonFallDownParam_S;

/* 违规变道检测参数 */
typedef struct _IllegalLaneChangeParam_
{
    bool      bEnable = false;                       /* 是否分析 */
    cv::Point alertLine1;                            /* 警戒线点1 */
    float     fIllegalLaneChangeBoxThreshold = 0.25; /* 触发违规变道事件框的阈值 */
    cv::Point alertLine2;                            /* 警戒线点2 */
} IllegalLaneChangeParam_S;

/* 逆行检测参数 */
typedef struct _DrivingAgainstTrafficParam_
{
    bool           bEnable = false;                                    /* 是否分析 */
    cv::Point      alertLine1;                                         /* 警戒线点1 */
    cv::Point      alertLine2;                                         /* 警戒线点2 */
    float          fDrivingAgainstTrafficBoxThreshold = 0.25;          /* 触发逆行事件框的阈值 */
    TripLineType_E eTripLineType                      = OVERFLOW_NONE; /* 检测逆行方向 */
} DrivingAgainstTrafficParam_S;

/* 停车检测参数 */
typedef struct _ParkingParam_
{
    bool                   bEnable = false;             /* 是否分析 */
    int                    nParkingTimeThreshold;       /* 触发违停事件时长阈值 */
    float                  fParkingBoxThreshold = 0.25; /* 触发违停事件框的阈值 */
    std::vector<cv::Point> vecPoints;                   /* 警戒多边形参数 */
} ParkingParam_S;

/* 拥堵检测参数 */
typedef struct _CongestionParam_
{
    bool  bEnable                 = false; /* 是否分析 */
    float fCongestionBoxThreshold = 0.25;  /* 框的置信度阈值 */
    int   nCongestionThreshold    = 10;    /* 车辆数量拥堵触发阈值 */
    // int                    nCongestionTimeThreshold;
} CongestionParam_S;

/* 检测分析参数 */
typedef struct _AnalyseParam_
{
    TripLineParam_S               stTripLineParam;
    IntrusionParam_S              stIntrusionParam;
    LeaveParam_S                  stLeaveParam;
    EntryParam_S                  stEntryParam;
    EmergencyLaneOccupancyParam_S stEmergencyLaneOccupancyParam;
    HeadCountParam_S              stHeadCountParam;
    /* 多个区域使用 */
    bool bVecEnable = false;

    /* ============================ 人车非检测相关 ============================ */
    /* 越界检测 */
    std::vector<TripLineParam_S> vstTripLineParam;
    /* 入侵检测 */
    std::vector<IntrusionParam_S> vstIntrusionParam;
    /* 离开检测 */
    std::vector<LeaveParam_S> vstLeaveParam;
    /* 进入检测 */
    std::vector<EntryParam_S> vstEntryParam;
    /* 应急车道检测区域 */
    std::vector<EmergencyLaneOccupancyParam_S> vstEmergencyLaneOccupancyParam;
    /* ============================ 人车非检测相关 ============================ */

    /* ============================ 人检测相关 ============================ */
    /* 徘徊检测 */
    std::vector<LoiteringParam_S> vsLoiteringParam;
    /*  翻越围栏检测 */
    std::vector<FenceClimbingParam_S> vstFenceClimbingParam;
    /*  离岗检测 */
    std::vector<LeavePostParam_S> vstLeavePostParam;
    /*  行人闯入识别 */
    std::vector<PedestrianIntrusionParam_S> vstPedestrianIntrusionParam;
    /*  人员聚集侦测 */
    std::vector<CrowdGatheringDetParam_S> vstCrowdGatheringDetParam;
    /* 楼道拥挤检测 */
    std::vector<StairwellDetParam_S> vstStairwellDetParam;
    /*  人员倒地识别 */
    PersonFallDownParam_S stPersonFallDownParam;
    /* 图书馆空位 */
    LibraryVacanciesParam_S stLibraryVacanciesDetectParam;
    /* ============================ 人检测相关 ============================ */

    /* ============================ 机动车检测相关 ============================ */
    /* 逆行检测区域 */
    std::vector<DrivingAgainstTrafficParam_S> vstDrivingAgainstTrafficParam;
    /* 违规变道检测区域 */
    std::vector<IllegalLaneChangeParam_S> vstIllegalLaneChangeParam;
    /* 停车检测区域 */
    std::vector<ParkingParam_S> vstParkingParam;
    /* 拥堵检测 */
    CongestionParam_S stCongestionParam;
    /* ============================ 机动车检测相关 ============================ */

    /* ============================ 非机动车检测相关 ============================ */
    /* 非机动车闯入识别 */
    std::vector<NonMotorVehicleIntrusionParam_S> vstNonMotorVehicleIntrusionParam;
    /* 非机动车车道检测 */
    std::vector<NonVehicleLaneDetParam_S> vstNonVehicleLaneDetParam;
    /* 电瓶车进电梯识别 */
    ElectricScooterParam_S stElectricScooterParam;
    /* ============================ 非机动车检测相关 ============================ */

    float fBoxThreshold = 0.25; /* 框的置信度阈值 */
    float fNmsThreshold = -1;   /* 两个框的重叠程度 */
} AnalyseParam_S;

/* =========================================================================== */

/* 初始化参数 */
typedef struct _InParam_
{
    std::string strModelPath; /* 模型路径 */

    /* 调试功能 */
    bool        bDebug = false;      /* 是否开启调试功能 */
    std::string strAnalyzeDataPath;  /* 设置分析后数据的保存路径, 文件夹路径 */
    std::string strOriginalDataPath; /* 原始数据保存路径, 文件夹路径 */
} InParam_S;

/* 输入数据 */
typedef struct _InData_
{
    int            nChnId;  /* 通道 */
    cv::Mat        inMat;   /* 图片 */
    AnalyseParam_S stParam; /* 参数 */
} InData_S;

/* =========================================================================== */

/* 检测结果 */
typedef struct _OutData_
{
    int nChnId; /* 通道 */
    // int nType;  /* 事件类型 */
    // bool        validResult;   /* 标识有效结果 */
    std::string savedFileName; /* 分析后图片路径 */
    
    std::vector<int> nCountType;    /* 人流统计越界类型 */
    int nLibraryVacanciesType = -1;/* 图书馆空位类型 */
#if defined(RK_3588) || defined(RK3576)
    int nTripLineType               = -1; /* 越界类型 */
    bool bTripLineType               = false; /* 越界触发 */
#else
    bool bTripLineType               = false; /* 越界类型 */
#endif
    bool bIntrusionFlag              = false; /* 是否入侵触发 */
    bool bEntryFlag                  = false; /* 是否进入触发 */
    bool bLeaveFlag                  = false; /* 是否离开触发 */
    bool bEmergencyLaneOccupancyFlag = false; /* 是否应急车道占用触发 */

    bool bLoiteringFlag           = false; /* 是否徘徊触发 */
    bool bFenceClimbFlag          = false; /* 是否翻越围栏触发 */
    bool bLeavePostFlag           = false; /* 是否离岗触发 */
    bool bPedestrianIntrusionFlag = false; /* 是否闯入触发 */

    bool bCrowdGatheringDetParamFlag = false; /* 是否人员聚集触发 */
    bool bStairwellIntrusionFlag     = false; /* 是否楼道拥挤触发 */
    bool bPersonFalldownFlag         = false; /* 是否人员倒地触发 */

    bool bIllegalLaneChangeFlag     = false; /* 是否违规变道触发 */
    bool bDrivingAgainstTrafficFlag = false; /* 是否逆行触发 */
    bool bParkingFlag               = false; /* 是否停车触发 */
    bool bCongestionFlag            = false; /* 是否拥堵触发 */

    bool bNonMotorVehicleIntrusionFlag = false; /* 是否非机动车闯入触发 */
    bool bNonVehicleLaneDetFlag        = false;            /* 是否非机动车车道检测触发 */
    bool bElectricScooter              = false; /* 是否电瓶车事件触发 */
} OutData_S;

/* 结果 */
typedef struct _Result_
{
    float       fX1 = 0;        /* 左上角x坐标 */
    float       fY1 = 0;        /* 左上角y坐标 */
    float       fX2 = 0;        /* 右下角x坐标 */
    float       fY2 = 0;        /* 右下角y坐标 */
    float       fBoxConfidence; /* 置信度 */
    int         nId = 0;        /* 框ID */
    int         nID = -1;       /* 种类ID: 0-人 1-机动车 2-非机动车 */
    std::string sClassName;     /* 种类名 */

} Result_S;

}  // namespace Group2Detect_NS