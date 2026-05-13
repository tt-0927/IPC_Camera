/**
 * @file FastPose.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-11-20
 *
 * @brief
 */
#include "FastHumanPose.hpp"

#include <algorithm>    // std::transform
#include <cmath>        // std::round（可选）
#include <cstring>
#include <memory>

#include "FastPosePostPress.hpp"


Inference_NS::CFastHumanPose::CFastHumanPose(std::string strConfigPath)
    : CCVInferenceCix(strConfigPath)
{
}

Inference_NS::CFastHumanPose::~CFastHumanPose()
{
}

/* 推理数据 */
bool Inference_NS::CFastHumanPose::inference(
    Inference_NS::InputData_S               stInputData,
    std::vector<Inference_NS::PointData_S>& vPointDatas)
{
    /* 输出数据清空 */
    if (!vPointDatas.empty())
    {
        vPointDatas.clear();
    }

    /* 使用模型推理前的相关变量判断 */
    stInputData.nDataSize /= sizeof(float); /* 计算元素个数 */
    bool bIsInfe           = inferenceInfe(0, stInputData.nDataSize);
    if (!bIsInfe)
    {
        return false;
    }

    /* 填充数据 */
    std::vector<void*> vInputData;
    vInputData.push_back((void*)stInputData.pData);
    std::vector<std::vector<float>> vOutputs(m_vOutputAttrs.size());
    /* 运行 */
    if (!m_pModel->run(vInputData, vOutputs))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    /* 将所有的模型输出头，加入容器中 */
    std::vector<float*> vInput;
    for (int i = 0; i < m_vOutputAttrs.size(); ++i)
    {
        vInput.push_back(vOutputs[i].data());
    }

    heatmap2Keypoint(vInput, vPointDatas);

    return true;
}

/* 重写父类的解析json模型数据，用于适配不同类型的模型 */
bool Inference_NS::CFastHumanPose::checkModelProConfig()
{
    return true;
}