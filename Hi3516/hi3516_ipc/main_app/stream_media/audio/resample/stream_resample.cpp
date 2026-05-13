/**
 * @FilePath     : stream_resample.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-29 10:56:28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-03 19:51:06
 * @Description  : stream 音频重采样
 */

#include "stream_resample.h"
#include "dlog.h"

HiResample_S *streamResample_init(Audio_NS::AudioConfig_S stAudioConfig)
{
    if (stAudioConfig.enFormat != Audio_NS::AudioFormat_E::G711A &&
        stAudioConfig.enFormat != Audio_NS::AudioFormat_E::G711U &&
        stAudioConfig.enFormat != Audio_NS::AudioFormat_E::G726)
    {
        dlog_info("音频格式不为8KHz,不使用重采样");
        return nullptr;
    }

    HiResampleNeedParam_S stNeedParam;
    /* 默认两通道，需同时采集mic、Linein */
    stNeedParam.nSoundNum = 1;
    stNeedParam.enInSampleRate = OT_AUDIO_SAMPLE_RATE_16000;
    stNeedParam.enOutSampleRate = OT_AUDIO_SAMPLE_RATE_8000;

    HiResample_S *pHandle = mppResample_alloc(stNeedParam);
    if (pHandle == NULL)
    {
        dlog_error("分配音频重采样句柄失败");
        return nullptr;
    }

    /* 初始化音频重采样 */
    if (pHandle->mppResample_init(pHandle) != TD_SUCCESS)
    {
        dlog_error("初始化音频重采样失败");
        return nullptr;
    }

    dlog_info("音频流重采样初始化成功");
    return pHandle;
}

int streamResample_uninit(HiResample_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    /* 去初始化音频流重采样 */
    if (pHandle->mppResample_uninit(pHandle) < TD_SUCCESS)
    {
        dlog_error("去初始化音频重采样失败");
        return ERR;
    }

    mppResample_release(pHandle);

    dlog_info("音频流重采样去初始化成功");
    return OK;
}
