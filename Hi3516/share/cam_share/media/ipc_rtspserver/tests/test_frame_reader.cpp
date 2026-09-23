/**
 * @FilePath     : test_frame_reader.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 18:33:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : FrameReader 测试：attach 边界、回放顺序、Stale 判定与 live 衔接边界
 */

#include "media/frame_reader.h"

#include "test_support.h"

#include <memory>

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

namespace
{

/** 构造待回放帧。
 *
 * @param size 帧字节数。
 * @param key 是否关键帧。
 * @param sequence 帧序号。
 * @return 已填充元信息与序号的 PendingFrame。
 */
PendingFrame MakePending(std::size_t size, bool key, std::uint64_t sequence)
{
    PendingFrame pending;
    pending.frame.data = std::shared_ptr<const std::uint8_t>(new std::uint8_t[size], std::default_delete<std::uint8_t[]>());
    pending.frame.size = size;
    pending.meta.codec = Codec::H264;
    pending.meta.key = key;
    pending.info.has_key_nal = key;
    pending.bytes = size;
    pending.sequence = sequence;
    return pending;
}

/** 构造一个包含 3 帧（key + P + P）的快照，回放边界在末尾。
 *
 * @param epoch 快照所属的 cache 代次（默认 7，用于与 Reader 侧当前代次比对）。
 * @return 带 storage 与回放边界的 GopSnapshot。
 */
GopSnapshot MakeSnapshot(std::uint64_t epoch = 7)
{
    auto storage = std::make_shared<GopStorage>();
    storage->epoch = epoch;
    storage->begin_sequence = 10;
    storage->end_sequence = 12;
    storage->bytes = 300;
    storage->frames.push_back(MakePending(100, true, 10));
    storage->frames.push_back(MakePending(100, false, 11));
    storage->frames.push_back(MakePending(100, false, 12));

    GopSnapshot snapshot;
    snapshot.storage = storage;
    snapshot.replay_end_index = storage->frames.size();
    snapshot.replay_end_sequence = storage->end_sequence;
    snapshot.epoch = epoch;
    return snapshot;
}

} // namespace

RTSP_TEST_CASE(frame_reader_unattached_reports_finished)
{
    /* 场景：未 attach 的 Reader；预期一切 Peek 都报 ReplayFinished，空快照拒绝 attach。 */
    FrameReader reader;
    const PendingFrame *frame = nullptr;
    RTSP_CHECK(reader.Peek(1, &frame) == ReaderNext::ReplayFinished);
    RTSP_CHECK(!reader.replay_pending());
    RTSP_CHECK(!reader.Attach(GopSnapshot{})); /* 空 storage 拒绝 attach */
}

RTSP_TEST_CASE(frame_reader_replays_frames_in_order_from_key)
{
    FrameReader reader;
    RTSP_CHECK(reader.Attach(MakeSnapshot()));
    RTSP_CHECK(reader.replay_pending());

    /* 回放必须从关键 AU 开始。 */
    const PendingFrame *frame = nullptr;
    RTSP_CHECK(reader.Peek(7, &frame) == ReaderNext::FrameReady);
    RTSP_CHECK(frame->info.has_key_nal);
    RTSP_CHECK_EQ(frame->sequence, static_cast<std::uint64_t>(10));

    reader.Consume();
    RTSP_CHECK(reader.Peek(7, &frame) == ReaderNext::FrameReady);
    RTSP_CHECK_EQ(frame->sequence, static_cast<std::uint64_t>(11));
    reader.Consume();
    RTSP_CHECK(reader.Peek(7, &frame) == ReaderNext::FrameReady);
    RTSP_CHECK_EQ(frame->sequence, static_cast<std::uint64_t>(12));
    reader.Consume();

    RTSP_CHECK(reader.Peek(7, &frame) == ReaderNext::ReplayFinished);
    RTSP_CHECK(!reader.replay_pending());
}

RTSP_TEST_CASE(frame_reader_reports_stale_when_epoch_moves)
{
    FrameReader reader;
    RTSP_CHECK(reader.Attach(MakeSnapshot(7)));

    const PendingFrame *frame = nullptr;
    RTSP_CHECK(reader.Peek(7, &frame) == ReaderNext::FrameReady);

    /* cache 已滚动/失效（epoch 变化）：旧快照必须立即判过期。 */
    RTSP_CHECK(reader.Peek(8, &frame) == ReaderNext::Stale);
    RTSP_CHECK(reader.IsStale(8));
    RTSP_CHECK(!reader.IsStale(7));

    /* Stale 不推进游标，Detach 后回到未 attach 语义。 */
    reader.Detach();
    RTSP_CHECK(reader.Peek(8, &frame) == ReaderNext::ReplayFinished);
}

RTSP_TEST_CASE(frame_reader_live_boundary_is_captured_at_attach)
{
    const GopSnapshot snapshot = MakeSnapshot(3);
    FrameReader reader;
    RTSP_CHECK(reader.Attach(snapshot));

    /* live 衔接基准：回放完成后第一个 live 帧 sequence 必须严格大于该值。 */
    RTSP_CHECK_EQ(reader.replay_end_sequence(), static_cast<std::uint64_t>(12));
    RTSP_CHECK_EQ(snapshot.replay_end_index, static_cast<std::size_t>(3));
}

RTSP_TEST_CASE(frame_reader_partial_replay_keeps_shared_storage_alive)
{
    GopSnapshot snapshot = MakeSnapshot(1);
    FrameReader reader;
    RTSP_CHECK(reader.Attach(snapshot));
    snapshot = GopSnapshot{}; /* 调用方引用释放后，Reader 仍持有 storage */

    const PendingFrame *frame = nullptr;
    RTSP_CHECK(reader.Peek(1, &frame) == ReaderNext::FrameReady);
    RTSP_CHECK_EQ(frame->sequence, static_cast<std::uint64_t>(10));
    reader.Consume();
    RTSP_CHECK(reader.replay_pending()); /* 还有 2 帧 */
}

RTSP_TEST_CASE(frame_reader_audio_cursor_replays_aligned_segment)
{
    GopSnapshot snapshot = MakeSnapshot();
    auto audio = std::make_shared<AudioReplaySegment>();
    AudioPendingFrame frame;
    frame.codec = AudioCodec::AAC;
    frame.size = 32;
    frame.pts_us = 100;
    frame.has_pts = true;
    audio->frames.push_back(frame);
    AudioPendingFrame second = frame;
    second.pts_us = 164;
    audio->frames.push_back(second);
    snapshot.audio = audio;

    FrameReader reader;
    RTSP_CHECK(reader.Attach(snapshot));

    /* 音频游标独立推进，两帧依序返回后完成。 */
    const AudioPendingFrame *out = nullptr;
    RTSP_CHECK(reader.PeekAudio(7, &out) == ReaderNext::FrameReady);
    RTSP_CHECK(out != nullptr && out->pts_us == 100);
    reader.ConsumeAudio();
    RTSP_CHECK(reader.PeekAudio(7, &out) == ReaderNext::FrameReady);
    RTSP_CHECK(out != nullptr && out->pts_us == 164);
    reader.ConsumeAudio();
    RTSP_CHECK(reader.PeekAudio(7, &out) == ReaderNext::ReplayFinished);

    /* 视频游标不受音频消费影响。 */
    const PendingFrame *video = nullptr;
    RTSP_CHECK(reader.Peek(7, &video) == ReaderNext::FrameReady);
}

RTSP_TEST_CASE(frame_reader_audio_stale_and_absent_segment)
{
    FrameReader reader;
    GopSnapshot snapshot = MakeSnapshot(7);
    RTSP_CHECK(reader.Attach(snapshot));

    /* 快照无音频段：恒 ReplayFinished。 */
    const AudioPendingFrame *out = nullptr;
    RTSP_CHECK(reader.PeekAudio(7, &out) == ReaderNext::ReplayFinished);

    /* epoch 落后于 cache 当前代次：Stale 优先于有无音频段的判定。 */
    auto audio = std::make_shared<AudioReplaySegment>();
    AudioPendingFrame frame;
    frame.pts_us = 100;
    frame.has_pts = true;
    audio->frames.push_back(frame);
    GopSnapshot updated = MakeSnapshot(8);
    updated.audio = audio;
    RTSP_CHECK(reader.Attach(updated));
    RTSP_CHECK(reader.PeekAudio(9, &out) == ReaderNext::Stale);
}
