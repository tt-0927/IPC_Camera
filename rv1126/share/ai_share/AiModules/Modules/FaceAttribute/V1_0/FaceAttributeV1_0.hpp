/**
 * @file FaceAttributeV1_0.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-20
 * 
 * @brief 人脸属性获取
 */
#pragma once

#include <unordered_map>
#include "Attribute.hpp"
#include "FaceAttributeExt.hpp"

namespace FaceAttribute_NS
{
    class CFaceAttributeV1_0
    {
    public:
        CFaceAttributeV1_0(InParam_S stInParam);
        ~CFaceAttributeV1_0();

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
         * @param vecResult 分析的参数
         * @return true
         * @return false
         */
        bool process(InData_S stInData, std::vector<Result_S> &vecResult);

    private:
        /**
         * @brief 等比例缩放图片
         * @param [cv::Mat] inputImage: 传入的图片数据
         * @param [cv::Mat&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage);

    private:
        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CAttribute *m_pFaceAttribute = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight = 0;
        int m_nLimitWidth = 0;
        int m_nLimitChannel = 0;

        /* 置信度阈值 */
        const float CONF_THRESHOLD = 0.5f;

        /* 胡子置信度阈值 */
        const float CONF_BEARD_THRESHOLD = 0.2f;
        /* 眼镜置信度阈值 */
        const float CONF_GLASS_THRESHOLD = 0.02f;
    };

} // namespace FaceAttribute_NS
