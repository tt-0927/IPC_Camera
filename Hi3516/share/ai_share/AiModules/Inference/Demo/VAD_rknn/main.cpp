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
#include "kaldi-native-fbank/csrc/online-feature.h"
#include <fstream>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <queue>
#include "sdk_network.h"

#include "VAD.hpp"

using namespace Inference_NS;

Sdk_Net_Handle_t g_communicate_voice_handle = NULL;

CVAD *VADDemo = nullptr;

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
        /* VAD推理 */
        Inference_NS::InputData_S stInputData;
        stInputData.pData = vfOutData.data();
        stInputData.nDataSize = vfOutData.size();
        Inference_NS::ASRData_S stASRData;
        if(!VADDemo->inference(stInputData, stASRData))
        {
            printf("VAD模型推理失败\n");
        }
        if(stASRData.bSpeech)
        {
            printf("检测到讲话\n");
        }

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
    if (argc < 1)
    {
        std::cerr << "Usage: " << argv[0] << " <vad_config_path>" << std::endl;
        return -1;
    }

    std::string vadPath = argv[1];
    /* 初始化模型 */
    VADDemo = new CVAD(vadPath);
    bool bT = VADDemo->init();
    if (!bT)
    {
        printf("初始化参数识别\n");
        exit(0);
    }

    voice_clint_communication();
    while (1)
    {
        usleep(50 * 1000);
    }
    delete VADDemo;
    return 0;
}
