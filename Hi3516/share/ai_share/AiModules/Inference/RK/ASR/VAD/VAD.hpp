/**
 * @file VAD.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-06-06
 *
 * @brief
 */
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "AVInferenceRK.hpp"
#include "iostream"
#include "CircularBuffer.hpp"

namespace Inference_NS
{

    class CVAD : public CAVInferenceRK
    {
    public:
        CVAD(std::string strConfigPath);
        ~CVAD();

        /**
         * @brief 特征提取
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ASRData_S>&] stASRData: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(
            Inference_NS::InputData_S stInputData,
            Inference_NS::ASRData_S &stASRData);

    private:
        bool run(float *pSamples, int32_t nDataSize, float &fConfidence);
        bool isSpeech(float fProb, float fThreshold, float fMinSpeechSamples, float fMinSilenceSamples);

    private:
        /* 存储输入的音频数据 */
        std::vector<float> m_vDataBuf;

        int m_nSampleRate = 16000;
        /* 窗口 */
        int32_t m_nWindowSize = 512;
        int32_t m_nWindowOverlap = 0;

        /* 后处理参数 */
        bool m_bTriggered = false;
        int32_t m_nTempStart = 0;
        int32_t m_nTempEnd = 0;
        int32_t m_nCurrentSample = 0;
        /* 长音频处理 */
        int32_t m_nMaxUtteranceLength = m_nSampleRate * 20;
        float m_fThresholdLong = 0.35;
        float m_fMinSilenceSamplesLong = m_nSampleRate * 0.1;
        /* 短音频处理 */
        float m_fThreshold = 0.2;
        float m_fMinSpeechSamples = m_nSampleRate * 0.25;
        float m_fMinSilenceSamples = m_nSampleRate * 0.5;
    };

} // namespace Inference_NS
