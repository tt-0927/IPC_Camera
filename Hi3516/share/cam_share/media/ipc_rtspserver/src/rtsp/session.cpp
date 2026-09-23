/**
 * @FilePath     : session.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 会话与 track 状态实现
 */

#include "rtsp/session.h"

#include "support/time.h"

#include <unistd.h>

namespace ipc_rtsp
{
namespace detail
{

void TrackRuntime::Release()
{
    /* 先摘订阅关系，避免 FrameHub 继续往已释放的订阅者投递。 */
    subscriber_view = nullptr;
    subscriber.reset();
    /* 释放 GOP 快照引用，让 cache 可以及时回收旧代次存储。 */
    reader.Detach();
    last_live_sequence = 0;

    rtcp.SendBye();
    rtcp.Configure(nullptr, true, 1, nullptr, -1, std::string());

    sender.reset();

    if (rtcp_fd >= 0)
    {
        ::close(rtcp_fd);
        rtcp_fd = -1;
    }

    setup = false;
    playing = false;
    kind = TransportKind::Unknown;
    rtp_channel = 0;
    rtcp_channel = 1;
    server_rtp_port = 0;
    server_rtcp_port = 0;
    client_rtp_port = 0;
    client_rtcp_port = 0;
}

void Session::GenerateId()
{
    /* 只生成会话 id，**不得**清空 tracks_：SETUP 会先建立 track，清空会让
     * 刚建立的 track 立刻失效，并让调用方持有的 TrackRuntime 指针悬空。
     * 清空语义由 Reset() 负责。 */
    const std::uint64_t random = RandomU64();
    char buffer[17];
    snprintf(buffer, sizeof buffer, "%016llx", static_cast<unsigned long long>(random));
    id_.assign(buffer, 16);
    played = false;
}

void Session::Reset()
{
    ReleaseAll();
    id_.clear();
    played = false;
}

TrackRuntime *Session::Track(StreamId stream, int index)
{
    for (TrackRuntime &track : tracks_)
    {
        if (track.stream == stream && track.index == index)
        {
            return &track;
        }
    }

    TrackRuntime track;
    track.stream = stream;
    track.index = index;
    tracks_.push_back(std::move(track));
    return &tracks_.back();
}

TrackRuntime *Session::Find(StreamId stream, int index)
{
    for (TrackRuntime &track : tracks_)
    {
        if (track.stream == stream && track.index == index)
        {
            return &track;
        }
    }
    return nullptr;
}

TrackRuntime *Session::FindByRtcpChannel(std::uint8_t channel)
{
    for (TrackRuntime &track : tracks_)
    {
        if (track.setup && track.kind == TransportKind::TcpInterleaved && track.rtcp_channel == channel)
        {
            return &track;
        }
    }
    return nullptr;
}

int Session::playing_count() const
{
    int count = 0;
    for (const TrackRuntime &track : tracks_)
    {
        if (track.playing)
        {
            ++count;
        }
    }
    return count;
}

std::size_t Session::udp_track_count() const
{
    std::size_t count = 0;
    for (const TrackRuntime &track : tracks_)
    {
        if (track.setup && track.kind == TransportKind::Udp)
        {
            ++count;
        }
    }
    return count;
}

void Session::ReleaseAll()
{
    for (TrackRuntime &track : tracks_)
    {
        track.Release();
    }
    tracks_.clear();
}

} // namespace detail
} // namespace ipc_rtsp
