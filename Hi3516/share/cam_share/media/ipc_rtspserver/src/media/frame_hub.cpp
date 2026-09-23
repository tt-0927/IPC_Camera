/**
 * @FilePath     : frame_hub.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 码流分发中枢实现
 */

#include "media/frame_hub.h"

#include "media/annexb_scanner.h"
#include "support/log.h"
#include "support/time.h"

#include <algorithm>

namespace ipc_rtsp
{
namespace detail
{

/* ----------------------------- Subscriber ----------------------------- */

Subscriber::Subscriber(std::size_t max_frames, std::size_t max_bytes)
    : max_frames_(max_frames == 0 ? 1u : max_frames), max_bytes_(max_bytes == 0 ? 1u : max_bytes)
{
}

void Subscriber::Activate()
{
    active_.store(true, std::memory_order_relaxed);
    state_ = SubscriberState::WaitStart;
}

void Subscriber::Deactivate()
{
    active_.store(false, std::memory_order_relaxed);
    state_ = SubscriberState::Idle;
    Clear();
}

bool Subscriber::MakeRoom(std::size_t incoming_bytes, bool incoming_key)
{
    while (!pending_.empty() && (pending_.size() >= max_frames_ || bytes_ + incoming_bytes > max_bytes_))
    {
        /* 优先淘汰最老的非关键帧，保证关键帧能被保留到新客户端拿到起点。 */
        auto victim = pending_.end();
        for (auto it = pending_.begin(); it != pending_.end(); ++it)
        {
            if (!it->info.has_key_nal)
            {
                victim = it;
                break;
            }
        }

        if (victim == pending_.end())
        {
            if (!incoming_key)
            {
                /* 队列里全是关键帧且新帧不是关键帧：丢新帧，保留已有起点。 */
                return false;
            }
            /* 新帧是关键帧：淘汰最老的关键帧腾位，避免关键帧把队列顶死。 */
            victim = pending_.begin();
        }

        bytes_ -= victim->bytes;
        pending_.erase(victim);
        ++dropped_frames_;
    }
    return true;
}

bool Subscriber::Enqueue(const PendingFrame &pending)
{
    if (!active_.load(std::memory_order_relaxed))
    {
        return false;
    }

    if (!MakeRoom(pending.bytes, pending.info.has_key_nal))
    {
        ++dropped_frames_;
        return false;
    }

    /* 拥塞态只保留关键帧，非关键帧直接丢弃（freshness > completeness）。 */
    if ((state_ == SubscriberState::Congested || state_ == SubscriberState::Recovering) && !pending.info.has_key_nal)
    {
        ++dropped_frames_;
        return false;
    }

    bytes_ += pending.bytes;
    pending_.push_back(pending);
    return true;
}

bool Subscriber::Pop(PendingFrame *out)
{
    if (pending_.empty())
    {
        return false;
    }
    *out = pending_.front();
    bytes_ -= pending_.front().bytes;
    pending_.pop_front();
    return true;
}

bool Subscriber::EnqueueAudio(const AudioPendingFrame &pending)
{
    if (!active_.load(std::memory_order_relaxed))
    {
        return false;
    }
    /* 音频队列很小：满时丢最老的音频帧，保证最新语音不落后。 */
    constexpr std::size_t kMaxAudioFrames = 4;
    while (pending_audio_.size() >= kMaxAudioFrames)
    {
        pending_audio_.pop_front();
        ++dropped_frames_;
    }
    pending_audio_.push_back(pending);
    return true;
}

bool Subscriber::PopAudio(AudioPendingFrame *out)
{
    if (pending_audio_.empty())
    {
        return false;
    }
    *out = pending_audio_.front();
    pending_audio_.pop_front();
    return true;
}

void Subscriber::Clear()
{
    pending_.clear();
    pending_audio_.clear();
    bytes_ = 0;
}

void Subscriber::ClearVideo()
{
    pending_.clear();
    bytes_ = 0;
}

std::size_t Subscriber::DropAudioBefore(std::int64_t boundary_pts_us)
{
    std::size_t dropped = 0;
    while (!pending_audio_.empty() && pending_audio_.front().pts_us <= boundary_pts_us)
    {
        pending_audio_.pop_front();
        ++dropped;
    }
    return dropped;
}

/* ------------------------------ FrameHub ------------------------------ */

namespace
{
constexpr const char *kTag = "hub";
}

FrameHub::FrameHub(StreamId id, const StreamConfig &config, const BackpressureConfig &backpressure)
    : id_(id), config_(config), backpressure_(backpressure), max_frame_bytes_(config.max_frame_bytes), audio_enabled_(config.audio_enabled),
      max_audio_frame_bytes_(config.max_audio_frame_bytes), codec_config_(config.codec)
{
    gop_cache_.Configure(config.gop_cache, config.max_frame_bytes);
    RefreshCodecSnapshot();
}

bool FrameHub::AttachReader(GopSnapshot *out)
{
    if (!gop_cache_.SnapshotForReader(out))
    {
        return false;
    }
    gop_replays_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void FrameHub::NoteReplayAbort()
{
    const std::uint64_t total = gop_replay_aborts_.fetch_add(1, std::memory_order_relaxed) + 1;
    /* 回放中止意味着播放中的客户端丢掉了待补发的历史（快照过期或拥塞
     * 放弃），属需关注事件；拥塞自身另有 LOGW，这里按 5s 限频兜底。 */
    const std::int64_t now_ms = NowMonotonicMs();
    std::int64_t last = last_replay_abort_warn_ms_;
    if (now_ms - last > 5000 && last_replay_abort_warn_ms_.compare_exchange_strong(last, now_ms))
    {
        IPC_RTSP_LOGW(kTag, "GOP回放中止 通道:%d 累计:%llu", static_cast<int>(id_), static_cast<unsigned long long>(total));
    }
}

void FrameHub::AddSubscriber(Subscriber *subscriber)
{
    if (subscriber == nullptr)
    {
        return;
    }
    if (std::find(subscribers_.begin(), subscribers_.end(), subscriber) != subscribers_.end())
    {
        return;
    }
    subscribers_.push_back(subscriber);
    subscriber_count_.store(subscribers_.size(), std::memory_order_relaxed);
}

void FrameHub::RemoveSubscriber(Subscriber *subscriber)
{
    const auto it = std::find(subscribers_.begin(), subscribers_.end(), subscriber);
    if (it == subscribers_.end())
    {
        return;
    }
    subscribers_.erase(it);
    subscriber_count_.store(subscribers_.size(), std::memory_order_relaxed);
}

bool FrameHub::HasWaitingSubscriber() const
{
    for (const Subscriber *subscriber : subscribers_)
    {
        if (subscriber->active() &&
            (subscriber->state() == SubscriberState::WaitStart || subscriber->state() == SubscriberState::Recovering))
        {
            return true;
        }
    }
    return false;
}

void FrameHub::RefreshCodecSnapshot()
{
    codec_snapshot_.codec = codec_config_.codec();
    codec_snapshot_.generation = codec_config_.Generation();
    codec_snapshot_.vps = codec_config_.vps();
    codec_snapshot_.sps = codec_config_.sps();
    codec_snapshot_.pps = codec_config_.pps();
}

void FrameHub::OnVideoFrame(const SharedVideoFrame &frame, const VideoFrameMeta &meta, const FrameInfo &info)
{
    /* 参数集更新与“无订阅者”无关：DESCRIBE 需要 sprop 参数集，因此即使没有
     * 客户端也要把最初的参数集缓存起来（生产者侧由 NeedsConfigCapture 决定
     * 是否复制这几帧）。 */
    const std::uint32_t generation_before = codec_config_.Generation();
    if (info.has_parameter_set && frame.data != nullptr)
    {
        (void) detail::ForEachNal(config_.codec,
                                  frame.data.get(),
                                  frame.size,
                                  [&](const NalRange &nal)
                                  {
                                      if (IsParameterSet(nal.type))
                                      {
                                          codec_config_.Update(nal.type, nal.data, nal.size);
                                      }
                                      return true;
                                  });
        parameter_set_updates_.fetch_add(1, std::memory_order_relaxed);
    }
    if (codec_config_.Generation() != generation_before)
    {
        /* 参数集内容变化：绑定旧参数集的 GOP 立即失效，防新旧参数集混代。 */
        gop_cache_.Invalidate(GopInvalidReason::CodecConfigChanged);
        RefreshCodecSnapshot();
    }

    if (!config_ready_.load(std::memory_order_relaxed) && codec_config_.Ready())
    {
        config_ready_.store(true, std::memory_order_relaxed);
    }

    PendingFrame pending;
    pending.frame = frame;
    pending.meta = meta;
    pending.info = info;
    pending.bytes = frame.size;
    pending.sequence = next_sequence_++;
    /* 无真实 PTS 时统一用单调钟作为时间轴（估 GOP 时长与 RTP 时间戳兜底同源）。 */
    if (!pending.meta.has_pts)
    {
        pending.meta.pts_us = NowMonotonicUs();
    }

    /* GOP cache 写入在扇出之前：cache 的 epoch 会随本帧滚动/失效，
     * 扇出帧必须携带写入后的最新代次。 */
    (void) gop_cache_.OnFrame(pending, codec_snapshot_, codec_config_.Ready());
    pending.gop_epoch = gop_cache_.epoch();

    if (subscriber_count_.load(std::memory_order_relaxed) == 0)
    {
        return;
    }

    for (Subscriber *subscriber : subscribers_)
    {
        subscriber->Enqueue(pending);
    }

    /* 记录队列高水位（只在 I/O 线程读，用于诊断）。 */
    std::size_t max_frames = 0;
    std::size_t max_bytes = 0;
    for (const Subscriber *subscriber : subscribers_)
    {
        max_frames = std::max(max_frames, subscriber->frames());
        max_bytes = std::max(max_bytes, subscriber->bytes());
    }
    const std::uint64_t observed_frames = queue_high_water_frames_.load(std::memory_order_relaxed);
    if (max_frames > observed_frames)
    {
        queue_high_water_frames_.store(max_frames, std::memory_order_relaxed);
    }
    const std::uint64_t observed_bytes = queue_high_water_bytes_.load(std::memory_order_relaxed);
    if (max_bytes > observed_bytes)
    {
        queue_high_water_bytes_.store(max_bytes, std::memory_order_relaxed);
    }
}

void FrameHub::OnAudioFrame(AudioPendingFrame &pending)
{
    audio_pushed_frames_.fetch_add(1, std::memory_order_relaxed);
    /* cache 写入在扇出之前：音频回放段与视频 GOP 对齐，无订阅者时也必须
     * 持续积累，否则新客户端 attach 时音频段缺失（起播音画起点错位）。 */
    gop_cache_.OnAudioFrame(pending);
    if (subscriber_count_.load(std::memory_order_relaxed) == 0)
    {
        return;
    }
    for (Subscriber *subscriber : subscribers_)
    {
        subscriber->EnqueueAudio(pending);
    }
}

void FrameHub::UpdateConfig(const StreamConfig &config)
{
    if (config.codec != config_.codec)
    {
        /* codec 切换需要重建参数集缓存：清空后等待新参数集。 */
        codec_config_ = CodecConfigCache(config.codec);
        config_ready_.store(false, std::memory_order_relaxed);
        RefreshCodecSnapshot();
    }
    /* 任一配置变化都使 GOP 失效（25.4.5）：RTP 相关口径、单帧上限、cache 预算
     * 都可能影响旧 GOP 的可用性；失效是廉价操作，等下一个关键帧重建即可。 */
    gop_cache_.Configure(config.gop_cache, config.max_frame_bytes);
    gop_cache_.Invalidate(GopInvalidReason::StreamConfigChanged);
    config_ = config;
    max_frame_bytes_.store(config.max_frame_bytes, std::memory_order_release);
    audio_enabled_.store(config.audio_enabled, std::memory_order_release);
    max_audio_frame_bytes_.store(config.max_audio_frame_bytes, std::memory_order_release);
}

void FrameHub::NotePushed(std::size_t bytes)
{
    pushed_frames_.fetch_add(1, std::memory_order_relaxed);
    pushed_bytes_.fetch_add(bytes, std::memory_order_relaxed);
}

void FrameHub::NoteDropped(bool key)
{
    dropped_frames_.fetch_add(1, std::memory_order_relaxed);
    if (key)
    {
        dropped_key_frames_.fetch_add(1, std::memory_order_relaxed);
    }
}

StreamStats FrameHub::Stats() const
{
    StreamStats stats = StatsFromAtomics();
    /* 以下为 I/O 线程私有状态，仅限 I/O 线程内读取（Stats 调用方均为
     * ExecuteControl 任务或测试）。 */
    stats.gop_cache_enabled = gop_cache_.enabled() ? 1u : 0u;
    stats.gop_cache_bytes = gop_cache_.current_bytes();
    stats.gop_cache_frames = gop_cache_.current_frames();
    stats.gop_epoch = gop_cache_.epoch();
    stats.gop_started = gop_cache_.started();
    stats.gop_invalidated = gop_cache_.invalidated();
    return stats;
}

StreamStats FrameHub::StatsFromAtomics() const
{
    StreamStats stats;
    stats.pushed_frames = pushed_frames_.load(std::memory_order_relaxed);
    stats.pushed_bytes = pushed_bytes_.load(std::memory_order_relaxed);
    stats.dropped_frames = dropped_frames_.load(std::memory_order_relaxed);
    stats.dropped_key_frames = dropped_key_frames_.load(std::memory_order_relaxed);
    stats.subscribers = subscriber_count_.load(std::memory_order_relaxed);
    stats.queue_high_water_frames = queue_high_water_frames_.load(std::memory_order_relaxed);
    stats.queue_high_water_bytes = queue_high_water_bytes_.load(std::memory_order_relaxed);
    stats.parameter_set_updates = parameter_set_updates_.load(std::memory_order_relaxed);
    stats.idr_requests = idr_requests_.load(std::memory_order_relaxed);
    stats.audio_pushed_frames = audio_pushed_frames_.load(std::memory_order_relaxed);
    /* gop_cache_enabled 等状态字段保持 0：本接口可能运行在 I/O 线程之外。 */
    stats.gop_replays = gop_replays_.load(std::memory_order_relaxed);
    stats.gop_replay_aborts = gop_replay_aborts_.load(std::memory_order_relaxed);
    return stats;
}

} // namespace detail
} // namespace ipc_rtsp
