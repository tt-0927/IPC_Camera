/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : RecordFrameServer.h
 * @Description  : 录像帧服务端实现，负责监听TCP端口，接收客户端连接，发送录像帧数据（视频/音频）
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "NetTVSDKCommon.h"
#include "PlatformCompat.h"

namespace tvsdk {

/**
 * @brief 录像帧流启动回调函数类型
 * @details 当客户端请求启动录像帧流时调用，由宿主程序实现实际的流开启逻辑
 * @param cond 流启动条件（通道、时间范围、媒体类型等）
 * @param info 流信息（输出参数，包含流ID、端口、媒体类型等）
 * @return 错误码，NET_TV_E_SUCCEED表示成功，其他值表示失败
 */
using RecordFrameStartCallback = std::function<NET_TV_COMMON_ECODE_E(const NET_RecordFrameStreamCond_S& cond,
                                                                     NET_RecordFrameStreamInfo_S& info)>;

/**
 * @brief 录像帧读取回调函数类型
 * @details 持续读取录像帧数据时调用，由宿主程序实现实际的帧读取逻辑
 * @param stream_id 流ID，标识当前播放会话
 * @param frame_info 帧信息（输出参数，包含序号、时间戳、媒体类型等）
 * @param buffer 帧数据缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际读取的字节数，0表示无数据，负数表示读取失败
 */
using RecordFrameReadCallback = std::function<int(const std::string& stream_id,
                                                  NET_RecordFrameInfo_S& frame_info,
                                                  char* buffer,
                                                  size_t buffer_size)>;

/**
 * @brief 录像帧流停止回调函数类型
 * @details 当客户端请求停止录像帧流时调用，由宿主程序实现实际的流停止逻辑
 * @param stream_id 流ID，标识要停止的播放会话
 * @return 错误码，NET_TV_E_SUCCEED表示成功，其他值表示失败
 */
using RecordFrameStopCallback = std::function<NET_TV_COMMON_ECODE_E(const std::string& stream_id)>;

/**
 * @brief 录像帧服务端类
 * @details 负责监听TCP端口，接收客户端连接，将录像帧数据（视频/音频）封装为RTP-like协议格式发送给客户端，
 *          采用单例模式，支持通过回调函数与宿主程序交互
 */
class RecordFrameServer {
public:
    /**
     * @brief 获取单例实例
     * @return 单例指针
     */
    static RecordFrameServer* instance();

    /**
     * @brief 启动服务端
     * @param port 监听端口，默认9007
     * @return true表示成功，false表示失败
     */
    bool start(int port = 9007);

    /**
     * @brief 停止服务端
     */
    void stop();

    /**
     * @brief 获取监听端口
     * @return 监听端口号
     */
    int port() const { return m_port; }

    /**
     * @brief 检查服务端是否正在运行
     * @return true表示正在运行，false表示已停止
     */
    bool is_running() const { return m_running; }

    /**
     * @brief 设置流启动回调函数
     * @param cb 流启动回调函数
     */
    void set_start_callback(RecordFrameStartCallback cb);

    /**
     * @brief 设置帧读取回调函数
     * @param cb 帧读取回调函数
     */
    void set_read_callback(RecordFrameReadCallback cb);

    /**
     * @brief 设置流停止回调函数
     * @param cb 流停止回调函数
     */
    void set_stop_callback(RecordFrameStopCallback cb);

    /**
     * @brief 开启录像帧流
     * @param cond 流启动条件（通道、时间范围、媒体类型等）
     * @param info 流信息（输出参数，包含流ID、端口、媒体类型等）
     * @return 错误码，NET_TV_E_SUCCEED表示成功，其他值表示失败
     */
    NET_TV_COMMON_ECODE_E open_stream(const NET_RecordFrameStreamCond_S& cond,
                                      NET_RecordFrameStreamInfo_S& info);

    /**
     * @brief 关闭录像帧流
     * @param stream_id 流ID，标识要关闭的播放会话
     * @return 错误码，NET_TV_E_SUCCEED表示成功，其他值表示失败
     */
    NET_TV_COMMON_ECODE_E close_stream(const std::string& stream_id);

private:
    /**
     * @brief 构造函数（私有，单例模式）
     */
    RecordFrameServer();

    /**
     * @brief 析构函数（私有，单例模式）
     */
    ~RecordFrameServer();

    /**
     * @brief 客户端连接接收循环
     * @details 持续监听端口，接收客户端连接，管理客户端socket
     */
    void accept_loop();

    /**
     * @brief 客户端数据发送循环
     * @details 持续从回调函数获取录像帧数据，封装为RTP-like协议格式发送给客户端
     * @param client_fd 客户端socket文件描述符
     */
    void client_send_loop(socket_fd_t client_fd);

    /**
     * @brief 发送帧数据包
     * @details 将帧信息和帧数据封装为RTP-like协议格式发送给客户端
     * @param fd socket文件描述符
     * @param frame_info 帧信息（序号、时间戳、媒体类型、数据长度等）
     * @param payload 帧数据指针
     * @return true表示成功，false表示失败
     */
    bool send_packet(socket_fd_t fd, const NET_RecordFrameInfo_S& frame_info, const char* payload);

    /**
     * @brief 获取当前流ID
     * @return 当前流ID字符串
     */
    std::string current_stream_id() const;

    socket_fd_t m_listen_fd{INVALID_SOCKET_FD};     ///< 监听socket文件描述符
    std::atomic<bool> m_running{false};             ///< 运行状态标志
    std::thread m_accept_thread;                    ///< 连接接收线程
    std::thread m_client_thread;                    ///< 客户端发送线程
    mutable std::mutex m_client_mutex;              ///< 客户端socket互斥锁
    socket_fd_t m_client_fd{INVALID_SOCKET_FD};     ///< 当前客户端socket文件描述符
    int m_port{9007};                               ///< 监听端口

    mutable std::mutex m_stream_mutex;              ///< 流信息互斥锁
    std::string m_stream_id;                        ///< 当前流ID
    uint32_t m_ssrc{1};                             ///< RTP-like SSRC标识

    mutable std::mutex m_callback_mutex;            ///< 回调函数互斥锁
    RecordFrameStartCallback m_start_cb;            ///< 流启动回调函数
    RecordFrameReadCallback m_read_cb;              ///< 帧读取回调函数
    RecordFrameStopCallback m_stop_cb;              ///< 流停止回调函数
};

}  // namespace tvsdk
