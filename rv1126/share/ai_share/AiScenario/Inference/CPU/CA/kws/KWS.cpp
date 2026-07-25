/*
 * @FilePath     : KWS.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-06-05 10:44:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-06-07 09:15:21
 * @Description  :
 */
#include "KWS.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>


using namespace InferenceV1_0_NS;

CKWS::CKWS(std::string strTokens,
           std::string strEncoder,
           std::string strDecoder,
           std::string strJoiner,
           std::string strKeywordsFile)
    : m_strTokens(strTokens),
      m_strEncoder(strEncoder),
      m_strDecoder(strDecoder),
      m_strJoiner(strJoiner),
      m_strKeywordsFile(strKeywordsFile),
      m_po("")
{
}

/* 初始化 */
bool CKWS::init()
{
    /* 模拟命令行参数 */
    std::string strArgsTokens       = "--tokens=" + m_strTokens;
    std::string strArgsEncoder      = "--encoder=" + m_strEncoder;
    std::string strArgsDecoder      = "--decoder=" + m_strDecoder;
    std::string strArgsJoiner       = "--joiner=" + m_strJoiner;
    std::string strArgsKeywordsFile = "--keywords-file=" + m_strKeywordsFile;

    const char* pchArgv[] = {
        "./keyword-spotter-alsa",
        strArgsTokens.c_str(),
        strArgsEncoder.c_str(),
        strArgsDecoder.c_str(),
        strArgsJoiner.c_str(),
        strArgsKeywordsFile.c_str(),
        "plughw:1,0"
    };


    int nArgc = sizeof(pchArgv) / sizeof(pchArgv[0]);

    m_config.Register(&m_po);
    m_po.Read(nArgc, pchArgv);
    if (!m_config.Validate())
    {
        fprintf(stderr, "Errors in m_config!\n");
        return false;
    }
    m_pSpotter = std::make_unique<sherpa_onnx::KeywordSpotter>(m_config);
    m_pStream  = m_pSpotter->CreateStream();

    return true;
}

/* 反初始化 */
bool InferenceV1_0_NS::CKWS::unInit()
{
    return true;
}

/* 推理数据 */
bool InferenceV1_0_NS::CKWS::inference(AiScenario_NS::CAData_S stInData, std::string& strOutData)
{
    strOutData.clear();

    if (!m_pSpotter || !m_pStream)
    {
        fprintf(stderr, "推理失败-模型未初始化或者初始化失败\n");
        return false;
    }

    if (!stInData.pData || stInData.nDataSize <= 0)
    {
        fprintf(stderr, "推理失败-传入参数为空\n");
        return false;
    }

    std::vector<int8_t> vfInData(stInData.pData, stInData.pData + stInData.nDataSize);
    std::vector<float>  vfOutData;

    /* 转换数据 */
    convertPCMToFloat(vfInData, stInData.nDepth, stInData.nChannel, true, vfOutData);

    m_pStream->AcceptWaveform(stInData.nSample, vfOutData.data(), vfOutData.size());
    while (m_pSpotter->IsReady(m_pStream.get()))
    {
        m_pSpotter->DecodeStream(m_pStream.get());
    }

    const auto result = m_pSpotter->GetResult(m_pStream.get());
    if (!result.keyword.empty())
    {
        strOutData = result.keyword;
    }

    return true;
}

/* 辅助函数：将给定位深度的样本转换为浮点数 */
float InferenceV1_0_NS::CKWS::convertSampleToFloat(const int8_t* pchSampleData, int nBitDepth)
{
    int32_t nSample = 0;
    switch (nBitDepth)
    {
        case 8:
        {
            nSample = static_cast<int8_t>(*pchSampleData);

            /* 128.0f 是 2^7 */
            return nSample / 128.0f;
        }

        case 16:
        {
            nSample = static_cast<int16_t>(
                (static_cast<uint8_t>(pchSampleData[1]) << 8) |
                static_cast<uint8_t>(pchSampleData[0]));

            /* 32768.0f 是 2^15 */
            return nSample / 32768.0f;
        }

        case 24:
        {
            nSample = (static_cast<int32_t>(static_cast<uint8_t>(pchSampleData[2])) << 16) |
                (static_cast<int32_t>(static_cast<uint8_t>(pchSampleData[1])) << 8) |
                static_cast<int32_t>(static_cast<uint8_t>(pchSampleData[0]));

            /* 对于24位PCM数据，进行符号扩展 */
            if (nSample & 0x800000)
            {
                nSample |= 0xFF000000;
            }

            /* 8388608.0f 是 2^23 */
            return nSample / 8388608.0f;
        }

        default:
        {
            printf("不支持的位深度, [%d]\n", nBitDepth);
            return 0.0f;
        }
    }
}

/* 主转换函数：将PCM数据转换为浮点数据 */
bool InferenceV1_0_NS::CKWS::convertPCMToFloat(
    const std::vector<int8_t>& vfPcmData,
    int                        nBitDepth,
    int                        nChannels,
    bool                       bToMono,
    std::vector<float>&        vfOutData)
{
    /* 检查位深度是否支持，以及输入大小是否有效 */
    if (nBitDepth != 8 && nBitDepth != 16 && nBitDepth != 24)
    {
        printf("不支持的位深度, [%d]\n", nBitDepth);
        return false;
    }

    size_t nSampleSize = nBitDepth / 8;
    if (vfPcmData.size() % (nSampleSize * nChannels) != 0)
    {
        printf("输入数据大小无效, [%ld]\n", vfPcmData.size());
        return false;
    }

    size_t nNumSamples    = vfPcmData.size() / (nSampleSize * nChannels);
    size_t nFloatDataSize = bToMono ? nNumSamples : vfPcmData.size() / nSampleSize;
    vfOutData.reserve(nFloatDataSize);

    for (size_t i = 0; i < nNumSamples; ++i)
    {
        float fSampleSum = 0.0f;

        for (int c = 0; c < nChannels; ++c)
        {
            size_t index            = (i * nChannels + c) * nSampleSize;
            float  normalizedSample = convertSampleToFloat(&vfPcmData[index], nBitDepth);

            if (bToMono)
            {
                fSampleSum += normalizedSample;
            }
            else
            {
                vfOutData.push_back(normalizedSample);
            }
        }

        if (bToMono)
        {
            vfOutData.push_back(fSampleSum / nChannels);
        }
    }

    return true;
}