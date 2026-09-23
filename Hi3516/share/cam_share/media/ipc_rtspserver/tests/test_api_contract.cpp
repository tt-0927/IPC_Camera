/**
 * @FilePath     : test_api_contract.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 公共 API 契约测试
 */

/*
 * 目的：把 ipc_share/push_stream/rtsp_smol（产品薄封装）依赖的公共 API 固定下来。
 * 该封装无法在本仓库单独编译（依赖父工程的 dlog/convert/设备能力宏），因此这里用
 * 等价的调用序列做编译期与运行期双重校验：
 *   - 编译期：配置字段、帧视图字段、方法签名一旦变更立刻报错；
 *   - 运行期：端口 0 自动分配、URL 生成、指标快照、启动/关停幂等。
 */

#include "ipc_rtsp/log.h"
#include "ipc_rtsp/server.h"

#include "test_support.h"

#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace ipc_rtsp;

namespace
{

/** 按产品画像构造两路码流配置（字段与 rtsp_smol 封装保持一致）。
 *
 * @return 两路码流配置（主码流 H.264 + 子码流 H.265）。
 */
std::vector<StreamConfig> MakeStreams()
{
    std::vector<StreamConfig> streams(2);

    streams[0].id = StreamId::Main;
    streams[0].path = "Streaming/Channels/101";
    streams[0].codec = Codec::H264;
    streams[0].payload_type = 96;
    streams[0].clock_rate = 90000;
    streams[0].fps = 25;
    streams[0].max_frame_bytes = 2u * 1024u * 1024u;
    streams[0].max_playing_clients = 2; /* 主码流按码率分档 */
    streams[0].audio_enabled = true;
    streams[0].audio_codec = AudioCodec::G711A;
    streams[0].audio_payload_type = 8;
    streams[0].audio_clock_rate = 8000;
    streams[0].audio_channels = 1;

    streams[1].id = StreamId::Sub;
    streams[1].path = "Streaming/Channels/102";
    streams[1].codec = Codec::H265;
    streams[1].payload_type = 96;
    streams[1].clock_rate = 90000;
    streams[1].fps = 25;
    streams[1].max_frame_bytes = 512u * 1024u;

    return streams;
}

/** 按产品画像构造服务配置。
 *
 * @return 预填端口/背压/Digest 鉴权的服务端配置。
 */
ServerConfig MakeServerConfig()
{
    ServerConfig config;
    config.port = 0; /* 0 = 由内核分配，便于测试并发运行 */
    config.bind_address = "127.0.0.1";
    config.advertised_ip = "127.0.0.1";
    config.max_connections = 8;
    config.max_playing_clients = 4;
    config.dscp = 0;
    config.server_name = "IPC RTSP Server";
    config.auth.mode = AuthMode::Digest;
    config.auth.realm = "Itc Streaming Server";
    config.auth.user = "admin";
    config.auth.password = "zfrl@168";

    config.backpressure.hub_max_video_frames = 6;
    config.backpressure.hub_max_video_bytes = 1u * 1024u * 1024u;
    config.backpressure.hub_max_audio_frames = 6;
    config.backpressure.hub_max_audio_bytes = 64u * 1024u;
    config.backpressure.client_soft_backlog_ms = 1500;
    config.backpressure.client_hard_backlog_ms = 3000;
    config.backpressure.client_output_guard_bytes = 1u * 1024u * 1024u;
    return config;
}

} // namespace

RTSP_TEST_CASE(api_contract_create_and_lifecycle)
{
    std::unique_ptr<Server> server;
    Result result = Server::Create(MakeServerConfig(), MakeStreams(), &server);
    RTSP_CHECK(result.ok());
    RTSP_CHECK(server != nullptr);
    if (server == nullptr)
    {
        /* 显式返回：既让失败路径明确，也让静态分析不必推断 RTSP_CHECK 会终止进程。 */
        return;
    }

    /* 未 Start 前推送应被拒绝（NotInitialized），而不是崩溃。 */
    const std::uint8_t dummy[16] = { 0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00 };
    VideoFrameView frame;
    frame.data = dummy;
    frame.size = sizeof dummy;
    frame.codec = Codec::H264;
    frame.first_nal = NalUnitType::IdrSlice;
    frame.key = true;
    RTSP_CHECK(!server->PushVideo(StreamId::Main, frame).ok());

    result = server->Start();
    RTSP_CHECK(result.ok());

    /* URL 必须保持产品约定的路径。 */
    const std::string url = server->Url(StreamId::Main, false);
    RTSP_CHECK(url.find("rtsp://127.0.0.1:") == 0);
    RTSP_CHECK(url.find("/Streaming/Channels/101") != std::string::npos);
    const std::string auth_url = server->Url(StreamId::Main, true);
    RTSP_CHECK(auth_url.find("admin:zfrl@168@") != std::string::npos);

    /* 无订阅者时推送不应报错（库内零拷贝丢弃），且指标可读。 */
    RTSP_CHECK(server->PushVideo(StreamId::Main, frame).ok());
    const MetricsSnapshot snapshot = server->Snapshot();
    RTSP_CHECK_EQ(snapshot.streams[0].pushed_frames, static_cast<std::uint64_t>(1));
    RTSP_CHECK_EQ(snapshot.connections_active, static_cast<std::uint64_t>(0));

    /* 共享帧路径（产品 SharedMediaFrame_S 的等价用法）。 */
    SharedVideoFrame shared;
    shared.data = std::shared_ptr<const std::uint8_t>(new std::uint8_t[sizeof dummy], std::default_delete<std::uint8_t[]>());
    std::memcpy(const_cast<std::uint8_t *>(shared.data.get()), dummy, sizeof dummy);
    shared.size = sizeof dummy;

    VideoFrameMeta meta;
    meta.codec = Codec::H264;
    meta.first_nal = NalUnitType::IdrSlice;
    meta.key = true;
    RTSP_CHECK(server->PushVideo(StreamId::Main, shared, meta).ok());

    /* 音频：未启用音频的码流应被拒绝；启用的码流接受。 */
    AudioFrameView audio;
    audio.data = dummy;
    audio.size = sizeof dummy;
    audio.codec = AudioCodec::G711A;
    RTSP_CHECK(server->PushAudio(StreamId::Main, audio).ok());
    RTSP_CHECK(!server->PushAudio(StreamId::Sub, audio).ok());

    /* 控制面热更新。 */
    RTSP_CHECK(server->UpdateCredential("admin2", "pwd2").ok());
    RTSP_CHECK(server->Url(StreamId::Main, true).find("admin2:pwd2@") != std::string::npos);
    RTSP_CHECK(server->UpdateDigestAlgorithm(DigestAlgorithm::Md5).ok());
    RTSP_CHECK(server->UpdateAdvertisedIp("192.168.1.10").ok());
    RTSP_CHECK(server->Url(StreamId::Sub, false).find("192.168.1.10") != std::string::npos);

    std::vector<StreamConfig> updated = MakeStreams();
    updated[0].max_playing_clients = 1;
    RTSP_CHECK(server->UpdateStreamConfig(updated).ok());

    server->SetIdrRequestCallback(
        [](StreamId)
        {
            /* 契约：回调在 I/O 线程内触发；测试里不触发任何阻塞动作。 */
        });

    /* 停机必须幂等且带截止时间。 */
    RTSP_CHECK(server->Shutdown(std::chrono::milliseconds(1000)).ok());
    RTSP_CHECK(server->Shutdown(std::chrono::milliseconds(1000)).ok());
}

RTSP_TEST_CASE(api_contract_rejects_invalid_configuration)
{
    std::unique_ptr<Server> server;

    /* 缺少子码流：必须拒绝而不是留下半初始化状态。 */
    std::vector<StreamConfig> only_main(1);
    only_main[0].id = StreamId::Main;
    only_main[0].path = "Streaming/Channels/101";
    only_main[0].codec = Codec::H264;
    RTSP_CHECK(!Server::Create(MakeServerConfig(), only_main, &server).ok());
    RTSP_CHECK(server == nullptr);

    /* 路径带前导 '/' 属于非法（产品约定不带）。 */
    std::vector<StreamConfig> bad_path = MakeStreams();
    bad_path[0].path = "/Streaming/Channels/101";
    RTSP_CHECK(!Server::Create(MakeServerConfig(), bad_path, &server).ok());

    /* 未知 codec。 */
    std::vector<StreamConfig> bad_codec = MakeStreams();
    bad_codec[0].codec = Codec::Unknown;
    RTSP_CHECK(!Server::Create(MakeServerConfig(), bad_codec, &server).ok());

    /* GOP cache 配置“部分为 0”：无法表达一致语义，直接拒绝（P1-01）。 */
    std::vector<StreamConfig> bad_gop_partial = MakeStreams();
    bad_gop_partial[0].gop_cache.max_bytes = 4u * 1024u * 1024u;
    bad_gop_partial[0].gop_cache.max_frames = 0;
    bad_gop_partial[0].gop_cache.max_duration_ms = 3000;
    RTSP_CHECK(!Server::Create(MakeServerConfig(), bad_gop_partial, &server).ok());

    /* 三项全 0（默认值）表示关闭 cache：合法，可正常创建。 */
    std::vector<StreamConfig> gop_disabled = MakeStreams();
    RTSP_CHECK(Server::Create(MakeServerConfig(), gop_disabled, &server).ok());
    RTSP_CHECK(server != nullptr);

    /* 产品初值（主 4MiB / 子 1MiB / 3000ms）可接受，且编译期锁定 GopCacheConfig
     * 字段存在于公共头（字段删改会直接编译失败）。 */
    std::vector<StreamConfig> gop_enabled = MakeStreams();
    gop_enabled[0].gop_cache.max_bytes = 4u * 1024u * 1024u;
    gop_enabled[0].gop_cache.max_frames = 60;
    gop_enabled[0].gop_cache.max_duration_ms = 3000;
    gop_enabled[1].gop_cache.max_bytes = 1u * 1024u * 1024u;
    gop_enabled[1].gop_cache.max_frames = 60;
    gop_enabled[1].gop_cache.max_duration_ms = 3000;
    RTSP_CHECK(Server::Create(MakeServerConfig(), gop_enabled, &server).ok());
    /* 未 Start（Stopped）时 ExecuteControl 在调用方线程直接执行任务，
     * 快照为全量口径，GOP 配置状态可见；仅 Running 且 I/O 线程超时才降级
     * （降级口径 GOP 状态字段为 0，见 BuildCountersOnlySnapshot）。 */
    const MetricsSnapshot metrics = server->Snapshot();
    RTSP_CHECK_EQ(metrics.streams[0].gop_cache_enabled, static_cast<std::uint64_t>(1));
    RTSP_CHECK_EQ(metrics.streams[1].gop_cache_enabled, static_cast<std::uint64_t>(1));
    server.reset();
}
