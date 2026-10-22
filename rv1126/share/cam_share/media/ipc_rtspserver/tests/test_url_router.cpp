/**
 * @FilePath     : test_url_router.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : URL 路由测试（产品兼容路径 101/102）
 */

#include "rtsp/url_router.h"

#include "test_support.h"

#include <vector>

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

namespace
{

/** 构造已注册产品路径 101/102 的路由器。
 *
 * @return 完成两路路径配置的 UrlRouter。
 */
UrlRouter MakeRouter()
{
    std::vector<StreamConfig> streams(2);
    streams[0].id = StreamId::Main;
    streams[0].path = "Streaming/Channels/101";
    streams[0].codec = Codec::H264;
    streams[1].id = StreamId::Sub;
    streams[1].path = "Streaming/Channels/102";
    streams[1].codec = Codec::H264;

    UrlRouter router;
    router.Configure(streams);
    return router;
}

} // namespace

RTSP_TEST_CASE(url_router_accepts_full_and_absolute_uris)
{
    /* 场景：绝对路径、完整 rtsp:// URI、带查询串与无前导斜杠；预期全部解析到正确的流。 */
    const UrlRouter router = MakeRouter();
    StreamId stream = StreamId::Sub;

    RTSP_CHECK(router.ResolveStream("/Streaming/Channels/101", &stream));
    RTSP_CHECK(stream == StreamId::Main);

    RTSP_CHECK(router.ResolveStream("rtsp://192.168.1.10:554/Streaming/Channels/102", &stream));
    RTSP_CHECK(stream == StreamId::Sub);

    /* 带查询串与不带前导斜杠的形式都要能解析。 */
    RTSP_CHECK(router.ResolveStream("rtsp://admin:pwd@10.0.0.1/Streaming/Channels/101?x=1", &stream));
    RTSP_CHECK(stream == StreamId::Main);
    RTSP_CHECK(router.ResolveStream("Streaming/Channels/102", &stream));
    RTSP_CHECK(stream == StreamId::Sub);
}

RTSP_TEST_CASE(url_router_track_suffix_forms)
{
    const UrlRouter router = MakeRouter();
    StreamId stream = StreamId::Main;
    int track = -1;

    /* 产品/主流客户端常见的四种写法。 */
    RTSP_CHECK(router.ResolveTrack("/Streaming/Channels/101/trackID=0", &stream, &track));
    RTSP_CHECK(stream == StreamId::Main);
    RTSP_CHECK_EQ(track, kTrackVideo);

    RTSP_CHECK(router.ResolveTrack("/Streaming/Channels/101/trackID=1", &stream, &track));
    RTSP_CHECK_EQ(track, kTrackAudio);

    RTSP_CHECK(router.ResolveTrack("/Streaming/Channels/102/track1", &stream, &track));
    RTSP_CHECK(stream == StreamId::Sub);
    RTSP_CHECK_EQ(track, kTrackVideo);

    RTSP_CHECK(router.ResolveTrack("/Streaming/Channels/102/track2", &stream, &track));
    RTSP_CHECK_EQ(track, kTrackAudio);

    RTSP_CHECK(router.ResolveTrack("/Streaming/Channels/101/video", &stream, &track));
    RTSP_CHECK_EQ(track, kTrackVideo);

    RTSP_CHECK(router.ResolveTrack("/Streaming/Channels/101/audio", &stream, &track));
    RTSP_CHECK_EQ(track, kTrackAudio);

    /* 不带后缀时默认视频轨。 */
    RTSP_CHECK(router.ResolveTrack("/Streaming/Channels/101", &stream, &track));
    RTSP_CHECK_EQ(track, kTrackVideo);
}

RTSP_TEST_CASE(url_router_rejects_unknown_paths)
{
    /* 场景：未配置的路径与未知 track 后缀；预期一律解析失败，不落到默认流。 */
    const UrlRouter router = MakeRouter();
    StreamId stream = StreamId::Main;

    RTSP_CHECK(!router.ResolveStream("/main", &stream));
    RTSP_CHECK(!router.ResolveStream("/Streaming/Channels/103", &stream));
    RTSP_CHECK(!router.ResolveStream("/Streaming/Channels/1010", &stream));

    int track = -1;
    RTSP_CHECK(!router.ResolveTrack("/Streaming/Channels/101/trackID=7", &stream, &track));
    RTSP_CHECK(!router.ResolveTrack("/Streaming/Channels/102/unknown", &stream, &track));
}

RTSP_TEST_CASE(url_router_exposes_configured_paths)
{
    /* 场景：按流 id 反查路径；预期输出与注册值一致（不带前导斜杠）。 */
    const UrlRouter router = MakeRouter();
    RTSP_CHECK_EQ(router.StreamPath(StreamId::Main), std::string("Streaming/Channels/101"));
    RTSP_CHECK_EQ(router.StreamPath(StreamId::Sub), std::string("Streaming/Channels/102"));
}
