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

// 音频数据回调: 音频帧由当前 VoiceCom 音频参数定义。
using VoiceComPlayCallback = std::function<void(const char* data, size_t size)>;
using VoiceComCaptureCallback = std::function<int(const NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param,
                                                  char* buffer,
                                                  size_t buffer_size)>;

class VoiceComServer {
public:
    static VoiceComServer* instance();

    // 启动TCP监听
    bool start(int port = 9006);
    // 停止监听
    void stop();

    // 发送麦克风采集的音频 → 已连接的NVR
    bool send_to_client(const char* data, size_t size);

    // 注册播放回调 (收到NVR音频时调用)
    void set_play_callback(VoiceComPlayCallback cb);

    // 注册采集回调 (SDK按协商参数拉取采集帧并发送给NVR)
    void set_capture_callback(VoiceComCaptureCallback cb);

    // 获取当前NVR协商的音频参数
    bool get_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) const;

    bool is_running() const { return m_running; }

private:
    VoiceComServer();
    ~VoiceComServer();

    void accept_loop();
    void client_recv_loop();
    void capture_loop();
    void notify_audio_param_ready();
    bool snapshot_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) const;

    socket_fd_t m_listen_fd{INVALID_SOCKET_FD};
    socket_fd_t m_client_fd{INVALID_SOCKET_FD};
    std::atomic<bool> m_running{false};
    VoiceComPlayCallback m_play_cb;
    VoiceComCaptureCallback m_capture_cb;
    std::thread m_accept_thread;
    std::thread m_recv_thread;
    std::thread m_capture_thread;
    std::mutex m_send_mutex;
    mutable std::mutex m_callback_mutex;
    mutable std::mutex m_param_mutex;
    std::condition_variable m_param_cv;
    NET_TV_VOICECOM_AUDIO_PARAM_S m_audio_param{};
    bool m_has_audio_param{false};
};

}  // namespace tvsdk
