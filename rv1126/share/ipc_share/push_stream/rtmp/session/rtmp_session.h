/**
 * @FilePath     : rtmp_session.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 17:30:00
 * @Description  : RTMP单路推流会话
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "IpcRet.h"
#include "audio/aac_adts_parser.h"
#include "audio_define.h"
#include "common/frame_queue.h"
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
                 const std::string &strUrl,
                 const Video_NS::VideoConfig_S &stVideoConfig,
                 const Audio_NS::AudioConfig_S &stAudioConfig);
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
    int send_video_frame(Video_NS::VideoFrame_S *pVideoFrame);

    /**
     * @brief 发送 VENC 只读帧视图
     * @param pData VENC pack 数据地址，仅在本次调用期间有效
     * @param nDataLen 编码数据长度
     * @param eType NAL 类型
     * @return 0成功，非0失败
     * @note 仅在有界队列入队时复制数据，不保存 VENC 原始指针。
     */
    int send_video_frame(const uint8_t *pData, int nDataLen, Video_NS::NalType_E eType);

    /**
     * @brief 发送共享媒体帧（引用计数零拷贝入队）
     * @param stSharedFrame 共享帧数据
     * @param eType NAL 类型
     * @return 0成功，非0失败
     * @note 入队不复制数据，仅增加 shared_ptr 引用计数，
     *       与 RTSP/录制共享同一份 buffer，降低多消费者总内存。
     */
    int send_video_frame(const Video_NS::SharedMediaFrame_S &stSharedFrame, Video_NS::NalType_E eType);

    /**
     * @brief 发送音频帧
     * @param pAudioFrame 音频帧数据
     * @return 0成功，非0失败
     */
    int send_audio_frame(Audio_NS::AudioFrame_S *pAudioFrame);

    /**
     * @brief 获取通道号
     * @return 通道号
     */
    int channel() const
    {
        return m_nChannel;
    }

    /**
     * @brief 获取推流URL
     * @return URL字符串
     */
    std::string url() const
    {
        return m_strUrl;
    }

private:
    /**
     * @brief 初始化视频流
     * @param pData 用于提取参数集的首个视频帧数据
     * @param nLen 视频帧数据长度
     * @param enVideoCodec 视频编码格式
     * @return 0成功，非0失败
     */
    int init_video_stream(const uint8_t *pData, int nLen, Video_NS::VideoCodec_E enVideoCodec);

    /**
     * @brief 初始化音频流
     * @param nFrameLen 用于解析ADTS参数的首个音频帧长度
     * @param stAdtsInfo ADTS解析信息
     * @return 0成功，非0失败
     */
    int init_audio_stream(int nFrameLen, const RtmpAudio_NS::AacAdtsInfo_S &stAdtsInfo);

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
     * @brief 发送线程主循环
     */
    void send_loop();

    /**
     * @brief 处理视频帧发送
     * @param pFrameData 帧数据
     * @return 0成功，非0失败
     */
    int process_video_frame(std::unique_ptr<FrameData> pFrameData);

    /**
     * @brief 处理音频帧发送
     * @param pFrameData 帧数据
     * @return 0成功，非0失败
     */
    int process_audio_frame(std::unique_ptr<FrameData> pFrameData);

    /**
     * @brief   : 按低延迟窗口刷新FFmpeg输出缓冲
     * @param    {bool} bForceFlush：是否立即刷新
     * @return   {int64_t} 实际flush耗时，未刷新时返回0
     * @note    : 必须在m_mutex保护下调用，避免每个音视频包都触发系统写操作
     */
    int64_t flush_if_due(bool bForceFlush);

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
    /* 互斥锁（保护流上下文和业务状态） */
    mutable std::mutex m_mutex;
    /* 视频流是否已根据首帧参数集初始化 */
    bool m_bVideoReady = false;
    /* 音频流是否已根据ADTS头初始化 */
    bool m_bAudioReady = false;
    /* 音频等待超时后是否降级为纯视频推流 */
    std::atomic<bool> m_bAudioDisabled{ false };
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
    /* 上次强制刷新输出缓冲的时间点（毫秒） */
    int64_t m_nLastFlushMs = 0;
    /* H.265 AnnexB转MP4复用缓存，仅发送线程访问 */
    std::vector<uint8_t> m_vH265Mp4Data;

    /* 视频帧队列 */
    std::unique_ptr<CThreadSafeFrameQueue> m_videoQueue;
    /* 音频帧队列 */
    std::unique_ptr<CThreadSafeFrameQueue> m_audioQueue;
    /* 发送线程 */
    std::thread m_sendThread;
    /* 停止发送标志 */
    std::atomic<bool> m_bStopSend{ false };
    /* 发送线程运行状态 */
    std::atomic<bool> m_bSendThreadRunning{ false };
    /* 队列操作互斥锁（保护队列指针访问） */
    mutable std::mutex m_mutexQueue;
    /* 队列到达或停止事件，避免发送线程固定周期空转 */
    std::condition_variable m_cvQueue;

    /* 禁止拷贝 */
    CRtmpSession(const CRtmpSession &) = delete;
    CRtmpSession &operator=(const CRtmpSession &) = delete;
};
