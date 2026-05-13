/**
 * @file ZipFormerDecode.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-06
 * 
 * @brief 
 */
#pragma once

#include "AVInferenceRK.hpp"
#include "iostream"

namespace Inference_NS
{

    class CZipFormerDecode : public CAVInferenceRK
    {
    public:
        CZipFormerDecode(std::string strConfigPath);
        ~CZipFormerDecode();

        /**
         * @brief 文字特征提取
         * @param [std::vector<Inference_NS::InputData_S>] vInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ClsData_S>&] vClsDatas: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference();

    };

} // namespace Inference_NS
