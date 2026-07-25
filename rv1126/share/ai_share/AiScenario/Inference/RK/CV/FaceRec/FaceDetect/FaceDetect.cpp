/*
 * @FilePath     : FaceDetect.cpp
 * @Author       : lih lih@kfb.cn
 * @Date         : 2024-06-19 15:31:40
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-02 16:41:38
 * @Description  : 人脸检测算法（带有5个关键点）
 */
#include "FaceDetect.hpp"

#include <cstring>

#include "dlog.h"

InferenceV1_0_NS::CFaceDetect::CFaceDetect(std::string strModelPath)
    : CCVInferenceRK(strModelPath)
{
    /* 后处理类初始化 */
    m_postProcess = new PostProcessV1_0_NS::cRetinafacePostProcess;

    /* 参数默认配置 */
    m_stExParam.fBoxThreshold = m_fBoxThreshold;
    m_stExParam.fNmsThreshold = m_fNmsThreshold;
}

InferenceV1_0_NS::CFaceDetect::~CFaceDetect()
{
    if (m_postProcess)
    {
        /* 后处理去初始化 */
        delete m_postProcess;
        m_postProcess = nullptr;
    }
}

/* 推理数据 */
bool InferenceV1_0_NS::CFaceDetect::inference(
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

    /* 后处理 */
    m_postProcess->postProcess(
        (float*)m_pOutputs[0].buf,
        (float*)m_pOutputs[1].buf,
        (float*)m_pOutputs[2].buf,
        m_vOutputAttrs[0].dims[1],
        m_nLimitHeight,
        m_nLimitWidth,
        m_stExParam.fBoxThreshold,
        m_stExParam.fNmsThreshold,
        vOutData);

    /* 必须释放 */
    m_pModel->releaseOutputs(m_pOutputs, m_vOutputAttrs.size());

    return true;
}
