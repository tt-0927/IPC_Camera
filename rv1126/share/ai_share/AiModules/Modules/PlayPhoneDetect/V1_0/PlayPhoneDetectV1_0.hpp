/**
 * @file PlayPhoneDetectV1_0.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 玩手机识别
 */
#pragma once

#include <unordered_map>

#include "PlayPhoneDetectExt.hpp"
#include "YoloUltralytics.hpp"

#define PlayPhoneDetect 0

namespace PlayPhoneDetect_NS
{
    class CPlayPhoneDetectV1_0
    {
    public:

        CPlayPhoneDetectV1_0(InParam_S stInParam);
        ~CPlayPhoneDetectV1_0();

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

        int m_nPlayPhoneFrameCount = 0;       // 玩手机检测帧数

        /* 检测类别 */
        enum  DetectStatus  
        {
            PLAY_PHONE = 0,                   // 玩手机
        };
    };

}    // namespace PlayPhoneDetect_NS