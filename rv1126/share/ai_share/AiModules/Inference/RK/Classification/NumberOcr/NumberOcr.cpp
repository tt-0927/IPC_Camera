/*
 * @FilePath     : NumberOcr.cpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-30 08:53:40
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-09 16:27:51
 * @Description  :
 */
#include "NumberOcr.hpp"
#include <memory>
#include <algorithm>
#include <cstring>


Inference_NS::CNumberOcr::CNumberOcr(std::string strConfigPath)
    : CCVInferenceRK(strConfigPath)
{
}

Inference_NS::CNumberOcr::~CNumberOcr()
{
}

/* 推理数据 */
bool Inference_NS::CNumberOcr::inference(
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

    /* 简单的过滤后处理 */
    int    nOutDims0 = m_vOutputAttrs[0].dims[0];
    int    nOutDims2 = m_vOutputAttrs[0].dims[2];
    float* pOutput   = (float*)vInput[0];
    /* 对输出的数据过滤多余的信息 */
    int    nOcrNum   = 0;
    int    nLastNum  = 0;
    Inference_NS::ClsData_S stClsData;
    for (int nDim0 = 0; nDim0 < nOutDims0; nDim0++)
    {
        int nMaxIndex = 0;
        for (int nDim2 = 0; nDim2 < nOutDims2; nDim2++)
        {
            if (pOutput[nDim0 * nOutDims2 + nDim2] > pOutput[nDim0 * nOutDims2 + nMaxIndex])
            {
                nMaxIndex = nDim2;
            }
        }
        if (nMaxIndex == 0 || nLastNum == nMaxIndex)
        {
            nLastNum = nMaxIndex;
            continue;
        }
        nOcrNum  = nOcrNum * 10 + (nMaxIndex - 1);
        nLastNum = nMaxIndex;
    }
    stClsData.stCls.nLabel = nOcrNum;
    /* 将获取的结果放到容器 */
    vClsDatas.push_back(stClsData);

    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CNumberOcr::checkModelProConfig()
{
    return true;
}