/**
 * @file CixModelOpt.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-11-07
 * 
 * @brief 
 */
#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "cix_noe_standard_api.h"

namespace Inference_NS
{
    class CCixModelOpt
    {
    public:
        CCixModelOpt(std::string strModelPath);
        ~CCixModelOpt();

        /**
         * @brief 设置模型路径
         * @return [*]
         * @note 再次初始化的时候生效
         */
        void setModelPath(std::string strModelPath);

        /**
         * @brief 初始化模型
         * @return [*]
         * @note
         */
        bool init();

        /**
         * @brief 反初始化模型
         * @return [*]
         * @note
         */
        bool unInit();

        /**
         * @brief 获取模型输入参数
         * @param [vector<std::vector<int64_t>>&] vInputAttrs: 输入参数
         * @return [*]
         * @note
         */
        bool getInputAttrs(std::vector<tensor_desc_t> &vInputAttrs);

        /**
         * @brief 获取模型输出参数
         * @param [std::vector<tensor_desc_t>&] vInputAttrs: 输出参数
         * @return [*]
         * @note
         */
        bool getOutputAttrs(std::vector<tensor_desc_t> &vOutputAttrs);

        /**
         * @brief 运行模型
         * @param [std::vector<void*>] vInputs: 输入的图片数据
         * @param [std::vector<std::vector<float>>&] vOutputs: 模型的输出
         * @return [*]
         * @note
         */
        bool run(
            std::vector<void *> vInputs,
            std::vector<std::vector<float>> &vOutputs);

    private:
        /* 格式打印 */
        const char *dataTypeString(noe_data_type_t stDataType);

    private:
        /* 模型路径 */
        std::string m_strModelPath;

        /* 模型句柄 */
        uint64_t m_nGraphId;
        context_handler_t *m_pCtx = nullptr;

        bool m_bInitialized = false;

        /* 模型输出 */
        std::vector<void *> m_vOutputs;

        /* 模型的输入和输出的属性 */
        std::vector<tensor_desc_t> m_vInputAttrs;
        std::vector<tensor_desc_t> m_vOutputAttrs;
    };

} // namespace Inference_NS