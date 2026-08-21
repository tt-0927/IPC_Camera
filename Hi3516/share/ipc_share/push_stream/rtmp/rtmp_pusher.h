/**
 * @FilePath     : rtmp_pusher.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-12 13:56:20
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 17:30:00
 * @Description  : RTMP推流管理器（单例）
 */

#pragma once

#include <unordered_map>
#include <cstdint>
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
                   const std::string &strUrl,
                   const Video_NS::VideoConfig_S &stVideoConfig,
                   const Audio_NS::AudioConfig_S &stAudioConfig);

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
    int send_video_data(int nChannel, Video_NS::VideoFrame_S *pVideoFrame);

    /**
     * @brief 发送 VENC 只读帧视图到指定通道
     * @param nChannel 通道号
     * @param pData VENC pack 数据地址，仅在本次调用期间有效
     * @param nDataLen 编码数据长度
     * @param eType NAL 类型
     * @return OK：成功，ERR：失败
     * @note 下游会在自己的有界队列入队时复制数据。
     */
    int send_video_data(int nChannel,
                        const uint8_t *pData,
                        int nDataLen,
                        Video_NS::NalType_E eType);

    /**
     * @brief 发送共享媒体帧到指定通道（引用计数零拷贝入队）
     * @param nChannel 通道号
     * @param stSharedFrame 共享帧数据
     * @param eType NAL 类型
     * @return OK：成功，ERR：失败
     * @note 入队不复制数据，仅增加 shared_ptr 引用计数。
     */
    int send_video_data(int nChannel,
                        const Video_NS::SharedMediaFrame_S &stSharedFrame,
                        Video_NS::NalType_E eType);

    /**
     * @brief 发送音频数据到指定通道
     * @param nChannel 通道号
     * @param pAudioFrame 音频帧数据
     * @return OK：成功，ERR：失败
     */
    int send_audio_data(int nChannel, Audio_NS::AudioFrame_S *pAudioFrame);

    /**
     * @brief 发送音频数据到所有已启动RTMP通道
     * @param pAudioFrame 音频帧数据
     * @return OK：成功，ERR：失败
     */
    int send_audio_data(Audio_NS::AudioFrame_S *pAudioFrame);

    /**
     * @brief 触发重连线程立即执行一次重连检查
     * @note 用于视频配置变更等场景，避免等待固定5秒轮询间隔
     */
    void trigger_reconnect();

private:
    /* 初始化标志 */
    std::atomic<bool> m_bInitFlag{ false };
    /* 会话映射：channel -> session；shared_ptr 保证发送路径离开管理锁后会话仍然有效 */
    std::unordered_map<int, std::shared_ptr<CRtmpSession>> m_mapSessions;
    /* 通道会话代次：防止过期的启动或重连结果重新安装已停止的会话 */
    std::unordered_map<int, uint64_t> m_mapSessionGenerations;
    /* 管理器锁：仅保护会话映射和代次，禁止覆盖网络、FFmpeg 或线程回收等慢操作 */
    mutable std::mutex m_mutex;
    /* 生命周期锁：串行化启动、停止与重连控制操作，但不参与媒体帧发送 */
    std::mutex m_mutexLifecycle;
    /* 重连线程 */
    std::thread m_reconnectThread;
    /* 停止重连标志 */
    std::atomic<bool> m_bStopReconnect{ false };
    /* 立即重连请求标志，避免条件变量通知发生在等待前时丢失请求 */
    std::atomic<bool> m_bReconnectRequested{ false };
    /* 重连条件变量，用于外部唤醒立即重连 */
    std::condition_variable m_cvReconnect;
    /* 重连互斥锁（专用于条件变量） */
    mutable std::mutex m_mutexReconnect;

    /**
     * @brief 重连循环线程函数
     */
    void reconnect_loop();

    /* 禁止拷贝 */
    CRtmpPusher(const CRtmpPusher &) = delete;
    CRtmpPusher &operator=(const CRtmpPusher &) = delete;
};
