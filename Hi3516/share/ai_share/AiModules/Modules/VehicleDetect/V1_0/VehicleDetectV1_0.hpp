/**
 * @file VehicleDetectV1_0.cpp
 * @author songww
 * @date 2025-10-29
 * 
 * @brief 车辆检测
 */

#pragma once

#include <unordered_map>

#include "BYTETracker.h"
#include "VehicleDetectExt.hpp"
#ifdef PLATFORM_RK35XX
    #include "MotorizedDetect.hpp"
#else
    #include "YoloUltralytics.hpp"
#endif

namespace VehicleDetect_NS
{
    class CVehicleDetectV1_0
    {
    public:
        /**
         * @brief 构造函数
         * @param stInParam1    车牌检测参数
         * @param stInParam2    车辆检测参数
         */
        CVehicleDetectV1_0(InParam_S stInParam1,InParam_S stInParam2);
        ~CVehicleDetectV1_0();

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
        bool processPlate(InData_S stInData, std::vector<Result_S>& vecResult);

        /**
         * @brief 处理数据
         * @param [cv::Mat] inMat: 传入的视频数据
         * @param [AnalyseParam_S] stParam: 分析的参数
         * @param [std::vector<Result_S>&] vecResult: 输出的处理结果
         * @return [*]
         * @note
         */
        bool processVehicle(InData_S stInData, std::vector<Result_S>& vecResult);



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
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);
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
        InParam_S m_stInParamPlate;
        InParam_S m_stInParamVehicle;

        Inference_NS::CYoloUltralytics* m_pYoloUltralytics_plate = nullptr;
        Inference_NS::CYoloUltralytics* m_pYoloUltralytics_vehicle = nullptr;

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

}    // namespace VehicleDetect_NS
