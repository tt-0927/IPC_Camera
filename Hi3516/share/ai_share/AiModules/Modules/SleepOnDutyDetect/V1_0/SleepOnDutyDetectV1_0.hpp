/**
 * @file SleepOnDutyDetectV1_0.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 睡岗识别
 */
#pragma once

#include <unordered_map>

#include "SleepOnDutyDetectExt.hpp"
#include "YoloUltralytics.hpp"

#define SleepOnDutyDetect 0

namespace SleepOnDutyDetect_NS
{
    class CSleepOnDutyDetectV1_0
    {
    public:

        CSleepOnDutyDetectV1_0(InParam_S stInParam);
        ~CSleepOnDutyDetectV1_0();

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

        int m_nSleepOnDutyFrameCount = 0;       // 睡岗检测帧数

        /* 检测类别 */
        enum  DetectStatus  
        {
            Sleep = 0,                   // 垃圾满溢
        };
    };

}    // namespace SleepOnDutyDetect_NS