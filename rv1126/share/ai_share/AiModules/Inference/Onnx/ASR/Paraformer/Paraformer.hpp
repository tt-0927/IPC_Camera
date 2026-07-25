/**
 * @file Paraformer.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "ParaformerEncoder.hpp"
#include "ParaformerDecoder.hpp"

namespace Inference_NS
{
    typedef struct _VocabEntry_
    {
        int index;
        char *token;
    } VocabEntry_S;

    class CParaformer
    {
    public:
        CParaformer(
            std::string strEncoderConfigPath,
            std::string strDecoderConfigPath,
            std::string strVocabPath);
        ~CParaformer();

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
         * @brief 推理数据
         * @param [Inference_NS::AVInputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ASRData_S>&] stASRData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::AVInputData_S stInputData, Inference_NS::ASRData_S &stASRData);

        /**
         * @brief 清除上一句的Cache
         */
        bool clearCache();

    private:
        /* 字符串转为整形 */
        bool stringToInt(std::string strData, int &nOutData);
        /* 去除字符串首尾空格（辅助函数） */
        std::string trim(const std::string &strInData);
        /* 字符串切割，转为std::vector<float> */
        bool stringToFloatVecotr(std::string strData, std::vector<float> &vOutData);
        /* 降帧操作 */
        std::vector<float> applyLFR(const std::vector<float> &vInputData);
        /* 归一化 */
        void applyCMVN(std::vector<float> *pInData);
        /* 位置编码 */
        void positionalEncoding(std::vector<float> *pInData, int32_t nTOffset);
        /* 读取词表 */
        bool readVocab(const char *fileName);
    private:
        std::string m_strEncoderConfigPath;                   /* 编码模型json配置路径 */
        std::string m_strDecoderConfigPath;                   /* 解码模型json配置路径 */
        CParaformerEncoder *m_pEncodeModel = nullptr;         /* 模型操作句柄 */
        CParaformerDecoder *m_pDecodeModel = nullptr;         /* 模型操作句柄 */
        Ort::AllocatorWithDefaultOptions m_stEncodeAllocator; /* 内存管理类 */
        Ort::AllocatorWithDefaultOptions m_stDecodeAllocator; /* 内存管理类 */

        /* 此表 */
        std::string m_strVocabPath;
        std::vector<VocabEntry_S> m_vVocab;

        float m_fAlphaThreshold = 1.0;
        /* 模型配置参数 */
        int nChunkSize = 61;
        int nLeftChunkSize = 5;
        int nRightChunkSize = 3;
        int m_nVocabSize;     /* 词表大小 */
        int m_nLfrWindowSize; /* 滑动窗口大小 */
        int m_nLfrWindowShift;
        int m_nEncoderOutputSize;
        int m_nDecoderNumBlocks;
        int m_nDecoderKernelSize;
        int m_nInFeatDim = 80;
        std::vector<float> m_vNegMean;   /* 均值 */
        std::vector<float> m_vInvStddev; /* 方差 */

        /* 模型的属性信息 */
        std::vector<std::vector<int64_t>> m_vEncoerInputAttrs;
        std::vector<std::vector<int64_t>> m_vEncoerOutputAttrs;
        std::vector<std::vector<int64_t>> m_vDecoerInputAttrs;
        std::vector<std::vector<int64_t>> m_vDecoerOutputAttrs;

        /* 模型缓存 */
        std::vector<Ort::Value> vStates;
        std::vector<float> vParaformerFeatCache;
        std::vector<float> vParaformerAlphaCache;
        std::vector<float> vParaformerEncoderOutCache;
    };

} // namespace Inference_NS
