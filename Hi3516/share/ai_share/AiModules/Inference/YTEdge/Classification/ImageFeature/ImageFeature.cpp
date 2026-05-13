/**
 * @file ImageFeature.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-11
 *
 * @brief 图片特征提取
 */
#include "ImageFeature.hpp"
#include <memory>
#include <cstring>

Inference_NS::CImageFeature::CImageFeature(std::string strConfigPath)
    : CCVInferenceYT(strConfigPath)
{
}

Inference_NS::CImageFeature::~CImageFeature()
{
}

/* 推理数据 */
bool Inference_NS::CImageFeature::inference(
    Inference_NS::InputData_S stInputData,
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
    vClsDatas.resize(m_nOutputNum);
    for (int i = 0; i < m_nOutputNum; ++i)
    {
        float *pOutput = static_cast<float *>(m_vOutputAttrs[i].stTensor.data);
        int nFeatureNum = m_vOutputAttrs[i].stTensor.size();
        vClsDatas[i].vFeature.assign(pOutput, pOutput + nFeatureNum);
    }

    return true;
}
