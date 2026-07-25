/**
 * @file TextFeature.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-11
 *
 * @brief 文字特征提取
 */
#include "TextFeature.hpp"
#include <memory>
#include <cstring>

Inference_NS::CTextFeature::CTextFeature(std::string strConfigPath)
    : CNLPInferenceOnnx(strConfigPath)
{
}

Inference_NS::CTextFeature::~CTextFeature()
{
}

/* 推理数据 */
bool Inference_NS::CTextFeature::inference(
    Inference_NS::InputData_S stInputData,
    std::vector<Inference_NS::ClsData_S> &vClsDatas)
{
    /* 输出数据清空 */
    if (!vClsDatas.empty())
    {
        vClsDatas.clear();
    }

    if (!m_pModel)
    {
        return false;
    }

    if (stInputData.strText.empty()) {
        printf("输入的字符串strText为空\n");
        return false;
    }

    int nSequenceLen = m_vInputAttrs[0][1];
    /* 分词以及生成向量 */
    std::vector<int64_t> vToken;
    vToken.reserve(nSequenceLen); 
    m_pTokenize->encode_text(stInputData.strText, vToken);

    if (vToken.size() > nSequenceLen)
    {
        printf("输入文本 \"%s\" token 大于模型的输入上限 %d\n", stInputData.strText.c_str(), nSequenceLen);
        return false;
    }

    int nTokenSize = vToken.size();
    for (int nP = 0; nP < nSequenceLen - nTokenSize; nP++)
    {
        vToken.push_back(0);
    }

    /* 使用模型推理前的相关变量判断 */
    bool bIsInfe = inferenceInfe(vToken.size());
    if (!bIsInfe)
    {
        return false;
    }

    /* 模型推理 */
    std::vector<Ort::Value> vOrtInputs;
    std::vector<Ort::Value> vOrtOutputs;
    Ort::MemoryInfo stMemoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    /* 第一个输入 */
    std::array<int64_t, 2> aInputShape{1, nSequenceLen};

    Ort::Value oInput = Ort::Value::CreateTensor(stMemoryInfo, vToken.data(), vToken.size(),
                                             aInputShape.data(), aInputShape.size());
    vOrtInputs.push_back(std::move(oInput));

    std::vector<float *> vOutputs;
    if (!m_pModel->run(
            vOrtInputs,
            vOrtOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }
    for(int nOutIndex=0; nOutIndex<vOrtOutputs.size(); nOutIndex++)
    {
        Ort::Value &oOut = vOrtOutputs[nOutIndex];
        float *pOut = oOut.GetTensorMutableData<float>();
        vOutputs.push_back(std::move(pOut));
    }
    
    /* 将所有的模型输出头，加入容器中 */
    int nFeatureNum = 1;
    vClsDatas.resize(m_nOutputNum);
    for (int i = 0; i < m_nOutputNum; ++i)
    {
        for(int j = 0; j < m_vOutputAttrs[i].size(); j++)
        {
            nFeatureNum *= m_vOutputAttrs[i][j];
        }
        vClsDatas[i].vFeature.assign(vOutputs[i], vOutputs[i] + nFeatureNum);
        nFeatureNum = 1;
    }

    return true;
}
