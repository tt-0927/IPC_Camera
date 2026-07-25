/**
 * @file FaceQualityAssessmentV1_0.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-09
 *
 * @brief
 */
#pragma once

#include <unordered_map>
#include "ImageFeature.hpp"
#include "FaceQualityAssessmentExt.hpp"

namespace FaceQualityAssessment_NS
{
    class CFaceQualityAssessmentV1_0
    {
    public:
        CFaceQualityAssessmentV1_0(InParam_S stInParam);
        ~CFaceQualityAssessmentV1_0();

        /**
         * @brief 初始化
         * @return true
         * @return false
         */
        bool init();

        /**
         * @brief 反初始化
         * @return true
         * @return false
         */
        bool unInit();

        /**
         * @brief 处理数据
         * @param stInData 传入的视频数据
         * @param fResult 人脸质量分数
         * @return true 
         * @return false 
         */
        bool process(InData_S stInData, float &fResult);

    private:
        /**
         * @brief 等比例缩放图片
         * @param [CVData_S] inputImage: 传入的图片数据
         * @param [char*&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);

    private:
        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CImageFeature *m_pImageFeature = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight = 112;
        int m_nLimitWidth = 112;
        int m_nLimitChannel = 0;
    };

} // namespace FaceQualityAssessment_NS
