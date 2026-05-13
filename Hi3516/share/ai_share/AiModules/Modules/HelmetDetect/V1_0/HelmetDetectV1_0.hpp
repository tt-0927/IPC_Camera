/**
 * @file HelmetDetectV1_0.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 安全帽检测
 */
#pragma once

#include <unordered_map>

#include "HelmetDetectExt.hpp"
#include "Yolov5.hpp"

#define HelmetDetect 0

namespace HelmetDetect_NS
{
    class CHelmetDetectV1_0
    {
    public:

        CHelmetDetectV1_0(InParam_S stInParam);
        ~CHelmetDetectV1_0();

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
        
        int m_nHelmetFrameCount = 0;       // 安全帽检测帧数

        /* 检测类别 */
        enum  DetectStatus  
        {
            WEAR_HELMET = 0,                   // 戴安全帽
            NOT_WEAR_HELMET = 1,               // 没戴安全帽
        };
    };

}    // namespace HelmetDetect_NS