/**
 * @file ZipFormer.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-07
 *
 * @brief
 */
#include "ZipFormer.hpp"
#include <memory>
#include <cstring>
#include <algorithm>

#include <fstream>
#include <cstdint>

Inference_NS::CZipFormer::CZipFormer(
    std::string strEncodeConfigPath,
    std::string strDecodeConfigPath,
    std::string strJoinercodeConfigPath,
    std::string strVocabPath) : m_strEncodeConfigPath(strEncodeConfigPath),
                                m_strDecodeConfigPath(strDecodeConfigPath),
                                m_strJoinercodeConfigPath(strJoinercodeConfigPath),
                                m_strVocabPath(strVocabPath)
{
}

Inference_NS::CZipFormer::~CZipFormer()
{
    unInit();
}

/* 初始化 */
bool Inference_NS::CZipFormer::init()
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
        m_pEncodeModel = new CZipFormerEncode(m_strEncodeConfigPath);

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
        m_pDecodeModel = new CZipFormerDecode(m_strDecodeConfigPath);

        if (m_pDecodeModel)
        {
            if (!m_pDecodeModel->init())
            {
                bModelInit = false;
            }
        }
    }

    if (!m_pJoinerModel)
    {
        /* 初始化模型 */
        m_pJoinerModel = new CZipFormerJoiner(m_strJoinercodeConfigPath);

        if (m_pJoinerModel)
        {
            if (!m_pJoinerModel->init())
            {
                bModelInit = false;
            }
        }
    }

    /* 获取相关的模型信息 */
    if (bModelInit)
    {
        m_nMels = m_pEncodeModel->m_vInputAttrs[0].dims[2];
        m_nSegment = m_pEncodeModel->m_vInputAttrs[0].dims[1];
        m_nEncoderOutputT = m_pEncodeModel->m_vOutputAttrs[0].dims[1];
        m_nDecoderSize = m_pDecodeModel->m_vInputAttrs[0].dims[1];
        m_nDecoderDim = m_pDecodeModel->m_vOutputAttrs[0].dims[1];
        m_nJonerOutputSize = m_pJoinerModel->m_vOutputAttrs[0].dims[1];
    }
    return bModelInit;
}

/* 反初始化 */
bool Inference_NS::CZipFormer::unInit()
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
    if (m_pJoinerModel)
    {
        delete m_pJoinerModel;
        m_pJoinerModel = nullptr;
    }
    return true;
}

/* 获取输入音频限制 */
bool Inference_NS::CZipFormer::getInputLimit(
    int &nSampleRate, int &nMels, int &nSegment, int &nEncoderOutputT)
{
    if (!m_pEncodeModel || !m_pDecodeModel || !m_pJoinerModel)
    {
        return false;
    }

    nSampleRate = m_nSampleRate;
    nMels = m_nMels;
    nSegment = m_nSegment;
    nEncoderOutputT = m_nEncoderOutputT;
    return true;
}

/* 推理数据 */
bool Inference_NS::CZipFormer::inference(
    Inference_NS::InputData_S stInputData,
    Inference_NS::ASRData_S &stASRData)
{
    if (!m_pEncodeModel || !m_pDecodeModel || !m_pJoinerModel)
    {
        return false;
    }
    stASRData.vTexts.clear();
    stASRData.vTimestamp.clear();

    float *pEncoderInput = (float *)m_pEncodeModel->m_pInputs[0].buf;
    float *pEncoderOutput = (float *)m_pEncodeModel->m_pOutputs[0].buf;
    int64_t *pHyp = (int64_t *)m_pDecodeModel->m_pInputs[0].buf;
    float *pDecoderOutput = (float *)m_pDecodeModel->m_pOutputs[0].buf;
    float *pJoinerOutput = (float *)m_pJoinerModel->m_pOutputs[0].buf;

    if (stInputData.nDataSize != m_nSegment * m_nMels)
    {
        printf("输入特征大小[%d]不等于模型输入[%d]\n", stInputData.nDataSize, m_nSegment * m_nMels);
        return false;
    }
    memcpy(pEncoderInput, stInputData.pData, m_nSegment * m_nMels * sizeof(float));

    /* 设置输入结构 */
    if (!m_pEncodeModel->inference())
    {
        printf("m_pEncodeModel 推理失败\n");
        return false;
    }

    /* 解码模型 */
    if (m_nNumProcessedFrames == 0)
    {
        /* 推理 */
        if (!m_pDecodeModel->inference())
        {
            printf("m_pDecodeModel 推理失败\n");
            return false;
        }
        m_nNumProcessedFrames++;
    }

    /* 连接模型 */
    for (int i = 0; i < m_nEncoderOutputT; i++)
    {
        float *cur_encoder_output = pEncoderOutput + i * m_nDecoderDim;
        memcpy(m_pJoinerModel->m_pInputs[0].buf, cur_encoder_output, m_pJoinerModel->m_vInputAttrs[0].n_elems * sizeof(float));
        memcpy(m_pJoinerModel->m_pInputs[1].buf, pDecoderOutput, m_pJoinerModel->m_vInputAttrs[1].n_elems * sizeof(float));
        /* 推理 */
        if (!m_pJoinerModel->inference())
        {
            printf("m_pJoinerModel 推理失败\n");
            return false;
        }

        int nNextToken = argmax(pJoinerOutput);
        if (nNextToken != m_nBLANKID && nNextToken != m_nUNKID)
        {

            for (int j = 0; j < m_nDecoderSize - 1; j++)
            {
                pHyp[j] = pHyp[j + 1];
            }
            pHyp[m_nDecoderSize - 1] = (int64_t)nNextToken;
            std::string next_token_str = m_vVocab[nNextToken].token;
            replace_substr(next_token_str, "▁", " ");
            stASRData.vTexts.push_back(next_token_str);
            stASRData.vTimestamp.push_back(i);
            /* 推理 */
            if (!m_pDecodeModel->inference())
            {
                printf("m_pDecodeModel 推理失败\n");
                return false;
            }
        }
    }
    return true;
}

/* 获取最大值的索引 */
int Inference_NS::CZipFormer::argmax(float *array)
{
    int start_index = 0;
    int max_index = start_index;
    float max_value = array[max_index];
    for (int i = start_index + 1; i < start_index + m_nJonerOutputSize; i++)
    {
        if (array[i] > max_value)
        {
            max_value = array[i];
            max_index = i;
        }
    }
    int relative_index = max_index - start_index;
    return relative_index;
}

/* 读取词表 */
bool Inference_NS::CZipFormer::readVocab(const char *fileName)
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

/* 替换子字符串 */
void Inference_NS::CZipFormer::replace_substr(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
        return; // Prevent infinite loop if 'from' is empty
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Advance position by length of the replacement
    }
}
