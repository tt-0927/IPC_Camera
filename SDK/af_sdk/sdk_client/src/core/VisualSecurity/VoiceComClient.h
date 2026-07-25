/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : VoiceComClient.h
 * @Description  : 语音对讲客户端实现，负责与设备建立TCP音频通道、收发音频数据
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

/**
 * @brief 语音对讲回调函数类型
 * @details 当接收到设备端回传的音频帧时调用，格式由NET_TV_StartVoiceCom协商参数定义
 * @param data 音频数据指针
 * @param size 音频数据大小（字节）
 */
using VoiceComCallback = std::function<void(const char* data, size_t size)>;

/**
 * @brief 语音对讲客户端类
 * @details 负责与设备建立TCP连接，发送音频参数协商帧，收发音频数据
 */
class VoiceComClient {
public:
    /**
     * @brief 构造函数
     */
    VoiceComClient();

    /**
     * @brief 析构函数
     * @details 自动停止对讲并释放资源
     */
    ~VoiceComClient();

    /**
     * @brief 连接设备音频端口并启动收发
     * @param host 设备IP地址
     * @param port 设备音频端口号
     * @param audio_param 音频参数（采样率、通道数、编码格式等）
     * @param callback 音频数据回调函数，用于接收设备发送的音频数据
     * @return true表示成功，false表示失败
     */
    bool start(const std::string& host,
               int port,
               const NET_VoiceComAudioParam_S& audio_param,
               VoiceComCallback callback);

    /**
     * @brief 发送音频数据到设备
     * @param data 音频数据指针
     * @param size 音频数据大小（字节）
     * @return true表示成功，false表示失败
     */
    bool send(const char* data, size_t size);

    /**
     * @brief 停止并关闭连接
     */
    void stop();

    /**
     * @brief 检查对讲是否正在运行
     * @return true表示正在运行，false表示已停止
     */
    bool is_running() const { return m_running; }

private:
    /**
     * @brief 发送音频参数协商帧
     * @param audio_param 音频参数结构体
     * @return true表示成功，false表示失败
     */
    bool send_audio_param(const NET_VoiceComAudioParam_S& audio_param);

    /**
     * @brief 发送一帧数据（带长度头）
     * @param data 帧数据指针
     * @param size 帧数据大小（字节）
     * @return true表示成功，false表示失败
     */
    bool send_frame(const char* data, size_t size);

    /**
     * @brief 接收数据循环
     * @details 持续从socket读取音频帧，解析后通过回调转发给上层
     */
    void recv_loop();

    socket_fd_t m_socket{INVALID_SOCKET_FD};     ///< TCP socket文件描述符
    std::atomic<bool> m_running{false};          ///< 运行状态标志
    VoiceComCallback m_callback;                 ///< 音频数据回调函数
    std::thread m_recv_thread;                   ///< 接收线程
    std::mutex m_send_mutex;                     ///< 发送互斥锁

#ifdef VOICECOM_DEBUG_PRINT
    std::atomic<uint64_t> m_send_frame_count{0}; ///< 发送帧计数（调试用）
    std::atomic<uint64_t> m_recv_frame_count{0}; ///< 接收帧计数（调试用）
#endif
};

}  // namespace tvsdk
