/**
 * @FilePath     : rtmp_stream_context.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:36:58
 * @Description  : RTMP FFmpeg流上下文封装实现
 */

#include "rtmp_stream_context.h"

#include <cstring>

#include "dlog.h"
#include "IpcRet.h"

CRtmpStreamContext::CRtmpStreamContext()
{
}

CRtmpStreamContext::~CRtmpStreamContext()
{
    close(false);
}

int CRtmpStreamContext::open(const std::string& strUrl)
{
    if (m_pFormatCtx)
    {
        return OK;
    }

    int nRet = avformat_alloc_output_context2(&m_pFormatCtx, nullptr, "flv", strUrl.c_str());
    if (nRet < 0 || !m_pFormatCtx)
    {
        char errBuf[256] = { 0 };
        av_strerror(nRet, errBuf, sizeof(errBuf));
        dlog_error("创建FLV输出上下文失败: %s", errBuf);
        return ERR;
    }

    if (!(m_pFormatCtx->oformat->flags & AVFMT_NOFILE))
    {
        nRet = avio_open(&m_pFormatCtx->pb, strUrl.c_str(), AVIO_FLAG_WRITE);
        if (nRet < 0)
        {
            char errBuf[256] = { 0 };
            av_strerror(nRet, errBuf, sizeof(errBuf));
            dlog_error("打开RTMP连接失败: %s, URL=%s", errBuf, strUrl.c_str());
            avformat_free_context(m_pFormatCtx);
            m_pFormatCtx = nullptr;
            return ERR;
        }
    }

    return OK;
}

void CRtmpStreamContext::close(bool bHeaderWritten)
{
    if (!m_pFormatCtx)
    {
        return;
    }

    if (bHeaderWritten)
    {
        av_write_trailer(m_pFormatCtx);
    }

    if (!(m_pFormatCtx->oformat->flags & AVFMT_NOFILE) && m_pFormatCtx->pb)
    {
        avio_closep(&m_pFormatCtx->pb);
    }

    avformat_free_context(m_pFormatCtx);
    m_pFormatCtx = nullptr;
    m_nVideoStreamIndex = -1;
    m_nAudioStreamIndex = -1;
}

int CRtmpStreamContext::create_video_stream(Video_NS::VideoCodec_E enCodec,
                                            int nWidth,
                                            int nHeight,
                                            int nFrameRate,
                                            const std::vector<uint8_t>& vExtradata,
                                            int nChannel)
{
    if (!m_pFormatCtx)
    {
        return ERR;
    }

    AVStream* pStream = avformat_new_stream(m_pFormatCtx, nullptr);
    if (!pStream)
    {
        dlog_error("创建视频流失败，通道=%d", nChannel);
        return ERR;
    }

    m_nVideoStreamIndex = pStream->index;
    pStream->time_base = { 1, 1000 };
    pStream->avg_frame_rate = { nFrameRate, 1 };

    AVCodecParameters* pCodecpar = pStream->codecpar;
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

int CRtmpStreamContext::create_audio_stream(int nSampleRate,
                                            int nChannels,
                                            const std::vector<uint8_t>& vExtradata,
                                            int nChannel)
{
    if (!m_pFormatCtx)
    {
        return ERR;
    }

    AVStream* pStream = avformat_new_stream(m_pFormatCtx, nullptr);
    if (!pStream)
    {
        dlog_error("创建音频流失败，通道=%d", nChannel);
        return ERR;
    }

    m_nAudioStreamIndex = pStream->index;
    pStream->time_base = { 1, nSampleRate };

    AVCodecParameters* pCodecpar = pStream->codecpar;
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

    AVDictionary* pOpts = nullptr;
    av_dict_set(&pOpts, "flvflags", "no_duration_filesize", 0);

    int nRet = avformat_write_header(m_pFormatCtx, &pOpts);
    av_dict_free(&pOpts);
    if (nRet < 0)
    {
        char errBuf[256] = { 0 };
        av_strerror(nRet, errBuf, sizeof(errBuf));
        dlog_error("写FLV header失败: %s, 通道=%d, codec=%d", errBuf, nChannel, static_cast<int>(enCodec));
        return ERR;
    }

    dlog_info("RTMP FLV header写入成功，通道=%d", nChannel);
    flush();
    return OK;
}

int CRtmpStreamContext::write_frame(AVPacket* pPacket)
{
    if (!m_pFormatCtx || !pPacket)
    {
        return ERR;
    }
    return av_interleaved_write_frame(m_pFormatCtx, pPacket);
}

void CRtmpStreamContext::flush()
{
    if (m_pFormatCtx && m_pFormatCtx->pb)
    {
        avio_flush(m_pFormatCtx->pb);
    }
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

int CRtmpStreamContext::set_codec_extradata(AVCodecParameters* pCodecpar, const std::vector<uint8_t>& vExtradata)
{
    if (!pCodecpar || vExtradata.empty())
    {
        return ERR;
    }

    pCodecpar->extradata = static_cast<uint8_t*>(av_mallocz(vExtradata.size() + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!pCodecpar->extradata)
    {
        return ERR;
    }
    std::memcpy(pCodecpar->extradata, vExtradata.data(), vExtradata.size());
    pCodecpar->extradata_size = static_cast<int>(vExtradata.size());
    return OK;
}
