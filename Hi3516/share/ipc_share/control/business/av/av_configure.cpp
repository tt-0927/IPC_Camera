/**
 * @FilePath     : av_configure.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-25 20:14:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-09 09:57:58
 * @Description  : 音视频配置
 */

#include "av_configure.h"
#include <algorithm>

#ifdef DEVICE_TV_3882TI
#include "event_configure.h"
#endif

namespace
{
    /* 在字符串能力列表中判断目标值是否存在 */
    bool contains_value(const std::vector<std::string> &values, const std::string &target)
    {
        return std::find(values.begin(), values.end(), target) != values.end();
    }

    /* 在整型能力列表中判断目标值是否存在（采样率/码率） */
    bool contains_value(const std::vector<int> &values, int target)
    {
        return std::find(values.begin(), values.end(), target) != values.end();
    }

    /*
     * 范围匹配规则：
     * 1) 范围未启用时直接放行；
     * 2) 必须落在 [nMin, nMax]；
     * 3) nStep>0 时需满足从 nMin 起按步长离散可达。
     */
    bool match_range_value(int value, const Audio_NS::AudioRange_S &range)
    {
        if (!range.bEnable)
        {
            return true;
        }
        if (value < range.nMin || value > range.nMax)
        {
            return false;
        }
        if (range.nStep <= 0)
        {
            return true;
        }
        return ((value - range.nMin) % range.nStep) == 0;
    }
}

CAVConfigure::CAVConfigure()
    : m_videoConfig(VIDEO_CONFIG_FILE), m_audioConfig(AUDIO_CONFIG_FILE),
      m_audioCapabilitySet(AUDIO_CAPABILITY_SET_FILE),
      m_videoRoiConfig(ROI_CONFIG_FILE), m_areaCropConfig(AREA_CROP_CONFIG_FILE),
      m_capabilitySet(VIDEO_CAPABILITY_SET_FILE)
{
    m_setVideoConfigCallback = nullptr;
    m_setAudioConfigCallback = nullptr;
    m_setAoSpeakCallback = nullptr;
    m_setVideoRoiConfigCallback = nullptr;
    m_waitAoDrainedCallback = nullptr;
    m_nOriginalIFrameInterval = DEFAULTE_GOP;
    m_bIFrameIntervalModified = false;

    Audio_NS::AudioCapabilitySet_S stAudioCapabilitySet;
    m_audioCapabilitySet.get(stAudioCapabilitySet);
    if (stAudioCapabilitySet.aInputTypes.empty() ||
        stAudioCapabilitySet.aOutputTypes.empty() ||
        stAudioCapabilitySet.aFormats.empty() ||
        stAudioCapabilitySet.aFormatDetail.empty())
    {
        fill_default_audio_capability_set(stAudioCapabilitySet);
        m_audioCapabilitySet.set(stAudioCapabilitySet);
    }
}

CAVConfigure::~CAVConfigure()
{
}

void CAVConfigure::setVideoConfigCallback(const SetVideoConfigCallback &callback)
{
    m_setVideoConfigCallback = callback;
}

int CAVConfigure::update_configure(const Video_NS::VideoConfig_S& data)
{
    return m_videoConfig.set(data);
}

int CAVConfigure::set_configure(const Video_NS::VideoConfig_S& data)
{
    int nRet = OK;
    /*验证配置参数的有效性*/
    Video_NS::VideoConfig_S stVideoConfig = data;

/*TV-3882TI机型大模型开启推理，系统负载高，限制高分辨率和高画质，防止花屏*/
#ifdef DEVICE_TV_3882TI
    Alarm::LLMAISceneAnalysis_S stAISceneAnalysisCfg;
    if (CEventConfigure::instance()->get_configure(stAISceneAnalysisCfg) == 0)
    {
        if(stAISceneAnalysisCfg.bEnable)
        {
            if((stVideoConfig.stVideoResolution.nWidth >= PIXEL_WIDTH_2_5K) && (stVideoConfig.stVideoResolution.nHeight >= PIXEL_HEIGHT_2_5K))
            {
                stVideoConfig.stVideoResolution.nWidth  = PIXEL_WIDTH_2_5K;
                stVideoConfig.stVideoResolution.nHeight = PIXEL_HEIGHT_2_5K;
            }

             if(stVideoConfig.enImageQuality >= Video_NS::ImageQuality_E::MEDIUM)
            {
                stVideoConfig.enImageQuality = Video_NS::ImageQuality_E::MEDIUM;
            }
        }
    }
#endif

    /* 帧率为分数帧率时，修改gop */
    if (stVideoConfig.getFrameRateAsInt() > 0xFFFF)
    {
        /* 首次强制修改时，记录原始I帧间隔值 */
        if (!m_bIFrameIntervalModified)
        {
            m_nOriginalIFrameInterval = stVideoConfig.nIFrameInterval;
            m_bIFrameIntervalModified = true;
        }
        /* I帧间隔强制为1 */
        stVideoConfig.nIFrameInterval = 1;
    }
    /* 帧率为小于5帧时，修改gop */
    else if (stVideoConfig.getFrameRateAsInt() <= 5)
    {
        /* 首次强制修改时，记录原始I帧间隔值 */
        if (!m_bIFrameIntervalModified)
        {
            m_nOriginalIFrameInterval = stVideoConfig.nIFrameInterval;
            m_bIFrameIntervalModified = true;
        }
        /* I帧间隔强制为1 */
        stVideoConfig.nIFrameInterval = 1;
    }
    /* 帧率恢复正常时，恢复I帧间隔到原始值 */
    else if (m_bIFrameIntervalModified)
    {
        stVideoConfig.nIFrameInterval = m_nOriginalIFrameInterval;
        m_bIFrameIntervalModified = false;
    }

    /*调用回调通知StreamVideo更新*/
    if (m_setVideoConfigCallback)
    {
        nRet = m_setVideoConfigCallback(stVideoConfig);
        if (nRet != OK)
        {
            dlog_debug("调用回调通知StreamVideo更新失败:%d", nRet);
            return nRet;
        }
    }

    return m_videoConfig.set(stVideoConfig);
}

int CAVConfigure::get_configure(Video_NS::VideoConfig_S &data) const
{
    return m_videoConfig.get(data);
}

int CAVConfigure::get_configure(std::set<Video_NS::VideoConfig_S> &data) const
{
    return m_videoConfig.get(data);
}

void CAVConfigure::setVideoRoiConfigCallback(const SetVideoRoiConfigCallback &callback)
{
    m_setVideoRoiConfigCallback = callback;
}

int CAVConfigure::set_configure(const Video_NS::VideoRoiConfig_S &data)
{
    int nRet = OK;
    /*验证配置参数的有效性*/
    
    /*调用回调通知StreamVideo更新*/
    if (m_setVideoRoiConfigCallback)
    {
        nRet = m_setVideoRoiConfigCallback(data);
        if (nRet != OK)
        {
            return nRet;
        }
    }

    return m_videoRoiConfig.set(data);
    
}
int CAVConfigure::get_configure(Video_NS::VideoRoiConfig_S &data) const
{
    return m_videoRoiConfig.get(data);
}

int CAVConfigure::get_configure(std::set<Video_NS::VideoRoiConfig_S> &data) const
{
    return m_videoRoiConfig.get(data);
}

void CAVConfigure::setAreaCropConfigCallback(const SetAreaCropConfigCallback &callback)
{
    m_setAreaCropConfigCallback = callback;
}

int CAVConfigure::set_configure(const Video_NS::AreaCrop_S &data)
{
    int nRet = OK;
    /*验证配置参数的有效性*/

    /*调用回调通知StreamVideo更新*/
    if (m_setAreaCropConfigCallback)
    {
        nRet = m_setAreaCropConfigCallback(data);
        if (nRet != OK)
        {
            return nRet;
        }
    }

    return m_areaCropConfig.set(data);
}

int CAVConfigure::get_configure(Video_NS::AreaCrop_S &data) const
{
    return m_areaCropConfig.get(data);
}

int CAVConfigure::get_configure(std::set<Video_NS::AreaCrop_S> &data) const
{
    return m_areaCropConfig.get(data);
}

int CAVConfigure::set_configure(const Video_NS::VideoCapabilitySet_S &data)
{
    return m_capabilitySet.set(data);
}

int CAVConfigure::get_configure(Video_NS::VideoCapabilitySet_S &data) const
{
    return m_capabilitySet.get(data);
}

void CAVConfigure::setAudioConfigCallback(const SetAudioConfigCallback &callback)
{
    m_setAudioConfigCallback = callback;
}

int CAVConfigure::set_configure(const Audio_NS::AudioConfig_S &data)
{
    int nRet = OK;
    /*验证配置参数的有效性*/
    if (!is_audio_config_supported(data))
    {
        dlog_error("音频配置超出能力集范围");
        return ERR_PARAM;
    }
    
    /*调用回调通知StreamAudio更新*/
    if (m_setAudioConfigCallback)
    {
        nRet = m_setAudioConfigCallback(data);
        if (nRet != OK)
        {
            return nRet;
        }
    }

    return m_audioConfig.set(data);
}

int CAVConfigure::get_configure(Audio_NS::AudioConfig_S &data) const
{
    return m_audioConfig.get(data);
}

int CAVConfigure::set_configure(const Audio_NS::AudioCapabilitySet_S &data)
{
    return m_audioCapabilitySet.set(data);
}

int CAVConfigure::get_configure(Audio_NS::AudioCapabilitySet_S &data) const
{
    return m_audioCapabilitySet.get(data);
}

void CAVConfigure::setAoSpeakCallback(const AudioSpeakCallback &callback)
{
    m_setAoSpeakCallback = callback;
}

void CAVConfigure::setAoSpeakInfo(const Audio_NS::AoInfo_S &data) const
{
    if(m_setAoSpeakCallback)
    {
        m_setAoSpeakCallback(data);
    }
}

#if CAP_EVENT_AUDIO_PLAYBACK_V2
void CAVConfigure::setAudioAoIdleCallback(const std::function<void()> &callback)
{
    m_setAudioAoIdleCallback = callback;
}

void CAVConfigure::setAudioAoIdle() const
{
    if (m_setAudioAoIdleCallback)
    {
        m_setAudioAoIdleCallback();
    }
}
void CAVConfigure::setMuteAudioOutputCallback(const std::function<void()> &callback)
{
    m_muteAudioOutputCallback = callback;
}
void CAVConfigure::muteAudioOutput() const
{
    if (m_muteAudioOutputCallback)
    {
        m_muteAudioOutputCallback();
    }
}
#endif
#if CAP_IO_EXTERNAL_DDR_00S
void CAVConfigure::muteAudioOutput() const
{
    if (m_muteAudioOutputCallback)
    {
        m_muteAudioOutputCallback();
    }
}


void CAVConfigure::setMuteAudioOutputCallback(const MuteAudioOutputCallback &callback)
{
    m_muteAudioOutputCallback = callback;
}
#endif

int CAVConfigure::setAudioAoSampleRateCallback(const SetAudioAoSampleRateCallback &callback)
{
    if (!callback)
    {
        dlog_error("无效的回调函数 (nullptr或未绑定)");
        return ERR;
    }

    m_setAudioAoSampleRateCallback = callback;

    dlog_info("设置音频模块AO采样率回调函数成功");
    return OK;
}

int CAVConfigure::setAudioAoSampleRate(const Audio_NS::AudioSamprate_E enSampRate) const
{
    if (m_setAudioAoSampleRateCallback)
    {
        int nRet = m_setAudioAoSampleRateCallback(enSampRate);
        if (nRet != OK)
        {
            return nRet;
        }
    }
    else
    {
        dlog_warn("未设置回调函数");
        return ERR;
    }
    return OK;
}

int CAVConfigure::setWaitAoDrainedCallback(const WaitAoDrainedCallback &callback)
{
    if (!callback)
    {
        dlog_error("无效的 waitAoDrained 回调函数");
        return ERR;
    }
    m_waitAoDrainedCallback = callback;
    dlog_info("设置 AO 排空等待回调函数成功");
    return OK;
}

int CAVConfigure::waitAoDrained(int nChn, int nTimeoutMs) const
{
    if (!m_waitAoDrainedCallback)
    {
        dlog_warn("waitAoDrained 回调未设置");
        return ERR;
    }
    return m_waitAoDrainedCallback(nChn, nTimeoutMs);
}

bool CAVConfigure::is_audio_config_supported(const Audio_NS::AudioConfig_S &data) const
{
    Audio_NS::AudioCapabilitySet_S stAudioCapabilitySet;
    m_audioCapabilitySet.get(stAudioCapabilitySet);
    if (stAudioCapabilitySet.aInputTypes.empty() ||
        stAudioCapabilitySet.aOutputTypes.empty() ||
        stAudioCapabilitySet.aFormats.empty())
    {
        fill_default_audio_capability_set(stAudioCapabilitySet);
    }

    const std::string strInputType = Audio_NS::audioInputType_toString(data.enInputType);
    const std::string strOutputType = Audio_NS::audioOutputType_toString(data.enOutputType);
    const std::string strFormat = Audio_NS::audioFormat_toString(data.enFormat);
    const int nSampleRate = static_cast<int>(data.enSampRate);
    const int nBitRate = static_cast<int>(data.enBitRate);

    if (!contains_value(stAudioCapabilitySet.aInputTypes, strInputType))
    {
        dlog_error("音频输入类型[%s]不受支持", strInputType.c_str());
        return false;
    }

    if (!contains_value(stAudioCapabilitySet.aOutputTypes, strOutputType))
    {
        dlog_error("音频输出类型[%s]不受支持", strOutputType.c_str());
        return false;
    }

    if (!contains_value(stAudioCapabilitySet.aFormats, strFormat))
    {
        dlog_error("音频编码格式[%s]不受支持", strFormat.c_str());
        return false;
    }

    /* 当格式是 AAC 或 MP2L2 时才继续做采样率/码率校验 */
    if (data.enFormat != Audio_NS::AudioFormat_E::AAC &&
        data.enFormat != Audio_NS::AudioFormat_E::MP2L2)
    {
        return true;
    }

    auto it = std::find_if(stAudioCapabilitySet.aFormatDetail.begin(),
                           stAudioCapabilitySet.aFormatDetail.end(),
                           [&strFormat](const Audio_NS::AudioFormatCapability_S &item)
                           { return item.strFormat == strFormat; });
    if (it == stAudioCapabilitySet.aFormatDetail.end())
    {
        return true;
    }

    if (!it->aSampleRates.empty() && !contains_value(it->aSampleRates, nSampleRate))
    {
        dlog_error("音频编码格式[%s]不支持采样率[%d]", strFormat.c_str(), nSampleRate);
        return false;
    }
    if (!match_range_value(nSampleRate, it->stSampleRateRange))
    {
        dlog_error("音频编码格式[%s]采样率[%d]超出范围", strFormat.c_str(), nSampleRate);
        return false;
    }

    if (!it->aBitRates.empty() && !contains_value(it->aBitRates, nBitRate))
    {
        dlog_error("音频编码格式[%s]不支持码率[%d]", strFormat.c_str(), nBitRate);
        return false;
    }
    if (!match_range_value(nBitRate, it->stBitRateRange))
    {
        dlog_error("音频编码格式[%s]码率[%d]超出范围", strFormat.c_str(), nBitRate);
        return false;
    }

    return true;
}
