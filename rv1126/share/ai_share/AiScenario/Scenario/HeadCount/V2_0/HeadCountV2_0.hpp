/*
 * @FilePath     : HeadCountV2_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-17 17:43:17
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-09-02 17:28:56
 * @Description  :
 */
#pragma once

#include "CVInferenceBase.hpp"
#include "ScenarioBase.hpp"

namespace Scenario_NS
{
    class CHeadCountV2_0 : public CScenarioBase
    {
    public:

        CHeadCountV2_0(AiScenario_NS::InParam_S stInParam);
        ~CHeadCountV2_0();

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
         * @param [int&] nDataSize: 数据大小
         * @return [*]
         * @note
         */
        bool process(AiScenario_NS::CVData_S stInData, char*& pchOutData, int& nDataSize) override;

        /**
         * @brief 处理数据
         * @param [CAData_S] stInData: 传入的音频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @param [int&] nDataSize: 数据大小
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

    private:

        InferenceV1_0_NS::CCVInferenceBase* m_pInference = nullptr;

        /* yolo的阈值 */
        float m_fBoxThreshold = 0.75;
        float m_fNmsThreshold = 0.25;

        /* 算法输入参数限制 */
        int m_nLimitHeight  = 0;
        int m_nLimitWidth   = 0;
        int m_nLimitChannel = 0;
    };

}    // namespace Scenario_NS
