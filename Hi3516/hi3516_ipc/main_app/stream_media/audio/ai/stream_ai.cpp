/**
 * @FilePath     : stream_ai.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 20:08:04
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-20 09:47:34
 * @Description  : AI 音频流采集输入
 */

#include "stream_ai.h"
#include "stream_ao.h"
#include "dlog.h"

HiAi_S *streamAi_init(int nAiDevice, Audio_NS::AudioConfig_S stAudioConfig)
{
    HiAiNeedParam_S stNeedParam;
    stNeedParam.nChn = nAiDevice;
    stNeedParam.enBitWidth = OT_AUDIO_BIT_WIDTH_16;
    /*默认录音格式为16KHz*/
    stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_16000;
    stNeedParam.enSoundMode = OT_AUDIO_SOUND_MODE_STEREO;
    stNeedParam.u32FrameNum = FRAME_NUM_DEFAULT;
    stNeedParam.u32PointNumPerFrame = EVS_SAMPLES_PER_FRAME;
    stNeedParam.nChnNum = 1;
    if(stAudioConfig.enFormat == Audio_NS::AudioFormat_E::AAC)
    {
        stNeedParam.u32PointNumPerFrame = AACLC_SAMPLES_PER_FRAME;
    }
    else if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::MP3)
    {
        stNeedParam.u32PointNumPerFrame = MP3_SAMPLES_PER_FRAME;
    }
    else if (stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A
             || stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U
             || stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G726)
    {
        stNeedParam.u32PointNumPerFrame = AMR_SAMPLES_PER_FRAME;
    }
    stNeedParam.nResampleEnable = TD_FALSE;
    stNeedParam.enResampleRate = (ot_audio_sample_rate)stAudioConfig.enSampRate;
    if(stAudioConfig.enSampRate != (Audio_NS::AudioSamprate_E)stNeedParam.enSampleRate)
    {
        stNeedParam.nResampleEnable = TD_TRUE;
    }
    /*VQE支持的采样率为16KHz*/
    stNeedParam.nVqeEnable = TD_TRUE;
    /* 使用TALKV2 开启AEC */
    stNeedParam.enVqeType = AUDIO_VQE_TYPE_TALKV2;
    /* AEC 需配置AO 设备号与通道号 */
    stNeedParam.nAoDev = AO_SPEAKER_CHN;
    stNeedParam.nAoChn = AO_SPEAKER_CHN;

    HiAi_S *pHandle = mppAi_alloc(stNeedParam);
    if (pHandle == NULL)
    {
        dlog_error("申请内存失败");
        return nullptr;
    }
    int nRet = OK;

    pHandle->stExParam.bEnableNr = (td_bool)stAudioConfig.bDenoise;
    if (stAudioConfig.enInputType == Audio_NS::AudioInputType_E::MICIN)
    {
        pHandle->stExParam.enTrackMode = OT_AUDIO_TRACK_RIGHT_MUTE;
    }
    else if (stAudioConfig.enInputType == Audio_NS::AudioInputType_E::LINEIN)
    {
        pHandle->stExParam.enTrackMode = OT_AUDIO_TRACK_LEFT_MUTE;
    }

    /* 去初始化ai */
    nRet = pHandle->mppAi_uninit(pHandle);
    if (nRet != OK)
    {
        dlog_error("mppAi_uninit error:%d", nRet);
        return nullptr;
    }

    /* 初始化ai */
    nRet = pHandle->mppAi_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("mppAi_init error:%d", nRet);
        return nullptr;
    }

    dlog_info("音频流采集初始化成功");
    return pHandle;
}

int streamAi_uninit(HiAi_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }
    int nRet = OK;
    /*反初始化ai*/
    nRet = pHandle->mppAi_uninit(pHandle);
    if (nRet < OK)
    {
        dlog_error("mppAi_uninit error");
        return nRet;
    }
    mppAi_release(pHandle);

    dlog_info("音频流采集去初始化成功");
    return OK;
}

int streamAi_set_vqe_rnr(HiAi_S *pHandle, int nAiDevice, bool bDenoise)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    /* 设置录音噪声消除 */
    if (OK != pHandle->mppAi_whether_enable_vqe_rnr(pHandle, nAiDevice, (td_bool) bDenoise))
    {
        dlog_error("设置录音噪声消除失败");
        return ERR;
    }

    return OK;
}

int streamAi_set_track_mode(HiAi_S *pHandle, ot_audio_track_mode enTrackMode)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    /* 设置AI声道模式 */
    if (OK != pHandle->mppAi_set_track_mode(pHandle, enTrackMode))
    {
        dlog_error("设置AI声道模式失败");
        return ERR;
    }

    return OK;
}
