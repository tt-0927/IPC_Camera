/**
 * @FilePath     : status.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 运行状态与指标快照
 */
/*
 * 设计原则：媒体热路径只累加计数器，不做字符串格式化；诊断读取低频快照。
 */
#pragma once

#include "ipc_rtsp/types.h"

#include <cstdint>

namespace ipc_rtsp
{

/** 单码流统计。 */
struct StreamStats
{
    std::uint64_t pushed_frames = 0;      /**< 收到的视频帧总数 */
    std::uint64_t pushed_bytes = 0;       /**< 收到的视频字节数 */
    std::uint64_t dropped_frames = 0;     /**< 因队列上限丢弃的帧数 */
    std::uint64_t dropped_key_frames = 0; /**< 被丢弃的关键帧数（正常应为 0） */
    std::uint64_t subscribers = 0;        /**< 当前订阅者数（PLAYING） */
    /* 队列帧数高水位：进程生命周期内的历史峰值（各订阅者间取最大，不回落）。 */
    std::uint64_t queue_high_water_frames = 0;
    /* 队列字节数高水位：进程生命周期内的历史峰值（同上）。 */
    std::uint64_t queue_high_water_bytes = 0;
    std::uint64_t parameter_set_updates = 0; /**< 参数集更新次数 */
    std::uint64_t idr_requests = 0;          /**< 触发的 IDR 请求次数 */
    std::uint64_t audio_pushed_frames = 0;   /**< 进入发送链路的音频帧数 */

    /* ---- GOP cache（25 号文档 25.4.8） ---- */
    std::uint64_t gop_cache_enabled = 0; /**< cache 是否启用（1/0） */
    std::uint64_t gop_cache_bytes = 0;   /**< 当前 GOP payload 字节数 */
    std::uint64_t gop_cache_frames = 0;  /**< 当前 GOP 缓存的 AU 数 */
    std::uint64_t gop_epoch = 0;         /**< 当前 GOP 代次（失效/滚动时递增） */
    std::uint64_t gop_started = 0;       /**< 累计新建 GOP 次数 */
    std::uint64_t gop_invalidated = 0;   /**< 累计失效次数（不含正常滚动） */
    std::uint64_t gop_replays = 0;       /**< 累计 Reader attach（回放起播）次数 */
    std::uint64_t gop_replay_aborts = 0; /**< 累计回放中止（epoch 过期/拥塞）次数 */
};

/** 单客户端统计。 */
struct ClientStats
{
    /* 累计发送的 RTP 包数。 */
    std::uint64_t rtp_packets = 0;
    /* 累计发送的媒体字节数（按帧 payload 计，不含 RTP/传输头）。 */
    std::uint64_t rtp_bytes = 0;
    /* 累计发送的 RTCP 包数（SR 等）。 */
    std::uint64_t rtcp_packets = 0;
    std::uint64_t dropped_frames = 0; /**< 拥塞期间丢掉的帧 */
    /* TCP 输出缓冲字节数高水位：历史峰值。 */
    std::uint64_t output_high_water_bytes = 0;
    /* 累计拥塞时长（毫秒）：每次退出拥塞态累加一次，非瞬时值。 */
    std::uint64_t congested_ms = 0;
    /* 断开原因；连接仍存活时为 Unknown。 */
    DisconnectReason disconnect_reason = DisconnectReason::Unknown;
};

/** 全局指标快照。 */
struct MetricsSnapshot
{
    /* 累计接受的 TCP 连接数。 */
    std::uint64_t connections_accepted = 0;
    /* 当前存活连接数（快照时刻）。 */
    std::uint64_t connections_active = 0;
    std::uint64_t connections_rejected = 0; /**< 超过连接上限被拒 */
    /* 当前已建立会话（已分配 session id）的连接数。 */
    std::uint64_t sessions_active = 0;
    /* 当前至少一个 track 处于 PLAYING 的连接数。 */
    std::uint64_t playing_clients = 0;
    /* 当前 PLAYING 的 TCP interleaved track 数。 */
    std::uint64_t tcp_clients = 0;
    /* 当前 PLAYING 的 UDP track 数。 */
    std::uint64_t udp_clients = 0;
    /* 累计经 socket 实际写出的字节数。 */
    std::uint64_t tx_bytes = 0;
    /* 累计发送的 RTP 包数（全服务汇总）。 */
    std::uint64_t rtp_packets = 0;
    /* 累计发送的 RTCP 包数（全服务汇总）。 */
    std::uint64_t rtcp_packets = 0;
    /* 累计认证失败次数。 */
    std::uint64_t auth_failures = 0;
    /* 累计请求解析失败/超上限次数（均回 400 并断开）。 */
    std::uint64_t parse_failures = 0;
    /* 累计因输出拥塞断开的连接数。 */
    std::uint64_t congestion_disconnects = 0;

    /* 各码流统计，下标经 StreamIndex(StreamId) 映射。 */
    StreamStats streams[kStreamCount];
};

} // namespace ipc_rtsp
