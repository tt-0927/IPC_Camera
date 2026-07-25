/*
 * @FilePath     : FightClassify.hpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-08-19 16:29:52
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-08-19 16:29:52
 * @Description  : 打架识别算法
 */
#pragma once

#include "CVInferenceRK_V1_0.hpp"

namespace InferenceV1_0_NS
{
    class CFightClassify : public CCVInferenceRK
    {
    public:

        CFightClassify(std::string strModelPath);
        ~CFightClassify();

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
