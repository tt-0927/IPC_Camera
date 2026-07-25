/**
 * @file ClipImageV2_0.hpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2026-06-09
 * 
 * @brief 图文检索 ONNX 版本
 */
#pragma once

#include <unordered_map>
#include "ImageFeature.hpp"
#include "ClipImageExt.hpp"

namespace ClipImage_NS
{
    class CClipImageV2_0
    {
    public:
        CClipImageV2_0(InParam_S stInParam);
        ~CClipImageV2_0();

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
         * @param vResult 分析的结果
         * @return true
         * @return false
         */
        bool process(InData_S stInData, std::vector<float> &vResult);

    private:
        /**
         * @brief 等比例缩放图片
         * @param [cv::Mat] inputImage: 传入的图片数据
         * @param [cv::Mat&] pchOutData: 输出的缩放后的图片
         * @return [*]
         * @note
         */
        bool resizeAndPadImage(cv::Mat inputImage, cv::Mat& outputImage);

        /**
         * @brief 图片预处理
         * @param [cv::Mat] aInput: 输入图片
         * @param [cv::Mat&] aOutput: 输出预处理后的图片
         * @param [int] nTargetWidth: 目标宽度
         * @param [int] nTargetHeight: 目标高度
         * @param [std::vector<float>] vMean: 均值
         * @param [std::vector<float>] vStd: 方差
         * @param [bool] bRgb: 是否转为RGB
         * @return [bool] 预处理是否成功
         */
        bool PreProcess(
            cv::Mat aInput,
            cv::Mat& aOutput,
            int nTargetWidth,
            int nTargetHeight,
            std::vector<float> vMean,
            std::vector<float> vStd,
            bool bRgb);

    private:
        /* 初始化参数 */
        InParam_S m_stInParam;

        Inference_NS::CImageFeature *m_pClipImage = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight = 0;
        int m_nLimitWidth = 0;
        int m_nLimitChannel = 0;
        std::vector<float> m_vMean;
        std::vector<float> m_vStd;
    };

} // namespace ClipImage_NS
