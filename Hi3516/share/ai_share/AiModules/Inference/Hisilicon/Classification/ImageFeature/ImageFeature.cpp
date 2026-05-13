/**
 * @file ImageFeature.cpp
 * @author liaoet (liaoet@kfb.cn)
 * @date 2025-7-3
 * 
 * @brief 图片特征提取
 */
#include "ImageFeature.hpp"
#include <memory>
#include <cstring>

Inference_NS::CImageFeature::CImageFeature(std::string strConfigPath)
    : CCVInferenceHISI(strConfigPath)
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
    setInputDatas((unsigned char*)stInputData.pData, 0);
    /* 运行 */
    if (!m_pModel->run(m_pInputs, m_pOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    /* 将所有的模型输出头，加入容器中 */
    std::vector<float *> vInput;
    for (int i = 0; i < m_nOutputNum; ++i)
    {
        /* 获取数据的虚拟地址 */
        void *dataBuf = svp_acl_get_data_buffer_addr(m_vOutputBuffers[i]);
        float *pOut = reinterpret_cast<float *>(dataBuf);
        vInput.push_back(pOut);
    }

    Inference_NS::ClsData_S stClsData;
    /* 预分配空间以提高效率 */
    int nFeatureNum = m_vOutputDims[0][1];
    stClsData.vFeature.reserve(nFeatureNum);
    float *pOutput = (float *)vInput[0];
    
    std::copy(pOutput, pOutput + nFeatureNum, std::back_inserter(stClsData.vFeature));

    vClsDatas.push_back(stClsData);
    return true;
}
