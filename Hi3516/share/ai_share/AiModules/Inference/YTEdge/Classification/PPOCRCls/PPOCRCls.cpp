/**
 * @file PPOCRCls.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-10-22
 *
 * @brief
 */
#include "PPOCRCls.hpp"
#include <cmath>
#include <memory>
#include <cstring>
#include <algorithm>

Inference_NS::CPPOCRCls::CPPOCRCls(std::string strConfigPath)
    : CCVInferenceYT(strConfigPath)
{
}

Inference_NS::CPPOCRCls::~CPPOCRCls()
{
}

/* 推理数据 */
bool Inference_NS::CPPOCRCls::inference(
    std::vector<Inference_NS::ClsData_S> &vClsDatas,
    bool bDCLResize)
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

    if (!bDCLResize)
    {
        if (!stInputData.pData)
        {
            printf("输入的pData数据为空\n");
            return false;
        }
        /* 输入float*转为unsigned void*，长度适配 */
        stInputData.nDataSize /= sizeof(float);
        /* 插入输入数据 */
        bool bIsInfe = setInputDatas((unsigned char *)stInputData.pData, stInputData.nDataSize, 0);
        if (!bIsInfe)
        {
            return false;
        }
    }

    /* 运行 */
    if (!m_pModel->run(m_pInputDataset, m_pOutputDataset))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    /* 将所有的模型输出头，加入容器中 */
    std::vector<float *> vInput;
    for (int i = 0; i < m_nOutputNum; ++i)
    {
        float *pOutput = (float *)m_vOutputAttrs[i].stTensor.data;
        vInput.push_back(pOutput);
    }

    Inference_NS::ClsData_S stClsData;

    /* 车牌号输出 */
    if (m_nLimitWidth <= 0)
    {
        printf("模型高限制m_nLimitWidth[%d]为非正数\n", m_nLimitWidth);
        return false;
    }
    int nOutSeqLen = m_vOutputAttrs[0].stTensor.dims[1];
    int nOutChannel = m_vOutputAttrs[0].stTensor.dims[2];
    recPostprocess(vInput[0],
                   nOutChannel,
                   nOutSeqLen,
                   stClsData);

    vClsDatas.push_back(stClsData);

    return true;
}

bool Inference_NS::CPPOCRCls::recPostprocess(float *pOutData, int nOutChannel, int nOutSeqLen, Inference_NS::ClsData_S &stClsData)
{
    int nArgmaxIdx;
    int nLastIndex = 0;
    float fMaxValue = 0.0f;

    for (int n = 0; n < nOutSeqLen; n++)
    {
        nArgmaxIdx = int(
            std::distance(&pOutData[n * nOutChannel], std::max_element(&pOutData[n * nOutChannel], &pOutData[(n + 1) * nOutChannel])));

        fMaxValue = float(*std::max_element(&pOutData[n * nOutChannel], &pOutData[(n + 1) * nOutChannel]));

        if (nArgmaxIdx > 0 && (!(n > 0 && nArgmaxIdx == nLastIndex)))
        {
            /* 存储文字类别下标和质细腻度 */
            Cls_S stCls;
            stCls.nLabel = nArgmaxIdx;
            stCls.fConfidence = fMaxValue;
            stClsData.vCls.push_back(stCls);
        }
        nLastIndex = nArgmaxIdx;
    }
    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CPPOCRCls::checkModelProConfig()
{
    return true;
}
