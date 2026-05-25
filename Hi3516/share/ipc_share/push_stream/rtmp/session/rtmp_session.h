/**
 * @FilePath     : rtmp_session.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:36:47
 * @Description  : RTMP单路推流会话
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include "IpcRet.h"
#include "audio/aac_adts_parser.h"
#include "audio_define.h"
#include "dlog.h"
#include "session/rtmp_stream_context.h"
#include "video_define.h"

/**
 * @brief RTMP单路推流会话
 * @note 负责一路码流的业务状态、header写入时序、PTS计数和关键帧等待
 */
class CRtmpSession
{
public:
    /**
     * @brief 构造函数
     * @param nChannel 通道号（0=主码流，1=子码流...）
     * @param strUrl RTMP推流URL
     * @param stVideoConfig 视频编码配置
     * @param stAudioConfig 音频编码配置
     */
    CRtmpSession(int nChannel,
                 const std::string& strUrl,
                 const Video_NS::VideoConfig_S& stVideoConfig,
                 const Audio_NS::AudioConfig_S& stAudioConfig);
    ~CRtmpSession();

    /**
     * @brief 初始化推流会话（连接RTMP服务器）
     * @return 0成功，非0失败
     */
    int init();

    /**
     * @brief 反初始化（断开连接、释放资源）
     */
    void deinit();

    /**
     * @brief 是否已初始化并连接成功
     * @return true已连接，false未连接
     */
    bool is_connected() const;

    /**
     * @brief 发送视频帧
     * @param pVideoFrame 视频帧数据
     * @return 0成功，非0失败
     */
    int send_video_frame(Video_NS::VideoFrame_S* pVideoFrame);

    /**
     * @brief 发送音频帧
     * @param pAudioFrame 音频帧数据
     * @return 0成功，非0失败
     */
    int send_audio_frame(Audio_NS::AudioFrame_S* pAudioFrame);

    /**
     * @brief 获取通道号
     * @return 通道号
     */
    int channel() const { return m_nChannel; }

    /**
     * @brief 获取推流URL
     * @return URL字符串
     */
    std::string url() const { return m_strUrl; }

private:
    /**
     * @brief 初始化视频流
     * @param pVideoFrame 用于提取参数集的首个视频帧
     * @return 0成功，非0失败
     */
    int init_video_stream(Video_NS::VideoFrame_S* pVideoFrame);

    /**
     * @brief 初始化音频流
     * @param pAudioFrame 用于解析ADTS参数的首个音频帧
     * @param stAdtsInfo ADTS解析信息
     * @return 0成功，非0失败
     */
    int init_audio_stream(Audio_NS::AudioFrame_S* pAudioFrame, const RtmpAudio_NS::AacAdtsInfo_S& stAdtsInfo);

    /**
     * @brief 在音视频参数齐备后写入FLV头
     * @return 0成功，非0失败
     */
    int try_write_header();

    /**
     * @brief 判断当前会话是否需要等待音频流参数
     * @return true：需要音频，false：仅视频即可写Header
     */
    bool need_audio_stream() const;

    /**
     * @brief 判断视频帧是否为关键帧
     * @param pVideoFrame 视频帧
     * @return true：关键帧，false：非关键帧
     */
    bool is_key_frame(Video_NS::VideoFrame_S* pVideoFrame) const;

private:
    /* FFmpeg流上下文 */
    CRtmpStreamContext m_stream_context;
    /* 通道号 */
    int m_nChannel = 0;
    /* RTMP推流URL */
    std::string m_strUrl;
    /* 视频编码配置 */
    Video_NS::VideoConfig_S m_stVideoConfig;
    /* 音频编码配置 */
    Audio_NS::AudioConfig_S m_stAudioConfig;
    /* 连接状态 */
    std::atomic<bool> m_bConnected{ false };
    /* 互斥锁 */
    mutable std::mutex m_mutex;
    /* 视频流是否已根据首帧参数集初始化 */
    bool m_bVideoReady = false;
    /* 音频流是否已根据ADTS头初始化 */
    bool m_bAudioReady = false;
    /* 音频等待超时后是否降级为纯视频推流 */
    bool m_bAudioDisabled = false;
    /* FLV Header是否已写入 */
    bool m_bHeaderWritten = false;
    /* 写Header后是否仍需等待视频关键帧 */
    bool m_bNeedVideoKeyFrame = true;
    /* 视频PTS计数器（毫秒） */
    int64_t m_nVideoPts = 0;
    /* 音频PTS计数器（毫秒） */
    int64_t m_nAudioPts = 0;
    /* 开始等待音频参数的时间点（毫秒） */
    int64_t m_nAudioWaitStartMs = 0;
    /* 视频帧时长（毫秒） */
    int64_t m_nVideoFrameDurationMs = 40;
    /* AAC帧时长（毫秒） */
    int64_t m_nAudioFrameDurationMs = 64;

    /* 禁止拷贝 */
    CRtmpSession(const CRtmpSession&) = delete;
    CRtmpSession& operator=(const CRtmpSession&) = delete;
};
