/**
 * @FilePath     : test_gop_cache.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 18:33:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : GOP cache 测试：key 起点、滚动、三重上限、generation 失效与单 epoch 保留
 */

#include "media/gop_cache.h"

#include "test_support.h"

#include <cstring>
#include <memory>

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

namespace
{

/** 构造 GOP cache 配置。
 *
 * @param max_bytes 字节上限（默认 64 KiB）。
 * @param max_frames 帧数上限（默认 8）。
 * @param max_duration_ms 时长上限毫秒（默认 5000）。
 * @return 三重上限已填好的配置。
 */
GopCacheConfig MakeCacheConfig(std::size_t max_bytes = 64u * 1024u, std::size_t max_frames = 8, std::uint32_t max_duration_ms = 5000)
{
    GopCacheConfig config;
    config.max_bytes = max_bytes;
    config.max_frames = max_frames;
    config.max_duration_ms = max_duration_ms;
    return config;
}

/** 构造 H.264 参数集快照。
 *
 * @param generation 参数集代次（默认 1）。
 * @return 含最小 SPS/PPS 的参数集快照。
 */
CodecConfigSnapshot MakeCodecSnapshot(std::uint32_t generation = 1)
{
    CodecConfigSnapshot snapshot;
    snapshot.codec = Codec::H264;
    snapshot.generation = generation;
    snapshot.sps.size = 4;
    snapshot.sps.data = { 0x67, 0x42, 0xE0, 0x1E };
    snapshot.pps.size = 2;
    snapshot.pps.data = { 0x68, 0xCE };
    return snapshot;
}

/** 构造待入队帧。
 *
 * @param size 帧字节数。
 * @param key 是否关键帧。
 * @param sequence 帧序号。
 * @param pts_us 帧时间戳（微秒）。
 * @return 已填充元信息、序号与时间戳的 PendingFrame。
 */
PendingFrame MakePending(std::size_t size, bool key, std::uint64_t sequence, std::int64_t pts_us)
{
    PendingFrame pending;
    pending.frame.data = std::shared_ptr<const std::uint8_t>(new std::uint8_t[size], std::default_delete<std::uint8_t[]>());
    pending.frame.size = size;
    pending.meta.codec = Codec::H264;
    pending.meta.key = key;
    pending.meta.pts_us = pts_us;
    pending.info.has_key_nal = key;
    pending.bytes = size;
    pending.sequence = sequence;
    return pending;
}

} // namespace

RTSP_TEST_CASE(gop_cache_disabled_when_config_all_zero)
{
    /* 场景：三项上限全 0 表示关闭 cache；预期任何帧都被忽略，快照与 epoch 不可用。 */
    GopCache cache;
    GopCacheConfig disabled;
    cache.Configure(disabled, 1024);
    RTSP_CHECK(!cache.enabled());

    const PendingFrame pending = MakePending(100, true, 1, 0);
    RTSP_CHECK(cache.OnFrame(pending, MakeCodecSnapshot(), true) == CacheUpdate::Ignored);
    GopSnapshot snapshot;
    RTSP_CHECK(!cache.SnapshotForReader(&snapshot));
    RTSP_CHECK_EQ(cache.epoch(), static_cast<std::uint64_t>(0));
}

RTSP_TEST_CASE(gop_cache_starts_only_on_key_with_codec_ready)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(), 1024);

    /* 参数集未就绪时，关键帧也不能建立 GOP（否则快照缺参数集，回放不可解码）。 */
    const PendingFrame key = MakePending(100, true, 1, 1000);
    RTSP_CHECK(cache.OnFrame(key, MakeCodecSnapshot(), false) == CacheUpdate::Ignored);
    GopSnapshot snapshot;
    RTSP_CHECK(!cache.SnapshotForReader(&snapshot));

    /* 非关键帧永远不能建立 GOP（起点必须由扫描出的 IDR/CRA 判定）。 */
    const PendingFrame non_key = MakePending(50, false, 2, 1100);
    RTSP_CHECK(cache.OnFrame(non_key, MakeCodecSnapshot(), true) == CacheUpdate::Ignored);

    RTSP_CHECK(cache.OnFrame(key, MakeCodecSnapshot(), true) == CacheUpdate::Started);
    RTSP_CHECK(cache.SnapshotForReader(&snapshot));
}

RTSP_TEST_CASE(gop_cache_appends_and_tracks_bounds)
{
    /* 场景：key 建立正常 GOP 后 P 帧追加；预期字节数/帧数同步累计，快照回放边界推进到末帧。 */
    GopCache cache;
    cache.Configure(MakeCacheConfig(), 1024);

    RTSP_CHECK(cache.OnFrame(MakePending(100, true, 1, 1000), MakeCodecSnapshot(), true) == CacheUpdate::Started);
    RTSP_CHECK(cache.OnFrame(MakePending(60, false, 2, 1140), MakeCodecSnapshot(), true) == CacheUpdate::Appended);
    RTSP_CHECK(cache.OnFrame(MakePending(40, false, 3, 1180), MakeCodecSnapshot(), true) == CacheUpdate::Appended);

    RTSP_CHECK_EQ(cache.current_bytes(), static_cast<std::size_t>(200));
    RTSP_CHECK_EQ(cache.current_frames(), static_cast<std::size_t>(3));

    GopSnapshot snapshot;
    RTSP_CHECK(cache.SnapshotForReader(&snapshot));
    RTSP_CHECK_EQ(snapshot.replay_end_index, static_cast<std::size_t>(3));
    RTSP_CHECK_EQ(snapshot.replay_end_sequence, static_cast<std::uint64_t>(3));
    RTSP_CHECK_EQ(snapshot.storage->begin_sequence, static_cast<std::uint64_t>(1));
    RTSP_CHECK_EQ(snapshot.storage->frames.front().info.has_key_nal, true);
}

RTSP_TEST_CASE(gop_cache_rolls_on_new_key_and_bumps_epoch)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(), 1024);

    const CodecConfigSnapshot codec = MakeCodecSnapshot();
    RTSP_CHECK(cache.OnFrame(MakePending(100, true, 1, 1000), codec, true) == CacheUpdate::Started);
    RTSP_CHECK(cache.OnFrame(MakePending(50, false, 2, 1140), codec, true) == CacheUpdate::Appended);
    const std::uint64_t epoch_before = cache.epoch();

    /* 新关键 AU：滚动出新一代，旧 storage 只由既有 Reader 保活。 */
    RTSP_CHECK(cache.OnFrame(MakePending(90, true, 3, 1240), codec, true) == CacheUpdate::Rolled);
    RTSP_CHECK_EQ(cache.epoch(), epoch_before + 1);
    RTSP_CHECK_EQ(cache.current_frames(), static_cast<std::size_t>(1));
    RTSP_CHECK_EQ(cache.current_bytes(), static_cast<std::size_t>(90));
}

RTSP_TEST_CASE(gop_cache_invalidates_on_byte_limit)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(200, 8, 5000), 1024);

    const CodecConfigSnapshot codec = MakeCodecSnapshot();
    RTSP_CHECK(cache.OnFrame(MakePending(150, true, 1, 1000), codec, true) == CacheUpdate::Started);
    const std::uint64_t epoch_before = cache.epoch();

    /* append 将突破 200 字节上限：整段失效，绝不做中间裁剪。 */
    RTSP_CHECK(cache.OnFrame(MakePending(60, false, 2, 1140), codec, true) == CacheUpdate::Invalidated);
    RTSP_CHECK_EQ(cache.epoch(), epoch_before + 1);
    GopSnapshot snapshot;
    RTSP_CHECK(!cache.SnapshotForReader(&snapshot));
    RTSP_CHECK_EQ(cache.current_frames(), static_cast<std::size_t>(0));
    RTSP_CHECK(cache.last_invalid_reason() == GopInvalidReason::Capacity);

    /* 失效后普通帧忽略，关键帧重建。 */
    RTSP_CHECK(cache.OnFrame(MakePending(60, false, 3, 1180), codec, true) == CacheUpdate::Ignored);
    RTSP_CHECK(cache.OnFrame(MakePending(80, true, 4, 1240), codec, true) == CacheUpdate::Started);
}

RTSP_TEST_CASE(gop_cache_invalidates_on_frame_and_duration_limits)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(64u * 1024u, 2, 100), 1024);

    const CodecConfigSnapshot codec = MakeCodecSnapshot();
    RTSP_CHECK(cache.OnFrame(MakePending(10, true, 1, 0), codec, true) == CacheUpdate::Started);
    RTSP_CHECK(cache.OnFrame(MakePending(10, false, 2, 40000), codec, true) == CacheUpdate::Appended);

    /* 帧数上限（max_frames=2）。 */
    RTSP_CHECK(cache.OnFrame(MakePending(10, false, 3, 80000), codec, true) == CacheUpdate::Invalidated);
    RTSP_CHECK(cache.last_invalid_reason() == GopInvalidReason::Capacity);

    /* 时长上限：失效后重建，第四帧相对 GOP 起点已超 100ms。 */
    RTSP_CHECK(cache.OnFrame(MakePending(10, true, 4, 200000), codec, true) == CacheUpdate::Started);
    RTSP_CHECK(cache.OnFrame(MakePending(10, false, 5, 400000), codec, true) == CacheUpdate::Invalidated);
    RTSP_CHECK(cache.last_invalid_reason() == GopInvalidReason::Capacity);
}

RTSP_TEST_CASE(gop_cache_invalidates_on_codec_generation_change)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(), 1024);

    RTSP_CHECK(cache.OnFrame(MakePending(100, true, 1, 1000), MakeCodecSnapshot(1), true) == CacheUpdate::Started);
    RTSP_CHECK(cache.OnFrame(MakePending(50, false, 2, 1140), MakeCodecSnapshot(1), true) == CacheUpdate::Appended);

    /* 参数集 generation 变化：绑定旧参数集的 GOP 立即失效防混代。 */
    RTSP_CHECK(cache.OnFrame(MakePending(50, false, 3, 1180), MakeCodecSnapshot(2), true) == CacheUpdate::Invalidated);
    RTSP_CHECK(cache.last_invalid_reason() == GopInvalidReason::CodecConfigChanged);
    GopSnapshot snapshot;
    RTSP_CHECK(!cache.SnapshotForReader(&snapshot));

    /* 新代次的关键帧按新参数集重建。 */
    RTSP_CHECK(cache.OnFrame(MakePending(70, true, 4, 1240), MakeCodecSnapshot(2), true) == CacheUpdate::Started);
    RTSP_CHECK(cache.SnapshotForReader(&snapshot));
    RTSP_CHECK_EQ(snapshot.codec_generation, static_cast<std::uint32_t>(2));
}

RTSP_TEST_CASE(gop_cache_configure_change_invalidates_current_gop)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(), 1024);
    RTSP_CHECK(cache.OnFrame(MakePending(100, true, 1, 1000), MakeCodecSnapshot(), true) == CacheUpdate::Started);
    GopSnapshot snapshot;
    RTSP_CHECK(cache.SnapshotForReader(&snapshot));

    /* 配置热更新：当前 GOP 失效，等下一个关键帧按新预算重建。 */
    cache.Configure(MakeCacheConfig(32u * 1024u, 4, 2000), 1024);
    RTSP_CHECK(!cache.SnapshotForReader(&snapshot));
    RTSP_CHECK(cache.last_invalid_reason() == GopInvalidReason::StreamConfigChanged);
}

RTSP_TEST_CASE(gop_cache_single_epoch_snapshot_is_immutable_view)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(), 1024);

    RTSP_CHECK(cache.OnFrame(MakePending(100, true, 1, 1000), MakeCodecSnapshot(), true) == CacheUpdate::Started);
    RTSP_CHECK(cache.OnFrame(MakePending(50, false, 2, 1140), MakeCodecSnapshot(), true) == CacheUpdate::Appended);
    GopSnapshot snapshot;
    RTSP_CHECK(cache.SnapshotForReader(&snapshot));

    /* 滚动后 Reader 持有的旧快照依然完整可回放（shared_ptr 保活）。 */
    RTSP_CHECK(cache.OnFrame(MakePending(90, true, 3, 1240), MakeCodecSnapshot(), true) == CacheUpdate::Rolled);
    RTSP_CHECK_EQ(snapshot.storage->frames.size(), static_cast<std::size_t>(2));
    RTSP_CHECK_EQ(snapshot.replay_end_sequence, static_cast<std::uint64_t>(2));
    RTSP_CHECK_EQ(snapshot.storage->begin_sequence, static_cast<std::uint64_t>(1));
}

RTSP_TEST_CASE(gop_cache_stats_counters)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(200, 8, 5000), 1024);

    const CodecConfigSnapshot codec = MakeCodecSnapshot();
    RTSP_CHECK(cache.OnFrame(MakePending(100, true, 1, 1000), codec, true) == CacheUpdate::Started);
    RTSP_CHECK(cache.OnFrame(MakePending(50, false, 2, 1140), codec, true) == CacheUpdate::Appended);
    RTSP_CHECK(cache.OnFrame(MakePending(60, true, 3, 1240), codec, true) == CacheUpdate::Rolled);
    /* 新 GOP（60B）+ 5000B 超过 200B 预算：整段失效。 */
    RTSP_CHECK(cache.OnFrame(MakePending(5000, false, 4, 1380), codec, true) == CacheUpdate::Invalidated);

    RTSP_CHECK_EQ(cache.started(), static_cast<std::uint64_t>(2));
    RTSP_CHECK_EQ(cache.invalidated(), static_cast<std::uint64_t>(1));
}

RTSP_TEST_CASE(gop_cache_audio_disabled_with_cache)
{
    GopCache cache;
    GopCacheConfig disabled;
    cache.Configure(disabled, 1024);
    AudioPendingFrame audio;
    audio.codec = AudioCodec::AAC;
    audio.size = 64;
    audio.pts_us = 1000;
    audio.has_pts = false;
    cache.OnAudioFrame(audio);
    /* cache 关闭时音频不入环，也不规范化 pts。 */
    RTSP_CHECK(!audio.has_pts);
    RTSP_CHECK_EQ(cache.audio_frames(), static_cast<std::size_t>(0));
}

RTSP_TEST_CASE(gop_cache_audio_pts_normalized_on_ingest)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(), 1024);

    AudioPendingFrame audio;
    audio.codec = AudioCodec::AAC;
    audio.size = 64;
    audio.pts_us = 0;
    audio.has_pts = false;
    cache.OnAudioFrame(audio);

    /* 无真实 PTS 的音频在入环时被规范化为单调钟（>0），之后 live 与回放同轴。 */
    RTSP_CHECK(audio.has_pts);
    RTSP_CHECK(audio.pts_us > 0);
    RTSP_CHECK_EQ(cache.audio_frames(), static_cast<std::size_t>(1));
}

RTSP_TEST_CASE(gop_cache_audio_snapshot_aligned_to_gop_start)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(), 1024);

    const std::int64_t base = 1000000;
    /* GOP：关键帧 1000000 起 + 两个 P 帧。 */
    RTSP_CHECK(cache.OnFrame(MakePending(100, true, 1, base), MakeCodecSnapshot(), true) == CacheUpdate::Started);
    RTSP_CHECK(cache.OnFrame(MakePending(100, false, 2, base + 33000), MakeCodecSnapshot(), true) == CacheUpdate::Appended);
    RTSP_CHECK(cache.OnFrame(MakePending(100, false, 3, base + 66000), MakeCodecSnapshot(), true) == CacheUpdate::Appended);

    /* GOP 起点之前的音频：不应进入回放段。 */
    AudioPendingFrame before;
    before.codec = AudioCodec::AAC;
    before.size = 32;
    before.pts_us = base - 64000;
    before.has_pts = true;
    cache.OnAudioFrame(before);

    /* GOP 区间内的音频：进入回放段。 */
    AudioPendingFrame in_gop;
    in_gop.codec = AudioCodec::AAC;
    in_gop.size = 32;
    in_gop.pts_us = base + 1000;
    in_gop.has_pts = true;
    cache.OnAudioFrame(in_gop);

    GopSnapshot snapshot;
    RTSP_CHECK(cache.SnapshotForReader(&snapshot));
    RTSP_CHECK(snapshot.audio != nullptr);
    RTSP_CHECK_EQ(snapshot.audio->frames.size(), static_cast<std::size_t>(1));
    RTSP_CHECK_EQ(snapshot.audio->frames[0].pts_us, base + 1000);
}

RTSP_TEST_CASE(gop_cache_audio_trimmed_on_gop_roll)
{
    GopCache cache;
    cache.Configure(MakeCacheConfig(), 1024);

    const std::int64_t gop1 = 1000000;
    RTSP_CHECK(cache.OnFrame(MakePending(100, true, 1, gop1), MakeCodecSnapshot(), true) == CacheUpdate::Started);
    AudioPendingFrame a1;
    a1.codec = AudioCodec::AAC;
    a1.size = 32;
    a1.pts_us = gop1 + 1000;
    a1.has_pts = true;
    cache.OnAudioFrame(a1);

    /* 新关键帧滚动 GOP：旧音频（pts < 新关键帧）被裁出音频环。 */
    const std::int64_t gop2 = gop1 + 2000000;
    RTSP_CHECK(cache.OnFrame(MakePending(100, true, 2, gop2), MakeCodecSnapshot(), true) == CacheUpdate::Rolled);
    RTSP_CHECK_EQ(cache.audio_frames(), static_cast<std::size_t>(0));

    AudioPendingFrame a2;
    a2.codec = AudioCodec::AAC;
    a2.size = 32;
    a2.pts_us = gop2 + 1000;
    a2.has_pts = true;
    cache.OnAudioFrame(a2);

    GopSnapshot snapshot;
    RTSP_CHECK(cache.SnapshotForReader(&snapshot));
    RTSP_CHECK(snapshot.audio != nullptr);
    RTSP_CHECK_EQ(snapshot.audio->frames.size(), static_cast<std::size_t>(1));
    RTSP_CHECK_EQ(snapshot.audio->frames[0].pts_us, gop2 + 1000);
}
