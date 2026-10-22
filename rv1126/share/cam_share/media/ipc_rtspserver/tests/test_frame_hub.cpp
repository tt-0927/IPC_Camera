/**
 * @FilePath     : test_frame_hub.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : FrameHub / Subscriber 背压与状态机测试
 */

#include "media/frame_hub.h"

#include "test_support.h"

#include <cstring>
#include <memory>

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

namespace
{

/** 构造主码流配置。
 *
 * @return 预填 path/codec/fps/帧缓冲上限的码流配置。
 */
StreamConfig MakeStreamConfig()
{
    StreamConfig config;
    config.id = StreamId::Main;
    config.path = "Streaming/Channels/101";
    config.codec = Codec::H264;
    config.max_frame_bytes = 64u * 1024u;
    config.fps = 25;
    return config;
}

/** 构造背压配置。
 *
 * @return 预填 hub 与 client 队列上限的背压配置。
 */
BackpressureConfig MakeBackpressure()
{
    BackpressureConfig config;
    config.hub_max_video_frames = 4;
    config.hub_max_video_bytes = 4096;
    config.client_max_pending_frames = 3;
    config.client_max_pending_bytes = 1024;
    return config;
}

/** 构造共享内存视频帧。
 *
 * @param size 帧字节数。
 * @return 已分配 size 字节缓冲的共享帧。
 */
SharedVideoFrame MakeFrame(std::size_t size)
{
    SharedVideoFrame frame;
    frame.data = std::shared_ptr<const std::uint8_t>(new std::uint8_t[size], std::default_delete<std::uint8_t[]>());
    frame.size = size;
    return frame;
}

/** 构造待入队视频帧。
 *
 * @param size 帧字节数。
 * @param key 是否关键帧（同步填充 meta 与 FrameInfo）。
 * @return 已填充元信息的 PendingFrame。
 */
PendingFrame MakePending(std::size_t size, bool key)
{
    PendingFrame pending;
    pending.frame = MakeFrame(size);
    pending.meta.codec = Codec::H264;
    pending.meta.key = key;
    pending.info.has_key_nal = key;
    pending.bytes = size;
    return pending;
}

} // namespace

RTSP_TEST_CASE(subscriber_pending_queue_is_bounded_by_frames)
{
    Subscriber subscriber(3, 1024);

    RTSP_CHECK(!subscriber.Enqueue(MakePending(10, false))); /* 未激活：拒绝 */
    subscriber.Activate();
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, false)));
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, false)));
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, false)));
    RTSP_CHECK_EQ(subscriber.frames(), static_cast<std::size_t>(3));

    /* 队列满时淘汰最老的非关键帧以接纳新帧（freshness > completeness），
     * 队列长度始终不超过上限，并累计丢帧计数。 */
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, false)));
    RTSP_CHECK_EQ(subscriber.frames(), static_cast<std::size_t>(3));
    RTSP_CHECK(subscriber.dropped_frames() > 0);
}

RTSP_TEST_CASE(subscriber_keeps_key_frames_when_full_of_keys)
{
    Subscriber subscriber(2, 1024);
    subscriber.Activate();
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, true)));
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, true)));

    /* 队列里全是关键帧时，新来的非关键帧必须被丢弃（保留可解码起点）。 */
    RTSP_CHECK(!subscriber.Enqueue(MakePending(10, false)));
    RTSP_CHECK_EQ(subscriber.frames(), static_cast<std::size_t>(2));

    /* 新的关键帧可以替换最老的关键帧，队列不增长。 */
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, true)));
    RTSP_CHECK_EQ(subscriber.frames(), static_cast<std::size_t>(2));
}

RTSP_TEST_CASE(subscriber_drops_oldest_non_key_before_key)
{
    Subscriber subscriber(3, 1024);
    subscriber.Activate();
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, false)));
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, false)));
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, true)));

    /* 新关键帧到达：优先淘汰最老的非关键帧。 */
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, true)));
    RTSP_CHECK_EQ(subscriber.frames(), static_cast<std::size_t>(3));

    PendingFrame out;
    RTSP_CHECK(subscriber.Pop(&out));
    RTSP_CHECK(!out.info.has_key_nal); /* 剩下的第一个仍是普通帧 */
}

RTSP_TEST_CASE(subscriber_respects_byte_budget)
{
    Subscriber subscriber(8, 100);
    subscriber.Activate();
    RTSP_CHECK(subscriber.Enqueue(MakePending(60, false)));
    RTSP_CHECK_EQ(subscriber.bytes(), static_cast<std::size_t>(60));
    /* 再加 60 字节会超字节上限：淘汰旧帧后入队。 */
    RTSP_CHECK(subscriber.Enqueue(MakePending(60, false)));
    RTSP_CHECK(subscriber.bytes() <= 100);
}

RTSP_TEST_CASE(subscriber_congested_state_only_keeps_key_frames)
{
    /* 场景：订阅者被标记为拥塞；预期非关键帧直接拒绝入队，关键帧仍可入队。 */
    Subscriber subscriber(8, 4096);
    subscriber.Activate();
    subscriber.SetState(SubscriberState::Congested);
    RTSP_CHECK(!subscriber.Enqueue(MakePending(10, false)));
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, true)));
    RTSP_CHECK_EQ(subscriber.frames(), static_cast<std::size_t>(1));
}

RTSP_TEST_CASE(subscriber_audio_queue_bounded_and_fifo)
{
    Subscriber subscriber(4, 1024);
    subscriber.Activate();

    AudioPendingFrame audio;
    audio.size = 160;
    for (int i = 0; i < 6; ++i)
    {
        audio.data[0] = static_cast<std::uint8_t>(i);
        RTSP_CHECK(subscriber.EnqueueAudio(audio));
    }
    /* 音频队列固定 4 条：只保留最新的。 */
    RTSP_CHECK_EQ(subscriber.audio_frames(), static_cast<std::size_t>(4));

    AudioPendingFrame out;
    RTSP_CHECK(subscriber.PopAudio(&out));
    RTSP_CHECK_EQ(static_cast<int>(out.data[0]), 2);
}

RTSP_TEST_CASE(frame_hub_subscriber_count_and_stats)
{
    const StreamConfig config = MakeStreamConfig();
    FrameHub hub(StreamId::Main, config, MakeBackpressure());

    Subscriber subscriber(4, 4096);
    RTSP_CHECK_EQ(hub.SubscriberCount(), static_cast<std::size_t>(0));
    RTSP_CHECK(hub.NeedsConfigCapture()); /* 参数集未就绪时需要捕获 */

    hub.AddSubscriber(&subscriber);
    RTSP_CHECK_EQ(hub.SubscriberCount(), static_cast<std::size_t>(1));

    subscriber.Activate();
    RTSP_CHECK(hub.HasWaitingSubscriber());

    /* 无参数集的一帧：只累计推送统计。 */
    hub.NotePushed(100);
    const SharedVideoFrame frame = MakeFrame(16);
    VideoFrameMeta meta;
    FrameInfo info;
    info.has_key_nal = false;
    hub.OnVideoFrame(frame, meta, info);

    const StreamStats stats = hub.Stats();
    RTSP_CHECK_EQ(stats.pushed_frames, static_cast<std::uint64_t>(1));
    RTSP_CHECK_EQ(stats.pushed_bytes, static_cast<std::uint64_t>(100));
    RTSP_CHECK_EQ(stats.subscribers, static_cast<std::uint64_t>(1));

    hub.RemoveSubscriber(&subscriber);
    RTSP_CHECK_EQ(hub.SubscriberCount(), static_cast<std::size_t>(0));
}

RTSP_TEST_CASE(frame_hub_caches_parameter_sets_without_subscribers)
{
    const StreamConfig config = MakeStreamConfig();
    FrameHub hub(StreamId::Main, config, MakeBackpressure());

    /* 单包复合帧：SPS + PPS + IDR。 */
    const std::uint8_t data[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xE0, 0x1E, 0x00, 0x00, 0x00, 0x01,
        0x68, 0xCE, 0x3C, 0x80, 0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84,
    };

    SharedVideoFrame frame;
    frame.data = std::shared_ptr<const std::uint8_t>(new std::uint8_t[sizeof data], std::default_delete<std::uint8_t[]>());
    std::memcpy(const_cast<std::uint8_t *>(frame.data.get()), data, sizeof data);
    frame.size = sizeof data;

    VideoFrameMeta meta;
    FrameInfo info;
    info.has_parameter_set = true;
    info.has_key_nal = true;

    hub.OnVideoFrame(frame, meta, info);

    RTSP_CHECK(hub.codec_config().Ready());
    RTSP_CHECK(!hub.NeedsConfigCapture());
    RTSP_CHECK_EQ(hub.codec_config().Generation(), static_cast<std::uint32_t>(3)); /* SPS/PPS 各更新一次 + 初始 1 */
    RTSP_CHECK(!hub.codec_config().H264SpropParameterSets().empty());
}

RTSP_TEST_CASE(frame_hub_gop_cache_attach_and_fanout_metadata)
{
    StreamConfig config = MakeStreamConfig();
    config.gop_cache.max_bytes = 64u * 1024u;
    config.gop_cache.max_frames = 8;
    config.gop_cache.max_duration_ms = 3000;
    FrameHub hub(StreamId::Main, config, MakeBackpressure());

    Subscriber subscriber(4, 64u * 1024u);
    hub.AddSubscriber(&subscriber);
    subscriber.Activate();

    /* key 帧（含 SPS/PPS/IDR）建立 GOP，随后 P 帧追加。 */
    const std::uint8_t key_data[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xE0, 0x1E, 0x00, 0x00, 0x00, 0x01,
        0x68, 0xCE, 0x3C, 0x80, 0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84,
    };
    const std::uint8_t p_data[] = { 0x00, 0x00, 0x00, 0x01, 0x41, 0x9A, 0x02, 0x05 };

    auto push = [&](const std::uint8_t *data, std::size_t size, bool key)
    {
        SharedVideoFrame frame;
        frame.data = std::shared_ptr<const std::uint8_t>(new std::uint8_t[size], std::default_delete<std::uint8_t[]>());
        std::memcpy(const_cast<std::uint8_t *>(frame.data.get()), data, size);
        frame.size = size;
        VideoFrameMeta meta;
        meta.key = key;
        FrameInfo info;
        info.has_key_nal = key;
        info.has_parameter_set = key;
        info.vcl_count = 1;
        hub.OnVideoFrame(frame, meta, info);
    };

    push(key_data, sizeof key_data, true);
    push(p_data, sizeof p_data, false);

    /* cache 命中：attach 返回快照，参数集与 GOP 同代。 */
    GopSnapshot snapshot;
    RTSP_CHECK(hub.AttachReader(&snapshot));
    RTSP_CHECK_EQ(snapshot.replay_end_index, static_cast<std::size_t>(2));
    RTSP_CHECK_EQ(snapshot.storage->codec_config.generation, hub.codec_config().Generation());
    RTSP_CHECK(snapshot.storage->codec_config.sps.valid());

    /* 扇出帧携带单调 sequence 与当前 epoch。 */
    PendingFrame out;
    RTSP_CHECK(subscriber.Pop(&out));
    RTSP_CHECK_EQ(out.sequence, static_cast<std::uint64_t>(1));
    RTSP_CHECK_EQ(out.gop_epoch, snapshot.epoch);
    RTSP_CHECK(subscriber.Pop(&out));
    RTSP_CHECK_EQ(out.sequence, static_cast<std::uint64_t>(2));
    RTSP_CHECK_EQ(out.gop_epoch, snapshot.epoch);

    const StreamStats stats = hub.Stats();
    RTSP_CHECK_EQ(stats.gop_cache_enabled, static_cast<std::uint64_t>(1));
    RTSP_CHECK_EQ(stats.gop_started, static_cast<std::uint64_t>(1));
    RTSP_CHECK_EQ(stats.gop_cache_frames, static_cast<std::size_t>(2));
}

RTSP_TEST_CASE(frame_hub_gop_cache_disabled_falls_back_to_live_only)
{
    StreamConfig config = MakeStreamConfig(); /* gop_cache 全 0：关闭 */
    FrameHub hub(StreamId::Main, config, MakeBackpressure());

    const std::uint8_t key_data[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xE0, 0x1E, 0x00, 0x00, 0x00, 0x01,
        0x68, 0xCE, 0x3C, 0x80, 0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84,
    };
    SharedVideoFrame frame;
    frame.data = std::shared_ptr<const std::uint8_t>(new std::uint8_t[sizeof key_data], std::default_delete<std::uint8_t[]>());
    std::memcpy(const_cast<std::uint8_t *>(frame.data.get()), key_data, sizeof key_data);
    frame.size = sizeof key_data;
    VideoFrameMeta meta;
    FrameInfo info;
    info.has_key_nal = true;
    info.has_parameter_set = true;
    hub.OnVideoFrame(frame, meta, info);

    GopSnapshot snapshot;
    RTSP_CHECK(!hub.AttachReader(&snapshot));
    const StreamStats stats = hub.Stats();
    RTSP_CHECK_EQ(stats.gop_cache_enabled, static_cast<std::uint64_t>(0));
}

RTSP_TEST_CASE(subscriber_clear_video_keeps_audio_queue)
{
    Subscriber subscriber(4, 4096);
    subscriber.Activate();

    RTSP_CHECK(subscriber.Enqueue(MakePending(10, false)));
    RTSP_CHECK(subscriber.Enqueue(MakePending(10, true)));

    AudioPendingFrame audio;
    audio.size = 160;
    RTSP_CHECK(subscriber.EnqueueAudio(audio));

    /* Reader attach 时只清视频帧（回放覆盖历史），音频队列不动。 */
    subscriber.ClearVideo();
    RTSP_CHECK_EQ(subscriber.frames(), static_cast<std::size_t>(0));
    RTSP_CHECK_EQ(subscriber.bytes(), static_cast<std::size_t>(0));
    RTSP_CHECK_EQ(subscriber.audio_frames(), static_cast<std::size_t>(1));
}
