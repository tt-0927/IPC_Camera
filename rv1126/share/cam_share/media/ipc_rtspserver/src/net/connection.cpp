/**
 * @FilePath     : connection.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 11:35:00
 * @Description  : RTSP 连接实现
 */

#include "net/connection.h"

#include "media/annexb_scanner.h"
#include "media/jpeg_parser.h"
#include "net/udp_socket.h"
#include "rtp/audio_payload.h"
#include "server_impl.h"
#include "support/log.h"
#include "support/time.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr const char *kTag = "conn";
constexpr std::uint16_t kStatusBadRequest = 400;
constexpr std::uint16_t kStatusServiceUnavailable = 503;

/** 参数集补发缓冲：VPS+SPS+PPS，含起始码。 */
constexpr std::size_t kParameterSetBufferBytes = 3u * (kMaxParameterSetBytes + 4u);

/** RTCP SR 周期（RFC 3550 建议 5s）：live555/VLC 依赖 SR 的 NTP↔RTP 映射建立播放时钟。 */
constexpr std::uint32_t kRtcpSrIntervalMs = 5000;
} // namespace

Connection::Connection(ServerImpl *server, EventLoop *loop, int fd, std::string peer_ip, std::uint16_t peer_port)
    : server_(server), loop_(loop), fd_(fd), peer_ip_(std::move(peer_ip)), peer_port_(peer_port)
{
    const ServerConfig &cfg = server_->config();
    output_.Configure(cfg.backpressure.client_output_guard_bytes, cfg.backpressure.client_control_reserve_bytes);
    input_.resize(cfg.backpressure.max_input_buffer_bytes);

    now_ms_ = NowMonotonicMs();
    last_activity_ms_ = now_ms_;
    session_.last_activity_ms = now_ms_;
    stats_.disconnect_reason = DisconnectReason::Unknown;

    controller_ = std::unique_ptr<Controller>(new Controller(this));
    auth_.nonce.Rotate();
}

Connection::~Connection()
{
    /* 释放链（与申请相反的顺序）：track/会话 → response → parser → writer → fd。 */
    TeardownTracks();
    session_.Reset();

    if (response_ != nullptr)
    {
        ipc_rtsp_shim_response_free(response_);
        response_ = nullptr;
    }
    if (parser_ != nullptr)
    {
        ipc_rtsp_shim_parser_free(parser_);
        parser_ = nullptr;
    }
    if (control_writer_ != nullptr)
    {
        ipc_rtsp_shim_writer_free(control_writer_);
        control_writer_ = nullptr;
    }
    if (media_writer_ != nullptr)
    {
        ipc_rtsp_shim_writer_free(media_writer_);
        media_writer_ = nullptr;
    }
    if (fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
}

Result Connection::Start()
{
    control_writer_ = ipc_rtsp_shim_writer_new(&Connection::WriteControlThunk, &Connection::FilledThunk, this);
    media_writer_ = ipc_rtsp_shim_writer_new(&Connection::WriteMediaThunk, &Connection::FilledThunk, this);
    parser_ = ipc_rtsp_shim_parser_new();
    if (control_writer_ == nullptr || media_writer_ == nullptr || parser_ == nullptr)
    {
        return Result::Fail(Status::Internal);
    }

    const Result added = loop_->Add(fd_,
                                    EPOLLIN,
                                    [this](std::uint32_t events)
                                    {
                                        now_ms_ = NowMonotonicMs();
                                        if ((events & EPOLLIN) != 0u)
                                        {
                                            OnReadable();
                                        }
                                        if (!closing_ && (events & EPOLLOUT) != 0u)
                                        {
                                            OnWritable();
                                        }
                                        if (!closing_ && (events & (EPOLLERR | EPOLLHUP)) != 0u)
                                        {
                                            /* 先尝试处理可读数据（对端可能已发完 TEARDOWN），再关闭。 */
                                            OnReadable();
                                            if (!closing_)
                                            {
                                                Close(DisconnectReason::ConnectionClosed);
                                            }
                                        }
                                    });
    if (!added.ok())
    {
        return added;
    }
    registered_ = true;
    ResetIdleTimer();
    return Result::Ok();
}

ServerImpl &Connection::server()
{
    return *server_;
}

void Connection::Close(DisconnectReason reason)
{
    if (closing_)
    {
        return;
    }
    /* Close 时序：撤定时器 → TeardownTracks → 摘除事件注册 → close(fd) → 通知
     * server 回收；对象本体由事件循环本轮结束后的回收阶段删除，这里只做状态收尾。 */
    closing_ = true;
    stats_.congested_ms += congestion_.Leave(NowMonotonicMs());
    disconnect_reason_ = reason;
    stats_.disconnect_reason = reason;

    if (loop_ != nullptr)
    {
        loop_->CancelTimer(idle_timer_);
        loop_->CancelTimer(describe_timer_);
        idle_timer_ = 0;
        describe_timer_ = 0;
    }

    if (ever_played_)
    {
        IPC_RTSP_LOGI(kTag,
                      "客户端断开 客户端:%s 原因:%s rtp包:%llu 丢帧:%llu 拥塞:%llums",
                      peer_ip_.c_str(),
                      DisconnectReasonToString(reason),
                      static_cast<unsigned long long>(stats_.rtp_packets),
                      static_cast<unsigned long long>(stats_.dropped_frames),
                      static_cast<unsigned long long>(stats_.congested_ms));
    }

    TeardownTracks();
    session_.Reset();
    pending_describe_ = false;

    if (registered_ && loop_ != nullptr)
    {
        loop_->Remove(fd_);
        registered_ = false;
    }
    if (fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
    }

    if (server_ != nullptr)
    {
        server_->NoteConnectionClosed(this);
    }
}

void Connection::MarkActivity()
{
    now_ms_ = NowMonotonicMs();
    last_activity_ms_ = now_ms_;
    session_.last_activity_ms = now_ms_;
}

std::uint32_t Connection::NoteAuthFailure()
{
    ++auth_.failures;
    server_->counters().auth_failures.fetch_add(1, std::memory_order_relaxed);
    return auth_.failures;
}

void Connection::NoteAuthSuccess()
{
    auth_.authenticated = true;
    auth_.failures = 0;
}

void Connection::ResetIdleTimer()
{
    if (loop_ == nullptr)
    {
        return;
    }
    if (idle_timer_ != 0)
    {
        loop_->CancelTimer(idle_timer_);
    }
    const std::int64_t interval_ms = std::max<std::int64_t>(1000, server_->config().request_timeout_ms / 2);
    idle_timer_ = loop_->AddTimer(NowMonotonicMs() + interval_ms,
                                  [this]()
                                  {
                                      OnIdleCheck();
                                  });
}

void Connection::OnIdleCheck()
{
    idle_timer_ = 0;
    if (closing_)
    {
        return;
    }

    now_ms_ = NowMonotonicMs();
    const ServerConfig &cfg = server_->config();

    if (session_.any_playing())
    {
        if ((now_ms_ - session_.last_activity_ms) > static_cast<std::int64_t>(cfg.session_timeout_ms))
        {
            Close(DisconnectReason::SessionTimeout);
            return;
        }
    }
    else if ((now_ms_ - last_activity_ms_) > static_cast<std::int64_t>(cfg.request_timeout_ms))
    {
        Close(DisconnectReason::IdleTimeout);
        return;
    }

    ResetIdleTimer();
}

ssize_t Connection::WriteControlThunk(void *user, const void *data, std::size_t len)
{
    Connection *self = static_cast<Connection *>(user);
    if (self == nullptr || self->closing_)
    {
        return -1;
    }
    return self->AppendControlToOutput(data, len) ? static_cast<ssize_t>(len) : -1;
}

ssize_t Connection::WriteMediaThunk(void *user, const void *data, std::size_t len)
{
    Connection *self = static_cast<Connection *>(user);
    if (self == nullptr || self->closing_)
    {
        return -1;
    }
    return self->AppendMediaToOutput(data, len) ? static_cast<ssize_t>(len) : -1;
}

std::size_t Connection::FilledThunk(void *user)
{
    const Connection *self = static_cast<const Connection *>(user);
    return self == nullptr ? 0u : self->output_.size();
}

bool Connection::AppendControlToOutput(const void *data, std::size_t len)
{
    return output_.AppendControl(data, len);
}

bool Connection::AppendMediaToOutput(const void *data, std::size_t len)
{
    return output_.AppendMedia(data, len);
}

void Connection::Respond(std::uint16_t code, const char *reason)
{
    if (closing_)
    {
        return;
    }
    if (response_ == nullptr)
    {
        IPC_RTSP_LOGE(kTag, "缺少响应上下文 客户端:%s", peer_ip_.c_str());
        Close(DisconnectReason::Internal);
        return;
    }

    /* 先记事务点：序列化/写出失败可整体回滚，避免半包响应留在输出缓冲。 */
    const std::size_t checkpoint = output_.Checkpoint();
    const ssize_t written = ipc_rtsp_shim_response_send(response_, code, reason);
    if (written < 0)
    {
        /* 序列化中途失败：回滚事务点丢弃半截响应，再按写拥塞断开。 */
        output_.Rollback(checkpoint);
        IPC_RTSP_LOGW(kTag, "响应写出失败 客户端:%s 状态:%u", peer_ip_.c_str(), static_cast<unsigned>(code));
        Close(DisconnectReason::OutputCongested);
        return;
    }
    FlushOutput();
}

void Connection::AddHeader(const char *name, const std::string &value)
{
    if (response_ == nullptr)
    {
        return;
    }
    ipc_rtsp_shim_response_header(response_, name, value.c_str());
}

void Connection::SetBody(const std::string &body)
{
    if (response_ == nullptr)
    {
        return;
    }
    ipc_rtsp_shim_response_body(response_, body.data(), body.size());
}

void Connection::UpdateInterest()
{
    if (!registered_ || loop_ == nullptr || closing_)
    {
        return;
    }
    std::uint32_t events = EPOLLIN;
    if (!output_.empty())
    {
        events |= EPOLLOUT;
    }
    const Result updated = loop_->Modify(fd_, events);
    if (!updated.ok())
    {
        /* 关注更新失败意味着写事件可能不再投递，输出缓冲将无法排空；
         * 与其等拥塞/空闲超时兜底，直接按 IO 错误回收该连接。 */
        IPC_RTSP_LOGW(kTag, "事件关注更新失败 客户端:%s 状态:%s", peer_ip_.c_str(), StatusToString(updated.code));
        Close(DisconnectReason::IoError);
    }
}

void Connection::FlushOutput()
{
    if (closing_ || fd_ < 0)
    {
        return;
    }

    const ssize_t written = output_.FlushTo(fd_);
    if (written < 0)
    {
        /* 负返回值为 -errno：socket 已不可写（对端 RST 等），按 IO 错误关闭。 */
        Close(DisconnectReason::IoError);
        return;
    }
    if (written > 0)
    {
        /* 仅实际写出字节才刷新拥塞的无进展计时（NoteProgress 语义）。 */
        server_->counters().tx_bytes.fetch_add(static_cast<std::uint64_t>(written), std::memory_order_relaxed);
        congestion_.NoteProgress(NowMonotonicMs(), static_cast<std::size_t>(written));
    }
    stats_.output_high_water_bytes = std::max(stats_.output_high_water_bytes, static_cast<std::uint64_t>(output_.high_water()));

    UpdateInterest();
}

void Connection::OnWritable()
{
    FlushOutput();
    if (!closing_)
    {
        FlushMedia();
    }
}

void Connection::OnReadable()
{
    if (closing_)
    {
        return;
    }

    for (;;)
    {
        if (input_len_ >= input_.size())
        {
            IPC_RTSP_LOGW(kTag, "输入缓冲已满 客户端:%s 字节:%zu", peer_ip_.c_str(), input_len_);
            Close(DisconnectReason::InputOverflow);
            return;
        }

        const ssize_t received = ::recv(fd_, input_.data() + input_len_, input_.size() - input_len_, 0);
        if (received > 0)
        {
            input_len_ += static_cast<std::size_t>(received);
            continue;
        }
        if (received == 0)
        {
            /* 对端半关闭：先把已收数据处理完（可能含最后一个请求），再关闭。 */
            ProcessInput();
            if (!closing_)
            {
                Close(DisconnectReason::ConnectionClosed);
            }
            return;
        }
        if (errno == EINTR)
        {
            /* 信号中断：重试本次读取。 */
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            /* 内核接收缓冲已读空：正常退出循环，等下一轮 EPOLLIN。 */
            break;
        }
        /* 其余 errno：真实读错误，按 IO 错误关闭。 */
        Close(DisconnectReason::IoError);
        return;
    }

    ProcessInput();
}

void Connection::ProcessInput()
{
    /* 消费缓冲头部后把剩余数据前移。 */
    auto consume = [this](std::size_t count)
    {
        if (count == 0 || count > input_len_)
        {
            return;
        }
        input_len_ -= count;
        if (input_len_ > 0)
        {
            std::memmove(input_.data(), input_.data() + count, input_len_);
        }
    };

    const std::size_t max_request = server_->config().backpressure.max_request_bytes;

    for (;;)
    {
        if (closing_ || input_len_ == 0)
        {
            return;
        }

        /* 1) TCP interleaved（客户端发来的 RTCP RR 等）。 */
        if (input_[0] == '$')
        {
            std::uint8_t channel = 0;
            const void *payload = nullptr;
            std::size_t payload_len = 0;
            std::size_t consumed = 0;
            const int status = ipc_rtsp_shim_parse_interleaved(input_.data(), input_len_, &channel, &payload, &payload_len, &consumed);
            if (status == IPC_RTSP_SHIM_IL_COMPLETE && consumed > 0)
            {
                HandleInterleaved(channel, static_cast<const std::uint8_t *>(payload), payload_len);
                consume(consumed);
                continue;
            }
            if (status == IPC_RTSP_SHIM_IL_PARTIAL)
            {
                /* 半包：等待后续字节；同样受请求/输入上限保护。 */
                if (input_len_ >= max_request)
                {
                    Close(DisconnectReason::InputOverflow);
                }
                return;
            }
        }

        /* 2) RTSP 请求。 */
        ipc_rtsp_shim_request *request = nullptr;
        std::size_t consumed = 0;
        const int status = ipc_rtsp_shim_parser_run(parser_, input_.data(), input_len_, &consumed, &request);

        if (status == IPC_RTSP_SHIM_PARSE_PARTIAL)
        {
            if (input_len_ >= max_request)
            {
                IPC_RTSP_LOGW(kTag, "请求超过上限 客户端:%s 字节:%zu 上限:%zu", peer_ip_.c_str(), input_len_, max_request);
                server_->counters().parse_failures.fetch_add(1, std::memory_order_relaxed);
                response_ = ipc_rtsp_shim_response_new_with_cseq(0, control_writer_);
                AddHeader("Connection", "close");
                Respond(kStatusBadRequest, "Bad Request");
                if (response_ != nullptr)
                {
                    ipc_rtsp_shim_response_free(response_);
                    response_ = nullptr;
                }
                Close(DisconnectReason::ParseError);
            }
            return;
        }

        if (status == IPC_RTSP_SHIM_PARSE_FAILURE)
        {
            server_->counters().parse_failures.fetch_add(1, std::memory_order_relaxed);
            IPC_RTSP_LOGW(kTag, "请求解析失败 客户端:%s", peer_ip_.c_str());
            response_ = ipc_rtsp_shim_response_new_with_cseq(0, control_writer_);
            AddHeader("Connection", "close");
            Respond(kStatusBadRequest, "Bad Request");
            if (response_ != nullptr)
            {
                ipc_rtsp_shim_response_free(response_);
                response_ = nullptr;
            }
            Close(DisconnectReason::ParseError);
            return;
        }

        if (consumed == 0)
        {
            /* 防御：解析器报告完成但没有消费字节，避免死循环。 */
            IPC_RTSP_LOGE(kTag, "解析器未消费字节 客户端:%s", peer_ip_.c_str());
            Close(DisconnectReason::Internal);
            return;
        }

        current_request_ = request;
        response_ = ipc_rtsp_shim_response_new(request, control_writer_);
        if (response_ == nullptr)
        {
            current_request_ = nullptr;
            Close(DisconnectReason::Internal);
            return;
        }

        (void) controller_->Handle(request);

        if (response_ != nullptr)
        {
            ipc_rtsp_shim_response_free(response_);
            response_ = nullptr;
        }
        current_request_ = nullptr;

        consume(consumed);
        FlushOutput();

        if (closing_)
        {
            return;
        }
    }
}

void Connection::HandleInterleaved(std::uint8_t channel, const std::uint8_t *payload, std::size_t size)
{
    (void) payload;
    (void) size;

    TrackRuntime *track = session_.FindByRtcpChannel(channel);
    if (track != nullptr)
    {
        ++stats_.rtcp_packets;
        server_->counters().rtcp_packets.fetch_add(1, std::memory_order_relaxed);
    }
    /* RTCP RR 内容在 v1 不做统计，仅作为会话活动（部分客户端用它保活）。 */
    MarkActivity();
}

Result Connection::EstablishTransport(TrackRuntime &track, const ipc_rtsp_shim_transport &transport)
{
    FrameHub *hub = server_->hub(track.stream);
    if (hub == nullptr)
    {
        return Result::Fail(Status::NotFound);
    }

    const StreamConfig &stream_config = hub->config();
    const ServerConfig &cfg = server_->config();
    const std::uint32_t ssrc = RandomU32();
    const std::uint64_t ts_base_us = RandomU64() & 0xffffffffu;
    const bool is_video = (track.index == kTrackVideo);
    const std::uint8_t payload_type = is_video ? stream_config.payload_type : stream_config.audio_payload_type;
    const std::uint32_t clock_rate = is_video ? stream_config.clock_rate : stream_config.audio_clock_rate;
    const Codec codec = is_video ? stream_config.codec : Codec::Unknown;

    std::string cname = "ipc@";
    cname += server_->AdvertisedIp();

    if (transport.lower_transport == IPC_RTSP_SHIM_LOWER_TCP)
    {
        track.kind = TransportKind::TcpInterleaved;
        track.rtp_channel = transport.rtp_channel;
        track.rtcp_channel = transport.rtcp_channel;

        /* 发送器字节水位 = 媒体区容量：仅用于打包循环的停止条件（媒体区将满），
         * 拥塞判定本身按积压时长，不使用该水位。 */
        const std::size_t media_guard = cfg.backpressure.client_output_guard_bytes > cfg.backpressure.client_control_reserve_bytes
                                            ? cfg.backpressure.client_output_guard_bytes - cfg.backpressure.client_control_reserve_bytes
                                            : cfg.backpressure.client_output_guard_bytes;
        track.sender.reset(RtpSender::CreateTcp(media_writer_,
                                                track.rtp_channel,
                                                media_guard,
                                                ssrc,
                                                payload_type,
                                                clock_rate,
                                                ts_base_us,
                                                codec,
                                                stream_config.max_nalu_bytes));
        if (!track.sender)
        {
            return Result::Fail(Status::NoCapacity);
        }
        track.rtcp.Configure(track.sender.get(), true, track.rtcp_channel, media_writer_, -1, cname);
        track.server_rtp_port = 0;
        track.server_rtcp_port = 0;
        track.setup = true;
        return Result::Ok();
    }

    /* UDP：RTP 与 RTCP 各一个已 connect 的 socket。 */
    if (!server_->TryAcquireUdpTrack())
    {
        IPC_RTSP_LOGW(kTag, "UDP轨道数已达上限 客户端:%s", peer_ip_.c_str());
        return Result::Fail(Status::NoCapacity);
    }

    int rtp_fd = -1;
    int rtcp_fd = -1;
    std::uint16_t rtp_port = 0;
    std::uint16_t rtcp_port = 0;

    Result created = UdpSocket::CreateConnected(peer_ip_, transport.client_rtp_port, cfg.dscp, &rtp_fd, &rtp_port);
    if (!created.ok())
    {
        server_->ReleaseUdpTrack();
        return created;
    }
    created = UdpSocket::CreateConnected(peer_ip_, transport.client_rtcp_port, cfg.dscp, &rtcp_fd, &rtcp_port);
    if (!created.ok())
    {
        ::close(rtp_fd);
        server_->ReleaseUdpTrack();
        return created;
    }

    track.kind = TransportKind::Udp;
    track.sender.reset(RtpSender::CreateUdp(rtp_fd, ssrc, payload_type, clock_rate, ts_base_us, codec, stream_config.max_nalu_bytes));
    if (!track.sender)
    {
        ::close(rtcp_fd);
        server_->ReleaseUdpTrack();
        return Result::Fail(Status::Internal);
    }

    track.rtcp_fd = rtcp_fd;
    track.server_rtp_port = rtp_port;
    track.server_rtcp_port = rtcp_port;
    track.client_rtp_port = transport.client_rtp_port;
    track.client_rtcp_port = transport.client_rtcp_port;
    track.client_ip = peer_ip_;
    track.rtcp.Configure(track.sender.get(), false, 0, nullptr, rtcp_fd, cname);
    track.setup = true;
    return Result::Ok();
}

Result Connection::StartPlaying(StreamId stream, int track_index)
{
    TrackRuntime *track = session_.Find(stream, track_index);
    if (track == nullptr || !track->setup)
    {
        return Result::Fail(Status::NotFound);
    }
    FrameHub *hub = server_->hub(stream);
    if (hub == nullptr)
    {
        return Result::Fail(Status::NotFound);
    }

    /* 同码流音/视频 track 共享订阅者：避免每 track 各注册一份导致视频帧被
     * 投递两份（一份还会经音频 track 的发送器以音频 PT 发出，破坏对端解包）。 */
    Subscriber *shared = nullptr;
    for (TrackRuntime &other : session_.tracks())
    {
        if (&other != track && other.stream == stream && other.subscriber_ptr() != nullptr)
        {
            shared = other.subscriber_ptr();
            break;
        }
    }
    if (shared != nullptr)
    {
        track->subscriber_view = shared;
        track->playing = true;
        session_.last_activity_ms = NowMonotonicMs();
        /* 音频轨复用视频轨订阅者时，同样要 attach 共享快照对齐音频回放段，
         * 否则音频从"现在"开始而视频从 GOP 关键帧开始（起点错位）。 */
        if (track_index == kTrackAudio && has_shared_play_snapshot_ && shared_play_snapshot_.audio &&
            !shared_play_snapshot_.audio->frames.empty())
        {
            shared->DropAudioBefore(shared_play_snapshot_.storage->last_monotonic_us);
            track->reader.Attach(shared_play_snapshot_);
        }
        return Result::Ok();
    }

    if (!track->subscriber)
    {
        track->subscriber = std::unique_ptr<Subscriber>(new Subscriber(hub->backpressure().client_max_pending_frames,
                                                         hub->backpressure().client_max_pending_bytes));
    }
    if (!track->subscriber_registered)
    {
        hub->AddSubscriber(track->subscriber.get());
        track->subscriber_registered = true;
    }

    track->subscriber->Activate();
    track->playing = true;
    session_.last_activity_ms = NowMonotonicMs();

    /* GOP cache 命中：清掉 WaitStart 期间积压的旧帧（sequence ≤ 回放边界，
     * 回放会覆盖它们），attach 后从缓存关键帧立即起播，不再请求 IDR。
     * 音视频轨共用同一份快照（同一 epoch、同一起点）：只回放视频会让音频
     * 从"现在"开始，两轨起点错位一个 GOP 相位，播放端音画同步的时钟在
     * 错位的轨之间来回切换（VLC 进度时间反复回跳 1~2 秒）。ZLMediaKit 的
     * flushGop 同样把音视频历史一起推给新 reader。 */
    if (!has_shared_play_snapshot_)
    {
        has_shared_play_snapshot_ = hub->AttachReader(&shared_play_snapshot_);
    }
    if (has_shared_play_snapshot_ && shared_play_snapshot_.storage)
    {
        if (track_index == kTrackVideo)
        {
            track->subscriber->ClearVideo();
            track->reader.Attach(shared_play_snapshot_);
            /* live 衔接基准 = 回放边界：回放完成后的首个 live 帧必须严格
             * 是 boundary+1，否则按 sequence 空洞处理。 */
            track->last_live_sequence = shared_play_snapshot_.replay_end_sequence;
            IPC_RTSP_LOGI(kTag,
                          "GOP回放起播 客户端:%s 通道:%d 帧:%zu 音:%zu 字节:%zu 代次:%llu",
                          peer_ip_.c_str(),
                          static_cast<int>(stream),
                          shared_play_snapshot_.replay_end_index,
                          shared_play_snapshot_.audio ? shared_play_snapshot_.audio->frames.size() : 0,
                          shared_play_snapshot_.storage ? shared_play_snapshot_.storage->bytes : 0,
                          static_cast<unsigned long long>(shared_play_snapshot_.epoch));
            return Result::Ok();
        }
        if (shared_play_snapshot_.audio && !shared_play_snapshot_.audio->frames.empty())
        {
            /* 音频轨：丢弃回放区间内的 live 音频（pts ≤ GOP 末帧），回放段
             * 与 live 段无缝衔接，且与视频回放共享起点。 */
            track->subscriber->DropAudioBefore(shared_play_snapshot_.storage->last_monotonic_us);
            track->reader.Attach(shared_play_snapshot_);
            return Result::Ok();
        }
        /* 音频段缺失（音频刚启用/无音频流）：无历史可回放，直接 live。 */
        return Result::Ok();
    }

    /* 未命中是常见状态（cache 关闭、首帧 key 未到、刚失效待重建）：
     * 回退 WaitStart + 限频 IDR，命中与否每次 PLAY 各打一条便于观测。 */
    if (track_index == kTrackVideo)
    {
        IPC_RTSP_LOGI(kTag,
                      "GOP缓存未命中 客户端:%s 通道:%d 开启:%d 代次:%llu 回退等待IDR",
                      peer_ip_.c_str(),
                      static_cast<int>(stream),
                      hub->gop_cache().enabled() ? 1 : 0,
                      static_cast<unsigned long long>(hub->gop_epoch()));
    }

    if (hub->HasWaitingSubscriber())
    {
        server_->RequestIdr(stream);
    }
    return Result::Ok();
}

Result Connection::PauseTrack(StreamId stream, int track_index)
{
    TrackRuntime *track = session_.Find(stream, track_index);
    if (track == nullptr)
    {
        return Result::Fail(Status::NotFound);
    }

    /* 共享者只解除自身引用；注销与去激活由属主 track 负责。 */
    if (track->subscriber == nullptr && track->subscriber_view != nullptr)
    {
        track->subscriber_view = nullptr;
        track->playing = false;
        return Result::Ok();
    }

    if (track->subscriber && track->subscriber_registered)
    {
        FrameHub *hub = server_->hub(stream);
        if (hub != nullptr)
        {
            hub->RemoveSubscriber(track->subscriber.get());
        }
        track->subscriber_registered = false;
    }
    if (track->subscriber)
    {
        track->subscriber->Deactivate();
    }
    track->playing = false;
    return Result::Ok();
}

void Connection::TeardownTracks()
{
    /* 先断开共享引用，再由属主 track 统一注销并释放订阅者。 */
    for (TrackRuntime &track : session_.tracks())
    {
        track.subscriber_view = nullptr;
    }
    for (TrackRuntime &track : session_.tracks())
    {
        if (track.subscriber && track.subscriber_registered)
        {
            FrameHub *hub = server_->hub(track.stream);
            if (hub != nullptr)
            {
                hub->RemoveSubscriber(track.subscriber.get());
            }
            track.subscriber_registered = false;
        }
        if (track.setup && track.kind == TransportKind::Udp)
        {
            server_->ReleaseUdpTrack();
        }
        track.Release();
    }
    session_.tracks().clear();
}

void Connection::DeferDescribe(const std::string &uri, std::int64_t deadline_ms)
{
    pending_describe_ = true;
    pending_describe_uri_ = uri;
    pending_describe_deadline_ms_ = deadline_ms;
    pending_describe_cseq_ = current_request_ == nullptr ? 0u : ipc_rtsp_shim_request_cseq(current_request_);

    if (loop_ == nullptr)
    {
        return;
    }
    if (describe_timer_ != 0)
    {
        loop_->CancelTimer(describe_timer_);
    }
    const std::int64_t now = NowMonotonicMs();
    const std::int64_t delay = deadline_ms > now ? (deadline_ms - now) : 1;
    describe_timer_ = loop_->AddTimer(now + delay,
                                      [this]()
                                      {
                                          describe_timer_ = 0;
                                          (void) TryServePendingDescribe();
                                      });
}

bool Connection::TryServePendingDescribe()
{
    if (!pending_describe_ || closing_)
    {
        return false;
    }

    now_ms_ = NowMonotonicMs();

    StreamId stream = StreamId::Main;
    if (!server_->router().ResolveStream(pending_describe_uri_, &stream))
    {
        pending_describe_ = false;
        return true;
    }

    FrameHub *hub = server_->hub(stream);
    if (hub == nullptr)
    {
        pending_describe_ = false;
        return true;
    }

    if (!hub->codec_config().Ready())
    {
        if (now_ms_ < pending_describe_deadline_ms_)
        {
            return false; /* 继续等待参数集 */
        }

        pending_describe_ = false;
        response_ = ipc_rtsp_shim_response_new_with_cseq(pending_describe_cseq_, control_writer_);
        AddHeader("Retry-After", "1");
        Respond(kStatusServiceUnavailable, "Service Unavailable");
        if (response_ != nullptr)
        {
            ipc_rtsp_shim_response_free(response_);
            response_ = nullptr;
        }
        return true;
    }

    pending_describe_ = false;
    response_ = ipc_rtsp_shim_response_new_with_cseq(pending_describe_cseq_, control_writer_);
    (void) controller_->SendDescribe(stream);
    if (response_ != nullptr)
    {
        ipc_rtsp_shim_response_free(response_);
        response_ = nullptr;
    }
    return true;
}

void Connection::SendParameterSets(StreamId stream, TrackRuntime &track, std::uint32_t raw_ts, const CodecConfigSnapshot *snapshot)
{
    FrameHub *hub = server_->hub(stream);
    if (hub == nullptr || !track.sender)
    {
        return;
    }

    /* 缺省从 hub 当前参数集发送；GOP 回放路径传入与快照绑定的参数集，
     * 防止发送期间参数集更新导致参数集与关键 AU 混代。 */
    const Codec codec = snapshot != nullptr ? snapshot->codec : hub->codec_config().codec();
    const ParameterSet &vps = snapshot != nullptr ? snapshot->vps : hub->codec_config().vps();
    const ParameterSet &sps = snapshot != nullptr ? snapshot->sps : hub->codec_config().sps();
    const ParameterSet &pps = snapshot != nullptr ? snapshot->pps : hub->codec_config().pps();

    std::uint8_t buffer[kParameterSetBufferBytes];
    std::size_t length = 0;
    auto append = [&](const ParameterSet &set)
    {
        if (!set.valid() || (length + 4u + set.size) > sizeof buffer)
        {
            return;
        }
        buffer[length] = 0;
        buffer[length + 1u] = 0;
        buffer[length + 2u] = 0;
        buffer[length + 3u] = 1;
        length += 4u;
        std::memcpy(buffer + length, set.data.data(), set.size);
        length += set.size;
    };

    append(vps);
    append(sps);
    append(pps);

    if (length == 0)
    {
        return;
    }

    /* 按发送器实际打包数计数：H264 只有 SPS/PPS 两个参数集（VPS 槽为空被
     * append 跳过），SendNal 也可能因背压丢包，固定 +3 会造成收包指标虚高。 */
    const std::uint32_t packets_before = track.sender->PacketCount();
    ForEachNal(codec,
               buffer,
               length,
               [&](const NalRange &nal)
               {
                   (void) track.sender->SendNal(nal.data, nal.size, raw_ts, false);
                   return true;
               });
    const std::uint32_t packets_sent = track.sender->PacketCount() - packets_before;
    stats_.rtp_packets += packets_sent;
    server_->counters().rtp_packets.fetch_add(packets_sent, std::memory_order_relaxed);
}

bool Connection::SendOneFrame(TrackRuntime &track, const PendingFrame &pending)
{
    if (!track.sender || pending.frame.data == nullptr || pending.frame.size == 0)
    {
        return false;
    }

    FrameHub *hub = server_->hub(track.stream);
    if (hub == nullptr)
    {
        return false;
    }

    /* 视频帧时间戳统一用 hub 规范化后的时间轴（无真实 PTS 时已是入队单调钟），
     * 保证 live 与 GOP 回放帧共享同一时间基准。 */
    const std::uint32_t raw_ts = track.sender->MapTimestamp(pending.meta.pts_us);

    if (hub->config().codec == Codec::MJPEG)
    {
        /* RFC 2435 规范打包：先定位主头字段/量化表/熵数据，再由发送器分片。
         * 解析失败（结构损坏/16bit 量化表等）只丢本帧并计数，不影响连接；
         * 拒帧日志附失败阶段与帧头 hex，供板端一次定位。 */
        JpegFrameInfo jpeg;
        JpegParseDiag diag;
        const Result parsed = ParseJpegFrame(pending.frame.data.get(), pending.frame.size, &jpeg, &diag);
        if (!parsed.ok())
        {
            server_->counters().media_parse_rejects.fetch_add(1, std::memory_order_relaxed);
            ++stats_.dropped_frames;
            /* 进程级静态（跨连接共享）：JPEG 拒帧日志限频 5s 一条，防异常码流刷屏。 */
            static std::atomic<std::int64_t> last_jpeg_warn_ms{ 0 };
            const std::int64_t now_ms = NowMonotonicMs();
            std::int64_t last = last_jpeg_warn_ms.load(std::memory_order_relaxed);
            if (now_ms - last > 5000 && last_jpeg_warn_ms.compare_exchange_strong(last, now_ms))
            {
                /* 帧头 hex 截前 48 字节：覆盖 SOI/APPn/DQT/SOF0 各段头。 */
                constexpr std::size_t kHeadDumpBytes = 48;
                const std::size_t head_bytes = std::min(pending.frame.size, kHeadDumpBytes);
                char head_hex[kHeadDumpBytes * 3 + 1] = { 0 };
                std::size_t off = 0;
                for (std::size_t k = 0; k < head_bytes; ++k)
                {
                    off += static_cast<std::size_t>(std::snprintf(head_hex + off, sizeof(head_hex) - off, "%02x ", pending.frame.data.get()[k]));
                }
                IPC_RTSP_LOGW(kTag,
                              "JPEG帧被拒绝 状态:%s 阶段:%s a:%u b:%u 字节:%zu 头:%s",
                              StatusToString(parsed.code),
                              JpegRejectStageToString(diag.stage),
                              diag.value_a,
                              diag.value_b,
                              pending.frame.size,
                              head_hex);
            }
            return true;
        }

        const std::uint32_t packets_before = track.sender->PacketCount();
        const Result sent = track.sender->SendJpegFrame(jpeg, raw_ts);
        if (!sent.ok())
        {
            return false;
        }
        const std::uint32_t frame_packets = track.sender->PacketCount() - packets_before;
        stats_.rtp_packets += frame_packets;
        server_->counters().rtp_packets.fetch_add(frame_packets, std::memory_order_relaxed);
        stats_.rtp_bytes += pending.frame.size;
        return true;
    }

    std::size_t nal_count = 0;
    ForEachNal(hub->config().codec,
               pending.frame.data.get(),
               pending.frame.size,
               [&](const NalRange &nal)
               {
                   ++nal_count;
                   return true;
               });

    std::size_t index = 0;
    bool ok = true;
    ForEachNal(hub->config().codec,
               pending.frame.data.get(),
               pending.frame.size,
               [&](const NalRange &nal)
               {
                   const bool au_end = (++index == nal_count);
                   const Result sent = track.sender->SendNal(nal.data, nal.size, raw_ts, au_end);
                   if (!sent.ok())
                   {
                       ok = false;
                       return false;
                   }
                   ++stats_.rtp_packets;
                   return true;
               });

    if (ok)
    {
        server_->counters().rtp_packets.fetch_add(nal_count, std::memory_order_relaxed);
        stats_.rtp_bytes += pending.frame.size;
        if (tx_log_budget_ > 0)
        {
            --tx_log_budget_;
            IPC_RTSP_LOGD(kTag,
                          "发送AU 客户端:%s 关键:%d pts_us:%lld ts:%u 字节:%zu nal数:%zu",
                          peer_ip_.c_str(),
                          pending.info.has_key_nal ? 1 : 0,
                          static_cast<long long>(pending.meta.pts_us),
                          raw_ts,
                          pending.frame.size,
                          nal_count);
        }
    }
    return ok;
}

void Connection::FlushMedia()
{
    if (closing_ || fd_ < 0)
    {
        return;
    }

    /* 先排空已有输出，让超时判定看到本轮真实 send 进展。 */
    if (!output_.empty())
    {
        FlushOutput();
        if (closing_)
        {
            return;
        }
    }

    now_ms_ = NowMonotonicMs();
    const BackpressureConfig &bp = server_->config().backpressure;
    bool wrote = false;

    for (TrackRuntime &track : session_.tracks())
    {
        Subscriber *subscriber = track.subscriber_ptr();
        if (!track.playing || subscriber == nullptr || !track.sender)
        {
            continue;
        }
        FrameHub *hub = server_->hub(track.stream);
        if (hub == nullptr)
        {
            continue;
        }

        /* 拥塞状态机：NORMAL/CONGESTED/RECOVERING。按积压时长判定（与码率/单帧
         * 大小无关）；字节护栏命中的场景由追加失败分支兜底进入拥塞态。 */
        const std::uint64_t backlog_ms = output_.BacklogMs(now_ms_);
        const bool congested = backlog_ms > bp.client_soft_backlog_ms;
        if (congested)
        {
            if (subscriber->state() != SubscriberState::Congested)
            {
                subscriber->SetState(SubscriberState::Congested);
                congestion_.Enter(now_ms_);
                IPC_RTSP_LOGW(kTag,
                              "客户端进入拥塞态 客户端:%s 积压:%llums 软阈值:%ums 输出:%zu",
                              peer_ip_.c_str(),
                              static_cast<unsigned long long>(backlog_ms),
                              static_cast<unsigned>(bp.client_soft_backlog_ms),
                              output_.size());
            }
        }
        else if (subscriber->state() == SubscriberState::Congested)
        {
            if (backlog_ms < bp.client_soft_backlog_ms / 2u)
            {
                subscriber->SetState(SubscriberState::Recovering);
            }
        }

        if (subscriber->state() == SubscriberState::Congested)
        {
            congestion_.Enter(now_ms_);
            if (congestion_.TimedOut(now_ms_, bp.congested_timeout_ms))
            {
                IPC_RTSP_LOGW(kTag,
                              "客户端拥塞无进展超时 客户端:%s 拥塞:%llums 无进展:%llums 输出:%zu",
                              peer_ip_.c_str(),
                              static_cast<unsigned long long>(congestion_.CongestedMs(now_ms_)),
                              static_cast<unsigned long long>(congestion_.NoProgressMs(now_ms_)),
                              output_.size());
                stats_.congested_ms += congestion_.Leave(now_ms_);
                server_->counters().congestion_disconnects.fetch_add(1, std::memory_order_relaxed);
                Close(DisconnectReason::OutputCongested);
                return;
            }
            if (backlog_ms > bp.client_hard_backlog_ms)
            {
                IPC_RTSP_LOGW(kTag,
                              "客户端积压超时断开 客户端:%s 积压:%llums 硬阈值:%ums 输出:%zu",
                              peer_ip_.c_str(),
                              static_cast<unsigned long long>(backlog_ms),
                              static_cast<unsigned>(bp.client_hard_backlog_ms),
                              output_.size());
                stats_.congested_ms += congestion_.Leave(now_ms_);
                server_->counters().congestion_disconnects.fetch_add(1, std::memory_order_relaxed);
                Close(DisconnectReason::OutputCongested);
                return;
            }
        }

        /* 音频优先：语音对延迟更敏感，且字节量小。仅音频 track 发送（订阅者
         * 与视频 track 共享，避免同帧被重复投递）。负载封装见 rtp/audio_payload。 */
        if (track.index == kTrackAudio)
        {
            std::array<std::uint8_t, kAacAuPrefixBytes + kAudioFrameSlotBytes> framed{};
            /* 音频 GOP 回放：与视频轨共用同一快照（同 epoch、同回放起点），
             * 先补历史再衔接 live 队列。快照音频帧的 pts 已在入 cache 时
             * 规范化为单调钟，与 live 发送同轴。 */
            while (!track.sender->IsFull() && track.reader.attached())
            {
                const AudioPendingFrame *replay = nullptr;
                const ReaderNext next = track.reader.PeekAudio(hub->gop_epoch(), &replay);
                if (next == ReaderNext::Stale || subscriber->state() == SubscriberState::Congested)
                {
                    track.reader.Detach();
                    hub->NoteReplayAbort();
                    break;
                }
                if (next == ReaderNext::ReplayFinished)
                {
                    track.reader.Detach();
                    break;
                }
                const std::uint32_t audio_ts = track.sender->MapTimestamp(replay->pts_us);
                const std::size_t payload_size = BuildAudioPayload(replay->codec,
                                                                   replay->data.data(),
                                                                   replay->size,
                                                                   framed.data(),
                                                                   framed.size());
                if (payload_size == 0)
                {
                    ++stats_.dropped_frames;
                    track.reader.ConsumeAudio();
                    continue;
                }
                if (!track.sender->SendPayload(framed.data(), payload_size, audio_ts, true).ok())
                {
                    break;
                }
                ++stats_.rtp_packets;
                server_->counters().rtp_packets.fetch_add(1, std::memory_order_relaxed);
                track.reader.ConsumeAudio();
                wrote = true;
            }
            AudioPendingFrame audio;
            while (!track.sender->IsFull() && subscriber->PopAudio(&audio))
            {
                const std::uint32_t audio_ts = track.sender->MapTimestamp(audio.has_pts ? audio.pts_us : NowMonotonicUs());
                const std::size_t payload_size = BuildAudioPayload(audio.codec,
                                                                   audio.data.data(),
                                                                   audio.size,
                                                                   framed.data(),
                                                                   framed.size());
                if (payload_size == 0)
                {
                    ++stats_.dropped_frames;
                    break;
                }
                /* marker=1：每个音频 RTP 包恰含一个完整 AU。live555/VLC 的
                 * MPEG4Generic 解包器以 marker 位判定帧完成，恒 0 会导致一个
                 * 音频帧都不上交（ffmpeg 不看 marker 所以无恙）。 */
                if (!track.sender->SendPayload(framed.data(), payload_size, audio_ts, true).ok())
                {
                    break;
                }
                ++stats_.rtp_packets;
                server_->counters().rtp_packets.fetch_add(1, std::memory_order_relaxed);
                wrote = true;
            }
        }

        /* RTCP SR 周期发送：live555/VLC 依赖 SR 的 NTP↔RTP 映射建立播放时钟与
         * A/V 同步；ffmpeg 不依赖但同样受益。I/O 线程内按 track 各自的 SSRC 发送。 */
        (void) track.rtcp.MaybeSendSr(now_ms_, kRtcpSrIntervalMs);

        /* 视频：先回放 GOP cache 历史（可解码起点），再衔接 live 队列。
         * 仅视频 track 发送（订阅者与音频 track 共享）。 */
        if (track.index != kTrackVideo)
        {
            continue;
        }

        /* cache 未命中不补挂：PLAY 响应的 RTP-Info 锚点按"下一个包 = 现在"
         * 语义给出（等 IDR 起播与此一致）；若此后再回放历史帧，时间戳会早于
         * 锚点，VLC 的时钟模型将在相对时间与墙钟之间反复跳变。ZLMediaKit 的
         * seek/恢复路径同样只等关键帧、不回放历史（RtspSession use_gop 注释）。 */

        /* ---- 阶段 A：GOP cache 回放，从关键 AU 逐帧推进 ---- */
        while (!track.sender->IsFull() && track.reader.attached())
        {
            const PendingFrame *replay = nullptr;
            const ReaderNext next = track.reader.Peek(hub->gop_epoch(), &replay);
            if (next == ReaderNext::Stale)
            {
                /* 快照已过期（新 GOP 滚动/失效）：中止旧回放，下一轮发送
                 * 事件会 attach 最新 GOP。 */
                track.reader.Detach();
                hub->NoteReplayAbort();
                break;
            }
            if (next == ReaderNext::ReplayFinished)
            {
                track.reader.Detach();
                break;
            }

            /* 拥塞时中止陈旧回放：历史数据让位于实时性，恢复时重新 attach。 */
            if (subscriber->state() == SubscriberState::Congested)
            {
                track.reader.Detach();
                hub->NoteReplayAbort();
                break;
            }

            if (subscriber->state() == SubscriberState::WaitStart || subscriber->state() == SubscriberState::Recovering)
            {
                const std::uint32_t raw_ts = track.sender->MapTimestamp(replay->meta.pts_us);
                IPC_RTSP_LOGD(kTag,
                              "回放起播补发参数集 客户端:%s 通道:%d ts:%u 代次:%llu",
                              peer_ip_.c_str(),
                              static_cast<int>(track.stream),
                              raw_ts,
                              static_cast<unsigned long long>(track.reader.snapshot().epoch));
                /* 用与 GOP 绑定的参数集快照发送，防止 hub 参数集在此期间更新导致混代。 */
                SendParameterSets(track.stream, track, raw_ts, &track.reader.snapshot().storage->codec_config);
                subscriber->SetState(SubscriberState::Streaming);
            }

            if (!SendOneFrame(track, *replay))
            {
                /* TCP 媒体包因输出护栏不足中断时与 live 同口径：立即切拥塞态，
                 * 保留游标（不 Consume），缓解后继续回放。 */
                if (track.kind == TransportKind::TcpInterleaved && !output_.has_media_room(1))
                {
                    subscriber->SetState(SubscriberState::Congested);
                    congestion_.Enter(now_ms_);
                }
                break;
            }
            track.reader.Consume();
            wrote = true;
        }

        /* ---- 阶段 B：live 队列（拥塞时只发关键帧；等待起点时先补参数集） ---- */
        PendingFrame pending;
        while (!track.sender->IsFull() && subscriber->Pop(&pending))
        {
            /* sequence 空洞：队列曾丢弃中间帧（P/B 依赖的参考帧缺失），
             * 继续发送会产生花屏——放弃本批 live 帧回到等待起点，
             * 由 cache attach 或限频 IDR 重建解码链。 */
            if (track.last_live_sequence != 0 && pending.sequence != track.last_live_sequence + 1)
            {
                IPC_RTSP_LOGW(kTag,
                              "实时流序列空洞 客户端:%s 通道:%d 期望:%llu 实际:%llu",
                              peer_ip_.c_str(),
                              static_cast<int>(track.stream),
                              static_cast<unsigned long long>(track.last_live_sequence + 1),
                              static_cast<unsigned long long>(pending.sequence));
                subscriber->ClearVideo();
                subscriber->SetState(SubscriberState::WaitStart);
                track.reader.Detach();
                track.last_live_sequence = 0;
                server_->RequestIdr(track.stream);
                break;
            }

            const bool frame_waiting_start = (subscriber->state() == SubscriberState::WaitStart) ||
                                             (subscriber->state() == SubscriberState::Recovering);
            if (frame_waiting_start && !pending.info.has_key_nal)
            {
                ++stats_.dropped_frames;
                if (stats_.dropped_frames <= 3)
                {
                    IPC_RTSP_LOGD(kTag,
                                  "等待关键帧丢帧 客户端:%s 关键:%d 含关键NAL:%d 参数集:%d vcl:%zu 字节:%zu 状态:%d",
                                  peer_ip_.c_str(),
                                  pending.meta.key ? 1 : 0,
                                  pending.info.has_key_nal ? 1 : 0,
                                  pending.info.has_parameter_set ? 1 : 0,
                                  pending.info.vcl_count,
                                  pending.bytes,
                                  static_cast<int>(subscriber->state()));
                }
                continue;
            }
            if (subscriber->state() == SubscriberState::Congested && !pending.info.has_key_nal)
            {
                ++stats_.dropped_frames;
                continue;
            }

            if (frame_waiting_start)
            {
                const std::uint32_t raw_ts = track.sender->MapTimestamp(pending.meta.pts_us);
                IPC_RTSP_LOGD(kTag,
                              "订阅者起播补发参数集 客户端:%s 通道:%d ts:%u 原状态:%d",
                              peer_ip_.c_str(),
                              static_cast<int>(track.stream),
                              raw_ts,
                              static_cast<int>(subscriber->state()));
                SendParameterSets(track.stream, track, raw_ts);
                subscriber->SetState(SubscriberState::Streaming);
            }

            if (!SendOneFrame(track, pending))
            {
                ++stats_.dropped_frames;
                hub->NoteDropped(pending.info.has_key_nal);
                /* TCP 媒体包因输出护栏不足而中断时，立即切到拥塞态；下一次只保留
                 * 关键帧，避免把同一 AU 的尾部当作可解码起点继续发送。 */
                if (track.kind == TransportKind::TcpInterleaved && !output_.has_media_room(1))
                {
                    subscriber->SetState(SubscriberState::Congested);
                    congestion_.Enter(now_ms_);
                }
                break;
            }
            track.last_live_sequence = pending.sequence;
            wrote = true;
        }
    }

    bool still_congested = false;
    for (const TrackRuntime &track : session_.tracks())
    {
        const Subscriber *subscriber = track.subscriber_ptr();
        if (subscriber == nullptr)
        {
            continue;
        }
        const SubscriberState state = subscriber->state();
        if (state == SubscriberState::Congested)
        {
            still_congested = true;
            break;
        }
    }
    if (!still_congested && congestion_.active())
    {
        stats_.congested_ms += congestion_.Leave(now_ms_);
    }

    if (wrote || !output_.empty())
    {
        FlushOutput();
    }
}

} // namespace detail
} // namespace ipc_rtsp
