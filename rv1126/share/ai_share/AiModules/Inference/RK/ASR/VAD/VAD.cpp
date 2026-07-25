/**
 * @file VAD.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-06
 *
 * @brief
 */
#include "VAD.hpp"
#include <memory>
#include <cstring>

Inference_NS::CVAD::CVAD(std::string strConfigPath)
    : CAVInferenceRK(strConfigPath)
{
}

Inference_NS::CVAD::~CVAD()
{
}

/* 推理数据 */
bool Inference_NS::CVAD::inference(
    Inference_NS::InputData_S stInputData,
    Inference_NS::ASRData_S &stASRData)
{
    if (!m_pModel && stInputData.pData == nullptr && stInputData.nDataSize <= 0)
    {
        return false;
    }
    /* 后处理阈值 */
    stASRData.bSpeech = false; /* 默认无人声 */
    float fThreshold, fMinSilenceSamples;

    if (stInputData.nDataSize > m_nMaxUtteranceLength)
    {
        fThreshold = m_fThresholdLong;
        fMinSilenceSamples = m_fMinSilenceSamplesLong;
    }
    else
    {
        fThreshold = m_fThreshold;
        fMinSilenceSamples = m_fMinSilenceSamples;
    }

    /* 滑动敞口 */
    m_nWindowSize = m_vInputAttrs[0].dims[1];
    int nWindowShift = m_nWindowSize + m_nWindowOverlap;
    m_vDataBuf.insert(m_vDataBuf.end(), stInputData.pData, stInputData.pData + stInputData.nDataSize);

    if (m_vDataBuf.size() < m_nWindowSize)
    {
        return true;
    }

    /* 滑动窗口，端点检测 */
    int32_t nK = (static_cast<int32_t>(m_vDataBuf.size()) - m_nWindowSize) / nWindowShift + 1;
    float *pDataBuf = m_vDataBuf.data();

    for (int32_t i = 0; i < nK; i++, pDataBuf += nWindowShift)
    {
        float fConfidence;
        if (!run(pDataBuf, nWindowShift, fConfidence))
        {
            return false;
        }
        bool bSpeech = isSpeech(fConfidence, fThreshold, m_fMinSpeechSamples, fMinSilenceSamples);
        stASRData.bSpeech = stASRData.bSpeech || bSpeech;
    }
    /* 更新buf */
    m_vDataBuf = std::vector<float>(
        pDataBuf,
        static_cast<float *>(m_vDataBuf.data()) + m_vDataBuf.size());

    return true;
}

/* 模型推理 */
bool Inference_NS::CVAD::run(float *pSamples, int32_t nDataSize, float &fConfidence)
{
    /* 输入预处理 */
    if (!inferenceInfe(nDataSize, 0))
    {
        return false;
    }
    memcpy(m_pInputs[0].buf, pSamples, nDataSize * sizeof(float));
    /* 运行 */
    if (!m_pModel->run(m_pInputs,
                       m_vInputAttrs.size(),
                       m_pOutputs,
                       m_vOutputAttrs.size()))
    {
        printf("推理失败-运行模型失败\n");
        return false;
    }

    /* 缓存分支拷贝 */
    for (int i = 1; i < m_vInputAttrs.size(); i++)
    {
        memcpy(m_pInputs[i].buf, m_pOutputs[i].buf, m_pInputs[i].size);
    }

    float *pOut0 = (float *)m_pOutputs[0].buf;
    fConfidence = pOut0[0];
    return true;
}

/* 是否讲话的后处理 */
bool Inference_NS::CVAD::isSpeech(float fProb, float fThreshold, float fMinSpeechSamples, float fMinSilenceSamples)
{
    m_nCurrentSample += m_nWindowSize;

    /* 结束端点变量归零 */
    if (fProb > fThreshold && m_nTempEnd != 0)
    {
        m_nTempEnd = 0;
    }
    if (fProb > fThreshold && m_nTempStart == 0)
    {
        m_nTempStart = m_nCurrentSample;
        return false;
    }

    if (fProb > fThreshold && m_nTempStart != 0 && !m_bTriggered)
    {
        if (m_nCurrentSample - m_nTempStart < fMinSpeechSamples)
        {
            return false;
        }
        m_bTriggered = true;
        return true;
    }
    if ((fProb < fThreshold) && !m_bTriggered)
    {
        /* 静音 */
        m_nTempStart = 0;
        m_nTempEnd = 0;
        return false;
    }
    /* 讲话 */
    if ((fProb > fThreshold - 0.15) && m_bTriggered)
    {
        return true;
    }

    /* 开始讲话 */
    if ((fProb > fThreshold) && !m_bTriggered)
    {
        m_bTriggered = true;
        return true;
    }

    if ((fProb < fThreshold) && m_bTriggered)
    {
        /* 结束讲话 */
        if (m_nTempEnd == 0)
        {
            m_nTempEnd = m_nCurrentSample;
        }

        /* 中途继续讲话 */
        if (m_nCurrentSample - m_nTempEnd < fMinSilenceSamples)
        {
            return true;
        }
        /* 讲话结束 */
        m_nTempStart = 0;
        m_nTempEnd = 0;
        m_bTriggered = false;
        return false;
    }

    return false;
}
