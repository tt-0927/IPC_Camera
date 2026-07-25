/**
 * @FilePath     : rtmp_stream_context.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:37:12
 * @Description  : RTMP FFmpeg流上下文封装
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "audio_define.h"
#include "video_define.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/channel_layout.h"
#include "libavutil/error.h"
#include "libavutil/mathematics.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
}

/**
 * @brief RTMP FFmpeg流上下文
 * @note 只负责FFmpeg资源、音视频流创建、header/trailer和写包，不处理业务时序
 */
class CRtmpStreamContext
{
public:
    CRtmpStreamContext();
    ~CRtmpStreamContext();

    /**
     * @brief 打开FLV输出上下文和RTMP IO连接
     * @param strUrl RTMP推流地址
     * @return OK：成功，ERR：失败
     */
    int open(const std::string& strUrl);

    /**
     * @brief 关闭RTMP IO并释放FFmpeg上下文
     * @param bHeaderWritten 是否已经写入FLV header
     */
    void close(bool bHeaderWritten);

    /**
     * @brief 创建视频流
     * @param enCodec 视频编码格式
     * @param nWidth 视频宽度
     * @param nHeight 视频高度
     * @param nFrameRate 帧率
     * @param vExtradata 视频extradata
     * @param nChannel 通道号，仅用于日志
     * @return OK：成功，ERR：失败
     */
    int create_video_stream(Video_NS::VideoCodec_E enCodec,
                            int nWidth,
                            int nHeight,
                            int nFrameRate,
                            const std::vector<uint8_t>& vExtradata,
                            int nChannel);

    /**
     * @brief 创建音频流
     * @param nSampleRate 采样率
     * @param nChannels 声道数
     * @param vExtradata 音频extradata
     * @param nChannel 通道号，仅用于日志
     * @return OK：成功，ERR：失败
     */
    int create_audio_stream(int nSampleRate, int nChannels, const std::vector<uint8_t>& vExtradata, int nChannel);

    /**
     * @brief 写入FLV header
     * @param enCodec 视频编码格式，仅用于日志
     * @param nChannel 通道号，仅用于日志
     * @return OK：成功，ERR：失败
     */
    int write_header(Video_NS::VideoCodec_E enCodec, int nChannel);

    /**
     * @brief 写入音视频包
     * @param pPacket FFmpeg包
     * @return OK：成功，ERR：失败
     */
    int write_frame(AVPacket* pPacket);

    /**
     * @brief 刷新FFmpeg内部缓冲区
     */
    void flush();

    /**
     * @brief 获取上下文是否已打开
     * @return true已打开，false未打开
     */
    bool is_open() const { return m_pFormatCtx != nullptr; }

    /**
     * @brief 获取视频流索引
     * @return 视频流索引
     */
    int video_stream_index() const { return m_nVideoStreamIndex; }

    /**
     * @brief 获取音频流索引
     * @return 音频流索引
     */
    int audio_stream_index() const { return m_nAudioStreamIndex; }

    /**
     * @brief 获取视频流time_base
     * @return 视频流time_base
     */
    AVRational video_time_base() const;

    /**
     * @brief 获取音频流time_base
     * @return 音频流time_base
     */
    AVRational audio_time_base() const;

    /**
     * @brief 设置超时时间
     * @param nTimeoutSec 超时时间（秒）
     */
    void set_timeout(int nTimeoutSec);

    /**
     * @brief 获取超时时间
     * @return 超时时间（秒）
     */
    int get_timeout_sec() const;

    /* 上次IO操作时间（秒），供中断回调访问 */
    int64_t m_nLastIoTimeSec = 0;

private:
    /**
     * @brief 将 VideoCodec_E 转换为 AVCodecID
     * @param enCodec 视频编码枚举
     * @return AVCodecID
     */
    AVCodecID video_codec_to_avcodecid(Video_NS::VideoCodec_E enCodec) const;

    /**
     * @brief 写入extradata到AVCodecParameters
     * @param pCodecpar FFmpeg编码参数
     * @param vExtradata extradata内容
     * @return OK：成功，ERR：失败
     */
    int set_codec_extradata(AVCodecParameters* pCodecpar, const std::vector<uint8_t>& vExtradata);

private:
    /* FFmpeg输出上下文 */
    AVFormatContext* m_pFormatCtx = nullptr;
    /* 视频流索引 */
    int m_nVideoStreamIndex = -1;
    /* 音频流索引 */
    int m_nAudioStreamIndex = -1;
    /* 超时时间（秒） */
    int m_nTimeoutSec = 5;

    /* 禁止拷贝 */
    CRtmpStreamContext(const CRtmpStreamContext&) = delete;
    CRtmpStreamContext& operator=(const CRtmpStreamContext&) = delete;
};
