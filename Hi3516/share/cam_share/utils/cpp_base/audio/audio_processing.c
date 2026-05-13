/**
 * @FilePath     : audio_processing.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-01-06 17:24:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-09 14:34:49
 * @Description  : pcm音频处理。复制声道、混音等
 */

#include "audio_processing.h"
#include <stdio.h>
#include <math.h>

void audio_processing_splitStereoChannels(int16_t *input, int8_t *left, int8_t *right, int numSamples)
{
    // 检查输入参数的有效性
    if (input == NULL || left == NULL || right == NULL || numSamples < 0)
    {
        fprintf(stderr, "Invalid input parameters.\n");
        return;
    }
    for (int i = 0; i < numSamples; i += 2)
    {
        left[i / 2] = input[i];
        right[i / 2] = input[i + 1];
    }
}

void audio_processing_extractLeftChannel(int16_t *input, int16_t *left, int numSamples)
{
    // 检查输入参数的有效性
    if (input == NULL || left == NULL || numSamples < 0)
    {
        fprintf(stderr, "Invalid input parameters.\n");
        return;
    }
    for (int i = 0; i < numSamples; i += 2)
    {
        left[i / 2] = input[i];
    }
}

void audio_processing_convertMonoToStereo(const char *monoData, int monoSize, char *stereoData)
{
    // 检查输入参数的有效性
    if (monoData == NULL || stereoData == NULL || monoSize < 0)
    {
        fprintf(stderr, "Invalid input parameters.\n");
        return;
    }
    if (monoData && stereoData)
    {
        // 采样点数 = 字节数 除以 2
        int numSamples = monoSize / 2;

        const int16_t *monoSamples = (const int16_t *)monoData;
        int16_t *stereoSamples = (int16_t *)stereoData;

        for (int i = 0; i < numSamples; i++)
        {
            stereoSamples[2 * i] = monoSamples[i];     // 左声道
            stereoSamples[2 * i + 1] = monoSamples[i]; // 右声道
        }
    }
}

int audio_processing_mix(int8_t *srcData, int8_t *dstData, int nSize)
{
    // 检查输入参数的有效性
    if (srcData == NULL || dstData == NULL || nSize < 0)
    {
        fprintf(stderr, "Invalid input parameters.\n");
        return -1;
    }
    int i = 0;
    short *newSrc = (short *)srcData;
    short *newDst = (short *)dstData;
    for (i = 0; i < nSize / 2; ++i)
    {
        int nSrc = 0, nDst = 0, nSample = 0;
        nSrc = newSrc[i];
        nDst = newDst[i];

        nSample = (nSrc + nDst) - ((nSrc * nDst) >> 0x10);
        if (nSample > 32767)
        {
            nSample = 32767;
        }
        else if (nSample < -32768)
        {
            nSample = -32768;
        }
        newDst[i] = nSample;
    }
    return 0;
}

int audio_processing_mixToMono(int8_t *srcData, int8_t *dstData, int nSize)
{
    // 检查输入参数的有效性
    if (srcData == NULL || dstData == NULL || nSize < 0)
    {
        fprintf(stderr, "Invalid input parameters.\n");
        return -1;
    }
    if (nSize % 4 != 0)
    {
        // 输入数据长度必须是4的倍数（每个采样点为2字节，每个通道有两个采样点）
        return -1;
    }

    int i = 0;
    short *src = (short *)srcData; // 源数据转换为short指针
    short *dst = (short *)dstData; // 目标数据转换为short指针

    for (i = 0; i < nSize / 4; ++i)
    {
        int left = src[2 * i];      // 左声道采样点
        int right = src[2 * i + 1]; // 右声道采样点

        // 混合算法（加权平均）
        int mixedSample = (left + right) - ((left * right) >> 0x10);
        // 防止溢出
        if (mixedSample > 32767)
        {
            mixedSample = 32767;
        }
        else if (mixedSample < -32768)
        {
            mixedSample = -32768;
        }

        dst[i] = (short)mixedSample;
    }

    return 0;
}

void audio_processing_volChange(int8_t *data, int bytes, float shift)
{
    short *ps, *pe;
    int num;

    ps = (short *) data;
    pe = (short *) (data + bytes);

    while (ps < pe)
    {
        num = (*ps) * shift;
        if (num > 32767)
        {
            *ps = 32767;
        }
        else if (num < -32768)
        {
            *ps = -32768;
        }
        else
        {
            *ps = num;
        }
        ++ps;
    }
}

float audio_processing_calculateRMS(const short *pSamples, int nSampleCount)
{
    if (!pSamples || nSampleCount <= 0)
    {
        return 0.0f;
    }

    double sum = 0.0;
    for (int i = 0; i < nSampleCount; i++)
    {
        sum += (double) pSamples[i] * pSamples[i];
    }

    return (float) sqrt(sum / nSampleCount);
}

float audio_processing_convertRMSToDecibel(float fRMS, int nBitsPerSample)
{
    if (fRMS <= 0.0f)
    {
        return -96.0f; /* 静音阈值 */
    }

    /* 计算最大采样值 */
    float fMaxSample = (float) ((1 << (nBitsPerSample - 1)) - 1);

    /* 转换为分贝: dB = 20 * log10(RMS / Reference) */
    float fDB = 20.0f * log10f(fRMS / fMaxSample);

    /**
     * 校准至声压级(SPL)
     * 假设0dBFS(Full Scale)对应94dB SPL(标准声压级参考)
     * 可根据实际麦克风灵敏度调整
     */
    fDB += 94.0f;

    /* 限制范围 */
    if (fDB < 0.0f)
        fDB = 0.0f;
    if (fDB > 140.0f)
        fDB = 140.0f;

    return fDB;
}