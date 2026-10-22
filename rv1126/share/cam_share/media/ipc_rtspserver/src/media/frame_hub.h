/**
 * @FilePath     : frame_hub.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 码流分发中枢：帧扇出、每订阅者待发队列与首帧/拥塞状态机
 */

/*
 * 职责边界：
 * - 生产者线程（VENC/AENC）只调用 Push*，把帧放进有界邮箱；
 * - I/O 线程负责把邮箱里的帧扇出到各订阅者的有界待发队列，并驱动发送。
 * 资源红线（见 21_资源预算与背压设计.md）：
 * - 队列同时限制条数与字节数，满时丢最老的非关键帧；
 * - 慢客户端只影响自己的待发队列与输出缓冲，不影响生产者与其他客户端；
 * - 一个客户端不会为同一帧复制多份 payload（共享同一 shared_ptr）。
 */
#pragma once

#include "ipc_rtsp/config.h"
#include "ipc_rtsp/frame.h"
#include "ipc_rtsp/status.h"
#include "ipc_rtsp/types.h"
#include "media/codec_config.h"
#include "media/gop_cache.h"
#include "media/pending_frame.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

/** 订阅者（= 一个客户端的一个 track）状态。 */
enum class SubscriberState
{
    /* 未 PLAY */
    Idle = 0,
    /* 等待可解码起点（参数集 + 关键帧） */
    WaitStart,
    /* 正常发送 */
    Streaming,
    /* 输出积压，只保留关键帧 */
    Congested,
    /* 积压缓解，等待下一个关键帧恢复 */
    Recovering,
};

/**
 * 单客户端（单 track）的接收端。
 *
 * 线程约定：除 `active_` 外只在 I/O 线程访问。
 */
class Subscriber
{
public:
    /**
     * @brief 构造订阅者。
     * @param max_frames 视频待发队列帧数上限（条；0 时按 1 处理）
     * @param max_bytes 视频待发队列字节上限（字节；0 时按 1 处理）
     */
    Subscriber(std::size_t max_frames, std::size_t max_bytes);

    /** PLAY：进入等待起始关键帧状态。 */
    void Activate();

    /** PAUSE/TEARDOWN：停止接收并清空待发队列。 */
    void Deactivate();

    /** @brief 是否处于 PLAY 激活态。@return 激活返回 true（原子读，可跨线程）。 */
    bool active() const
    {
        return active_.load(std::memory_order_relaxed);
    }

    /** @brief 当前发送状态。@return 状态枚举值。 */
    SubscriberState state() const
    {
        return state_;
    }

    /**
     * @brief 设置发送状态（仅 I/O 线程，由拥塞状态机驱动）。
     * @param state 新状态
     */
    void SetState(SubscriberState state)
    {
        state_ = state;
    }

    /**
     * 尝试把一帧放入待发队列。
     *
     * @param pending 待发帧（只复制共享引用与元数据）
     * @return true 表示已入队；false 表示按策略丢弃
     */
    bool Enqueue(const PendingFrame &pending);

    /**
     * @brief 取出最早一帧。
     * @param[out] out 成功时写入队首帧
     * @return 队列为空返回 false
     */
    bool Pop(PendingFrame *out);

    /** @brief 视频待发队列是否为空。@return 为空返回 1，否则 0。 */
    std::size_t Empty() const
    {
        return pending_.empty();
    }

    /** @brief 当前视频待发帧数。@return 帧数（条）。 */
    std::size_t frames() const
    {
        return pending_.size();
    }

    /** @brief 当前视频待发字节总数。@return 字节数。 */
    std::size_t bytes() const
    {
        return bytes_;
    }

    /** @brief 累计丢弃帧数。@return 腾位淘汰 + 入队拒绝 + 音频挤占之和。 */
    std::uint64_t dropped_frames() const
    {
        return dropped_frames_;
    }

    /**
     * @brief 入队一帧音频；队列满（4 帧）时丢最老的音频帧。
     * @param pending 音频待发帧（payload 内联拷贝）
     * @return 未激活返回 false；其余情况均入队成功
     */
    bool EnqueueAudio(const AudioPendingFrame &pending);

    /**
     * @brief 取出最早的一帧音频。
     * @param[out] out 成功时写入队首帧
     * @return 队列为空返回 false
     */
    bool PopAudio(AudioPendingFrame *out);

    /**
     * 丢弃 pts ≤ 边界的 live 音频帧（Reader attach 后回放段将覆盖
     * [GOP 关键帧, 边界] 区间，避免回放与 live 重复/乱序）。
     *
     * @param boundary_pts_us 边界时间戳（us，单调时间轴）
     * @return 丢弃的帧数
     */
    std::size_t DropAudioBefore(std::int64_t boundary_pts_us);

    /** @brief 当前音频待发帧数。@return 帧数（条）。 */
    std::size_t audio_frames() const
    {
        return pending_audio_.size();
    }

    /** 清空视频与音频待发队列并把字节计数归零；PendingFrame 只持有
     * payload 的共享引用，清空仅释放本订阅者的引用，不影响其他持有者。 */
    void Clear();

    /** 只清空视频待发队列（Reader attach 时丢弃回放范围外的旧帧，音频不受影响）。 */
    void ClearVideo();

private:
    /**
     * @brief 为自己的待发队列腾出空间：优先淘汰最老的非关键帧。
     * @param incoming_bytes 新帧字节数
     * @param incoming_key 新帧是否含关键 NAL
     * @return 队列可容纳新帧返回 true；新帧按策略被丢弃返回 false
     */
    bool MakeRoom(std::size_t incoming_bytes, bool incoming_key);

    /* 视频待发队列（仅 I/O 线程访问）。 */
    std::deque<PendingFrame> pending_;
    /* 音频待发队列，上限 4 帧（EnqueueAudio 满时丢最老）。 */
    std::deque<AudioPendingFrame> pending_audio_;
    /* 视频队列帧数上限（条）。 */
    std::size_t max_frames_;
    /* 视频队列字节上限（字节）。 */
    std::size_t max_bytes_;
    /* 当前待发字节总数（字节，随入队/出队/淘汰增减）。 */
    std::size_t bytes_ = 0;
    /* 累计丢弃帧数（条；腾位淘汰 + 入队拒绝 + 音频挤占）。 */
    std::uint64_t dropped_frames_ = 0;
    /* 发送状态机；不变量：active_ 为 false 时必须处于 Idle。 */
    SubscriberState state_ = SubscriberState::Idle;
    /* PLAY 激活标志：本类唯一可跨线程访问的成员（生产者线程读）。 */
    std::atomic<bool> active_{ false };
};

/**
 * 一路码流的分发中枢。
 *
 * 生产者线程只读 `SubscriberCount()`（原子），其余状态只在 I/O 线程访问。
 */
class FrameHub
{
public:
    /**
     * @brief 构造分发中枢并应用初始配置。
     * @param id 码流通道 ID
     * @param config 码流配置
     * @param backpressure 每客户端背压参数
     */
    FrameHub(StreamId id, const StreamConfig &config, const BackpressureConfig &backpressure);

    /** @brief 所属码流通道 ID。@return 通道 ID。 */
    StreamId id() const
    {
        return id_;
    }

    /** @brief 当前生效的码流配置。@return 配置常引用（仅 I/O 线程读写）。 */
    const StreamConfig &config() const
    {
        return config_;
    }

    /** 生产者线程可读的帧上限快照。@return 单帧字节上限（字节）。 */
    std::size_t max_frame_bytes() const
    {
        return max_frame_bytes_.load(std::memory_order_acquire);
    }

    /** 生产者线程可读的音频配置快照。@return 音频是否启用。 */
    bool audio_enabled() const
    {
        return audio_enabled_.load(std::memory_order_acquire);
    }

    /** @brief 生产者线程可读的音频单帧上限快照。@return 音频单帧字节上限（字节）。 */
    std::size_t max_audio_frame_bytes() const
    {
        return max_audio_frame_bytes_.load(std::memory_order_acquire);
    }

    /** 参数集缓存（I/O 线程访问）。@return 缓存引用。 */
    CodecConfigCache &codec_config()
    {
        return codec_config_;
    }

    /**
     * 尝试为回放捕获当前 GOP 快照（PLAY/拥塞恢复时调用）。
     *
     * @param out 输出快照；失败时被置为默认值
     * @return 有可回放 GOP 时返回 true；cache 关闭、尚未建立 GOP 或
     *         参数集未就绪时返回 false，调用方回退 WaitStart + 限频 IDR。
     */
    bool AttachReader(GopSnapshot *out);

    /** 回放中止计数（快照过期或拥塞放弃，I/O 线程调用；带限频告警）。 */
    void NoteReplayAbort();

    /** 当前 cache 代次（扇出帧携带，Reader 判过期用）。@return 代次。 */
    std::uint64_t gop_epoch() const
    {
        return gop_cache_.epoch();
    }

    /** GOP cache 只读访问（指标与测试用）。@return cache 常引用。 */
    const GopCache &gop_cache() const
    {
        return gop_cache_;
    }

    /**
     * 注册订阅者（I/O 线程）。
     *
     * @param subscriber 非拥有指针（memory: 由调用方 track 的 unique_ptr
     *                   持有）；hub 仅保存指针、不参与其生命周期。重复注册
     *                   同一指针为幂等空操作。
     */
    void AddSubscriber(Subscriber *subscriber);

    /**
     * 注销订阅者（I/O 线程）。
     *
     * @param subscriber 非拥有指针；未注册时为幂等空操作。调用方须在
     *                   销毁/重置订阅者对象前先注销，避免 hub 悬垂指针。
     */
    void RemoveSubscriber(Subscriber *subscriber);

    /** 当前活跃订阅者数量（生产者线程可读，用于决定是否复制 payload）。
     *  @return 订阅者数量。 */
    std::size_t SubscriberCount() const
    {
        return subscriber_count_.load(std::memory_order_relaxed);
    }

    /**
     * 是否还需要在无订阅者时复制帧。
     *
     * 参数集缓存尚未就绪时，生产者必须继续送帧，否则 DESCRIBE 无法生成
     * 带 sprop 参数集的 SDP；GOP cache 开启时同样需要持续送帧，否则
     * cache 停在旧快照，新客户端 PLAY 命中的是过期数据。
     *
     * @return 需要无订阅者也持续送帧返回 true
     */
    bool NeedsConfigCapture() const
    {
        return !config_ready_.load(std::memory_order_relaxed) || gop_cache_.enabled();
    }

    /**
     * 音频帧是否需要无订阅者也持续入 hub：GOP cache 的音频回放段必须
     * 与视频 GOP 同时积累，停供会让新客户端 attach 时音频段缺失。
     *
     * @return GOP cache 开启时返回 true
     */
    bool NeedsAudioIngest() const
    {
        return gop_cache_.enabled();
    }

    /** 是否已有订阅者等待媒体（用于 IDR 请求决策）。@return 存在
     *  WaitStart/Recovering 状态的活跃订阅者返回 true。 */
    bool HasWaitingSubscriber() const;

    /**
     * 扇出一帧（I/O 线程）。
     *
     * @param frame   共享帧
     * @param meta    元数据
     * @param info    本帧分析结果
     */
    void OnVideoFrame(const SharedVideoFrame &frame, const VideoFrameMeta &meta, const FrameInfo &info);

    /**
     * @brief 扇出一帧音频（I/O 线程）。
     * @param pending 音频待发帧（先写入 GOP cache 音频环再扇出；
     *                无 PTS 时会被 GopCache::OnAudioFrame 就地补单调钟）
     */
    void OnAudioFrame(AudioPendingFrame &pending);

    /** 背压配置（用于创建订阅者）。@return 配置常引用。 */
    const BackpressureConfig &backpressure() const
    {
        return backpressure_;
    }

    /**
     * 更新码流配置（控制面热更新）。
     *
     * 新配置作用于后续新建的 track；已在播放的订阅关系不会被强制中断。
     *
     * @param config 新码流配置
     */
    void UpdateConfig(const StreamConfig &config);

    /**
     * @brief 累计入队统计（生产者线程调用，仅计数）。
     * @param bytes 本次入队字节数
     */
    void NotePushed(std::size_t bytes);

    /**
     * @brief 累计丢帧统计（生产者线程调用，仅计数）。
     * @param key 丢弃的是否为关键帧
     */
    void NoteDropped(bool key);

    /** @brief 完整统计快照（含 GOP cache 状态，仅 I/O 线程调用）。
     *  @return 统计值。 */
    StreamStats Stats() const;

    /**
     * 仅原子计数的安全统计：供控制线程超时降级路径（ServerImpl::Snapshot
     * 降级）在 I/O 线程外调用。GOP cache 的状态字段（bytes/frames/epoch 等）
     * 由 I/O 线程私有且非原子，跨线程读取在 ARM32 上可撕裂，此接口一律填 0。
     *
     * @return 仅含原子计数与 GOP 回放计数的统计快照
     */
    StreamStats StatsFromAtomics() const;

private:
    /* 所属码流通道 ID。 */
    StreamId id_;
    /* 当前生效码流配置（仅 I/O 线程读写）。 */
    StreamConfig config_;
    /* 每客户端背压参数（创建订阅者时读取）。 */
    BackpressureConfig backpressure_;
    /* 生产者线程可读的单帧字节上限快照（字节；release/acquire 传递）。 */
    std::atomic<std::size_t> max_frame_bytes_{ 0 };
    /* 生产者线程可读的音频启用快照。 */
    std::atomic<bool> audio_enabled_{ false };
    /* 生产者线程可读的音频单帧字节上限快照（字节）。 */
    std::atomic<std::size_t> max_audio_frame_bytes_{ 0 };
    /* 参数集缓存（仅 I/O 线程）。 */
    CodecConfigCache codec_config_;
    /* 本路 GOP cache（仅 I/O 线程；enabled() 原子开关例外，见其说明）。 */
    GopCache gop_cache_;
    /** AU 序列号分配（I/O 线程单调递增，从 1 开始）。 */
    std::uint64_t next_sequence_ = 1;
    /** 参数集快照缓存：仅在 generation 变化时重建，每帧传引用给 GopCache。 */
    CodecConfigSnapshot codec_snapshot_;
    /* 已注册订阅者（memory: 非拥有，由各 track 的 unique_ptr 持有；
     * 仅 I/O 线程访问，销毁前调用方须先 RemoveSubscriber）。 */
    std::vector<Subscriber *> subscribers_;
    /* 订阅者数量快照（生产者线程可读）。 */
    std::atomic<std::size_t> subscriber_count_{ 0 };
    /** 参数集缓存是否已就绪（I/O 线程写，生产者线程读）。 */
    std::atomic<bool> config_ready_{ false };

    /* 统计：生产者与 I/O 线程都会写，用原子计数避免加锁。 */
    std::atomic<std::uint64_t> pushed_frames_{ 0 };
    std::atomic<std::uint64_t> pushed_bytes_{ 0 };
    std::atomic<std::uint64_t> dropped_frames_{ 0 };
    std::atomic<std::uint64_t> dropped_key_frames_{ 0 };
    std::atomic<std::uint64_t> queue_high_water_frames_{ 0 };
    std::atomic<std::uint64_t> queue_high_water_bytes_{ 0 };
    std::atomic<std::uint64_t> parameter_set_updates_{ 0 };
    std::atomic<std::uint64_t> idr_requests_{ 0 };
    std::atomic<std::uint64_t> audio_pushed_frames_{ 0 };
    std::atomic<std::uint64_t> gop_replays_{ 0 };
    std::atomic<std::uint64_t> gop_replay_aborts_{ 0 };
    /** 回放中止告警的限频时间戳（仅 I/O 线程访问）。 */
    std::atomic<std::int64_t> last_replay_abort_warn_ms_{ 0 };

    /** 从 codec_config_ 重建参数集快照（构造与 generation 变化时调用）。 */
    void RefreshCodecSnapshot();
};

} // namespace detail
} // namespace ipc_rtsp
