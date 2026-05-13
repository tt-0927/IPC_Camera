/*
 * @FilePath     : NumberOcrV1_0.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-30 08:53:40
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:49:04
 * @Description  : 黑底白数字识别
 */
#pragma once

#include "CVInferenceBase.hpp"
#include "ScenarioBase.hpp"

namespace Scenario_NS
{
    class CNumberOcrV1_0 : public CScenarioBase
    {
    public:

        CNumberOcrV1_0(AiScenario_NS::InParam_S stInParam);
        ~CNumberOcrV1_0();

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init() override;

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit() override;

        /**
         * @brief 处理数据
         * @param [CVData_S] stInData: 传入的视频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @return [*]
         * @note
         */
        bool process(AiScenario_NS::CVData_S stInData, char*& pchOutData, int& nDataSize) override;

        /**
         * @brief 处理数据
         * @param [CAData_S] stInData: 传入的音频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @return [*]
         * @note
         */
        bool process(AiScenario_NS::CAData_S stInData, char*& pchOutData, int& nDataSize) override;

        /**
         * @brief 释放处理结果
         * @param [char*&] pchOutData: 处理结果指针
         * @return [*]
         * @note
         */
        bool releaseData(char*& pchOutData) override;

    private:

        bool convertToJson(std::vector<float> vPointsXY, char** pchOutData, int& nDataSize);
        bool convertToString(std::vector<float> vPointsXY, char** pchOutData, int& nDataSize);

    private:

        InferenceV1_0_NS::CCVInferenceBase* m_pInference = nullptr;

        /* 模型处理数据限制 */
        int m_nLimitHeight  = 0;
        int m_nLimitWidth   = 0;
        int m_nLimitChannel = 0;

    };

}    // namespace Scenario_NS
