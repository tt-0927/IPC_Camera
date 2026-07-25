/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : RecordFrameClient.h
 * @Description  : 录像帧客户端实现，负责与设备建立TCP连接，接收录像帧数据（视频/音频）
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "NetTVSDKCommon.h"
#include "PlatformCompat.h"

namespace tvsdk {

/**
 * @brief 录像帧回调函数类型
 * @details 当接收到设备端发送的录像帧时调用，包含帧信息和帧数据
 * @param frame_info 帧信息结构体（序号、时间戳、媒体类型、数据长度等）
 * @param data 帧数据指针
 * @param size 帧数据大小（字节）
 */
using RecordFrameCallback = std::function<void(const NET_RecordFrameInfo_S& frame_info,
                                               const char* data,
                                               size_t size)>;

/**
 * @brief 录像帧客户端类
 * @details 负责与设备建立TCP连接，接收录像帧数据（视频/音频），解析RTP-like协议帧头，
 *          将帧数据通过回调函数转发给上层
 */
class RecordFrameClient {
public:
    /**
     * @brief 构造函数
     */
    RecordFrameClient();

    /**
     * @brief 析构函数
     * @details 自动停止接收并释放资源
     */
    ~RecordFrameClient();

    /**
     * @brief 连接设备录像帧端口并启动接收
     * @param host 设备IP地址
     * @param port 设备录像帧端口号
     * @param stream_id 流ID，标识当前播放会话
     * @param callback 帧数据回调函数，用于接收设备发送的录像帧数据
     * @return true表示成功，false表示失败
     */
    bool start(const std::string& host,
               int port,
               const std::string& stream_id,
               RecordFrameCallback callback);

    /**
     * @brief 停止并关闭连接
     */
    void stop();

    /**
     * @brief 检查录像帧接收是否正在运行
     * @return true表示正在运行，false表示已停止
     */
    bool is_running() const { return m_running; }

    /**
     * @brief 获取当前流ID
     * @return 流ID字符串
     */
    const std::string& stream_id() const { return m_stream_id; }

private:
    /**
     * @brief 接收数据循环
     * @details 持续从socket读取录像帧，解析RTP-like帧头后通过回调转发给上层
     */
    void recv_loop();

    socket_fd_t m_socket{INVALID_SOCKET_FD};      ///< TCP socket文件描述符
    std::atomic<bool> m_running{false};           ///< 运行状态标志
    std::string m_stream_id;                      ///< 当前流ID
    RecordFrameCallback m_callback;               ///< 帧数据回调函数
    std::thread m_recv_thread;                    ///< 接收线程
    std::mutex m_stop_mutex;                      ///< 停止操作互斥锁
};

}  // namespace tvsdk
