/**
 * @FilePath     : controller.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTSP 控制面实现
 */

#include "rtsp/controller.h"

#include "adapter/smolrtsp_shim.h"
#include "auth/digest.h"
#include "net/connection.h"
#include "rtsp/sdp.h"
#include "rtsp/url_router.h"
#include "server_impl.h"
#include "support/log.h"
#include "support/text.h"
#include "support/time.h"

#include <cstdio>
#include <cstring>

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr const char *kTag = "rtsp";

/* RTSP 状态码（避免在业务代码里引入 smolrtsp 头文件）。 */
constexpr std::uint16_t kStatusOk = 200;
constexpr std::uint16_t kStatusBadRequest = 400;
constexpr std::uint16_t kStatusUnauthorized = 401;
constexpr std::uint16_t kStatusNotFound = 404;
constexpr std::uint16_t kStatusMethodNotAllowed = 405;
constexpr std::uint16_t kStatusNotEnoughBandwidth = 453;
constexpr std::uint16_t kStatusSessionNotFound = 454;
constexpr std::uint16_t kStatusUnsupportedTransport = 461;
constexpr std::uint16_t kStatusInternalError = 500;
constexpr std::uint16_t kStatusServiceUnavailable = 503;

/** DESCRIBE 等待参数集的超时（毫秒）。 */
constexpr std::int64_t kDescribeWaitMs = 3000;
} // namespace

Controller::Controller(Connection *connection) : connection_(connection)
{
}

bool Controller::Header(const char *name, std::string *value) const
{
    const ipc_rtsp_shim_request *request = connection_->request();
    if (request == nullptr)
    {
        return false;
    }
    std::size_t len = 0;
    const char *data = ipc_rtsp_shim_request_header(request, name, &len);
    if (data == nullptr)
    {
        return false;
    }
    if (value != nullptr)
    {
        value->assign(data, len);
    }
    return true;
}

bool Controller::CheckAuth(bool anonymous_allowed)
{
    AuthConfig &auth = connection_->server().mutable_config().auth;
    if (auth.mode == AuthMode::None)
    {
        return true;
    }
    if (anonymous_allowed && auth.allow_anonymous_options)
    {
        return true;
    }

    std::string authorization;
    const bool has_header = Header("Authorization", &authorization);
    const std::string &nonce = connection_->auth_state().nonce.Current(connection_->now_ms(), auth.nonce_ttl_s);

    if (!has_header)
    {
        connection_->AddHeader("WWW-Authenticate", BuildDigestChallenge(auth, nonce));
        connection_->Respond(kStatusUnauthorized, "Unauthorized");
        return false;
    }

    const AuthResult result = VerifyDigest(auth, connection_->request_method(), connection_->request_uri(), authorization, nonce);
    if (result == AuthResult::Ok)
    {
        connection_->NoteAuthSuccess();
        return true;
    }

    if (result == AuthResult::BadNonce)
    {
        /* nonce 过期：轮换后重新挑战，客户端会带新 nonce 重试。 */
        connection_->auth_state().nonce.Rotate();
        const std::string &fresh = connection_->auth_state().nonce.Current(connection_->now_ms(), auth.nonce_ttl_s);
        connection_->AddHeader("WWW-Authenticate", BuildDigestChallenge(auth, fresh));
        connection_->Respond(kStatusUnauthorized, "Unauthorized");
        return false;
    }

    const std::uint32_t failures = connection_->NoteAuthFailure();
    IPC_RTSP_LOGW(kTag, "认证失败 客户端:%s 累计:%u", connection_->peer_ip().c_str(), failures);
    /* 普通认证失败（response 错）：nonce 未失效，附当前 challenge 让客户端直接重试。 */
    connection_->AddHeader("WWW-Authenticate", BuildDigestChallenge(auth, nonce));
    connection_->Respond(kStatusUnauthorized, "Unauthorized");

    if (failures >= auth.max_failures_per_connection)
    {
        connection_->Close(DisconnectReason::AuthFailures);
    }
    return false;
}

bool Controller::Handle(const ipc_rtsp_shim_request *request)
{
    if (request == nullptr || connection_ == nullptr)
    {
        return false;
    }

    std::size_t method_len = 0;
    const char *method_data = ipc_rtsp_shim_request_method(request, &method_len);
    const std::string method(method_data == nullptr ? "" : method_data, method_len);

    std::size_t uri_len = 0;
    const char *uri_data = ipc_rtsp_shim_request_uri(request, &uri_len);
    connection_->request_uri_.assign(uri_data == nullptr ? "" : uri_data, uri_len);
    connection_->request_method_ = method;
    connection_->current_request_ = request;

    connection_->MarkActivity();

    IPC_RTSP_LOGD(kTag,
                  "请求 客户端:%s 方法:%s 会话:%s 有效:%d 轨道:%zu",
                  connection_->peer_ip().c_str(),
                  method.c_str(),
                  connection_->session().id().c_str(),
                  connection_->session().valid() ? 1 : 0,
                  connection_->session().tracks().size());

    if (method == "OPTIONS")
    {
        if (!CheckAuth(true))
        {
            return true; /* 401 已回复，连接保持 */
        }
        HandleOptions();
    }
    else if (method == "DESCRIBE")
    {
        if (!CheckAuth(false))
        {
            return true;
        }
        HandleDescribe();
    }
    else if (method == "SETUP")
    {
        if (!CheckAuth(false))
        {
            return true;
        }
        HandleSetup();
    }
    else if (method == "PLAY")
    {
        if (!CheckAuth(false))
        {
            return true;
        }
        HandlePlay();
    }
    else if (method == "PAUSE")
    {
        if (!CheckAuth(false))
        {
            return true;
        }
        HandlePause();
    }
    else if (method == "TEARDOWN")
    {
        if (!CheckAuth(false))
        {
            return true;
        }
        HandleTeardown();
    }
    else if (method == "GET_PARAMETER")
    {
        if (!CheckAuth(false))
        {
            return true;
        }
        HandleGetParameter();
    }
    else if (method == "SET_PARAMETER")
    {
        if (!CheckAuth(false))
        {
            return true;
        }
        HandleSetParameter();
    }
    else
    {
        if (!CheckAuth(false))
        {
            return true;
        }
        HandleUnknown(method);
    }

    return true;
}

void Controller::HandleOptions()
{
    connection_->AddHeader("Public", "OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, TEARDOWN, GET_PARAMETER, SET_PARAMETER");
    connection_->Respond(kStatusOk, "OK");
}

bool Controller::SendDescribe(StreamId stream)
{
    ServerImpl &server = connection_->server();
    FrameHub *hub = server.hub(stream);
    if (hub == nullptr)
    {
        connection_->Respond(kStatusNotFound, "Not Found");
        return false;
    }

    if (!hub->codec_config().Ready())
    {
        /* 参数集还没到（例如编码器刚启动）：挂起请求，等参数集或超时。
         * 直接回 503 会让多数客户端放弃，因此这里与 live555 的行为对齐：
         * 先不回复，等首帧参数集到达后再补发。 */
        connection_->DeferDescribe(connection_->request_uri(), NowMonotonicMs() + kDescribeWaitMs);
        IPC_RTSP_LOGI(kTag, "DESCRIBE 等待参数集 客户端:%s 通道:%d", connection_->peer_ip().c_str(), static_cast<int>(stream));
        return false;
    }

    SdpInput input;
    input.stream = &hub->config();
    input.codec_config = &hub->codec_config();
    input.advertised_ip = server.AdvertisedIp();
    input.session_id = static_cast<std::uint32_t>(RandomU32());
    input.server_name = server.config().server_name;
    input.include_audio = hub->config().audio_enabled;

    std::string sdp;
    if (!BuildStreamSdp(input, &sdp))
    {
        connection_->Respond(kStatusInternalError, "Internal Server Error");
        return false;
    }

    std::string content_base = "rtsp://";
    content_base += server.AdvertisedIp();
    content_base += ":";
    content_base += std::to_string(server.config().port);
    content_base += "/";
    content_base += hub->config().path;

    connection_->AddHeader("Content-Type", "application/sdp");
    connection_->AddHeader("Content-Base", content_base);
    connection_->SetBody(sdp);
    connection_->Respond(kStatusOk, "OK");
    return true;
}

void Controller::HandleDescribe()
{
    ServerImpl &server = connection_->server();
    StreamId stream = StreamId::Main;
    if (!server.router().ResolveStream(connection_->request_uri(), &stream))
    {
        connection_->Respond(kStatusNotFound, "Not Found");
        return;
    }
    (void) SendDescribe(stream);
}

void Controller::HandleSetup()
{
    ServerImpl &server = connection_->server();

    StreamId stream = StreamId::Main;
    int track_index = kTrackVideo;
    if (!server.router().ResolveTrack(connection_->request_uri(), &stream, &track_index))
    {
        connection_->Respond(kStatusNotFound, "Not Found");
        return;
    }

    FrameHub *hub = server.hub(stream);
    if (hub == nullptr)
    {
        connection_->Respond(kStatusNotFound, "Not Found");
        return;
    }

    if (track_index == kTrackAudio && (!hub->config().audio_enabled || hub->config().audio_codec == AudioCodec::None))
    {
        connection_->Respond(kStatusNotFound, "Not Found");
        return;
    }

    std::string transport_header;
    if (!Header("Transport", &transport_header))
    {
        connection_->Respond(kStatusBadRequest, "Bad Request");
        return;
    }

    ipc_rtsp_shim_transport transport{};
    if (ipc_rtsp_shim_parse_transport(transport_header.data(), transport_header.size(), &transport) != 0)
    {
        connection_->Respond(kStatusBadRequest, "Bad Request");
        return;
    }
    /* TCP 下层必须给出 interleaved 通道号：缺 channel 无法在 RTSP 连接上复用 RTP/RTCP。 */
    if (transport.lower_transport == IPC_RTSP_SHIM_LOWER_TCP && !transport.has_interleaved)
    {
        connection_->Respond(kStatusUnsupportedTransport, "Unsupported Transport");
        return;
    }
    /* UDP 下层必须给出 client_port：缺端口无法向客户端回发 RTP/RTCP。 */
    if (transport.lower_transport == IPC_RTSP_SHIM_LOWER_UDP && !transport.has_client_port)
    {
        connection_->Respond(kStatusUnsupportedTransport, "Unsupported Transport");
        return;
    }

    /* 准入：服务级与码流级播放上限（新连接才校验，已有播放中的连接不受影响）。
     * 计数口径与业界一致（MediaMTX/ZLMediaKit 的 session 级读者数）：
     * 复合流客户端的音视频轨只计 1；同一连接拉主+子码流按 2 条流计入总额。 */
    const bool already_playing = connection_->session().any_playing();
    const int playing_total = server.PlayingStreamCount();
    const int playing_stream = server.PlayingClientCount(stream);
    const int stream_limit = hub->config().max_playing_clients;
    const bool global_limit_reached = !already_playing && playing_total >= server.config().max_playing_clients;
    const bool stream_limit_reached = stream_limit > 0 && playing_stream >= stream_limit;
    if (global_limit_reached || stream_limit_reached)
    {
        IPC_RTSP_LOGW(kTag,
                      "拒绝新客户端 client:%s stream:%d 总流数:%d/%d 流级客户端:%d/%d reason:%s",
                      connection_->peer_ip().c_str(),
                      static_cast<int>(stream),
                      playing_total,
                      server.config().max_playing_clients,
                      playing_stream,
                      stream_limit,
                      global_limit_reached ? "global" : "stream");
        connection_->Respond(kStatusNotEnoughBandwidth, "Not Enough Bandwidth");
        return;
    }

    TrackRuntime *track = connection_->session().Track(stream, track_index);
    if (track == nullptr)
    {
        connection_->Respond(kStatusInternalError, "Internal Server Error");
        return;
    }

    /* 重复 SETUP 同一 track：先释放旧传输，保持幂等。UDP 配额必须在此归还，
     * 否则客户端"UDP SETUP 成功后回退 TCP 再 SETUP"每次泄漏一个配额
     * （VLC 每次启动都这么干），8 次后所有 UDP SETUP 永久 453。 */
    if (track->setup && track->kind == TransportKind::Udp)
    {
        connection_->server().ReleaseUdpTrack();
    }
    track->Release();

    const Result established = connection_->EstablishTransport(*track, transport);
    if (!established.ok())
    {
        connection_->Respond(established.code == Status::NoCapacity ? kStatusNotEnoughBandwidth : kStatusInternalError,
                             established.code == Status::NoCapacity ? "Not Enough Bandwidth" : "Internal Server Error");
        return;
    }

    if (!connection_->session().valid())
    {
        connection_->session().GenerateId();
    }

    char session_value[64];
    /* Session 头语义 "id;timeout=<秒>"：配置以毫秒计，协议头以秒计，除以 1000 换算。 */
    snprintf(session_value,
             sizeof session_value,
             "%s;timeout=%u",
             connection_->session().id().c_str(),
             static_cast<unsigned>(server.config().session_timeout_ms / 1000));
    connection_->AddHeader("Session", session_value);

    char transport_value[128];
    if (track->kind == TransportKind::TcpInterleaved)
    {
        snprintf(transport_value,
                 sizeof transport_value,
                 "RTP/AVP/TCP;unicast;interleaved=%u-%u",
                 static_cast<unsigned>(track->rtp_channel),
                 static_cast<unsigned>(track->rtcp_channel));
    }
    else
    {
        snprintf(transport_value,
                 sizeof transport_value,
                 "RTP/AVP;unicast;client_port=%u-%u;server_port=%u-%u",
                 static_cast<unsigned>(track->client_rtp_port),
                 static_cast<unsigned>(track->client_rtcp_port),
                 static_cast<unsigned>(track->server_rtp_port),
                 static_cast<unsigned>(track->server_rtcp_port));
    }
    connection_->AddHeader("Transport", transport_value);
    connection_->Respond(kStatusOk, "OK");
}

void Controller::HandlePlay()
{
    if (!connection_->session().valid())
    {
        connection_->Respond(kStatusSessionNotFound, "Session Not Found");
        return;
    }

    /* 每个 PLAY 周期重新 attach：音视频轨在下面的循环里共用同一份快照。 */
    connection_->ResetSharedPlaySnapshot();

    int activated = 0;
    for (TrackRuntime &track : connection_->session().tracks())
    {
        if (!track.setup)
        {
            continue;
        }
        const Result started = connection_->StartPlaying(track.stream, track.index);
        if (!started.ok())
        {
            connection_->Respond(kStatusInternalError, "Internal Server Error");
            return;
        }
        ++activated;
    }

    if (activated == 0)
    {
        connection_->Respond(kStatusSessionNotFound, "Session Not Found");
        return;
    }

    connection_->session().played = true;
    connection_->set_ever_played(true);
    /* 直播语义：npt=now- 表示从当前时刻播放、无确定终点，不支持随机 seek。 */
    connection_->AddHeader("Range", "npt=now-");

    /* RTP-Info：给 live555/VLC 提供 NPT 基线。live555 要求 seq 与 rtptime 必须
     * 同时存在（RTSPClient::parseRTPInfoParams 源码），缺 seq 时其 NPT 计算
     * 恒为 0，VLC 播放时间会一直停在 00:00；ZLMediaKit 同样两者齐发。
     * rtptime 必须是**实际将发送的首包**时间戳（ZLMediaKit 同款：取 GOP 回放
     * 首帧）：若按 PLAY 时刻取值，命中回放时首包时间戳早于锚点几百毫秒到
     * 一个 GOP，VLC 的时钟模型在相对时间与 RTCP SR 墙钟之间反复跳变。 */
    {
        ServerImpl &server = connection_->server();
        std::string rtp_info;
        char item[256];
        for (const TrackRuntime &track : connection_->session().tracks())
        {
            if (!track.sender)
            {
                continue;
            }
            const FrameHub *stream_hub = server.hub(track.stream);
            if (stream_hub == nullptr)
            {
                continue;
            }
            /* 回放路径：锚点 = 实际将发送的首包时间戳（ZLMediaKit 同款）。
             * 视频取回放关键帧，音频取与之对齐的音频段首帧——两轨锚点必须
             * 指向同一播放起点，否则播放端音画时钟互斥切换。 */
            std::uint32_t rtptime = track.sender->MapTimestamp(NowMonotonicUs());
            const GopSnapshot &snapshot = track.reader.snapshot();
            if (track.index == kTrackVideo)
            {
                if (track.reader.attached() && snapshot.storage && !snapshot.storage->frames.empty())
                {
                    rtptime = track.sender->MapTimestamp(snapshot.storage->frames.front().meta.pts_us);
                }
            }
            else if (track.reader.attached() && snapshot.audio && !snapshot.audio->frames.empty())
            {
                rtptime = track.sender->MapTimestamp(snapshot.audio->frames.front().pts_us);
            }
            std::snprintf(item,
                          sizeof item,
                          "%surl=rtsp://%s:%u/%s/trackID=%d;seq=%u;rtptime=%u",
                          rtp_info.empty() ? "" : ",",
                          server.AdvertisedIp().c_str(),
                          static_cast<unsigned>(server.config().port),
                          stream_hub->config().path.c_str(),
                          track.index,
                          static_cast<unsigned>(track.sender->NextSequence()),
                          rtptime);
            rtp_info += item;
        }
        if (!rtp_info.empty())
        {
            connection_->AddHeader("RTP-Info", rtp_info.c_str());
        }
    }
    connection_->Respond(kStatusOk, "OK");
    IPC_RTSP_LOGI(kTag,
                  "客户端开始播放 客户端:%s 会话:%s 轨道:%d",
                  connection_->peer_ip().c_str(),
                  connection_->session().id().c_str(),
                  activated);
}

void Controller::HandlePause()
{
    if (!connection_->session().valid())
    {
        connection_->Respond(kStatusSessionNotFound, "Session Not Found");
        return;
    }

    for (TrackRuntime &track : connection_->session().tracks())
    {
        if (track.setup)
        {
            (void) connection_->PauseTrack(track.stream, track.index);
        }
    }
    connection_->Respond(kStatusOk, "OK");
}

void Controller::HandleTeardown()
{
    connection_->TeardownTracks();
    connection_->session().Reset();
    connection_->Respond(kStatusOk, "OK");
    IPC_RTSP_LOGI(kTag, "客户端请求TEARDOWN 客户端:%s", connection_->peer_ip().c_str());
}

void Controller::HandleGetParameter()
{
    /* keepalive：刷新活动时间并回 200（不解析 body）。 */
    connection_->Respond(kStatusOk, "OK");
}

void Controller::HandleSetParameter()
{
    /* 兼容处理：接受但忽略参数，回 200（与现网 live555 行为一致）。 */
    connection_->Respond(kStatusOk, "OK");
}

void Controller::HandleUnknown(const std::string &method)
{
    IPC_RTSP_LOGD(kTag, "未支持的方法 客户端:%s 方法:%s", connection_->peer_ip().c_str(), method.c_str());
    connection_->AddHeader("Allow", "OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, TEARDOWN, GET_PARAMETER, SET_PARAMETER");
    connection_->Respond(kStatusMethodNotAllowed, "Method Not Allowed");
}

} // namespace detail
} // namespace ipc_rtsp
