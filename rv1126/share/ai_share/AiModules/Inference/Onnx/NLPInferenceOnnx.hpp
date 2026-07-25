/**
 * @file NLPInferenceRK.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-05
 * 
 * @brief 
 */
#pragma once

#include <fstream>
#include "OnnxModelOpt.hpp"
#include "JsonInterfase.h"
#include "clip_tokenizer.h"
#include "Tokenizer.hpp"

namespace Inference_NS
{
    class CNLPInferenceOnnx
    {
    public:
        CNLPInferenceOnnx(std::string strConfigPath);

        ~CNLPInferenceOnnx();

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
         * @brief 校验模型配置文件中的模型推理信息
         * @return [*]
         */
        virtual bool checkModelInferConfig();

    protected:
        std::string m_strConfigPath;            /* 模型json配置路径 */
        std::string m_strModelPath;             /* 模型路径 */
        std::string m_strVocabPath;             /* 分词文件路径 */
        COnnxModelOpt *m_pModel = nullptr;          /* 模型操作句柄 */
        int m_nInputNum = 0;                    /* 模型输入参数数量 */
        int m_nOutputNum = 0;                   /* 模型输出参数数量 */

        /* 模型的属性信息 */
        std::vector<std::vector<int64_t>> m_vInputAttrs;
        std::vector<std::vector<int64_t>> m_vOutputAttrs;

        /* 模型处理数据限制 */
        std::vector<int> m_vModelSize;

        // CLIPTokenizer *m_pTokenize = nullptr; /* 英文接口 */
        // TokenizerClip *m_pTokenize = nullptr;
        TokenizerClipChinese *m_pTokenize = nullptr;

        /* 模型推理的相关信息 */
        std::string strFramework;  /* 推理架构 */
        int m_nCpuInferThread = 1; /* CPU推理的线程数 */
        int m_nDeviceId = 0;    /* 选择 GPU 设备的序列号 */
        long long m_nGpuMemLimit = 2 * 1024 * 1024 * 1024;  /* 模型限制最大显存 */
    };

} // namespace Inference_NS
