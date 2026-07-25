/*
 * @FilePath     : CAInferenceBase.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-17 16:41:34
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-06-05 15:07:01
 * @Description  : 音频推理基类
 */
#pragma once

#include "CAExtern.hpp"

namespace InferenceV1_0_NS
{
    class CCAInferenceBase
    {
    public:

        CCAInferenceBase()
        {
        }

        virtual ~CCAInferenceBase()
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
         * @brief 推理数据
         * @param [Mat] inMat:传入的图片数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @return [*]
         * @note
         */
        virtual bool inference(AiScenario_NS::CAData_S stInData, std::string& strOutData) = 0;


    protected:
    };

}    // namespace InferenceV1_0_NS
