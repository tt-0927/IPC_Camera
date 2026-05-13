/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2025-04-29
 *
 * @brief
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <fstream>
#include <cstdint>
#include <fstream>
#include <cstdint>
#include <mutex>
#include <string>
#include <cctype>
#include <algorithm> // 必须包含此头文件

#include <iostream>
#include <chrono>
#include <queue>

#include "SenceVoice.hpp"
#include "kaldi-native-fbank/csrc/online-feature.h"

using namespace Inference_NS;

static void ToFloat(std::vector<int16_t> &in, int32_t num_channels,
                    std::vector<float> *out)
{
    out->resize(in.size() / num_channels);

    int32_t n = in.size();
    for (int32_t i = 0, k = 0; i < n; i += num_channels, ++k)
    {
        (*out)[k] = in[i] / 32768.0f; // 这里只取第一个通道
    }
}

std::vector<float> read_pcm(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    std::vector<float> audio_data;

    if (!file)
    {
        std::cerr << "Failed to open PCM file: " << path << std::endl;
        return audio_data;
    }

    // 读取整个文件内容
    std::vector<int16_t> buffer;
    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    buffer.resize(file_size / sizeof(int16_t));
    file.read(reinterpret_cast<char *>(buffer.data()), file_size);

    // 转换为 float，并归一化到 [-1.0, 1.0]
    audio_data.reserve(buffer.size());
    for (int16_t sample : buffer)
    {
        audio_data.push_back(static_cast<float>(sample) / 32768.0f);
    }

    return audio_data;
}

int main(int argc, char **argv)
{
    if (argc < 1)
    {
        std::cerr << "Usage: " << argv[0] << " <encode_config_path>" << std::endl;
        return -1;
    }

    std::string encodePath = argv[1];

    /* knf特征提取 */
    knf::FbankOptions m_stFbankOpts; /* fbank特征提取相关的参数 */
    knf::OnlineFbank *pFbank = nullptr;
    int nSampleRate = 16000;

    /* 初始化模型 */
    CSenceVoice *demo = new CSenceVoice(encodePath);
    bool bT = demo->init();
    if (!bT)
    {
        printf("初始化参数识别\n");
        exit(0);
    }
    if (!pFbank)
    {
        /* 设置帧处理参数 */
        m_stFbankOpts.frame_opts.samp_freq = nSampleRate;       /* 采样率16kHz */
        m_stFbankOpts.frame_opts.frame_shift_ms = 10;           /* 帧移10ms */
        m_stFbankOpts.frame_opts.frame_length_ms = 25;          /* 帧长25ms */
        m_stFbankOpts.frame_opts.dither = 0;                    /* 禁用抖动 */
        m_stFbankOpts.frame_opts.preemph_coeff = 0;             /* 禁用预加重 */
        m_stFbankOpts.frame_opts.remove_dc_offset = false;      /* 不移除直流分量 */
        m_stFbankOpts.frame_opts.window_type = "hann";          /* 使用汉宁窗 */
        m_stFbankOpts.frame_opts.round_to_power_of_two = false; /* 不将帧长调整为2的幂 */
        m_stFbankOpts.frame_opts.snip_edges = false;            /* 不裁剪边缘 */
        m_stFbankOpts.mel_opts.num_bins = 80;

        pFbank = new knf::OnlineFbank(m_stFbankOpts);
    }

    /* 设置语言 */
    demo->setLanguage(3, true);

    for (int i = 0; i < 11; i++)
    {
        /* 音频读取 */
        std::string strId = "wav/" + std::to_string(i);
        std::vector<float> vfOutData = read_pcm(strId);

        /* 音频组装 */
        pFbank->AcceptWaveform(nSampleRate, vfOutData.data(), vfOutData.size());
        pFbank->InputFinished();
        int nNumFrames = pFbank->NumFramesReady();
        int nFeatureDim = m_stFbankOpts.mel_opts.num_bins;
        Inference_NS::AVInputData_S stInputData;
        Inference_NS::ASRData_S stASRData;
        stInputData.vFeature.resize(nNumFrames * nFeatureDim);
        /* 特征堆叠 */
        for (int32_t i = 0; i < nNumFrames; ++i)
        {
            const float *pframe = pFbank->GetFrame(i);
            memcpy(stInputData.vFeature.data() + i * nFeatureDim,
                   pframe,
                   nFeatureDim * sizeof(float));
        }
        /* 模型推理 */
        demo->inference(stInputData, stASRData);

        /* 识别token对应的10s的时间占比 */
        std::cout << "识别到的token列表: [";
        for (int jj = 0; jj < stASRData.vTexts.size(); jj++)
        {
            std::cout << stASRData.vTexts[jj];
            if (jj != stASRData.vTexts.size() - 1)
                printf(",");
        }
        std::cout << "]" << std::endl;
        /* 完整结果暑促 */
        std::cout << "识别到的token对应的10s占比列表: [";
        for (int jj = 0; jj < stASRData.vTimestamp.size(); jj++)
        {
            std::cout << stASRData.vTimestamp[jj];
            if (jj != stASRData.vTimestamp.size() - 1)
                printf(",");
        }
        std::cout << "]" << std::endl;

        std::cout << "识别内容: [";
        for (int jj = 0; jj < stASRData.vTexts.size(); jj++)
        {
            std::cout << stASRData.vTexts[jj];
        }
        std::cout << "]" << std::endl;
        /* 重新初始化特征提取对象 */
        pFbank->Pop(nNumFrames);
        delete pFbank;
        pFbank = nullptr;
        pFbank = new knf::OnlineFbank(m_stFbankOpts);
        std::cout << "=============================================" << std::endl;
    }

    delete demo;
    if (pFbank)
    {
        delete pFbank;
    }
    return 0;
}
