/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : VoiceComServer.h
 * @Description  : 语音对讲服务端实现，负责监听TCP端口，接收客户端连接，协商音频参数，收发双向音频数据
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

/**
 * @brief 语音对讲播放回调函数类型
 * @details 当收到客户端发来的音频数据时调用，由宿主程序实现实际的音频播放逻辑
 * @param data 音频数据指针
 * @param size 音频数据大小（字节）
 * @note 音频帧格式由当前 VoiceCom 音频参数定义
 */
using VoiceComPlayCallback = std::function<void(const char* data, size_t size)>;

/**
 * @brief 语音对讲采集回调函数类型
 * @details SDK按协商的音频参数循环调用此回调获取音频数据，由宿主程序实现实际的音频采集逻辑
 * @param audio_param 协商后的音频参数（采样率、编码格式、通道数等）
 * @param buffer 音频数据输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return >0 实际采集的数据长度；=0 无数据可用（继续等待）；<0 采集失败或结束
 */
using VoiceComCaptureCallback = std::function<int(const NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param,
                                                  char* buffer,
                                                  size_t buffer_size)>;

/**
 * @brief 语音对讲服务端类
 * @details 负责监听TCP端口，接收客户端连接，协商音频参数，收发双向音频数据，
 *          采用单例模式，支持通过回调函数与宿主程序交互，包含三个核心线程：
 *          accept_loop（接收连接）、client_recv_loop（接收客户端音频）、capture_loop（采集并发送音频）
 */
class VoiceComServer {
public:
    /**
     * @brief 获取单例实例
     * @return 单例指针
     */
    static VoiceComServer* instance();

    /**
     * @brief 启动TCP监听服务
     * @param port 监听端口，默认9006
     * @return true表示成功，false表示失败
     */
    bool start(int port = 9006);

    /**
     * @brief 停止TCP监听服务
     * @details 停止所有线程，关闭客户端连接，释放资源
     */
    void stop();

    /**
     * @brief 发送音频数据给已连接的客户端
     * @details 将麦克风采集的音频数据发送给客户端（NVR），线程安全
     * @param data 音频数据指针
     * @param size 音频数据大小（字节）
     * @return true表示成功，false表示失败
     */
    bool send_to_client(const char* data, size_t size);

    /**
     * @brief 注册播放回调函数
     * @details 当收到客户端发来的音频数据时调用此回调，用于播放音频
     * @param cb 播放回调函数
     */
    void set_play_callback(VoiceComPlayCallback cb);

    /**
     * @brief 注册采集回调函数
     * @details SDK按协商参数循环调用此回调获取采集帧并发送给客户端
     * @param cb 采集回调函数
     */
    void set_capture_callback(VoiceComCaptureCallback cb);

    /**
     * @brief 获取当前协商的音频参数
     * @param audio_param 音频参数结构体（输出参数）
     * @return true表示成功获取参数，false表示参数尚未协商完成
     */
    bool get_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) const;

    /**
     * @brief 检查服务端是否正在运行
     * @return true表示正在运行，false表示已停止
     */
    bool is_running() const { return m_running; }

private:
    /**
     * @brief 构造函数（私有，单例模式）
     */
    VoiceComServer();

    /**
     * @brief 析构函数（私有，单例模式）
     */
    ~VoiceComServer();

    /**
     * @brief 客户端连接接收循环
     * @details 持续监听端口，接收客户端连接，启动client_recv_loop和capture_loop线程
     */
    void accept_loop();

    /**
     * @brief 客户端数据接收循环
     * @details 持续从客户端socket读取音频帧，解析VCP1参数帧和音频数据帧，调用播放回调
     */
    void client_recv_loop(socket_fd_t client_fd);


    /**
     * @brief 音频采集循环
     * @details 等待音频参数协商完成后，循环调用采集回调获取音频数据，发送给客户端
     */
    void capture_loop();

    /**
     * @brief 通知音频参数已就绪
     * @details 客户端发送VCP1参数帧后，唤醒采集线程开始采集音频
     */
    void notify_audio_param_ready();

    /**
     * @brief 获取当前音频参数快照
     * @param audio_param 音频参数结构体（输出参数）
     * @return true表示成功获取，false表示失败
     */
    bool snapshot_audio_param(NET_TV_VOICECOM_AUDIO_PARAM_S& audio_param) const;

    socket_fd_t m_listen_fd{INVALID_SOCKET_FD};     ///< 监听socket文件描述符
    socket_fd_t m_client_fd{INVALID_SOCKET_FD};     ///< 当前客户端socket文件描述符
    std::atomic<bool> m_running{false};             ///< 运行状态标志
    VoiceComPlayCallback m_play_cb;                 ///< 播放回调函数（收到客户端音频时调用）
    VoiceComCaptureCallback m_capture_cb;           ///< 采集回调函数（SDK循环调用获取音频数据）
    std::thread m_accept_thread;                    ///< 连接接收线程
    std::thread m_recv_thread;                      ///< 客户端接收线程
    std::thread m_capture_thread;                   ///< 音频采集线程
    std::mutex m_send_mutex;                        ///< 发送操作互斥锁
    mutable std::mutex m_callback_mutex;            ///< 回调函数互斥锁
    mutable std::mutex m_param_mutex;               ///< 音频参数互斥锁
    std::condition_variable m_param_cv;             ///< 音频参数条件变量（用于同步采集线程）
    NET_TV_VOICECOM_AUDIO_PARAM_S m_audio_param{};  ///< 当前协商的音频参数
    bool m_has_audio_param{false};                  ///< 音频参数是否已协商完成标志
};

}  // namespace tvsdk
