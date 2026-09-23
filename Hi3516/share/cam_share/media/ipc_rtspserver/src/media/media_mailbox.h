/**
 * @FilePath     : media_mailbox.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 生产者线程 → I/O 线程的有界媒体邮箱
 */

/*
 * 目的：VENC/AENC 线程只做"入槽 + 唤醒"，不做任何 socket 操作；
 * 槽位数量与音频槽位大小都是编译期常量，稳态不产生 per-frame 动态分配。
 * 满时策略：直接丢弃新帧并返回 false，由调用方计数（freshness > completeness）。
 */
#pragma once

#include "ipc_rtsp/frame.h"
#include "ipc_rtsp/types.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace ipc_rtsp
{
namespace detail
{

/** 单个音频槽容量：G.711/AAC 单帧远小于该值。 */
constexpr std::size_t kAudioSlotBytes = 4096;

/** 邮箱中的视频帧。 */
struct VideoMailboxItem
{
    /* 共享帧（payload 引用计数，不拷贝数据）。 */
    SharedVideoFrame frame;
    /* 帧元数据（pts_us 单位 us；has_pts 为 false 时由 hub 规范为单调钟）。 */
    VideoFrameMeta meta;
    /* 目标码流通道。 */
    StreamId stream = StreamId::Main;
};

/** 邮箱中的音频帧（payload 内联，避免额外分配）。 */
struct AudioMailboxItem
{
    /* 来源码流通道。 */
    StreamId stream = StreamId::Main;
    /* 音频编码类型。 */
    AudioCodec codec = AudioCodec::None;
    /* 有效 payload 字节数（字节，≤ kAudioSlotBytes）。 */
    std::size_t size = 0;
    /* 采集时间戳（us；基准由采集侧决定，has_pts 为 false 时无效）。 */
    std::int64_t pts_us = 0;
    /* pts_us 是否有效。 */
    bool has_pts = false;
    /* 内联 payload 缓冲（容量 kAudioSlotBytes 字节）。 */
    std::array<std::uint8_t, kAudioSlotBytes> data{};
};

/**
 * 多生产者（VENC/AENC 推流线程）+ 单消费者（I/O 线程）的有界环形邮箱。
 */
class MediaMailbox
{
public:
    /** 视频槽数量：覆盖两路码流各若干帧的突发。 */
    static constexpr std::size_t kVideoSlots = 16;
    /** 音频槽数量。 */
    static constexpr std::size_t kAudioSlots = 16;

    /**
     * @brief 推入一帧视频；邮箱满时丢弃新帧（由调用方计数）。
     * @param frame 共享帧（移动接入槽位）
     * @param meta 帧元数据
     * @param stream 目标码流通道
     * @return 入队成功返回 true；视频槽已满返回 false
     */
    bool PushVideo(SharedVideoFrame &&frame, const VideoFrameMeta &meta, StreamId stream)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (video_count_ >= kVideoSlots)
        {
            return false;
        }
        VideoMailboxItem &slot = video_[(video_head_ + video_count_) % kVideoSlots];
        slot.frame = std::move(frame);
        slot.meta = meta;
        slot.stream = stream;
        ++video_count_;
        return true;
    }

    /**
     * @brief 推入一帧音频；邮箱满或超长时丢弃新帧。
     * @param stream 目标码流通道
     * @param frame 音频帧视图（payload 拷贝入槽）
     * @return 入队成功返回 true；空帧、超过 kAudioSlotBytes 或音频槽已满返回 false
     */
    bool PushAudio(StreamId stream, const AudioFrameView &frame)
    {
        if (frame.data == nullptr || frame.size == 0 || frame.size > kAudioSlotBytes)
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (audio_count_ >= kAudioSlots)
        {
            return false;
        }
        AudioMailboxItem &slot = audio_[(audio_head_ + audio_count_) % kAudioSlots];
        slot.stream = stream;
        slot.codec = frame.codec;
        slot.size = frame.size;
        slot.pts_us = frame.pts_us;
        slot.has_pts = frame.has_pts;
        std::memcpy(slot.data.data(), frame.data, frame.size);
        ++audio_count_;
        return true;
    }

    /**
     * @brief 取出最早的一帧视频。
     * @param[out] out 成功时接收帧（移动语义）
     * @return 邮箱为空返回 false
     */
    bool PopVideo(VideoMailboxItem *out)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (video_count_ == 0)
        {
            return false;
        }
        *out = std::move(video_[video_head_]);
        video_[video_head_].frame = SharedVideoFrame();
        video_head_ = (video_head_ + 1) % kVideoSlots;
        --video_count_;
        return true;
    }

    /**
     * @brief 取出最早的一帧音频。
     * @param[out] out 成功时接收帧
     * @return 邮箱为空返回 false
     */
    bool PopAudio(AudioMailboxItem *out)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (audio_count_ == 0)
        {
            return false;
        }
        *out = audio_[audio_head_];
        audio_head_ = (audio_head_ + 1) % kAudioSlots;
        --audio_count_;
        return true;
    }

    /** 是否还有待处理数据（用于决定唤醒）。@return 视频与音频队列均空返回 true。 */
    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return video_count_ == 0 && audio_count_ == 0;
    }

    /** 释放所有待处理帧，用于关停和同实例重启隔离。 */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (VideoMailboxItem &slot : video_)
        {
            slot = VideoMailboxItem();
        }
        for (AudioMailboxItem &slot : audio_)
        {
            slot = AudioMailboxItem();
        }
        video_head_ = 0;
        video_count_ = 0;
        audio_head_ = 0;
        audio_count_ = 0;
    }

private:
    /* lock: 保护下方 video_/audio_ 槽位数组与 head/count 游标；
     * Push/Pop/Empty/Clear 全部持锁访问。 */
    mutable std::mutex mutex_;
    /* 视频环形槽位数组（容量 kVideoSlots）。 */
    std::array<VideoMailboxItem, kVideoSlots> video_{};
    /* 视频队首下标；head+count 联合寻址：队尾槽 = (video_head_ + video_count_) % kVideoSlots。 */
    std::size_t video_head_ = 0;
    /* 当前视频帧数（条）；不变量：video_count_ ≤ kVideoSlots。 */
    std::size_t video_count_ = 0;
    /* 音频环形槽位数组（容量 kAudioSlots）。 */
    std::array<AudioMailboxItem, kAudioSlots> audio_{};
    /* 音频队首下标；head+count 联合寻址：队尾槽 = (audio_head_ + audio_count_) % kAudioSlots。 */
    std::size_t audio_head_ = 0;
    /* 当前音频帧数（条）；不变量：audio_count_ ≤ kAudioSlots。 */
    std::size_t audio_count_ = 0;
};

} // namespace detail
} // namespace ipc_rtsp
