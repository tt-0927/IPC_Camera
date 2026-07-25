/*
 * @FilePath     : HeadDetect.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-22 20:32:40
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-06-05 14:05:37
 * @Description  : 人头检测算法
 */
#include "HeadDetect.hpp"

#include <cstring>

#include "dlog.h"

InferenceV1_0_NS::CHeadDetect::CHeadDetect(std::string strModelPath)
    : CCVInferenceRK(strModelPath)
{
    /* 后处理初始化 */
    m_postProcess = new PostProcessV1_0_NS::cYOLOV5PostProcess;

    /* 参数默认配置 */
    m_stExParam.fBoxThreshold = m_fBoxThreshold;
    m_stExParam.fNmsThreshold = m_fNmsThreshold;
}

InferenceV1_0_NS::CHeadDetect::~CHeadDetect()
{
    if (m_postProcess)
    {
        /* 后处理去初始化 */
        delete m_postProcess;
        m_postProcess = nullptr;
    }
}

/* 推理数据 */
bool InferenceV1_0_NS::CHeadDetect::inference(
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

    /* 使用模型推理前的相关变量判断 */
    bool bIsInfe = inferenceInfe(stInData);
    if (!bIsInfe)
    {
        return false;
    }
    cv::cvtColor(stInData.inMat, stInData.inMat, cv::COLOR_BGR2RGB);
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
    /* 输出数据清空 */
    if (!vOutData.empty())
    {
        vOutData.clear();
    }

    m_nCLASS_NUM = (m_vOutputAttrs[0].dims[1] / 3) - 5;

    /* 后处理 */
    m_postProcess->postProcess(
        (float*)m_pOutputs[0].buf,
        (float*)m_pOutputs[1].buf,
        (float*)m_pOutputs[2].buf,
        m_nLimitHeight,
        m_nLimitWidth,
        m_stExParam.fBoxThreshold,
        m_stExParam.fNmsThreshold,
        m_nCLASS_NUM,
        vOutData);
    /* 必须释放 */
    m_pModel->releaseOutputs(m_pOutputs, m_vOutputAttrs.size());

    return true;
}
