/*
 * @FilePath     : CVInferenceBase.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-17 16:41:34
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-02 17:21:10
 * @Description  : 视频推理基类
 */
#pragma once

#include "CVExtern.hpp"
#include "Extern.hpp"

namespace InferenceV1_0_NS
{
    class CCVInferenceBase
    {
    public:

        CCVInferenceBase()
        {
        }

        virtual ~CCVInferenceBase()
        {
        }

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        virtual bool init() = 0;

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        virtual bool unInit() = 0;

        /**
         * @brief 获取输入图片限制
         * @param [int] nIndex: 第几个输入图片, 0开始
         * @param [int&] nWidth: 需要的图像宽度
         * @param [int&] nHeight: 需要的图像高度
         * @param [int&] nChannel: 需要的图像通道号
         * @return [*]
         * @note
         */
        virtual bool getSizeLimit(int nIndex, int& nWidth, int& nHeight, int& nChannel) = 0;

        /**
         * @brief 推理数据
         * @param [AiScenario_NS::CVData_S] stInData: 传入的图片数据
         * @param [std::vector<void*>&] vOutData: 推理出来的数据
         * @return [*]
         * @note
         */
        virtual bool inference(AiScenario_NS::CVData_S stInData, std::vector<float>& vOutData) = 0;

        /**
         * @brief 设置可选参数
         * @param [ExParam_S] stExParam: 配置结构体
         * @return [*]
         * @note
         */
        bool setExParams(AiScenario_NS::ExParam_S stExParam)
        {
            /* 保存上一次的参数 */
            AiScenario_NS::ExParam_S stExParamTmp = m_stExParam;

            m_stExParam = stExParam;

            /* 设置置信度和非极大值抑制的阈值，如果输入为0，则使用默认参数 */

            /* 判断是否设置了参数 */
            if (stExParam.fBoxThreshold == 0.0)
            {
                m_stExParam.fBoxThreshold = stExParamTmp.fBoxThreshold;
            }

            /* 判断是否设置了参数 */
            if (stExParam.fNmsThreshold == 0.0)
            {
                m_stExParam.fNmsThreshold = stExParamTmp.fNmsThreshold;
            }

            return true;
        }

    protected:

        /**
         * @brief 初始化输入输出参数
         * @return [*]
         * @note
         */
        virtual bool initParams() = 0;

    protected:

        /* 可选参数 */
        AiScenario_NS::ExParam_S m_stExParam;
    };

}    // namespace InferenceV1_0_NS
