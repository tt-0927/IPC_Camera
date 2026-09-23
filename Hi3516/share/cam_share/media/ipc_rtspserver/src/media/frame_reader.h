/**
 * @FilePath     : frame_reader.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 18:33:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : FrameReader：共享 GOP 快照上的回放游标（每视频 track 一个）
 */

/*
 * Reader 只持有 GopStorage 的 shared_ptr 与游标，不复制 payload；Peek() 返回
 * 的指针只在当前 I/O 事件处理期间有效（cache 滚动后旧 storage 由 Reader 独立
 * 保活，但属过期数据，由 epoch 判定后中止）。不变量（25 号文档 25.4.4）：
 * - 历史帧 sequence ≤ replay_end_sequence；live 队列只接收更大的 sequence；
 * - 回放必须从关键 AU（frames[0]）开始，之后才是 GOP 内的 P/B AU；
 * - 每轮事件只推进到 RTP sender 允许的位置，不一次性序列化完整 GOP。
 * 线程约定：只在 I/O 线程访问。
 */
#pragma once

#include "media/gop_cache.h"
#include "media/pending_frame.h"

#include <cstddef>
#include <cstdint>

namespace ipc_rtsp
{
namespace detail
{

/** Peek 结果。 */
enum class ReaderNext
{
    /* *out 指向待回放帧，发送成功后调 Consume() */
    FrameReady,
    /* 历史回放完毕（或未 attach），切 live 队列 */
    ReplayFinished,
    /* 快照已过期（cache 已滚动/失效），应中止并重新 attach */
    Stale,
};

class FrameReader
{
public:
    /**
     * 捕获快照并重置游标。调用方须在同一线程序列里清空 live 队列中
     * sequence ≤ replay_end_sequence 的旧帧，防止回放与 live 重复。
     *
     * @param snapshot 待 attach 的回放快照（拷贝共享引用保活历史帧）
     * @return 快照无视频存储时返回 false（attach 失败，游标保持复位）
     */
    bool Attach(const GopSnapshot &snapshot);

    /** 释放快照引用；detach 后 Peek 恒返回 ReplayFinished。 */
    void Detach();

    /** @brief 是否已持有快照。@return attach 成功且未 detach 返回 true。 */
    bool attached() const
    {
        return static_cast<bool>(snapshot_.storage);
    }

    /** 该 Reader 回放的快照是否已落后于 cache 当前代次。@return 已 attach
     *  且快照 epoch 不等于 current_epoch 返回 true。 */
    bool IsStale(std::uint64_t current_epoch) const
    {
        return attached() && snapshot_.epoch != current_epoch;
    }

    /**
     * 查看当前待回放帧。
     *
     * @param current_epoch hub 当前 cache 代次（判过期）
     * @param out 成功时指向快照内的帧；仅在下次 Consume/Detach 前有效
     * @return FrameReady/ReplayFinished/Stale，含义见 ReaderNext
     */
    ReaderNext Peek(std::uint64_t current_epoch, const PendingFrame **out) const;

    /** 游标前进一帧（RTP 序列化完成后才允许调用）。 */
    void Consume();

    /**
     * 音频回放游标：与视频游标共用同一快照（同一 epoch、同一起点），
     * Peek/Consume 语义与视频侧一致。快照无音频段时返回 ReplayFinished。
     *
     * @param current_epoch hub 当前 cache 代次（判过期）
     * @param out 成功时指向快照内的音频帧；仅在下次 ConsumeAudio/Detach 前有效
     * @return FrameReady/ReplayFinished/Stale，含义见 ReaderNext
     */
    ReaderNext PeekAudio(std::uint64_t current_epoch, const AudioPendingFrame **out) const;

    /** 音频游标前进一帧（音频 RTP 序列化完成后才允许调用）。 */
    void ConsumeAudio();

    /** 已回放完成的首个 live sequence 边界（衔接去重用）。@return 边界 sequence。 */
    std::uint64_t replay_end_sequence() const
    {
        return snapshot_.replay_end_sequence;
    }

    /** 回放是否尚未完成：attached 且游标未走完 replay 区间时为 true；
     *  未 attach 或已走完为 false。 */
    bool replay_pending() const
    {
        return attached() && cursor_ < snapshot_.replay_end_index;
    }

    /** @brief 所持快照。@return 快照常引用（未 attach 时为默认构造空快照）。 */
    const GopSnapshot &snapshot() const
    {
        return snapshot_;
    }

private:
    /* attach 时拷贝的快照（memory: storage/audio 为 shared_ptr 共享拥有，
     * 持有期间保证 cache 滚动/失效后历史帧与音频段仍存活）。 */
    GopSnapshot snapshot_;
    /* 视频回放游标（frames 下标，∈ [0, replay_end_index]）。 */
    std::size_t cursor_ = 0;
    /* 音频回放游标（audio->frames 下标）。 */
    std::size_t audio_cursor_ = 0;
};

} // namespace detail
} // namespace ipc_rtsp
