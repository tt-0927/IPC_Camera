/*
 * @FilePath     : HumanCount.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-22 20:28:39
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-02 16:43:42
 * @Description  : 基于热力图IIM的人数统计算法
 */
#include "HumanCount.hpp"

#include <cstring>

#include "dlog.h"


InferenceV1_0_NS::CHumanCount::CHumanCount(std::string strModelPath)
    : CCVInferenceRK(strModelPath)
{
    /* 后处理初始化 */
    m_postProcess = new PostProcessV1_0_NS::cIIMPostProcess;
}

InferenceV1_0_NS::CHumanCount::~CHumanCount()
{
    if (m_postProcess)
    {
        /* 后处理去初始化 */
        delete m_postProcess;
        m_postProcess = nullptr;
    }
}

/* 推理数据 */
bool InferenceV1_0_NS::CHumanCount::inference(
    AiScenario_NS::CVData_S stInData,
    std::vector<float>&     vOutData)
{

    if(!m_pModel)
    {
        return false;
    }

    if (!m_postProcess)
    {
        return false;
    }

    bool bRet    = false;
    /* 使用模型推理前的相关变量判断 */
    bool bIsInfe = inferenceInfe(stInData);
    if (!bIsInfe)
    {
        return false;
    }

    /* 填充数据 */
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


    if (m_pOutputs[0].size == (m_nLimitHeight * m_nLimitWidth * sizeof(float)) ||
        m_pOutputs[1].size == (m_nLimitHeight * m_nLimitWidth * sizeof(float)))
    {
        /* 算法后处理 */
        m_postProcess->postProcess(
            (float*)m_pOutputs[0].buf,
            (float*)m_pOutputs[1].buf,
            m_nLimitHeight,
            m_nLimitWidth,
            vOutData);
        bRet = true;
    }
    else
    {
        dlog(LOG_ERROR, "数据后处理失败-需要处理的数据大小异常\n[0]:[%d]!=[%ld]\n[1]:[%d]!=[%ld]",
             m_pOutputs[0].size,
             m_nLimitHeight * m_nLimitWidth * sizeof(float),
             m_pOutputs[1].size,
             m_nLimitHeight * m_nLimitWidth * sizeof(float));
        bRet = false;
    }

    /* 必须释放 */
    m_pModel->releaseOutputs(m_pOutputs, m_vOutputAttrs.size());


    if (!bRet)
    {
        dlog(LOG_ERROR, "推理失败-后处理失败");
        return false;
    }

    return true;
}
