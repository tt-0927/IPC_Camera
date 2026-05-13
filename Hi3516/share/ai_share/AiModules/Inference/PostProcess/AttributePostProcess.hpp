/**
 * @file AttributePostProcess.hpp
 * @author caishengjie (caisj@kfb.cn)
 * @date 2024-10-09
 *
 * @brief
 */

#pragma once

#include <vector>

namespace PostProcess_NS
{
    class cAttributePostProcess
    {
    public:
        /**
         * @brief
         * @param pfInput 模型输出的向量作为后处理的输入
         * @param fConfThreshold 用于筛选多属性输出的阈值
         * @param vOutputIndex vector用于接后处理输出的属性索引
         * @return true
         * @return false
         */
        bool postProcess(
            float *pfInput,
            float nConfThreshold,
            std::vector<float> &vOutput);

        void setParam(int nClassNum, std::vector<std::vector<int>> vGroupOnce);

        cAttributePostProcess();
        ~cAttributePostProcess();

    private:
        int m_nClassNum = 0;
        std::vector<std::vector<int>> m_vGroupOnce;

        static float sigmoid(float fX);

        static float unsigmoid(float fY);

        /**
         * @brief 获取每个子数组最大属性的索引
         * @param vArray 模型输出的向量作为输入
         * @param vResult vector用于接收每个子数组输出的属性索引
         * @return true
         * @return false
         */
        bool getMaxIndex(
            float *vArray,
            std::vector<float> &vResult);

        /**
         * @brief 获取超过阈值的属性索引
         * @param vResult vector用于接收超过阈值的的属性索引
         * @return true
         * @return false
         */
        bool getThresholdIndex(
            float fThreshold,
            std::vector<float> &vResult);
    };
} // namespace PostProcess_NS
