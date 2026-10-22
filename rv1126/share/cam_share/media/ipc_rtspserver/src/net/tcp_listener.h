/**
 * @FilePath     : tcp_listener.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : TCP 监听器
 */
#pragma once

#include "ipc_rtsp/result.h"
#include "net/event_loop.h"

#include <cstdint>
#include <functional>
#include <string>

namespace ipc_rtsp
{
namespace detail
{

/** 监听器：accept 后把 fd 交给上层建立 Connection。 */
class TcpListener
{
public:
    /** accept 回调（在 I/O 线程执行）；fd 所有权随回调移交上层。 */
    using AcceptCallback = std::function<void(int fd, const std::string &peer_ip, std::uint16_t peer_port)>;

    /** @brief 默认构造。 @return {void} */
    TcpListener() = default;

    /** @brief 析构：停止监听并关闭 fd。 @return {void} */
    ~TcpListener();

    TcpListener(const TcpListener &) = delete;
    TcpListener &operator=(const TcpListener &) = delete;

    /**
     * @brief 绑定并监听；成功后注册到事件循环。
     *
     * @param loop 事件循环（memory: 非拥有，生命周期须覆盖本对象）
     * @param bind_address 绑定 IPv4 文本；空或 "0.0.0.0" 表示 INADDR_ANY
     * @param port 监听端口；0 表示由内核分配（可经 port() 回读）
     * @param callback accept 回调（在 I/O 线程执行）
     * @return 成功返回 Ok；参数非法或 socket/bind/listen/注册失败返回对应错误
     * @note 可重复调用；内部先 Stop() 释放旧监听。
     */
    Result Start(EventLoop *loop, const std::string &bind_address, std::uint16_t port, AcceptCallback callback);

    /** @brief 关闭监听（幂等）；摘除注册 → close(fd) → 置空回调。 @return {void} */
    void Stop();

    /** 实际监听端口（由内核分配时可回读）。 */
    std::uint16_t port() const
    {
        return port_;
    }

private:
    void OnReadable(std::uint32_t events);

    /** 事件循环（memory: 非拥有），Start() 传入。 */
    EventLoop *loop_ = nullptr;
    /** 监听 socket；Start() 申请，Stop()/析构时 close。 */
    int fd_ = -1;
    /** 实际监听端口（port=0 时由内核分配并回读）。 */
    std::uint16_t port_ = 0;
    /** accept 回调；Stop() 时置空，防止监听 fd 已关闭后回调误入。 */
    AcceptCallback callback_;
};

} // namespace detail
} // namespace ipc_rtsp
