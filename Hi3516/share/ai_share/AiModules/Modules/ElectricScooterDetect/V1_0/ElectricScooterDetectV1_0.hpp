/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-19 20:24:13
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-11-20 10:10:11
 * @FilePath: /1126/share/ai_share/AiModules/Modules/ElectricScooterDetect/V1_0/ElectricScooterDetect.hpp
 * @Description: 电瓶车检测
 */

#pragma once

#include <unordered_map>

#include "ElectricScooterDetectExt.hpp"
#include "YoloUltralytics.hpp"

#define ElectricScooterDetect 0

namespace ElectricScooterDetect_NS
{
    class CElectricScooterDetectV1_0
    {
    public:

        CElectricScooterDetectV1_0(InParam_S stInParam);
        ~CElectricScooterDetectV1_0();

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

        int m_nElectricScooterFrameCount = 0;       // 电瓶车检测帧数

        /* 检测类别 */
        enum  DetectStatus  
        {
            ElectricScooter = 0,                   // 电瓶车
        };
    };

}    // namespace ElectricScooterDetect_NS