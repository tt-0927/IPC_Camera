/*
 * @FilePath     : NumberOcr.cpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-30 08:53:40
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-09 16:27:51
 * @Description  :
 */
#include "NumberOcr.hpp"

#include <algorithm>
#include <cstring>

#include "dlog.h"

InferenceV1_0_NS::CNumberOcr::CNumberOcr(std::string strModelPath)
    : CCVInferenceRK(strModelPath)
{
}

InferenceV1_0_NS::CNumberOcr::~CNumberOcr()
{
}

/* 推理数据 */
bool InferenceV1_0_NS::CNumberOcr::inference(
    AiScenario_NS::CVData_S stInData,
    std::vector<float>&     vOutData)
{
    if (!m_pModel)
    {
        return false;
    }

    /* 使用模型推理前的相关变量判断 */
    bool bIsInfe = inferenceInfe(stInData);
    if (!bIsInfe)
    {
        return false;
    }

    m_pInputs[0].buf = (void*)stInData.inMat.data;


    /* 运行 */
    if (!m_pModel->run(m_pInputs,
                       m_vInputAttrs.size(),
                       m_pOutputs,
                       m_vOutputAttrs.size()))
    {
        dlog(LOG_ERROR, "推理失败-运行模型失败");
        return false;
    }


    if (!vOutData.empty())
    {
        vOutData.clear();
    }

    /* 简单的过滤后处理 */
    int    nOutDims0 = m_vOutputAttrs[0].dims[0];
    int    nOutDims2 = m_vOutputAttrs[0].dims[2];
    float* pOutput   = (float*)m_pOutputs[0].buf;
    /* 对输出的数据过滤多余的信息 */
    int    nOcrNum   = 0;
    int    nLastNum  = 0;
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
    /* 将获取的结果放到容器 */
    vOutData.push_back(nOcrNum);

    /* 必须释放 */
    m_pModel->releaseOutputs(m_pOutputs, m_vOutputAttrs.size());


    return true;
}
