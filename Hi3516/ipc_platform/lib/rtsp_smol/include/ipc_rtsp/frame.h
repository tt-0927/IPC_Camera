/**
 * @FilePath     : frame.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 媒体帧输入视图
 */
/*
 * 输入契约（与现有产品 Live_Stream_Info_t / SharedMediaFrame_S 对齐）：
 * - 一个 VENC pack = 一个完整 Access Unit；库内按 Annex-B 起始码扫描 NAL 序列，
 *   first_nal 只描述该 pack 的第一个 NAL；
 * - VideoFrameView 的 data 只在本次 Push 调用期间有效：库在有订阅者时复制一次，
 *   无订阅者时不复制；
 * - SharedVideoFrame 通过引用计数共享，库内不再复制 payload；
 * - 同一 AU 的所有 NAL 共享同一个 timestamp，只有最后一个 NAL 的 is_au_end 为 true
 *   （由库内扫描结果决定，调用方无需提供）。
 */
#pragma once

#include "ipc_rtsp/types.h"

#include <cstdint>
#include <cstddef>
#include <memory>

namespace ipc_rtsp
{

/**
 * 只读视频帧视图。
 *
 * 生命周期：仅在 PushVideo 调用期间有效。
 */
struct VideoFrameView
{
    /* memory: 非拥有指针；仅在本次 PushVideo 调用期间有效。 */
    const std::uint8_t *data = nullptr;
    /* 帧字节数。 */
    std::size_t size = 0;
    /* 视频编码格式。 */
    Codec codec = Codec::Unknown;
    /* 本帧（pack）第一个 NAL 的类型。 */
    NalUnitType first_nal = NalUnitType::Unknown;
    /** 采集时间戳（微秒）；has_pts=false 时库用系统单调时钟补齐。 */
    std::int64_t pts_us = 0;
    /* pts_us 是否有效。 */
    bool has_pts = false;
    /** 是否可独立解码（IDR/CRA/MJPEG 整帧）；调用方已知时可置位，库内仍会校验。 */
    bool key = false;
};

/** 共享视频帧（引用计数 buffer）。 */
struct SharedVideoFrame
{
    /* memory: 共享拥有（引用计数）；库内有订阅者时共享这一份 payload，不再复制。 */
    std::shared_ptr<const std::uint8_t[]> data;
    /* 帧字节数。 */
    std::size_t size = 0;
};

/** 共享帧的附加元数据。 */
struct VideoFrameMeta
{
    /* 视频编码格式。 */
    Codec codec = Codec::Unknown;
    /* 本帧（pack）第一个 NAL 的类型。 */
    NalUnitType first_nal = NalUnitType::Unknown;
    /* 采集时间戳（微秒）。 */
    std::int64_t pts_us = 0;
    /* pts_us 是否有效。 */
    bool has_pts = false;
    /* 是否可独立解码（IDR/CRA/MJPEG 整帧）。 */
    bool key = false;
};

/** 只读音频帧视图（仅在 PushAudio 调用期间有效）。 */
struct AudioFrameView
{
    /* memory: 非拥有指针；仅在本次 PushAudio 调用期间有效。 */
    const std::uint8_t *data = nullptr;
    /* 帧字节数。 */
    std::size_t size = 0;
    /* 音频编码格式。 */
    AudioCodec codec = AudioCodec::None;
    /* 采集时间戳（微秒）；has_pts=false 时库用系统单调时钟补齐。 */
    std::int64_t pts_us = 0;
    /* pts_us 是否有效。 */
    bool has_pts = false;
};

} // namespace ipc_rtsp
