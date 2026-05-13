/**
 * @file RubishDetectV1_0.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 垃圾检测
 */
#pragma once

#include <unordered_map>

#include "RubishDetectExt.hpp"
#include "YoloUltralytics.hpp"

#define RubishDetect_DEBUG 0

namespace RubishDetect_NS
{
    class CRubishDetectV1_0
    {
    public:

        CRubishDetectV1_0(InParam_S stInParam);
        ~CRubishDetectV1_0();

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

        Inference_NS::CYoloUltralytics * m_pYoloUltralytics  = nullptr;

        int m_nManholeFrameCount = 0;       // 井盖连续检测帧数
        int m_nRoadFrameCount = 0;          // 道路连续检测帧数

        /* 检测类别 */
        enum  RubishStatus  
        {
            OVERFLOW = 0,                   // 垃圾满溢
            EXPOSURE = 1,                   // 垃圾暴露
        };
    };

}    // namespace FireDetect_NS