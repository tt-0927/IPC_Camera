/**
 * @file SafetyropeAndSoilDetectV1_0.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 高空安全带黄土裸露检测
 */
#pragma once

#include <unordered_map>

#include "SafetyropeAndSoilDetectExt.hpp"
#include "YoloUltralytics.hpp"

#define SafetyropeAndSoilDetect 0

namespace SafetyropeAndSoilDetect_NS
{
    class CSafetyropeAndSoilDetectV1_0
    {
    public:

        CSafetyropeAndSoilDetectV1_0(InParam_S stInParam);
        ~CSafetyropeAndSoilDetectV1_0();

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

        int m_nSafetyropeFrameCount = 0;        // 高空安全带检测帧数
        int m_nSoilFrameCount = 0;              // 黄土裸露检测帧数

        /* 检测类别 */
        enum  DetectStatus  
        {
            HIGH_ALTITUDE_SEATBELT = 0,                     // 高空安全带
            BARE_SOIL = 1,                                  // 黄土裸露
        };
    };

}    // namespace SafetyropeAndSoilDetect_NS