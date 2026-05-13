/**
 * @file OpencvPreprocess.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-24
 *
 * @brief
 */

#pragma once

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/dnn.hpp>
#include <vector>
#include "cix_noe_standard_api.h"

namespace PreProcess_NS
{

    inline cv::Mat QuantizeData(cv::Mat aInputData, float fScale, int nZeroPoint, noe_data_type_t eDataType)
    {
        if (aInputData.type() != CV_32F)
        {
            aInputData.convertTo(aInputData, CV_32F);
        }

        cv::Mat aScaled;
        aInputData.convertTo(aScaled, CV_32F, fScale, -nZeroPoint);
        int nCvType = 0;
        switch (eDataType) {
            case NOE_DATA_TYPE_S8:
                nCvType = CV_8S;
                break;
            case NOE_DATA_TYPE_U8:
                nCvType = CV_8U;
                break;
            case NOE_DATA_TYPE_S16:
                nCvType = CV_16S;
                break;
            case NOE_DATA_TYPE_U16:
                nCvType = CV_16U;
                break;
            case NOE_DATA_TYPE_F16:
                nCvType = CV_16F;
                break;
            case NOE_DATA_TYPE_F32:
                nCvType = CV_32F;
                break;
            default:
                throw std::runtime_error("Unsupported output data type");
        }
        aScaled.convertTo(aScaled, nCvType, 1.0, 0.5f);
        return aScaled;
    }

    inline bool CixPreprocess(
        cv::Mat aInput,
        cv::Mat &aOutput,
        int nWidth,
        int nHeight,
        std::vector<float> vfMean,
        std::vector<float> vfStd,
        bool bRgb,
        float fScale,
        int nZeroPoint,
        noe_data_type_t eDataType)
    {
        /* 各通道的归一化倍数是否一样 */
        bool bNormal = std::adjacent_find(vfStd.begin(), vfStd.end(), std::not_equal_to<>()) == vfStd.end();
        /* 归一化 */
        if (!bNormal)
        {
            /* 1. 分离三个通道 */
            std::vector<cv::Mat> aChannels;
            cv::split(aInput, aChannels);
            /* 2. 对每个通道分别进行缩放 */
            for (int nC = 0; nC < aChannels.size(); nC++)
            {
                cv::multiply(aChannels[nC], 1.0 / vfStd[nC], aChannels[nC]);
            }

            /* 3. 合并通道 */
            cv::merge(aChannels, aInput);
        }

        /* 设置目标尺寸 */
        cv::Size stTargetSize = cv::Size(nWidth, nHeight);

        /* 设置方差 */
        float fSC = bNormal ? (1.0 / vfStd[0]) : 1.0;
        /* 设置均值 */
        cv::Scalar stvfMean = cv::Scalar(0, 0, 0);
        if (vfMean.size() == 1)
        {
            stvfMean = cv::Scalar(vfMean[0]);
        }
        else if (vfMean.size() == 3)
        {
            stvfMean = cv::Scalar(vfMean[0], vfMean[1], vfMean[2]);
        }
        /* 创建 4D blob，适用于神经网络输入 */
        aOutput = cv::dnn::blobFromImage(
            aInput,       /* 输入图像 */
            fSC,          /* 缩放因子 */
            stTargetSize, /* 目标尺寸 */
            stvfMean,     /* 均值（减去） */
            bRgb,         /* 是否交换 BGR 和 RGB 通道 */
            false,        /* 是否裁剪图像 */
            CV_32F        /* 输出数据类型 */
        );

        /* 模型缩放 */
        aOutput = QuantizeData(aOutput, fScale, nZeroPoint, eDataType);
        return true;
    }
} // namespace PreProcess_NS
