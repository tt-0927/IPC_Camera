/**
 * @FilePath     : stream_audio.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-31 15:31:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-02 15:24:16
 * @Description  : 流媒体音频模块
 */

#include "stream_audio.h"
#include "convert_interface.h"
#include "share_data.h"
#include "audio_wav.h"
#include "RtpServer.h"
#include "stream_server.h"
#include "system_utils.h"
#include "record_ctrl.h"

/*用来控制1:1输出的比例*/
#define VOLUME_RATION (1.12)

namespace
{
    /**
     * @brief   : 校验当前设备支持的音频配置
     * @param    {Audio_NS::AudioConfig_S} &stAudioConfig：音频配置
     * @return   {int} 0：成功，非0：失败
     */
    int check_audio_config_for_device(const Audio_NS::AudioConfig_S& stAudioConfig)
    {
#if !CAP_AUDIO_INPUT_LINEIN
        if (stAudioConfig.enInputType == Audio_NS::AudioInputType_E::LINEIN)
        {
            dlog_error("当前设备不支持LineIn输入");
            return ERR_PARAM;
        }
#endif

#if !CAP_AUDIO_OUTPUT_LINEOUT
        if (stAudioConfig.enOutputType == Audio_NS::AudioOutputType_E::LINEOUT)
        {
            dlog_error("当前设备不支持LineOut输出");
            return ERR_PARAM;
        }
#endif

        return OK;
    }
} // namespace

CStreamAudio* CStreamAudio::m_self = NULL;
std::mutex CStreamAudio::m_mutex;

CStreamAudio::CStreamAudio() : m_strConfigPath(AUDIO_CONFIG_FILE)
{
    /*读取音频配置文件*/
    if (Convert::read_file(m_strConfigPath, m_stAudioConfig))
    {
        using namespace Audio_NS;
        m_stAudioConfig.bAudioSwitch = true;
        m_stAudioConfig.enInputType = AudioInputType_E::MICIN;
        m_stAudioConfig.enFormat = AudioFormat_E::AAC;
        m_stAudioConfig.enSampRate = AudioSamprate_E::AUDIO_SAMPRATE_16000;
        m_stAudioConfig.enBitRate = AudioBitrate_E::AUDIO_BITRATE_48K;
        m_stAudioConfig.u32InputVolume = 50;
        m_stAudioConfig.bDenoise = true;
        m_stAudioConfig.enOutputType = AudioOutputType_E::SPEAKER;
        m_stAudioConfig.u32OutputVolume = 50;
        Convert::write_file(m_strConfigPath, m_stAudioConfig);
    }

    if (m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::AAC)
    {
        m_bIsAac = true;
    }
}

CStreamAudio::~CStreamAudio()
{
    m_bIsAac = false;
}

IpcRet_E CStreamAudio::init()
{
    if(m_stAudioConfig.bAudioSwitch == false) // 音频未开启
    {
        return OK;
    }

    m_atVolumL.store(m_stAudioConfig.u32InputVolume);
    m_atVolumR.store(m_stAudioConfig.u32InputVolume);

    /* 初始化与各模块的回调绑定 */
    initCallbackBinding();

    /* 初始化Audio模块 */
    stream_audio_sys_init();

    /* 音频输出初始化 */
    m_pAoHandle[AO_SPEAKER_CHN] = streamAo_init(AO_SPEAKER_CHN, m_stAudioConfig);
    if (m_pAoHandle[AO_SPEAKER_CHN] == nullptr)
    {
        dlog_error("功放喇叭音频输出初始化失败");
        goto err;
    }
    /* AO 输出设备初始化 */
    if(m_streamAO.init(m_stAudioConfig))
    {
        dlog_error("AO输出设备初始化失败");
        goto err;
    }
    /* 音频采集初始化 */
    m_pAiHandle[AI_MIC_CHN] = streamAi_init(AI_MIC_CHN, m_stAudioConfig);
    if (m_pAiHandle[AI_MIC_CHN] == nullptr)
    {
        dlog_error("采集音频输入初始化失败");
        goto err;
    }
    /* 音频编码初始化 */
    for (int nAencChn = AENC_AAC_CHN; nAencChn < AENC_MAX_CHN; nAencChn++)
    {
        /* 编码通道不为 AAC 编码通道时，如果音频配置为AAC，则跳过 */
        if (nAencChn > AENC_AAC_CHN /* && m_bIsAac */) // note 暂不需要开启额外的编码通道
        {
            continue;
        }
        m_pAencHandle[nAencChn] = streamAenc_init(AENC_AAC_CHN, m_stAudioConfig);
        if (m_pAencHandle[nAencChn] == nullptr)
        {
            dlog_error("音频编码初始化失败");
            goto err;
        }
    }

    /* 音频解码初始化 */
    m_pAdecHandle[SPEAK_CHN] = streamAdec_init(SPEAK_CHN, m_stAudioConfig);
    if (m_pAdecHandle[SPEAK_CHN] == nullptr)
    {
        dlog_error("音频编码初始化失败");
        goto err;
    }
    /*处理音频数据解码线程*/
    // adecThread[SPEAK_CHN] = std::thread(&CStreamAudio::deal_adecFrame_thr, this, SPEAK_CHN);
    // m_bGetAdecFlag[SPEAK_CHN].store(true, std::memory_order_release);

    /* 音频重采样初始化 */
    m_pResampleHandle = streamResample_init(m_stAudioConfig);

    /*绑定模块*/
    bindModule();

    /*获取mic、linein音频数据，进行处理*/
    aiThread[AI_MIC_CHN] = std::thread(&CStreamAudio::deal_aiFrame_thr, this, AI_MIC_CHN);
    // if (aiThread[AI_MIC_CHN].joinable())
    // {
    //     aiThread[AI_MIC_CHN].detach();
    // }
    /* 设置线程优先级 */
    // setThreadPriority(aiThread[AI_MIC_CHN], SCHED_RR, AUDIO_DATA_THREAD_PRIORITY);
    // m_aiRawThread = std::thread(&CStreamAudio::deal_aiRawFrame_thr, this, AI_MIC_CHN);
    m_bGetAiFlag[AI_MIC_CHN].store(true, std::memory_order_release);

    // note 只使用AENC模块编码AAC
    /*处理音频编码数据线程*/
    aencThread[AENC_AAC_CHN] = std::thread(&CStreamAudio::deal_aencFrame_thr, this, AENC_AAC_CHN);
    // if (aencThread[AENC_AAC_CHN].joinable())
    // {
    //     aencThread[AENC_AAC_CHN].detach();
    // }
    /* 设置线程优先级 */
    // setThreadPriority(aencThread[AENC_AAC_CHN], SCHED_RR, AUDIO_DATA_THREAD_PRIORITY);
    m_bGetAencFlag[AENC_AAC_CHN].store(true, std::memory_order_release);

    m_bInitFlag = true;

    dlog_info("流媒体音频初始化成功");
    return OK;

err:
    /* 音频重采样去初始化 */
    if (m_pResampleHandle != nullptr)
    {
        streamResample_uninit(m_pResampleHandle);
    }
    /*音频解码 去初始化*/
    if (m_pAdecHandle[SPEAK_CHN] != nullptr)
    {
        streamAdec_uninit(m_pAdecHandle[SPEAK_CHN]);
    }
    /*音频编码 去初始化*/
    if (m_pAencHandle[AENC_AAC_CHN] != nullptr)
    {
        streamAenc_uninit(m_pAencHandle[AENC_AAC_CHN]);
    }
    /*音频采集 去初始化*/
    if (m_pAiHandle[AI_MIC_CHN] != nullptr)
    {
        streamAi_uninit(m_pAiHandle[AI_MIC_CHN]);
    }
    /* AO输出设备 去初始化 */
    m_streamAO.uninit();
    /*音频输出 去初始化*/
    if (m_pAoHandle[AO_SPEAKER_CHN] != nullptr)
    {
        streamAo_uninit(m_pAoHandle[AO_SPEAKER_CHN]);
    }
    dlog_info("流媒体音频初始化失败");
    return ERR;
}

IpcRet_E CStreamAudio::deinit()
{
    if(m_stAudioConfig.bAudioSwitch == false) // 音频未开启
    {
        return OK;
    }

    m_bInitFlag = false;

    /*关闭音频数据解码线程*/
    // m_bGetAdecFlag[SPEAK_CHN].store(false, std::memory_order_release);

    //! 需先关闭音频采集线程再关闭编码
    /*关闭音频数据采集线程*/
    m_bGetAiFlag[AI_MIC_CHN].store(false, std::memory_order_release);
    if(aiThread[AI_MIC_CHN].joinable())
    {
        aiThread[AI_MIC_CHN].join();
    }
    // if(m_aiRawThread.joinable())
    // {
    //     m_aiRawThread.join();
    // }

    /*音频编码 去初始化*/
    /*关闭音频数据编码线程*/
    m_bGetAencFlag[AENC_AAC_CHN].store(false, std::memory_order_release);
    if(aencThread[AENC_AAC_CHN].joinable())
    {
        aencThread[AENC_AAC_CHN].join();
    }

    /*解绑模块*/
    unbindModule();

    /* 音频重采样去初始化 */
    if (m_pResampleHandle != nullptr)
    {
        streamResample_uninit(m_pResampleHandle);
    }
    /*音频解码 去初始化*/
    if (m_pAdecHandle[SPEAK_CHN] != nullptr)
    {
        streamAdec_uninit(m_pAdecHandle[SPEAK_CHN]);
    }
    /*音频编码 去初始化*/
    for (int nAencChn = AENC_AAC_CHN; nAencChn < AENC_MAX_CHN; nAencChn++)
    {
        /* 编码通道不为 AAC 编码通道时，如果音频配置为AAC，则跳过 */
        if (nAencChn > AENC_AAC_CHN /* && m_bIsAac */) // note 暂不需要开启额外的编码通道
        {
            continue;
        }

        if (m_pAencHandle[nAencChn] != nullptr)
        {
            streamAenc_uninit(m_pAencHandle[nAencChn]);
        }
    }
    /*音频采集 去初始化*/
    if (m_pAiHandle[AI_MIC_CHN] != nullptr)
    {
        streamAi_uninit(m_pAiHandle[AI_MIC_CHN]);
    }
    /* AO输出设备 去初始化 */
    m_streamAO.uninit();
    /*音频输出 去初始化*/
    if (m_pAoHandle[AO_SPEAKER_CHN] != nullptr)
    {
        streamAo_uninit(m_pAoHandle[AO_SPEAKER_CHN]);
    }

    /* 去初始化Audio模块 */
    stream_audio_sys_deinit();

    dlog_info("流媒体音频去初始化成功");
    return OK;
}

IpcRet_E CStreamAudio::reboot()
{
    int nRet = OK;
    std::lock_guard<std::mutex> lock(m_mutexCtrl);

    if (m_bInitFlag)
    {
        nRet = deinit();
        if (nRet < 0)
        {
            dlog_error("反初始化音频流模块失败");
            return ERR;
        }
    }
    nRet = init();
    if (nRet < 0)
    {
        dlog_error("初始化音频流模块失败");
        return ERR;
    }

    return OK;
}

int CStreamAudio::sendAudio_to_Adec(const Audio_NS::AoInfo_S &stAoInfo)
{
    if(stAoInfo.pData == NULL)
    {
        dlog_error("指针为空");
        return ERR;
    }
    /*检查通道号是否有效*/
    if (stAoInfo.nChannel < SPEAK_CHN || stAoInfo.nChannel >= ADEC_MAX_CHN)
    {
        dlog_error("无效的通道号: %d", stAoInfo.nChannel);
        return ERR;
    }

    uint32_t unTalkbackDataSize;
    uint8_t* pTalkbackData = NULL;

    if(stAoInfo.enAudioFormat == Audio_NS::AudioFormat_E::G711A)
    {
        unTalkbackDataSize = stAoInfo.nLen * 2;
        m_bytesTalkbackData.clear();
        m_bytesTalkbackData.resize(unTalkbackDataSize);
        pTalkbackData = m_bytesTalkbackData.data();

        /*解码g711a格式*/
        codec_g711a_decode(stAoInfo.pData, pTalkbackData, stAoInfo.nLen);
    }
    else if(stAoInfo.enAudioFormat == Audio_NS::AudioFormat_E::AAC)
    {
        ot_audio_stream stStream;
        stStream.len = stAoInfo.nLen;
        stStream.stream = stAoInfo.pData;
        m_pAdecHandle[SPEAK_CHN]->mppAdec_sendStream(m_pAdecHandle[SPEAK_CHN],&stStream,TD_FALSE);
        return OK;
    }
    else if(stAoInfo.enAudioFormat == Audio_NS::AudioFormat_E::PCM)
    {
        unTalkbackDataSize = stAoInfo.nLen;
        m_bytesTalkbackData.clear();
        m_bytesTalkbackData.resize(unTalkbackDataSize);
        pTalkbackData = m_bytesTalkbackData.data();

        memcpy(pTalkbackData, stAoInfo.pData, stAoInfo.nLen);
    }
    else 
    {
        dlog_error("不支持此格式");
        return ERR;
    }

    /*组音频帧，送数据至ao功放*/
    ot_audio_frame stFrame;
    stFrame.bit_width = OT_AUDIO_BIT_WIDTH_16;
    stFrame.snd_mode = OT_AUDIO_SOUND_MODE_MONO;
    stFrame.virt_addr[0] = pTalkbackData;
    stFrame.len = unTalkbackDataSize;
    m_pAoHandle[stAoInfo.nChannel]->mppAo_sendFrame(m_pAoHandle[stAoInfo.nChannel], stAoInfo.nChannel, &stFrame, 0);

    return OK;
}

int CStreamAudio::getAudioConfig(Audio_NS::AudioConfig_S &stAudioConfig)
{
    stAudioConfig = m_stAudioConfig;
    return OK;
}

int CStreamAudio::setAudioConfig(const Audio_NS::AudioConfig_S &stAudioConfig)
{
    int nRet = OK;
    nRet = check_audio_config_for_device(stAudioConfig);
    if (nRet != OK)
    {
        return ERR_PARAM;
    }

    bool bIsReset = false;

    /* 降噪开关改变 */
    if(m_stAudioConfig.bDenoise != stAudioConfig.bDenoise)
    {
        streamAi_set_vqe_rnr(m_pAiHandle[AI_MIC_CHN], AI_MIC_CHN, stAudioConfig.bDenoise);
    }

    /* 输入类型改变 */
    if(m_stAudioConfig.enInputType != stAudioConfig.enInputType)
    {
        ot_audio_track_mode enTrackMode = OT_AUDIO_TRACK_NORMAL;
        if (stAudioConfig.enInputType == Audio_NS::AudioInputType_E::MICIN)
        {
            enTrackMode = OT_AUDIO_TRACK_RIGHT_MUTE;
        }
        else if (stAudioConfig.enInputType == Audio_NS::AudioInputType_E::LINEIN)
        {
            enTrackMode = OT_AUDIO_TRACK_LEFT_MUTE;
        }
        streamAi_set_track_mode(m_pAiHandle[AI_MIC_CHN], enTrackMode);
    }

    /* 输出类型改变 */
    if (m_stAudioConfig.enOutputType != stAudioConfig.enOutputType)
    {
        m_streamAO.update_audioConfig(stAudioConfig);
    }

    /* 输出音量改变 */
    if (m_stAudioConfig.u32OutputVolume != stAudioConfig.u32OutputVolume)
    {
        if (TD_SUCCESS != m_pAoHandle[AO_SPEAKER_CHN]->mppAo_setVolume(m_pAoHandle[AO_SPEAKER_CHN], stAudioConfig.u32OutputVolume))
        {
            dlog_error("设置音频输出音量失败");
        }
    }

    /* 是否为AAC编码格式 */
    m_bIsAac = stAudioConfig.enFormat == Audio_NS::AudioFormat_E::AAC ? true : false;

    /* 音频格式改变 */
    if(m_stAudioConfig.enFormat != stAudioConfig.enFormat)
    {
        bIsReset = true;
    }

    /*更新音频配置*/
    m_stAudioConfig = stAudioConfig;

    if(bIsReset)
    {
        /* 同步RTSP */
        CRtspServer::instance()->setAudioConfig(m_stAudioConfig);
        /*重新启动RTSP服务器*/
        CRtspServer::instance()->reboot();
        /*重新启动音频流模块*/
        reboot();
    }
    return OK;
}

int CStreamAudio::setAoSampleRate(const Audio_NS::AudioSamprate_E enSampRate)
{
    if(enSampRate != Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_16000 && enSampRate != Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_8000)
    {
        dlog_error("设置音频输出采样率参数错误");
        return ERR_PARAM;
    }

    int nAoDevice = AO_SPEAKER_CHN;
    if (m_pAoHandle[nAoDevice]->stNeedParam.enSampleRate != (ot_audio_sample_rate) enSampRate)
    {
        Audio_NS::AudioConfig_S stAudioConfig;
        stAudioConfig.enSampRate = enSampRate;
        if (streamAo_reboot(m_pAoHandle[nAoDevice], nAoDevice, stAudioConfig))
        {
            dlog_error("音频输出重启失败");
            return ERR;
        }
    }
    return OK;
}

HiAi_S *CStreamAudio::get_aiHandle(int nChn)
{
    return m_pAiHandle[nChn];
}

int CStreamAudio::bindModule()
{
    int nRet = OK;
    // note 需进行声音音量调节，使用 ai 获取数据
    // nRet |= mppAi_bind_aenc(m_pAiHandle[AI_MIC_CHN]->stExParam.nDevId, m_pAiHandle[AI_MIC_CHN]->stNeedParam.nChn, m_pAencHandle[AENC_AAC_CHN]->stNeedParam.nChn);
    nRet |= mppAdec_bind_ao(m_pAdecHandle[SPEAK_CHN]->stNeedParam.nChn, m_pAoHandle[AO_SPEAKER_CHN]->stNeedParam.nDevId, m_pAoHandle[AO_SPEAKER_CHN]->stNeedParam.nChn);

    return nRet;
}

int CStreamAudio::unbindModule()
{
    int nRet = OK;
    // nRet |= mppAi_unbind_aenc(m_pAiHandle[AI_MIC_CHN]->stExParam.nDevId, m_pAiHandle[AI_MIC_CHN]->stNeedParam.nChn, m_pAencHandle[AENC_AAC_CHN]->stNeedParam.nChn);
    nRet |= mppAdec_unbind_ao(m_pAdecHandle[SPEAK_CHN]->stNeedParam.nChn, m_pAoHandle[AO_SPEAKER_CHN]->stNeedParam.nDevId, m_pAoHandle[AO_SPEAKER_CHN]->stNeedParam.nChn);

    return nRet;
}

int CStreamAudio::audioMange_set_volume(int nVolumeL,  int nVolumeR)
{
    if((nVolumeL < 0 || nVolumeL > 100) || (nVolumeR < 0 || nVolumeR > 100))
    {
        return ERR;
    }
    m_atVolumL.store(nVolumeL);
    m_atVolumR.store(nVolumeR);

    return OK;
}

void CStreamAudio::initCallbackBinding()
{
    /*绑定音频配置更新回调*/
    CAVConfigure::instance()->setAudioConfigCallback(
        [this](const Audio_NS::AudioConfig_S &stAudioConfig) -> int
        {
            return this->setAudioConfig(stAudioConfig);
        });

    /*绑定音频对讲更新回调*/
    CAVConfigure::instance()->setAoSpeakCallback(
        [this](const Audio_NS::AoInfo_S &stAoInfo) -> int
        {
            return this->sendAudio_to_Adec(stAoInfo);
        });

    /* 绑定绑定音频对讲更新回调 */
    CAVConfigure::instance()->setAudioAoSampleRateCallback(
        [this](const Audio_NS::AudioSamprate &enSamprate) -> int
        {
            return this->setAoSampleRate(enSamprate);
        });
}

Audio_NS::AudioFrame_S *CStreamAudio::createFrame(uint8_t *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        dlog_error("传入参数不正确");
        return nullptr;
    }

    /*分配连续内存：结构体 + 数据*/
    Audio_NS::AudioFrame_S *pAudioFrame = nullptr;
    pAudioFrame = (Audio_NS::AudioFrame_S *) malloc(sizeof(Audio_NS::AudioFrame_S) + nDataLen);
    if (!pAudioFrame)
    {
        dlog_error("内存分配失败");
        return nullptr;
    }

    memcpy(pAudioFrame->pData, pData, nDataLen);
    pAudioFrame->nLen = nDataLen;
    pAudioFrame->enFormat = m_stAudioConfig.enFormat;

    return pAudioFrame;
}

void CStreamAudio::freeFrame(Audio_NS::AudioFrame_S *pAudioFrame)
{
    if (pAudioFrame)
    {
        delete pAudioFrame;
        pAudioFrame = nullptr;
    }
}

int CStreamAudio::volume_adjust(int8_t *pData, int nSize)
{
    if (pData == NULL)
    {
        return ERR;
    }
    int nVolume = m_stAudioConfig.u32InputVolume;

    int16_t *pcm = (int16_t *)pData;
    int pcmvall = 0;
    // int pcmvalR = 0;
    int i = 0;
    int nCount = nSize / 2;
    float nVol = 1.0 * nVolume / 100 * VOLUME_RATION;

    for (i = 0; i < nCount; i++)
    {
        pcmvall = (1 * pcm[i] * nVol);

        if (pcmvall > 32767)
        {
            pcmvall = 32767;
        }
        else if (pcmvall < -32768)
        {
            pcmvall = -32768;
        }

        pcm[i] = pcmvall;
    }

    return OK;
}

Audio_NS::AudioFrame_S *CStreamAudio::encode_g711(uint8_t *pData, int nDataLen)
{
    if(pData == NULL)
    {
        dlog_error("指针为空");
        return nullptr;
    }
    if(nDataLen < 0)
    {
        return nullptr;
    }

    /* 编码后数据长度减半 */
    uint32_t unDataSize = nDataLen / 2;
    /* 存储编码后的数据 */
    std::vector<uint8_t> vecOutData;
    vecOutData.resize(unDataSize);
    uint8_t *pOutData = vecOutData.data();

    if (m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A)
    {
        /*编码g711a格式*/
        codec_g711a_encode(pData, pOutData, nDataLen);
    }
    else if (m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U)
    {
        /*编码g711u格式*/
        codec_g711u_encode(pData, pOutData, nDataLen);
    }

    return createFrame(pOutData, unDataSize);
}

//info /*----------------------- 私有线程函数 -----------------------*/

void CStreamAudio::deal_aiFrame_thr(int param)
{
    pthread_setname_np(pthread_self(), "deal_aiFrame_thr");
    try
    {
        int nRet = 0;
        ot_audio_frame stFrame;
        ot_aec_frame stAecFrame;
        int nChannel = param;
        HiAi_S *pHandle = m_pAiHandle[nChannel];
        
        // 初始化音频WAV文件
        // CAudioWav audio_wav,audio_wav2;
        // audio_wav.init(16000, 16, 1);
        // audio_wav.open("/opt/cam/bin/ai-mic.wav");
        // audio_wav2.init(16000, 16, 1);
        // audio_wav2.open("/opt/cam/bin/ai-mic2.wav");

        /* 重采样输出缓冲区 */
        std::vector<td_s16> vecResampleBuffer;
        /* 预分配重采样输出缓冲区 */
        if (m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A
            || m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U)
        {
            if (m_pResampleHandle)
            {
                /* 根据配置计算最大可能需要的缓冲区大小 */
                int nInitialCapacity = m_pResampleHandle->mppResample_get_max_output_num(m_pResampleHandle, pHandle->stNeedParam.u32PointNumPerFrame);
                vecResampleBuffer.reserve(nInitialCapacity);
            }
        }

        while (true == m_bGetAiFlag[nChannel].load(std::memory_order_acquire))
        {
            memset(&stFrame, 0, sizeof(ot_audio_frame));
            memset(&stAecFrame, 0, sizeof(ot_aec_frame));
            /*获取ai采集数据*/
            nRet = pHandle->mppAi_getFrame(pHandle, nChannel, &stFrame, &stAecFrame, -1);
            if (nRet != OK)
            {
                dlog_info("aenc %d error %x", nChannel, nRet);
                continue;
            }

            {
                /* mic 音量调整 */
                // audio_processing_volChange(reinterpret_cast<int8_t *>(stFrame.virt_addr[0]), stFrame.len, 2.0f);

                if (AI_MIC_CHN == nChannel) /* 第一码流 */
                {
                    // audio_wav.write(stFrame.virt_addr[0], stFrame.len);
                    // audio_wav2.write(stAecFrame.ref_frame.virt_addr[0], stAecFrame.ref_frame.len);
                    // note 同时采集mic与linein
                    stFrame.snd_mode = OT_AUDIO_SOUND_MODE_MONO;
                    if (m_stAudioConfig.enInputType == Audio_NS::AudioInputType_E::LINEIN)
                    {
                        memcpy_s(stFrame.virt_addr[0], stFrame.len, stFrame.virt_addr[1], stFrame.len);
                    }
                    memset_s(stFrame.virt_addr[1], stFrame.len, 0, stFrame.len);

                    /* 音量调整 */
                    volume_adjust(reinterpret_cast<int8_t *>(stFrame.virt_addr[0]), stFrame.len);

                    /* 送 AENC 模块 */
                    m_pAencHandle[nChannel]->mppAenc_sendFrame(m_pAencHandle[nChannel], &stFrame);

                    /* 发送数据到 AI_APP */
                    algo_send_audioStreamData(stFrame.virt_addr[0], stFrame.len);

                    if (m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A
                        || m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U)
                    { /* 重采样 16KHz -> 8KHz */
                        if(m_pResampleHandle)
                        {
                            /* 检查输出缓冲区是否需要扩容 */
                            unsigned int nRequiredCapacity = m_pResampleHandle->mppResample_get_max_output_num(
                                m_pResampleHandle,
                                pHandle->stNeedParam.u32PointNumPerFrame);

                            if (nRequiredCapacity > vecResampleBuffer.capacity())
                            {
                                /* 需要扩容 */
                                vecResampleBuffer.reserve(nRequiredCapacity);
                            }

                            /* 重采样处理 */
                            int nActualOutputPoints = nRequiredCapacity;
                            m_pResampleHandle->mppResample_process(m_pResampleHandle,
                                                                    reinterpret_cast<td_s16 *>(stFrame.virt_addr[0]),
                                                                    pHandle->stNeedParam.u32PointNumPerFrame,
                                                                    vecResampleBuffer.data(),
                                                                    &nActualOutputPoints);

                            /* 编码g711格式 */
                            int nOutputBytes = nActualOutputPoints * sizeof(td_s16);
                            Audio_NS::AudioFrame_S *pAudioFrame = encode_g711(reinterpret_cast<uint8_t *>(vecResampleBuffer.data()), nOutputBytes);
    
                            /* 送推流模块 RTSP */
                            CPushStream::instance()->sendAudioData(pAudioFrame);
    
                            /* 释放帧数据 */
                            freeFrame(pAudioFrame);
                        }
                    }
                    else if (m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::PCM)
                    {
                        // todo 未完成 PCM
                    }

                    stFrame.snd_mode = OT_AUDIO_SOUND_MODE_STEREO;
                }
            }

            /*释放ai获取的音频数据缓存*/
            pHandle->mppAi_releaseFrame(pHandle, nChannel, &stFrame, &stAecFrame);

            /* 主动放弃CPU */
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // audio_wav.uninit();
        // audio_wav2.uninit();
    }
    catch(const std::exception& e)
    {
        dlog_error("%s",e.what());
    }
}

#if 0
void CStreamAudio::deal_aiRawFrame_thr(int param)
{
    pthread_setname_np(pthread_self(), "dealAiRawFrame");
    try
    {
        int nRet = 0;
        ot_audio_frame stFrame;
        int nChannel = param;
        HiAi_S *pHandle = m_pAiHandle[nChannel];
        CAudioWav audio_wav;
        audio_wav.init(16000, 16, 1);
        audio_wav.open("/opt/cam/bin/ai-micRaw.wav");
        while (true == m_bGetAiFlag[nChannel].load(std::memory_order_acquire))
        {
            memset(&stFrame, 0, sizeof(ot_audio_frame));
            /*获取ai采集数据*/
            nRet = pHandle->mppAi_getRawFrame(pHandle, nChannel, &stFrame, TIMEOUT_100_MS);
            if (nRet != OK)
            {
                dlog_info("mppAi_getRawFrame %d error %x", nChannel, nRet);
                usleep(10 * 1000);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(m_mutexSendData);
                if (AI_MIC_CHN == nChannel) /* 第一码流 */
                {
                    // note 同时采集mic与linein
                    if (m_stAudioConfig.enInputType == Audio_NS::AudioInputType_E::LINEIN)
                    {
                        memcpy_s(stFrame.virt_addr[0], stFrame.len, stFrame.virt_addr[1], stFrame.len);
                    }
                    audio_wav.write(stFrame.virt_addr[0], stFrame.len);
                    /* 发送数据到 AI_APP */
                    algo_send_audioStreamData(&stFrame);
                }
            }
            usleep(400 * 1000);
        }
        audio_wav.uninit();
    }
    catch(const std::exception& e)
    {
        dlog_error("%s",e.what());
    }
}
#endif

void CStreamAudio::deal_aencFrame_thr(int param)
{
    pthread_setname_np(pthread_self(), "deal_aencFrame_thr");
    try
    {
        int nRet = 0;
        ot_audio_stream stFrame;
        int nChannel = param;
        HiAenc_S *pHandle = m_pAencHandle[nChannel];

        // FILE *fd = fopen("/opt/cam/bin/aenc.aac","w+");
        // FILE *fd_g711 = fopen("/opt/cam/bin/aenc.g711","w+");
        while (true == m_bGetAencFlag[nChannel].load(std::memory_order_acquire))
        {
            memset(&stFrame, 0, sizeof(ot_audio_stream));
            /*获取编码码流*/
            nRet = pHandle->mppAenc_getFrame(pHandle, &stFrame, TIMEOUT_500_MS);
            if (nRet != OK)
            {
                dlog_info("获取aenc数据帧失败:%x", nRet);
                continue;
            }

            if (stFrame.stream == nullptr)
            {
                /*释放码流缓存*/
                pHandle->mppAenc_releaseFrame(pHandle, &stFrame);
                continue;
            }

            /*送编码视频数据*/
            if (AENC_AAC_CHN == nChannel) /* 第一码流 */
            {
                // fwrite(stFrame.stream, stFrame.len, 1, fd);
                // fwrite(stFrame.stream, stFrame.len, 1, fd_g711);
                // dlog_debug("stFrame:%d",stFrame.len);

                //note /*移除ADTS头，RTSP流不需要ADTS头，只需要AAC裸数据*/
                int nADTSLen = 0;
                /*检查同步字（0xFFF）*/
                if ((stFrame.stream[0] != 0xFF) || ((stFrame.stream[1] & 0xF0) != 0xF0))
                {
                    dlog_warn("当前AAC音频无ADTS头 Len:[%d]", stFrame.len);
                    /*释放码流缓存*/
                    pHandle->mppAenc_releaseFrame(pHandle, &stFrame);
                    continue;
                }
                /*提取保护位（第2字节的最低位）判断是否有CRC校验*/
                if( !(stFrame.stream[1] & 0x01))
                {
                    nADTSLen = 9; //有CRC校验
                }else{
                    nADTSLen = 7; //无CRC校验
                }
                // dlog_debug("stFrame.len: %d nADTSLen: %d", stFrame.len, nADTSLen);
                Audio_NS::AudioFrame_S *pAudioFrame = createFrame(stFrame.stream + nADTSLen, stFrame.len - nADTSLen);

                {
                    std::lock_guard<std::mutex> lock(m_mutexSendData);
                    if (m_bIsAac)
                    {
                        /* 送推流模块 RTSP */
                        CPushStream::instance()->sendAudioData(pAudioFrame);
                        /* 推给gb28181 */
                        // SIP::CRtpServer::instance()->sendAudioData(pAudioFrame);
                    }
                    if (CRecordCtrl::instance()->get_record_status() == Record_NS::Status_E::RECORD_OPERATION)
                    {
                        /* 录制模块 */
                        CStreamServer::instance()->sendAudioData(pAudioFrame);
                    }
                }

                /* 释放帧数据 */
                freeFrame(pAudioFrame);
            }
            else
            {
                // note 暂无新的编码通道
            }
            /*释放码流缓存*/
            pHandle->mppAenc_releaseFrame(pHandle, &stFrame);
            /* 主动放弃CPU */
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // fclose(fd);
        // fclose(fd_g711);
    }
    catch(const std::exception& e)
    {
        dlog_error("%s",e.what());
    }
}

#if 0
void CStreamAudio::deal_adecFrame_thr(int param)
{
    pthread_setname_np(pthread_self(), "deal_adecFrame_thr");
    try
    {
        int nRet = 0;
        ot_audio_frame_info stFrameInfo;
        unsigned char *pData = NULL;
        int nDataLen = 0;
        int nChannel = param;
        HiAdec_S *pHandle = m_pAdecHandle[nChannel];

        // FILE *fd = fopen("/opt/cam/bin/aenc.aac","w+");
        // FILE *fd_g711 = fopen("/opt/cam/bin/adec.g711","w+");
        while (true == m_bGetAdecFlag[nChannel].load(std::memory_order_acquire))
        {
            memset(&stFrameInfo, 0, sizeof(ot_audio_frame_info));
            /*获取解码码流*/
            nRet = pHandle->mppAdec_getFrame(pHandle, &stFrameInfo, TD_TRUE);
            if (nRet != OK)
            {
                dlog_info("adec %d error %x", nChannel, nRet);
                continue;
            }
            std::lock_guard<std::mutex> lock(m_mutexSendData);

            /*送编码视频数据*/
            if (SPEAK_CHN == nChannel) /* 第一码流 */
            {
                // fwrite(stFrame.stream, stFrame.len, 1, fd);
                // fwrite(stFrameInfo.frame->virt_addr[0], stFrameInfo.frame->len, 1, fd_g711);
                // dlog_debug("stFrame:%d",stFrameInfo.frame->len);
            }
            /*释放码流缓存*/
            pHandle->mppAdec_releaseFrame(pHandle, &stFrameInfo);
        }
        // fclose(fd);
        // fclose(fd_g711);
    }
    catch(const std::exception& e)
    {
        dlog_error("%s",e.what());
    }
}
#endif
