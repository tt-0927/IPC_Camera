/**
 * @FilePath     : pending_frame.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 18:33:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 媒体待发帧与帧分析结果的公共定义
 */

/*
 * 从 frame_hub.h 抽出：GopCache 与 FrameHub 都依赖这些类型，
 * 独立成头避免互相 include。
 */
#pragma once

#include "ipc_rtsp/frame.h"
#include "ipc_rtsp/types.h"

#include <array>
#include <cstddef>

namespace ipc_rtsp
{
namespace detail
{

/** 一帧的分析结果（在 I/O 线程内扫描一次得到，供扇出与打包复用）。 */
struct FrameInfo
{
    /* 含 IDR/CRA（MJPEG 每帧为 true）。 */
    bool has_key_nal = false;
    /* 含 VPS/SPS/PPS。 */
    bool has_parameter_set = false;
    /* VCL NAL 数量，用于 AU 边界判定。 */
    std::size_t vcl_count = 0;
};

/** 待发帧：只持有共享引用与元数据，不复制 payload。 */
struct PendingFrame
{
    /* 共享帧（payload 引用，零拷贝扇出）。 */
    SharedVideoFrame frame;
    /* 帧元数据（pts_us 单位 us；无真实 PTS 时 hub 已规范为单调钟）。 */
    VideoFrameMeta meta;
    /* 扫描分析结果。 */
    FrameInfo info;
    /* 帧总字节数（字节，等于 frame.size；队列字节预算按此计）。 */
    std::size_t bytes = 0;
    /** FrameHub 分配的单调 AU 序列号（从 1 开始；0 表示未分配）。 */
    std::uint64_t sequence = 0;
    /** 扇出时刻的 cache 代次；Reader 借此判定所持快照是否过期。 */
    std::uint64_t gop_epoch = 0;
};

/** 音频待发帧（payload 内联，避免每帧分配）。 */
struct AudioPendingFrame
{
    /* 音频编码类型。 */
    AudioCodec codec = AudioCodec::None;
    /* 有效 payload 字节数（字节）。 */
    std::size_t size = 0;
    /* 时间戳（us；与视频同基准——真实 PTS 或入队单调钟）。 */
    std::int64_t pts_us = 0;
    /* pts_us 是否有效。 */
    bool has_pts = false;
    /* 内联 payload 缓冲；容量须与 media_mailbox.h 的 kAudioSlotBytes（4096）保持一致。 */
    std::array<std::uint8_t, 4096> data{};
};

} // namespace detail
} // namespace ipc_rtsp
