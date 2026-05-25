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
    dlog_info("RTMP会话连接成功，通道=%d, URL=%s", m_nChannel, m_strUrl.c_str());

    return OK;
}

void CRtmpSession::deinit()
{
    std::lock_guard<std::mutex> lock(m_mutex);

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

    /* 逐帧日志默认关闭，当前日志等级会打印 DEBUG，频繁输出会影响实时推流性能。 */
    /* dlog_debug("RTMP收到视频帧，通道=%d, 长度=%d, 类型=%d, 编码=%d",
                 m_nChannel, pVideoFrame->nLen,
                 static_cast<int>(pVideoFrame->eType),
                 static_cast<int>(pVideoFrame->enVideoCodec)); */

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected.load() || !m_stream_context.is_open())
    {
        /* 未连接时上层会触发重连，逐帧丢弃日志默认关闭，避免断线期间刷屏。 */
        /* dlog_debug("RTMP视频帧丢弃，未连接，通道=%d", m_nChannel); */
        return ERR;
    }

    if (!m_bVideoReady)
    {
        /* 首个可用关键帧通常携带 SPS/PPS 或 VPS/SPS/PPS，需先提取参数集后才能写 FLV header。 */
        int nRet = init_video_stream(pVideoFrame);
        if (nRet < 0)
        {
            return OK;
        }
        m_bVideoReady = true;
    }

    if (try_write_header() < 0 || !m_bHeaderWritten)
    {
        /* 参数集或音频参数未就绪时会暂丢视频帧，该路径为启动阶段常态，默认不打印逐帧日志。 */
        /* dlog_debug("RTMP视频帧丢弃，Header未就绪，通道=%d", m_nChannel); */
        return OK;
    }

    const bool bIsKeyFrame = is_key_frame(pVideoFrame);
    /* 关键帧判断属于逐帧日志，排查关键帧识别问题时再临时打开。 */
    /* dlog_debug("RTMP视频帧判断，通道=%d, 类型=%d, 是否关键帧=%d, needKeyFrame=%d",
                 m_nChannel, static_cast<int>(pVideoFrame->eType),
                 bIsKeyFrame ? 1 : 0, m_bNeedVideoKeyFrame ? 1 : 0); */

    if (m_bNeedVideoKeyFrame && !bIsKeyFrame)
    {
        /* Header 写出后必须从关键帧开始推送，避免播放器拿到不可解码的 P/B 帧。 */
        /* dlog_debug("RTMP视频帧丢弃，等待关键帧，通道=%d", m_nChannel); */
        return OK;
    }
    if (m_bNeedVideoKeyFrame && bIsKeyFrame)
    {
        m_bNeedVideoKeyFrame = false;
        dlog_info("RTMP收到关键帧，开始发送视频，通道=%d", m_nChannel);
    }

    std::vector<uint8_t> vMp4Data;
    if (pVideoFrame->enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        /* FFmpeg 的 FLV HEVC 写包逻辑期望长度前缀 NALU，这里将编码器输出的 AnnexB 起始码转为 MP4 格式。 */
        if (!RtmpFlv_NS::annexb_to_mp4(pVideoFrame->pData, pVideoFrame->nLen, vMp4Data))
        {
            dlog_error("H.265视频帧AnnexB转MP4失败，通道=%d", m_nChannel);
            return ERR;
        }
    }

    AVPacket pkt = { 0 };
    if (pVideoFrame->enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        pkt.data = vMp4Data.data();
        pkt.size = static_cast<int>(vMp4Data.size());
    }
    else
    {
        pkt.data = pVideoFrame->pData;
        pkt.size = pVideoFrame->nLen;
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

    int nRet = m_stream_context.write_frame(&pkt);
    if (nRet < 0)
    {
        char errBuf[256] = { 0 };
        av_strerror(nRet, errBuf, sizeof(errBuf));
        dlog_error("写入视频帧失败: %s", errBuf);
        m_bConnected.store(false);
        return ERR;
    }

    m_stream_context.flush();
    return OK;
}

int CRtmpSession::send_audio_frame(Audio_NS::AudioFrame_S* pAudioFrame)
{
    if (!pAudioFrame || pAudioFrame->nLen <= 0)
    {
        dlog_error("音频帧数据无效");
        return ERR_PARAM;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bConnected.load() || !m_stream_context.is_open())
    {
        return ERR;
    }

    if (m_bAudioDisabled)
    {
        /* 纯视频会话不再动态追加音频流，避免 FLV header 已写出后流结构变化。 */
        return OK;
    }

    RtmpAudio_NS::AacAdtsInfo_S stAdtsInfo;
    bool bIsAdts = RtmpAudio_NS::parse_adts_header(pAudioFrame->pData, pAudioFrame->nLen, stAdtsInfo);

    if (!bIsAdts)
    {
        /* 未检测到 ADTS 头时按裸 AAC 处理，依赖 AudioConfig 补齐采样率等 FLV header 必要参数。 */
        int nSampleRate = static_cast<int>(m_stAudioConfig.enSampRate);
        int nSampleRateIndex = RtmpAudio_NS::find_sample_rate_index(nSampleRate);
        if (nSampleRateIndex < 0)
        {
            dlog_error("AudioConfig采样率不支持RTMP：%d，通道=%d", nSampleRate, m_nChannel);
            return ERR;
        }

        /* 裸 AAC 默认使用 AAC-LC (profile=1, object_type=2) */
        stAdtsInfo.profile = 1;
        stAdtsInfo.object_type = 2;
        stAdtsInfo.sample_rate_index = nSampleRateIndex;
        stAdtsInfo.sample_rate = nSampleRate;
        stAdtsInfo.channels = 1;
        stAdtsInfo.header_size = 0;
    }

    if (!m_bAudioReady)
    {
        /* 音频流只在首个有效音频帧到达时创建，避免裸 AAC 缺少参数导致 header 写错。 */
        int nRet = init_audio_stream(pAudioFrame, stAdtsInfo);
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

    if (bIsAdts && pAudioFrame->nLen <= stAdtsInfo.header_size)
    {
        dlog_error("AAC帧长度小于ADTS头长度，通道=%d", m_nChannel);
        return ERR;
    }

    AVPacket pkt = { 0 };
    pkt.data = pAudioFrame->pData + stAdtsInfo.header_size;
    pkt.size = pAudioFrame->nLen - stAdtsInfo.header_size;
    pkt.stream_index = m_stream_context.audio_stream_index();
    pkt.pts = m_nAudioPts;
    pkt.dts = m_nAudioPts;
    m_nAudioPts += m_nAudioFrameDurationMs;

    AVRational stTimeBase = { 1, 1000 };
    AVRational stStreamTimeBase = m_stream_context.audio_time_base();
    pkt.pts = av_rescale_q(pkt.pts, stTimeBase, stStreamTimeBase);
    pkt.dts = av_rescale_q(pkt.dts, stTimeBase, stStreamTimeBase);
    pkt.duration = av_rescale_q(m_nAudioFrameDurationMs, stTimeBase, stStreamTimeBase);

    int nRet = m_stream_context.write_frame(&pkt);
    if (nRet < 0)
    {
        char errBuf[256] = { 0 };
        av_strerror(nRet, errBuf, sizeof(errBuf));
        dlog_error("写入音频帧失败: %s", errBuf);
        m_bConnected.store(false);
        return ERR;
    }

    m_stream_context.flush();
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
            dlog_warn("等待H.264 SPS/PPS参数集，通道=%d", m_nChannel);
            return ERR;
        }
    }
    else if (pVideoFrame->enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        if (!RtmpVideo_NS::extract_h265_parameter_sets(pVideoFrame->pData, pVideoFrame->nLen, stSets) ||
            !RtmpVideo_NS::build_hevc_extradata(stSets, vExtradata))
        {
            dlog_warn("等待H.265 VPS/SPS/PPS参数集，通道=%d", m_nChannel);
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
    dlog_info("RTMP视频流初始化成功，通道=%d, codec=%d, width=%d, height=%d, fps=%d",
              m_nChannel,
              static_cast<int>(pVideoFrame->enVideoCodec),
              m_stVideoConfig.stVideoResolution.nWidth,
              m_stVideoConfig.stVideoResolution.nHeight,
              nFrameRate);
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
    dlog_info("RTMP音频流初始化成功，通道=%d, sample_rate=%d, channels=%d, frame_len=%d",
              m_nChannel,
              stAdtsInfo.sample_rate,
              stAdtsInfo.channels,
              pAudioFrame ? pAudioFrame->nLen : 0);
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
