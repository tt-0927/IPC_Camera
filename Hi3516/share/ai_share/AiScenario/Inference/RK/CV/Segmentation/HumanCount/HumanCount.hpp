/*
 * @FilePath     : HumanCount.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-22 20:28:45
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-08-01 16:29:52
 * @Description  : 基于热力图IIM的人数统计算法
 */
#pragma once
#include "IIMPostProcess.hpp"
#include "CVInferenceRK_V1_0.hpp"

namespace InferenceV1_0_NS
{
    class CHumanCount : public CCVInferenceRK
    {
    public:

        CHumanCount(std::string strModelPath);
        ~CHumanCount();

    private:
        /**
         * @brief 推理数据
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @param [std::vector<float>&] vOutData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(AiScenario_NS::CVData_S stInData, std::vector<float>& vOutData) override;
        
	/* 后处理 */
        PostProcessV1_0_NS::cIIMPostProcess* m_postProcess;
    };


}    // namespace InferenceV1_0_NS
