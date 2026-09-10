/**
 * @FilePath     : rtmp_stream_context.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-29 09:10:43
 * @Description  : RTMP FFmpeg流上下文封装实现
 */

#include "rtmp_stream_context.h"

#include <cstring>

#include "dlog.h"
#include "IpcRet.h"

namespace
{
/* IO级诊断阈值：连接、写Header或关闭超过该值时输出warn */
const int64_t RTMP_IO_WARN_MS = 300;

int64_t rtmp_io_now_ms()
{
    return av_gettime_relative() / 1000;
}
}

/**
 * @brief FFmpeg IO中断回调
 * @param pCtx 上下文指针（CRtmpStreamContext实例）
 * @return 0：继续，1：中断
 */
static int interrupt_callback(void *pCtx)
{
    CRtmpStreamContext *pContext = static_cast<CRtmpStreamContext *>(pCtx);
    if (!pContext)
    {
        return 0;
    }

    if (pContext->is_interrupt_requested())
    {
        return 1;
    }

    int64_t currentTime = av_gettime_relative() / 1000000; // 转换为秒

    /* 检查是否超时 */
    if (pContext->m_nLastIoTimeSec > 0 && currentTime - pContext->m_nLastIoTimeSec > pContext->get_timeout_sec())
    {
        dlog_error("RTMP IO超时，超时时间=%d秒", pContext->get_timeout_sec());
        return 1; // 中断
    }

    return 0; // 继续
}

CRtmpStreamContext::CRtmpStreamContext()
{
}

CRtmpStreamContext::~CRtmpStreamContext()
{
    close(false);
}

int CRtmpStreamContext::open(const std::string &strUrl)
{
    if (m_pFormatCtx)
    {
        return OK;
    }

    reset_interrupt();
    const int64_t nOpenStartMs = rtmp_io_now_ms();
    dlog_info("RTMP开始打开FFmpeg输出上下文，URL=%s", strUrl.c_str());

    int nRet = avformat_alloc_output_context2(&m_pFormatCtx, nullptr, "flv", strUrl.c_str());
    if (nRet < 0 || !m_pFormatCtx)
    {
        char errBuf[256] = { 0 };
        av_strerror(nRet, errBuf, sizeof(errBuf));
        dlog_error("创建FLV输出上下文失败: %s", errBuf);
        return ERR;
    }

    /* 设置超时回调 */
    m_pFormatCtx->interrupt_callback.callback = interrupt_callback;
    m_pFormatCtx->interrupt_callback.opaque = this;

    /* 初始化IO时间 */
    m_nLastIoTimeSec = av_gettime_relative() / 1000000;

    if (!(m_pFormatCtx->oformat->flags & AVFMT_NOFILE))
    {
        /* 使用 avio_open2 并传入中断回调，保护连接阶段 */
        AVIOInterruptCB intCb = { interrupt_callback, this };
        const int64_t nIoOpenStartMs = rtmp_io_now_ms();
        nRet = avio_open2(&m_pFormatCtx->pb, strUrl.c_str(), AVIO_FLAG_WRITE, &intCb, nullptr);
        const int64_t nIoOpenCostMs = rtmp_io_now_ms() - nIoOpenStartMs;
        if (nRet < 0)
        {
            char errBuf[256] = { 0 };
            av_strerror(nRet, errBuf, sizeof(errBuf));
            dlog_error("打开RTMP连接失败: %s, URL=%s, cost=%lldms",
                       errBuf,
                       strUrl.c_str(),
                       static_cast<long long>(nIoOpenCostMs));
            avformat_free_context(m_pFormatCtx);
            m_pFormatCtx = nullptr;
            return ERR;
        }
        if (nIoOpenCostMs >= RTMP_IO_WARN_MS)
        {
            dlog_warn("打开RTMP连接耗时偏高，URL=%s, cost=%lldms", strUrl.c_str(), static_cast<long long>(nIoOpenCostMs));
        }
    }

    const int64_t nOpenCostMs = rtmp_io_now_ms() - nOpenStartMs;
    dlog_info("RTMP FFmpeg输出上下文打开成功，URL=%s, cost=%lldms", strUrl.c_str(), static_cast<long long>(nOpenCostMs));
    return OK;
}

void CRtmpStreamContext::close(bool bHeaderWritten)
{
    if (!m_pFormatCtx)
    {
        return;
    }

    const int64_t nCloseStartMs = rtmp_io_now_ms();
    if (bHeaderWritten)
    {
        const int64_t nTrailerStartMs = rtmp_io_now_ms();
        av_write_trailer(m_pFormatCtx);
        const int64_t nTrailerCostMs = rtmp_io_now_ms() - nTrailerStartMs;
        if (nTrailerCostMs >= RTMP_IO_WARN_MS)
        {
            dlog_warn("RTMP写trailer耗时偏高，cost=%lldms", static_cast<long long>(nTrailerCostMs));
        }
    }

    if (!(m_pFormatCtx->oformat->flags & AVFMT_NOFILE) && m_pFormatCtx->pb)
    {
        const int64_t nCloseIoStartMs = rtmp_io_now_ms();
        avio_closep(&m_pFormatCtx->pb);
        const int64_t nCloseIoCostMs = rtmp_io_now_ms() - nCloseIoStartMs;
        if (nCloseIoCostMs >= RTMP_IO_WARN_MS)
        {
            dlog_warn("RTMP关闭IO耗时偏高，cost=%lldms", static_cast<long long>(nCloseIoCostMs));
        }
    }

    avformat_free_context(m_pFormatCtx);
    m_pFormatCtx = nullptr;
    m_nVideoStreamIndex = -1;
    m_nAudioStreamIndex = -1;
    dlog_info("RTMP FFmpeg输出上下文已关闭，header_written=%d, cost=%lldms",
              bHeaderWritten ? 1 : 0,
              static_cast<long long>(rtmp_io_now_ms() - nCloseStartMs));
}

int CRtmpStreamContext::create_video_stream(Video_NS::VideoCodec_E enCodec,
                                            int nWidth,
                                            int nHeight,
                                            int nFrameRate,
                                            const std::vector<uint8_t> &vExtradata,
                                            int nChannel)
{
    if (!m_pFormatCtx)
    {
        return ERR;
    }

    AVStream *pStream = avformat_new_stream(m_pFormatCtx, nullptr);
    if (!pStream)
    {
        dlog_error("创建视频流失败，通道=%d", nChannel);
        return ERR;
    }

    m_nVideoStreamIndex = pStream->index;
    pStream->time_base = { 1, 1000 };
    pStream->avg_frame_rate = { nFrameRate, 1 };

    AVCodecParameters *pCodecpar = pStream->codecpar;
    pCodecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecpar->codec_id = video_codec_to_avcodecid(enCodec);
    if (enCodec == Video_NS::VideoCodec_E::H265)
    {
        /* FFmpeg HEVC FLV 补丁使用 codec_tag=12 标识 FLV_CODECID_HEVC。 */
        pCodecpar->codec_tag = 12;
    }
    pCodecpar->width = nWidth;
    pCodecpar->height = nHeight;
    pCodecpar->format = AV_PIX_FMT_YUV420P;
    if (set_codec_extradata(pCodecpar, vExtradata) < 0)
    {
        dlog_error("设置视频extradata失败，通道=%d", nChannel);
        return ERR;
    }

    return OK;
}

int CRtmpStreamContext::create_audio_stream(int nSampleRate, int nChannels, const std::vector<uint8_t> &vExtradata, int nChannel)
{
    if (!m_pFormatCtx)
    {
        return ERR;
    }

    AVStream *pStream = avformat_new_stream(m_pFormatCtx, nullptr);
    if (!pStream)
    {
        dlog_error("创建音频流失败，通道=%d", nChannel);
        return ERR;
    }

    m_nAudioStreamIndex = pStream->index;
    pStream->time_base = { 1, nSampleRate };

    AVCodecParameters *pCodecpar = pStream->codecpar;
    pCodecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    pCodecpar->codec_id = AV_CODEC_ID_AAC;
    pCodecpar->sample_rate = nSampleRate;
    pCodecpar->channels = nChannels;
    pCodecpar->channel_layout = av_get_default_channel_layout(nChannels);
    pCodecpar->format = AV_SAMPLE_FMT_FLTP;
    if (set_codec_extradata(pCodecpar, vExtradata) < 0)
    {
        dlog_error("设置音频extradata失败，通道=%d", nChannel);
        return ERR;
    }

    return OK;
}

int CRtmpStreamContext::write_header(Video_NS::VideoCodec_E enCodec, int nChannel)
{
    if (!m_pFormatCtx)
    {
        return ERR;
    }

    AVDictionary *pOpts = nullptr;
    av_dict_set(&pOpts, "flvflags", "no_duration_filesize", 0);

    const int64_t nHeaderStartMs = rtmp_io_now_ms();
    int nRet = avformat_write_header(m_pFormatCtx, &pOpts);
    const int64_t nHeaderCostMs = rtmp_io_now_ms() - nHeaderStartMs;
    av_dict_free(&pOpts);
    if (nRet < 0)
    {
        char errBuf[256] = { 0 };
        av_strerror(nRet, errBuf, sizeof(errBuf));
        dlog_error("写FLV header失败: %s, 通道=%d, codec=%d, cost=%lldms",
                   errBuf,
                   nChannel,
                   static_cast<int>(enCodec),
                   static_cast<long long>(nHeaderCostMs));
        return ERR;
    }

    dlog_info("RTMP FLV header写入成功，通道=%d, cost=%lldms", nChannel, static_cast<long long>(nHeaderCostMs));
    if (nHeaderCostMs >= RTMP_IO_WARN_MS)
    {
        dlog_warn("RTMP写FLV header耗时偏高，通道=%d, cost=%lldms", nChannel, static_cast<long long>(nHeaderCostMs));
    }
    const int64_t nFlushStartMs = rtmp_io_now_ms();
    flush();
    const int64_t nFlushCostMs = rtmp_io_now_ms() - nFlushStartMs;
    if (nFlushCostMs >= RTMP_IO_WARN_MS)
    {
        dlog_warn("RTMP FLV header flush耗时偏高，通道=%d, cost=%lldms", nChannel, static_cast<long long>(nFlushCostMs));
    }
    return OK;
}

int CRtmpStreamContext::write_frame(AVPacket *pPacket)
{
    if (!m_pFormatCtx || !pPacket)
    {
        return ERR;
    }

    /* 更新IO时间 */
    m_nLastIoTimeSec = av_gettime_relative() / 1000000;

    return av_interleaved_write_frame(m_pFormatCtx, pPacket);
}

void CRtmpStreamContext::flush()
{
    if (m_pFormatCtx && m_pFormatCtx->pb)
    {
        avio_flush(m_pFormatCtx->pb);
    }
}

void CRtmpStreamContext::request_interrupt()
{
    m_bInterruptRequested.store(true);
}

void CRtmpStreamContext::reset_interrupt()
{
    m_bInterruptRequested.store(false);
}

AVRational CRtmpStreamContext::video_time_base() const
{
    if (!m_pFormatCtx || m_nVideoStreamIndex < 0)
    {
        return { 1, 1000 };
    }
    return m_pFormatCtx->streams[m_nVideoStreamIndex]->time_base;
}

AVRational CRtmpStreamContext::audio_time_base() const
{
    if (!m_pFormatCtx || m_nAudioStreamIndex < 0)
    {
        return { 1, 1000 };
    }
    return m_pFormatCtx->streams[m_nAudioStreamIndex]->time_base;
}

AVCodecID CRtmpStreamContext::video_codec_to_avcodecid(Video_NS::VideoCodec_E enCodec) const
{
    switch (enCodec)
    {
    case Video_NS::VideoCodec_E::H264:
        return AV_CODEC_ID_H264;
    case Video_NS::VideoCodec_E::H265:
        return AV_CODEC_ID_HEVC;
    default:
        return AV_CODEC_ID_H264;
    }
}

int CRtmpStreamContext::set_codec_extradata(AVCodecParameters *pCodecpar, const std::vector<uint8_t> &vExtradata)
{
    if (!pCodecpar || vExtradata.empty())
    {
        return ERR;
    }

    pCodecpar->extradata = static_cast<uint8_t *>(av_mallocz(vExtradata.size() + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!pCodecpar->extradata)
    {
        return ERR;
    }
    std::memcpy(pCodecpar->extradata, vExtradata.data(), vExtradata.size());
    pCodecpar->extradata_size = static_cast<int>(vExtradata.size());
    return OK;
}

void CRtmpStreamContext::set_timeout(int nTimeoutSec)
{
    m_nTimeoutSec = nTimeoutSec;
}

int CRtmpStreamContext::get_timeout_sec() const
{
    return m_nTimeoutSec;
}
