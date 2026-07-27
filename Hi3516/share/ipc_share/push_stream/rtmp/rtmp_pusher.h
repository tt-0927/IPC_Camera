/**
 * @FilePath     : rtmp_pusher.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-12 13:56:20
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:39:39
 * @Description  : RTMP推流管理器（单例）
 */

#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "dlog.h"
#include "IpcRet.h"
#include "video_define.h"
#include "audio_define.h"
#include "rtmp_session.h"
#include "Singleton.h"

/**
 * @brief RTMP推流管理器（单例）
 * @note 管理多路RTMP推流会话，支持自动重连
 */
class CRtmpPusher : public CSingleton<CRtmpPusher>
{
private:
    CRtmpPusher();

public:
    virtual ~CRtmpPusher();
    friend class CSingleton<CRtmpPusher>;

    /**
     * @brief 初始化推流管理器
     * @return RETURN_ERROR：失败
     */
    IpcRet_E init();

    /**
     * @brief 反初始化
     * @return 非0：失败
     */
    IpcRet_E deinit();

    /**
     * @brief 获取初始化状态
     * @return true已初始化
     */
    bool is_init() const;

    /**
     * @brief 启动指定通道的推流
     * @param nChannel 通道号（0=主码流，1=子码流...）
     * @param strUrl RTMP推流URL
     * @param stVideoConfig 视频编码配置
     * @param stAudioConfig 音频编码配置
     * @return OK：成功，ERR：失败
     */
    int start_push(int nChannel,
                   const std::string& strUrl,
                   const Video_NS::VideoConfig_S& stVideoConfig,
                   const Audio_NS::AudioConfig_S& stAudioConfig);

    /**
     * @brief 停止指定通道的推流
     * @param nChannel 通道号
     * @return OK：成功，ERR：失败
     */
    int stop_push(int nChannel);

    /**
     * @brief 发送视频数据到指定通道
     * @param nChannel 通道号
     * @param pVideoFrame 视频帧数据
     * @return OK：成功，ERR：失败
     */
    int send_video_data(int nChannel, Video_NS::VideoFrame_S* pVideoFrame);

    /**
     * @brief 发送音频数据到指定通道
     * @param nChannel 通道号
     * @param pAudioFrame 音频帧数据
     * @return OK：成功，ERR：失败
     */
    int send_audio_data(int nChannel, Audio_NS::AudioFrame_S* pAudioFrame);

    /**
     * @brief 发送音频数据到所有已启动RTMP通道
     * @param pAudioFrame 音频帧数据
     * @return OK：成功，ERR：失败
     */
    int send_audio_data(Audio_NS::AudioFrame_S* pAudioFrame);

    /**
     * @brief 触发重连线程立即执行一次重连检查
     * @note 用于视频配置变更等场景，避免等待固定5秒轮询间隔
     */
    void trigger_reconnect();

private:
    /* 初始化标志 */
    std::atomic<bool> m_bInitFlag{false};
    /* 会话映射：channel -> session */
    std::unordered_map<int, std::unique_ptr<CRtmpSession>> m_mapSessions;
    /* 互斥锁 */
    mutable std::mutex m_mutex;
    /* 重连线程 */
    std::thread m_reconnectThread;
    /* 停止重连标志 */
    std::atomic<bool> m_bStopReconnect{false};
    /* 立即重连请求标志，避免条件变量通知发生在等待前时丢失请求 */
    std::atomic<bool> m_bReconnectRequested{false};
    /* 重连条件变量，用于外部唤醒立即重连 */
    std::condition_variable m_cvReconnect;
    /* 重连互斥锁（专用于条件变量） */
    mutable std::mutex m_mutexReconnect;

    /**
     * @brief 重连循环线程函数
     */
    void reconnect_loop();

    /* 禁止拷贝 */
    CRtmpPusher(const CRtmpPusher&) = delete;
    CRtmpPusher& operator=(const CRtmpPusher&) = delete;
};
