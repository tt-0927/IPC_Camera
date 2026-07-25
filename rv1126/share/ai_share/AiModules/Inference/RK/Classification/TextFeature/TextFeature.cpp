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
    : CNLPInferenceRK(strConfigPath)
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

    int nSequenceLen = m_vInputAttrs[0].dims[1];
    /* 分词以及生成向量 */
    std::vector<int> vToken;
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

    /* 填充数据 */
    memcpy(m_pInputs[0]->virt_addr, (int32_t *)vToken.data(), m_pInputs[0]->size);

    /* 运行 */
    if (!m_pModel->run())
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }
    /* 将所有的模型输出头，加入容器中 */
    std::vector<float *> vInput;
    std::vector<std::unique_ptr<float[]>> vBufKeeper;  /* 引入智能指针-保证内存生命周期 */
    for (int i = 0; i < m_nOutputNum; ++i)
    {
        /* int8输出通过反量化转为float */
        if (m_vOutputAttrs[i].type == RKNN_TENSOR_INT8)
        {
            int8_t *pOut = (int8_t *)m_pOutputs[i]->virt_addr;
            int nOutSize = m_vOutputAttrs[i].size_with_stride;
            auto buf = std::make_unique<float[]>(nOutSize);
            float* pDst = buf.get();
            for (int index = 0; index < m_vOutputAttrs[i].n_elems; ++index)
            {
                pDst[index] = (pOut[index] - m_vOutputAttrs[i].zp) * m_vOutputAttrs[i].scale;
            }
            vInput.push_back(pDst);
            vBufKeeper.push_back(std::move(buf));  /* 保留智能指针，防止悬垂 */
        }
        else
        {
            float* pOut = (float*) m_pOutputs[i]->virt_addr;
            vInput.push_back(pOut);
        }
    }

    Inference_NS::ClsData_S stClsData;

    /* 预分配空间以提高效率 */
    int nFeatureNum = m_vOutputAttrs[0].dims[1];
    stClsData.vFeature.reserve(nFeatureNum);
    float *pOutput = (float *)vInput[0];
    std::copy(pOutput, pOutput + nFeatureNum, std::back_inserter(stClsData.vFeature));

    vClsDatas.push_back(stClsData);

    return true;
}
