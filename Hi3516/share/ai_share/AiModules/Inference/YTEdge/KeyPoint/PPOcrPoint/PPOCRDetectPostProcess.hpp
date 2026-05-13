/*
 * @FilePath     : HeadCountPPV2_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:39
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-30 20:03:29
 * @Description  :
 */
#pragma once
#include <stdio.h>
#include <cmath>
#include <vector>
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "Clipper.hpp"
#include "OutputDataEXT.hpp"

namespace PostProcess_NS
{
    class cPPOCRDetectPostProcess
    {
    public:
        /*
         * @brief 数据后处理
         * @param [std::vector<float *>] vInput: 神经网络的输出
         * @param [int] nOutuutWidth: 输入模型宽
         * @param [int] nOutputHeight: 输入模型高
         * @param [float] fMaskThreshold: Mask掩码阈值
         * @param [float] fBoxThreshold: 文本框阈值
         * @param [float &] fUnclipRatio: 文本框扩展比例
         * @param [bool] bPolyType: 是否启动多边形优化
         * @param [std::vector<Inference_NS::PointData_S>&] vPointDatas: 处理后的数据
         * @return [*]
         * @note
        */
        bool postPolyProcess(
            std::vector<float *> vInput,
            int nOutuutWidth,
            int nOutputHeight,
            float fMaskThreshold,
            float fBoxThreshold,
            const float &fUnclipRatio,
            bool bPolyType,
            std::vector<Inference_NS::PointData_S> &vPointDatas);

    private:
        inline static int clamp(float fVal, int nMin, int nMax)
        {
            return fVal > nMin ? (fVal < nMax ? fVal : nMax) : nMin;
        }
        std::vector<std::vector<int>> orderPointsClockwise(std::vector<std::vector<int>> vvPts);
        float boxScoreFast(std::vector<std::vector<float>> vBoxArray, cv::Mat aPred);
        std::vector<std::vector<float>> getMiniBoxes(cv::RotatedRect aBox, float &fSsid);
        cv::RotatedRect unClip(std::vector<std::vector<float>> &vvBox, const float &fUnclipRatio);
        float polygonScoreAcc(std::vector<cv::Point> vContour, cv::Mat aPred);
    
    private:
        bool m_bUseDilation = false;
    
    };
} // namespace PostProcess_NS
