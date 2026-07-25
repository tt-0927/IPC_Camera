/**
 * @file StreeDetectV1_0.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 道路积水井盖检测
 */
#pragma once

#include <unordered_map>

#include "StreeDetectExt.hpp"
#include "YoloUltralytics.hpp"

#define StreeDetect_DEBUG 1

namespace StreeDetect_NS
{
    class CStreeDetectV1_0
    {
    public:

        CStreeDetectV1_0(InParam_S stInParam);
        ~CStreeDetectV1_0();

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
        enum  ManholeCoverStatus  
        {
            INTACT = 0,                 // 井盖完好
            DAMAGED = 1,                // 井盖破损
            MISSING = 2,                // 井盖丢失
            UNCOVERED = 3,              // 未盖井盖
            EDGE_DAMAGED = 4,           // 井盖外边沿破损
            ROAD_FLOODED = 5            // 道路积水
        };
    };

}    // namespace FireDetect_NS