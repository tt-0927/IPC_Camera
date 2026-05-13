/**
 * @FilePath     : stream_adec.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-05-13 15:41:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-11 16:59:37
 * @Description  : AENC 音频解码
 */

#include "stream_adec.h"
#include "dlog.h"

HiAdec_S *streamAdec_init(int nAdecChn, Audio_NS::AudioConfig_S stAudioConfig)
{
    stAudioConfig.enFormat = Audio_NS::AudioFormat_E::G711A;

    HiAdecNeedParam_S stNeedParam;
    stNeedParam.nChn = nAdecChn;
    stNeedParam.enSampleRate = (ot_audio_sample_rate)stAudioConfig.enSampRate;
    stNeedParam.enSoundMode = OT_AUDIO_SOUND_MODE_MONO;
    stNeedParam.enAdecMode = OT_ADEC_MODE_STREAM;
    stNeedParam.u32BufSize = 20;
    if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::AAC)
    {
        stNeedParam.enAdecType = OT_PT_AAC;
    }
    else if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A)
    {
        stNeedParam.enAdecType = OT_PT_G711A;
        stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_8000;
    }
    else if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U)
    {
        stNeedParam.enAdecType = OT_PT_G711U;
        stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_8000;
    }
    else if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G726)
    {
        stNeedParam.enAdecType = OT_PT_G726;
        stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_8000;
    }

    HiAdec_S *pHandle = mppAdec_alloc(stNeedParam);
    if (pHandle == NULL)
    {
        dlog_error("申请内存失败");
        return nullptr;
    }
    int nRet = OK;

    /* 去初始化adec */
    nRet = pHandle->mppAdec_uninit(pHandle);
    if (nRet != OK)
    {
        dlog_error("mppAdec_uninit error:%d", nRet);
        return nullptr;
    }

    /* 初始化adec */
    nRet = pHandle->mppAdec_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("mppAdec_init error:%d", nRet);
        return nullptr;
    }

    dlog_info("音频流解码初始化成功");
    return pHandle;
}

int streamAdec_uninit(HiAdec_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }
    int nRet = OK;
    /*反初始化adec*/
    nRet = pHandle->mppAdec_uninit(pHandle);
    if (nRet < OK)
    {
        dlog_error("mppAdec_uninit error");
        return nRet;
    }
    mppAdec_release(pHandle);

    dlog_info("音频流解码去初始化成功");
    return OK;
}
