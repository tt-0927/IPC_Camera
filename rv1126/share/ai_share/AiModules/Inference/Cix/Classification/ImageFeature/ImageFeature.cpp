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
    : CCVInferenceCix(strConfigPath)
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
        printf("未初始化-推理失败\n");
        return false;
    }
    
    /* 使用模型推理前的相关变量判断 */
    stInputData.nDataSize /= sizeof(float); /* 计算元素个数 */
    bool bIsInfe = inferenceInfe(0, stInputData.nDataSize);
    if (!bIsInfe)
    {
        printf("使用模型推理前的相关变量失败-推理失败\n");
        return false;
    }

    /* 填充数据 */
    std::vector<void *> vInputData;
    vInputData.push_back((void *)stInputData.pData);
    std::vector<std::vector<float>> vOutputs(m_vOutputAttrs.size());
    /* 运行 */
    if (!m_pModel->run(vInputData, vOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    /* 将所有的模型输出头，加入容器中 */
    std::vector<float *> vInput;
    for (int i = 0; i < m_vOutputAttrs.size(); ++i)
    {
        vInput.push_back(vOutputs[i].data());
    }

    /* 后处理 */
    if (m_vOutputAttrs.size() != m_vOutSizes.size())
    {
        printf("json文件的 output_shape 数组长度[%ld] 不等于 模型输入个数[%ld]\n", m_vOutSizes.size(), m_vOutputAttrs.size());
        return false;
    }
    /* 将所有的模型输出头，加入容器中 */
    vClsDatas.resize(m_nOutputNum);
    for (int i = 0; i < m_nOutputNum; ++i)
    {
        int nFeatureNum = m_vOutputAttrs[i].size;
        vClsDatas[i].vFeature.assign(vInput[i], vInput[i] + nFeatureNum);
    }

    return true;
}
