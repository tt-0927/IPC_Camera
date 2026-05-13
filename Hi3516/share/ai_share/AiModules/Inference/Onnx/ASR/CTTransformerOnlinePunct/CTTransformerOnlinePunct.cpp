/**
 * @file CTTransformerOnlinePunct.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */
#include "CTTransformerOnlinePunct.hpp"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cstring>

Inference_NS::CCTTransformerOnlinePunct::CCTTransformerOnlinePunct(std::string strConfigPath)
    : CAVInferenceOnnx(strConfigPath)
{
}

Inference_NS::CCTTransformerOnlinePunct::~CCTTransformerOnlinePunct()
{
}

/* 获取模型内部的Metadata */
bool Inference_NS::CCTTransformerOnlinePunct::initMetadata()
{
    std::string strTokens;
    std::string strVocabSize;
    std::string strPunctuations;
    std::string strUnkSymbol;
    m_pModel->getStringMetadata("tokens", strTokens);
    m_pModel->getStringMetadata("vocab_size", strVocabSize);
    m_pModel->getStringMetadata("punctuations", strPunctuations);
    m_pModel->getStringMetadata("unk_symbol", strUnkSymbol);

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
    /* 中文符号到英文符号的映射 */
    stMetaData.punct_ch2en["。"] = ".";
    stMetaData.punct_ch2en["，"] = ",";
    stMetaData.punct_ch2en["？"] = "?";
    stMetaData.punct_ch2en["、"] = " ";

    return true;
}

/* 推理数据 */
bool Inference_NS::CCTTransformerOnlinePunct::inference(
    Inference_NS::InputData_S stInputData,
    Inference_NS::ASRData_S &stASRData)
{
    if (!m_pModel)
    {
        return false;
    }

    std::string strText;
    for (const auto &s : m_vArrCache)
    {
        strText += s;
    }

    /* 如果上一句的结尾是英语字母，并且这一句的开始也是英语字母，应该添加空格 */
    // if (!strText.empty() && !(strText.back() & 0x80) &&
    //     !stInputData.strText.empty() && !(stInputData.strText.front() & 0x80))
    // {
    //     strText += " "; // 添加空格
    // }

    strText += stInputData.strText;
    // printf("====================================\n");
    // printf("[%s]\n", strText.c_str());

    if (strText.empty())
    {
        printf("输入数据为空\n");
        return false;
    }

    std::vector<int32_t> InputData;
    std::vector<std::string> strOut;
    std::vector<std::string> vTokens = SplitUtf8(strText);
    InputData.reserve(vTokens.size());
    strOut.reserve(vTokens.size());
    for (const auto &t : vTokens)
    {
        std::string strToken = ToLowerCase(t);
        if (stMetaData.token2id.count(strToken))
        {
            InputData.push_back(stMetaData.token2id.at(strToken));
            strOut.push_back(strToken);
        }
        else
        {
            InputData.push_back(stMetaData.unk_id);
            strOut.push_back(strToken);
        }
    }

    int nTotalBatch = ceil((float)InputData.size() / nSegmentSize);
    int nCurBatch = -1;
    int nSentEnd = -1, nLastCommaIndex = -1;
    std::vector<int32_t> RemainIDs;                                   //
    std::vector<std::string> RemainStr;                               //
    std::vector<int> new_mini_sentence_punc;                          //          sentence_punc_list = []
    std::vector<std::string> sentenceOut;                             // sentenceOut
    std::vector<std::string> sentence_punc_list, sentence_words_list; // sentence_words_list = []

    int nSkipNum = 0;
    int nDiff = 0;
    for (size_t i = 0; i < InputData.size(); i += nSegmentSize)
    {
        nDiff = (i + nSegmentSize) < InputData.size() ? (0) : (i + nSegmentSize - InputData.size());
        std::vector<int32_t> InputIDs(InputData.begin() + i, InputData.begin() + i + (nSegmentSize - nDiff));
        std::vector<std::string> InputStr(strOut.begin() + i, strOut.begin() + i + (nSegmentSize - nDiff));
        InputIDs.insert(InputIDs.begin(), RemainIDs.begin(), RemainIDs.end()); // RemainIDs+InputIDs;
        InputStr.insert(InputStr.begin(), RemainStr.begin(), RemainStr.end()); // RemainStr+InputStr;

        std::vector<int> Punction;
        if (!detect(InputIDs, m_vArrCache.size(), Punction))
        {
            return false;
        }

        nCurBatch = i / nSegmentSize;
        if (nCurBatch < nTotalBatch - 1) // not the last minisetence
        {
            nSentEnd = -1;
            nLastCommaIndex = -1;
            for (int nIndex = Punction.size() - 2; nIndex > 0; nIndex--)
            {
                if (Punction[nIndex] == stMetaData.dot_id || Punction[nIndex] == stMetaData.quest_id)
                {
                    nSentEnd = nIndex;
                    break;
                }
                if (nLastCommaIndex < 0 && Punction[nIndex] == stMetaData.comma_id)
                {
                    nLastCommaIndex = nIndex;
                }
            }
            if (nSentEnd < 0 && InputStr.size() > nMaxLen && nLastCommaIndex > 0)
            {
                nSentEnd = nLastCommaIndex;
                Punction[nSentEnd] = stMetaData.dot_id;
            }
            RemainStr.assign(InputStr.begin() + (nSentEnd + 1), InputStr.end());
            RemainIDs.assign(InputIDs.begin() + (nSentEnd + 1), InputIDs.end());
            InputStr.assign(InputStr.begin(), InputStr.begin() + (nSentEnd + 1)); // minit_sentence
            Punction.assign(Punction.begin(), Punction.begin() + (nSentEnd + 1));
        }

        for (auto &item : Punction)
        {
            sentence_punc_list.push_back(stMetaData.id2punct[item]);
        }

        sentence_words_list.insert(sentence_words_list.end(), InputStr.begin(), InputStr.end());
        new_mini_sentence_punc.insert(new_mini_sentence_punc.end(), Punction.begin(), Punction.end());
    }
    std::vector<std::string> WordWithPunc;
    for (int i = 0; i < sentence_words_list.size(); i++) // for i in range(0, len(sentence_words_list)):
    {
        if (sentence_words_list[i].size() > 0 && !(sentence_words_list[i][0] & 0x80) && (i + 1) < sentence_words_list.size() && !(sentence_words_list[i + 1][0] & 0x80))
        {
            sentence_words_list[i] = sentence_words_list[i] + " ";
        }
        if (nSkipNum < m_vArrCache.size()) //    if skip_num < len(cache):
            nSkipNum++;
        else
            WordWithPunc.push_back(sentence_words_list[i]);

        if (nSkipNum >= m_vArrCache.size())
        {
            if (sentence_punc_list[i] != stMetaData.id2punct[stMetaData.underline_id])
            {
                WordWithPunc.push_back(sentence_punc_list[i]);
            }
        }
    }

    sentenceOut.insert(sentenceOut.end(), WordWithPunc.begin(), WordWithPunc.end()); //
    nSentEnd = -1;
    for (int i = sentence_punc_list.size() - 2; i > 0; i--)
    {
        if (new_mini_sentence_punc[i] == stMetaData.dot_id || new_mini_sentence_punc[i] == stMetaData.quest_id)
        {
            nSentEnd = i;
            break;
        }
    }
    m_vArrCache.assign(sentence_words_list.begin() + (nSentEnd + 1), sentence_words_list.end());

    /* 移除末尾多余的标点*/
    if (sentenceOut.size() > 0 && (stMetaData.punct2id.count(sentenceOut.back()) > 0))
    {
        m_strEndRes = sentenceOut.back();
        sentenceOut.pop_back();
    }

    stASRData.strText = "";
    for (const auto &s : sentenceOut)
    {
        stASRData.strText += s;
    }
    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CCTTransformerOnlinePunct::checkModelProConfig()
{
    return true;
}

/* 字符串转为整形 */
bool Inference_NS::CCTTransformerOnlinePunct::stringToInt(std::string strData, int &nOutData)
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
void Inference_NS::CCTTransformerOnlinePunct::splitStringToVector(
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

bool Inference_NS::CCTTransformerOnlinePunct::detect(std::vector<int32_t> vInputData, int nCacheSize, std::vector<int> &vPunction)
{
    /* 模型输入信息配置 */
    std::vector<Ort::Value> vInputs;
    std::vector<Ort::Value> vOutputs;

    Ort::MemoryInfo oMemoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::array<int64_t, 2> aInputShape{1, (int64_t)vInputData.size()};
    Ort::Value oInput = Ort::Value::CreateTensor(
        oMemoryInfo,
        vInputData.data(),
        vInputData.size() * sizeof(int32_t),
        aInputShape.data(),
        aInputShape.size(), ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32);
    vInputs.push_back(std::move(oInput));

    std::array<int32_t, 1> nTextLengths{(int32_t)vInputData.size()};
    std::array<int64_t, 1> aTextLengthsDim{1};
    Ort::Value oTextLengths = Ort::Value::CreateTensor<int32_t>(
        oMemoryInfo,
        nTextLengths.data(),
        nTextLengths.size(),
        aTextLengthsDim.data(),
        aTextLengthsDim.size());
    vInputs.push_back(std::move(oTextLengths));

    /* vad_mask */
    std::vector<float> vVadMask;
    int nTextLength = vInputData.size();
    vadMask(nTextLength, nCacheSize, vVadMask);
    std::array<int64_t, 4> aVadMaskDim{1, 1, nTextLength, nTextLength};
    Ort::Value oVadMask = Ort::Value::CreateTensor<float>(
        oMemoryInfo,
        vVadMask.data(),
        vVadMask.size(),
        aVadMaskDim.data(),
        aVadMaskDim.size());
    vInputs.push_back(std::move(oVadMask));

    /* sub_masks */
    std::array<int64_t, 4> aSubMaskDim{1, 1, nTextLength, nTextLength};
    Ort::Value oSubMask = Ort::Value::CreateTensor<float>(
        oMemoryInfo,
        vVadMask.data(),
        vVadMask.size(),
        aSubMaskDim.data(),
        aSubMaskDim.size()); // , ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT);
    vInputs.push_back(std::move(oSubMask));

    /* 模型推理 */
    if (!m_pModel->run(
            vInputs,
            vOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }
    std::vector<int64_t> vOutputShape = vOutputs[0].GetTensorTypeAndShapeInfo().GetShape();
    int64_t nOutputCount = 1;
    for (const auto &n : vOutputShape)
    {
        nOutputCount *= n;
    }
    float *pData = vOutputs[0].GetTensorMutableData<float>();

    for (int i = 0; i < nOutputCount; i += nCandidateNum)
    {
        int nIndex = std::distance(pData + i, std::max_element(pData + i, pData + i + nCandidateNum - 1));
        vPunction.push_back(nIndex);
    }
    return true;
}

void Inference_NS::CCTTransformerOnlinePunct::vadMask(int nSize, int vad_pos, std::vector<float> &Result)
{
    Result.resize(0);
    Result.assign(nSize * nSize, 1);
    if (vad_pos <= 0 || vad_pos >= nSize)
    {
        return;
    }
    for (int i = 0; i < vad_pos - 1; i++)
    {
        for (int j = vad_pos; j < nSize; j++)
        {
            Result[i * nSize + j] = 0.0f;
        }
    }
}

std::string Inference_NS::CCTTransformerOnlinePunct::clearArrCache()
{
    m_vArrCache.clear();
    std::string strEndRes = m_strEndRes;
    m_strEndRes = "";
    return strEndRes;
}