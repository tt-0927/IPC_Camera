/**
 * @file ParaformerEncoder.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-22
 *
 * @brief
 */
#include "ParaformerEncoder.hpp"
#include <chrono>

#include <cstring>

Inference_NS::CParaformerEncoder::CParaformerEncoder(std::string strConfigPath)
    : CAVInferenceOnnx(strConfigPath)
{
}

Inference_NS::CParaformerEncoder::~CParaformerEncoder()
{
}

/* 推理数据 */
bool Inference_NS::CParaformerEncoder::inference(
    std::vector<Ort::Value> &vInputs,
    std::vector<Ort::Value> &vOutputs)
{
    /* 模型推理 */
    if (!m_pModel->run(
            vInputs, vOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    return true;
}

/* 获取模型内部数据 */
std::string Inference_NS::CParaformerEncoder::getMetadata(std::string strName)
{
    std::string strOutData;
    m_pModel->getStringMetadata(strName, strOutData);
    return strOutData;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CParaformerEncoder::checkModelProConfig()
{
    return true;
}
