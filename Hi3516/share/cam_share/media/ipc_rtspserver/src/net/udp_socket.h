/**
 * @FilePath     : udp_socket.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : UDP socket 与端口获取
 */

/*
 * RTP/RTCP over UDP 使用两个已 connect 的非阻塞 socket，fd 交由调用方的
 * track/sender 持有并负责 close。端口由内核分配（getsockname 回读），不自建
 * 固定端口池，把资源约束落在 fd 数量与 track 数量上限上（见 21 文档）。
 */
#pragma once

#include "ipc_rtsp/result.h"

#include <cstdint>
#include <string>

namespace ipc_rtsp
{
namespace detail
{

/** UDP socket 工具。 */
class UdpSocket
{
public:
    /**
     * @brief 创建已 connect 到对端地址的 UDP socket（非阻塞 + CLOEXEC）。
     *
     * @param ip 对端 IPv4 文本（空视为非法）
     * @param port 对端端口（0 视为非法）
     * @param dscp DSCP 标记值（0-63）；-1 表示禁用标记
     * @param[out] fd 成功时写入新 socket（memory: 调用方负责 close）
     * @param[out] local_port 本端端口（内核分配；getsockname 失败时写 0；可为 nullptr）
     * @return 成功返回 Ok；参数非法或系统调用失败返回对应错误
     */
    static Result CreateConnected(const std::string &ip, std::uint16_t port, int dscp, int *fd, std::uint16_t *local_port);
};

} // namespace detail
} // namespace ipc_rtsp
