/**
 * @file ImageFeature.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-11
 * 
 * @brief 图片特征提取
 */
#include "ImageFeature.hpp"

#include <cstring>

Inference_NS::CImageFeature::CImageFeature(std::string strConfigPath)
    : CCVInferenceMOL(strConfigPath)
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

    /* 填充数据 */
    std::memcpy(reinterpret_cast<void *>(m_vInputs[0].dataIn.virAddr), stInputData.pData, m_vInputs[0].dataIn.size);

    /* 运行 */
    if (!m_pModel->run(m_vInputs,
                        m_vOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    Inference_NS::ClsData_S stClsData;

    /* 预分配空间以提高效率 */
    int nFeatureNum = m_stModelDesc.ioDesc.out[0].tensor.dims[1];
    stClsData.vFeature.reserve(nFeatureNum);
    float *pOutput =  reinterpret_cast<float*>(m_vOutputs[0].dataOut.virAddr);
    std::copy(pOutput, pOutput + nFeatureNum, std::back_inserter(stClsData.vFeature));

    vClsDatas.push_back(stClsData);
    return true;
}
