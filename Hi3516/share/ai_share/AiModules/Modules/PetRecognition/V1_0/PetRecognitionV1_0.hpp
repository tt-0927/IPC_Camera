/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-24 14:39:18
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-11-25 08:38:24
 * @FilePath: /1126/share/ai_share/AiModules/Modules/PetRecognition/V1_0/PetRecognitionV1_0.hpp
 * @Description: 宠物识别
 */

#pragma once

#include "PetRecognitionExt.hpp"
#include "YoloUltralytics.hpp"

#define PetRecognition 0

namespace PetRecognition_NS
{
    class CPetRecognitionV1_0
    {
    public:

        CPetRecognitionV1_0(InParam_S stInParam);
        ~CPetRecognitionV1_0();

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
        bool process(InData_S stInData, std::vector<Result_S>& vecResult);

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
    };

}    // namespace PetRecognition_NS