/**
 * @file NLPInferenceRK.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-05
 * 
 * @brief 
 */
#pragma once

#include <fstream>
#include "ModelOpt.hpp"
#ifndef SHAREJSON_ENABLE//公共库的json文件
#include "JsonInterfase.h"
#else
#include "Json.h"
#endif
#include "clip_tokenizer.h"
#include "Tokenizer.hpp"

namespace Inference_NS
{
    class CNLPInferenceRK
    {
    public:
        CNLPInferenceRK(std::string strConfigPath);

        ~CNLPInferenceRK();

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
         * @brief 获取输入图片限制
         * @param [int] nIndex: 第几个输入图片, 0开始
         * @param [std::vector<int>&] vModelSize: 模型输出大小
         * @return [*]
         * @note
         */
        bool getSizeLimit(int nIndex, std::vector<int> &vModelSize);

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

    protected:
        std::string m_strConfigPath;            /* 模型json配置路径 */
        std::string m_strModelPath;             /* 模型路径 */
        std::string m_strVocabPath;             /* 分词文件路径 */
        CModelOpt *m_pModel = nullptr;          /* 模型操作句柄 */
        rknn_tensor_mem **m_pInputs = nullptr;  /* 模型输入参数 */
        int m_nInputNum = 0;                    /* 模型输入参数数量 */
        rknn_tensor_mem **m_pOutputs = nullptr; /* 模型输出参数 */
        int m_nOutputNum = 0;                   /* 模型输出参数数量 */

        /* 模型的属性信息 */
        std::vector<rknn_tensor_attr> m_vInputAttrs;
        std::vector<rknn_tensor_attr> m_vOutputAttrs;

        /* 模型处理数据限制 */
        std::vector<int> m_vModelSize;

        // CLIPTokenizer *m_pTokenize = nullptr; /* 英文接口 */
        // TokenizerClip *m_pTokenize = nullptr;
        TokenizerClipChinese *m_pTokenize = nullptr;
    };

} // namespace Inference_NS
