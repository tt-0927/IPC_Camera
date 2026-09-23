/**
 * @FilePath     : test_server_lifecycle.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 14:38:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : ServerImpl 生命周期与控制面线程归属回归测试
 */

#include "server_impl.h"
#include "test_support.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{

using ipc_rtsp::AuthMode;
using ipc_rtsp::Codec;
using ipc_rtsp::Result;
using ipc_rtsp::ServerConfig;
using ipc_rtsp::Status;
using ipc_rtsp::StreamConfig;
using ipc_rtsp::StreamId;
using ipc_rtsp::detail::ServerImpl;

/** 构造主/子两路码流配置。
 *
 * @return 两路码流配置（主 H.264 + 子 H.265）。
 */
std::vector<StreamConfig> MakeStreams()
{
    std::vector<StreamConfig> streams(2);
    streams[0].id = StreamId::Main;
    streams[0].path = "Streaming/Channels/101";
    streams[0].codec = Codec::H264;
    streams[0].max_frame_bytes = 2u * 1024u * 1024u;

    streams[1].id = StreamId::Sub;
    streams[1].path = "Streaming/Channels/102";
    streams[1].codec = Codec::H265;
    streams[1].max_frame_bytes = 512u * 1024u;
    return streams;
}

/** 构造最小可用的服务配置。
 *
 * @return 预填绑定地址与 Digest 鉴权的服务配置；端口 0 由内核分配。
 */
ServerConfig MakeConfig()
{
    ServerConfig config;
    config.port = 0;
    config.bind_address = "127.0.0.1";
    config.advertised_ip = "127.0.0.1";
    config.auth.mode = AuthMode::Digest;
    config.auth.user = "admin";
    config.auth.password = "initial";
    return config;
}

/** 自旋等待条件成立，超时返回 false。
 *
 * @param predicate 轮询谓词。
 * @param timeout_ms 等待上限（毫秒），默认 1000。
 * @return 截止时间内条件成立返回 true，否则返回 false。
 */
template <typename Predicate>
bool WaitFor(Predicate predicate, int timeout_ms = 1000)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (predicate())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return predicate();
}

} // namespace

RTSP_TEST_CASE(server_config_g711_clock_normalized_to_8000)
{
    /* G.711/G.726 的 RTSP 时钟是协议常量 8000：上层误传设备采集采样率
     * （如 16000）时必须归一化，否则 8kHz 数据被播放器按 16kHz 消费，
     * 听感两倍速（板端 G711a 16kHz 加速事故的回归）。AAC 时钟保留传值。 */
    ServerImpl server;
    std::vector<StreamConfig> streams = MakeStreams();
    streams[0].audio_codec = ipc_rtsp::AudioCodec::G711A;
    streams[0].audio_clock_rate = 16000;
    streams[1].audio_codec = ipc_rtsp::AudioCodec::AAC;
    streams[1].audio_clock_rate = 16000;
    RTSP_CHECK(server.Configure(MakeConfig(), streams).ok());

    const ipc_rtsp::StreamConfig &main = server.hub(StreamId::Main)->config();
    RTSP_CHECK_EQ(static_cast<int>(main.audio_codec), static_cast<int>(ipc_rtsp::AudioCodec::G711A));
    RTSP_CHECK_EQ(static_cast<unsigned>(main.audio_clock_rate), 8000u);

    const ipc_rtsp::StreamConfig &sub = server.hub(StreamId::Sub)->config();
    RTSP_CHECK_EQ(static_cast<unsigned>(sub.audio_clock_rate), 16000u); /* AAC 不动 */
}

RTSP_TEST_CASE(server_config_g726_and_g711u_clock_normalized)
{
    /* 场景：上层误传 G711U/G726 采集采样率；预期配置期一律归一化到协议常量 8000。 */
    ServerImpl server;
    std::vector<StreamConfig> streams = MakeStreams();
    streams[0].audio_codec = ipc_rtsp::AudioCodec::G711U;
    streams[0].audio_clock_rate = 16000;
    streams[1].audio_codec = ipc_rtsp::AudioCodec::G726_32;
    streams[1].audio_clock_rate = 32000;
    RTSP_CHECK(server.Configure(MakeConfig(), streams).ok());

    RTSP_CHECK_EQ(static_cast<unsigned>(server.hub(StreamId::Main)->config().audio_clock_rate), 8000u);
    RTSP_CHECK_EQ(static_cast<unsigned>(server.hub(StreamId::Sub)->config().audio_clock_rate), 8000u);
}

RTSP_TEST_CASE(server_lifecycle_ready_restart_and_generation)
{
    ServerImpl server;
    RTSP_CHECK(server.Configure(MakeConfig(), MakeStreams()).ok());

    /* 未启动和已关停时 Shutdown 均幂等。 */
    RTSP_CHECK(server.Shutdown(std::chrono::milliseconds(0)).ok());
    std::uint64_t previous_generation = server.instance_generation();

    /* 快速重启回归：Start 返回时 loop 必须 ready，每代严格递增。 */
    constexpr int kIterations = 64;
    for (int i = 0; i < kIterations; ++i)
    {
        RTSP_CHECK(server.Start().ok());
        RTSP_CHECK(server.loop().running());
        RTSP_CHECK(server.instance_generation() > previous_generation);
        previous_generation = server.instance_generation();
        RTSP_CHECK(server.Shutdown(std::chrono::milliseconds(1000)).ok());
        RTSP_CHECK(!server.loop().running());
    }

    RTSP_CHECK(server.Shutdown(std::chrono::milliseconds(0)).ok());
}

RTSP_TEST_CASE(server_lifecycle_shutdown_reports_timeout_and_finishes_safely)
{
    ServerImpl server;
    RTSP_CHECK(server.Configure(MakeConfig(), MakeStreams()).ok());
    RTSP_CHECK(server.Start().ok());

    std::atomic<bool> blocker_entered{ false };
    server.loop().Post(
        [&blocker_entered]()
        {
            blocker_entered.store(true, std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        });
    RTSP_CHECK(WaitFor(
        [&blocker_entered]()
        {
            return blocker_entered.load(std::memory_order_acquire);
        }));

    const Result timed_out = server.Shutdown(std::chrono::milliseconds(1));
    RTSP_CHECK_EQ(timed_out.code, Status::Timeout);

    /* 首次返回 Timeout 后 I/O 线程继续安全收尾，第二次等待负责 join。 */
    RTSP_CHECK(server.Shutdown(std::chrono::milliseconds(1000)).ok());
    RTSP_CHECK(!server.loop().running());
}

RTSP_TEST_CASE(server_control_calls_are_serialized_on_io_thread)
{
    /* 场景：运行中连续发起控制面调用；预期在 I/O 线程串行执行，读回结果立即与最后一次更新一致。 */
    ServerImpl server;
    RTSP_CHECK(server.Configure(MakeConfig(), MakeStreams()).ok());
    RTSP_CHECK(server.Start().ok());

    for (int i = 0; i < 32; ++i)
    {
        const std::string user = "admin" + std::to_string(i);
        const std::string password = "pwd" + std::to_string(i);
        RTSP_CHECK(server.UpdateCredential(user, password).ok());
        RTSP_CHECK(server.Url(StreamId::Main, true).find(user + ":" + password + "@") != std::string::npos);

        std::vector<StreamConfig> streams = MakeStreams();
        streams[0].max_playing_clients = (i % 2) + 1;
        RTSP_CHECK(server.UpdateStreamConfig(streams).ok());
        (void) server.Snapshot();
    }

    RTSP_CHECK(server.Shutdown(std::chrono::milliseconds(1000)).ok());
}

RTSP_TEST_CASE(server_accept_rejects_connections_beyond_per_ip_limit)
{
    /* 回归：总连接配额之外无单 IP 限制，单点可占满全部配额拒绝服务。 */
    ServerImpl server;
    ServerConfig config = MakeConfig();
    config.max_connections_per_ip = 2;
    RTSP_CHECK(server.Configure(config, MakeStreams()).ok());
    RTSP_CHECK(server.Start().ok());

    /* config.port = 0 由内核分配，从 Url 解析实际监听端口。 */
    const std::string url = server.Url(StreamId::Main, false);
    const std::size_t scheme_end = url.find("rtsp://") + 7;
    const std::size_t colon = url.find(':', scheme_end);
    const std::size_t slash = url.find('/', colon);
    const int port = std::atoi(url.substr(colon + 1, slash - colon - 1).c_str());
    RTSP_CHECK(port > 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<std::uint16_t>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    const auto connect_once = [&addr]()
    {
        const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        RTSP_CHECK(fd >= 0);
        /* 非阻塞 connect 返回 EINPROGRESS 属预期。 */
        (void) ::connect(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof addr);
        return fd;
    };

    const int first = connect_once();
    const int second = connect_once();
    const int third = connect_once();

    /* 第三个连接超过单 IP 配额：服务端 accept 后立即关闭，客户端读到 EOF/错误。 */
    bool third_closed = false;
    for (int i = 0; i < 1000 && !third_closed; ++i)
    {
        char byte = 0;
        const ssize_t received = ::recv(third, &byte, 1, MSG_DONTWAIT);
        third_closed = (received == 0) || (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK);
        if (!third_closed)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    RTSP_CHECK(third_closed);

    /* 配额内的连接不受影响：对端未主动关闭（无可读数据、无挂断）。 */
    for (int fd : { first, second })
    {
        pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLIN;
        RTSP_CHECK_EQ(::poll(&pfd, 1, 0), 0);
        RTSP_CHECK_EQ(pfd.revents & (POLLIN | POLLHUP), 0);
    }

    ::close(first);
    ::close(second);
    ::close(third);
    RTSP_CHECK(server.Shutdown(std::chrono::milliseconds(1000)).ok());
}
