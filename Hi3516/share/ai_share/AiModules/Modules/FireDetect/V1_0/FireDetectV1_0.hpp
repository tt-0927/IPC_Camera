/**
 * @file FireDetectV1_0.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 火焰烟雾检测
 */
#pragma once

#include <unordered_map>

#include "FireDetectExt.hpp"
#include "YoloUltralytics.hpp"

#define FireDetect_DEBUG 1

namespace FireDetect_NS
{
    class CFireDetectV1_0
    {
    public:

        CFireDetectV1_0(InParam_S stInParam);
        ~CFireDetectV1_0();

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

        int m_nFireFrameCount = 0;     // 火焰连续检测帧数
        int m_nSmokeFrameCount = 0;    // 烟雾连续检测帧数

        /* 检测类别 */
        enum FireClass { SMOKE, FLAME, UNKOWN };
    };

}    // namespace FireDetect_NS