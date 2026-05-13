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
#include "ZipFormer.hpp"
#include <fstream>
#include <cstdint>
#include "kaldi-native-fbank/csrc/online-feature.h"
#include <fstream>
#include <cstdint>

#include <iostream>
#include <chrono>
#include <queue>
#include "sdk_network.h"

using namespace Inference_NS;

Sdk_Net_Handle_t g_communicate_voice_handle = NULL;

/* knf特征提取 */
knf::FbankOptions m_stFbankOpts; /* fbank特征提取相关的参数 */
knf::OnlineFbank *pFbank = nullptr;
int nSampleRate = 16000;

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

/* 消息回调 */
int communicate_voice_DealCmd(NetCallbackMsg_t *param)
{
    if (param == NULL || param->recvvalue == NULL || param->sOperHandle == NULL)
    {
        dlog(LOG_ERROR, "communicate_voice_DealCmd is fail\n");
        return -1;
    }

    if (param->Code == 1111)
    {
        int16_t *pInput = reinterpret_cast<int16_t *>(param->recvvalue);
        size_t sample_count = param->nLen / sizeof(int16_t);
        std::vector<int16_t> vfInData(pInput, pInput + sample_count);
        std::vector<float> vfOutData;
        /* 转换数据 */
        ToFloat(vfInData, 1, &vfOutData);
        pFbank->AcceptWaveform(nSampleRate, vfOutData.data(), vfOutData.size());
    }
    return 0;
}

/* 网络状态 */
int communicate_voice_netstatus(
    Net_Status_t status,
    Sdk_Net_Handle_t handle,
    void *inparam)
{
    return 0;
}

/* 日志 */
int communicate_voice_logMsg(const char *format, ...)
{
    return 0;
}

void voice_clint_communication(void)
{
    InparamClientNet_t netparm;
    memset(&netparm, 0, sizeof(InparamClientNet_t));
    netparm.cmdfun = communicate_voice_DealCmd;
    netparm.statusFun = communicate_voice_netstatus;
    netparm.logFun = communicate_voice_logMsg;
    netparm.overtime = 500;
    netparm.nReconnect = 1;
    netparm.asynchronous = 1;
    strncpy(netparm.ip, "172.16.25.177", sizeof(netparm.ip));

    netparm.nPort = 12345;
    netparm.param = NULL;
    g_communicate_voice_handle = sdkclient_init_net(netparm);
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <encode_config_path> <decode_config_path> <joiner_config_path> <tokens_path>" << std::endl;
        return -1;
    }

    std::string encodePath = argv[1];
    std::string decodePath = argv[2];
    std::string joinerPath = argv[3];
    std::string tokens = argv[4];

    /* 初始化模型 */
    CZipFormer *demo = new CZipFormer(encodePath, decodePath, joinerPath, tokens);
    bool bT = demo->init();
    if (!bT)
    {
        printf("初始化参数识别\n");
        exit(0);
    }
    int num_trailing_blanks = 0;
    int tell_len = 0;
    /* 配置信息 */
    int nNumProcessedFrames = 0;
    int nFrameOffset = 0;
    int nMels, nSegment, nEncoderOutputT;
    demo->getInputLimit(
        nSampleRate, nMels, nSegment, nEncoderOutputT);

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

    voice_clint_communication();

    float frame_shift_s = 10 / 1000.0 * (nSegment / nEncoderOutputT);
    bool is_kong = false;
    while (1)
    {
        usleep(50 * 1000);

        Inference_NS::InputData_S stInputData;
        Inference_NS::ASRData_S     stASRData;
        std::vector<float> vInputData;
        int nNumFrames = pFbank->NumFramesReady();

        if (nNumFrames > (nSegment + nNumProcessedFrames))
        {
            vInputData.resize(nSegment * nMels);
            for (int i = 0; i < nSegment; i++)
            {
                const float *pframe = pFbank->GetFrame(i + nNumProcessedFrames);
                memcpy(vInputData.data() + i * nMels,
                       pframe,
                       nMels * sizeof(float));
            }
            stInputData.pData       = vInputData.data();
            stInputData.nDataSize   = vInputData.size();
            demo->inference(stInputData, stASRData);
        }
        else
        {
            continue;
        }

        for (int jj = 0; jj < stASRData.vTexts.size(); jj++)
        {
            float fTime = (nFrameOffset + stASRData.vTimestamp[jj]) * frame_shift_s;
            printf("%s", stASRData.vTexts[jj].c_str());
            // printf("[%f]-[%s] ", fTime, stASRData.vTexts[jj].c_str());
        }
        if (stASRData.vTexts.size() == 0)
        {
            std::cout << std::endl;

            /* 充值 */
            // pFbank->Reset();
            // nFrameOffset = 0;
            // nNumProcessedFrames = 0;
        }
        std::flush(std::cout); /* 强制刷新缓冲区 */

        nFrameOffset += nEncoderOutputT;
        nNumProcessedFrames += nSegment;
        /* 丢弃之前的特征 */
        pFbank->Pop(nSegment);
    }

    delete demo;
    if (pFbank)
    {
        delete pFbank;
    }
    return 0;
}
