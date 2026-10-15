/**
 * @FilePath     : stream_ao.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-10 20:48:56
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-27 10:32:01
 * @Description  : AO 音频流输出
 */

#include "stream_ao.h"
#include "dlog.h"
#include "IpcRet.h"

CStreamAo* CStreamAo::m_self = NULL;
std::mutex CStreamAo::m_mutex;

RkAo_S *streamAo_init(int enAoDevice, Audio_NS::AudioConfig_S stAudioConfig)
{
    int nRet = OK;
    RkAoNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(RkAoNeedParam_S));
    snprintf(stNeedParam.aDevName, sizeof(SOUND_CARD_SPEAK), "%s", SOUND_CARD_SPEAK);
    stNeedParam.enbitWidth = AUDIO_BIT_WIDTH_16;
    /*默认录音格式为16KHz*/
    stNeedParam.enSampleRate = AUDIO_SAMPLE_RATE_16000;
    stNeedParam.enSoundMode = AUDIO_SOUND_MODE_MONO;
    stNeedParam.u32FrameNum = FRAME_NUM_DEFAULT;
    /*
     * 每帧采样点数。原值 2048（@16kHz 即 128ms/帧）过大：AO 设备缓冲深度为
     * u32FrameNum × u32PointNumPerFrame，4 × 2048 采样 = 512ms，直接抬高了
     * 对讲、广播等实时音频的播放延迟。
     * 改为 320（@16kHz 即 20ms/帧）后缓冲降为 4 × 20ms = 80ms，且与对讲 PCM
     * 20ms 一帧的发送节拍正好一一对应（640 字节/帧），无需额外聚合。
     * 取值须在 RK 允许集合内：80/160/240/320/480/1024/2048（见 rk_comm_aio.h）。
     */
    stNeedParam.u32PointNumPerFrame = 320;
    stNeedParam.nChnNum = 1;
    stNeedParam.bResampleEnable = RK_FALSE;
    stNeedParam.enResampleRate = (AUDIO_SAMPLE_RATE_E)stAudioConfig.enSampRate;
    if(stAudioConfig.enSampRate != (Audio_NS::AudioSamprate_E)stNeedParam.enSampleRate)
    {
        stNeedParam.bResampleEnable = RK_TRUE;
    }
    /* 默认不开启VQE */
    stNeedParam.bVqeEnable = RK_FALSE;

    /*分配句柄*/
    RkAo_S *pHandle = rockitAo_alloc(stNeedParam);
    if (pHandle == nullptr)
    {
        dlog_error("分配rockit Ao句柄失败");
        return nullptr;
    }

    pHandle->stExParam.nDevSampleRate = AUDIO_SAMPLE_RATE_16000;
    if (pHandle->stNeedParam.enSoundMode == AUDIO_SOUND_MODE_MONO)
    {
        pHandle->stExParam.enTrackMode = AUDIO_TRACK_OUT_STEREO;
    }

    /*初始化*/
    nRet = pHandle->rockitAo_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("初始化 Ao 失败");
        free(pHandle);
        pHandle = nullptr;
        return nullptr;
    }

    dlog_info("音频流输出初始化成功");
    return pHandle;
}

int streamAo_uninit(RkAo_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    /*反初始化ao*/
    int nRet = pHandle->rockitAo_uninit(pHandle);
    if (nRet != RK_SUCCESS)
    {
        dlog_error("去初始化 Ao 失败");
        return ERR;
    }
    rockitAo_release(pHandle);

    dlog_info("音频流输出去初始化成功");
    return OK;
}

int streamAo_reboot(RkAo_S *pHandle, int nAoDevice, const Audio_NS::AudioConfig_S &stAudioConfig)
{
    if(nAoDevice >= AO_MAX_CHN || nAoDevice < AO_SPEAKER_CHN)
    {
        dlog_error("音频输出设备号错误");
        return ERR_PARAM;
    }

    int nRet = OK;
    if (pHandle != NULL)
    {
        nRet = streamAo_uninit(pHandle);
        if (nRet < OK)
        {
            dlog_error("反初始化ao失败");
            return nRet;
        }
    }
    if (pHandle == NULL)
    {
        pHandle = streamAo_init(nAoDevice, stAudioConfig);
        if (pHandle == NULL)
        {
            dlog_error("初始化ao失败");
            return nRet;
        }
    }
    return OK;
}

int streamAo_setVolume(RkAo_S *pHandle, int nVolume)
{
    if (pHandle == NULL)
    {
        dlog_error("句柄为空");
        return ERR_PTR_NULL;
    }

    if (nVolume < 0 || nVolume > 100)
    {
        dlog_error("音量值错误 Volume:%d", nVolume);
        return ERR_PARAM;
    }

    if (RK_SUCCESS != pHandle->rockitAo_set_volume(pHandle, nVolume))
    {
        dlog_error("设置AO设备音量失败");
        return ERR;
    }
    return OK;
}

CStreamAo::CStreamAo()
{
    GpioNeedParam_S stNeedParam;
    stNeedParam.nGpio = SPEAKER_GPIO;
    stNeedParam.nIsOutput = true;
    stNeedParam.nLowActive = false;
    stNeedParam.nValue = 1;
    m_pSpeaker = gpio_alloc(stNeedParam);
    if (m_pSpeaker == nullptr)
    {
        dlog_error("创建扬声器输出gpio失败");
    }
    /* 默认Line out不使能，静音 */
    stNeedParam.nGpio = LINEOUT_GPIO;
    stNeedParam.nIsOutput = true;
    stNeedParam.nLowActive = false;
    stNeedParam.nValue = 0;
    m_pLineOut = gpio_alloc(stNeedParam);
    if (m_pLineOut == nullptr)
    {
        dlog_error("创建线路输出gpio失败");
    }

    m_bInit = false;
}

CStreamAo::~CStreamAo()
{
    if (m_bInit)
    {
        uninit();
    }
    if (m_pSpeaker)
    {
        if (OK != gpio_release(m_pSpeaker))
        {
            dlog_error("释放扬声器输出gpio失败");
        }
    }
    if (m_pLineOut)
    {
        if (OK != gpio_release(m_pLineOut))
        {
            dlog_error("释放线路输出gpio失败");
        }
    }
}

int CStreamAo::init(Audio_NS::AudioConfig_S &stConfig)
{
    if (m_pSpeaker)
    {
        m_pSpeaker->gpio_init(m_pSpeaker);
    }
    if (m_pLineOut)
    {
        m_pLineOut->gpio_init(m_pLineOut);
    }

    /* 更新音频配置，调整音频输出类型 */
    //update_audioOutputType(stConfig.enOutputType);

    m_bInit = true;
    return OK;
}

int CStreamAo::uninit()
{
    /* 静音 */
    update_audioOutputType(Audio_NS::AudioOutputType_E::MUTE);

    if (m_pSpeaker)
    {
        m_pSpeaker->gpio_uninit(m_pSpeaker);
    }
    if (m_pLineOut)
    {
        m_pLineOut->gpio_uninit(m_pLineOut);
    }

    m_bInit = false;
    return OK;
}

int CStreamAo::update_audioOutputType(const Audio_NS::AudioOutputType_E &enType)
{
    using namespace Audio_NS;
    if (enType == AudioOutputType_E::SPEAKER)
    {
        /* 扬声器输出时，拉高Speaker使能脚，拉低LineOut的使能脚 */
        m_pSpeaker->set_value(m_pSpeaker->stNeedParam.nGpio, true);
        m_pLineOut->set_value(m_pLineOut->stNeedParam.nGpio, false);
    }
    else if (enType == AudioOutputType_E::LINEOUT)
    {
        /* LineOut输出时，拉高LineOut使能脚，拉低Speaker的使能脚 */
        m_pSpeaker->set_value(m_pSpeaker->stNeedParam.nGpio, false);
        m_pLineOut->set_value(m_pLineOut->stNeedParam.nGpio, true);
    }
    else if (enType == AudioOutputType_E::MUTE)
    {
        /* 静音时，拉低LineOut使能脚，拉低Speaker的使能脚 */
        m_pSpeaker->set_value(m_pSpeaker->stNeedParam.nGpio, false);
        m_pLineOut->set_value(m_pLineOut->stNeedParam.nGpio, false);
    }

    return OK;
}
