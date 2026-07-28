/**
 * @file VoiceComClient.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CVoiceComClient 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CVoiceComClient 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "NetTVSDKCommon.h"
#include "PlatformCompat.h"

namespace tvsdk {

/* 语音对讲回调: 设备端回传的音频帧，格式由 NET_TV_StartVoiceCom 协商参数定义。 */
using VoiceComCallback = std::function<void(const char* data, size_t size)>;

class CVoiceComClient {
public:
    CVoiceComClient();
    ~CVoiceComClient();

    /* 连接设备音频端口, 启动收发 */
    bool start(const std::string& host,
               int port,
               const NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param,
               VoiceComCallback callback);
    /* 发送音频数据到设备 */
    bool send(const char* data, size_t size);
    /* 停止并关闭连接 */
    void stop();

    bool is_running() const { return m_bRunning; }

private:
    bool send_audio_param(const NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param);
    bool send_frame(const char* data, size_t size);
    void recv_loop();

    socket_fd_t m_nSocket{INVALID_SOCKET_FD};
    std::atomic<bool> m_bRunning{false};
    VoiceComCallback m_fnCallback;
    std::thread m_stReceiveThread;
    std::mutex m_stSendMutex;
};

}  /* namespace tvsdk */
