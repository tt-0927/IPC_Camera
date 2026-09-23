/**
 * @FilePath     : gop_cache.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 18:33:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 11:35:00
 * @Description  : GOP cache 实现
 */

#include "media/gop_cache.h"

#include "support/log.h"
#include "support/time.h"

#include <algorithm>
#include <cstdio>

namespace ipc_rtsp
{
namespace detail
{

namespace
{

constexpr const char *kTag = "gop";

/** 实现硬上限（25.4.2）：配置校验负责拦截非法值，这里只钳制突破硬上限的配置。 */
constexpr std::uint32_t kMaxDurationHardMs = 5000;
constexpr std::size_t kMaxFramesHard = 300;

/**
 * @brief 失效原因转日志用短描述。
 * @param reason 失效原因
 * @return 中文短描述；None/未知值返回 "none"
 */
const char *InvalidReasonToString(GopInvalidReason reason)
{
    switch (reason)
    {
    case GopInvalidReason::Capacity:
        return "容量超限";
    case GopInvalidReason::CodecConfigChanged:
        return "参数集变化";
    case GopInvalidReason::StreamConfigChanged:
        return "配置变化";
    case GopInvalidReason::PtsRollback:
        return "时间回退";
    case GopInvalidReason::ServerRestart:
        return "服务重启";
    case GopInvalidReason::None:
        break;
    }
    return "none";
}

} // namespace

void GopCache::Configure(const GopCacheConfig &config, std::size_t stream_max_frame_bytes)
{
    GopCacheConfig effective = config;
    effective.max_duration_ms = std::min(effective.max_duration_ms, kMaxDurationHardMs);
    effective.max_frames = std::min(effective.max_frames, kMaxFramesHard);

    const bool config_changed = effective.max_bytes != config_.max_bytes || effective.max_frames != config_.max_frames ||
                                effective.max_duration_ms != config_.max_duration_ms;
    stream_max_frame_bytes_ = stream_max_frame_bytes;

    if (config_changed)
    {
        /* 配置变化使当前 GOP 失效：max_frames 预留与时长刻度都变了，
         * 继续沿用旧 GOP 可能与新配置的预算口径冲突。 */
        if (current_)
        {
            Invalidate(GopInvalidReason::StreamConfigChanged);
        }
        config_ = effective;
    }
    else
    {
        config_ = effective;
    }
    /* 开关单独维护为原子：生产者线程每帧经 NeedsConfigCapture 读取，
     * 不得与 I/O 线程的 Configure 写构成数据竞争。 */
    enabled_.store(config_.max_bytes != 0, std::memory_order_relaxed);

    if (enabled() && stream_max_frame_bytes_ > config_.max_bytes)
    {
        /* 单帧超过 cache 预算：实时发送不受影响，该帧不入 cache。预算告警由
         * 服务层统一打印（启动 Configure 与热更新 UpdateStreamConfig 共用
         * WarnGopBudgetIfUnderflow），这里无需重复。 */
    }
}

CacheUpdate GopCache::StartGop(const PendingFrame &pending, const CodecConfigSnapshot &codec_config)
{
    auto gop = std::make_shared<GopStorage>();
    epoch_ += 1;
    gop->epoch = epoch_;
    gop->begin_sequence = pending.sequence;
    gop->end_sequence = pending.sequence;
    gop->bytes = pending.bytes;
    gop->first_monotonic_us = pending.meta.pts_us;
    gop->last_monotonic_us = pending.meta.pts_us;
    gop->codec_config = codec_config;
    gop->frames.reserve(config_.max_frames);
    gop->frames.push_back(pending);
    current_ = std::move(gop);
    /* 新 GOP 立即生效：按新起点裁音频环，旧 GOP 区间的音频不再保留。 */
    TrimAudioRing();
    ++started_;
    return CacheUpdate::Started;
}

CacheUpdate GopCache::OnFrame(const PendingFrame &pending, const CodecConfigSnapshot &codec_config, bool codec_ready)
{
#if 0 /* GOP 状态逐帧跟踪（p1d 排查 VLC 时间回跳时临时启用过，日常关闭防刷屏；                                 \
         需要时整段放开，前 40 帧输出 key/参数集/GOP 装载状态）。 */
    {
        static int dbg = 0;
        if (dbg++ < 40)
        {
            std::fprintf(stderr,
                         "[dbg] seq=%llu key=%d ready=%d gen=%u cur_gen=%u cur_frames=%zu pts=%lld first=%lld dur=%lld\n",
                         (unsigned long long)pending.sequence,
                         pending.info.has_key_nal ? 1 : 0,
                         codec_ready ? 1 : 0,
                         codec_config.generation,
                         current_ ? current_->codec_config.generation : 0,
                         current_ ? current_->frames.size() : 0,
                         (long long)pending.meta.pts_us,
                         current_ ? (long long)current_->first_monotonic_us : -1LL,
                         current_ ? (long long)(pending.meta.pts_us - current_->first_monotonic_us) : -1LL);
        }
    }
#endif
    if (!enabled())
    {
        return CacheUpdate::Ignored;
    }

    const bool is_key = pending.info.has_key_nal;

    /* 参数集代次变化：旧 GOP 绑定的参数集已过期，整段失效防混代。 */
    if (current_ && current_->codec_config.generation != codec_config.generation)
    {
        Invalidate(GopInvalidReason::CodecConfigChanged);
        if (!is_key)
        {
            /* 失效由本帧触发且本帧不能作为新起点，向调用方报失效。 */
            return CacheUpdate::Invalidated;
        }
        /* 关键帧继续往下按新参数集建立新 GOP。 */
    }

    if (!current_)
    {
        /* EMPTY/INVALID：普通 AU 忽略；关键 AU 且参数集就绪才建立新 GOP。 */
        if (!is_key || !codec_ready)
        {
            return CacheUpdate::Ignored;
        }
        return StartGop(pending, codec_config);
    }

    if (is_key)
    {
        /* 新关键 AU：滚动出新一代 GOP。老 storage 由残余 Reader 短暂保活，
         * Reader 在下一轮发送事件按 epoch 判过期后中止并释放。 */
        const CacheUpdate update = StartGop(pending, codec_config);
        return update == CacheUpdate::Started ? CacheUpdate::Rolled : update;
    }

    /* append 前预检查三重上限；任一超限整段失效，绝不从 GOP 中间裁剪。 */
    const std::int64_t duration_us = pending.meta.pts_us - current_->first_monotonic_us;
    if (current_->bytes + pending.bytes > config_.max_bytes || current_->frames.size() >= config_.max_frames ||
        duration_us > static_cast<std::int64_t>(config_.max_duration_ms) * 1000)
    {
        Invalidate(GopInvalidReason::Capacity);
        return CacheUpdate::Invalidated;
    }

    current_->end_sequence = pending.sequence;
    current_->last_monotonic_us = pending.meta.pts_us;
    current_->bytes += pending.bytes;
    current_->frames.push_back(pending);
    return CacheUpdate::Appended;
}

void GopCache::OnAudioFrame(AudioPendingFrame &pending)
{
    if (!enabled())
    {
        return;
    }

    /* 无真实 PTS 的音频与视频同口径：入队时刻的单调钟。live 发送与
     * 回放从此共享同一条时间轴，发送侧不得再各自现取时钟。 */
    if (!pending.has_pts)
    {
        pending.pts_us = NowMonotonicUs();
        pending.has_pts = true;
    }

    audio_ring_.push_back(pending);
    TrimAudioRing();
}

void GopCache::TrimAudioRing()
{
    if (audio_ring_.empty())
    {
        return;
    }

    /* 环只需要覆盖回放可能引用的区间：有视频 GOP 时从其关键帧前 50ms
     * 起保留（对齐锚点 + 容忍帧间抖动）；无 GOP 时按上限时长滚动。
     * 帧数硬兜底防时钟异常导致的无限堆积。 */
    const std::int64_t cutoff_us = current_ ? current_->first_monotonic_us - 50000
                                            : audio_ring_.back().pts_us - static_cast<std::int64_t>(config_.max_duration_ms) * 1000;
    while (!audio_ring_.empty() && audio_ring_.front().pts_us < cutoff_us)
    {
        audio_ring_.pop_front();
    }
    constexpr std::size_t kAudioRingHardFrames = 600; /* ≈38s@64ms，防御性上限 */
    while (audio_ring_.size() > kAudioRingHardFrames)
    {
        audio_ring_.pop_front();
    }
}

bool GopCache::SnapshotForReader(GopSnapshot *out) const
{
    if (out == nullptr)
    {
        return false;
    }
    *out = GopSnapshot{};
    if (!enabled() || !current_)
    {
        return false;
    }
    GopSnapshot snapshot;
    snapshot.storage = current_;
    snapshot.replay_end_index = current_->frames.size();
    snapshot.replay_end_sequence = current_->end_sequence;
    snapshot.codec_generation = current_->codec_config.generation;
    snapshot.epoch = current_->epoch;

    /* 音频段与视频 GOP 对齐：只保留 [关键帧 pts, ∞) 的音频，与视频回放
     * 共享同一起点。段拷贝一次进快照，Reader 端零共享可变状态。 */
    if (!audio_ring_.empty())
    {
        auto segment = std::make_shared<AudioReplaySegment>();
        for (const AudioPendingFrame &frame : audio_ring_)
        {
            if (frame.pts_us >= current_->first_monotonic_us)
            {
                segment->frames.push_back(frame);
            }
        }
        if (!segment->frames.empty())
        {
            snapshot.audio = std::move(segment);
        }
    }
    *out = std::move(snapshot);
    return true;
}

void GopCache::Invalidate(GopInvalidReason reason)
{
    if (!current_)
    {
        return;
    }
    /* 失效是需关注事件：capacity 表示 GOP 超出预算（低帧率长 GOP 或预算
     * 配小了），codec-config/stream-config 属配置变更的正常失效。幂等保护
     * 已防重复打印；重建后再次超限会在下个 GOP 周期再告警一次。 */
    IPC_RTSP_LOGW(kTag,
                  "GOP缓存失效 原因:%s 代次:%llu 帧:%zu 字节:%zu",
                  InvalidReasonToString(reason),
                  static_cast<unsigned long long>(epoch_),
                  current_->frames.size(),
                  current_->bytes);
    current_.reset();
    epoch_ += 1;
    ++invalidated_;
    last_invalid_reason_ = reason;
    /* 失效后无 GOP 锚点：音频环退化为按上限时长滚动。 */
    TrimAudioRing();
}

} // namespace detail
} // namespace ipc_rtsp
