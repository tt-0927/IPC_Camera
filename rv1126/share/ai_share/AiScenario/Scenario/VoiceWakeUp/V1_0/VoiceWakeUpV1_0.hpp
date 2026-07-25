/*
 * @FilePath     : VoiceWakeUpV1_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-31 15:36:31
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-09-03 10:40:33
 * @Description  :
 */
#pragma once

#include "CAInferenceBase.hpp"
#include "ScenarioBase.hpp"

namespace Scenario_NS
{
    class CVoiceWakeUpV1_0 : public CScenarioBase
    {
    public:

        CVoiceWakeUpV1_0(AiScenario_NS::InParam_S stInParam);
        ~CVoiceWakeUpV1_0();

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        bool init();

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        bool unInit();

        /**
         * @brief 处理数据
         * @param [CVData_S] stInData: 传入的视频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @param [int&] nDataSize: 数据大小
         * @return [*]
         * @note
         */
        bool process(AiScenario_NS::CVData_S stInData, char*& pchOutData, int& nDataSize);

        /**
         * @brief 处理数据
         * @param [CAData_S] stInData: 传入的音频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @param [int&] nDataSize: 数据大小
         * @return [*]
         * @note
         */
        bool process(AiScenario_NS::CAData_S stInData, char*& pchOutData, int& nDataSize);

        /**
         * @brief 释放处理结果
         * @param [char*&] pchOutData: 处理结果指针
         * @return [*]
         * @note
         */
        bool releaseData(char*& pchOutData);

    private:

        bool convertToJson(std::string strData, char** pchOutData, int& nDataSize);

    private:

        InferenceV1_0_NS::CCAInferenceBase* m_pInference = nullptr;
    };

}    // namespace Scenario_NS