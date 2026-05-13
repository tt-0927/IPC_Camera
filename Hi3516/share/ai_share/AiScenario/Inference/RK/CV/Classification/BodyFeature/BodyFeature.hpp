/*
 * @FilePath     : BodyFeature.hpp
 * @Author       : lih lih@kfb.cn
 * @Date         : 2024-06-19 15:31:40
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-08-01 16:29:52
 * @Description  : 人体特征点提取
 */
#pragma once

#include "CVInferenceRK_V1_0.hpp"

namespace InferenceV1_0_NS
{
    class CBodyFeature : public CCVInferenceRK
    {
    public:

        CBodyFeature(std::string strModelPath);
        ~CBodyFeature();

    private:


        /**
         * @brief 推理数据
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @param [std::vector<float>&] vOutData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(AiScenario_NS::CVData_S stInData, std::vector<float>& vOutData) override;

    };


}    // namespace InferenceV1_0_NS
