/**
 * @file TripDetectV1_0.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 摔倒识别
 */
#pragma once

#include <unordered_map>

#include "TripDetectExt.hpp"
#include "Yolov5.hpp"

#define TripDetectDetect 0

namespace TripDetect_NS
{
    class CTripDetectV1_0
    {
    public:

        CTripDetectV1_0(InParam_S stInParam);
        ~CTripDetectV1_0();

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

        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CYolov5 * m_pYolov5  = nullptr;
        
        int m_nTripFrameCount = 0;       // 睡岗检测帧数

        /* 检测类别 */
        enum  DetectStatus  
        {
            TRIP = 0,                   //摔倒
        };
    };

}    // namespace TripDetect_NS