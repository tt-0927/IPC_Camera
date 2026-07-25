/*
 * @FilePath     : FaceDetect.hpp
 * @Author       : lih lih@kfb.cn
 * @Date         : 2024-06-19 15:31:40
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-08-01 16:29:52
 * @Description  : 人脸检测算法（带有5个关键点）
 */
#pragma once

#include "CVInferenceRK_V1_0.hpp"
#include "RetinafacePostProcess.hpp"

namespace InferenceV1_0_NS
{
    class CFaceDetect : public CCVInferenceRK
    {
    public:
        CFaceDetect(std::string strModelPath);
        ~CFaceDetect();

    private:

        /**
         * @brief 推理数据
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @param [std::vector<float>&] vOutData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(AiScenario_NS::CVData_S stInData, std::vector<float>& vOutData) override;

    private:

        /* 后处理 */
        PostProcessV1_0_NS::cRetinafacePostProcess* m_postProcess;

        /* 人脸检测的阈值 */
        float m_fBoxThreshold = 0.75;
        float m_fNmsThreshold = 0.25;
    };


}    // namespace InferenceV1_0_NS

