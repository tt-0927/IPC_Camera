/**
 * @file ZipFormer.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-07
 *
 * @brief
 */
#pragma once

#include <fstream>
#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "ZipFormerEncode.hpp"
#include "ZipFormerDecode.hpp"
#include "ZipFormerJoiner.hpp"
namespace Inference_NS
{
    typedef struct _VocabEntry_
    {
        int index;
        char *token;
    } VocabEntry_S;

    class CZipFormer
    {
    public:
        CZipFormer(
            std::string strEncodeModelPath,
            std::string strDecodeModelPath,
            std::string strJoinercodeModelPath,
            std::string strVocabPath);

        ~CZipFormer();

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
         * @param [int&] nSampleRate: 模型输入的音频采样率
         * @param [int&] nMels: 特征向量个数
         * @param [int&] nSegment: 特征向量维度
         * @param [int&] nEncoderOutputT: 解码输出的特征维度
         * @return [*]
         * @note
         */
        bool getInputLimit(
            int& nSampleRate, int& nMels, int& nSegment, int& nEncoderOutputT
        );

        /**
         * @brief 特征提取
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ASRData_S>&] stASRData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(
            Inference_NS::InputData_S stInputData,
            Inference_NS::ASRData_S &stASRData);

    private:
        int argmax(float *array);
        bool readVocab(const char *fileName);
        void replace_substr(std::string &str, const std::string &from, const std::string &to);
        float convertSampleToFloat(const int8_t* pchSampleData, int nBitDepth);
        bool convertPCMToFloat(
            const std::vector<int8_t>& vfPcmData,
            int                        nBitDepth,
            int                        nChannels,
            bool                       bToMono,
            std::vector<float>&        vfOutData);

    private:
        std::string m_strEncodeConfigPath;          /* 编码模型json配置路径 */
        std::string m_strDecodeConfigPath;          /* 解码模型json配置路径 */
        std::string m_strJoinercodeConfigPath;      /* 链接模型json配置路径 */
        CZipFormerEncode *m_pEncodeModel = nullptr; /* 模型操作句柄 */
        CZipFormerDecode *m_pDecodeModel = nullptr; /* 模型操作句柄 */
        CZipFormerJoiner *m_pJoinerModel = nullptr; /* 模型操作句柄 */

        std::string m_strVocabPath;
        std::vector<VocabEntry_S> m_vVocab;

        /* 配置信息 */
        int m_nNumProcessedFrames = 0;
        int m_nSampleRate = 16000;
        int m_nMels = 80;
        int m_nSegment = 103;
        int m_nEncoderOutputT = 24;

        int m_nDecoderDim = 512;
        int m_nDecoderSize = 2;
        int m_nJonerOutputSize = 6254;

        /* 词表 */
        int m_nBLANKID = 0;
        int m_nUNKID = 2;

    };

} // namespace Inference_NS
