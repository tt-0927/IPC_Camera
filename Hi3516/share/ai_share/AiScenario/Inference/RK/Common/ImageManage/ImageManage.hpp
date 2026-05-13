/*
 * @FilePath     : ImageManage.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-07-22 09:07:30
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-07-22 14:58:46
 * @Description  : 图像操作-使用RK硬件
 */
#pragma once

#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

namespace InferenceV1_0_NS
{
    class CImageManage
    {
    public:

        CImageManage();
        ~CImageManage();

    private:

        /**
         * @brief 裁剪
         * @param [Mat] inMat: 输入图片数据
         * @param [Rect] rect: 裁剪矩形参数
         * @param [Mat&] outMat: 输出图片
         * @return [*]
         * @note
         */
        bool cropping(cv::Mat inMat, cv::Rect rect, cv::Mat& outMat);
        // bool cropping(cv::Mat inMat, cv::Rect rect, cv::Mat& outMat);
    };

}    // namespace InferenceV1_0_NS