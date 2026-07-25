/**
 * @FilePath     : rtmp_session.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:36:21
 * @Description  : RTMP单路推流会话实现
 */

#include "rtmp_session.h"

#include <algorithm>

#include "aac_extradata_builder.h"
#include "flv_packet_converter.h"
#include "annexb_nal_parser.h"
#include "video_extradata_builder.h"

namespace
{
    /* 音频开启但首帧异常时，最多等待该时长后降级为纯视频RTMP，避免视频长期无法出流 */
    const int64_t AUDIO_WAIT_TIMEOUT_MS = 3000;
    /* 常规诊断日志间隔，避免每帧打印影响实时推流 */
    const int64_t RTMP_DIAG_LOG_INTERVAL_MS = 5000;
    /* 启动等待类日志间隔，用于观察是否卡在参数集、音频或关键帧 */
    const int64_t RTMP_WAIT_LOG_INTERVAL_MS = 2000;
    /* 单次写包或flush超过该阈值时认为可能存在阻塞 */
    const int64_t RTMP_WRITE_BLOCK_WARN_MS = 300;

    int64_t rtmp_now_ms()
    {
        return av_gettime_relative() / 1000;
    }

    bool should_log_by_interval(int64_t& nLastLogMs, int64_t nIntervalMs)
    {
        const int64_t nNowMs = rtmp_now_ms();
        if (nLastLogMs == 0 || nNowMs - nLastLogMs >= nIntervalMs)
        {
            nLastLogMs = nNowMs;
            return true;
        }
        return false;
    }

    uint8_t data_byte_at(const uint8_t* pData, int nLen, int nIndex)
    {
        if (!pData || nIndex < 0 || nIndex >= nLen)
        {
            return 0;
        }
        return pData[nIndex];
    }

    /**
     * @brief 计算视频帧时长
     * @param stVideoConfig 视频配置
     * @return 帧时长，单位毫秒
     */
    int64_t calc_video_frame_duration_ms(const Video_NS::VideoConfig_S& stVideoConfig)
    {
        float fFrameRate = stVideoConfig.getFrameRateAsFloat();
        if (fFrameRate <= 0.1f)
        {
            fFrameRate = 25.0f;
        }
        return std::max<int64_t>(1, static_cast<int64_t>((1000.0f / fFrameRate) + 0.5f));
    }

    /**
     * @brief 计算AAC帧时长
     * @param nSampleRate 采样率
     * @return AAC帧时长，单位毫秒
     */
    int64_t calc_aac_frame_duration_ms(int nSampleRate)
    {
        if (nSampleRate <= 0)
        {
            nSampleRate = 16000;
        }
        return std::max<int64_t>(1, static_cast<int64_t>((1024.0 * 1000.0 / nSampleRate) + 0.5));
    }
}

CRtmpSession::CRtmpSession(int nChannel,
                           const std::string& strUrl,
                           const Video_NS::VideoConfig_S& stVideoConfig,
                           const Audio_NS::AudioConfig_S& stAudioConfig)
    : m_nChannel(nChannel)
    , m_strUrl(strUrl)
    , m_stVideoConfig(stVideoConfig)
    , m_stAudioConfig(stAudioConfig)
    , m_nVideoFrameDurationMs(calc_video_frame_duration_ms(stVideoConfig))
{
}

CRtmpSession::~CRtmpSession()
{
    deinit();
}

int CRtmpSession::init()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_bConnected.load())
    {
        dlog_warn("RTMP会话已初始化，通道=%d", m_nChannel);
        return OK;
    }

    dlog_info("RTMP会话准备连接，通道=%d, URL=%s, codec=%d, width=%d, height=%d, fps=%d, audio_switch=%d, audio_format=%d, sample_rate=%d",
              m_nChannel,
              m_strUrl.c_str(),
              static_cast<int>(m_stVideoConfig.enVideoCodec),
              m_stVideoConfig.stVideoResolution.nWidth,
              m_stVideoConfig.stVideoResolution.nHeight,
              m_stVideoConfig.getFrameRateAsInt(),
              m_stAudioConfig.bAudioSwitch ? 1 : 0,
              static_cast<int>(m_stAudioConfig.enFormat),
              static_cast<int>(m_stAudioConfig.enSampRate));

    int nRet = m_stream_context.open(m_strUrl);
    if (nRet < 0)
    {
        return nRet;
    }

    m_bConnected.store(true);
    m_bVideoReady = false;
    m_bAudioReady = false;
    m_bAudioDisabled = false;
    m_bHeaderWritten = false;
    m_bNeedVideoKeyFrame = true;
    m_nVideoPts = 0;
    m_nAudioPts = 0;
    m_nAudioWaitStartMs = 0;
    m_uVideoEnqueueCount.store(0);
    m_uAudioEnqueueCount.store(0);
    m_uVideoSendCount.store(0);
    m_uAudioSendCount.store(0);
    m_uVideoDropCount.store(0);
    m_uAudioDropCount.store(0);
    m_uVideoWaitParamCount.store(0);
    m_uVideoWaitKeyCount.store(0);
    m_nLastQueueLogMs = 0;
    m_nLastStateLogMs = 0;
    m_nLastWaitParamLogMs = 0;
    m_nLastWaitAudioLogMs = 0;
    m_nLastWaitKeyLogMs = 0;

    /* 初始化帧队列（在锁保护下） */
    {
        std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
        m_videoQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_VIDEO_FRAME);
        m_audioQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_AUDIO_FRAME);
    }

    /* 启动发送线程 */
    m_bStopSend.store(false);
    m_sendThread = std::thread(&CRtmpSession::send_loop, this);

    dlog_info("RTMP会话连接成功，通道=%d, URL=%s", m_nChannel, m_strUrl.c_str());

    return OK;
}

void CRtmpSession::deinit()
{
    /* 停止发送线程 */
    m_bStopSend.store(true);

    /* 停止队列等待，使发送线程快速退出 */
    {
        std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
        if (m_videoQueue) m_videoQueue->stop();
        if (m_audioQueue) m_audioQueue->stop();
    }

    /* 等待发送线程退出（设置合理超时） */
    if (m_sendThread.joinable())
    {
        auto startTime = std::chrono::steady_clock::now();
        while (m_bSendThreadRunning.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > 3)
            {
                dlog_warn("RTMP发送线程退出超时，强制分离，通道=%d", m_nChannel);
                m_sendThread.detach();
                break;
            }
        }
        if (m_sendThread.joinable())
        {
            m_sendThread.join();
        }
    }

    size_t nVideoQueueSize = 0;
    size_t nAudioQueueSize = 0;
    {
        std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
        nVideoQueueSize = m_videoQueue ? m_videoQueue->size() : 0;
        nAudioQueueSize = m_audioQueue ? m_audioQueue->size() : 0;
    }
    dlog_info("RTMP会话退出诊断，通道=%d, video_enq=%llu, video_send=%llu, video_drop=%llu, audio_enq=%llu, audio_send=%llu, audio_drop=%llu, wait_param=%llu, wait_key=%llu, video_q=%d, audio_q=%d",
              m_nChannel,
              static_cast<unsigned long long>(m_uVideoEnqueueCount.load()),
              static_cast<unsigned long long>(m_uVideoSendCount.load()),
              static_cast<unsigned long long>(m_uVideoDropCount.load()),
              static_cast<unsigned long long>(m_uAudioEnqueueCount.load()),
              static_cast<unsigned long long>(m_uAudioSendCount.load()),
              static_cast<unsigned long long>(m_uAudioDropCount.load()),
              static_cast<unsigned long long>(m_uVideoWaitParamCount.load()),
              static_cast<unsigned long long>(m_uVideoWaitKeyCount.load()),
              static_cast<int>(nVideoQueueSize),
              static_cast<int>(nAudioQueueSize));

    std::lock_guard<std::mutex> lock(m_mutex);

    /* 清空队列 */
    {
        std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
        if (m_videoQueue)
        {
            m_videoQueue->clear();
            m_videoQueue.reset();
        }
        if (m_audioQueue)
        {
            m_audioQueue->clear();
            m_audioQueue.reset();
        }
    }

    m_stream_context.close(m_bHeaderWritten);
    m_bConnected.store(false);
    m_bVideoReady = false;
    m_bAudioReady = false;
    m_bAudioDisabled = false;
    m_bHeaderWritten = false;
    m_bNeedVideoKeyFrame = true;
    m_nVideoPts = 0;
    m_nAudioPts = 0;
    m_nAudioWaitStartMs = 0;
}

bool CRtmpSession::is_connected() const
{
    return m_bConnected.load();
}

int CRtmpSession::send_video_frame(Video_NS::VideoFrame_S* pVideoFrame)
{
    if (!pVideoFrame || pVideoFrame->nLen <= 0)
    {
        dlog_error("视频帧数据无效");
        return ERR;
    }

    /* 检查连接状态（原子变量，无需锁） */
    if (!m_bConnected.load())
    {
        return ERR;
    }

    /* 创建帧数据 */
    auto frameData = std::make_unique<FrameData>();
    frameData->data = std::make_unique<unsigned char[]>(pVideoFrame->nLen);
    frameData->type = FRAME_TYPE_VIDEO;
    frameData->frameSize = pVideoFrame->nLen;
    frameData->iFrame = is_key_frame(pVideoFrame) ? 1 : 0;
    const int nIsKeyFrame = frameData->iFrame;

    /* 拷贝数据 */
    memcpy(frameData->data.get(), pVideoFrame->pData, pVideoFrame->nLen);

    /* 入队（队列满时丢帧）- 使用独立的队列锁 */
    std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
    const bool bPushOk = m_videoQueue && m_videoQueue->push(std::move(frameData));
    const size_t nVideoQueueSize = m_videoQueue ? m_videoQueue->size() : 0;
    const size_t nAudioQueueSize = m_audioQueue ? m_audioQueue->size() : 0;
    if (!bPushOk)
    {
        const uint64_t uDropCount = m_uVideoDropCount.fetch_add(1) + 1;
        // dlog_warn("RTMP视频队列已满或未初始化，丢弃帧，通道=%d, len=%d, key=%d, video_q=%d, audio_q=%d, video_drop=%llu",
        //           m_nChannel,
        //           pVideoFrame->nLen,
        //           nIsKeyFrame,
        //           static_cast<int>(nVideoQueueSize),
        //           static_cast<int>(nAudioQueueSize),
        //           static_cast<unsigned long long>(uDropCount));
    }
    else
    {
        m_uVideoEnqueueCount.fetch_add(1);
        // if (should_log_by_interval(m_nLastQueueLogMs, RTMP_DIAG_LOG_INTERVAL_MS))
        // {
        //     dlog_info("RTMP入队诊断，通道=%d, video_q=%d, audio_q=%d, video_enq=%llu, video_send=%llu, video_drop=%llu, audio_enq=%llu, audio_send=%llu, audio_drop=%llu",
        //               m_nChannel,
        //               static_cast<int>(nVideoQueueSize),
        //               static_cast<int>(nAudioQueueSize),
        //               static_cast<unsigned long long>(m_uVideoEnqueueCount.load()),
        //               static_cast<unsigned long long>(m_uVideoSendCount.load()),
        //               static_cast<unsigned long long>(m_uVideoDropCount.load()),
        //               static_cast<unsigned long long>(m_uAudioEnqueueCount.load()),
        //               static_cast<unsigned long long>(m_uAudioSendCount.load()),
        //               static_cast<unsigned long long>(m_uAudioDropCount.load()));
        // }
    }

    return OK;
}

int CRtmpSession::send_audio_frame(Audio_NS::AudioFrame_S* pAudioFrame)
{
    if (!pAudioFrame || pAudioFrame->nLen <= 0)
    {
        dlog_error("音频帧数据无效");
        return ERR_PARAM;
    }

    /* 检查连接状态（原子变量，无需锁） */
    if (!m_bConnected.load())
    {
        return ERR;
    }

    if (m_bAudioDisabled)
    {
        return OK;
    }

    /* 创建帧数据 */
    auto frameData = std::make_unique<FrameData>();
    frameData->data = std::make_unique<unsigned char[]>(pAudioFrame->nLen);
    frameData->type = FRAME_TYPE_AUDIO;
    frameData->frameSize = pAudioFrame->nLen;

    /* 拷贝数据 */
    memcpy(frameData->data.get(), pAudioFrame->pData, pAudioFrame->nLen);

    /* 入队（队列满时丢帧）- 使用独立的队列锁 */
    std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
    const bool bPushOk = m_audioQueue && m_audioQueue->push(std::move(frameData));
    const size_t nVideoQueueSize = m_videoQueue ? m_videoQueue->size() : 0;
    const size_t nAudioQueueSize = m_audioQueue ? m_audioQueue->size() : 0;
    if (!bPushOk)
    {
        const uint64_t uDropCount = m_uAudioDropCount.fetch_add(1) + 1;
        dlog_warn("RTMP音频队列已满或未初始化，丢弃帧，通道=%d, len=%d, video_q=%d, audio_q=%d, audio_drop=%llu",
                  m_nChannel,
                  pAudioFrame->nLen,
                  static_cast<int>(nVideoQueueSize),
                  static_cast<int>(nAudioQueueSize),
                  static_cast<unsigned long long>(uDropCount));
    }
    else
    {
        m_uAudioEnqueueCount.fetch_add(1);
    }

    return OK;
}

int CRtmpSession::init_video_stream(Video_NS::VideoFrame_S* pVideoFrame)
{
    RtmpVideo_NS::VideoParameterSets_S stSets;
    std::vector<uint8_t> vExtradata;
    if (pVideoFrame->enVideoCodec == Video_NS::VideoCodec_E::H264)
    {
        if (!RtmpVideo_NS::extract_h264_parameter_sets(pVideoFrame->pData, pVideoFrame->nLen, stSets) ||
            !RtmpVideo_NS::build_avc_extradata(stSets, vExtradata))
        {
            const uint64_t uWaitCount = m_uVideoWaitParamCount.fetch_add(1) + 1;
            if (should_log_by_interval(m_nLastWaitParamLogMs, RTMP_WAIT_LOG_INTERVAL_MS))
            {
                dlog_warn("RTMP等待H.264 SPS/PPS参数集，通道=%d, len=%d, wait_count=%llu, first=%02x %02x %02x %02x %02x %02x %02x %02x",
                          m_nChannel,
                          pVideoFrame->nLen,
                          static_cast<unsigned long long>(uWaitCount),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 0),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 1),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 2),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 3),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 4),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 5),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 6),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 7));
            }
            return ERR;
        }
    }
    else if (pVideoFrame->enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        if (!RtmpVideo_NS::extract_h265_parameter_sets(pVideoFrame->pData, pVideoFrame->nLen, stSets) ||
            !RtmpVideo_NS::build_hevc_extradata(stSets, vExtradata))
        {
            const uint64_t uWaitCount = m_uVideoWaitParamCount.fetch_add(1) + 1;
            if (should_log_by_interval(m_nLastWaitParamLogMs, RTMP_WAIT_LOG_INTERVAL_MS))
            {
                dlog_warn("RTMP等待H.265 VPS/SPS/PPS参数集，通道=%d, len=%d, wait_count=%llu, first=%02x %02x %02x %02x %02x %02x %02x %02x",
                          m_nChannel,
                          pVideoFrame->nLen,
                          static_cast<unsigned long long>(uWaitCount),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 0),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 1),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 2),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 3),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 4),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 5),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 6),
                          data_byte_at(pVideoFrame->pData, pVideoFrame->nLen, 7));
            }
            return ERR;
        }
    }
    else
    {
        dlog_error("RTMP不支持当前视频编码，通道=%d, codec=%d",
                   m_nChannel,
                   static_cast<int>(pVideoFrame->enVideoCodec));
        return ERR;
    }

    int nFrameRate = m_stVideoConfig.getFrameRateAsInt();
    if (nFrameRate <= 0 || nFrameRate > 0xffff)
    {
        nFrameRate = 25;
    }

    int nRet = m_stream_context.create_video_stream(pVideoFrame->enVideoCodec,
                                                    m_stVideoConfig.stVideoResolution.nWidth,
                                                    m_stVideoConfig.stVideoResolution.nHeight,
                                                    nFrameRate,
                                                    vExtradata,
                                                    m_nChannel);
    if (nRet < 0)
    {
        return nRet;
    }

    m_nVideoFrameDurationMs = calc_video_frame_duration_ms(m_stVideoConfig);
    dlog_info("RTMP视频流初始化成功，通道=%d, codec=%d, width=%d, height=%d, fps=%d, first_frame_len=%d, extradata=%d",
              m_nChannel,
              static_cast<int>(pVideoFrame->enVideoCodec),
              m_stVideoConfig.stVideoResolution.nWidth,
              m_stVideoConfig.stVideoResolution.nHeight,
              nFrameRate,
              pVideoFrame->nLen,
              static_cast<int>(vExtradata.size()));
    return OK;
}

int CRtmpSession::init_audio_stream(Audio_NS::AudioFrame_S* pAudioFrame, const RtmpAudio_NS::AacAdtsInfo_S& stAdtsInfo)
{
    std::vector<uint8_t> vExtradata;
    if (!RtmpAudio_NS::build_aac_extradata(stAdtsInfo, vExtradata))
    {
        dlog_error("构造AAC extradata失败，通道=%d", m_nChannel);
        return ERR;
    }

    int nRet = m_stream_context.create_audio_stream(stAdtsInfo.sample_rate, stAdtsInfo.channels, vExtradata, m_nChannel);
    if (nRet < 0)
    {
        return nRet;
    }

    m_nAudioFrameDurationMs = calc_aac_frame_duration_ms(stAdtsInfo.sample_rate);
    dlog_info("RTMP音频流初始化成功，通道=%d, sample_rate=%d, channels=%d, frame_len=%d, extradata=%d",
              m_nChannel,
              stAdtsInfo.sample_rate,
              stAdtsInfo.channels,
              pAudioFrame ? pAudioFrame->nLen : 0,
              static_cast<int>(vExtradata.size()));
    return OK;
}

int CRtmpSession::try_write_header()
{
    if (m_bHeaderWritten)
    {
        return OK;
    }
    if (!m_bVideoReady)
    {
        return OK;
    }
    if (need_audio_stream() && !m_bAudioReady)
    {
        /* 配置开启音频时，优先等待首个音频帧创建 AAC 流，保证 header 同时声明音视频。 */
        int64_t nNowMs = av_gettime_relative() / 1000;
        if (m_nAudioWaitStartMs <= 0)
        {
            m_nAudioWaitStartMs = nNowMs;
            return OK;
        }
        if (nNowMs - m_nAudioWaitStartMs < AUDIO_WAIT_TIMEOUT_MS)
        {
            if (should_log_by_interval(m_nLastWaitAudioLogMs, RTMP_WAIT_LOG_INTERVAL_MS))
            {
                dlog_warn("RTMP等待音频首帧后再写FLV header，通道=%d, waited=%lldms, timeout=%lldms, audio_enq=%llu",
                          m_nChannel,
                          static_cast<long long>(nNowMs - m_nAudioWaitStartMs),
                          static_cast<long long>(AUDIO_WAIT_TIMEOUT_MS),
                          static_cast<unsigned long long>(m_uAudioEnqueueCount.load()));
            }
            return OK;
        }

        /* 音频链路异常不能阻塞视频出流，超时后固定为纯视频会话。 */
        m_bAudioDisabled = true;
        dlog_warn("RTMP等待音频超时，降级为纯视频推流，通道=%d, timeout=%lldms",
                  m_nChannel,
                  static_cast<long long>(AUDIO_WAIT_TIMEOUT_MS));
    }
    if (!m_bAudioReady && !need_audio_stream())
    {
        /* 未配置 AAC 或已降级时，header 写出后不允许再追加音频流。 */
        m_bAudioDisabled = true;
    }

    dlog_info("RTMP准备写FLV header，通道=%d, video_ready=%d, audio_ready=%d, audio_disabled=%d, video_enq=%llu, audio_enq=%llu",
              m_nChannel,
              m_bVideoReady ? 1 : 0,
              m_bAudioReady ? 1 : 0,
              m_bAudioDisabled ? 1 : 0,
              static_cast<unsigned long long>(m_uVideoEnqueueCount.load()),
              static_cast<unsigned long long>(m_uAudioEnqueueCount.load()));

    int nRet = m_stream_context.write_header(m_stVideoConfig.enVideoCodec, m_nChannel);
    if (nRet < 0)
    {
        m_bConnected.store(false);
        return nRet;
    }

    m_bHeaderWritten = true;
    m_bNeedVideoKeyFrame = true;
    return OK;
}

bool CRtmpSession::need_audio_stream() const
{
    return !m_bAudioDisabled && m_stAudioConfig.bAudioSwitch &&
           m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::AAC;
}

bool CRtmpSession::is_key_frame(Video_NS::VideoFrame_S* pVideoFrame) const
{
    if (!pVideoFrame)
    {
        return false;
    }

    if (pVideoFrame->eType == Video_NS::H264_TYPE_IDR ||
        pVideoFrame->eType == Video_NS::H265_TYPE_IDR_W_RADL ||
        pVideoFrame->eType == Video_NS::H265_TYPE_IDR_N_LP ||
        pVideoFrame->eType == Video_NS::H265_TYPE_CRA)
    {
        return true;
    }

    return RtmpVideo_NS::has_key_frame_nal(pVideoFrame->pData, pVideoFrame->nLen, pVideoFrame->enVideoCodec);
}

void CRtmpSession::send_loop()
{
    pthread_setname_np(pthread_self(), "rtmp_send");
    m_bSendThreadRunning.store(true);
    dlog_info("RTMP发送线程启动，通道=%d", m_nChannel);

    while (!m_bStopSend.load())
    {
        if (!m_bConnected.load())
        {
            /* 未连接时等待 */
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (should_log_by_interval(m_nLastStateLogMs, RTMP_DIAG_LOG_INTERVAL_MS))
        {
            size_t nVideoQueueSize = 0;
            size_t nAudioQueueSize = 0;
            {
                std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
                nVideoQueueSize = m_videoQueue ? m_videoQueue->size() : 0;
                nAudioQueueSize = m_audioQueue ? m_audioQueue->size() : 0;
            }
            // dlog_info("RTMP发送线程诊断，通道=%d, connected=%d, header=%d, video_ready=%d, audio_ready=%d, audio_disabled=%d, need_key=%d, video_q=%d, audio_q=%d, video_enq=%llu, video_send=%llu, video_drop=%llu, audio_enq=%llu, audio_send=%llu, audio_drop=%llu",
            //           m_nChannel,
            //           m_bConnected.load() ? 1 : 0,
            //           m_bHeaderWritten ? 1 : 0,
            //           m_bVideoReady ? 1 : 0,
            //           m_bAudioReady ? 1 : 0,
            //           m_bAudioDisabled ? 1 : 0,
            //           m_bNeedVideoKeyFrame ? 1 : 0,
            //           static_cast<int>(nVideoQueueSize),
            //           static_cast<int>(nAudioQueueSize),
            //           static_cast<unsigned long long>(m_uVideoEnqueueCount.load()),
            //           static_cast<unsigned long long>(m_uVideoSendCount.load()),
            //           static_cast<unsigned long long>(m_uVideoDropCount.load()),
            //           static_cast<unsigned long long>(m_uAudioEnqueueCount.load()),
            //           static_cast<unsigned long long>(m_uAudioSendCount.load()),
            //           static_cast<unsigned long long>(m_uAudioDropCount.load()));
        }

        /* 获取视频帧 */
        std::unique_ptr<FrameData> videoFrame;
        {
            std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
            if (m_videoQueue)
            {
                videoFrame = m_videoQueue->pop();
            }
        }

        if (videoFrame)
        {
            int nRet = process_video_frame(std::move(videoFrame));
            if (nRet < 0)
            {
                dlog_error("RTMP视频帧发送失败，通道=%d", m_nChannel);
                m_bConnected.store(false);
                /* 清空队列 */
                std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
                if (m_videoQueue) m_videoQueue->clear();
                if (m_audioQueue) m_audioQueue->clear();
                continue;
            }
        }

        /* 处理音频帧 */
        std::unique_ptr<FrameData> audioFrame;
        {
            std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
            if (m_audioQueue)
            {
                audioFrame = m_audioQueue->pop();
            }
        }

        if (audioFrame)
        {
            int nRet = process_audio_frame(std::move(audioFrame));
            if (nRet < 0)
            {
                dlog_error("RTMP音频帧发送失败，通道=%d", m_nChannel);
                m_bConnected.store(false);
                /* 清空队列 */
                std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
                if (m_videoQueue) m_videoQueue->clear();
                if (m_audioQueue) m_audioQueue->clear();
                continue;
            }
        }

        /* 队列为空时短暂休眠，避免空转 */
        if (!videoFrame && !audioFrame)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    m_bSendThreadRunning.store(false);
    dlog_info("RTMP发送线程退出，通道=%d", m_nChannel);
}

int CRtmpSession::process_video_frame(std::unique_ptr<FrameData> pFrameData)
{
    if (!pFrameData || pFrameData->frameSize <= 0)
    {
        return ERR;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected.load() || !m_stream_context.is_open())
    {
        return ERR;
    }

    /* 获取帧数据指针和长度 */
    uint8_t* pData = pFrameData->data.get();
    int nLen = pFrameData->frameSize;
    bool bIsKeyFrame = pFrameData->iFrame == 1;
    Video_NS::VideoCodec_E enVideoCodec = m_stVideoConfig.enVideoCodec;

    if (!m_bVideoReady)
    {
        /* 构造临时VideoFrame_S用于初始化 */
        /* 注意：VideoFrame_S使用柔性数组，需要动态分配 */
        int nFrameSize = sizeof(Video_NS::VideoFrame_S) + nLen;
        Video_NS::VideoFrame_S* pVideoFrame = reinterpret_cast<Video_NS::VideoFrame_S*>(new uint8_t[nFrameSize]);
        pVideoFrame->enVideoCodec = enVideoCodec;
        pVideoFrame->eType = bIsKeyFrame ? Video_NS::H265_TYPE_IDR_W_RADL : Video_NS::H265_TYPE_TRAIL_R;
        pVideoFrame->nLen = nLen;
        memcpy(pVideoFrame->pData, pData, nLen);

        /* 初始化失败时静默丢帧，等待后续关键帧重试 */
        int nRet = init_video_stream(pVideoFrame);
        delete[] reinterpret_cast<uint8_t*>(pVideoFrame);
        if (nRet < 0)
        {
            return OK;
        }
        m_bVideoReady = true;
    }

    if (try_write_header() < 0 || !m_bHeaderWritten)
    {
        return OK;
    }

    if (m_bNeedVideoKeyFrame && !bIsKeyFrame)
    {
        const uint64_t uWaitKeyCount = m_uVideoWaitKeyCount.fetch_add(1) + 1;
        if (should_log_by_interval(m_nLastWaitKeyLogMs, RTMP_WAIT_LOG_INTERVAL_MS))
        {
            dlog_warn("RTMP等待关键帧，通道=%d, len=%d, wait_key=%llu, first=%02x %02x %02x %02x %02x %02x %02x %02x",
                      m_nChannel,
                      nLen,
                      static_cast<unsigned long long>(uWaitKeyCount),
                      data_byte_at(pData, nLen, 0),
                      data_byte_at(pData, nLen, 1),
                      data_byte_at(pData, nLen, 2),
                      data_byte_at(pData, nLen, 3),
                      data_byte_at(pData, nLen, 4),
                      data_byte_at(pData, nLen, 5),
                      data_byte_at(pData, nLen, 6),
                      data_byte_at(pData, nLen, 7));
        }
        return OK;
    }
    if (m_bNeedVideoKeyFrame && bIsKeyFrame)
    {
        m_bNeedVideoKeyFrame = false;
        dlog_info("RTMP收到关键帧，开始发送视频，通道=%d", m_nChannel);
    }

    std::vector<uint8_t> vMp4Data;
    if (enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        if (!RtmpFlv_NS::annexb_to_mp4(pData, nLen, vMp4Data))
        {
            dlog_error("H.265视频帧AnnexB转MP4失败，通道=%d", m_nChannel);
            return ERR;
        }
    }

    AVPacket pkt = { 0 };
    if (enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        pkt.data = vMp4Data.data();
        pkt.size = static_cast<int>(vMp4Data.size());
    }
    else
    {
        pkt.data = pData;
        pkt.size = nLen;
    }
    pkt.stream_index = m_stream_context.video_stream_index();
    if (bIsKeyFrame)
    {
        pkt.flags |= AV_PKT_FLAG_KEY;
    }

    pkt.pts = m_nVideoPts;
    pkt.dts = m_nVideoPts;
    m_nVideoPts += m_nVideoFrameDurationMs;

    AVRational stTimeBase = { 1, 1000 };
    AVRational stStreamTimeBase = m_stream_context.video_time_base();
    pkt.pts = av_rescale_q(pkt.pts, stTimeBase, stStreamTimeBase);
    pkt.dts = av_rescale_q(pkt.dts, stTimeBase, stStreamTimeBase);
    pkt.duration = av_rescale_q(m_nVideoFrameDurationMs, stTimeBase, stStreamTimeBase);

    const int64_t nWriteStartMs = rtmp_now_ms();
    int nRet = m_stream_context.write_frame(&pkt);
    const int64_t nWriteCostMs = rtmp_now_ms() - nWriteStartMs;
    if (nRet < 0)
    {
        char errBuf[256] = { 0 };
        av_strerror(nRet, errBuf, sizeof(errBuf));
        dlog_error("写入视频帧失败: %s", errBuf);
        return ERR;
    }

    const int64_t nFlushStartMs = rtmp_now_ms();
    m_stream_context.flush();
    const int64_t nFlushCostMs = rtmp_now_ms() - nFlushStartMs;
    const uint64_t uSendCount = m_uVideoSendCount.fetch_add(1) + 1;
    if (nWriteCostMs >= RTMP_WRITE_BLOCK_WARN_MS || nFlushCostMs >= RTMP_WRITE_BLOCK_WARN_MS)
    {
        dlog_warn("RTMP视频写包耗时偏高，通道=%d, len=%d, pkt_size=%d, key=%d, write_cost=%lldms, flush_cost=%lldms, video_send=%llu",
                  m_nChannel,
                  nLen,
                  pkt.size,
                  bIsKeyFrame ? 1 : 0,
                  static_cast<long long>(nWriteCostMs),
                  static_cast<long long>(nFlushCostMs),
                  static_cast<unsigned long long>(uSendCount));
    }
    return OK;
}

int CRtmpSession::process_audio_frame(std::unique_ptr<FrameData> pFrameData)
{
    if (!pFrameData || pFrameData->frameSize <= 0)
    {
        return ERR;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected.load() || !m_stream_context.is_open())
    {
        return ERR;
    }

    if (m_bAudioDisabled)
    {
        return OK;
    }

    /* 获取帧数据指针和长度 */
    uint8_t* pData = pFrameData->data.get();
    int nLen = pFrameData->frameSize;

    RtmpAudio_NS::AacAdtsInfo_S stAdtsInfo;
    bool bIsAdts = RtmpAudio_NS::parse_adts_header(pData, nLen, stAdtsInfo);

    if (!bIsAdts)
    {
        int nSampleRate = static_cast<int>(m_stAudioConfig.enSampRate);
        int nSampleRateIndex = RtmpAudio_NS::find_sample_rate_index(nSampleRate);
        if (nSampleRateIndex < 0)
        {
            dlog_error("AudioConfig采样率不支持RTMP：%d，通道=%d", nSampleRate, m_nChannel);
            return ERR;
        }

        stAdtsInfo.profile = 1;
        stAdtsInfo.object_type = 2;
        stAdtsInfo.sample_rate_index = nSampleRateIndex;
        stAdtsInfo.sample_rate = nSampleRate;
        stAdtsInfo.channels = 1;
        stAdtsInfo.header_size = 0;
    }

    if (!m_bAudioReady)
    {
        /* 构造临时AudioFrame_S用于初始化 */
        /* 注意：AudioFrame_S使用柔性数组，需要动态分配 */
        int nFrameSize = sizeof(Audio_NS::AudioFrame_S) + nLen;
        Audio_NS::AudioFrame_S* pAudioFrame = reinterpret_cast<Audio_NS::AudioFrame_S*>(new uint8_t[nFrameSize]);
        pAudioFrame->nLen = nLen;
        memcpy(pAudioFrame->pData, pData, nLen);

        /* 初始化失败时静默丢帧，等待后续音频帧重试 */
        int nRet = init_audio_stream(pAudioFrame, stAdtsInfo);
        delete[] reinterpret_cast<uint8_t*>(pAudioFrame);
        if (nRet < 0)
        {
            return OK;
        }
        m_bAudioReady = true;
        m_nAudioFrameDurationMs = calc_aac_frame_duration_ms(stAdtsInfo.sample_rate);
    }

    if (try_write_header() < 0 || !m_bHeaderWritten)
    {
        return OK;
    }

    if (bIsAdts && nLen <= stAdtsInfo.header_size)
    {
        dlog_error("AAC帧长度小于ADTS头长度，通道=%d", m_nChannel);
        return ERR;
    }

    AVPacket pkt = { 0 };
    pkt.data = pData + stAdtsInfo.header_size;
    pkt.size = nLen - stAdtsInfo.header_size;
    pkt.stream_index = m_stream_context.audio_stream_index();
    pkt.pts = m_nAudioPts;
    pkt.dts = m_nAudioPts;
    m_nAudioPts += m_nAudioFrameDurationMs;

    AVRational stTimeBase = { 1, 1000 };
    AVRational stStreamTimeBase = m_stream_context.audio_time_base();
    pkt.pts = av_rescale_q(pkt.pts, stTimeBase, stStreamTimeBase);
    pkt.dts = av_rescale_q(pkt.dts, stTimeBase, stStreamTimeBase);
    pkt.duration = av_rescale_q(m_nAudioFrameDurationMs, stTimeBase, stStreamTimeBase);

    const int64_t nWriteStartMs = rtmp_now_ms();
    int nRet = m_stream_context.write_frame(&pkt);
    const int64_t nWriteCostMs = rtmp_now_ms() - nWriteStartMs;
    if (nRet < 0)
    {
        char errBuf[256] = { 0 };
        av_strerror(nRet, errBuf, sizeof(errBuf));
        dlog_error("写入音频帧失败: %s", errBuf);
        return ERR;
    }

    const int64_t nFlushStartMs = rtmp_now_ms();
    m_stream_context.flush();
    const int64_t nFlushCostMs = rtmp_now_ms() - nFlushStartMs;
    const uint64_t uSendCount = m_uAudioSendCount.fetch_add(1) + 1;
    if (nWriteCostMs >= RTMP_WRITE_BLOCK_WARN_MS || nFlushCostMs >= RTMP_WRITE_BLOCK_WARN_MS)
    {
        dlog_warn("RTMP音频写包耗时偏高，通道=%d, len=%d, pkt_size=%d, write_cost=%lldms, flush_cost=%lldms, audio_send=%llu",
                  m_nChannel,
                  nLen,
                  pkt.size,
                  static_cast<long long>(nWriteCostMs),
                  static_cast<long long>(nFlushCostMs),
                  static_cast<unsigned long long>(uSendCount));
    }
    return OK;
}
