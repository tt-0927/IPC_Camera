/*
 * @FilePath     : NumberOcr.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-30 08:53:40
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-05 10:01:05
 * @Description  : 黑底白数字识别
 */
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "CVInferenceRK.hpp"

namespace Inference_NS
{
    class CNumberOcr : public CCVInferenceRK
    {
    public:

        CNumberOcr(std::string strConfigPath);
        ~CNumberOcr();

    public:

        /**
         * @brief 重写父类的解析json模型后处理数据，用于适配不同类型的模型
         * @return true 
         * @return false 
         */
        bool checkModelProConfig() override;

        /**
         * @brief 推理数据
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ClsData_S>&] vClsDatas: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::ClsData_S>& vClsDatas);

    };


}    // namespace Inference_NS
