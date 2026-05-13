
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "AttributePostProcess.hpp"
#include "CVInferenceMOL.hpp"
#include "iostream"
#include <vector>
#include <string>

namespace Inference_NS
{

    class CAttribute : public CCVInferenceMOL
    {
    public:
        CAttribute(std::string strConfigPath);
        ~CAttribute();

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

        bool setParam(float fThreshold = 0.5);

    private:
        PostProcess_NS::cAttributePostProcess *m_postProcess = nullptr;

    public:
        bool bPerformanceTest = false;  /* 是否启用模型性能测试 */
        std::vector<float> vTestResult; /* 神经网络的输出 */

    private:
        int m_nClassNum = 0;
        std::vector<std::vector<int>> m_vGroupOnce;
        float m_fConfThreshold = 0.5;
    };
} // namespace Inference_NS
