/**
 * @FilePath     : stream_adec.cpp
 * @Author       : cyc
 * @Date         : 2025-04-03 14:33:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-11 17:08:16
 * @Description  : 音频解码模块
 */

#include "stream_adec.h"
#include "dlog.h"
#include "IpcRet.h"

RkAdec_S *streamAdec_init(int nAdecChn, Audio_NS::AudioConfig_S stAudioConfig)
{
    int nRet = OK;
    RkAdecNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(RkAdecNeedParam_S));

    stNeedParam.nChn = nAdecChn;
    /* pack方式解码 */
    stNeedParam.enMode = ADEC_MODE_STREAM;
    stNeedParam.nChannels = 1;
    stNeedParam.enType = RK_AUDIO_ID_PCM_ALAW;
    stNeedParam.nSampleRate = (int) stAudioConfig.enSampRate;
    stNeedParam.nBitPerCodedSample = AUDIO_BIT_WIDTH_16;

    if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A)
    {
        stNeedParam.enType = RK_AUDIO_ID_PCM_ALAW;
        stNeedParam.nSampleRate = 8000;
    }
    else if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U)
    {
        stNeedParam.enType = RK_AUDIO_ID_PCM_MULAW;
        stNeedParam.nSampleRate = 8000;
    }
    else if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G726)
    {
        stNeedParam.enType = RK_AUDIO_ID_ADPCM_G726;
        stNeedParam.nSampleRate = 8000;
    }

    /*分配句柄*/
    RkAdec_S *pHandle = rockitAdec_alloc(stNeedParam);
    if (pHandle == NULL)
    {
        dlog_error("申请内存失败");
        return nullptr;
    }

    /*初始化*/
    nRet = pHandle->rockitAdec_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("rockit adec init error");
        free(pHandle);
        pHandle = NULL;
        return NULL;
    }

    dlog_info("Adec 音频解码 初始化成功");
    return pHandle;
}

void streamAdec_uninit(RkAdec_S *pHandle)
{
    if (pHandle)
    {
        RK_S32 nRet = pHandle->rockitAdec_uninit(pHandle);
        if (nRet != RK_SUCCESS)
        {
            dlog_error("rockit Aenc deinit error");
        }
        rockitAdec_release(pHandle);
        dlog_info("ADEC 音频编码去初始化成功");
    }
    return;
}