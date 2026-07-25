/**
 * @file AVInferenceRK.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-05
 * 
 * @brief 
 */
#pragma once

#include <fstream>
#ifndef SHAREJSON_ENABLE//公共库的json文件
#include "JsonInterfase.h"
#else
#include "Json.h"
#endif
#include "ModelOpt.hpp"

namespace Inference_NS
{
    class CAVInferenceRK
    {
    public:
        CAVInferenceRK(std::string strConfigPath);

        ~CAVInferenceRK();

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
         * @brief 获取输入音频限制
         * @param [int] nIndex: 第几个输入音频, 0开始
         * @param [std::vector<int>&] vModelSize: 模型输出大小
         * @return [*]
         * @note
         */
        bool getSizeLimit(int nIndex, std::vector<int> &vModelSize);

        /**
         * @brief 获取输出音频限制
         * @param [int] nIndex: 第几个输出音频, 0开始
         * @param [std::vector<int>&] vModelSize: 模型输出大小
         * @return [*]
         * @note
         */
        bool getOutputSizeLimit(int nIndex, std::vector<int> &vModelSize);

        /**
         * @brief 初始化输入输出参数
         * @return [*]
         * @note
         */
        bool initParams();

        /**
         * @brief 推理的使用前判断
         * @param [int] nTextSize: 传入的文本token大小
         * @param [int] nInputIndex: 神经网络输入的下标，默认为0
         * @return [*]
         * @note
         */
        bool inferenceInfe(int nTextSize, int nInputIndex = 0);

        /**
         * @brief 校验模型配置文件的公共信息
         * @return [*]
         */
        bool checkModelConfig();

        /**
         * @brief 校验模型配置文件中的预处理信息
         * @return [*]
         */
        virtual bool checkModelPreConfig();

        /**
         * @brief 校验模型配置文件中的模型推理信息
         * @return [*]
         */
        virtual bool checkModelInferConfig();
        
        /**
         * @brief 校验模型配置文件中的后处理信息
         * @return [*]
         */
        virtual bool checkModelProConfig();

    public:
        std::string m_strConfigPath;        /* 模型json配置路径 */
        std::string m_strModelPath;             /* 模型路径 */
        CModelOpt *m_pModel = nullptr;          /* 模型操作句柄 */
        rknn_input*  m_pInputs    = nullptr; /* 模型输入参数 */
        int m_nInputNum = 0;                    /* 模型输入参数数量 */
        rknn_output* m_pOutputs   = nullptr; /* 模型输出参数 */
        int m_nOutputNum = 0;                   /* 模型输出参数数量 */

        /* 模型的属性信息 */
        std::vector<rknn_tensor_attr> m_vInputAttrs;
        std::vector<rknn_tensor_attr> m_vOutputAttrs;

        /* 模型处理数据限制 */
        std::vector<int> m_vModelSize;

        /* 模型推理的相关信息 */
        std::string m_strFramework;  /* 推理架构 */
    };

} // namespace Inference_NS
