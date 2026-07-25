/**
 * @file ParkingDetectV1_0.hpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2025-03-05
 * 
 * @brief 停车检测
 */

#pragma once

#include <unordered_map>

#include "BYTETracker.h"
#include "ParkingDetectExt.hpp"
#ifdef PLATFORM_RK35XX
    #include "MotorizedDetect.hpp"
#else
    #include "YoloUltralytics.hpp"
#endif

namespace ParkingDetect_NS
{
    class CParkingDetectV1_0
    {
    public:

        CParkingDetectV1_0(InParam_S stInParam);
        ~CParkingDetectV1_0();

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
        bool process(InData_S stInData, std::vector<Result_S>& vecResult, OutData_S* stOutData = nullptr);


    private:

        /* 行人数据 */
        typedef struct _Vehicle_
        {
            int       nId;        /* 人的ID */
            cv::Point startPoint; /* 起始坐标点 */
            cv::Point curPoint;   /* 当前坐标点 */
            int       ndwellTime; /* 放弃跟踪时间 */
            int       nexistTime; /* 目标停留时间 */
            bool      isUsed;     /* 当前数据是否使用 */
        } Vehicle_S;

        /**
         * @brief 等比例缩放图片
         * @param [CVData_S] inputImage: 传入的图片数据
         * @param [char*&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage);

        /**
         * @brief 等比例缩放图片2
         * @param [CVData_S] inputImage: 传入的图片数据
         * @param [char*&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage2(cv::Mat inputImage, cv::Mat& outputImage);

        /**
         * @brief 判断两个多边形是否有交点
         * @param [std::vector<cv::Point>] rectPolygon: 目标框四个点坐标
         * @param [std::vector<cv::Point>] polygons: 多边形区域
         * @return [bool] true: 离开区域  false: 未离开
         * @note
         */
        bool isIntersecting(std::vector<cv::Point> rectPolygon,
                            std::vector<cv::Point> polygons);


    private:

        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CYoloUltralytics* m_pYoloUltralytics = nullptr;

        Inference_NS::cBYTETracker* m_pByteTracker = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight  = 640;
        int m_nLimitWidth   = 640;
        int m_nLimitChannel = 0;

        /* 缩放填充后左上角的坐标 */
        int   m_nXOffset     = 0;
        int   m_nYOffset     = 0;
        /* 缩放比例 */
        float m_fResizeScale = 1.0;

        /* 跟踪算法的参数 */
        /* [float] fTrackThresh[0-1,0.8]: 追踪阈值，这个值用于设置初始目标检测的置信度阈值。 */
        float m_fTrackThresh = 0.5;
        /* [float] fHighThresh[0-1]: 高置信度阈值，用于确定哪些检测结果非常可靠。 */
        float m_fHighThresh  = 0.4;
        /* [float] fMatchThresh[0-1]: 匹配阈值，在目标跟踪过程中，这个值用于决定两帧之间跟踪目标是否匹配。 */
        float m_fMatchThresh = 0.8;
        /* [int] nFrameId: 起始的ID */
        int   m_nFrameId     = 0;
        /* [int] nMaxTimeLost[>0]: 最大丢失时间，这个变量决定跟踪对象在连续几帧未能匹配到检测结果时，会被认为丢失。 */
        int   m_nMaxTimeLost = 20;

        // std::vector<Vehicle_S> m_vecVehicle;
        std::unordered_map<int, Vehicle_S> m_mapVehicle;
    };

}    // namespace ParkingDetect_NS
