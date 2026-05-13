/*
 * @FilePath     : PPOCRCls.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-29 16:29:52
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-09-29 16:29:52
 * @Description  : 车牌识别算法
 */
#include "PPOCRCls.hpp"
#include <cmath>
#include <memory>
#include <cstring>
#include <algorithm>

Inference_NS::CPPOCRCls::CPPOCRCls(std::string strConfigPath)
    : CCVInferenceRK(strConfigPath)
{
}

Inference_NS::CPPOCRCls::~CPPOCRCls()
{
}

/* 推理数据 */
bool Inference_NS::CPPOCRCls::inference(
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

    /* 使用模型推理前的相关变量判断 */
    bool bIsInfe = inferenceInfe(stInputData.nDataSize);
    if (!bIsInfe)
    {
        return false;
    }

    /* 填充数据 */
    setInputDatas((unsigned char*)stInputData.pData, 0);
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

    /* 车牌号输出 */
    if(m_nLimitWidth<=0)
    {
        printf("模型高限制m_nLimitWidth[%d]为非正数\n", m_nLimitWidth);
        return false;
    }
    int nOutSeqLen = m_vOutputAttrs[0].dims[1];
    int nOutChannel = m_vOutputAttrs[0].dims[2];
    recPostprocess(vInput[0],
        nOutChannel,
        nOutSeqLen,
        stClsData
    );

    vClsDatas.push_back(stClsData);

    return true;
}

bool Inference_NS::CPPOCRCls::recPostprocess(float* pOutData, int nOutChannel, int nOutSeqLen, Inference_NS::ClsData_S& stClsData)
{
    int nArgmaxIdx;
    int nLastIndex = 0;
    float fMaxValue = 0.0f;

    for (int n = 0; n < nOutSeqLen; n++) 
    {
        nArgmaxIdx = int(
            std::distance(&pOutData[n * nOutChannel], std::max_element(&pOutData[n * nOutChannel], &pOutData[(n + 1) * nOutChannel]))
        );

        fMaxValue = float(*std::max_element(&pOutData[n * nOutChannel], &pOutData[(n + 1) * nOutChannel]));

        if (nArgmaxIdx > 0 && (!(n > 0 && nArgmaxIdx == nLastIndex))) 
        {
            /* 存储文字类别下标和质细腻度 */ 
            Cls_S  stCls;
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
