/**
 * @file SenceVoice.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-06
 *
 * @brief
 */
#include "SenceVoice.hpp"
#include <sstream>
#include <memory>
#include <iostream>
#include <cstring>
#include <numeric>
#include <algorithm>

#include <vector>
#include <fstream>
#include <stdexcept>

// 读取整个 bin 文件到 vector<float>
std::vector<float> LoadFloatVector(const std::string &path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
        throw std::runtime_error("cannot open " + path);

    // 求文件长度
    ifs.seekg(0, std::ios::end);
    size_t bytes = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    if (bytes % sizeof(float) != 0)
        throw std::runtime_error("file size is not aligned to float");

    std::vector<float> vec(bytes / sizeof(float));
    ifs.read(reinterpret_cast<char *>(vec.data()), bytes);
    if (!ifs)
        throw std::runtime_error("read failed");
    return vec;
}

Inference_NS::CSenceVoice::CSenceVoice(std::string strConfigPath)
    : CAVInferenceRK(strConfigPath)
{
}

Inference_NS::CSenceVoice::~CSenceVoice()
{
}

/* 设置识别的语言 */
bool Inference_NS::CSenceVoice::setLanguage(int nLanguage, bool bTextNormalization)
{
    const std::vector<int> kLangs = {0, 3, 4, 7, 11, 12, 13};
    bool bOk = std::any_of(kLangs.begin(), kLangs.end(),
                           [&](int v)
                           { return v == nLanguage; });
    if (!bOk)
    {
        printf("[%d] 不在 {0,3,4,7,11,12,13}语言下标范围内\n", nLanguage);
        return false;
    }
    m_vPrompt[0] = nLanguage;
    m_vPrompt[3] = bTextNormalization ? 14 : 15;

    return true;
}

/* 推理数据 */
bool Inference_NS::CSenceVoice::inference(
    Inference_NS::AVInputData_S stInputData,
    Inference_NS::ASRData_S &stASRData)
{
    if (!m_pModel || stInputData.vFeature.size() == 0)
    {
        return false;
    }
    stASRData.vTexts.clear();

    /* 预处理 */
    stInputData.vFeature = applyLFR(stInputData.vFeature);

    if (stInputData.vFeature.size() * sizeof(float) > m_pInputs[0].size)
    {
        printf("输入数据[%ld]大于模型需要输入的数据长度[%d]\n", stInputData.vFeature.size() * sizeof(float), m_pInputs[0].size);
        return false;
    }

    memset(m_pInputs[0].buf, 0, m_pInputs[0].size);
    memcpy(m_pInputs[0].buf, (float *)stInputData.vFeature.data(), stInputData.vFeature.size() * sizeof(float));
    memset(m_pInputs[1].buf, 0, m_pInputs[1].size);
    memcpy(m_pInputs[1].buf, (int32_t *)m_vPrompt.data(), m_vPrompt.size() * sizeof(int32_t));

    /* 运行 */
    if (!m_pModel->run(m_pInputs,
                       m_vInputAttrs.size(),
                       m_pOutputs,
                       m_vOutputAttrs.size()))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }
    float *p_logit = (float *)m_pOutputs[0].buf;
    for (int j = 0; j < m_vOutputAttrs[0].dims[1]; ++j)
    {
        /* lambda 访问某一列的元素 */
        int nDim2 = m_vOutputAttrs[0].dims[2];
        auto columnAccessor = [p_logit, nDim2, j](int row)
        {
            return p_logit[j * nDim2 + row];
        };

        /* 行索引 [0, a) */
        std::vector<int> rowIndices(m_vOutputAttrs[0].dims[2]);
        std::iota(rowIndices.begin(), rowIndices.end(), 0);

        /* 找该列最大值对应的行号 */
        auto maxIt = std::max_element(
            rowIndices.begin(), rowIndices.end(),
            [&columnAccessor](int r1, int r2)
            {
                return columnAccessor(r1) < columnAccessor(r2);
            });

        int y = *maxIt;
        if (m_vVocab[y].index == 0)
        {
            continue;
        }
        stASRData.vTexts.push_back(m_vVocab[y].token);
        stASRData.vTimestamp.push_back(j * 1.0 / m_vOutputAttrs[0].dims[1]);
        // printf("%s",m_vVocab[y].token);
    }
    printf("\n");
    return true;
}

/* 降帧操作 */
std::vector<float> Inference_NS::CSenceVoice::applyLFR(const std::vector<float> &vInputData)
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

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CSenceVoice::checkModelProConfig()
{
    /* 读取json文件 */
    std::ifstream File(m_strConfigPath);
    if (!File)
    {
        printf("[无法打开json文件]: %s\n", m_strConfigPath.c_str());
        return false;
    }
    /* 读取文件内容到 std::string */
    std::string strJson((std::istreambuf_iterator<char>(File)),
                        std::istreambuf_iterator<char>());
    const char *pchJson = strJson.c_str();
    /* 解析Json数据-获取错误 */
    if (NULL == pchJson)
    {
        printf("传入参数异常\n");
        return false;
    }

    Json::Object *pJsonHandle = NULL;
    Json::Object *pJsonData = NULL;
    bool bRet = true;
    bool bMetaFlag = true;

    pJsonHandle = Json::init(pchJson);

    pJsonData = Json::get(pJsonHandle, "post_precess");
    if (!pJsonData)
    {
        printf("解析[data]字段失败\n");
        bRet = false;
        goto EXIT;
    }
    /* 配置参数 */
    bRet = Json::get(pJsonData, "vocab_path", m_strVocabPath);
    if (!bRet)
    {
        printf("解析vocab_path字段失败\n");
        goto EXIT;
    }
    bRet = Json::get(pJsonData, "vocab_size", m_nVocabSize);
    if (!bRet)
    {
        printf("解析vocab_size字段失败\n");
        goto EXIT;
    }
    bRet = Json::get(pJsonData, "lfr_window_size", m_nLfrWindowSize);
    if (!bRet)
    {
        printf("解析lfr_window_size字段失败\n");
        goto EXIT;
    }
    /* 读取词表 */
    for (auto& entry : m_vVocab) 
    {
        free(entry.token);  /* 释放 strdup 的内存 */
    }
    m_vVocab.clear();
    bRet = readVocab(m_strVocabPath.c_str());
    if (!bRet)
    {
        printf("词表[%s]读取失败\n", m_strVocabPath.c_str());
        goto EXIT;
    }

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }
    return bRet;
}

/* 去除字符串首尾空格（辅助函数） */
std::string Inference_NS::CSenceVoice::trim(const std::string &strInData)
{
    size_t nStart = strInData.find_first_not_of(" \t\r\n");
    if (nStart == std::string::npos)
    {
        return "";
    }
    size_t nEnd = strInData.find_last_not_of(" \t\r\n");
    return strInData.substr(nStart, nEnd - nStart + 1);
}

/* 读取词表 */
bool Inference_NS::CSenceVoice::readVocab(const char *fileName)
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
