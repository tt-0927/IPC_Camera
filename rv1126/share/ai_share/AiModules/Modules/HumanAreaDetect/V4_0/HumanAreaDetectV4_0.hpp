/**
 * @file HumanAreaDetectV4_0.hpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2026-06-09
 * 
 * @brief 区域检测 ONNX 简化版，只有检测逻辑
 */
#pragma once

#include <vector>
#include "YoloUltralytics.hpp"
#include "HumanAreaDetectExt.hpp"

namespace HumanAreaDetect_NS
{
    class CHumanAreaDetectV4_0
    {
    public:
        CHumanAreaDetectV4_0(InParam_S stInParam);
        ~CHumanAreaDetectV4_0();

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
         * @brief 处理数据 - 返回检测到的目标框
         * @param stInData 传入的视频数据
         * @param vecResult 检测到的目标结果
         * @return true
         * @return false
         */
        bool process(InData_S stInData, std::vector<Result_S>& vecResult);

    private:
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

    Inference_NS::CYoloUltralytics* m_pYoloUltralytics = nullptr;

    /* 算法输入参数限制 */
    int m_nLimitHeight = 0;
    int m_nLimitWidth = 0;
    int m_nLimitChannel = 0;
    std::vector<float> m_vMean;
    std::vector<float> m_vStd;

    /* 缩放和填充相关的变量 */
    float m_fResizeScale = 1.0f;
    int m_nXOffset = 0;
    int m_nYOffset = 0;
    
    /* 缩放和填充图像 */
    bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);
};

} // namespace HumanAreaDetect_NS
