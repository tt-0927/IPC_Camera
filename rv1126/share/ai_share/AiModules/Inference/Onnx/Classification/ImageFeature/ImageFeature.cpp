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
    : CCVInferenceOnnx(strConfigPath)
{
}

Inference_NS::CImageFeature::~CImageFeature()
{
}

/* 推理数据 */
bool Inference_NS::CImageFeature::inference(
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

    /* 模型推理 */
    std::vector<float *> vInput = {reinterpret_cast<float *>(stInputData.pData)};
    std::vector<int64_t> nInputDataSizes = {static_cast<int64_t>(stInputData.nDataSize)};
    std::vector<float *> vOutputs;
    if (!m_pModel->run(
            vInput,
            nInputDataSizes,
            vOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
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
