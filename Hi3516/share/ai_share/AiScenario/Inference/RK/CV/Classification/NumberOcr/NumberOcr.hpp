/*
 * @FilePath     : NumberOcr.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-30 08:53:40
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-05 10:01:05
 * @Description  : 黑底白数字识别
 */
#pragma once

#include "CVInferenceRK_V1_0.hpp"

namespace InferenceV1_0_NS
{
    class CNumberOcr : public CCVInferenceRK
    {
    public:

        CNumberOcr(std::string strModelPath);
        ~CNumberOcr();

    private:


        /**
         * @brief 推理数据
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @param [std::vector<float>&] vOutData: 推理出来的数据
         * @return [*] 
         * @note vOutData数据结构: 长度1,只有一个数字
         */
        bool inference(AiScenario_NS::CVData_S stInData, std::vector<float>& vOutData) override;

    };


}    // namespace InferenceV1_0_NS
