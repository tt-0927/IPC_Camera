/*
 * @FilePath     : Fastpose.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-01 16:29:52
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-08-01 16:29:52
 * @Description  : 单人的26个人体关键点识别
 */
#pragma once

#include "CVInferenceRK_V1_0.hpp"
#include "YOLOV5PostProcessV1_0.hpp"

namespace InferenceV1_0_NS
{
    class CFastpose : public CCVInferenceRK
    {
    public:

        CFastpose(std::string strModelPath);
        ~CFastpose();

    private:
        /**
         * @brief 推理数据
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @param [std::vector<float>&] vOutData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(AiScenario_NS::CVData_S stInData, std::vector<float>& vOutData);
    };


}    // namespace InferenceV1_0_NS
