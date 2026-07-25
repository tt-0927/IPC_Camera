/**
 * @file CTTransformerPunct.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */
#include "CTTransformerPunct.hpp"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cstring>

Inference_NS::CCTTransformerPunct::CCTTransformerPunct(std::string strConfigPath)
    : CAVInferenceOnnx(strConfigPath)
{
}

Inference_NS::CCTTransformerPunct::~CCTTransformerPunct()
{
}

/* 获取模型内部的Metadata */
bool Inference_NS::CCTTransformerPunct::initMetadata()
{
    std::string strTokens;
    std::string strVocabSize;
    std::string strPunctuations;
    std::string strUnkSymbol;
    m_pModel->getStringMetadata("tokens", strTokens);
    m_pModel->getStringMetadata("vocab_size", strVocabSize);
    m_pModel->getStringMetadata("punctuations", strPunctuations);
    m_pModel->getStringMetadata("unk_symbol", strUnkSymbol);

    int32_t nVocabSize = 0;
    std::vector<std::string> vTokens;
    stringToInt(strVocabSize, nVocabSize);
    splitStringToVector(strTokens.c_str(), '|', &vTokens);
    splitStringToVector(strPunctuations.c_str(), '|', &stMetaData.id2punct);

    /* 判断获取的词表是否正确 */
    if (static_cast<int32_t>(vTokens.size()) != nVocabSize)
    {
        printf("vTokens.size() %d != nVocabSize %d",
               static_cast<int32_t>(vTokens.size()), nVocabSize);
        return false;
    }

    /* output shape is (N, T, num_punctuations) */
    stMetaData.num_punctuations = m_vOutputAttrs[0][2];

    int32_t i = 0;
    for (const auto &t : vTokens)
    {
        stMetaData.token2id[t] = i;
        i += 1;
    }
    i = 0;
    for (const auto &p : stMetaData.id2punct)
    {
        stMetaData.punct2id[p] = i;
        i += 1;
    }
    stMetaData.unk_id = stMetaData.token2id.at(strUnkSymbol);
    stMetaData.dot_id = stMetaData.punct2id.at("。");
    stMetaData.comma_id = stMetaData.punct2id.at("，");
    stMetaData.quest_id = stMetaData.punct2id.at("？");
    stMetaData.pause_id = stMetaData.punct2id.at("、");
    stMetaData.underline_id = stMetaData.punct2id.at("_");

    return true;
}

/* 推理数据 */
bool Inference_NS::CCTTransformerPunct::inference(
    Inference_NS::InputData_S stInputData,
    Inference_NS::ASRData_S &stASRData)
{
    if (stInputData.strText.empty())
    {
        printf("输入数据为空\n");
        return false;
    }
    if (!m_pModel)
    {
        return false;
    }

    std::vector<std::string> vTokens = SplitUtf8(stInputData.strText);
    std::vector<int32_t> vTokenIds;
    vTokenIds.reserve(vTokens.size());
    for (const auto &t : vTokens)
    {
        std::string strToken = ToLowerCase(t);
        if (stMetaData.token2id.count(strToken))
        {
            vTokenIds.push_back(stMetaData.token2id.at(strToken));
        }
        else
        {
            vTokenIds.push_back(stMetaData.unk_id);
        }
    }

    Ort::MemoryInfo stMemoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    int32_t nNumSegments = ceil((static_cast<float>(vTokenIds.size()) + nSegmentSize - 1) / nSegmentSize);

    std::vector<int32_t> vPunctuations;
    int32_t nLast = -1;
    for (int32_t i = 0; i != nNumSegments; ++i)
    {
        int32_t nThisStart = i * nSegmentSize;
        int32_t nThisEnd = nThisStart + nSegmentSize;
        if (nThisEnd > static_cast<int32_t>(vTokenIds.size()))
        {
            nThisEnd = vTokenIds.size();
        }

        if (nLast != -1)
        {
            nThisStart = nLast;
        }

        /* 模型输入信息配置 */
        std::vector<Ort::Value> vInputs;
        std::vector<Ort::Value> vOutputs;
        std::array<int64_t, 2> aXShape = {1, nThisEnd - nThisStart};
        Ort::Value oX = Ort::Value::CreateTensor(stMemoryInfo, vTokenIds.data() + nThisStart,
                                                 aXShape[1], aXShape.data(), aXShape.size());
        vInputs.push_back(std::move(oX));
        int64_t nLenShape = 1;
        int32_t nLen = aXShape[1];
        Ort::Value oXLen = Ort::Value::CreateTensor(stMemoryInfo, &nLen, 1, &nLenShape, 1);
        vInputs.push_back(std::move(oXLen));

        /* 模型推理 */
        if (!m_pModel->run(
                vInputs,
                vOutputs))
        {
            printf("推理失败-运行模型失败\n");
            return false;
        }

        /* [N, T, num_punctuations] */
        std::vector<int64_t> out_shape = vOutputs[0].GetTensorTypeAndShapeInfo().GetShape();

        if ((out_shape[0] != 1) || (out_shape[1] != nLen) || (out_shape[2] != stMetaData.num_punctuations))
        {
            printf("out_shape[%d] != 1 或 out_shape[%d] != nLen[%d] 或 out_shape[%d] != num_punctuations[%d]",
                   out_shape[0], out_shape[1], nLen, out_shape[2], stMetaData.num_punctuations);
            return false;
        }

        std::vector<int32_t> vThisPunctuations;
        vThisPunctuations.reserve(nLen);
        const float *p = vOutputs[0].GetTensorData<float>();
        for (int32_t k = 0; k != nLen; ++k, p += stMetaData.num_punctuations)
        {
            auto index = static_cast<int32_t>(std::distance(
                p, std::max_element(p, p + stMetaData.num_punctuations)));
            vThisPunctuations.push_back(index);
        }

        int32_t nDotIndex = -1;
        int32_t nCommaIndex = -1;

        for (int32_t m = static_cast<int32_t>(vThisPunctuations.size()) - 2; m >= 1; --m)
        {
            int32_t nPunctId = vThisPunctuations[m];

            if (nPunctId == stMetaData.dot_id || nPunctId == stMetaData.quest_id)
            {
                nDotIndex = m;
                break;
            }

            if (nCommaIndex == -1 && nPunctId == stMetaData.comma_id)
            {
                nCommaIndex = m;
            }
        }
        if (nDotIndex == -1 && nLen >= nMaxLen && nCommaIndex != -1)
        {
            nDotIndex = nCommaIndex;
            vThisPunctuations[nDotIndex] = stMetaData.dot_id;
        }

        if (nDotIndex == -1)
        {
            if (nLast == -1)
            {
                nLast = nThisStart;
            }

            if (i == nNumSegments - 1)
            {
                nDotIndex = static_cast<int32_t>(vThisPunctuations.size()) - 1;
            }
        }
        else
        {
            nLast = nThisStart + nDotIndex + 1;
        }

        if (nDotIndex != -1)
        {
            vPunctuations.insert(vPunctuations.end(), vThisPunctuations.begin(),
                                 vThisPunctuations.begin() + (nDotIndex + 1));
        }
    }

    if (vPunctuations.empty())
    {
        stASRData.strText = stInputData.strText + stMetaData.id2punct[stMetaData.dot_id];
        return true;
    }
    std::vector<std::string> vWordsPunct;

    for (int32_t i = 0; i != static_cast<int32_t>(vPunctuations.size()); ++i)
    {
        if (i >= static_cast<int32_t>(vTokens.size()))
        {
            break;
        }
        std::string &w = vTokens[i];
        if (i > 0 && !(vWordsPunct.back()[0] & 0x80) && !(w[0] & 0x80))
        {
            vWordsPunct.push_back(" ");
        }
        vWordsPunct.push_back(std::move(w));

        if (vPunctuations[i] != stMetaData.underline_id)
        {
            vWordsPunct.push_back(stMetaData.id2punct[vPunctuations[i]]);
        }
    }

    if (vWordsPunct.back() == stMetaData.id2punct[stMetaData.comma_id] ||
        vWordsPunct.back() == stMetaData.id2punct[stMetaData.pause_id])
    {
        vWordsPunct.back() = stMetaData.id2punct[stMetaData.dot_id];
    }

    if (vWordsPunct.back() != stMetaData.id2punct[stMetaData.dot_id] &&
        vWordsPunct.back() != stMetaData.id2punct[stMetaData.quest_id])
    {
        vWordsPunct.push_back(stMetaData.id2punct[stMetaData.dot_id]);
    }

    for (const auto &w : vWordsPunct)
    {
        stASRData.strText.append(w);
    }

    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CCTTransformerPunct::checkModelProConfig()
{
    return true;
}

/* 字符串转为整形 */
bool Inference_NS::CCTTransformerPunct::stringToInt(std::string strData, int &nOutData)
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

/* 字符串切割 */
void Inference_NS::CCTTransformerPunct::splitStringToVector(
    const std::string &strInData,
    const char cDelim,
    std::vector<std::string> *pOutData)
{
    pOutData->clear();
    std::istringstream tokenStream(strInData);
    std::string strToken;
    /* 逐段读取分割结果 */
    while (std::getline(tokenStream, strToken, cDelim))
    {
        /* 跳过空字符串 */
        if (!strToken.empty())
        {
            pOutData->push_back(strToken);
        }
    }
}
