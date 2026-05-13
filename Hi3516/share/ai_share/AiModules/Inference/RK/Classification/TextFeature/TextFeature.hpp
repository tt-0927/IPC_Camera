/**
 * @file TextFeature.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-11
 *
 * @brief
 */
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "NLPInferenceRK.hpp"
#include "iostream"

namespace Inference_NS
{

    class CTextFeature : public CNLPInferenceRK
    {
    public:
        CTextFeature(std::string strConfigPath);
        ~CTextFeature();

        /**
         * @brief 文字特征提取
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ClsData_S>&] vClsDatas: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::ClsData_S>& vClsDatas);

    };

} // namespace Inference_NS
