/*
 * @FilePath     : StudentBehaviorV2_0.hpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-01 16:29:52
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-09-03 10:40:10
 * @Description  : 基于骨骼点检测的行为分析算法应用逻辑
 */
#pragma once

#include "CVInferenceBase.hpp"
#include "ScenarioBase.hpp"

namespace Scenario_NS
{
    class CStudentBehaviorV2_0 : public CScenarioBase
    {
    public:

        CStudentBehaviorV2_0(AiScenario_NS::InParam_S stInParam);
        ~CStudentBehaviorV2_0();

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

        bool convertToJson(
            const std::vector<float>&              vecBoxes,
            const std::vector<int>&                vecBehavior,
            const std::vector<std::vector<float>>& vecKeyPoses,
            char**                                 pchOutData,
            int&                                   nDataSize);

    private:

        InferenceV1_0_NS::CCVInferenceBase* m_pHInference = nullptr;
        InferenceV1_0_NS::CCVInferenceBase* m_pBInference = nullptr;

        /* 人头检测模型处理数据限制 */
        int m_nHeadDetectLimitHeight  = 0;
        int m_nHeadDetectLimitWidth   = 0;
        int m_nHeadDetectLimitChannel = 0;

        /* 人体关键点提取模型处理数据限制 */
        int m_nFastPoseLimitHeight   = 0;
        int m_nFastPoseLimitWidth    = 0;
        int m_nFastPosetLimitChannel = 0;

        /* yolo的阈值 */
        float m_fBoxThreshold = 0.75;
        float m_fNmsThreshold = 0.25;
    };

}    // namespace Scenario_NS
