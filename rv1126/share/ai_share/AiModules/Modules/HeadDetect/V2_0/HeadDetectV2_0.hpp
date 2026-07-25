/**
 * @file HeadDetectV2_0.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-02-20
 * 
 * @brief 
 */
#pragma once

#include <unordered_map>
#include "Yolov5.hpp"
#include "HeadDetectExt.hpp"

namespace HeadDetect_NS
{
    class CHeadDetectV2_0
    {
    public:
        CHeadDetectV2_0(InParam_S stInParam);
        ~CHeadDetectV2_0();

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
         * @param vResults 分析的参数
         * @return true
         * @return false
         */
        bool process(InData_S stInData, std::vector<Result_S> &vResults);

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

        Inference_NS::CYolov5 *m_pYolov5 = nullptr;

        /* 算法输入参数限制 */
        int m_nLimitHeight = 0;
        int m_nLimitWidth = 0;
        int m_nLimitChannel = 0;

        /* 缩放填充后左上角的坐标 */
        int m_nXOffset = 0;
        int m_nYOffset = 0;
        /* 缩放比例 */
        float m_fResizeScale = 1.0;
    };

} // namespace HeadDetect_NS
