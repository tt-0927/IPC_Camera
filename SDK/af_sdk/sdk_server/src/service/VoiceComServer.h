/**
 * @file VoiceComServer.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CVoiceComServer 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CVoiceComServer 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "NetTVSDKCommon.h"
#include "PlatformCompat.h"

namespace tvsdk {

/* 音频数据回调: 音频帧由当前 VoiceCom 音频参数定义。 */
using VoiceComPlayCallback = std::function<void(const char* data, size_t size)>;
using VoiceComCaptureCallback = std::function<int(const NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param,
                                                  char* buffer,
                                                  size_t buffer_size)>;

class CVoiceComServer {
public:
    static CVoiceComServer* instance();

    /* 启动TCP监听 */
    bool start(int port = 9006);
    /* 停止监听 */
    void stop();

    /* 发送麦克风采集的音频 → 已连接的NVR */
    bool send_to_client(const char* data, size_t size);

    /* 注册播放回调 (收到NVR音频时调用) */
    void set_play_callback(VoiceComPlayCallback cb);

    /* 注册采集回调 (SDK按协商参数拉取采集帧并发送给NVR) */
    void set_capture_callback(VoiceComCaptureCallback cb);

    /* 获取当前NVR协商的音频参数 */
    bool get_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) const;

    bool is_running() const { return m_bRunning; }

private:
    CVoiceComServer();
    ~CVoiceComServer();

    void accept_loop();
    void client_recv_loop();
    void capture_loop();
    void notify_audio_param_ready();
    bool snapshot_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) const;

    socket_fd_t m_nListenFd{INVALID_SOCKET_FD};
    socket_fd_t m_nClientFd{INVALID_SOCKET_FD};
    std::atomic<bool> m_bRunning{false};
    VoiceComPlayCallback m_fnPlayCallback;
    VoiceComCaptureCallback m_fnCaptureCallback;
    std::thread m_stAcceptThread;
    std::thread m_stReceiveThread;
    std::thread m_stCaptureThread;
    std::mutex m_stSendMutex;
    mutable std::mutex m_stCallbackMutex;
    mutable std::mutex m_stParameterMutex;
    std::condition_variable m_stParameterCondition;
    NET_TV_VOICECOM_AUDIO_PARAM_S m_stAudioParameter{};
    bool m_bHasAudioParameter{false};
};

}  /* namespace tvsdk */
