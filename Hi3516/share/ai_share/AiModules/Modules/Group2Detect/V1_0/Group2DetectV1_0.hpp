/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-09 11:34:24
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-02-26 16:48:29
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group2Detect/V1_0/Group2DetectV1_0.hpp
 * @Description: 人、车、非相关事件检测
 */

#pragma once

#include <unordered_map>

#include "BYTETracker.h"
#include "Group2DetectExt.hpp"
#include "YoloUltralytics.hpp"

// #define Group2Detect_DEBUG 1
struct TripLineStatus{
    int lastStatus; //上次线段相交状态
    int lastlineType;//线段类型 0横线 1竖线
    int lastlinePlace;//上次位置
    int firstPlace;//首次位置
    int lastPlace;//最后位置
    int lostFrameCount;//丢失帧次数
    bool isUsing;//是否正在使用线段
    void init()
    {
        lastStatus = -1;
        lastlineType = -1;
        lastlinePlace = -2;
        lostFrameCount = 0;
        isUsing = true;
        firstPlace = 0;
        lastPlace = 0;
    }
};

namespace Group2Detect_NS {
class CGroup2DetectV1_0 {
  public:
    enum PMNMClass
    {
        PERSON,
        MOTOR_VEHICLE,
        NON_MOTOR_VEHICLE,
        UNKOWN
    };

  public:
    CGroup2DetectV1_0(InParam_S stInParam);
    ~CGroup2DetectV1_0();

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
        Loitering_S stLoitering; /* 徘徊侦测 */
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
     * @return -1: 左侧, 1: 右侧, 0: 在线段上或线段垂直
     */
    int pointLeftOrRightOfLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point);

    /**
     * @brief 判断点在线段的上方还是下方（适用于非水平的线段）
     * @param linePt1 线段起点
     * @param linePt2 线段终点
     * @param point 待判断的点
     * @return 1: 上方, -1: 下方, 0: 在线段上或线段水平
     */
    int pointAboveOrBelowLine(const cv::Point2f& linePt1, const cv::Point2f& linePt2, const cv::Point2f& point);
    
    /**
     * @brief 计算两条直线的夹角（锐角，0-90度）
     * @param linePt1 第一条直线的起点
     * @param linePt2 第一条直线的终点
     * @return 与垂直线的夹角（度）
     */
    double calculateAngleWithVertical(const cv::Point2f& linePt1, const cv::Point2f& linePt2);
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
    float m_fMatchThresh[3] = {0.8f, 0.7f, 0.8f}; /* 0-人 1-机动车 2-非机动车 */
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

    int m_nElectricScooterFrameCount = 0;  // 检测到电瓶车 连续帧数

    uint64_t m_llPersonFalldownTimestamp = 0;  // 人员倒地时间戳

    int64_t m_llLeavePostStartTime[4] = {0};

    int64_t m_llLibraryTime[2] = {0}; /* 1占用座位时间 2离开座位时间*/

    // personId status
    std::unordered_map<int, TripLineStatus> m_TripLineStatus;
    std::unordered_map<int, TripLineStatus> m_HeadCountStatus;
};

}  // namespace Group2Detect_NS
