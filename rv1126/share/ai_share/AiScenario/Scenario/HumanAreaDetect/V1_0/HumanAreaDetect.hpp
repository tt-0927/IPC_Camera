/*
 * @FilePath     : HumanAreaDetect.hpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:33:22
 * @Description  : 
 */
#pragma once

#include "CVInferenceBase.hpp"
#include "ScenarioBase.hpp"
#include "BYTETracker.h"

#include <unordered_map>

namespace Scenario_NS
{
    class CHumanAreaDetect : public CScenarioBase
    {
    public:

        CHumanAreaDetect(AiScenario_NS::InParam_S stInParam);
        ~CHumanAreaDetect();

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init() override;

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit() override;

        /**
         * @brief 处理数据
         * @param [CVData_S] stInData: 传入的视频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @return [*]
         * @note
         */
        bool process(AiScenario_NS::CVData_S stInData, char*& pchOutData, int& nDataSize) override;

        /**
         * @brief 处理数据
         * @param [CAData_S] stInData: 传入的音频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @return [*]
         * @note
         */
        bool process(AiScenario_NS::CAData_S stInData, char*& pchOutData, int& nDataSize) override;

        /**
         * @brief 释放处理结果
         * @param [char*&] pchOutData: 处理结果指针
         * @return [*]
         * @note
         */
        bool releaseData(char*& pchOutData) override;

        

    private:

        typedef struct _Penson_
        {
            int nId;
            cv::Point startPoint;
            int ndwellTime;
            bool isUsed;
        }Penson_S;

        bool convertToJson(std::vector<cSTrack> vPointsXY, char** pchOutData, int& nDataSize);

        /**
         * @brief 等比例缩放图片
         * @param [CVData_S] inputImage: 传入的图片数据
         * @param [char*&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage);

        /**
         * @brief 拌线检测：判断两条线段是否有交点
         * @param [cv::Point] lineA1: 线段A的第1个端点
         * @param [cv::Point] lineA2: 线段A的第2个端点
         * @param [cv::Point] lineB1: 线段B的第1个端点
         * @param [cv::Point] lineB2: 线段B的第2个端点
         * @return [bool] true: 有交点  false: 没有交点
         * @note
         */
        bool tripLineDetection(const cv::Point &lineA1, const cv::Point &lineA2,
                               const cv::Point &lineB1, const cv::Point &lineB2);

        /**
         * @brief 通过Bounding Box方法快速排除两条线段没有交点
         * @param [cv::Point] lineA1: 线段A的第1个端点
         * @param [cv::Point] lineA2: 线段A的第2个端点
         * @param [cv::Point] lineB1: 线段B的第1个端点
         * @param [cv::Point] lineB2: 线段B的第2个端点
         * @return [*]
         * @note
         */
        bool isBoundingBoxIntersecting(const cv::Point &lineA1, const cv::Point &lineA2,
                                       const cv::Point &lineB1, const cv::Point &lineB2);
        
        /**
         * @brief 判断一个点是否在两点组成的线段上
         * @param [cv::Point] line1: 线段的第1个端点
         * @param [cv::Point] line2: 线段的第2个端点
         * @param [cv::Point] testPoint: 待测的点
         * @return [*]
         * @note
         */
        bool isPointOnSegment(const cv::Point &line1,
                              const cv::Point &line2,
                              const cv::Point &testPoint);

        /**
         * @brief 入侵检测：判断禁止入侵的区域有无点
         * @param [cv::Point] LastPoint: 待检测点
         * @param [std::vector<cv::Point>] Polygons: 禁止闯入的多边形区域
         * @return [bool] true: 闯入区域  false: 未闯入
         * @note
         */
        bool intrusionZoneDetection(const cv::Point &LastPoint,
			                        std::vector<cv::Point> Polygons);

        /**
         * @brief 进入检测：根据起始点和当前点的关系，判断是否进入
         * @param [cv::Point] StartPoint: 待检测的起始点
         * @param [cv::Point] LastPoint: 待检测的当前点
         * @param [std::vector<cv::Point>] Polygons: 多边形区域
         * @return [bool] true: 进入区域  false: 未进入
         * @note
         */
        bool entryZoneDetection(const cv::Point &StartPoint,
			                    const cv::Point &LastPoint,
			                    std::vector<cv::Point> Polygons);
        
        /**
         * @brief 离开检测：根据起始点和当前点的关系，判断是否离开
         * @param [cv::Point] StartPoint: 待检测的起始点
         * @param [cv::Point] LastPoint: 待检测的当前点
         * @param [std::vector<cv::Point>] Polygons: 多边形区域
         * @return [bool] true: 离开区域  false: 未离开
         * @note
         */
        bool leaveZoneDetection(const cv::Point &StartPoint,
			                    const cv::Point &LastPoint,
			                    std::vector<cv::Point> Polygons);

    private:

        InferenceV1_0_NS::CCVInferenceBase* m_pInference = nullptr;

        cBYTETracker* ByteTracker = nullptr;

        /* yolo的阈值 */
        float m_fBoxThreshold = 0.25;
        float m_fNmsThreshold = 0.25;

        /* 算法输入参数限制 */
        int m_nLimitHeight  = 0;
        int m_nLimitWidth   = 0;
        int m_nLimitChannel = 0;

        /* 缩放填充后左上角的坐标 */
        int m_nXOffset = 0;
        int m_nYOffset = 0;
        /* 缩放比例 */
        float m_fResizeScale = 0.0;

        /* 跟踪算法的参数 */
        /* [float] fTrackThresh[0-1,0.8]: 追踪阈值，这个值用于设置初始目标检测的置信度阈值。 */
        float m_fTrackThresh = 0.5;
        /* [float] fHighThresh[0-1]: 高置信度阈值，用于确定哪些检测结果非常可靠。 */
        float m_fHighThresh = 0.25;
        /* [float] fMatchThresh[0-1]: 匹配阈值，在目标跟踪过程中，这个值用于决定两帧之间跟踪目标是否匹配。 */
        float m_fMatchThresh = 0.8;
        /* [int] nFrameId: 起始的ID */
        int m_nFrameId = 0;
        /* [int] nMaxTimeLost[>0]: 最大丢失时间，这个变量决定跟踪对象在连续几帧未能匹配到检测结果时，会被认为丢失。 */
        int m_nMaxTimeLost = 30;

        // std::vector<Penson_S> m_vecPenson;
        std::unordered_map<int, Penson_S> m_mapPenson;
    };

}    // namespace Scenario_NS
