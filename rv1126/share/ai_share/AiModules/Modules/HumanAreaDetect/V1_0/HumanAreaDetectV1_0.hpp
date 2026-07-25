/*
 * @FilePath     : HumanAreaDetectV1_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-29 13:59:59
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-29 14:16:56
 * @Description  : 
 */

#pragma once

#include <unordered_map>

#include "BYTETracker.h"
#include "HumanAreaDetectExt.hpp"
#include "YoloUltralytics.hpp"

namespace HumanAreaDetect_NS
{
    class CHumanAreaDetectV1_0
    {
    public:

        CHumanAreaDetectV1_0(InParam_S stInParam);
        ~CHumanAreaDetectV1_0();

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
        typedef struct _Penson_
        {
            int       nId;        /* 人的ID */
            cv::Point startPoint; /* 起始坐标点 */
            cv::Point curPoint;   /* 当前坐标点 */
            int       ndwellTime; /* 放弃跟踪时间 */
            bool      isUsed;     /* 当前数据是否使用 */
        } Penson_S;

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
         * @brief 拌线检测：判断两条线段是否有交点
         * @param [cv::Point] startPoint: 行人的起始坐标点
         * @param [cv::Point] lastPoint: 行人的当前坐标点
         * @param [cv::Point] alertLineFirst: 警戒线第一个点坐标
         * @param [cv::Point] alertLineSecond: 警戒线第二个点坐标
         * @return [TripLineType_E] 类型
         * @note
         */
        TripLineType_E tripLineDetection(const cv::Point& startPoint,
                                         const cv::Point& lastPoint,
                                         const cv::Point& alertLineFirst,
                                         const cv::Point& alertLineSecond);

        /**
         * @brief 通过Bounding Box方法快速排除两条线段没有交点
         * @param [cv::Point] lineA1: 线段A的第1个端点
         * @param [cv::Point] lineA2: 线段A的第2个端点
         * @param [cv::Point] lineB1: 线段B的第1个端点
         * @param [cv::Point] lineB2: 线段B的第2个端点
         * @return [*]
         * @note
         */
        bool isBoundingBoxIntersecting(const cv::Point& lineA1,
                                       const cv::Point& lineA2,
                                       const cv::Point& lineB1,
                                       const cv::Point& lineB2);

        /**
         * @brief 计算叉积
         * @param [cv::Point] alertLineStart: 线段的第1个端点
         * @param [cv::Point] alertLineEnd: 线段的第2个端点
         * @param [cv::Point] testPoint: 待测的点
         * @return [*]
         * @note
         */
        int crossProduct(const cv::Point& alertLineStart,
                         const cv::Point& alertLineEnd,
                         const cv::Point& testPoint);

        /**
         * @brief 入侵检测：判断禁止入侵的区域有无点
         * @param [cv::Point] lastPoint: 待检测点
         * @param [std::vector<cv::Point>] polygons: 禁止闯入的多边形区域
         * @return [bool] true: 闯入区域  false: 未闯入
         * @note
         */
        bool intrusionZoneDetection(const cv::Point&       lastPoint,
                                    std::vector<cv::Point> polygons);

        /**
         * @brief 进入检测：根据起始点和当前点的关系，判断是否进入
         * @param [cv::Point] startPoint: 待检测的起始点
         * @param [cv::Point] lastPoint: 待检测的当前点
         * @param [std::vector<cv::Point>] polygons: 多边形区域
         * @return [bool] true: 进入区域  false: 未进入
         * @note
         */
        bool entryZoneDetection(const cv::Point&       startPoint,
                                const cv::Point&       lastPoint,
                                std::vector<cv::Point> polygons);

        /**
         * @brief 离开检测：根据起始点和当前点的关系，判断是否离开
         * @param [cv::Point] startPoint: 待检测的起始点
         * @param [cv::Point] lastPoint: 待检测的当前点
         * @param [std::vector<cv::Point>] polygons: 多边形区域
         * @return [bool] true: 离开区域  false: 未离开
         * @note
         */
        bool leaveZoneDetection(const cv::Point&       startPoint,
                                const cv::Point&       lastPoint,
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
        bool isLineIntersectingRect(const cv::Point& topLeft,
                                    const cv::Point& bottomRight,
                                    const cv::Point& alertLineFirst,
                                    const cv::Point& alertLineSecond);



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
        float m_fMatchThresh = 0.4;
        /* [int] nFrameId: 起始的ID */
        int   m_nFrameId     = 0;
        /* [int] nMaxTimeLost[>0]: 最大丢失时间，这个变量决定跟踪对象在连续几帧未能匹配到检测结果时，会被认为丢失。 */
        int   m_nMaxTimeLost = 30;

        // std::vector<Penson_S> m_vecPenson;
        std::unordered_map<int, Penson_S> m_mapPenson;
    };

}    // namespace HumanAreaDetect_NS
