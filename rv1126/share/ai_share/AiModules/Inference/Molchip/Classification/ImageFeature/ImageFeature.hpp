/**
 * @file ImageFeature.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-11
 * 
 * @brief 图片特征提取
 */
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "CVInferenceMOL.hpp"
#include "iostream"

namespace Inference_NS
{

    class CImageFeature : public CCVInferenceMOL
    {
    public:
        CImageFeature(std::string strConfigPath);
        ~CImageFeature();

        /**
         * @brief 推理数据
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ClsData_S>&] vClsDatas: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::ClsData_S>& vClsDatas);

    };

} // namespace Inference_NS
