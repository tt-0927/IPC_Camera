/*
 * @FilePath     : ModelOptV1_0.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-29 17:30:28
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-05-30 08:40:26
 * @Description  : RK模型操作
 */
#pragma once

#include <string>
#include <vector>

#include "rknn_api.h"

namespace InferenceV1_0_NS
{
    class CModelOpt
    {
    public:

        CModelOpt(std::string strModelPath);
        ~CModelOpt();

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
         * @param [std::vector<rknn_tensor_attr>&] vInputAttrs: 输入参数
         * @return [*]
         * @note
         */
        bool getInputAttrs(std::vector<rknn_tensor_attr>& vInputAttrs);

        /**
         * @brief 获取模型输出参数
         * @param [std::vector<rknn_tensor_attr>&] vInputAttrs: 输出参数
         * @return [*]
         * @note
         */
        bool getOutputAttrs(std::vector<rknn_tensor_attr>& vOutputAttrs);

        /**
         * @brief 释放输出参数
         * @param [rknn_output *&] pOutputs: 输出参数
         * @param [int] nNum: 输出参数数量
         * @return [*]
         * @note
         */
        bool releaseOutputs(rknn_output*& pOutputs, int nNum);

        /**
         * @brief 运行模型
         * @param [rknn_input*] pInputs: 输入参数
         * @param [int] nInSize: 输入参数数量
         * @param [rknn_output*&] pOutputs: 输出参数
         * @param [int] nOutSize: 输出参数数量
         * @return [*]
         * @note
         */
        bool run(rknn_input* pInputs, int nInNum, rknn_output*& pOutputs, int nOutNum);

    private:

        /**
         * @brief 载入模型
         * @param [std::string] strFileName: 模型路径
         * @param [int&] nModelSize: 模型大小
         * @return [*]
         * @note
         */
        unsigned char* loadModel(std::string strFileName, int& nModelSize);

    private:

        /* 模型路径 */
        std::string m_strModelPath;

        /* 模型句柄 */
        rknn_context m_hCtx;
        bool         m_bInitialized = false;

        /* 模型输入输出IO信息 */
        rknn_input_output_num m_stIoNumInfo;

        /* tensor的属性信息 */
        std::vector<rknn_tensor_attr> m_vInputAttrs;
        std::vector<rknn_tensor_attr> m_vOutputAttrs;
    };

}    // namespace InferenceV1_0_NS