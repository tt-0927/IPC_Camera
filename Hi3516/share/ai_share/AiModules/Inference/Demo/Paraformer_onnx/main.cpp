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
#include <chrono>

#include "VAD.hpp"
#include "sdk_network.h"
#include "Paraformer.hpp"
#include "CTTransformerOnlinePunct.hpp"
#include "kaldi-native-fbank/csrc/online-feature.h"

using namespace Inference_NS;

Sdk_Net_Handle_t g_communicate_voice_handle = NULL;
std::vector<std::vector<int16_t>> startVideo; /* VAD前数据 */
std::vector<std::vector<float>> fStartVideo;  /* VAD前数据 */
int nendVideo = 0;                            /* VAD后数据 */
std::vector<float> lastFeature;
int nCountStop = 0;
std::vector<std::string> vZF; /* 用于存储结尾后，@@中断的字符 */
bool bStartVedie = true; /* 讲话的开始 */

CParaformer *ParafDemo = nullptr;
CCTTransformerOnlinePunct *PunctDemo = nullptr;
CVAD *VADDemo = nullptr;
/* knf特征提取 */
knf::FbankOptions m_stFbankOpts; /* fbank特征提取相关的参数 */
std::unique_ptr<knf::OnlineFbank> pFbank = nullptr;
int nSampleRate = 16000;
int32_t nFrameIndex = 0;
int32_t nLastFrameIndex = 0;
int32_t nFeatureDim = 80;
int32_t nSegment = 61;
bool bStart = false;

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

/* 将在线 paraformer 解码器的结果转换为在线识别器结果 */
std::string convert(std::vector<std::string> vInTokens, std::vector<std::string> &vOutTokens)
{
    // 初始化转换后的识别结果对象
    vOutTokens.reserve(vInTokens.size()); // 预分配 tokens 容量，提高性能

    std::string text; // 存储最终的识别文本

    // 当前 token 以 "@@" 结尾时设置 mergeable 为 true，表示可合并
    bool mergeable = false;

    for (int32_t i = 0; i != vInTokens.size(); ++i)
    {
        // 获取当前 token 的符号字符串
        auto sym = vInTokens[i];
        vOutTokens.push_back(sym); // 添加到结果 tokens 中

        // 判断当前符号是否不以 "@@" 结尾
        if ((sym.back() != '@') || (sym.size() > 2 && sym[sym.size() - 2] != '@'))
        {
            const uint8_t *p = reinterpret_cast<const uint8_t *>(sym.c_str());
            if (p[0] < 0x80)
            {
                // ASCII 字符处理
                if (mergeable)
                {
                    // 上一个字符是 "@@"，继续合并
                    mergeable = false;
                    text.append(sym);
                }
                else
                {
                    // 新增空格后添加当前字符
                    text.append(" ");
                    text.append(sym);
                }
            }
            else
            {
                // 非 ASCII 字符处理
                mergeable = false;

                // 如果不是第一个字符，且上一个字符是 ASCII，则添加空格
                if (i > 0)
                {
                    const uint8_t p = reinterpret_cast<const uint8_t *>(vInTokens[i - 1].c_str())[0];
                    if (p < 0x80)
                    {
                        text.append(" ");
                    }
                }
                text.append(sym);
            }
        }
        else
        {
            // 当前符号以 "@@" 结尾
            sym = std::string(sym.data(), sym.size() - 2); // 去除 "@@"
            if (mergeable)
            {
                // 继续合并
                text.append(sym);
            }
            else
            {
                // 新增空格后合并
                text.append(" ");
                text.append(sym);
                mergeable = true;
            }
        }
    }
    return text; // 返回转换后的识别结果
}

void getTime()
{
    auto now = std::chrono::system_clock::now();
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch())
                            .count();
    std::flush(std::cout);
    std::cout << "[" << timestamp_ms << "]";
    std::flush(std::cout); /* 强制刷新缓冲区 */
}

bool detect()
{
    Inference_NS::AVInputData_S stInputData;
    Inference_NS::ASRData_S stASRData;
    int nFrameIndex = pFbank->NumFramesReady();
    if (nFrameIndex >= (nSegment + nLastFrameIndex))
    {
        // printf("--[%d]\n", nFrameIndex);
        stInputData.nFrameIndex = nLastFrameIndex;
        stInputData.vFeature.resize(nSegment * nFeatureDim);
        for (int i = 0; i < nSegment; i++)
        {
            const float *pframe = pFbank->GetFrame(i + nLastFrameIndex);
            memcpy(stInputData.vFeature.data() + i * nFeatureDim,
                   pframe,
                   nFeatureDim * sizeof(float));
        }
        lastFeature = stInputData.vFeature;
        ParafDemo->inference(stInputData, stASRData);
        std::vector<std::string> vOutTokens;
        /* 缓存不完整的英文 */
        stASRData.vTexts.insert(stASRData.vTexts.begin(), vZF.begin(), vZF.end());
        vZF.clear();
        for (int ii = stASRData.vTexts.size() - 1; ii >= 0; ii--)
        {
            if (stASRData.vTexts[ii].find('@') != std::string::npos)
            {
                vZF.push_back(stASRData.vTexts[ii]);
                stASRData.vTexts.pop_back();
            }
            else
            {
                break;
            }
        }

        std::string strRes = convert(stASRData.vTexts, vOutTokens);
        Inference_NS::InputData_S stPunctInputData;
        Inference_NS::ASRData_S stPunctASRData;
        stPunctInputData.strText = strRes;
        if (strRes.size() > 0)
        {
            if (!PunctDemo->inference(stPunctInputData, stPunctASRData))
            {
                printf("PunctDemo 推理失败\n");
            }
        }

        /* 音频开始时间戳 */
        if(bStartVedie)
        {
            getTime();
            bStartVedie=false;
        }
        std::flush(std::cout);
        std::cout << stPunctASRData.strText;
        std::flush(std::cout); /* 强制刷新缓冲区 */

        nLastFrameIndex += nSegment;
        /* 丢弃之前的特征 */
        pFbank->Pop(nSegment);
    }
    return true;
}
bool detectEnd()
{
    int nFrameIndex_ = pFbank->NumFramesReady();
    int nC = (nFrameIndex_ - nLastFrameIndex) / nSegment;
    for (int i = 0; i < nC; i++)
    {
        detect();
    }

    nFrameIndex_ = pFbank->NumFramesReady();
    int nUser = nFrameIndex_ - nLastFrameIndex;
    Inference_NS::AVInputData_S stInputData;
    Inference_NS::ASRData_S stASRData;
    stInputData.nFrameIndex = nLastFrameIndex;
    stInputData.vFeature.resize(nSegment * nFeatureDim);
    for (int i = 0; i < nUser; i++)
    {
        const float *pframe = pFbank->GetFrame(i + nLastFrameIndex);
        memcpy(stInputData.vFeature.data() + i * nFeatureDim,
               pframe,
               nFeatureDim * sizeof(float));
    }
    /* 解决尾字丢失问题 */
    if (nUser < nSegment)
    {
        stInputData.vFeature.insert(stInputData.vFeature.end(), lastFeature.begin(), lastFeature.begin() + (nSegment - nUser) * nFeatureDim);
    }

    ParafDemo->inference(stInputData, stASRData);
    std::vector<std::string> vOutTokens;
    std::string strRes = convert(stASRData.vTexts, vOutTokens);
    Inference_NS::InputData_S stPunctInputData;
    Inference_NS::ASRData_S stPunctASRData;
    stPunctInputData.strText = strRes;
    if (strRes.size() > 0)
    {
        if (!PunctDemo->inference(stPunctInputData, stPunctASRData))
        {
            printf("PunctDemo 推理失败\n");
        }
    }

    std::flush(std::cout);
    std::cout << stPunctASRData.strText;
    std::flush(std::cout); /* 强制刷新缓冲区 */

    nLastFrameIndex += nUser;
    /* 丢弃之前的特征 */
    pFbank->Pop(nUser);

    return true;
}

void save_pcm(const void *pData, size_t nSize)
{
    FILE *fp = fopen("./test_client.pcm", "ab"); //"ab" 追加
    if (!fp)
    {
        perror("Failed to open ./test.pcm");
        return;
    }

    size_t written = fwrite(pData, 1, nSize, fp);
    if (written != nSize)
    {
        fprintf(stderr, "Warning: Only wrote %zu of %zu bytes\n", written, nSize);
    }

    fclose(fp);
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
        if (!VADDemo->inference(stInputData, stASRData))
        {
            printf("VAD模型推理失败\n");
        }
        if (stASRData.bSpeech)
        {
            for (int j = 0; j < startVideo.size(); j++)
            {
                pFbank->AcceptWaveform(nSampleRate, fStartVideo[j].data(), fStartVideo[j].size());
                save_pcm(startVideo[j].data(), startVideo[j].size() * sizeof(int16_t));
            }
            startVideo.clear();
            fStartVideo.clear();
            pFbank->AcceptWaveform(nSampleRate, vfOutData.data(), vfOutData.size());
            save_pcm(vfInData.data(), vfInData.size() * sizeof(int16_t));
            detect();
            bStart = true;
            // printf("*********非静音\n");
            nendVideo = 0;
        }
        else
        {
            if (!bStart)
            {
                if (startVideo.size() < 20) /* 缓存静音的前20帧 */
                {
                    startVideo.push_back(vfInData);
                    fStartVideo.push_back(vfOutData);
                }
                else
                {
                    if (!startVideo.empty())
                    {                                           // 1. 检查非空
                        startVideo.erase(startVideo.begin());   // 2. 删除第一个元素
                        fStartVideo.erase(fStartVideo.begin()); // 2. 删除第一个元素
                    }
                    startVideo.push_back(vfInData); // 3. 尾部插入新元素
                    fStartVideo.push_back(vfOutData);
                }
            }
            else
            {
                if (nendVideo < 3)
                {
                    nendVideo++;
                    save_pcm(vfInData.data(), vfInData.size() * sizeof(int16_t));
                    pFbank->AcceptWaveform(nSampleRate, vfOutData.data(), vfOutData.size());
                    return 0;
                }
                bStart = false;
                pFbank->InputFinished();
                detectEnd();
                /* 缓存清空 */
                ParafDemo->clearCache();
                std::string endStr1 = PunctDemo->clearArrCache();
                std::flush(std::cout);
                std::cout << endStr1 << std::endl;
                std::flush(std::cout);

                nFrameIndex = 0;
                nLastFrameIndex = 0;
                /* 重新初始化特征提取对象 */
                pFbank = std::make_unique<knf::OnlineFbank>(m_stFbankOpts);
                bStart = false;
                std::flush(std::cout);
                getTime();
                std::cout << "\n=============================================" << std::endl;
                std::flush(std::cout);
                nCountStop = 0;

                bStartVedie = true;
            }
            // printf("------静音\n");
        }
    }
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
    if (argc < 6)
    {
        std::cerr << "Usage: " << argv[0] << " <encode_config_path> <decode_config_path> <tokens_path> <punctx_config_path> <vad_config_path> <VAD_START>" << std::endl;
        return -1;
    }

    std::string encodePath = argv[1];
    std::string decodePath = argv[2];
    std::string tokens = argv[3];
    std::string punctPath = argv[4];

    /* 是否开启端点检测 */
    std::string vadPath = argv[5];
    std::string vadStart = argv[6];

    VADDemo = new CVAD(vadPath);
    bool bT = VADDemo->init();
    if (!bT)
    {
        printf("初始化参数识别\n");
        exit(0);
    }

    /* 初始化模型 */
    ParafDemo = new CParaformer(encodePath, decodePath, tokens);
    bT = ParafDemo->init();
    if (!bT)
    {
        printf("初始化参数识别\n");
        exit(0);
    }
    PunctDemo = new CCTTransformerOnlinePunct(punctPath);
    bT = PunctDemo->init();
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

        pFbank = std::make_unique<knf::OnlineFbank>(m_stFbankOpts);
    }

    nFeatureDim = pFbank->Dim();
    voice_clint_communication();
    while (1)
    {
        usleep(50 * 1000);
    }

    // for (int i = 0; i < 11; i++)
    // {
    //     std::string strId = "wav/" + std::to_string(i);
    //     std::vector<float> vfOutData = read_pcm(strId);
    //     pFbank->AcceptWaveform(nSampleRate, vfOutData.data(), vfOutData.size());
    //     pFbank->InputFinished();
    //     detect();
    //     detectEnd();
    //     /* 缓存清空 */
    //     ParafDemo->clearCache();
    //     std::string endStr1 = PunctDemo->clearArrCache();
    //     std::flush(std::cout);
    //     std::cout << endStr1 << std::endl;
    //     std::flush(std::cout);

    //     nFrameIndex = 0;
    //     nLastFrameIndex = 0;
    //     /* 重新初始化特征提取对象 */
    //     pFbank = std::make_unique<knf::OnlineFbank>(m_stFbankOpts);
    //     bStart = false;
    //     std::flush(std::cout);
    //     std::cout << "=============================================" << std::endl;
    //     std::flush(std::cout);
    // }

    delete VADDemo;
    delete ParafDemo;
    delete PunctDemo;
    return 0;
}
