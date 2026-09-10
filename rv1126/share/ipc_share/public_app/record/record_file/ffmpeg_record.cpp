/**
 * @FilePath     : ffmpeg_record.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-08 17:01:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-05 15:34:24
 * @Description  : ffmpeg录制类
 */

#include <stdlib.h>
#include <sys/time.h>
#include <algorithm>
#include "dlog.h"
#include "ffmpeg_record.h"

#include "time_utils.h"
#include "time_tools.h"

int get_spsPpsLen(unsigned char *pData, int nSize)
{
    unsigned char *pchData = pData;
    int nIndex = 0;
    int nLastTye = 0;
    int nSpsPpsLen = 0;
    int nLastIndex = -1;
    int nDataSize = nSize;
    for (nIndex = 0; nIndex < nDataSize - 5; nIndex++)
    {
        if (pchData[nIndex] == 0x00 && pchData[nIndex + 1] == 0x00 && pchData[nIndex + 2] == 0x00 && pchData[nIndex + 3] == 0x01)
        {
            if (nLastIndex >= 0 && nLastTye == 0x8)
            {
                nSpsPpsLen = nIndex;
                return nSpsPpsLen;
            }
            nLastIndex = nIndex;
            nLastTye = pchData[nIndex + 4] & 0x1f;
        }
    }
    return nSpsPpsLen;
}

int FfmpegRecord::init(SliceInfo_S stSliceInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stSliceInfo = stSliceInfo;

    int nRet = init_context();
    if (nRet < 0)
    {
        return nRet;
    }
    init_videoStream();
    init_audioStream();
    nRet = open_file();
    if (nRet < 0)
    {
        return nRet;
    }
    return nRet;
}
void FfmpegRecord::deinit()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stSliceInfo.nEndTimeMs = m_nVptsMs;
    struct timeval stTval;
    gettimeofday(&stTval, NULL);
    m_stSliceInfo.nEndTimestampMs = stTval.tv_sec * 1000 + stTval.tv_usec / 1000;

    if (m_pFormatContext != nullptr)
    {
        /* 仅在成功写入文件头后写入文件尾 */
        if (m_nFirstFrame == 1)
        {
            av_write_trailer(m_pFormatContext);
        }

        /* 即使首个I帧尚未到达，也必须关闭文件并释放上下文 */
        if (m_pFormatContext->pb != nullptr)
        {
            avio_closep(&m_pFormatContext->pb);
        }
        avformat_free_context(m_pFormatContext);
        m_pFormatContext = nullptr;
    }

    /* 重置流索引值 */
    m_stVideoStream.nIndex = 0;
    m_stAudioStream.nIndex = 0;

    /*视频首帧*/
    m_nFirstFrame = 0;
}
void FfmpegRecord::reset()
{
    /*开始录制时,当天00:00:00到现在经过的毫秒数*/
    m_nRecStartTimeMs = 0;

    /*开始录制时的实际时间戳，毫秒*/
    m_nStartTimeStampMs = 0;
    /*开始录制时的单调时间戳，毫秒*/
    m_nStartMonotonicTimestampMs = 0;
    /* 音频帧数 */
    m_nAudioCount = 0;
    /* 视频帧数 */
    m_nVideoCount = 0;
    /*记录视频时间戳*/
    m_nVptsMs = 0;
    /*记录音频时间戳*/
    m_nAptsMs = 0;
    
    /*重置PTS基准*/
    m_lastVideoPts = -1;
    m_lastAudioPts = -1;
}

int FfmpegRecord::reset_lastPts()
{
    m_lastVideoPts = -1;
    m_lastAudioPts = -1;
    return 0;
}

bool FfmpegRecord::is_init()
{
    return m_pFormatContext ? true : false;
}

int FfmpegRecord::init_context()
{
    int nRet = avformat_alloc_output_context2(&m_pFormatContext,
                                              nullptr,
                                              "mpegts",
                                              m_stSliceInfo.filename.c_str());
    if (nRet < 0 || m_pFormatContext == nullptr)
    {
        dlog_error("创建mpegts输出上下文失败, ret:%d, file:%s",
                   nRet,
                   m_stSliceInfo.filename.c_str());
        return -1;
    }
    return 0;
}

void FfmpegRecord::init_videoStream()
{
    /* 开启视频录制 */
    if (!m_stSliceInfo.nVideoFlag)
    {
        return;
    }

    
    /* 创建流 */
    m_stVideoStream.pAvStream = avformat_new_stream(m_pFormatContext, NULL);
    if (!m_stVideoStream.pAvStream)
    {
        dlog_debug("创建视频流失败");
        return;
    }

    /* 创建流成功 */
    m_stVideoStream.pAvStream->id = m_pFormatContext->nb_streams - 1;
    m_stVideoStream.nIndex = m_stVideoStream.pAvStream->index;

    /* 设置流的时间基 */
    m_stVideoStream.pAvStream->time_base = (AVRational){1, m_stSliceInfo.nRealFrameRate};
    m_stVideoStream.pAvStream->avg_frame_rate = (AVRational){m_stSliceInfo.nRealFrameRate, 1};

    /* 直接设置 codecpar，用于复用 */
    AVCodecParameters *codecpar = m_stVideoStream.pAvStream->codecpar;
    codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    codecpar->codec_id = (AVCodecID)m_stSliceInfo.nVideoCodeID;
    codecpar->width = m_stSliceInfo.nVencWidth;
    codecpar->height = m_stSliceInfo.nVencHeight;
    codecpar->format = AV_PIX_FMT_YUVJ420P;
    codecpar->bit_rate = 400000;
    
    dlog_debug("视频流创建成功，codec_id=%d", codecpar->codec_id);
}

void FfmpegRecord::init_audioStream()
{
    /* 开启音频录制 */
    if (!m_stSliceInfo.nAudioFlag)
    {
        return;
    }

    /*过滤暂不支持格式*/
    if (m_stSliceInfo.nAudioCodeID != AV_CODEC_ID_AAC)
    {
        dlog_debug("不支持的音频格式: %d", m_stSliceInfo.nAudioCodeID);
        return;
    }
    /* 直接创建流，不需要编码器 */
    m_stAudioStream.pAvStream = avformat_new_stream(m_pFormatContext, NULL);
    if (!m_stAudioStream.pAvStream)
    {
        dlog_debug("创建音频流失败");
        return;
    }

    m_stAudioStream.pAvStream->id = m_pFormatContext->nb_streams - 1;
    m_stAudioStream.nIndex = m_stAudioStream.pAvStream->index;

    /* 设置流的时间基 */
    m_stAudioStream.pAvStream->time_base = (AVRational) { 1, m_stSliceInfo.nSampleRate };

    /* 直接设置 codecpar，用于复用 */
    AVCodecParameters* codecpar = m_stAudioStream.pAvStream->codecpar;
    codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    codecpar->codec_id = (AVCodecID) m_stSliceInfo.nAudioCodeID;
    codecpar->sample_rate = m_stSliceInfo.nSampleRate;
    codecpar->format = m_stSliceInfo.nSampleFmt;
    codecpar->bit_rate = 128000;
    codecpar->channel_layout = m_stSliceInfo.nChannel;
    codecpar->channels = av_get_channel_layout_nb_channels(codecpar->channel_layout);

    dlog_debug("音频流创建成功，codec_id=%d, sample_rate=%d", codecpar->codec_id, codecpar->sample_rate);
}

int FfmpegRecord::open_file()
{
    int nRet = avio_open(&m_pFormatContext->pb, m_stSliceInfo.filename.c_str(), AVIO_FLAG_WRITE);
    if (nRet < 0)
    {
        char buf[1024] = {0};
        av_make_error_string(buf, sizeof(buf), nRet);
        dlog_error("Could not open '%s': %s\n", m_stSliceInfo.filename.c_str(), buf);
        return -1;
    }
    return 0;
}

int FfmpegRecord::av_sync(int nType, RecordFrame_S &stFrameData, AVRational &stSrcTimebase, AVRational &stDstTimebase)
{
    /*音视频同步策略*/
    if (AVMEDIA_TYPE_VIDEO == nType && m_stSliceInfo.nVideoFlag && m_pFormatContext->streams)
    {
        stSrcTimebase = (AVRational){1, m_stSliceInfo.nRealFrameRate * 1000};
        stDstTimebase.num = m_pFormatContext->streams[m_stVideoStream.nIndex]->time_base.num;
        stDstTimebase.den = m_pFormatContext->streams[m_stVideoStream.nIndex]->time_base.den;

        /*视频*/
        stFrameData.nPts = 1000 * m_nVideoCount;
        stFrameData.nDts = stFrameData.nPts;
        stFrameData.nStreamIndex = m_stVideoStream.nIndex;
        m_nVideoCount++;

        /*将视频时间戳转换到毫秒单位*/
        m_nVptsMs = av_rescale_q(stFrameData.nPts, stSrcTimebase, (AVRational){1, 1000});

        /*判断有音频，并且音频时间戳大于视频时间戳 500ms ,同步视频时间戳*/
        if (m_nAptsMs - m_nVptsMs > 500 && m_stSliceInfo.nAudioFlag)
        {
            /* 使用单调时间计算录制经过时长，避免系统校时或时区切换导致PTS回退 */
            int64_t nCurrentMonotonicMs = TimeUtils_NS::get_monotonicTimestampMs();
            int64_t nTimeMs = nCurrentMonotonicMs - m_nStartMonotonicTimestampMs + m_nRecStartTimeMs;
            m_nVptsMs = nTimeMs;

            /*将毫秒单位转换到相应单位*/
            stFrameData.nPts = av_rescale_q(m_nVptsMs, (AVRational){1, 1000}, stSrcTimebase);
            stFrameData.nDts = stFrameData.nPts;

            m_nVideoCount = stFrameData.nPts / 1000;
            dlog_debug("同步视频时间戳,nVideoCount=%ld,nPts=%d,monoNow=%lld,monoStart=%lld",
                       m_nVideoCount,
                       stFrameData.nPts,
                       nCurrentMonotonicMs,
                       m_nStartMonotonicTimestampMs);
        }
    }
    else if (AVMEDIA_TYPE_AUDIO == nType && m_stSliceInfo.nAudioFlag && m_stAudioStream.pAvStream)
    {
        stSrcTimebase = (AVRational){1, m_stSliceInfo.nSampleRate};
        stDstTimebase = m_stAudioStream.pAvStream->time_base;
        stFrameData.nStreamIndex = m_stAudioStream.nIndex;

        int nSamplePerFrame = 1024;

        if (m_stSliceInfo.nAudioCodeID == (AVCodecID)AV_CODEC_ID_AAC)
        {
            nSamplePerFrame = 1024;
            stFrameData.nPts = m_nAudioCount * nSamplePerFrame;
            stFrameData.nDts = stFrameData.nPts;
            /*将音频时间戳转换到毫秒单位*/
            m_nAptsMs = av_rescale_q(stFrameData.nPts, stSrcTimebase, (AVRational){1, 1000});
            m_nAudioCount++;
        }
        else if (m_stSliceInfo.nAudioCodeID == (AVCodecID)AV_CODEC_ID_MP3 ||
                 m_stSliceInfo.nAudioCodeID == (AVCodecID)AV_CODEC_ID_MP2)
        {
            nSamplePerFrame = 1152;
            stFrameData.nPts = m_nAudioCount * nSamplePerFrame;
            stFrameData.nDts = stFrameData.nPts;
            /*将音频时间戳转换到毫秒单位*/
            m_nAptsMs = av_rescale_q(stFrameData.nPts, stSrcTimebase, (AVRational){1, 1000});
            m_nAudioCount++;
        }

        /*判断有视频，并且视频时间戳大于音频时间戳 500ms ,同步音频时间戳*/
        if (m_nVptsMs - m_nAptsMs > 500 && m_stSliceInfo.nVideoFlag)
        {
            /* 使用单调时间计算录制经过时长，避免系统校时或时区切换导致PTS回退 */
            int64_t nCurrentMonotonicMs = TimeUtils_NS::get_monotonicTimestampMs();
            int64_t nTimeMs = nCurrentMonotonicMs - m_nStartMonotonicTimestampMs + m_nRecStartTimeMs;
            m_nAptsMs = nTimeMs;

            /*将毫秒单位转换到相应单位*/
            stFrameData.nPts = av_rescale_q(m_nAptsMs, (AVRational){1, 1000}, stSrcTimebase);
            stFrameData.nDts = stFrameData.nPts;
            // m_nAudioCount = stFrameData.nPts/nSamplePerFrame;
            // 根据不同格式重新计算音频帧数
            if (m_stSliceInfo.nAudioCodeID == (AVCodecID)AV_CODEC_ID_AAC)
            {
                m_nAudioCount = stFrameData.nPts / 1024;
            }
            else if (m_stSliceInfo.nAudioCodeID == (AVCodecID)AV_CODEC_ID_MP3 ||
                     m_stSliceInfo.nAudioCodeID == (AVCodecID)AV_CODEC_ID_MP2)
            {
                m_nAudioCount = stFrameData.nPts / 1152;
            }
            dlog_debug("同步音频时间戳,nAudioCount=%ld,nPts=%d,monoNow=%lld,monoStart=%lld",
                       m_nAudioCount,
                       stFrameData.nPts,
                       nCurrentMonotonicMs,
                       m_nStartMonotonicTimestampMs);
        }
    }
    else
    {
        dlog_error("av_sync is error, nType=%d, AudioFlag=%d, VideoFlag=%d", nType, m_stSliceInfo.nAudioFlag, m_stSliceInfo.nVideoFlag);
        return -1;
    }
    return 0;
}

int FfmpegRecord::write(RecordData_S &stRecordDate)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pFormatContext == nullptr)
    {
        dlog_error("m_pFormatContext is NULL");
        return -1;
    }

    if (stRecordDate.nType == AVMEDIA_TYPE_VIDEO && 0 == m_nFirstFrame && 1 == stRecordDate.nKey)
    {
        /*获取sps和pps*/
        int nSize = get_spsPpsLen(stRecordDate.pData, stRecordDate.nSize);
        /*写文件头*/
        if (write_head(stRecordDate.pData, nSize) != 0)
        {
            dlog_debug("write_head error");
            return -1;
        }
        else
        {
            m_nFirstFrame = 1;
        }
    }
    else if (0 == m_nFirstFrame)
    {
        return -1;
    }
    RecordFrame_S stFrameData;
    stFrameData.nKey = stRecordDate.nKey;
    stFrameData.pData = stRecordDate.pData;
    stFrameData.nSize = stRecordDate.nSize;

    AVRational stSrcTimebase;
    AVRational stDstTimebase;
    int nRet = av_sync(stRecordDate.nType, stFrameData, stSrcTimebase, stDstTimebase);
    if (nRet < 0)
    {
        dlog_error("av_sync is error");
        return -1;
    }
    AVPacket stPacket;
    av_init_packet(&stPacket);
    /*准备写入文件*/
    stPacket.data = stFrameData.pData;
    stPacket.size = stFrameData.nSize;
    stPacket.flags = stFrameData.nKey ? AV_PKT_FLAG_KEY : 0;
    stPacket.stream_index = stFrameData.nStreamIndex;
    stPacket.pts = av_rescale_q(stFrameData.nPts, stSrcTimebase, stDstTimebase);
    stPacket.dts = stPacket.pts;

    /* 在 av_write_frame 之前添加时间戳验证 */
    // static int64_t lastVideoPts = -1;
    // static int64_t lastAudioPts = -1;

    if (stRecordDate.nType == AVMEDIA_TYPE_VIDEO)
    {
        if (m_lastVideoPts != -1 && stPacket.pts <= m_lastVideoPts)
        {
            dlog_debug("视频时间戳异常: 当前PTS: %lld, 上一个PTS: %lld",
                       stPacket.pts, m_lastVideoPts);
            stPacket.pts = m_lastVideoPts + 1; // 强制单调递增
            stPacket.dts = stPacket.pts;
        }
        m_lastVideoPts = stPacket.pts;
    }
    else if (stRecordDate.nType == AVMEDIA_TYPE_AUDIO)
    {
        if (m_lastAudioPts != -1 && stPacket.pts <= m_lastAudioPts)
        {
            dlog_debug("音频时间戳异常: 当前PTS: %lld, 上一个PTS: %lld",
                       stPacket.pts, m_lastAudioPts);
            stPacket.pts = m_lastAudioPts + 1; // 强制单调递增
            stPacket.dts = stPacket.pts;
        }
        m_lastAudioPts = stPacket.pts;
    }

    nRet = av_write_frame(m_pFormatContext, &stPacket);
    if (nRet != 0)
    {
        dlog_debug("av_write_frame error,nRet=%d", nRet);
        return -1;
    }
    m_stSliceInfo.nSize += stPacket.size;
    return nRet;
}

/*初始化录制时间*/
int FfmpegRecord::init_startTime(uint64_t nCurrentTimeMs)
{
    m_nStartTimeStampMs = nCurrentTimeMs;
    m_nStartMonotonicTimestampMs = TimeUtils_NS::get_monotonicTimestampMs();

    /* 连续分片时沿用上一片段PTS，保证PTS单调递增 */
    if (m_nVptsMs > 0 || m_nAptsMs > 0)
    {
        m_nRecStartTimeMs = std::max(m_nVptsMs, m_nAptsMs);
        dlog_debug("连续分片沿用上次PTS, recStartMs:%lld, videoPtsMs:%lld, audioPtsMs:%lld",
                   m_nRecStartTimeMs, m_nVptsMs, m_nAptsMs);
    }
    else
    {
        /* 首次录制以本地当天00:00:00为PTS基准 */
        struct tm stTm;
        time_t nTimeStamp = time(NULL);
        localtime_r(&nTimeStamp, &stTm);
        stTm.tm_hour = 0;
        stTm.tm_min = 0;
        stTm.tm_sec = 0;
        uint64_t nTimeMs = mktime(&stTm) * 1000;

        /* 从当天00:00:00起经过的毫秒数 */
        m_nRecStartTimeMs = nCurrentTimeMs - nTimeMs;
        m_nVptsMs = m_nRecStartTimeMs;
        m_nAptsMs = m_nRecStartTimeMs;
        dlog_debug("首次录制初始化PTS, recStartMs:%lld", m_nRecStartTimeMs);
    }

    /* 视频：毫秒转帧时间基 */
    AVRational stVideoSrcTimebase = (AVRational){1, m_stSliceInfo.nRealFrameRate * 1000};
    int64_t nVPts = av_rescale_q(m_nRecStartTimeMs, (AVRational){1, 1000}, stVideoSrcTimebase);
    m_nVideoCount = nVPts / 1000;

    /* 音频：毫秒转采样时间基 */
    AVRational stAudioSrcTimebase = (AVRational){1, m_stSliceInfo.nSampleRate};
    int64_t nAPts = av_rescale_q(m_nRecStartTimeMs, (AVRational){1, 1000}, stAudioSrcTimebase);

    if (m_stSliceInfo.nAudioCodeID == (AVCodecID)AV_CODEC_ID_AAC)
    {
        m_nAudioCount = nAPts / 1024;
    }
    else if (m_stSliceInfo.nAudioCodeID == (AVCodecID)AV_CODEC_ID_MP3 ||
             m_stSliceInfo.nAudioCodeID == (AVCodecID)AV_CODEC_ID_MP2)
    {
        m_nAudioCount = nAPts / 1152;
    }

    dlog_debug("初始化PTS基准完成, recStartMs:%lld, videoCount:%lld, audioCount:%lld",
               m_nRecStartTimeMs, m_nVideoCount, m_nAudioCount);

    return 0;
}

int FfmpegRecord::write_head(unsigned char *pData, int nSize)
{
    int nRet = 0;
    if (m_stSliceInfo.nVideoFlag && m_stVideoStream.pAvStream)
    {
        AVCodecParameters *codecpar = m_stVideoStream.pAvStream->codecpar;
        codecpar->extradata = (uint8_t *)av_malloc(nSize + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!codecpar->extradata)
        {
            dlog_debug("分配视频extradata失败");
            return -1;
        }
        memset(codecpar->extradata, 0, nSize + AV_INPUT_BUFFER_PADDING_SIZE);
        memcpy(codecpar->extradata, pData, nSize);
        codecpar->extradata_size = nSize;
        m_stSliceInfo.nSize += nSize;
        
        dlog_debug("设置视频extradata成功，大小=%d", nSize);
    }

    if (m_stSliceInfo.nAudioFlag && m_stAudioStream.pAvStream)
    {
        AVCodecParameters *codecpar = m_stAudioStream.pAvStream->codecpar;

        if (m_stSliceInfo.nAudioCodeID == AV_CODEC_ID_AAC)
        {
            codecpar->extradata = (uint8_t *)av_malloc(2 + AV_INPUT_BUFFER_PADDING_SIZE);
            if (!codecpar->extradata)
            {
                dlog_debug("分配音频extradata失败");
                return -1;
            }
            memset(codecpar->extradata, 0, 2 + AV_INPUT_BUFFER_PADDING_SIZE);

            /*根据实际采样率设置频率索引*/
            int nFrequency = 11; // 8000Hz对应索引11
            if (m_stSliceInfo.nSampleRate == 48000)
                nFrequency = 3;
            else if (m_stSliceInfo.nSampleRate == 44100)
                nFrequency = 4;
            else if (m_stSliceInfo.nSampleRate == 32000)
                nFrequency = 5;
            else if (m_stSliceInfo.nSampleRate == 16000)
                nFrequency = 8;
            else if (m_stSliceInfo.nSampleRate == 8000)
                nFrequency = 11;

            int nConfigurations = m_stSliceInfo.nChannel;
            if (nConfigurations == AV_CH_LAYOUT_STEREO)
            {
                nConfigurations = 2;
            }
            if (nConfigurations == AV_CH_LAYOUT_MONO)
            {
                nConfigurations = 1;
            }

            codecpar->extradata[0] = 2 << 3 | nFrequency >> 1;
            codecpar->extradata[1] = (nFrequency & 1) << 7 | nConfigurations << 3;
            codecpar->extradata_size = 2;
        }
    }

    /*获取系统时间，保存片段起始时间*/
    struct timeval stTval;
    gettimeofday(&stTval, NULL);
    uint64_t nCurrentTimeMs = stTval.tv_sec * 1000 + stTval.tv_usec / 1000;
    m_stSliceInfo.nStartTimestampMs = nCurrentTimeMs;
    m_stSliceInfo.startTime = Time::get_curTime();

    /*判断计数值为0，说明刚开始录制,或需要同步时间戳，记录系统00:00:00起经过的毫秒数*/
    if (m_nVideoCount == 0)
    {
        init_startTime(nCurrentTimeMs);
    }
    else
    {
        /* 使用单调时间计算00:00:00起到现在经过的毫秒数 */
        int64_t nCurrentMonotonicMs = TimeUtils_NS::get_monotonicTimestampMs();
        int64_t nTimeMs = nCurrentMonotonicMs - m_nStartMonotonicTimestampMs + m_nRecStartTimeMs;
        /* 判断时间大于1秒，视频丢帧或录制线程阻塞，同步时间 */
        if (abs(nTimeMs - m_nVptsMs) > 1000)
        {
            dlog_warn("视频丢帧或录制线程阻塞,fps=%d,nRealFrameRate=%d,monoNow=%lld,monoStart=%lld,nTimeMs=%lld,nVptsMs=%lld",
                       m_stSliceInfo.nFps,
                       m_stSliceInfo.nRealFrameRate,
                       nCurrentMonotonicMs,
                       m_nStartMonotonicTimestampMs,
                       nTimeMs,
                       m_nVptsMs);
            init_startTime(nCurrentTimeMs);
        }
    }

    m_stSliceInfo.nStartTimeMs = m_nVptsMs;

    nRet = avformat_write_header(m_pFormatContext, NULL);

    if (nRet < 0)
    {
        char buf[1024] = {0};
        av_make_error_string(buf, sizeof(buf), nRet);
        dlog_debug("Error occurred when opening output file: %s", buf);
        return -1;
    }
    // av_dump_format(m_pFormatContext, 0, m_stSliceInfo.filename.c_str(), 1);
    return 0;
}

int64_t FfmpegRecord::get_videoCount()
{
    return m_nVideoCount;
}

int64_t FfmpegRecord::get_audioCount()
{
    return m_nAudioCount;
}

void FfmpegRecord::clear_count()
{
    m_nVideoCount = 0;
    m_nAudioCount = 0;
}

int FfmpegRecord::get_durationMs()
{
    int64_t nCurrentMonotonicMs = TimeUtils_NS::get_monotonicTimestampMs();
    return nCurrentMonotonicMs - m_nStartMonotonicTimestampMs;
}

std::string FfmpegRecord::get_startTime()
{
    return m_stSliceInfo.startTime;
}

void FfmpegRecord::set_audioPts(int64_t nPts)
{
    m_nAptsMs = nPts;
}

int64_t FfmpegRecord::get_audioPts()
{
    return m_nAptsMs;
}

void FfmpegRecord::set_videoPts(int64_t nPts)
{
    m_nVptsMs = nPts;
}

int64_t FfmpegRecord::get_videoPts()
{
    return m_nVptsMs;
}

void FfmpegRecord::set_frameRate(int nFrameRate)
{
    m_stSliceInfo.nRealFrameRate = nFrameRate;
}

int64_t FfmpegRecord::get_startTimeStampMs()
{
    return m_stSliceInfo.nStartTimestampMs;
}

int64_t FfmpegRecord::get_endTimestampMs()
{
    return m_stSliceInfo.nEndTimestampMs;
}

void FfmpegRecord::set_mediaInfo(SliceInfo_S stSliceInfo)
{
    m_stSliceInfo = std::move(stSliceInfo);
}

SliceInfo_S FfmpegRecord::get_mediaInfo()
{
    return m_stSliceInfo;
}
