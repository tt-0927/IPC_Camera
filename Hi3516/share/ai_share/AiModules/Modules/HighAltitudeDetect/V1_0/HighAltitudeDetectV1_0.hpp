/*
 * @FilePath     : HighAltitudeDetectV1_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-09-29 13:59:59
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-29 14:16:56
 * @Description  : 
 */

#pragma once

#include <unordered_map>

#include "HighAltitudeTracker.hpp"
#include "HighAltitudeDetectExt.hpp"

namespace HighAltitudeDetect_NS
{
    class CHighAltitudeDetectV1_0
    {
    public:

        CHighAltitudeDetectV1_0(InParam_S stInParam);
        ~CHighAltitudeDetectV1_0();

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
        bool process(InData_S stInData, std::vector<Result_S>& vecResult);



    private:

        /* 目标数据 */
        typedef struct _Target_
        {
            int       nId;         /* 目标ID */
            float       previousX; /* 上一帧中心X坐标 */
            float       previousY; /* 上一帧中心Y坐标 */
            float       previousW; /* 上一帧宽 */
            float       previousH; /* 上一帧高 */
            int       ndwellTime;  /* 放弃跟踪时间 */
            bool      isUsed;      /* 当前数据是否使用 */
        } Target_S;

        /**
         * @brief 等比例缩放图片
         * @param [CVData_S] inputImage: 传入的图片数据
         * @param [char*&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage);



    private:

        /* 初始化参数 */
        InParam_S m_stInParam;

        cv::Ptr<cv::BackgroundSubtractorKNN> m_pKNNDetector = nullptr;

        Inference_NS::cHighAltitudeTracker* m_HighAltitudeTracker = nullptr;

        /* 检测处理的参数 */
        int m_nHistory = 500;
        double m_fDist2Threshold = 400.0;
        bool m_bDetectShadows = false;
        int m_nMinArea = 30;
        cv::Mat m_pKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));

        /* 算法输入参数限制 */
        int m_nLimitHeight  = 0;
        int m_nLimitWidth   = 0;
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
        float m_fMatchThresh = 0.3;
        /* [int] nFrameId: 起始的ID */
        int   m_nFrameId     = 0;
        /* [int] nMaxTimeLost[>0]: 最大丢失时间，这个变量决定跟踪对象在连续几帧未能匹配到检测结果时，会被认为丢失。 */
        int   m_nMaxTimeLost = 30;

        std::unordered_map<int, Target_S> m_mapTarget;
    };

}    // namespace HighAltitudeDetect_NS
