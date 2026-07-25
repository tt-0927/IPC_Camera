/*
 * @FilePath     : stream_aenc.c
 * @Author       : zhouzirui
 * @Date         : 2024-11-26 17:06:21
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-11-24 15:55:37
 * @Description  : 音频编码模块
 */

#include <stdio.h>

#include "stream_aenc.h"
#include "dlog.h"
#include "IpcRet.h"

RkAenc_S *streamAenc_init(Audio_NS::AudioConfig_S stAudioConfig)
{
    int nRet = OK;
    RkAenc_S *pRkAenc = (RkAenc_S *)malloc(sizeof(RkAenc_S));
    RkAencNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(RkAencNeedParam_S));

    stNeedParam.nChn = 0;

    switch (stAudioConfig.enFormat)
    {
        // case Audio_NS::AudioFormat_E::AAC:
        //     stNeedParam.enType = RK_AUDIO_ID_ACC;
        //     stNeedParam.nChannels = 2;
        //     stNeedParam.nSampleRate = static_cast<int>(stAudioConfig.enSampRate);
        //     stNeedParam.enBitWidth = AUDIO_BIT_WIDTH_16;
        //     // stNeedParam.nBitrate = 0;
        //     break;
        case Audio_NS::AudioFormat_E::G711A:
            stNeedParam.enType = RK_AUDIO_ID_PCM_ALAW;
            stNeedParam.nChannels = 1;
            stNeedParam.nSampleRate = static_cast<int>(stAudioConfig.enSampRate);
            stNeedParam.enBitWidth = AUDIO_BIT_WIDTH_16;
            // stNeedParam.nBitrate = 64000;
            break;
        case Audio_NS::AudioFormat_E::G711U:
            stNeedParam.enType = RK_AUDIO_ID_PCM_MULAW;
            stNeedParam.nChannels = 1;
            stNeedParam.nSampleRate = static_cast<int>(stAudioConfig.enSampRate);
            stNeedParam.enBitWidth = AUDIO_BIT_WIDTH_8;
            break;
        case Audio_NS::AudioFormat_E::G726:
            stNeedParam.enType = RK_AUDIO_ID_ADPCM_G726;
            stNeedParam.nChannels = 1;
            stNeedParam.nSampleRate = static_cast<int>(stAudioConfig.enSampRate);
            stNeedParam.enBitWidth = AUDIO_BIT_WIDTH_8;
            break;
        case Audio_NS::AudioFormat_E::G722_1:
            stNeedParam.enType = RK_AUDIO_ID_ADPCM_G722;
            stNeedParam.nChannels = 1;
            stNeedParam.nSampleRate = static_cast<int>(stAudioConfig.enSampRate);
            stNeedParam.enBitWidth = AUDIO_BIT_WIDTH_16;
            break;
        case Audio_NS::AudioFormat_E::MP2L2:
            stNeedParam.enType = RK_AUDIO_ID_MP2;
            stNeedParam.nChannels = 2;
            stNeedParam.nSampleRate = static_cast<int>(stAudioConfig.enSampRate);
            stNeedParam.enBitWidth = AUDIO_BIT_WIDTH_16;
            break;
        default:
            break;
    }

    /*分配句柄*/
    pRkAenc = rockitAenc_alloc(stNeedParam);
    if (pRkAenc == NULL)
    {
        dlog(LOG_ERROR, "rockit aenc alloc error");
        free(pRkAenc);
        pRkAenc = NULL;
        return NULL;
    }

    /*初始化*/
    nRet = pRkAenc->rockitAenc_init(pRkAenc);
    if (nRet != OK)
    {
        dlog(LOG_ERROR, "rockit aenc init error");
        free(pRkAenc);
        pRkAenc = NULL;
        return NULL;
    }
    
    dlog(LOG_INFO, "Aenc 音频编码 初始化成功");
    return pRkAenc;
}

void streamAenc_uninit(RkAenc_S *pRkAenc)
{
    if (pRkAenc)
    {
        RK_S32 nRet = pRkAenc->rockitAenc_uninit(pRkAenc);
        if (nRet != RK_SUCCESS)
        {
            dlog(LOG_ERROR, "rockit Aenc deinit error");
        }
        rockitAenc_release(pRkAenc);
        pRkAenc = NULL; // 释放后置为NULL
        dlog(LOG_INFO, "AENC 音频编码去初始化成功");
    }
    return;
}