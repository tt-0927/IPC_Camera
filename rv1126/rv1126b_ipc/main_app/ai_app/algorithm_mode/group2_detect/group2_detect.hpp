/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-09 11:34:24
 * @LastEditors: xushou
 * @LastEditTime: 2026-11-03 11:07:00
 * @FilePath: rv1126b_ipc/main_app/ai_app/algorithm_mode/group2_detect/group2_detect.hpp
 * @Description: 人群聚集检测算法组件（从Group2DetectV1_0迁移）
 */

#pragma once

#include <unordered_map>

#include "BYTETracker.h"
#include "YoloUltralytics.hpp"

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
    int                    nNonMotorVehicleIntrusionTimeThreshold = 2;     /* 触发非机动车闯入事件时长阈值 */
    std::vector<int>       veDetectionTargetTypes;                     /* 检测目标类型 */
    std::vector<cv::Point> vecPoints;                                  /* 警戒多边形参数 */
} NonMotorVehicleIntrusionParam_S;

/* 非机动车车道检测参数 */
typedef struct _NonVehicleLaneDetParam_
{
    bool                   bEnable                            = false; /* 是否分析 */
    float                  fThreshold = 0.5;                           /* 非机动车的置信度阈值 */
    int                    nFrameRateThreshold = 2;                    /* 触发非机动车车道事件帧数阈值 */
    std::vector<int>       veDetectionTargetTypes;                     /* 检测目标类型 */
    std::vector<cv::Point> vecPoints;                                  /* 警戒多边形参数 */
} NonVehicleLaneDetParam_S;


/*电瓶车识别参数 */
typedef struct _ElectricScooterParam_
{
    bool         bEnable      = false; /* 是否分析 */
    float        fConfidence  = 0.5;   /* 置信度 */
    unsigned int nDetectFrame = 1;     /* 检测多少帧才触发 */
    unsigned int nTimeThreshold = 0;   /* 时间阈值触发 */
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
    std::vector<TripLineParam_S> vstTripLineParam;
    std::vector<IntrusionParam_S> vstIntrusionParam;
    std::vector<LeaveParam_S> vstLeaveParam;
    std::vector<EntryParam_S> vstEntryParam;
    std::vector<EmergencyLaneOccupancyParam_S> vstEmergencyLaneOccupancyParam;
    /* ============================ 人车非检测相关 ============================ */

    /* ============================ 人检测相关 ============================ */
    std::vector<LoiteringParam_S> vsLoiteringParam;
    std::vector<FenceClimbingParam_S> vstFenceClimbingParam;
    std::vector<LeavePostParam_S> vstLeavePostParam;
    std::vector<PedestrianIntrusionParam_S> vstPedestrianIntrusionParam;
    std::vector<CrowdGatheringDetParam_S> vstCrowdGatheringDetParam;
    std::vector<StairwellDetParam_S> vstStairwellDetParam;
    PersonFallDownParam_S stPersonFallDownParam;
    std::vector<LibraryVacanciesParam_S> stLibraryVacanciesDetectParam;
    /* ============================ 人检测相关 ============================ */

    /* ============================ 机动车检测相关 ============================ */
    std::vector<DrivingAgainstTrafficParam_S> vstDrivingAgainstTrafficParam;
    std::vector<IllegalLaneChangeParam_S> vstIllegalLaneChangeParam;
    std::vector<ParkingParam_S> vstParkingParam;
    CongestionParam_S stCongestionParam;
    /* ============================ 机动车检测相关 ============================ */

    /* ============================ 非机动车检测相关 ============================ */
    std::vector<NonMotorVehicleIntrusionParam_S> vstNonMotorVehicleIntrusionParam;
    std::vector<NonVehicleLaneDetParam_S> vstNonVehicleLaneDetParam;
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
    std::string savedFileName; /* 分析后图片路径 */

    std::vector<int> nCountType;    /* 人流统计越界类型 */
    bool bLibraryVacanciesType = false;/* 图书馆空位类型 */
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

using namespace Group2Detect_NS;

// #define Group2Detect_DEBUG 1
struct TripLineStatus{
    int lastStatus; //上次线段相交状态
    int lastlineType;//线段类型 0横线 1竖线
    int lastlinePlace;//上次位置
    int firstPlace;//首次位置
    int lastPlace;//最后位置
    int lostFrameCount;//丢失帧次数
    int nCooldownFrames;// 穿越触发后冷却帧数，防止边界振荡重复计数（可调，默认5帧）
    bool isUsing;//是否正在使用线段
    bool isFirst;//是否是第一次出现在画面里
    void init()
    {
        lastStatus = -1;
        lastlineType = -1;
        lastlinePlace = -2;
        lostFrameCount = 0;
        nCooldownFrames = 0;
        isUsing = true;
        firstPlace = 0;
        lastPlace = 0;
        isFirst = true;
    }
};

struct FenceClimbStatus{
    int nCrossStage;     // 穿越阶段: 0=未穿越任何线, 1=已穿越第一条线, -1=超时重置
    int nLine1First;     // 第一条线穿越前位置
    int nLine2First;     // 第二条线穿越前位置
    int nLine1Last;      // 第一条线当前位置
    int nLine2Last;      // 第二条线当前位置
    int lostFrameCount;  // 丢失帧次数
    bool isUsing;       // 是否正在使用当前对象
    void init()
    {
        nCrossStage = 0;
        nLine1First = 0;
        nLine2First = 0;
        nLine1Last = 0;
        nLine2Last = 0;
        lostFrameCount = 0;
        isUsing = true;
    }
};

/** 楼道拥挤相关参数
 *  每个区域(m_CrowdStatus[i])独立维护两种触发条件的状态
 */
struct CrowdStatus{
    // ===== 条件1：人员静止不动检测 =====
    // 人员在区域内保持静止(IOU>阈值)超过设定时间则触发
    // 各map的key均为personId，实现每个目标独立的静止追踪
    std::unordered_map<int, time_t>   mapStaticStartTime;   // 静止开始时间戳
    std::unordered_map<int, bool>     mapStaticTracking;     // 是否正在静止计时
    std::unordered_map<int, cv::Rect> mapLastPersonBox;      // 上一帧检测框(用于IOU计算)
    std::unordered_map<int, bool>     mapSeenCurFrame;       // 本帧出现在区域内的人员ID(用于清理离区人员状态)
    double  fStaticIouThreshold = 0.9;                      // 静止IOU阈值(可调,越大要求越静止)
    int     nStaticTimeThreshold = 5;                        // 静止触发时间阈值/秒(可调)

    // ===== 条件2：新增人员数量检测 =====
    // 时间窗口内新增人员ID数量超过阈值则触发
    time_t  nWindowStartTime = 0;                             // 计数窗口起始时间
    int     nNewPersonThreshold = 3;                          // 新增人员数量阈值(可调)
    int     nWindowDuration = 10;                             // 计数窗口时长/秒(可调)
    std::unordered_map<int, bool> mapCountedPerson;           // 窗口内已计数的人员ID(去重用)

    bool isTrigger = false;                                   // 是否已触发报警
};

/**
 * @brief 人群聚集检测算法组件
 * @note 从Group2DetectV1_0迁移，保留完整功能
 */
class CGroup2Detect {
  public:
    enum PMNMClass
    {
        PERSON,
        MOTOR_VEHICLE,
        NON_MOTOR_VEHICLE,
        UNKOWN
    };

  public:
    CGroup2Detect(InParam_S stInParam);
    ~CGroup2Detect();

    /**
     * @brief 初始化
     * @return [*]
     * @note
     */
    bool init();

    /**
     * @brief 反初始化
     * @return [*]
     * @note
     */
    bool unInit();

    /**
     * @brief 处理数据
     * @param [cv::Mat] inMat: 传入的视频数据
     * @param [AnalyseParam_S] stParam: 分析的参数
     * @param [std::vector<Result_S>&] vecResult: 输出的处理结果
     * @return [*]
     * @note
     */
    bool process(InData_S stInData, std::vector<Result_S> &vecResult, std::vector<Result_S> &vecAllResult, OutData_S *stOutData = nullptr,std::vector<Result_S>* vecResultOne = nullptr);
    // bool process(InData_S stInData, std::vector<Result_S> &vecResult, OutData_S *stOutData = nullptr);

    /**
     * @brief 重置非机动车闯入
     * @return [*]
     * @note
     */
    void resetNonMotorVehicleIntrusionStatus();

    /**
     * @brief 重置行人闯入
     * @return [*]
     * @note
     */
    void resetPedestrianIntrusionStatus();

  private:
    /* 应急车道触发参数 */
    typedef struct _EmergencyLaneOccupancy_
    {
        bool    bEmergencyLaneOccupancy          = false; /* 是否开启徘徊计时 */
        int64_t nEmergencyLaneOccupancyTimeStamp = 0;     /* 开始进入应急车道时间戳 */
    } EmergencyLaneOccupancy_S;

    /* 非机动车闯入触发参数 */
    typedef struct _NonMotorVehicleIntrusion_
    {
        bool bNonMotorVehicleIntrusion = false; /* 是否开启非机动车闯入计时 */
        // int                     nAreaCode = -1;                            /* 区域编号 */
        int64_t nNonMotorVehicleIntrusionTimeStamp = 0; /* 开始进入非机动车闯入区域时间戳 */
    } NonMotorVehicleIntrusion_S;

    /* 非机动车车道触发参数 */
    typedef struct _NonMotorVehicleLine_
    {
        int64_t nNonMotorVehicleLineFrameCount = 0; /* 开始进入非机动车车道区域连续帧数,默认1帧 */
    } NonMotorVehicleLine_S;

    /* 翻越围栏触发参数 */
    typedef struct _FenceClimbing_
    {
        unsigned int nFrameCount = 0; /* 连续检测帧数,默认1帧 */
    } FenceClimbing_S;

    /* 离开区域触发参数参数 */
    typedef struct _Leave_
    {
        bool    bEnLeave        = false; /* 是否开启离开区域计时 */
        int64_t nLeaveTimeStamp = 0;     /* 离开区域时间戳 */
    } Leave_S;

    /* 进入区域触发参数参数 */
    typedef struct _Entry_
    {
        bool    bEntry          = false; /* 是否开启进入区域计时 */
        int64_t nEntryTimeStamp = 0;     /* 进入区域时间戳 */
    } Entry_S;

    /* 徘徊侦测触发参数参数 */
    typedef struct _Loitering_
    {
        bool    bLoiter          = false; /* 是否开启徘徊计时 */
        int64_t nLoiterTimeStamp = 0;     /* 开始徘徊时间戳 */
    } Loitering_S;

    /* 停车触发参数 */
    typedef struct _Parking_
    {
        bool    bParking          = false; /* 是否开启违停计时 */
        int64_t nParkingTimeStamp = 0;     /* 开始进入违停区域时间戳 */
    } Parking_S;

    /* 区域入侵触发参数 */
    typedef struct _Intrusion_
    {
        bool    bIntrusion          = false; /* 是否开启区域入侵计时 */
        int64_t nIntrusionTimeStamp = 0;     /* 开始区域入侵时间戳 */
    } Intrusion_S;

    /* 目标数据 */
    typedef struct _Target_
    {
        int       nId;                 /* 目标的ID */
        cv::Point startPoint;          /* 起始坐标点 */
        cv::Point curPoint;            /* 当前坐标点 */
        cv::Point bottomMidPoint;      /* 底边中点 */
        float     fAspectRatio = 0.0f; /* 长宽比 */
        int       ndwellTime;          /* 放弃跟踪时间 */
        bool      isUsed;              /* 当前数据是否使用 */

        EmergencyLaneOccupancy_S stEmergencyLaneOccupancy; /* 应急车道触发参数 */
        /* 行人检测保存参数 */
        FenceClimbing_S stFenceClimbing; /* 翻越围栏检测 */
        Leave_S         stLeavePost;     /* 离岗检测 */
        // Entry_S         stPedestrianIntrusion; /* 行人闯入检测 */
        std::vector<Loitering_S> vstLoitering; /* 徘徊侦测（每个区域独立状态，索引与vsLoiteringParam对应，避免多区域间互相干扰） */
        std::vector<Entry_S> vstPerEntrying;
        Intrusion_S stIntrusion; /* 区域入侵 */
        /* 车辆检测保存参数 */
        Parking_S stParking;
    } Target_S;

    int regularProcess(InData_S &stInData, const std::array<std::vector<DetectResult_S>, 3> &vecBoxs, OutData_S *pstOutData);

    /**
     * @brief 等比例缩放图片
     * @param [CVData_S] inputImage: 传入的图片数据
     * @param [char*&] pchOutData: 输出的缩放后的图片
     * @return [*]
     * @note
     */
    bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);

    /**
     * @brief 等比例缩放图片2
     * @param [CVData_S] inputImage: 传入的图片数据
     * @param [char*&] pchOutData: 输出的缩放后的图片
     * @return [*]
     * @note
     */
    bool resizeAndPadImage2(cv::Mat inputImage, cv::Mat &outputImage);

    /**
     * @brief 等比例缩放图片2
     * @param [CVData_S] inputImage: 传入的图片数据
     * @param [char*&] pchOutData: 输出的缩放后的图片
     * @return [*]
     * @note
     */
    bool resizeAndPadImage3(cv::Mat inputImage, cv::Mat &outputImage);

    /**
     * @brief 拌线检测：判断两条线段是否有交点
     * @param [cv::Point] startPoint: 行人的起始坐标点
     * @param [cv::Point] lastPoint: 行人的当前坐标点
     * @param [cv::Point] alertLineFirst: 警戒线第一个点坐标
     * @param [cv::Point] alertLineSecond: 警戒线第二个点坐标
     * @param [bool] bBidirectional: 是否支持双向判断
     * @return [TripLineType_E] 类型
     * @note
     */
    TripLineType_E tripLineDetection(const cv::Point &startPoint,
                                     const cv::Point &lastPoint,
                                     const cv::Point &alertLineFirst,
                                     const cv::Point &alertLineSecond,
                                     bool             bBidirectional

    );

    /**
     * @brief 通过Bounding Box方法快速排除两条线段没有交点
     * @param [cv::Point] lineA1: 线段A的第1个端点
     * @param [cv::Point] lineA2: 线段A的第2个端点
     * @param [cv::Point] lineB1: 线段B的第1个端点
     * @param [cv::Point] lineB2: 线段B的第2个端点
     * @return [*]
     * @note
     */
    bool isBoundingBoxIntersecting(const cv::Point &lineA1,
                                   const cv::Point &lineA2,
                                   const cv::Point &lineB1,
                                   const cv::Point &lineB2);

    /**
     * @brief 计算叉积
     * @param [cv::Point] alertLineStart: 线段的第1个端点
     * @param [cv::Point] alertLineEnd: 线段的第2个端点
     * @param [cv::Point] testPoint: 待测的点
     * @return [*]
     * @note
     */
    int crossProduct(const cv::Point &alertLineStart,
                     const cv::Point &alertLineEnd,
                     const cv::Point &testPoint);

    /**
     * @brief 入侵检测：判断禁止入侵的区域有无点
     * @param [cv::Point] lastPoint: 待检测点
     * @param [std::vector<cv::Point>] polygons: 禁止闯入的多边形区域
     * @return [bool] true: 闯入区域  false: 未闯入
     * @note
     */
    bool intrusionZoneDetection(const cv::Point       &lastPoint,
                                std::vector<cv::Point> polygons);

    /**
     * @brief 进入检测：根据起始点和当前点的关系，判断是否进入
     * @param [cv::Point] startPoint: 待检测的起始点
     * @param [cv::Point] lastPoint: 待检测的当前点
     * @param [std::vector<cv::Point>] polygons: 多边形区域
     * @return [bool] true: 进入区域  false: 未进入
     * @note
     */
    bool entryZoneDetection(const cv::Point       &startPoint,
                            const cv::Point       &lastPoint,
                            std::vector<cv::Point> polygons);

    /**
     * @brief 离开检测：根据起始点和当前点的关系，判断是否离开
     * @param [cv::Point] startPoint: 待检测的起始点
     * @param [cv::Point] lastPoint: 待检测的当前点
     * @param [std::vector<cv::Point>] polygons: 多边形区域
     * @return [bool] true: 离开区域  false: 未离开
     * @note
     */
    bool leaveZoneDetection(const cv::Point       &startPoint,
                            const cv::Point       &lastPoint,
                            std::vector<cv::Point> polygons);

    /**
     * @brief 判断两个多边形是否有交点
     * @param [std::vector<cv::Point>] rectPolygon: 目标框四个点坐标
     * @param [std::vector<cv::Point>] polygons: 多边形区域
     * @return [bool] true: 离开区域  false: 未离开
     * @note
     */
    bool isIntersecting(std::vector<cv::Point> rectPolygon,
                        std::vector<cv::Point> polygons);

    /**
     * @brief 判断目标框与线段是否有交点
     * @param [cv::Point] topLeft: 目标框左上角点
     * @param [cv::Point] bottomRight: 目标框右下角点
     * @param [cv::Point] alertLineFirst: 警戒线第一个点坐标
     * @param [cv::Point] alertLineSecond: 警戒线第二个点坐标
     * @return [TripLineType_E] 类型
     * @note
     */
    bool isLineIntersectingRect(const cv::Point &topLeft,
                                const cv::Point &bottomRight,
                                const cv::Point &alertLineFirst,
                                const cv::Point &alertLineSecond);

    /**
     * @brief   : 判断是否跨越线段
     * @param    {PosF_S} &p1 线段A的第1个端点
     * @param    {PosF_S} &q1 线段A的第2个端点
     * @param    {PosF_S} &p2 线段B的第1个端点
     * @param    {PosF_S} &q2 线段B的第2个端点
     * @return   {*} true:跨越 false:未跨越
     */
    bool doLinesIntersect(const cv::Point &p1, const cv::Point &q1, const cv::Point &p2, const cv::Point &q2);

    int64_t getSteadyTimeStampMs()
    {
        auto now      = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return duration.count();
    }
    /**
     * @brief 判断记录的时间与当前时间的间隔是否大于指定秒数
     * @param nRecordTime  已记录的时间（毫秒级，通过cv::getTickCount()计算得到）
     * @param nThresholdSec  阈值（秒）
     * @return true：间隔大于阈值；false：间隔小于等于阈值
     */
    bool isTimeIntervalExceeded(int64_t nRecordTime, int nThresholdSec);


    TripLineType_E tripLineDetection(Result_S& stResult,const cv::Point &alertLineFirst,const cv::Point &alertLineSecond);

    /**
     * @brief 判断点在线段的左侧还是右侧（适用于非垂直的线段）
     * @param linePt1 线段起点
     * @param linePt2 线段终点
     * @param point 待判断的点
     * @param bFlag 是否计算投影有效性
     * @return -1: 左侧, 1: 右侧, 0: 在线段上或线段垂直
     */
    int pointLeftOrRightOfLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point,bool bFlag = true);

    /**
     * @brief 判断点在线段的上方还是下方（适用于非水平的线段）
     * @param linePt1 线段起点
     * @param linePt2 线段终点
     * @param point 待判断的点
     * @return 1: 上方, -1: 下方, 0: 在线段上或线段水平
     */
    int pointAboveOrBelowLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point,bool bFlag = true);
    

    int pointSideOfLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point,bool bFlag = true);
    /**
     * @brief 计算两条直线的夹角（锐角，0-90度）
     * @param linePt1 第一条直线的起点
     * @param linePt2 第一条直线的终点
     * @return 与垂直线的夹角（度）
     */
    double calculateAngleWithVertical(const cv::Point2f& linePt1, const cv::Point2f& linePt2);

    void DrawPoint(cv::Mat& mat,cv::Point point,cv::Scalar sc = cv::Scalar(0,255,0), bool isWriteFile = false);
    void DrawLine(cv::Mat& mat,cv::Point point1, cv::Point point2,cv::Scalar sc = cv::Scalar(0,255,0), bool isWriteFile = false);
    void DrawArea(cv::Mat& mat,cv::Point point1, cv::Point point2,cv::Scalar sc = cv::Scalar(0,255,0), bool isWriteFile = false);
    void DrawArea(cv::Mat& mat,std::vector<cv::Point> points,cv::Scalar sc = cv::Scalar(0,255,0), bool isWriteFile = false);
    void DrawText(cv::Mat& mat,std::string text,cv::Point point,cv::Scalar sc = cv::Scalar(0,255,0), bool isWriteFile = false);
  private:
    /* 初始化参数 */
    InParam_S m_stInParam;

    Inference_NS::CYoloUltralytics *m_pYoloUltralytics = nullptr;

    /* 0-人 1-机动车 2-非机动车 */
    Inference_NS::cBYTETracker *m_pByteTracker[3] = {nullptr, nullptr, nullptr};

    // Inference_NS::cBYTETracker* m_pPersonByteTracker = nullptr;         /* 跟踪人 */
    // Inference_NS::cBYTETracker* m_pMotorVehicleByteTracker = nullptr;   /* 跟踪机动车 */
    // Inference_NS::cBYTETracker* m_pNonMotorVehicleByteTracker = nullptr;/* 跟踪非机动车 */

    /* 算法输入参数限制 */
    int m_nLimitWidth   = 640;
    int m_nLimitHeight  = 384;
    int m_nLimitChannel = 3;

    /* 缩放填充后左上角的坐标 */
    int m_nXOffset = 0;
    int m_nYOffset = 0;
    /* 缩放比例 */
    float m_fResizeScale = 1.0;

    /* 跟踪算法的参数 */
    /* [float] fTrackThresh[0-1,0.8]: 追踪阈值，这个值用于设置初始目标检测的置信度阈值。 */
    float m_fTrackThresh[3] = {0.5f, 0.6f, 0.5f}; /* 0-人 1-机动车 2-非机动车 */
    /* [float] fHighThresh[0-1]: 高置信度阈值，用于确定哪些检测结果非常可靠。 */
    float m_fHighThresh[3] = {0.6f, 0.5f, 0.4f}; /* 0-人 1-机动车 2-非机动车 */
    /* [float] fMatchThresh[0-1]: 匹配阈值，在目标跟踪过程中，这个值用于决定两帧之间跟踪目标是否匹配。 */
    float m_fMatchThresh[3] = {0.5f, 0.7f, 0.8f}; /* 0-人 1-机动车 2-非机动车 */
    /* [int] nFrameId: 起始的ID */
    int m_nFrameId[3] = {0, 0, 0};  // 每类独立帧计数, /* 0-人 1-机动车 2-非机动车 */
    /* [int] nMaxTimeLost[>0]: 最大丢失时间，这个变量决定跟踪对象在连续几帧未能匹配到检测结果时，会被认为丢失。 */
    int m_nMaxTimeLost = 30;

    // std::vector<Penson_S> m_vecPenson;
    std::unordered_map<int, Target_S> m_mapPenson;
    std::unordered_map<int, Target_S> m_mapVehicle;
    std::unordered_map<int, Target_S> m_mapNonMotorVehicle;

    NonMotorVehicleIntrusion_S m_stNonMotorVehicleIntrusion[4]; /* 非机动车闯入最多设置检测四个区域 */
    Entry_S                    m_stPedestrianIntrusion[4];      /* 行人闯入最多设置检测四个区域 */
    NonMotorVehicleLine_S      m_stNonMotorVehicleLane[4]; /* 非机动车车道检测 */

    int m_nElectricScooterFrameCount = 0;  // 检测到电瓶车 连续帧数
    long long m_nElectricScooterTime = 0;  // 检测到电瓶车 连续帧数

    uint64_t m_llPersonFalldownTimestamp = 0;  // 人员倒地时间戳

    int64_t m_llLeavePostStartTime[4] = {0};

    std::vector<int64_t> m_llLibraryTime; /* 1占用座位时间 2离开座位时间*/

    CrowdStatus m_CrowdStatus[4];

    long long nCrowdGatheringDetTime[4] = {0};  // 人员聚集阈值
    long long nStairwellDetTime[4] = {0};  // 楼道拥挤阈值
    // personId status
    std::unordered_map<int, TripLineStatus> m_TripLineStatus;
    std::unordered_map<int, TripLineStatus> m_HeadCountStatus;
    std::unordered_map<int, FenceClimbStatus> m_FenceClimbStatus;
};
