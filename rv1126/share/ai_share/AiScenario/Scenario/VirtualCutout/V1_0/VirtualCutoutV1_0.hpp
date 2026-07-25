/*
 * @FilePath     : VirtualCutoutV1_0.hpp
 * @Author       : 李辉 lihui@kfb.cn
 * @Date         : 2024-09-02 08:43:17
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-09-03 10:40:39
 * @Description  : 虚拟抠像场景
 */
#pragma once

#include "ColorCutout.hpp"
#include "ScenarioBase.hpp"

namespace Scenario_NS
{
    class CVirtualCutoutV1_0 : public CScenarioBase
    {
    public:

        CVirtualCutoutV1_0(AiScenario_NS::InParam_S stInParam);
        ~CVirtualCutoutV1_0();

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

        bool convertToPic(cv::Mat aInputImage, char** pchOutData, int& nDataSize);

    private:

        ColorCutout_NS::CColorCutout* m_pInference = nullptr;
    };

}    // namespace Scenario_NS
