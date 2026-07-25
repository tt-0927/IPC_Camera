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

// 语音对讲回调: 设备端回传的音频帧，格式由 NET_TV_StartVoiceCom 协商参数定义。
using VoiceComCallback = std::function<void(const char* data, size_t size)>;

class VoiceComClient {
public:
    VoiceComClient();
    ~VoiceComClient();

    // 连接设备音频端口, 启动收发
    bool start(const std::string& host,
               int port,
               const NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param,
               VoiceComCallback callback);
    // 发送音频数据到设备
    bool send(const char* data, size_t size);
    // 停止并关闭连接
    void stop();

    bool is_running() const { return m_running; }

private:
    bool send_audio_param(const NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param);
    bool send_frame(const char* data, size_t size);
    void recv_loop();

    socket_fd_t m_socket{INVALID_SOCKET_FD};
    std::atomic<bool> m_running{false};
    VoiceComCallback m_callback;
    std::thread m_recv_thread;
    std::mutex m_send_mutex;
};

}  // namespace tvsdk
