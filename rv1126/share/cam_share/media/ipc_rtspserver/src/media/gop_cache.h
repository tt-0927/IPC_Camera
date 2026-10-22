/**
 * @FilePath     : gop_cache.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 18:33:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : GOP cache：每路视频流缓存最近一个从 IDR/CRA 开始的可解码 GOP 快照
 */

/*
 * 设计与不变量（25 号文档 25.4.2/25.4.3/25.4.4）：
 * - 每路流最多保留一个完整 GopStorage（current_gop，shared_ptr 共享）；
 * - PendingFrame 只持有共享 payload 引用，不按客户端复制数据；
 * - 起点 AU 由 Annex-B 扫描结果（IDR/CRA）判定，不信任调用方 key hint；
 * - 任一上限（bytes/frames/duration）或参数集 generation 变化即整段失效，
 *   绝不从 GOP 中间裁剪 P/B 帧；
 * - epoch 在失效与滚动时递增，Reader 借此感知所持快照是否已过期。
 * 线程约定：除 enabled()（原子开关，生产者线程可读）外只在 I/O 线程访问。
 */
#pragma once

#include "ipc_rtsp/config.h"
#include "ipc_rtsp/types.h"
#include "media/codec_config.h"
#include "media/pending_frame.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

/** 与 GOP 绑定的参数集快照（值拷贝，防止参数集后续变化导致混代）。 */
struct CodecConfigSnapshot
{
    /* 码流编码类型。 */
    Codec codec = Codec::Unknown;
    /* 捕获时刻的参数集代次（与 CodecConfigCache::Generation() 同源）；
     * 0 表示值初始化的"未组装快照"，真实缓存代次恒从 1 起。 */
    std::uint32_t generation = 0;
    /* VPS 副本（H.265 用）。 */
    ParameterSet vps;
    /* SPS 副本。 */
    ParameterSet sps;
    /* PPS 副本。 */
    ParameterSet pps;
};

/** 一个完整 GOP 的共享存储。Reader 与 cache 通过 shared_ptr 共享同一份。 */
struct GopStorage
{
    /* 创建时的 cache 代次，Reader 判快照过期用。 */
    std::uint64_t epoch = 0;
    /* 首个 AU 的序列号。 */
    std::uint64_t begin_sequence = 0;
    /* 末个 AU 的序列号。 */
    std::uint64_t end_sequence = 0;
    /* payload 总字节数（字节）。 */
    std::size_t bytes = 0;
    /* 首帧时间轴（us；真实 PTS 或单调钟，估时长用）。 */
    std::int64_t first_monotonic_us = 0;
    /* 末帧时间轴（us；与 first_monotonic_us 同基准，音频回放对齐锚点）。 */
    std::int64_t last_monotonic_us = 0;
    /* 建 GOP 时的参数集快照，storage 生命周期内不变。 */
    CodecConfigSnapshot codec_config;
    /* AU 序列（创建时按 max_frames 预留容量）。 */
    std::vector<PendingFrame> frames;
};

/** 与 GOP 对齐的音频回放段：覆盖 [关键帧 pts, GOP 末帧 pts] 的音频帧。 */
struct AudioReplaySegment
{
    /* 音频帧序列（pts 升序；payload 已拷贝）。 */
    std::vector<AudioPendingFrame> frames;
};

/** attach 时捕获的回放快照：回放范围 [0, replay_end_index)。 */
struct GopSnapshot
{
    /* 历史视频帧存储（memory: shared_ptr 共享拥有；cache 滚动/失效后由
     * 本引用继续保活，直至 Reader 判过期释放）。 */
    std::shared_ptr<const GopStorage> storage;
    /** 与本 GOP 对齐的音频段；无音频/音频未覆盖时为空。音视频回放必须
     * 共用同一快照：两轨起点对齐是播放端音画同步的前提（VLC/live555
     * 的显示时钟在错位的轨之间切换会造成进度时间回跳）。 */
    std::shared_ptr<const AudioReplaySegment> audio;
    /* 视频回放帧数边界（frames 下标，不含）；游标到达即切 live。 */
    std::size_t replay_end_index = 0;
    /* live 衔接边界：历史帧 sequence ≤ 该值。 */
    std::uint64_t replay_end_sequence = 0;
    /* 建 GOP 时的参数集代次。 */
    std::uint32_t codec_generation = 0;
    /* 建 GOP 时的 cache 代次；与 hub 当前 epoch 不等即快照过期。 */
    std::uint64_t epoch = 0;
};

/** GOP 失效原因（用于指标与限频日志）。 */
enum class GopInvalidReason
{
    /* 无失效记录。 */
    None,
    /* 任一上限（bytes/frames/duration）超限。 */
    Capacity,
    /* 参数集 generation 变化。 */
    CodecConfigChanged,
    /* 码流配置热更新。 */
    StreamConfigChanged,
    /* PTS 时间回退。 */
    PtsRollback, /* review: 当前全库无产生点，预留。 */
    /* 服务重启。 */
    ServerRestart, /* review: 当前全库无产生点，预留。 */
};

/** 单帧写入结果。 */
enum class CacheUpdate
{
    /* 未消费（关闭、非关键帧且无 cache、参数集未就绪） */
    Ignored,
    /* 新建 GOP */
    Started,
    /* 追加到当前 GOP */
    Appended,
    /* 新关键 AU 滚动出新一代 GOP */
    Rolled,
    /* 当前 GOP 失效 */
    Invalidated,
};

class GopCache
{
public:
    /** @brief 默认构造：cache 关闭，等待 Configure。@return {void} */
    GopCache() = default;

    /**
     * 应用配置（构造后或热更新时调用）。
     *
     * @param config 三项全 0 表示关闭；配置变化会使当前 GOP 失效
     * @param stream_max_frame_bytes 码流单帧上限，用于单帧超预算告警
     */
    void Configure(const GopCacheConfig &config, std::size_t stream_max_frame_bytes);

    /** cache 是否启用。Configure 在 I/O 线程写，生产者线程经
     * FrameHub::NeedsConfigCapture 并发读，故用原子。 */
    bool enabled() const
    {
        return enabled_.load(std::memory_order_relaxed);
    }

    /** 当前代次（失效与滚动都递增）；Reader 与扇出帧用它判断快照是否过期。
     *  @return 当前代次。 */
    std::uint64_t epoch() const
    {
        return epoch_;
    }

    /**
     * 写入一个 AU（I/O 线程，DrainMedia 扇出前调用）。
     *
     * @param pending        已填好 sequence 与扫描结果（info）的待发帧；
     *                       meta.pts_us 已被 hub 规范为单调时间轴
     * @param codec_config   当前参数集快照（hub 从 CodecConfigCache 组装）
     * @param codec_ready    参数集是否满足当前 codec 的最小集
     */
    CacheUpdate OnFrame(const PendingFrame &pending, const CodecConfigSnapshot &codec_config, bool codec_ready);

    /**
     * 写入一帧音频（I/O 线程，DrainMedia 扇出前调用）。
     *
     * 无真实 PTS 的帧在此就地规范化为入队单调钟（与视频同口径），
     * 之后 live 发送与回放共享同一条时间轴。
     *
     * @param pending 音频待发帧（无 PTS 时被就地补 pts_us/has_pts）
     */
    void OnAudioFrame(AudioPendingFrame &pending);

    /** 当前音频环帧数（指标快照用）。@return 帧数（条）。 */
    std::size_t audio_frames() const
    {
        return audio_ring_.size();
    }

    /**
     * 当前 GOP 可用于回放时输出快照。
     *
     * @param out 输出快照；失败时被置为默认值
     * @return 关闭/未建立返回 false
     */
    bool SnapshotForReader(GopSnapshot *out) const;

    /**
     * @brief 使当前 GOP 整段失效（无 GOP 时幂等）。
     * @param reason 失效原因（计入指标与限频告警日志）
     */
    void Invalidate(GopInvalidReason reason);

    /** 当前 GOP 状态摘要（指标快照用）。@return payload 字节数，无 GOP 为 0。 */
    std::size_t current_bytes() const
    {
        return current_ ? current_->bytes : 0;
    }

    /** @brief 当前 GOP 帧数（指标快照用）。@return AU 数，无 GOP 为 0。 */
    std::size_t current_frames() const
    {
        return current_ ? current_->frames.size() : 0;
    }

    /* ---- 累计指标 ---- */

    /** @return 累计新建 GOP 次数。 */
    std::uint64_t started() const
    {
        return started_;
    }

    /** @return 累计失效次数。 */
    std::uint64_t invalidated() const
    {
        return invalidated_;
    }

    /** @return 最近一次失效原因（从未失效为 None）。 */
    GopInvalidReason last_invalid_reason() const
    {
        return last_invalid_reason_;
    }

private:
    /**
     * @brief 用新关键 AU 建立新一代 GOP（epoch 在此递增）。
     * @param pending 关键 AU 待发帧（成为新 GOP 首帧）
     * @param codec_config 当前参数集快照（绑定进新 storage）
     * @return 恒为 CacheUpdate::Started
     */
    CacheUpdate StartGop(const PendingFrame &pending, const CodecConfigSnapshot &codec_config);

    /** 丢弃音频环里不会再被回放的旧帧（覆盖当前 GOP 跨度即可）。 */
    void TrimAudioRing();

    /* cache 配置（仅 I/O 线程读写）。 */
    GopCacheConfig config_;
    /* 码流单帧上限（字节），用于单帧超 cache 预算的判定。 */
    std::size_t stream_max_frame_bytes_ = 0;
    /** cache 开关快照（Configure 时写入）；生产者线程只读该原子，
     * 不接触 I/O 线程私有的 config_/current_。 */
    std::atomic<bool> enabled_{ false };
    /* 当前 GOP（memory: 与 Reader 经 shared_ptr 共享拥有；置空即整段
     * 失效，残余 Reader 的引用继续保活至其判过期释放）。 */
    std::shared_ptr<GopStorage> current_;
    /* 最近一段音频（us 单调时间轴，时间升序）。视频滚动/失效后按新锚点或时长裁剪，
     * 只需覆盖回放可能引用的 [GOP 关键帧, GOP 末帧] 区间。 */
    std::deque<AudioPendingFrame> audio_ring_;
    /* cache 代次：新建 GOP（StartGop）与失效（Invalidate）时 +1，初始 0；
     * Reader 据此判断所持快照是否过期。 */
    std::uint64_t epoch_ = 0;
    /* 累计新建 GOP 次数。 */
    std::uint64_t started_ = 0;
    /* 累计失效次数。 */
    std::uint64_t invalidated_ = 0;
    /* 最近一次失效原因。 */
    GopInvalidReason last_invalid_reason_ = GopInvalidReason::None;
};

} // namespace detail
} // namespace ipc_rtsp
