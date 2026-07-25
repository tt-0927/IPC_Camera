/*
 * @FilePath     : FaceRecPPV1_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:39
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-30 15:58:37
 * @Description  :
 */
#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <set>
#include <iostream>
#include <cmath>
#include <vector>

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

namespace PostProcessV1_0_NS
{
    class cRetinafacePostProcess
    {
    public:

        /**
         * @brief 数据后处理
         * @param [float*] pInput0: 神经网络的第一个输出
         * @param [float*] pInput1: 神经网络的第二个输出
         * @param [int] nHeight: 输出数据高
         * @param [int] nWidth: 输出数据宽
         * @param [std::vector<float>&] vPointsXY: 处理后的数据
         * @return [*]
         * @note
         */
        bool postProcess(float*              pInput0,
                         float*              pInput1,
                         float*              pInput2,
                         int                 nModelNeed,
                         int                 nHeight,
                         int                 nWidth,
                         float               nConfThreshold,
                         float               nNmsThreshold,
                         std::vector<float>& vPointsXY);

    private:
        /**
         * @brief 辅助解码容器的生产
         * @param [std::vector<float> &] vPriors
         * @param [std::vector<int> &] vNeedVector: 神经网络的第二个输出
         * @param [int] nHeight: 输出数据高
         * @param [int] nWidth: 输出数据宽
         * @return [*]
         * @note
         */
        void vPriorBoxVector(std::vector<float> &vPriors,std::vector<int> &vNeedVector,int nWidth, int nHeight);

        /**
         * @brief 预测框的解码
         * @param [std::vector<int>] vNeedVector
         * @param [std::vector<float>] vPriors
         * @param [float *] pInput0
         * @param [int] nWidth: 输出数据宽
         * @param [int] nHeight: 输出数据高
         * @return [*]
         * @note 
         */
        void decodeBoxs(std::vector<int> vNeedVector, std::vector<float> vPriors, float *pInput0, int nWidth, int nHeight);

        /**
         * @brief 五个人脸关键点的解码
         * @param [std::vector<int>] vNeedVector
         * @param [std::vector<float>] vPriors
         * @param [float *] pInput0
         * @param [int] nWidth: 输出数据宽
         * @param [int] nHeight: 输出数据高
         * @return [*]
         * @note 
         */
        void decodeLandm(std::vector<int> vNeedVector,std::vector<float> vPriors,float *pInput2,int nWidth, int nHeight);
    };

}    // namespace PostProcessV1_0_NS
