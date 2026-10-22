/**
 * @FilePath     : metrics.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 服务级计数器
 */

/* 媒体热路径只做原子自增（relaxed）；快照由低频诊断路径读取，允许近似值。 */
#pragma once

#include "ipc_rtsp/status.h"

#include <atomic>
#include <cstdint>

namespace ipc_rtsp
{
namespace detail
{

/** 服务级计数器集合（热路径 relaxed 自增，跨线程读为近似值）。 */
struct ServerCounters
{
    /** 已接受并登记的连接累计。 */
    std::atomic<std::uint64_t> connections_accepted{ 0 };
    /** 因总连接数或单 IP 上限被拒绝的连接累计（fd 直接关闭）。 */
    std::atomic<std::uint64_t> connections_rejected{ 0 };
    /** 自 TCP 输出缓冲成功写往 socket 的累计字节数（UDP 直发不经此缓冲，不计入）。 */
    std::atomic<std::uint64_t> tx_bytes{ 0 };
    /** 已成功发送的 RTP 包累计（分片每包计 1；GOP 重放参数集按 +3 近似计数）。 */
    std::atomic<std::uint64_t> rtp_packets{ 0 };
    /** 已收到的客户端 interleaved RTCP 包累计（RR 等，作会话活动统计）。 */
    std::atomic<std::uint64_t> rtcp_packets{ 0 };
    /** 鉴权失败次数累计（每次校验不过即计）。 */
    std::atomic<std::uint64_t> auth_failures{ 0 };
    /** RTSP 请求解析失败/请求超长累计（连接层，回应 400 并断开）。 */
    std::atomic<std::uint64_t> parse_failures{ 0 };
    /** 拥塞/积压超时主动断开的连接累计。 */
    std::atomic<std::uint64_t> congestion_disconnects{ 0 };
    /** 生产者入队阶段丢弃的媒体帧数（单位：帧；邮箱满/超上限/分配失败）。 */
    std::atomic<std::uint64_t> ingest_dropped{ 0 };
    /**
     * 媒体帧在发送路径被拒绝（如 JPEG 帧不符合 RFC 2435 能力）。与
     * parse_failures 的口径对照：后者是连接层 RTSP 请求文本解析失败，本
     * 字段是已入队的媒体数据在打包前被拒，不影响连接存活。
     */
    std::atomic<std::uint64_t> media_parse_rejects{ 0 };

    /**
     * 全部计数清零。
     *
     * @return {void}
     * @note 原子 store 无锁；与热路径自增并发时允许近似（当前无调用方，
     *       保留给诊断/测试路径）。
     */
    void Reset()
    {
        connections_accepted.store(0, std::memory_order_relaxed);
        connections_rejected.store(0, std::memory_order_relaxed);
        tx_bytes.store(0, std::memory_order_relaxed);
        rtp_packets.store(0, std::memory_order_relaxed);
        rtcp_packets.store(0, std::memory_order_relaxed);
        auth_failures.store(0, std::memory_order_relaxed);
        parse_failures.store(0, std::memory_order_relaxed);
        congestion_disconnects.store(0, std::memory_order_relaxed);
        ingest_dropped.store(0, std::memory_order_relaxed);
        media_parse_rejects.store(0, std::memory_order_relaxed);
    }
};

} // namespace detail
} // namespace ipc_rtsp
