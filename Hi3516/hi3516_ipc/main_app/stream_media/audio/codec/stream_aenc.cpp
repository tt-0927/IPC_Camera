/**
 * @FilePath     : stream_aenc.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 20:08:32
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-11 17:00:14
 * @Description  : AENC 音频编码
 */

#include "stream_aenc.h"
#include "dlog.h"

HiAenc_S *streamAenc_init(int nAencChn, Audio_NS::AudioConfig_S stAudioConfig)
{
    HiAencNeedParam_S stNeedParam;
    stNeedParam.nChn = nAencChn;
    stNeedParam.enSampleRate = (ot_audio_sample_rate)stAudioConfig.enSampRate;
    stNeedParam.enSoundMode = OT_AUDIO_SOUND_MODE_MONO;
    stNeedParam.u32PointNumPerFrame = EVS_SAMPLES_PER_FRAME;
    stNeedParam.u32BufSize = 30;
    stNeedParam.u32BitRate = (uint32_t)stAudioConfig.enBitRate;

    if(nAencChn == AENC_AAC_CHN) //固定编码AAC
    {
        stNeedParam.enAencType = OT_PT_AAC;
        stNeedParam.u32PointNumPerFrame = AACLC_SAMPLES_PER_FRAME;
        stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_16000;
    }
    else
    {
        if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A)
        {
            stNeedParam.enAencType = OT_PT_G711A;
            stNeedParam.u32PointNumPerFrame = AMR_SAMPLES_PER_FRAME;
            stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_8000;
        }
        else if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U)
        {
            stNeedParam.enAencType = OT_PT_G711U;
            stNeedParam.u32PointNumPerFrame = AMR_SAMPLES_PER_FRAME;
            stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_8000;
        }
        else if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G726)
        {
            stNeedParam.enAencType = OT_PT_G726;
            stNeedParam.u32PointNumPerFrame = AMR_SAMPLES_PER_FRAME;
            stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_8000;
        }
    }

    HiAenc_S *pHandle = mppAenc_alloc(stNeedParam);
    if (pHandle == NULL)
    {
        dlog_error("申请内存失败");
        return nullptr;
    }
    int nRet = OK;

    /* 去初始化aenc */
    nRet = pHandle->mppAenc_uninit(pHandle);
    if (nRet != OK)
    {
        dlog_error("mppAenc_uninit error:%d", nRet);
        return nullptr;
    }

    /* 初始化aenc */
    nRet = pHandle->mppAenc_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("mppAenc_init error:%d", nRet);
        return nullptr;
    }

    dlog_info("音频流编码初始化成功");
    return pHandle;
}

int streamAenc_uninit(HiAenc_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }
    int nRet = OK;
    /*反初始化aenc*/
    nRet = pHandle->mppAenc_uninit(pHandle);
    if (nRet < OK)
    {
        dlog_error("mppAenc_uninit error");
        return nRet;
    }
    mppAenc_release(pHandle);

    dlog_info("音频流编码去初始化成功");
    return OK;
}
