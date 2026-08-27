/**
 * @file RecordFrameServer.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CRecordFrameServer 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CRecordFrameServer 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
/*
 * @Author       : chenchl
 * @Date         : 2025-01-02 16:01:20
 * @LastEditors  : chenchl
 * @LastEditTime : 2025-01-02 17:03:03
 * @FilePath     : CRecordFrameServer.h
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
 * @author tianl (tianl@kfb.cn)
 * @brief 录像帧流启动回调函数类型
 * @details 当客户端请求启动录像帧流时调用，由宿主程序实现实际的流开启逻辑
 * @param cond 流启动条件（通道、时间范围、媒体类型等）
 * @param info 流信息（输出参数，包含流ID、端口、媒体类型等）
 * @return 错误码，NET_E_SUCCEED表示成功，其他值表示失败
 */
using RecordFrameStartCallback = std::function<NET_COMMON_ECODE_E(const NET_RecordFrameStreamCond_S& cond,
                                                                     NET_RecordFrameStreamInfo_S& info)>;

/**
 * @author tianl (tianl@kfb.cn)
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
 * @author tianl (tianl@kfb.cn)
 * @brief 录像帧流停止回调函数类型
 * @details 当客户端请求停止录像帧流时调用，由宿主程序实现实际的流停止逻辑
 * @param stream_id 流ID，标识要停止的播放会话
 * @return 错误码，NET_E_SUCCEED表示成功，其他值表示失败
 */
using RecordFrameStopCallback = std::function<NET_COMMON_ECODE_E(const std::string& stream_id)>;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 录像帧服务端类
 * @details 负责监听TCP端口，接收客户端连接，将录像帧数据（视频/音频）封装为RTP-like协议格式发送给客户端，
 *          采用单例模式，支持通过回调函数与宿主程序交互
 */
class CRecordFrameServer {
public:
    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取单例实例
     * @return 单例指针
     */
    static CRecordFrameServer* instance();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 启动服务端
     * @param port 监听端口，默认9007
     * @return true表示成功，false表示失败
     */
    bool start(int port = 9007);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 停止服务端
     */
    void stop();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取监听端口
     * @return 监听端口号
     */
    int port() const { return m_nPort; }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 检查服务端是否正在运行
     * @return true表示正在运行，false表示已停止
     */
    bool is_running() const { return m_bRunning; }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置流启动回调函数
     * @param cb 流启动回调函数
     */
    void set_start_callback(RecordFrameStartCallback cb);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置帧读取回调函数
     * @param cb 帧读取回调函数
     */
    void set_read_callback(RecordFrameReadCallback cb);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 设置流停止回调函数
     * @param cb 流停止回调函数
     */
    void set_stop_callback(RecordFrameStopCallback cb);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 开启录像帧流
     * @param cond 流启动条件（通道、时间范围、媒体类型等）
     * @param info 流信息（输出参数，包含流ID、端口、媒体类型等）
     * @return 错误码，NET_E_SUCCEED表示成功，其他值表示失败
     */
    NET_COMMON_ECODE_E open_stream(const NET_RecordFrameStreamCond_S& cond,
                                      NET_RecordFrameStreamInfo_S& info);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 关闭录像帧流
     * @param stream_id 流ID，标识要关闭的播放会话
     * @return 错误码，NET_E_SUCCEED表示成功，其他值表示失败
     */
    NET_COMMON_ECODE_E close_stream(const std::string& stream_id);

private:
    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 构造函数（私有，单例模式）
     */
    CRecordFrameServer();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 析构函数（私有，单例模式）
     */
    ~CRecordFrameServer();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 客户端连接接收循环
     * @details 持续监听端口，接收客户端连接，管理客户端socket
     */
    void accept_loop();

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 客户端数据发送循环
     * @details 持续从回调函数获取录像帧数据，封装为RTP-like协议格式发送给客户端
     * @param client_fd 客户端socket文件描述符
     */
    void client_send_loop(socket_fd_t client_fd);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 发送帧数据包
     * @details 将帧信息和帧数据封装为RTP-like协议格式发送给客户端
     * @param fd socket文件描述符
     * @param frame_info 帧信息（序号、时间戳、媒体类型、数据长度等）
     * @param payload 帧数据指针
     * @return true表示成功，false表示失败
     */
    bool send_packet(socket_fd_t fd, const NET_RecordFrameInfo_S& frame_info, const char* payload);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取当前流ID
     * @return 当前流ID字符串
     */
    std::string current_stream_id() const;

    socket_fd_t m_nListenFd{INVALID_SOCKET_FD};     /*/< 监听socket文件描述符 */
    std::atomic<bool> m_bRunning{false};             /*/< 运行状态标志 */
    std::thread m_stAcceptThread;                    /*/< 连接接收线程 */
    std::thread m_stClientThread;                    /*/< 客户端发送线程 */
    mutable std::mutex m_stClientMutex;              /*/< 客户端socket互斥锁 */
    socket_fd_t m_nClientFd{INVALID_SOCKET_FD};     /*/< 当前客户端socket文件描述符 */
    int m_nPort{9007};                               /*/< 监听端口 */

    mutable std::mutex m_stStreamMutex;              /*/< 流信息互斥锁 */
    std::string m_strStreamId;                        /*/< 当前流ID */
    uint32_t m_uSsrc{1};                             /*/< RTP-like SSRC标识 */

    mutable std::mutex m_stCallbackMutex;            /*/< 回调函数互斥锁 */
    RecordFrameStartCallback m_fnStartCallback;            /*/< 流启动回调函数 */
    RecordFrameReadCallback m_fnReadCallback;              /*/< 帧读取回调函数 */
    RecordFrameStopCallback m_fnStopCallback;              /*/< 流停止回调函数 */
};

}  /* namespace tvsdk */
