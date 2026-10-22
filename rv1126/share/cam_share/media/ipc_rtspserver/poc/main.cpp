/**
 * @FilePath     : main.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 主机验证程序：用离线 Annex-B 文件模拟 VENC，按帧率推送
 */

/*
 * 示例：
 *   ./rtspserver_poc --port 8554 --main main.h264 --sub sub.h265 --fps 25
 *   ffplay -rtsp_transport tcp rtsp://127.0.0.1:8554/Streaming/Channels/101
 *   ffplay -rtsp_transport udp rtsp://127.0.0.1:8554/Streaming/Channels/102
 */

#include "ipc_rtsp/log.h"
#include "ipc_rtsp/server.h"

#include "adts_source.h"
#include "media/file_source.h"
#include "media/jpeg_file_source.h"
#include "raw_audio_source.h"
#include "support/time.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

namespace
{

/* 主循环运行标志：信号到达后置 false。 */
std::atomic<bool> g_running{ true };

/** 信号处理：仅置位运行标志，不做异步不安全操作。 */
void OnSignal(int signal_number)
{
    (void) signal_number;
    g_running.store(false);
}

/** 简单命令行解析：--key value。
 *
 * @param argc 参数个数。
 * @param argv 参数数组。
 * @param key 选项名。
 * @param fallback 缺省值。
 * @return 命中的选项值；未命中返回 fallback。
 */
std::string ArgValue(int argc, char **argv, const char *key, const std::string &fallback)
{
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (std::strcmp(argv[i], key) == 0)
        {
            return argv[i + 1];
        }
    }
    return fallback;
}

/** 解析整型命令行选项（--key value）。
 *
 * @param argc 参数个数。
 * @param argv 参数数组。
 * @param key 选项名。
 * @param fallback 缺省值。
 * @return 命中的选项值；未命中或转换失败返回 fallback。
 */
int ArgInt(int argc, char **argv, const char *key, int fallback)
{
    const std::string value = ArgValue(argc, argv, key, std::string());
    return value.empty() ? fallback : std::atoi(value.c_str());
}

/** 库日志转发：按级别前缀打印到 stdout。
 *
 * @param user 未使用的用户上下文。
 * @param level 日志级别。
 * @param tag 日志标签。
 * @param message 日志内容。
 */
void LogSink(void *user, ipc_rtsp::LogLevel level, const char *tag, const char *message)
{
    (void) user;
    static const char *kNames[] = { "E", "W", "I", "D" };
    std::printf("[%s][%s] %s\n", kNames[static_cast<int>(level)], tag, message);
    std::fflush(stdout);
}

/** 按帧率推送一路码流（Annex-B H.264/H.265 或 Motion-JPEG 文件源）。
 *
 * @param server 目标服务实例。
 * @param stream_id 推送目标码流。
 * @param source 视频文件源。
 * @param fps 目标帧率。
 * @param pushed 成功推送计数（原子累加）。
 */
template <typename SourceT>
void PumpStream(ipc_rtsp::Server *server, ipc_rtsp::StreamId stream_id, SourceT *source, int fps, std::atomic<std::uint64_t> *pushed)
{
    const std::int64_t frame_interval_us = fps > 0 ? (1000000 / fps) : 40000;
    std::int64_t next_us = ipc_rtsp::detail::NowMonotonicUs();
    std::int64_t pts_us = 0;

    while (g_running.load())
    {
        ipc_rtsp::VideoFrameView frame;
        if (!source->NextAu(&frame))
        {
            break;
        }
        frame.pts_us = pts_us;
        /* 与板端产品路径一致（has_pts=false，hub 内用单调钟兜底时间轴）。 */
        frame.has_pts = false;
        pts_us += frame_interval_us;

        const ipc_rtsp::Result result = server->PushVideo(stream_id, frame);
        if (result.ok())
        {
            pushed->fetch_add(1);
        }

        next_us += frame_interval_us;
        const std::int64_t now_us = ipc_rtsp::detail::NowMonotonicUs();
        if (next_us > now_us)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(next_us - now_us));
        }
        else
        {
            /* 落后时重置节拍，避免累积追帧。 */
            next_us = now_us;
        }
    }
}

/** 按帧时长推送 AAC 音频（裸帧，已剥离 ADTS 头）。
 *
 * @param server 目标服务实例。
 * @param source ADTS 文件源。
 * @param pushed 成功推送计数（原子累加）。
 */
void PumpAudio(ipc_rtsp::Server *server, ipc_rtsp::detail::AdtsFileSource *source, std::atomic<std::uint64_t> *pushed)
{
    const std::int64_t frame_us = 1024 * 1000000LL / (source->sample_rate() > 0 ? source->sample_rate() : 16000);
    std::int64_t next_us = ipc_rtsp::detail::NowMonotonicUs();

    while (g_running.load())
    {
        const std::uint8_t *payload = nullptr;
        std::size_t size = 0;
        if (!source->NextFrame(&payload, &size))
        {
            break;
        }
        ipc_rtsp::AudioFrameView frame;
        frame.data = payload;
        frame.size = size;
        frame.codec = ipc_rtsp::AudioCodec::AAC;
        /* 与板端口径一致：不传 PTS，由库内以入队单调钟统一时间轴
         * （音视频必须同轴，cache 回放对齐与播放端音画同步都依赖它）。 */
        frame.has_pts = false;
        frame.pts_us = 0;

        if (server->PushAudio(ipc_rtsp::StreamId::Main, frame).ok())
        {
            pushed->fetch_add(1);
        }

        next_us += frame_us;
        const std::int64_t now_us = ipc_rtsp::detail::NowMonotonicUs();
        if (next_us > now_us)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(next_us - now_us));
        }
        else
        {
            next_us = now_us;
        }
    }
}

/** 按帧时长推送定长裸帧音频（G711a/G711u/G726）。
 *
 * @param server 目标服务实例。
 * @param source 裸音频文件源。
 * @param codec 音频编码。
 * @param frame_us 单帧时长（微秒）。
 * @param pushed 成功推送计数（原子累加）。
 */
void PumpRawAudio(ipc_rtsp::Server *server,
                  ipc_rtsp::detail::RawAudioFileSource *source,
                  ipc_rtsp::AudioCodec codec,
                  std::int64_t frame_us,
                  std::atomic<std::uint64_t> *pushed)
{
    std::int64_t next_us = ipc_rtsp::detail::NowMonotonicUs();
    std::int64_t pts_us = 0;

    while (g_running.load())
    {
        const std::uint8_t *payload = nullptr;
        std::size_t size = 0;
        if (!source->NextFrame(&payload, &size))
        {
            break;
        }
        ipc_rtsp::AudioFrameView frame;
        frame.data = payload;
        frame.size = size;
        frame.codec = codec;
        frame.pts_us = pts_us;
        frame.has_pts = true;
        pts_us += frame_us;

        if (server->PushAudio(ipc_rtsp::StreamId::Main, frame).ok())
        {
            pushed->fetch_add(1);
        }

        next_us += frame_us;
        const std::int64_t now_us = ipc_rtsp::detail::NowMonotonicUs();
        if (next_us > now_us)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(next_us - now_us));
        }
        else
        {
            next_us = now_us;
        }
    }
}

} // namespace

int main(int argc, char **argv)
{
    const int port = ArgInt(argc, argv, "--port", 8554);
    const int fps = ArgInt(argc, argv, "--fps", 25);
    const std::string main_path = ArgValue(argc, argv, "--main", std::string());
    const std::string sub_path = ArgValue(argc, argv, "--sub", std::string());
    const std::string audio_path = ArgValue(argc, argv, "--audio", std::string());
    const std::string audio_format = ArgValue(argc, argv, "--audio-format", "aac");
    const std::string codec_name = ArgValue(argc, argv, "--codec", "h264");
    /* 子码流 codec：默认跟随 --codec；测试"主 H264 + 子 H265"组合时用 --sub-codec h265。 */
    const std::string sub_codec_name = ArgValue(argc, argv, "--sub-codec", codec_name);
    const std::string user = ArgValue(argc, argv, "--user", std::string());
    const std::string password = ArgValue(argc, argv, "--password", std::string());
    const std::string bind_ip = ArgValue(argc, argv, "--bind", "0.0.0.0");
    const std::string advertised = ArgValue(argc, argv, "--advertised", std::string("127.0.0.1"));
    const bool verbose = ArgValue(argc, argv, "--verbose", std::string()) == "1";

    if (main_path.empty() && sub_path.empty())
    {
        std::fprintf(stderr,
                     "用法: %s --main <h264/h265/mjpeg文件> [--sub <文件>] [--audio <音频文件>] [--port 8554] [--fps 25]\n"
                     "        [--codec h264|h265|mjpeg] [--audio-format aac|alaw|ulaw|g726] [--user admin --password pwd] [--verbose 1]\n",
                     argv[0]);
        return EXIT_FAILURE;
    }

    ipc_rtsp::Codec main_codec = ipc_rtsp::Codec::H264;
    if (codec_name == "h265")
    {
        main_codec = ipc_rtsp::Codec::H265;
    }
    else if (codec_name == "mjpeg")
    {
        main_codec = ipc_rtsp::Codec::MJPEG;
    }

    ipc_rtsp::Codec sub_codec = main_codec;
    if (sub_codec_name == "h264")
    {
        sub_codec = ipc_rtsp::Codec::H264;
    }
    else if (sub_codec_name == "h265")
    {
        sub_codec = ipc_rtsp::Codec::H265;
    }
    else if (sub_codec_name == "mjpeg")
    {
        sub_codec = ipc_rtsp::Codec::MJPEG;
    }

    ipc_rtsp::detail::AnnexBFileSource main_source;
    ipc_rtsp::detail::AnnexBFileSource sub_source;
    ipc_rtsp::detail::JpegFileSource jpeg_source;
    ipc_rtsp::detail::AdtsFileSource aac_source;
    ipc_rtsp::detail::RawAudioFileSource raw_audio_source;

    if (!main_path.empty())
    {
        const ipc_rtsp::Result loaded = (main_codec == ipc_rtsp::Codec::MJPEG) ? jpeg_source.Load(main_path)
                                                                               : main_source.Load(main_path, main_codec);
        if (!loaded.ok())
        {
            std::fprintf(stderr, "加载主码流失败: %s\n", main_path.c_str());
            return EXIT_FAILURE;
        }
    }
    if (!sub_path.empty())
    {
        const ipc_rtsp::Result loaded = sub_codec == ipc_rtsp::Codec::MJPEG ? jpeg_source.Load(sub_path)
                                                                            : sub_source.Load(sub_path, sub_codec);
        if (!loaded.ok())
        {
            std::fprintf(stderr, "加载子码流失败: %s\n", sub_path.c_str());
            return EXIT_FAILURE;
        }
    }

    ipc_rtsp::SetLogSink(&LogSink, nullptr);
    ipc_rtsp::SetLogLevel(verbose ? ipc_rtsp::LogLevel::Debug : ipc_rtsp::LogLevel::Info);

    ipc_rtsp::ServerConfig config;
    config.port = static_cast<std::uint16_t>(port);
    config.bind_address = bind_ip;
    config.advertised_ip = advertised;
    config.max_connections = 8;
    config.max_playing_clients = 4;
    if (!user.empty())
    {
        config.auth.mode = ipc_rtsp::AuthMode::Digest;
        config.auth.user = user;
        config.auth.password = password;
    }

    std::vector<ipc_rtsp::StreamConfig> streams(2);
    streams[0].id = ipc_rtsp::StreamId::Main;
    streams[0].path = "Streaming/Channels/101";
    streams[0].codec = main_codec;
    streams[0].fps = fps;
    streams[0].max_frame_bytes = 2u * 1024u * 1024u;
    if (!audio_path.empty())
    {
        const ipc_rtsp::Result loaded = aac_source.Load(audio_path.c_str());
        const bool is_aac = (audio_format == "aac");
        if (is_aac && !loaded.ok())
        {
            std::fprintf(stderr, "加载音频失败: %s\n", audio_path.c_str());
            return EXIT_FAILURE;
        }
        if (!is_aac)
        {
            const ipc_rtsp::Result raw_loaded = raw_audio_source.Load(audio_path.c_str());
            if (!raw_loaded.ok())
            {
                std::fprintf(stderr, "加载音频失败: %s\n", audio_path.c_str());
                return EXIT_FAILURE;
            }
            /* 板端 AENC 常规参数：8kHz、25fps（40ms 帧）→ G711 每帧 320 字节、G726-32 每帧 160 字节。 */
            raw_audio_source.set_frame_bytes(audio_format == "g726" ? 160 : 320);
        }

        streams[0].audio_enabled = true;
        if (is_aac)
        {
            streams[0].audio_codec = ipc_rtsp::AudioCodec::AAC;
            streams[0].audio_payload_type = 97;
            streams[0].audio_clock_rate = aac_source.sample_rate() > 0 ? aac_source.sample_rate() : 16000;
            streams[0].audio_channels = aac_source.channels() > 0 ? aac_source.channels() : 1;
        }
        else if (audio_format == "ulaw")
        {
            streams[0].audio_codec = ipc_rtsp::AudioCodec::G711U;
            streams[0].audio_payload_type = 0; /* PCMU 静态 PT 0 */
            streams[0].audio_clock_rate = 8000;
        }
        else if (audio_format == "g726")
        {
            streams[0].audio_codec = ipc_rtsp::AudioCodec::G726_32;
            streams[0].audio_payload_type = 97; /* G726-32 用动态 PT */
            streams[0].audio_clock_rate = 8000;
        }
        else
        {
            streams[0].audio_codec = ipc_rtsp::AudioCodec::G711A;
            streams[0].audio_payload_type = 8; /* PCMA 静态 PT 8 */
            streams[0].audio_clock_rate = 8000;
        }
    }
    streams[1].id = ipc_rtsp::StreamId::Sub;
    streams[1].path = "Streaming/Channels/102";
    streams[1].codec = sub_codec;
    streams[1].fps = fps;
    streams[1].max_frame_bytes = 1u * 1024u * 1024u;

    /* 对照实验开关：--no-gop 时关闭 cache（回退 WaitStart+IDR 旧起播路径）。 */
    if (ArgValue(argc, argv, "--no-gop", "0") != "1")
    {
        streams[0].gop_cache.max_bytes = 4u * 1024u * 1024u;
        streams[0].gop_cache.max_frames = 300;
        streams[0].gop_cache.max_duration_ms = 3000;
        streams[1].gop_cache.max_bytes = 1u * 1024u * 1024u;
        streams[1].gop_cache.max_frames = 300;
        streams[1].gop_cache.max_duration_ms = 3000;
    }

    std::unique_ptr<ipc_rtsp::Server> server;
    ipc_rtsp::Result result = ipc_rtsp::Server::Create(config, streams, &server);
    if (!result.ok())
    {
        std::fprintf(stderr, "创建服务失败: %s\n", ipc_rtsp::StatusToString(result.code));
        return EXIT_FAILURE;
    }

    server->SetIdrRequestCallback(
        [](ipc_rtsp::StreamId id)
        {
            /* 板端应在此投递到编码线程请求 IDR；PoC 里文件源天然周期出现关键帧。 */
            std::printf("[I][poc] 收到IDR请求 stream:%d\n", static_cast<int>(id));
        });

    result = server->Start();
    if (!result.ok())
    {
        std::fprintf(stderr, "启动失败: %s (errno=%d)\n", ipc_rtsp::StatusToString(result.code), result.sys_errno);
        return EXIT_FAILURE;
    }

    std::printf("主码流: %s\n", server->Url(ipc_rtsp::StreamId::Main, false).c_str());
    std::printf("子码流: %s\n", server->Url(ipc_rtsp::StreamId::Sub, false).c_str());

    std::signal(SIGINT, &OnSignal);
    std::signal(SIGTERM, &OnSignal);

    std::atomic<std::uint64_t> main_pushed{ 0 };
    std::atomic<std::uint64_t> sub_pushed{ 0 };
    std::atomic<std::uint64_t> audio_pushed{ 0 };

    std::thread main_thread;
    std::thread sub_thread;
    std::thread audio_thread;
    if (!main_path.empty())
    {
        if (main_codec == ipc_rtsp::Codec::MJPEG)
        {
            main_thread = std::thread(PumpStream<ipc_rtsp::detail::JpegFileSource>,
                                      server.get(),
                                      ipc_rtsp::StreamId::Main,
                                      &jpeg_source,
                                      fps,
                                      &main_pushed);
        }
        else
        {
            main_thread = std::thread(PumpStream<ipc_rtsp::detail::AnnexBFileSource>,
                                      server.get(),
                                      ipc_rtsp::StreamId::Main,
                                      &main_source,
                                      fps,
                                      &main_pushed);
        }
    }
    if (!sub_path.empty())
    {
        if (sub_codec == ipc_rtsp::Codec::MJPEG)
        {
            sub_thread = std::thread(PumpStream<ipc_rtsp::detail::JpegFileSource>,
                                     server.get(),
                                     ipc_rtsp::StreamId::Sub,
                                     &jpeg_source,
                                     fps,
                                     &sub_pushed);
        }
        else
        {
            sub_thread = std::thread(PumpStream<ipc_rtsp::detail::AnnexBFileSource>,
                                     server.get(),
                                     ipc_rtsp::StreamId::Sub,
                                     &sub_source,
                                     fps,
                                     &sub_pushed);
        }
    }
    if (!audio_path.empty())
    {
        if (audio_format == "aac")
        {
            audio_thread = std::thread(PumpAudio, server.get(), &aac_source, &audio_pushed);
        }
        else
        {
            /* G711 320B@8kHz 与 G726-32 160B@32kbps 的帧时长同为 40ms。 */
            audio_thread = std::thread(PumpRawAudio, server.get(), &raw_audio_source, streams[0].audio_codec, 40000, &audio_pushed);
        }
    }

    /* 每 5 秒打印一次指标，便于观察队列/丢帧/拥塞。 */
    while (g_running.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        const ipc_rtsp::MetricsSnapshot snapshot = server->Snapshot();
        std::printf("[I][poc] 连接:%llu/%llu 会话:%llu 播放:%llu TCP:%llu UDP:%llu 收帧:%llu/%llu 丢帧:%llu/%llu 队列峰值:%lluB\n",
                    static_cast<unsigned long long>(snapshot.connections_active),
                    static_cast<unsigned long long>(snapshot.connections_accepted),
                    static_cast<unsigned long long>(snapshot.sessions_active),
                    static_cast<unsigned long long>(snapshot.playing_clients),
                    static_cast<unsigned long long>(snapshot.tcp_clients),
                    static_cast<unsigned long long>(snapshot.udp_clients),
                    static_cast<unsigned long long>(snapshot.streams[0].pushed_frames),
                    static_cast<unsigned long long>(snapshot.streams[1].pushed_frames),
                    static_cast<unsigned long long>(snapshot.streams[0].dropped_frames),
                    static_cast<unsigned long long>(snapshot.streams[1].dropped_frames),
                    static_cast<unsigned long long>(snapshot.streams[0].queue_high_water_bytes));
        std::printf("[I][poc] GOP主: 开:%llu 帧:%llu/%lluB 代:%llu 建:%llu 失效:%llu 回放:%llu 中止:%llu GOP子: 帧:%llu/%lluB\n",
                    static_cast<unsigned long long>(snapshot.streams[0].gop_cache_enabled),
                    static_cast<unsigned long long>(snapshot.streams[0].gop_cache_frames),
                    static_cast<unsigned long long>(snapshot.streams[0].gop_cache_bytes),
                    static_cast<unsigned long long>(snapshot.streams[0].gop_epoch),
                    static_cast<unsigned long long>(snapshot.streams[0].gop_started),
                    static_cast<unsigned long long>(snapshot.streams[0].gop_invalidated),
                    static_cast<unsigned long long>(snapshot.streams[0].gop_replays),
                    static_cast<unsigned long long>(snapshot.streams[0].gop_replay_aborts),
                    static_cast<unsigned long long>(snapshot.streams[1].gop_cache_frames),
                    static_cast<unsigned long long>(snapshot.streams[1].gop_cache_bytes));
        std::fflush(stdout);
    }

    std::printf("[I][poc] 正在停止...\n");
    if (main_thread.joinable())
    {
        main_thread.join();
    }
    if (sub_thread.joinable())
    {
        sub_thread.join();
    }
    if (audio_thread.joinable())
    {
        audio_thread.join();
    }
    (void) server->Shutdown(std::chrono::milliseconds(2000));
    std::printf("[I][poc] 已停止，主码流推送帧数:%llu 子码流:%llu\n",
                static_cast<unsigned long long>(main_pushed.load()),
                static_cast<unsigned long long>(sub_pushed.load()));
    return EXIT_SUCCESS;
}
