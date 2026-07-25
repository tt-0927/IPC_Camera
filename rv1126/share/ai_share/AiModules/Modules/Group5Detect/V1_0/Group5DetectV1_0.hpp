/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-12-24 16:23:34
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-20 11:28:29
 * @FilePath: /1126/share/ai_share/AiModules/Modules/Group5Detect/V1_0/Group5DetectV1_0.hpp
 * @Description: metalFence(金属栅栏)、ConeTank(锥形桶)、CrashBarrels(防撞桶)、fence(防护栏)
 */

#pragma once

#include <unordered_map>

#include "Group5DetectExt.hpp"
#include "YoloUltralytics.hpp"

#define Group5Detect 0

namespace Group5Detect_NS
{
    class CGroup5DetectV1_0
    {
    public:


        /* 检测类别 */
        enum FireClass
        { 
            /* 金属栅栏 */
            METALFENCE, 
            /* 锥形桶 */
            CONETANK,
            /* 防撞桶 */
            CRASHBARRELS,
            /* 防护栏 */
            FENCE, 
            /* 未知类型 */
            UNKOWN 
        };

        CGroup5DetectV1_0(InParam_S stInParam);
        ~CGroup5DetectV1_0();

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

        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);
    private:

        /* 算法输入参数限制 */
        int m_nLimitWidth   = 640;
        int m_nLimitHeight  = 384;

        /* 缩放填充后左上角的坐标 */
        int m_nXOffset = 0;
        int m_nYOffset = 0;
        /* 缩放比例 */
        float m_fResizeScale = 1.0;

        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CYoloUltralytics * m_pYoloUltralytics  = nullptr;

        int m_nHoleProtectionBarDetectFrameCount = 0;        //防护栏连续检测帧数
        int m_nConstructionEncroachmentRoadFrameCount = 0;   // 施工占道连续检测帧数
    };

}    // namespace Group5Detect_NS