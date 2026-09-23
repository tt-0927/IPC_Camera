/**
 * @FilePath     : stream_audio.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-02 09:50:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-27 16:31:35
 * @Description  : 流媒体音频模块
 */

#include "stream_audio.h"
#include "audio_processing.h"
#include "audio_wav.h"
#include "IpcRet.h"
#include "push_stream.h"
#include "RtpServer.h"
#include "stream_server.h"
#include "algo_detect.h"
#include "record_ctrl.h"
#include "voice_com_capture_source.h"

/*用来控制1:1输出的比例*/
#define VOLUME_RATION   (1.12)
/*音频延时总和*/
#define AUDIO_DEALY_SUM 16

uint64_t CStreamAudio::getSteadyTimeMS() 
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

// 定义一个结构体来保持“历史状态”，防止帧与帧之间出现爆音
typedef struct
{
    int16_t history[4]; // 只需要保存最后4个采样点
} ResampleContext;

ResampleContext ctx;

/**
 * 高质量 16k 转 8k 重采样 (5-Tap FIR 滤波器 + 2x 抽取)
 *
 * @param ctx      历史状态上下文指针 (必传，否则会有爆音)
 * @param in_buf   16k 输入数据 (int16_t)
 * @param out_buf  8k 输出数据 buffer
 * @param in_len   输入数据的字节长度 (注意是字节数，不是sample数)
 * @return         输出数据的字节长度
 */
static int resample_16k_to_8k_fir(ResampleContext *ctx, int16_t *in_buf, int16_t *out_buf, int in_len)
{
    int src_samples = in_len / 2;
    int dst_samples = src_samples / 2;
    int i;

    // 滤波器系数: [-1, 4, 10, 4, -1] / 16
    // 这是一组经典的整数近似系数，能在保留人声的同时滤除混叠杂音

    for (i = 0; i < dst_samples; i++)
    {
        int src_idx = i * 2; // 2倍抽取，对应输入的位置

        // 我们需要读取 src_idx 周围的5个点: p[-2], p[-1], p[0], p[1], p[2]
        // 使用 int32_t 防止计算溢出
        int32_t val = 0;
        int16_t p_2, p_1, p0, p1, p2;

        // --- 获取采样点 (处理边界问题) ---

        // p[-2] (前前一个点)
        if (src_idx - 2 < 0)
            p_2 = ctx->history[2 + (src_idx - 2)]; // 从历史取
        else
            p_2 = in_buf[src_idx - 2];

        // p[-1] (前一个点)
        if (src_idx - 1 < 0)
            p_1 = ctx->history[2 + (src_idx - 1)]; // 从历史取
        else
            p_1 = in_buf[src_idx - 1];

        // p[0] (当前中心点)
        p0 = in_buf[src_idx];

        // p[1] (后一个点)
        if (src_idx + 1 >= src_samples)
            p1 = in_buf[src_samples - 1]; // 简单边界保护(复制最后一个)
        else
            p1 = in_buf[src_idx + 1];

        // p[2] (后后一个点)
        if (src_idx + 2 >= src_samples)
            p2 = in_buf[src_samples - 1]; // 简单边界保护
        else
            p2 = in_buf[src_idx + 2];

        // --- FIR 滤波计算 ---
        // 公式: (-1*x[-2] + 4*x[-1] + 10*x[0] + 4*x[1] - 1*x[2]) / 16
        // 这里的 16 可以用 >> 4 实现，极其快速

        val = -p_2 + (p_1 << 2) + (p0 * 10) + (p1 << 2) - p2;

        // 舍入并右移4位 (除以16)
        // 加8是为了四舍五入 (16/2=8)
        val = (val + 8) >> 4;

        // 钳位处理 (防止溢出 int16 范围，虽然这组系数一般不会溢出，但为了安全)
        if (val > 32767)
            val = 32767;
        else if (val < -32768)
            val = -32768;

        out_buf[i] = (int16_t) val;
    }

    // --- 更新历史数据，供下一帧使用 ---
    // 保存本帧最后4个点
    if (src_samples >= 4)
    {
        ctx->history[0] = in_buf[src_samples - 4];
        ctx->history[1] = in_buf[src_samples - 3];
        ctx->history[2] = in_buf[src_samples - 2];
        ctx->history[3] = in_buf[src_samples - 1];
    }

    return dst_samples * 2; // 返回输出字节数
}

CStreamAudio::CStreamAudio() : m_strConfigPath(AUDIO_CONFIG_FILE)
{
    /*获取单例实例*/
    m_streamAO  = CStreamAo::instance();

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
}

int CStreamAudio::init()
{
    /*初始化与各模块的回调绑定*/
    initCallbackBinding();

    /* 初始化音频流采集处理模块 */
    if (OK != initStreamAudio(m_stAudioConfig))
    {
        dlog_error("音频模块初始化失败");
        return ERR;
    }

    m_bInitFlag = true;

    return OK;
}

int CStreamAudio::deinit()
{
    m_bInitFlag = false;

    /*去初始化音频流采集处理模块*/
    if (OK != deinitStreamAudio())
    {
        dlog_error("音频模块去初始化失败");
        return ERR;
    }
    return OK;
}

int CStreamAudio::reboot()
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

int CStreamAudio::initStreamAudio(Audio_NS::AudioConfig_S stAudioConfig)
{
    if (stAudioConfig.bAudioSwitch == false) // 音频未开启
    {
        return OK;
    }

    m_atVolumL.store(stAudioConfig.u32InputVolume);
    m_atVolumR.store(stAudioConfig.u32InputVolume);

    /*音频采集 初始化*/
    m_pAiHandle[AI_MIC_CHN] = streamAi_init(AI_MIC_CHN, stAudioConfig);
    if (m_pAiHandle[AI_MIC_CHN] == nullptr)
    {
        dlog(LOG_ERROR, "Mic Ai_init error");
        goto err;
    }

    /* AO 输出设备初始化 */
    if (m_streamAO->init(stAudioConfig))
    {
        dlog_error("AO输出设备初始化失败");
        goto err;
    }
    m_bStopThread = false;
    m_monitorThread = std::thread(&CStreamAudio::paMonitorTimer_thr, this);

    // note 初始使能ao静音，避免砰砰声
    m_streamAO->update_audioOutputType(Audio_NS::AudioOutputType_E::MUTE);

    /*音频输出 初始化*/
    m_pAoHandle[AO_SPEAKER_CHN] = streamAo_init(AO_SPEAKER_CHN, stAudioConfig);
    if (m_pAoHandle[AO_SPEAKER_CHN] == nullptr)
    {
        dlog(LOG_ERROR, "Ao_init error");
        goto err;
    }

    /* 更新音频输出类型，启用喇叭功放或者线路输出 */
    //m_streamAO->update_audioOutputType(stAudioConfig.enOutputType);
    dlog_debug("音频输出成功");

    /* 音频解码初始化 */
    // m_pAdecHandle[ADEC_SPEAK_CHN] = streamAdec_init(ADEC_SPEAK_CHN, m_stAudioConfig);
    // if (m_pAdecHandle[ADEC_SPEAK_CHN] == nullptr)
    // {
    //     dlog_error("音频编码初始化失败");
    //     goto err;
    // }

    /* 初始化ffmpeg音频编码 */
    if (init_ff_encode(stAudioConfig) != OK)
    {
        dlog(LOG_ERROR, "初始化ffmpeg音频编码失败");
        goto err;
    }

    /*绑定模块*/
    bindModule();

    /*获取mic、linein音频数据，进行处理*/
    m_bGetAiFlag[AI_MIC_CHN].store(true, std::memory_order_release);
    aiThread[AI_MIC_CHN] = std::thread(&CStreamAudio::deal_aiFrame_thr, this, AI_MIC_CHN);

    // for (int nAencChn = AENC_MIC_CHN; nAencChn < AENC_MAX_CHN; nAencChn++)
    // {
    //     /*处理音频编码数据线程*/
    //     aencThread[nAencChn] = std::thread(&CStreamAudio::deal_aencFrame_thr, this, nAencChn);
    //     if (aencThread[nAencChn].joinable())
    //     {
    //         aencThread[nAencChn].detach();
    //     }
    //     /* 设置线程优先级 */
    //     // setThreadPriority(aencThread[nAencChn], SCHED_RR, AUDIO_DATA_THREAD_PRIORITY);
    //     m_bGetAencFlag[nAencChn].store(true, std::memory_order_release);
    // }

    dlog_info("流媒体音频初始化成功");
    return OK;

err:
    /*音频解码 去初始化*/
    // if (m_pAdecHandle[ADEC_SPEAK_CHN] != nullptr)
    // {
    //     streamAdec_uninit(m_pAdecHandle[ADEC_SPEAK_CHN]);
    // }
    /* AO输出设备 去初始化 */
    m_streamAO->uninit();
    /*音频输出 去初始化*/
    if (m_pAoHandle[AO_SPEAKER_CHN] != nullptr)
    {
        streamAo_uninit(m_pAoHandle[AO_SPEAKER_CHN]);
    }
    /*音频采集 去初始化*/
    if (m_pAiHandle[AI_MIC_CHN] != nullptr)
    {
        streamAi_uninit(m_pAiHandle[AI_MIC_CHN]);
    }

    /* 去初始化ffmpeg音频编码 */
    deinit_ff_encode();

    /*关闭监测AO对应输出模块GPIO状态线程*/
    m_bStopThread = true;
    if (m_monitorThread.joinable()) 
    {
        m_monitorThread.join();
    }

    dlog_info("流媒体音频初始化失败");
    return ERR;
}

int CStreamAudio::deinitStreamAudio()
{
    /* 音频未开启 初始化未成功 */
    if (m_stAudioConfig.bAudioSwitch == 0)
    {
        return OK;
    }

    //! 需先关闭音频采集线程再关闭编码
    /*关闭音频数据采集线程*/
    m_bGetAiFlag[AI_MIC_CHN].store(false, std::memory_order_release);
    if (aiThread[AI_MIC_CHN].joinable())
    {
        aiThread[AI_MIC_CHN].join();
    }

    /*音频编码 去初始化*/
    // for (int nAencChn = AENC_MIC_CHN; nAencChn < AENC_MAX_CHN; nAencChn++)
    // {
    //     /*关闭音频数据编码线程*/
    //     m_bGetAencFlag[nAencChn].store(false, std::memory_order_release);
    //     if(aencThread[nAencChn].joinable())
    //     {
    //         aencThread[nAencChn].join();
    //     }
    // }

    /*解绑模块*/
    unbindModule();

    /* 去初始化ffmpeg音频编码 */
    if (deinit_ff_encode() != OK)
    {
        dlog_error("去初始化ffmpeg音频编码失败");
    }

    /*音频解码 去初始化*/
    // if (m_pAdecHandle[ADEC_SPEAK_CHN] != nullptr)
    // {
    //     streamAdec_uninit(m_pAdecHandle[ADEC_SPEAK_CHN]);
    // }
    /*音频编码 去初始化*/
    // for (int nAencChn = AENC_MIC_CHN; nAencChn < AENC_MAX_CHN; nAencChn++)
    // {
    //     if (m_pAencHandle[nAencChn] != nullptr)
    //     {
    //         streamAenc_uninit(m_pAencHandle[nAencChn]);
    //     }
    // }
    /*音频采集 去初始化*/
    if (m_pAiHandle[AI_MIC_CHN] != nullptr)
    {
        streamAi_uninit(m_pAiHandle[AI_MIC_CHN]);
    }
    /* AO输出设备 去初始化 */
    m_streamAO->uninit();
    /*音频输出 去初始化*/
    if (m_pAoHandle[AO_SPEAKER_CHN] != nullptr)
    {
        streamAo_uninit(m_pAoHandle[AO_SPEAKER_CHN]);
    }

    /*关闭监测AO对应输出模块GPIO状态线程*/
    m_bStopThread = true;
    if (m_monitorThread.joinable()) 
    {
        m_monitorThread.join();
    }

    dlog_info("流媒体音频去初始化成功");
    return OK;
}

int CStreamAudio::init_ff_encode(const Audio_NS::AudioConfig_S stAudioConfig)
{
    ff_AudioEncNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(ff_AudioEncNeedParam_S));
    stNeedParam.nPacketCnt = AUDIO_DEALY_SUM + 4;
    stNeedParam.enSampleFormat = AV_SAMPLE_FMT_S16;
    stNeedParam.nChannel = 1; // LINE_IN_CHN;
    stNeedParam.nSampleRate = (int) stAudioConfig.enSampRate;
    stNeedParam.enCodecId = AV_CODEC_ID_AAC;
    m_pFfAencHandle = ff_audioEnc_alloc(stNeedParam, "libfdk_aac");
    if (m_pFfAencHandle)
    {
        m_pFfAencHandle->stExParam.nBite = static_cast<unsigned int>(stAudioConfig.enBitRate) * 1024;
        dlog_debug("nBite:%d", static_cast<int>(stAudioConfig.enBitRate));
    }
    m_pFfAencHandle->stExParam.nDelaySum = AUDIO_DEALY_SUM;

    if (m_pFfAencHandle->ff_audioEnc_init(m_pFfAencHandle) != OK)
    {
        return ERR;
    }

    /*处理音频编码数据线程*/
    m_bGetFfAencFlag.store(true, std::memory_order_release);
    ffAencThread = std::thread(&CStreamAudio::deal_ffAencFrame_thr, this);

    return OK;
}

int CStreamAudio::deinit_ff_encode()
{
    if (m_pFfAencHandle == nullptr)
    {
        return OK;
    }

    /* 关闭获取ffmpeg音频编码数据线程 */
    m_bGetFfAencFlag.store(false, std::memory_order_release);
    if (ffAencThread.joinable())
    {
        ffAencThread.join();
    }

    if (m_pFfAencHandle->ff_audioEnc_uninit(m_pFfAencHandle) != OK)
    {
        return ERR;
    }

    return OK;
}

int CStreamAudio::sendAudio_to_Adec(Audio_NS::AoInfo_S stAoInfo)
{
    if (stAoInfo.pData == NULL)
    {
        dlog_error("指针为空");
        return ERR;
    }
    /*检查通道号是否有效*/
    if (stAoInfo.nChannel < ADEC_SPEAK_CHN || stAoInfo.nChannel >= ADEC_MAX_CHN)
    {
        dlog_error("无效的通道号: %d", stAoInfo.nChannel);
        return ERR;
    }

    uint32_t unTalkbackDataSize;
    uint8_t *pTalkbackData = NULL;

    /*
     * 功放延时开启策略（消除开头噗声）：
     *   音频到达时，先将数据送入 Codec（功放保持关闭），让 Codec 偏置电压稳定建立。
     *   延时 PA_STARTUP_DELAY_MS 毫秒后，再开启功放，此时 Codec 输出已稳定，无噗声。
     *   此逻辑对声音联动、对讲、广播三条音频路径均生效（共用 sendAudio_to_Adec）。
     */
    if (!m_paEnable.load())
    {
        if (!m_bPaDelayActive.load())
        {
            /* 首帧到达：根据音频来源选择延时时长，启动延时计时，功放暂不开启 */
            m_paDelayMs = (stAoInfo.enSource == Audio_NS::AoSource_E::AO_SOURCE_REALTIME)
                          ? PA_DELAY_REALTIME_MS : PA_DELAY_ALARM_MS;
            m_bPaDelayActive.store(true);
            m_paDelayStartTime = getSteadyTimeMS();
            dlog_info("功放启动延时开始 (%llums, 来源=%d)", m_paDelayMs, static_cast<int>(stAoInfo.enSource));
        }

        uint64_t elapsed = getSteadyTimeMS() - m_paDelayStartTime;
        if (elapsed >= m_paDelayMs)
        {
            /* 延时结束：开启功放 */
            m_streamAO->update_audioOutputType(m_stAudioConfig.enOutputType);
            m_paEnable.store(true);
            m_bPaDelayActive.store(false);
            dlog_info("功放已开启 (延时 %llums)", elapsed);
        }
        /* 无论延时是否结束，都将音频数据送入 Codec（功放关闭期间静默输出，保持 Codec 活跃） */
    }

    // 更新最后一次发送的时间戳
    m_lastAudioTime.store(getSteadyTimeMS());

    if (stAoInfo.enAudioFormat == Audio_NS::AudioFormat_E::G711A)
    {
        unTalkbackDataSize = stAoInfo.nLen * 2;
        m_bytesTalkbackData.clear();
        m_bytesTalkbackData.resize(unTalkbackDataSize);
        pTalkbackData = m_bytesTalkbackData.data();

        // 320字节
        /*解码g711a格式*/
        codec_g711a_decode(stAoInfo.pData, pTalkbackData, stAoInfo.nLen);
    }
    else if (stAoInfo.enAudioFormat == Audio_NS::AudioFormat_E::G711U)
    {
        unTalkbackDataSize = stAoInfo.nLen * 2;
        m_bytesTalkbackData.clear();
        m_bytesTalkbackData.resize(unTalkbackDataSize);
        pTalkbackData = m_bytesTalkbackData.data();

        /*解码g711u格式*/
        codec_g711u_decode(stAoInfo.pData, pTalkbackData, stAoInfo.nLen);
    }
    else if (stAoInfo.enAudioFormat == Audio_NS::AudioFormat_E::PCM)
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

    /*软件数据PCM 放大音量*/
    if (m_pAoHandle[AO_SPEAKER_CHN]->stNeedParam.enbitWidth == AUDIO_BIT_WIDTH_16)
    {
        short *ps16Data = (short *) pTalkbackData;
        int nSamples = unTalkbackDataSize / sizeof(short);

        for (int i = 0; i < nSamples; i++)
        {
            /* 放大 3 倍*/
            int val = (int) (ps16Data[i] * 3);
            if (val > 32767)
                val = 32767;
            else if (val < -32768)
                val = -32768;
            ps16Data[i] = (short) val;
        }
    }

    /*组音频帧，送数据至ao功放*/
    m_pAoHandle[AO_SPEAKER_CHN]->rockitAo_send_pcmData(m_pAoHandle[AO_SPEAKER_CHN], AO_SPEAKER_CHN, pTalkbackData, unTalkbackDataSize, -1);

    return OK;
}

int CStreamAudio::getAudioConfig(Audio_NS::AudioConfig_S &stAudioConfig)
{
    stAudioConfig = m_stAudioConfig;
    return OK;
}

int CStreamAudio::setAudioConfig(const Audio_NS::AudioConfig_S &stAudioConfig)
{
    /* 是否重启音频模块 */
    bool bIsReboot = false;
    /* 是否重新设置Rtsp */
    bool bIsResetRtsp = false;

    /* 输入类型改变 */
    if (m_stAudioConfig.enInputType != stAudioConfig.enInputType)
    {
        bIsReboot = true;
    }

    /* 输出类型改变 */
    if (m_stAudioConfig.enOutputType != stAudioConfig.enOutputType)
    {
        //m_streamAO->update_audioOutputType(stAudioConfig.enOutputType);
    }

    /* 输出音量改变 */
    if (m_stAudioConfig.u32OutputVolume != stAudioConfig.u32OutputVolume)
    {
        if (OK != streamAo_setVolume(m_pAoHandle[AO_SPEAKER_CHN], stAudioConfig.u32OutputVolume))
        {
            dlog_error("设置音频输出音量失败");
        }
    }

    m_bIsAac = stAudioConfig.enFormat == Audio_NS::AudioFormat_E::AAC ? true : false;

    /* 音频格式改变 */
    if (m_stAudioConfig.enFormat != stAudioConfig.enFormat)
    {
        bIsResetRtsp = true;
        bIsReboot = true;
    }

    /* 降噪开关改变 */
    if (m_stAudioConfig.bDenoise != stAudioConfig.bDenoise)
    {
        bIsReboot = true;
    }

    /*更新音频配置*/
    m_stAudioConfig = stAudioConfig;

    if (bIsResetRtsp)
    {
        /* 同步RTSP */
        CRtspServer::instance()->setAudioConfig(m_stAudioConfig);
        /*重新启动RTSP服务器*/
        CRtspServer::instance()->reboot();
    }

    if (bIsReboot)
    {
        /*重新启动音频流模块*/
        return reboot();
    }

    return OK;
}

int CStreamAudio::setAoSampleRate(const Audio_NS::AudioSamprate_E enSampRate)
{
    if (enSampRate != Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_16000 && enSampRate != Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_8000)
    {
        dlog_error("设置音频输出采样率参数错误");
        return ERR_PARAM;
    }

    int nAoDevice = AO_SPEAKER_CHN;
    if (m_pAoHandle[nAoDevice]->stNeedParam.enSampleRate != (AUDIO_SAMPLE_RATE_E) enSampRate)
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

int CStreamAudio::bindModule()
{
    int nRet = OK;
    // int nAdecDevId = 0;
    // int nAdecChnId = m_pAdecHandle[ADEC_SPEAK_CHN]->stNeedParam.nChn;
    // int nAoDevId = m_pAoHandle[AO_SPEAKER_CHN]->stNeedParam.nDevId;
    // int nAoChnId = AO_SPEAKER_CHN;
    // nRet |= rockitAdec_bind_ao(nAdecDevId, nAdecChnId, nAoDevId, nAoChnId);
    return nRet;
}

int CStreamAudio::unbindModule()
{
    int nRet = OK;
    // int nAdecDevId = 0;
    // int nAdecChnId = m_pAdecHandle[ADEC_SPEAK_CHN]->stNeedParam.nChn;
    // int nAoDevId = m_pAoHandle[AO_SPEAKER_CHN]->stNeedParam.nDevId;
    // int nAoChnId = AO_SPEAKER_CHN;
    // nRet |= rockitAdec_unbind_ao(nAdecDevId, nAdecChnId, nAoDevId, nAoChnId);

    return nRet;
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

    /* 绑定绑定音频采样率更新回调 */
    CAVConfigure::instance()->setAudioAoSampleRateCallback(
        [this](const Audio_NS::AudioSamprate &enSamprate) -> int
        {
            return this->setAoSampleRate(enSamprate);
        });

    /* 绑定静音功放输出回调（播放结束时提前关闭功放，消除结尾噗声） */
    CAVConfigure::instance()->setMuteAudioOutputCallback(
        [this]()
        {
            this->mutePa();
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
    Audio_NS::AudioFrame_S *pAudioFrame = (Audio_NS::AudioFrame_S *) malloc(sizeof(Audio_NS::AudioFrame_S) + nDataLen);
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
        /* 仅释放结构体，不释放 pData */
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

    int16_t *pcm = (int16_t *) pData;
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
    if (pData == NULL)
    {
        dlog_error("指针为空");
        return nullptr;
    }
    if (nDataLen < 0)
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

// info /*----------------------- 私有线程函数 -----------------------*/

void CStreamAudio::deal_aiFrame_thr(int param)
{
    pthread_setname_np(pthread_self(), "deal_aiFrame_thr");
    int nRet = OK;
    AUDIO_FRAME_S stFrame;
    AEC_FRAME_S stAecFrame;
    /* 音频数据指针 */
    unsigned char *pData = NULL;
    char tempBuf8k[4096];
    int nChannel = param;
    RkAi_S *pHandle = m_pAiHandle[nChannel];

    // 初始化音频WAV文件
    // CAudioWav audio_wav;
    // audio_wav.init(16000, 16, 1);
    // audio_wav.open("/tmp/ai.wav");

    while (true == m_bGetAiFlag[nChannel].load(std::memory_order_acquire))
    {
        memset(&stFrame, 0, sizeof(AUDIO_FRAME_S));

        /* 获取ai采集数据 */
        nRet = pHandle->rockitAi_get_frame(pHandle, nChannel, &stFrame, &stAecFrame, -1);
        if (nRet != RK_SUCCESS)
        {
            dlog_info("获取ai chn[%d] 采集数据失败 ret:[%x]", nChannel, nRet);
            continue;
        }

        /* 拿编码后的虚拟地址 */
        pData = pHandle->rockitAi_get_virData(&stFrame);

        {
            std::lock_guard<std::mutex> lock(m_mutexSendData);
            // dlog_debug("stFrame.u32Len:%d", stFrame.u32Len);
            // audio_wav.write(pData, stFrame.u32Len);
            if (AI_MIC_CHN == nChannel)
            {
                /* 音量调整 */
                volume_adjust(reinterpret_cast<int8_t *>(pData), stFrame.u32Len);
                CVoiceComCaptureSource::instance()->push_pcm_frame(pData, stFrame.u32Len);

                /* 送ffmpeg AAC编码 */
                if (m_pFfAencHandle != NULL)
                {
                    m_pFfAencHandle->send_frame(m_pFfAencHandle, (char *) pData, stFrame.u32Len);
                }

                /* 发送数据到 AI_APP */
                algo_send_audioStreamData(pData, stFrame.u32Len);

                if (!m_bIsAac)
                {
                    if (m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A || m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U)
                    {
                        /* 防止越界 */
                        if ((stFrame.u32Len / 2) > sizeof(tempBuf8k))
                            dlog(LOG_ERROR, "Error: Frame too large for buffer!\n");

                        /* 执行重采样 16K->8K */
                        /* G711A 和 G711U 标准要求8K采样率 */
                        resample_16k_to_8k_fir(&ctx, (int16_t *) pData, (int16_t *) tempBuf8k, stFrame.u32Len);

                        /* 编码g711格式 */
                        Audio_NS::AudioFrame_S *pAudioFrame = encode_g711((uint8_t *) tempBuf8k, stFrame.u32Len / 2);
                        /* 送推流模块 RTSP */
                        CPushStream::instance()->sendAudioData(pAudioFrame);
                        /* 释放帧数据 */
                        freeFrame(pAudioFrame);
                    }
                    else if (m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::PCM)
                    {
                        // todo 未完成 PCM
                    }
                }
            }
        }

        /*释放ai获取的音频数据缓存*/
        pHandle->rockitAi_release_frame(pHandle, nChannel, &stFrame, &stAecFrame);

        /* 主动放弃CPU */
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // audio_wav.uninit();
    pData = NULL;
    return;
}

#if 0
void CStreamAudio::deal_aencFrame_thr(int param)
{
    pthread_setname_np(pthread_self(), "deal_aencFrame_thr");

    Audio_NS::AudioFrame_S *pAudioFrame = nullptr;

    int nRet = OK;
    AUDIO_STREAM_S stStream;
    int nChannel = param;
    RkAenc_S *pHandle = m_pAencHandle[nChannel];
    uint8_t *pData = nullptr;

    while (m_bGetAencFlag[nChannel].load(std::memory_order_acquire))
    {
        memset(&stStream, 0, sizeof(AUDIO_STREAM_S));
        /*获取编码数据*/
        nRet = pHandle->rockitAenc_get_frame(pHandle, &stStream,-1);
        if (nRet != OK)
        {
            dlog_info("获取aenc数据帧失败:%x", nRet);
            continue;
        }

        /* 拿编码后的虚拟地址 */
        pData = pHandle->rockitAenc_get_virData(&stStream);
        if (pData == nullptr)
        {
            /*释放码流缓存*/
            pHandle->rockitAenc_release_frame(pHandle, &stStream);
            continue;
        }

        pAudioFrame = createFrame(pData, stStream.u32Len);
        pHandle->rockitAenc_release_frame(pHandle, &stStream);

        /* 录制模块 */
        CStreamServer::instance()->sendAudioData(pAudioFrame);
        /* 推给rtsp */
        CPushStream::instance()->sendAudioData(pAudioFrame, true, true);
        /* 推给gb28181 */
        SIP::CRtpServer::instance()->sendAudioData(pAudioFrame);

        freeFrame(pAudioFrame);
    }

    return;
}
#endif

void CStreamAudio::deal_ffAencFrame_thr()
{
    pthread_setname_np(pthread_self(), "dealffAenc_thr");

    Audio_NS::AudioFrame_S *pAudioFrame = NULL;

    // std::ofstream aacFile;
    // aacFile.open("/opt/cam/bin/ffAenc.aac", std::ios::binary);

    while (true == m_bGetFfAencFlag.load(std::memory_order_acquire))
    {
        /*获取编码数据*/
        AVPacket *pPacket = m_pFfAencHandle->receive_packet(m_pFfAencHandle, -1);
        if (!pPacket)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        // if (aacFile.is_open())
        // {
        //     aacFile.write(reinterpret_cast<const char *>(pPacket->data), pPacket->size);
        // }

        // note /*移除ADTS头，RTSP流不需要ADTS头，只需要AAC裸数据*/
        int nADTSLen = 0;
        /*检查同步字（0xFFF）*/
        if ((pPacket->data[0] != 0xFF) || ((pPacket->data[1] & 0xF0) != 0xF0))
        {
            dlog_debug("非ADTS头");
            /*释放码流缓存*/
            m_pFfAencHandle->release_packet(pPacket);
            continue;
        }
        /*提取保护位（第2字节的最低位）判断是否有CRC校验*/
        if (!(pPacket->data[1] & 0x01))
        {
            nADTSLen = 9; // 有CRC校验
        }
        else
        {
            nADTSLen = 7; // 无CRC校验
        }

        pAudioFrame = createFrame(pPacket->data + nADTSLen, pPacket->size - nADTSLen);
        m_pFfAencHandle->release_packet(pPacket);

        if (CRecordCtrl::instance()->get_record_status() == Record_NS::Status_E::RECORD_OPERATION)
        {
            /* 录制模块 */
            CStreamServer::instance()->sendAudioData(pAudioFrame);
        }

        if (m_bIsAac)
        {
            /* 推给rtsp */
            // CPushStream::instance()->sendAudioData(pAudioFrame, true, true);
            CPushStream::instance()->sendAudioData(pAudioFrame);
            /* 推给gb28181 */
            SIP::CRtpServer::instance()->sendAudioData(pAudioFrame);
        }

        freeFrame(pAudioFrame);
    }

    // aacFile.close();

    return;
}

void CStreamAudio::mutePa()
{
    m_streamAO->update_audioOutputType(Audio_NS::AudioOutputType_E::MUTE);
    m_paEnable.store(false);
    m_bPaDelayActive.store(false);
    dlog_info("功放已静音");
}

/*监测AO对应输出模块GPIO状态线程*/
void CStreamAudio::paMonitorTimer_thr()
{
    pthread_setname_np(pthread_self(), "Audio_PA_Watchdog_Timer_thr");

    const uint64_t IDLE_TIMEOUT_MS = 2000; // 2秒无声就自动静音

    while (!m_bStopThread.load())
    {
        uint64_t now = getSteadyTimeMS();
        uint64_t last = m_lastAudioTime.load();

        // 如果 PA 是开着的，且当前时间距离上次发声超过了 2 秒
        if (m_paEnable.load() && (now - last > IDLE_TIMEOUT_MS))
        {
            // 执行硬件静音
            m_streamAO->update_audioOutputType(Audio_NS::AudioOutputType_E::MUTE);
            // 更新标志位
            m_paEnable.store(false);
            m_bPaDelayActive.store(false);
        }

        // 每 200 毫秒轮询一次
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
