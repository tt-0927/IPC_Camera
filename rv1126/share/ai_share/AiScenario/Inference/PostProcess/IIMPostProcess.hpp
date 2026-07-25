/*
 * @FilePath     : HeadCountPPV1_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:39
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-30 15:58:37
 * @Description  :
 */
#pragma once

#include <vector>

namespace PostProcessV1_0_NS
{
    class cIIMPostProcess
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
                         int                 nHeight,
                         int                 nWidth,
                         std::vector<float>& vPointsXY);
    };

}    // namespace PostProcessV1_0_NS
