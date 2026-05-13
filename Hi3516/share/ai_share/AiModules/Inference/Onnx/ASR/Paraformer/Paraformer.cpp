/**
 * @file Paraformer.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */

#include "Paraformer.hpp"
#include <chrono>
#include <cctype>    // std::isspace
#include <stdexcept> // std::invalid_argument
#include <cstring>

#include <fstream>
#include <iostream>

Inference_NS::CParaformer::CParaformer(
    std::string strEncoderConfigPath,
    std::string strDecoderConfigPath,
    std::string strVocabPath) : m_strEncoderConfigPath(strEncoderConfigPath),
                                m_strDecoderConfigPath(strDecoderConfigPath),
                                m_strVocabPath(strVocabPath)
{
}

Inference_NS::CParaformer::~CParaformer()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CParaformer::init()
{
    bool bModelInit = true;
    /* 读取词表 */
    bool bRet = readVocab(m_strVocabPath.c_str());
    if (!bRet)
    {
        printf("词表[%s]读取失败\n", m_strVocabPath.c_str());
        bModelInit = false;
    }

    if (!m_pEncodeModel)
    {
        /* 初始化模型 */
        m_pEncodeModel = new CParaformerEncoder(m_strEncoderConfigPath);

        if (m_pEncodeModel)
        {
            if (!m_pEncodeModel->init())
            {
                bModelInit = false;
            }
        }
    }
    if (!m_pDecodeModel)
    {
        /* 初始化模型 */
        m_pDecodeModel = new CParaformerDecoder(m_strDecoderConfigPath);

        if (m_pDecodeModel)
        {
            if (!m_pDecodeModel->init())
            {
                bModelInit = false;
            }
        }
    }

    if (bModelInit)
    {
        /* 获取内存管理类 */
        m_pEncodeModel->getAllocatorWithDefaultOptions(m_stEncodeAllocator);
        m_pDecodeModel->getAllocatorWithDefaultOptions(m_stDecodeAllocator);

        /* 获取网络的形状 */
        m_pEncodeModel->getAttrs(m_vEncoerInputAttrs, m_vEncoerOutputAttrs);
        m_pDecodeModel->getAttrs(m_vDecoerInputAttrs, m_vDecoerOutputAttrs);

        /* 获取网络内部数据信息 */
        std::string strVocabSize = m_pEncodeModel->getMetadata("vocab_size");
        std::string strLfrWindowSize = m_pEncodeModel->getMetadata("lfr_window_size");
        std::string strLfrWindowShift = m_pEncodeModel->getMetadata("lfr_window_shift");
        std::string strEncoderOutputSize = m_pEncodeModel->getMetadata("encoder_output_size");
        std::string strDecoderNumBlocks = m_pEncodeModel->getMetadata("decoder_num_blocks");
        std::string strDecoderKernelSize = m_pEncodeModel->getMetadata("decoder_kernel_size");
        std::string strNegMean = m_pEncodeModel->getMetadata("neg_mean");
        std::string strInvStddev = m_pEncodeModel->getMetadata("inv_stddev");
        /* 字符串切割 */
        bool bMetaFlag = true;
        bMetaFlag &= stringToInt(strVocabSize, m_nVocabSize);
        bMetaFlag &= stringToInt(strLfrWindowSize, m_nLfrWindowSize);
        bMetaFlag &= stringToInt(strLfrWindowShift, m_nLfrWindowShift);
        bMetaFlag &= stringToInt(strEncoderOutputSize, m_nEncoderOutputSize);
        bMetaFlag &= stringToInt(strDecoderNumBlocks, m_nDecoderNumBlocks);
        bMetaFlag &= stringToInt(strDecoderKernelSize, m_nDecoderKernelSize);
        bMetaFlag &= stringToFloatVecotr(strNegMean, m_vNegMean);
        bMetaFlag &= stringToFloatVecotr(strInvStddev, m_vInvStddev);
        if (!bMetaFlag)
        {
            printf("获取模型内部信息失败\n");
            return false;
        }
        float fScale = std::sqrt(m_nEncoderOutputSize);
        for (auto &f : m_vInvStddev)
        {
            f *= fScale;
        }
    }
    return bModelInit;
}

/* 反初始化 */
bool Inference_NS::CParaformer::unInit()
{
    if (m_pEncodeModel)
    {
        delete m_pEncodeModel;
        m_pEncodeModel = nullptr;
    }
    if (m_pDecodeModel)
    {
        delete m_pDecodeModel;
        m_pDecodeModel = nullptr;
    }
    return true;
}

/*  y[i] += x[i] * scale */
static void scaleAddInPlace(const float *x, int32_t n, float scale, float *y)
{
    for (int32_t i = 0; i != n; ++i)
    {
        y[i] += x[i] * scale;
    }
}
/*  y[i] = x[i] * scale */
static void scale(const float *x, int32_t n, float scale, float *y)
{
    for (int32_t i = 0; i != n; ++i)
    {
        y[i] = x[i] * scale;
    }
}

/* 从文件加载数据 */
std::vector<float> LoadFramesFromFile(const std::string &filename)
{
    std::ifstream in_file(filename, std::ios::binary);
    if (!in_file.is_open())
    {
        std::cerr << "Error: Failed to open " << filename << " for reading.\n";
        return {};
    }

    size_t size = 0;
    in_file.read(reinterpret_cast<char *>(&size), sizeof(size));
    std::vector<float> frames(size);
    in_file.read(reinterpret_cast<char *>(frames.data()), size * sizeof(float));

    if (in_file.gcount() != static_cast<std::streamsize>(size * sizeof(float)))
    {
        std::cerr << "Error: Incomplete read from " << filename << ".\n";
        return {};
    }
    in_file.close();
    return frames;
}

/* 推理数据 */
bool Inference_NS::CParaformer::inference(
    Inference_NS::AVInputData_S stInputData,
    Inference_NS::ASRData_S &stASRData)
{
    if (!m_pEncodeModel || !m_pDecodeModel || stInputData.vFeature.size() == 0)
    {
        return false;
    }
    stInputData.vFeature = applyLFR(stInputData.vFeature);
    applyCMVN(&stInputData.vFeature);
    positionalEncoding(&stInputData.vFeature, stInputData.nFrameIndex / m_nLfrWindowShift);

    int32_t nFeatDim = m_vNegMean.size();
    /* add overlap chunk */
    if (vParaformerFeatCache.empty())
    {
        int32_t n = (nLeftChunkSize + nRightChunkSize) * nFeatDim;
        vParaformerFeatCache.resize(n, 0);
    }

    stInputData.vFeature.insert(stInputData.vFeature.begin(), vParaformerFeatCache.begin(), vParaformerFeatCache.end());
    std::copy(stInputData.vFeature.end() - vParaformerFeatCache.size(), stInputData.vFeature.end(),
              vParaformerFeatCache.begin());
    int32_t nNumFrames = stInputData.vFeature.size() / nFeatDim;

    /* Encoder推理 */
    std::vector<Ort::Value> vEncoderInputs;
    std::vector<Ort::Value> vEncoderOutputs;
    Ort::MemoryInfo stMemoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    /* 第一个输入 */
    std::array<int64_t, 3> aXShape{1, nNumFrames, nFeatDim};
    Ort::Value oX = Ort::Value::CreateTensor(stMemoryInfo, stInputData.vFeature.data(), stInputData.vFeature.size(),
                                             aXShape.data(), aXShape.size());
    vEncoderInputs.push_back(std::move(oX));
    /* 第二个输入 */
    int64_t nXLenShape = 1;
    int32_t NXLenVal = nNumFrames;
    Ort::Value oXLength = Ort::Value::CreateTensor(stMemoryInfo, &NXLenVal, 1, &nXLenShape, 1);
    vEncoderInputs.push_back(std::move(oXLength));
    if (!m_pEncodeModel->inference(
            vEncoderInputs,
            vEncoderOutputs))
    {
        printf("Encoder推理失败\n");
        return false;
    }

    /* Decoder */
    /* CIF search */
    Ort::Value &oEncoderOut = vEncoderOutputs[0];
    Ort::Value &oEncoderOutLen = vEncoderOutputs[1];
    Ort::Value &oAlpha = vEncoderOutputs[2];

    float *pAlpha = oAlpha.GetTensorMutableData<float>();
    std::vector<int64_t> vAlphaShape =
        oAlpha.GetTensorTypeAndShapeInfo().GetShape();
    std::fill(pAlpha, pAlpha + nLeftChunkSize, 0);
    std::fill(pAlpha + vAlphaShape[1] - nRightChunkSize,
              pAlpha + vAlphaShape[1], 0);

    const float *pEncoderOut = oEncoderOut.GetTensorData<float>();
    std::vector<int64_t> nEncoderOutShape = oEncoderOut.GetTensorTypeAndShapeInfo().GetShape();
    /* 相关变量初始化 */
    if (vParaformerEncoderOutCache.empty())
    {
        vParaformerEncoderOutCache.resize(nEncoderOutShape[2]);
    }
    if (vParaformerAlphaCache.empty())
    {
        vParaformerAlphaCache.resize(1);
    }

    std::vector<float> vAcousticEmbedding;
    vAcousticEmbedding.reserve(nEncoderOutShape[1] * nEncoderOutShape[2]);

    float fIntegrate = vParaformerAlphaCache[0];
    for (int32_t i = 0; i != nEncoderOutShape[1]; ++i)
    {
        float fThisAlpha = pAlpha[i];
        if (fIntegrate + fThisAlpha < m_fAlphaThreshold)
        {
            fIntegrate += fThisAlpha;
            scaleAddInPlace(pEncoderOut + i * nEncoderOutShape[2],
                            nEncoderOutShape[2], fThisAlpha,
                            vParaformerEncoderOutCache.data());
            continue;
        }

        /* fire */
        scaleAddInPlace(pEncoderOut + i * nEncoderOutShape[2],
                        nEncoderOutShape[2], m_fAlphaThreshold - fIntegrate,
                        vParaformerEncoderOutCache.data());
        vAcousticEmbedding.insert(vAcousticEmbedding.end(),
                                  vParaformerEncoderOutCache.begin(), vParaformerEncoderOutCache.end());
        fIntegrate += fThisAlpha - m_fAlphaThreshold;
        scale(pEncoderOut + i * nEncoderOutShape[2], nEncoderOutShape[2],
              fIntegrate, vParaformerEncoderOutCache.data());
    }

    vParaformerAlphaCache[0] = fIntegrate;
    if (vAcousticEmbedding.empty())
    {
        return true;
    }
    if (vStates.empty())
    {
        vStates.reserve(m_nDecoderNumBlocks);
        std::array<int64_t, 3> vStatesShape{1, m_nEncoderOutputSize,
                                            m_nDecoderKernelSize - 1};
        const int32_t nNumBytes = sizeof(float) * vStatesShape[0] * vStatesShape[1] * vStatesShape[2];
        for (int32_t i = 0; i != m_nDecoderNumBlocks; ++i)
        {
            Ort::Value oOneState = Ort::Value::CreateTensor<float>(
                m_stEncodeAllocator, vStatesShape.data(), vStatesShape.size());
            memset(oOneState.GetTensorMutableData<float>(), 0, nNumBytes);
            vStates.push_back(std::move(oOneState));
        }
    }

    /* Decoder推理 */
    std::vector<Ort::Value> vDecoderInputs;
    vDecoderInputs.reserve(m_vDecoerInputAttrs.size());
    std::vector<Ort::Value> vDecoderOutputs;

    vDecoderInputs.push_back(std::move(oEncoderOut));
    vDecoderInputs.push_back(std::move(oEncoderOutLen));

    int32_t nNumTokens = vAcousticEmbedding.size() / vParaformerEncoderOutCache.size();
    std::array<int64_t, 3> aAcousticEmbeddingShape{
        1, nNumTokens, static_cast<int32_t>(vParaformerEncoderOutCache.size())};
    Ort::Value oAcousticEmbeddingTensor = Ort::Value::CreateTensor(
        stMemoryInfo, vAcousticEmbedding.data(), vAcousticEmbedding.size(),
        aAcousticEmbeddingShape.data(), aAcousticEmbeddingShape.size());
    vDecoderInputs.push_back(std::move(oAcousticEmbeddingTensor));

    std::array<int64_t, 1> aAcousticEmbeddingLengthShape{1};
    Ort::Value oAcousticEmbeddingLengthTensor = Ort::Value::CreateTensor(
        stMemoryInfo, &nNumTokens, 1, aAcousticEmbeddingLengthShape.data(),
        aAcousticEmbeddingLengthShape.size());
    vDecoderInputs.push_back(std::move(oAcousticEmbeddingLengthTensor));

    for (auto &v : vStates)
    {
        vDecoderInputs.push_back(std::move(v));
    }
    if (!m_pDecodeModel->inference(vDecoderInputs, vDecoderOutputs))
    {
        printf("Decoder推理失败\n");
        return false;
    }

    // vStates.clear();
    // vStates.reserve(m_nDecoderNumBlocks);
    for (int32_t i = 2; i != vDecoderOutputs.size(); ++i)
    {
        vStates[i - 2] = std::move(vDecoderOutputs[i]);
        // vStates.push_back(std::move(vDecoderOutputs[i]));
    }

    const auto &nSampleIds = vDecoderOutputs[1];
    const int64_t *pNSampleIds = nSampleIds.GetTensorData<int64_t>();

    for (int32_t i = 0; i != nNumTokens; ++i)
    {
        int32_t t = pNSampleIds[i];
        if (t == 0)
        {
            continue;
        }
        stASRData.vTexts.push_back(m_vVocab[t].token);
    }

    return true;
}

/* @brief 清除上一句的Cache */
bool Inference_NS::CParaformer::clearCache()
{
    vStates.clear();
    vParaformerFeatCache.clear();
    vParaformerAlphaCache.clear();
    vParaformerEncoderOutCache.clear();
    return true;
}

/* 降帧操作 */
std::vector<float> Inference_NS::CParaformer::applyLFR(const std::vector<float> &vInputData)
{
    int32_t nInNumFrames = vInputData.size() / m_nInFeatDim;
    int32_t nOutNumFrames = (nInNumFrames - m_nLfrWindowSize) / m_nLfrWindowShift + 1;
    int32_t nOutFeatDim = m_nInFeatDim * m_nLfrWindowSize;

    std::vector<float> vOutData(nOutNumFrames * nOutFeatDim);

    const float *pIn = vInputData.data();
    float *pOut = vOutData.data();
    for (int32_t i = 0; i != nOutNumFrames; ++i)
    {
        std::copy(pIn, pIn + nOutFeatDim, pOut);
        pOut += nOutFeatDim;
        pIn += m_nLfrWindowShift * m_nInFeatDim;
    }

    return vOutData;
}
/* 归一化 */
void Inference_NS::CParaformer::applyCMVN(std::vector<float> *pInData)
{
    int32_t nDim = m_vNegMean.size();
    int32_t nNumFrames = pInData->size() / nDim;

    float *p = pInData->data();
    for (int32_t i = 0; i != nNumFrames; ++i)
    {
        for (int32_t k = 0; k != nDim; ++k)
        {
            p[k] = (p[k] + m_vNegMean[k]) * m_vInvStddev[k];
        }

        p += nDim;
    }
}
/* 位置编码 */
void Inference_NS::CParaformer::positionalEncoding(std::vector<float> *pInData, int32_t nTOffset)
{
    int32_t nFeatDim = m_nInFeatDim * m_nLfrWindowSize;
    int32_t T = pInData->size() / nFeatDim;

    constexpr float kScale = -0.03301197265941284;

    for (int32_t t = 0; t != T; ++t)
    {
        float *p = pInData->data() + t * nFeatDim;
        int32_t offset = t + 1 + nTOffset;

        for (int32_t d = 0; d < nFeatDim / 2; ++d)
        {
            float inv_timescale = offset * std::exp(d * kScale);
            float sin_d = std::sin(inv_timescale);
            float cos_d = std::cos(inv_timescale);
            p[d] += sin_d;
            p[d + nFeatDim / 2] += cos_d;
        }
    }
}

/* 字符串转为整形 */
bool Inference_NS::CParaformer::stringToInt(std::string strData, int &nOutData)
{
    if (strData.empty())
    {
        printf("输入字符串为空\n");
        return false;
    }

    nOutData = atoi(strData.c_str());
    if (nOutData < 0)
    {
        printf("输入的数据nOutData[%d]<0", nOutData);
        return false;
    }
    return true;
}
/* 去除字符串首尾空格（辅助函数） */
std::string Inference_NS::CParaformer::trim(const std::string &strInData)
{
    size_t nStart = strInData.find_first_not_of(" \t\r\n");
    if (nStart == std::string::npos)
    {
        return "";
    }
    size_t nEnd = strInData.find_last_not_of(" \t\r\n");
    return strInData.substr(nStart, nEnd - nStart + 1);
}
/* 字符串切割，转为std::vector<float> */
bool Inference_NS::CParaformer::stringToFloatVecotr(std::string strData, std::vector<float> &vOutData)
{
    /* 核心逻辑：分割字符串 → 转换为 float → 存入 vector */
    std::vector<std::string> vTokens;
    size_t nStart = 0, nEnd = 0;
    while ((nEnd = strData.find(',', nStart)) != std::string::npos)
    {
        vTokens.push_back(strData.substr(nStart, nEnd - nStart));
        nStart = nEnd + 1;
    }
    vTokens.push_back(strData.substr(nStart));
    /* 转换并存储为 float */

    for (auto &token : vTokens)
    {
        /* 跳过空字符串 */
        if (token.empty())
        {
            printf("存在空数据\n");
            continue;
        }
        /* 去除首尾空格 */
        std::string strTrimmed = trim(token);
        if (strTrimmed.empty())
        {
            printf("存在纯空格数据\n");
            continue;
        }
        try
        {
            /* 转换为 float 并存储 */
            vOutData.push_back(std::stof(strTrimmed));
        }
        catch (const std::invalid_argument &e)
        {
            printf("转换失败: %s 不是有效浮点数\n", strTrimmed.c_str());
            return false;
        }
        catch (const std::out_of_range &e)
        {
            printf("处理超出范围的值\n");
            return false;
        }
    }
    return true;
}

/* 读取词表 */
bool Inference_NS::CParaformer::readVocab(const char *fileName)
{
    FILE *fp;
    char line[512];

    fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return false;
    }

    int count = 0;
    while (fgets(line, sizeof(line), fp))
    {
        VocabEntry_S stOneToken;
        stOneToken.index = atoi(strchr(line, ' ') + 1); /* get token before the first space */
        char *token = strtok(line, " ");
        stOneToken.token = strdup(token); /* Get index after the first space */
        m_vVocab.push_back(stOneToken);
        count++;
    }

    fclose(fp);

    return true;
}
