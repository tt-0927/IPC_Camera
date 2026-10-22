/**
 * @FilePath     : rtcp_sender.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTCP 发送器：SR(+SDES CNAME) 定时发送与 BYE
 */

/*
 * smolrtsp 只提供 RTCP 序列化，定时器、NTP↔RTP 映射、UDP/TCP 发送通道都在这里实现。
 */
#pragma once

#include "ipc_rtsp/result.h"
#include "rtp/rtp_sender.h"

#include <cstdint>
#include <string>

struct ipc_rtsp_shim_response;
struct ipc_rtsp_shim_writer;

namespace ipc_rtsp
{
namespace detail
{

/** RTCP 发送通道。 */
class RtcpSender
{
public:
    /**
     * @brief 构造未配置的发送通道（Configure 之前发送直接失败/跳过）。
     *
     * @return {void}
     */
    RtcpSender() = default;

    /**
     * @brief 配置发送通道；可重复调用，每次重置 last_sr_ms_/bye_sent_
     *        （重复 SETUP 同一 track 时 SR 周期与 BYE 标志重新开始）。
     *
     * @param rtp 对应的 RTP 发送器（提供 SSRC 与计数），必须比本对象活得久
     * @param tcp true 走 RTSP interleaved，false 走 UDP socket
     * @param channel interleaved RTCP 通道号
     * @param writer TCP 模式下的连接级 writer 句柄（非拥有）
     * @param rtcp_fd UDP 模式下已 connect 的 RTCP socket（不拥有，不负责 close）
     * @param cname SDES CNAME（空则不发 SDES）
     * @return {void}
     */
    void Configure(RtpSender *rtp, bool tcp, std::uint8_t channel, ipc_rtsp_shim_writer *writer, int rtcp_fd, std::string cname);

    /**
     * @brief 到达发送间隔时发送一个 SR（附带 SDES）。
     *
     * @param now_ms 当前时间（单调时钟，毫秒）
     * @param interval_ms SR 发送间隔（毫秒）；首次调用立即发送
     * @return 本次是否实际发送
     */
    bool MaybeSendSr(std::int64_t now_ms, std::uint32_t interval_ms);

    /**
     * @brief 发送 BYE（断开/拆除时调用，只发一次）。
     *
     * @return {void}
     */
    void SendBye();

    /** 已成功发送的 RTCP 包数（compound 包按 1 计）。 */
    std::uint64_t packets_sent() const
    {
        return packets_sent_;
    }

private:
    /**
     * @brief 发送一个 compound RTCP 包（按配置走 TCP interleaved 或 UDP）。
     *
     * @param data 包数据（SR/SDES/BYE 已序列化）
     * @param size 包字节数
     * @return 发送结果；成功时递增 packets_sent_
     */
    Result SendPacket(const std::uint8_t *data, std::size_t size);

    /* 对应的 RTP 发送器，非拥有：提供 SSRC/包计数与时间戳映射，须比本对象活得久。 */
    RtpSender *rtp_ = nullptr;
    /* true 走 RTSP interleaved（writer_），false 走独立 UDP socket（rtcp_fd_）。 */
    bool tcp_ = true;
    /* interleaved RTCP 通道号（仅 TCP 模式使用）。 */
    std::uint8_t channel_ = 1;
    /* 连接级 writer，非拥有；仅 TCP 模式使用，须比本对象活得久。 */
    ipc_rtsp_shim_writer *writer_ = nullptr;
    /* 已 connect 的 RTCP socket fd；不拥有、不负责 close（归属 TrackRuntime::rtcp_fd）。 */
    int rtcp_fd_ = -1;
    /* SDES CNAME；空表示 SR 后不附 SDES。 */
    std::string cname_;

    /* 上次 SR 发送时刻（单调时钟，毫秒）；0 为"尚未发送"哨兵（首个 SR 立即发）。 */
    std::int64_t last_sr_ms_ = 0;
    /* BYE 只发送一次的标志。 */
    bool bye_sent_ = false;
    /* 已成功发送的 RTCP 包数（每个 compound 包计 1）。 */
    std::uint64_t packets_sent_ = 0;
};

} // namespace detail
} // namespace ipc_rtsp
