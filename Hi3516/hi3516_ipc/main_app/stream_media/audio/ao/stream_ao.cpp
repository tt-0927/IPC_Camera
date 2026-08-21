/**
 * @FilePath     : stream_ao.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 20:08:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-23 16:06:19
 * @Description  : AO 音频流输出
 */

#include "stream_ao.h"
#include "dlog.h"

namespace
{
struct AudioOutputVolumeCalibration_S
{
    /* AO 输出音量最小值，单位 dB */
    int nMinVolume;
    /* AO 输出音量最大值，单位 dB */
    int nMaxVolume;
    /* 指数音量曲线底数 */
    double dCurveBase;
};

/**
 * @brief   : 获取当前设备的扬声器输出音量标定参数
 * @return  {AudioOutputVolumeCalibration_S} AO 输出音量范围和指数曲线参数
 * @note    : 2W 功放型号单独使用较低的起始 dB 和更陡的曲线，避免中低档音量过大。
 */
AudioOutputVolumeCalibration_S get_output_volume_calibration()
{
#if defined(DEVICE_TV_3852T) || defined(DEVICE_TV_3852TL)
    // return {-25, 6, 0.167355};
    return {-24, -12, 0.510204};
#elif defined(DEVICE_TV_3852H) || defined(DEVICE_TV_3852HL) || defined(DEVICE_TV_3852HZT)
    return {-25, -14, 0.049383};
#elif defined(DEVICE_TV_3852TLW) || defined(DEVICE_TV_3852TL4G)
    return {-25, -5, 0.444444};
#else
    return {-25, 6, 1.573};
#endif
}
} // namespace

HiAo_S *streamAo_init(int nAoDevice, const Audio_NS::AudioConfig_S &stAudioConfig)
{
    if(nAoDevice >= AO_MAX_CHN || nAoDevice < AO_SPEAKER_CHN)
    {
        dlog_error("音频采集设备号错误");
        return nullptr;
    }

    HiAoNeedParam_S stNeedParam;
    stNeedParam.nDevId = nAoDevice;
    stNeedParam.nChn = nAoDevice;
    stNeedParam.enBitWidth = OT_AUDIO_BIT_WIDTH_16;
    stNeedParam.enSampleRate = OT_AUDIO_SAMPLE_RATE_16000;
    stNeedParam.enSoundMode = OT_AUDIO_SOUND_MODE_MONO;
    stNeedParam.u32FrameNum = FRAME_NUM_DEFAULT;
    stNeedParam.u32PointNumPerFrame = EVS_SAMPLES_PER_FRAME;
    if(stAudioConfig.enFormat == Audio_NS::AudioFormat_E::AAC)
    {
        stNeedParam.u32PointNumPerFrame = AACLC_SAMPLES_PER_FRAME;
    }
    else if(stAudioConfig.enFormat == Audio_NS::AudioFormat_E::MP3)
    {
        stNeedParam.u32PointNumPerFrame = MP3_SAMPLES_PER_FRAME;
    }else if(stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A || stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U || stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G726)
    {
        stNeedParam.u32PointNumPerFrame = AMR_SAMPLES_PER_FRAME;
    }
    stNeedParam.nChnNum = 1;
    stNeedParam.nResampleEnable = TD_FALSE;
    stNeedParam.enResampleRate = (ot_audio_sample_rate)stAudioConfig.enSampRate;
    if(stAudioConfig.enSampRate != (Audio_NS::AudioSamprate_E)stNeedParam.enSampleRate)
    {
        stNeedParam.nResampleEnable = TD_TRUE;
        stNeedParam.u32FrameNum = 30; //开启重采样，buffer需要大于4096字节
    }
    stNeedParam.nVqeEnable = TD_TRUE;

    HiAo_S *pHandle = mppAo_alloc(stNeedParam);
    if (pHandle == NULL)
    {
        dlog_error("申请内存失败");
        return nullptr;
    }
    int nRet = OK;

    /* 根据扬声器功放标定设置默认输出音量范围和指数曲线。 */
    const AudioOutputVolumeCalibration_S stVolumeCalibration = get_output_volume_calibration();
    pHandle->stExParam.nVolume = stAudioConfig.u32OutputVolume;
    pHandle->stExParam.nMinVolume = stVolumeCalibration.nMinVolume;
    pHandle->stExParam.nMaxVolume = stVolumeCalibration.nMaxVolume;
    pHandle->stExParam.dVolumeCurveBase = stVolumeCalibration.dCurveBase;

    /* 去初始化ao */
    nRet = pHandle->mppAo_uninit(pHandle);
    if (nRet != OK)
    {
        dlog_error("mppAo_uninit error:%d", nRet);
        return nullptr;
    }

    /*初始化ao*/
    nRet = pHandle->mppAo_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("mppAo_init error:%d", nRet);
        return nullptr;
    }

    dlog_info("音频流输出初始化成功");
    return pHandle;
}

int streamAo_uninit(HiAo_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }
    int nRet = OK;
    /*反初始化ao*/
    nRet = pHandle->mppAo_uninit(pHandle);
    if (nRet < OK)
    {
        dlog_error("mppAo_uninit error");
        return nRet;
    }
    mppAo_release(pHandle);

    dlog_info("音频流输出去初始化成功");
    return OK;
}

int streamAo_reboot(HiAo_S *pHandle, int nAoDevice, const Audio_NS::AudioConfig_S &stAudioConfig)
{
    if(nAoDevice >= AO_MAX_CHN || nAoDevice < AO_SPEAKER_CHN)
    {
        dlog_error("音频采集设备号错误");
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
    // stNeedParam.nValue = 0;
    /* 上电默认关闭功放，避免AO/Codec尚未稳定时产生爆音 */
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
    update_audioConfig(stConfig);

    m_bInit = true;
    return OK;
}

int CStreamAo::uninit()
{
    /* 必须先静音再释放GPIO，避免GPIO进入高阻态时功放产生爆音 */
    setOutputEnable(false);
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

int CStreamAo::update_audioConfig(const Audio_NS::AudioConfig_S &stConfig)
{
    // using namespace Audio_NS;
    // if (stConfig.enOutputType == AudioOutputType_E::SPEAKER)
    // {
    //     /* 扬声器输出时，拉高Speaker使能脚，拉低LineOut的使能脚 */
    //     m_pSpeaker->set_value(m_pSpeaker->stNeedParam.nGpio, true);
    //     m_pLineOut->set_value(m_pLineOut->stNeedParam.nGpio, false);
    // }
    // else if (stConfig.enOutputType == AudioOutputType_E::LINEOUT)
    // {
    //     /* LineOut输出时，拉高LineOut使能脚，拉低Speaker的使能脚 */
    //     m_pSpeaker->set_value(m_pSpeaker->stNeedParam.nGpio, false);
    //     m_pLineOut->set_value(m_pLineOut->stNeedParam.nGpio, true);
    // }

    // return OK;
    std::lock_guard<std::mutex> lock(m_outputMutex);
    m_enOutputType = stConfig.enOutputType;

    /* 切换输出类型时先关闭两个输出，下一帧音频到来时再开启选中的输出 */
    int nRet = OK;
    if (m_pSpeaker)
    {
        nRet |= m_pSpeaker->set_value(m_pSpeaker->stNeedParam.nGpio, false);
    }
    if (m_pLineOut)
    {
        nRet |= m_pLineOut->set_value(m_pLineOut->stNeedParam.nGpio, false);
    }
    m_bOutputEnabled = false;
    return nRet;
}

int CStreamAo::setOutputEnable(bool enable)
{
    std::lock_guard<std::mutex> lock(m_outputMutex);
    if (m_bOutputEnabled == enable)
    {
        return OK;
    }

    int nRet = OK;

    /* 先关闭非选中输出，防止Speaker和LineOut同时使能 */
    if (m_pSpeaker)
    {
        const bool bSpeakerEnable = enable &&
            m_enOutputType == Audio_NS::AudioOutputType_E::SPEAKER;
        nRet |= m_pSpeaker->set_value(m_pSpeaker->stNeedParam.nGpio, bSpeakerEnable);
    }
    if (m_pLineOut)
    {
        const bool bLineOutEnable = enable &&
            m_enOutputType == Audio_NS::AudioOutputType_E::LINEOUT;
        nRet |= m_pLineOut->set_value(m_pLineOut->stNeedParam.nGpio, bLineOutEnable);
    }

    if (nRet == OK)
    {
        m_bOutputEnabled = enable;
    }

    return nRet;
}