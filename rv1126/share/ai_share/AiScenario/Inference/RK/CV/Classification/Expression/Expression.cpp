/*
 * @FilePath     : Expression.cpp
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-08-01 16:29:52
 * @LastEditors  : 吴才朋 wucp@kfb.cn
 * @LastEditTime : 2024-08-01 16:29:52
 * @Description  : 表情识别算法
 */
#include "Expression.hpp"
#include <algorithm> 
#include <cstring>

#include "dlog.h"

InferenceV1_0_NS::CExpression::CExpression(std::string strModelPath)
    : CCVInferenceRK(strModelPath)
{
}

InferenceV1_0_NS::CExpression::~CExpression()
{
}

/* 推理数据 */
bool InferenceV1_0_NS::CExpression::inference(
    AiScenario_NS::CVData_S stInData,
    std::vector<float>&     vOutData)
{
    if(!m_pModel)
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
    /* 预分配空间以提高效率 */
    int nFeatureNum = m_vOutputAttrs[0].dims[1];
    vOutData.reserve(nFeatureNum);
    float* pOutput = (float*) m_pOutputs[0].buf;
    std::copy(pOutput, pOutput + nFeatureNum, std::back_inserter(vOutData));

    /* 必须释放 */
    m_pModel->releaseOutputs(m_pOutputs, m_vOutputAttrs.size());


    return true;
}

