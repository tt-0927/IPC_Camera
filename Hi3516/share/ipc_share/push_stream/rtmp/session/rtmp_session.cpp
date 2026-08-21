/**
 * @FilePath     : rtmp_session.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 16:00:23
 * @Description  : RTMP单路推流会话实现
 */

#include "rtmp_session.h"

#include <algorithm>
#include <cstring>

#include "aac_extradata_builder.h"
#include "flv_packet_converter.h"
#include "annexb_nal_parser.h"
#include "video_extradata_builder.h"

namespace
{
/* 音频开启但首帧异常时，最多等待该时长后降级为纯视频RTMP，避免视频长期无法出流 */
const int64_t AUDIO_WAIT_TIMEOUT_MS = 3000;
/* 发送循环条件等待的超时兜底间隔，避免无帧到达时线程永久阻塞 */
const int64_t RTMP_SEND_LOOP_WAKE_INTERVAL_MS = 5000;
/* 单次写包或flush超过该阈值时认为可能存在阻塞 */
const int64_t RTMP_WRITE_BLOCK_WARN_MS = 300;
/* 低延迟输出缓冲窗口：降低逐包flush系统调用，同时将额外延迟限制在该范围内 */
const int64_t RTMP_FLUSH_INTERVAL_MS = 40;

int64_t rtmp_now_ms()
{
    return av_gettime_relative() / 1000;
}

/**
 * @brief 计算视频帧时长
 * @param stVideoConfig 视频配置
 * @return 帧时长，单位毫秒
 */
int64_t calc_video_frame_duration_ms(const Video_NS::VideoConfig_S &stVideoConfig)
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
                           const std::string &strUrl,
                           const Video_NS::VideoConfig_S &stVideoConfig,
                           const Audio_NS::AudioConfig_S &stAudioConfig)
    : m_nChannel(nChannel), m_strUrl(strUrl), m_stVideoConfig(stVideoConfig), m_stAudioConfig(stAudioConfig),
      m_nVideoFrameDurationMs(calc_video_frame_duration_ms(stVideoConfig))
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

    dlog_info("RTMP会话准备连接，通道=%d, URL=%s, codec=%d, width=%d, height=%d, fps=%d, audio_switch=%d, audio_format=%d, "
              "sample_rate=%d",
              m_nChannel,
              m_strUrl.c_str(),
              static_cast<int>(m_stVideoConfig.enVideoCodec),
              m_stVideoConfig.stVideoResolution.nWidth,
              m_stVideoConfig.stVideoResolution.nHeight,
              m_stVideoConfig.getFrameRateAsInt(),
              m_stAudioConfig.bAudioSwitch ? 1 : 0,
              static_cast<int>(m_stAudioConfig.enFormat),
              static_cast<int>(m_stAudioConfig.enSampRate));

    m_stream_context.reset_interrupt();
    int nRet = m_stream_context.open(m_strUrl);
    if (nRet < 0)
    {
        return nRet;
    }

    m_bVideoReady = false;
    m_bAudioReady = false;
    m_bAudioDisabled.store(false);
    m_bHeaderWritten = false;
    m_bNeedVideoKeyFrame = true;
    m_nVideoPts = 0;
    m_nAudioPts = 0;
    m_nAudioWaitStartMs = 0;
    m_nLastFlushMs = 0;
    m_vH265Mp4Data.clear();

    /* 初始化帧队列（在锁保护下） */
    {
        std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
        m_videoQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_VIDEO_FRAME);
        m_audioQueue = std::make_unique<CThreadSafeFrameQueue>(MAX_AUDIO_FRAME);
    }

    /* 队列准备完毕后再发布连接状态，避免媒体线程看到已连接但队列尚未创建。 */
    m_bStopSend.store(false);
    m_bConnected.store(true);
    m_sendThread = std::thread(&CRtmpSession::send_loop, this);

    dlog_info("RTMP会话连接成功，通道=%d, URL=%s", m_nChannel, m_strUrl.c_str());

    return OK;
}

void CRtmpSession::deinit()
{
    /* ! 先拒绝新的媒体帧，再终止阻塞IO；避免关闭期间继续向将被销毁的队列入队。 */
    m_bConnected.store(false);
    m_bStopSend.store(true);
    m_stream_context.request_interrupt();

    /* 停止队列等待，使发送线程快速退出。 */
    {
        std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
        if (m_videoQueue)
            m_videoQueue->stop();
        if (m_audioQueue)
            m_audioQueue->stop();
    }
    m_cvQueue.notify_all();

    /*
     * ! 禁止detach：线程仍可能访问m_stream_context和队列，detach后析构会产生UAF。
     * 中断回调已收到退出请求，因此此处必须join以保证资源生命周期完整。
     */
    if (m_sendThread.joinable())
    {
        m_sendThread.join();
    }

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
    m_bVideoReady = false;
    m_bAudioReady = false;
    m_bAudioDisabled.store(false);
    m_bHeaderWritten = false;
    m_bNeedVideoKeyFrame = true;
    m_nVideoPts = 0;
    m_nAudioPts = 0;
    m_nAudioWaitStartMs = 0;
    m_nLastFlushMs = 0;
}

bool CRtmpSession::is_connected() const
{
    return m_bConnected.load();
}

int CRtmpSession::send_video_frame(Video_NS::VideoFrame_S *pVideoFrame)
{
    if (!pVideoFrame)
    {
        dlog_error("视频帧数据无效");
        return ERR;
    }

    return send_video_frame(pVideoFrame->pData, pVideoFrame->nLen, pVideoFrame->eType);
}

int CRtmpSession::send_video_frame(const uint8_t *pData,
                                   int nDataLen,
                                   Video_NS::NalType_E eType)
{
    if (!pData || nDataLen <= 0)
    {
        dlog_error("视频帧数据无效");
        return ERR;
    }

    /* 检查连接状态（原子变量，无需锁） */
    if (!m_bConnected.load())
    {
        return ERR;
    }

    const bool bIsMarkedKeyFrame = eType == Video_NS::H264_TYPE_IDR ||
                                   eType == Video_NS::H265_TYPE_IDR_W_RADL ||
                                   eType == Video_NS::H265_TYPE_IDR_N_LP ||
                                   eType == Video_NS::H265_TYPE_CRA;

    /*
     * perf: 队列容量确认后才申请并复制完整编码帧。弱网满队列时直接丢帧，
     * 禁止发生“new[] + memcpy后才发现队列已满”的无效CPU和内存开销。
     */
    std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
    if (!m_videoQueue)
    {
        return OK;
    }
    if (m_videoQueue->isFull())
    {
        if (!bIsMarkedKeyFrame)
        {
            return OK;
        }

        /* 低延迟策略：关键帧到达时淘汰积压的旧视频，确保重连后尽快恢复可解码画面。 */
        m_videoQueue->clear();
    }

    std::unique_ptr<FrameData> pFrameData(new FrameData());
    /* memory: new[] + shared_ptr 显式删除器，绕开 make_shared 数组缺陷（同 RTSP 路径） */
    pFrameData->data = std::shared_ptr<unsigned char[]>(
        new unsigned char[nDataLen],
        std::default_delete<unsigned char[]>());
    pFrameData->type = FRAME_TYPE_VIDEO;
    pFrameData->frameSize = nDataLen;
    /*
     * perf: 编码器未标注NAL类型时延后到发送线程解析，队列满时无需扫描整帧。
    * iFrame=-1 表示未知，1表示明确关键帧，0表示明确非关键帧。
     */
    pFrameData->iFrame = bIsMarkedKeyFrame ? 1 : -1;
    /* memory: 仅在RTMP有界队列入队前复制一次，不保存VENC原始指针。 */
    std::memcpy(pFrameData->data.get(), pData, nDataLen);
    m_videoQueue->push(std::move(pFrameData));
    m_cvQueue.notify_one();

    return OK;
}

int CRtmpSession::send_video_frame(const Video_NS::SharedMediaFrame_S &stSharedFrame, Video_NS::NalType_E eType)
{
    if (!stSharedFrame.pData || stSharedFrame.nLen <= 0)
    {
        dlog_error("视频帧数据无效");
        return ERR;
    }

    /* 检查连接状态（原子变量，无需锁） */
    if (!m_bConnected.load())
    {
        return ERR;
    }

    const bool bIsMarkedKeyFrame = eType == Video_NS::H264_TYPE_IDR || eType == Video_NS::H265_TYPE_IDR_W_RADL ||
                                   eType == Video_NS::H265_TYPE_IDR_N_LP || eType == Video_NS::H265_TYPE_CRA;

    /*
     * perf: 队列容量确认后才增加引用计数，弱网满队列时直接丢帧，
     * 避免无效的 shared_ptr 引用开销。
     */
    std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
    if (!m_videoQueue)
    {
        return OK;
    }
    if (m_videoQueue->isFull())
    {
        if (!bIsMarkedKeyFrame)
        {
            return OK;
        }

        /* 低延迟策略：关键帧到达时淘汰积压的旧视频，确保重连后尽快恢复可解码画面。 */
        m_videoQueue->clear();
    }

    std::unique_ptr<FrameData> pFrameData(new FrameData());
    /* memory: 直接持有共享帧引用（零拷贝，引用计数+1），与 RTSP/录制共享同一份 buffer。 */
    pFrameData->data = stSharedFrame.pData;
    pFrameData->type = FRAME_TYPE_VIDEO;
    pFrameData->frameSize = stSharedFrame.nLen;
    pFrameData->iFrame = bIsMarkedKeyFrame ? 1 : -1;
    m_videoQueue->push(std::move(pFrameData));
    m_cvQueue.notify_one();

    return OK;
}

int CRtmpSession::send_audio_frame(Audio_NS::AudioFrame_S *pAudioFrame)
{
    if (!pAudioFrame || !pAudioFrame->pData || pAudioFrame->nLen <= 0)
    {
        dlog_error("音频帧数据无效");
        return ERR_PARAM;
    }

    /* 检查连接状态（原子变量，无需锁） */
    if (!m_bConnected.load())
    {
        return ERR;
    }

    if (m_bAudioDisabled.load())
    {
        return OK;
    }

    /* perf: 与视频相同，音频队列满时不分配、不复制，避免背压反向耗尽采集CPU。 */
    std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
    if (!m_audioQueue || m_audioQueue->isFull())
    {
        return OK;
    }

    std::unique_ptr<FrameData> pFrameData(new FrameData());
    /* memory: new[] + shared_ptr 显式删除器，绕开 make_shared 数组缺陷（同视频路径） */
    pFrameData->data = std::shared_ptr<unsigned char[]>(
        new unsigned char[pAudioFrame->nLen],
        std::default_delete<unsigned char[]>());
    pFrameData->type = FRAME_TYPE_AUDIO;
    pFrameData->frameSize = pAudioFrame->nLen;
    std::memcpy(pFrameData->data.get(), pAudioFrame->pData, pAudioFrame->nLen);
    m_audioQueue->push(std::move(pFrameData));
    m_cvQueue.notify_one();

    return OK;
}

int CRtmpSession::init_video_stream(const uint8_t *pData, int nLen, Video_NS::VideoCodec_E enVideoCodec)
{
    RtmpVideo_NS::VideoParameterSets_S stSets;
    std::vector<uint8_t> vExtradata;
    if (enVideoCodec == Video_NS::VideoCodec_E::H264)
    {
        if (!RtmpVideo_NS::extract_h264_parameter_sets(pData, nLen, stSets) ||
            !RtmpVideo_NS::build_avc_extradata(stSets, vExtradata))
        {
            return ERR;
        }
    }
    else if (enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        if (!RtmpVideo_NS::extract_h265_parameter_sets(pData, nLen, stSets) ||
            !RtmpVideo_NS::build_hevc_extradata(stSets, vExtradata))
        {
            return ERR;
        }
    }
    else
    {
        dlog_error("RTMP不支持当前视频编码，通道=%d, codec=%d", m_nChannel, static_cast<int>(enVideoCodec));
        return ERR;
    }

    int nFrameRate = m_stVideoConfig.getFrameRateAsInt();
    if (nFrameRate <= 0 || nFrameRate > 0xffff)
    {
        nFrameRate = 25;
    }

    int nRet = m_stream_context.create_video_stream(enVideoCodec,
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
              static_cast<int>(enVideoCodec),
              m_stVideoConfig.stVideoResolution.nWidth,
              m_stVideoConfig.stVideoResolution.nHeight,
              nFrameRate,
              nLen,
              static_cast<int>(vExtradata.size()));
    return OK;
}

int CRtmpSession::init_audio_stream(int nFrameLen, const RtmpAudio_NS::AacAdtsInfo_S &stAdtsInfo)
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
              nFrameLen,
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
            return OK;
        }

        /* 音频链路异常不能阻塞视频出流，超时后固定为纯视频会话。 */
        m_bAudioDisabled.store(true);
        dlog_warn("RTMP等待音频超时，降级为纯视频推流，通道=%d, timeout=%lldms",
                  m_nChannel,
                  static_cast<long long>(AUDIO_WAIT_TIMEOUT_MS));
    }
    if (!m_bAudioReady && !need_audio_stream())
    {
        /* 未配置 AAC 或已降级时，header 写出后不允许再追加音频流。 */
        m_bAudioDisabled.store(true);
    }

    int nRet = m_stream_context.write_header(m_stVideoConfig.enVideoCodec, m_nChannel);
    if (nRet < 0)
    {
        m_bConnected.store(false);
        return nRet;
    }

    m_bHeaderWritten = true;
    m_bNeedVideoKeyFrame = true;
    m_nLastFlushMs = rtmp_now_ms();
    return OK;
}

bool CRtmpSession::need_audio_stream() const
{
    return !m_bAudioDisabled.load() && m_stAudioConfig.bAudioSwitch && m_stAudioConfig.enFormat == Audio_NS::AudioFormat_E::AAC;
}

int64_t CRtmpSession::flush_if_due(bool bForceFlush)
{
    const int64_t nNowMs = rtmp_now_ms();
    if (!bForceFlush && m_nLastFlushMs > 0 && nNowMs - m_nLastFlushMs < RTMP_FLUSH_INTERVAL_MS)
    {
        return 0;
    }

    const int64_t nFlushStartMs = rtmp_now_ms();
    m_stream_context.flush();
    const int64_t nFlushCostMs = rtmp_now_ms() - nFlushStartMs;
    m_nLastFlushMs = nNowMs;
    return nFlushCostMs;
}

void CRtmpSession::send_loop()
{
    pthread_setname_np(pthread_self(), "rtmp_send");
    m_bSendThreadRunning.store(true);
    dlog_info("RTMP发送线程启动，通道=%d", m_nChannel);

    while (!m_bStopSend.load())
    {
        std::unique_ptr<FrameData> videoFrame;
        std::unique_ptr<FrameData> audioFrame;
        {
            /*
             * perf: 视频、音频共用一个到达事件，避免原来的5ms空队列轮询；
             * 有任一媒体帧到达或会话停止时才唤醒，超时仅作兜底。
             */
            std::unique_lock<std::mutex> lockQueue(m_mutexQueue);
            m_cvQueue.wait_for(lockQueue,
                               std::chrono::milliseconds(RTMP_SEND_LOOP_WAKE_INTERVAL_MS),
                               [this]()
                               {
                                   return m_bStopSend.load() ||
                                          (m_bConnected.load() && ((m_videoQueue && !m_videoQueue->empty()) ||
                                                                   (m_audioQueue && !m_audioQueue->empty())));
                               });

            if (m_bStopSend.load())
            {
                break;
            }
            if (!m_bConnected.load())
            {
                continue;
            }
            if (m_videoQueue)
            {
                videoFrame = m_videoQueue->pop();
            }
            if (m_audioQueue)
            {
                audioFrame = m_audioQueue->pop();
            }
        }

        if (videoFrame)
        {
            int nRet = process_video_frame(std::move(videoFrame));
            if (nRet < 0)
            {
                dlog_error("RTMP视频帧发送失败，通道=%d", m_nChannel);
                m_bConnected.store(false);
                std::lock_guard<std::mutex> lockQueue(m_mutexQueue);
                if (m_videoQueue)
                    m_videoQueue->clear();
                if (m_audioQueue)
                    m_audioQueue->clear();
                continue;
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
                if (m_videoQueue)
                    m_videoQueue->clear();
                if (m_audioQueue)
                    m_audioQueue->clear();
                continue;
            }
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
    uint8_t *pData = pFrameData->data.get();
    int nLen = pFrameData->frameSize;
    const bool bIsKeyFrame = pFrameData->iFrame == 1 ||
                             (pFrameData->iFrame < 0 &&
                              RtmpVideo_NS::has_key_frame_nal(pData, nLen, m_stVideoConfig.enVideoCodec));
    Video_NS::VideoCodec_E enVideoCodec = m_stVideoConfig.enVideoCodec;

    if (!m_bVideoReady)
    {
        /* perf: 参数集解析直接复用队列帧，避免首帧额外申请柔性结构体并复制整帧。 */
        int nRet = init_video_stream(pData, nLen, enVideoCodec);
        if (nRet < 0)
        {
            return OK;
        }
        m_bVideoReady = true;
    }

    const int nHeaderRet = try_write_header();
    if (nHeaderRet < 0)
    {
        return nHeaderRet;
    }
    if (!m_bHeaderWritten)
    {
        return OK;
    }

    if (m_bNeedVideoKeyFrame && !bIsKeyFrame)
    {
        return OK;
    }
    if (m_bNeedVideoKeyFrame && bIsKeyFrame)
    {
        m_bNeedVideoKeyFrame = false;
        dlog_info("RTMP收到关键帧，开始发送视频，通道=%d", m_nChannel);
    }

    if (enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        if (!RtmpFlv_NS::annexb_to_mp4(pData, nLen, m_vH265Mp4Data))
        {
            dlog_error("H.265视频帧AnnexB转MP4失败，通道=%d", m_nChannel);
            return ERR;
        }
    }

    AVPacket pkt = { 0 };
    if (enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        pkt.data = m_vH265Mp4Data.data();
        pkt.size = static_cast<int>(m_vH265Mp4Data.size());
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

    const int64_t nFlushCostMs = flush_if_due(bIsKeyFrame);
    if (nWriteCostMs >= RTMP_WRITE_BLOCK_WARN_MS || nFlushCostMs >= RTMP_WRITE_BLOCK_WARN_MS)
    {
        dlog_warn(
            "RTMP视频写包耗时偏高，通道=%d, len=%d, pkt_size=%d, key=%d, write_cost=%lldms, flush_cost=%lldms",
            m_nChannel,
            nLen,
            pkt.size,
            bIsKeyFrame ? 1 : 0,
            static_cast<long long>(nWriteCostMs),
            static_cast<long long>(nFlushCostMs));
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

    if (m_bAudioDisabled.load())
    {
        return OK;
    }

    /* 获取帧数据指针和长度 */
    uint8_t *pData = pFrameData->data.get();
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
        /* perf: 音频流初始化只依赖ADTS信息和帧长，无需再复制柔性数组帧。 */
        int nRet = init_audio_stream(nLen, stAdtsInfo);
        if (nRet < 0)
        {
            return OK;
        }
        m_bAudioReady = true;
        m_nAudioFrameDurationMs = calc_aac_frame_duration_ms(stAdtsInfo.sample_rate);
    }

    const int nHeaderRet = try_write_header();
    if (nHeaderRet < 0)
    {
        return nHeaderRet;
    }
    if (!m_bHeaderWritten)
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

    const int64_t nFlushCostMs = flush_if_due(false);
    if (nWriteCostMs >= RTMP_WRITE_BLOCK_WARN_MS || nFlushCostMs >= RTMP_WRITE_BLOCK_WARN_MS)
    {
        dlog_warn("RTMP音频写包耗时偏高，通道=%d, len=%d, pkt_size=%d, write_cost=%lldms, flush_cost=%lldms",
                  m_nChannel,
                  nLen,
                  pkt.size,
                  static_cast<long long>(nWriteCostMs),
                  static_cast<long long>(nFlushCostMs));
    }
    return OK;
}
