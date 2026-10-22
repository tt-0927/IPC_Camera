/**
 * @FilePath     : session.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTSP 会话与 track 运行时状态
 */

/*
 * 会话模型（对应 ADR D5 与 05 文档 5.10）：一条 TCP 连接一个 aggregate Session，
 * 一个 Session 最多两个 track（video/audio）。
 */
#pragma once

#include "ipc_rtsp/config.h"
#include "ipc_rtsp/types.h"
#include "media/frame_hub.h"
#include "media/frame_reader.h"
#include "rtp/rtcp_sender.h"
#include "rtp/rtp_sender.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

/** 一个 track 的运行时状态。 */
struct TrackRuntime
{
    /* 所属码流（主/子）。 */
    StreamId stream = StreamId::Main;
    int index = 0; /**< kTrackVideo / kTrackAudio */
    /* 是否已完成 SETUP（传输已建立）。 */
    bool setup = false;
    /* 本 track 实际使用的下层传输。 */
    TransportKind kind = TransportKind::Unknown;

    /* interleaved RTP 通道号（TCP 模式）。 */
    std::uint8_t rtp_channel = 0;
    /* interleaved RTCP 通道号（TCP 模式，通常为 RTP 通道 +1）。 */
    std::uint8_t rtcp_channel = 1;

    /* UDP 模式的 RTCP socket fd（拥有，Release 时 close）；RTP fd 归 sender 所有。 */
    int rtcp_fd = -1;
    /* 服务端 RTP/RTCP 端口（UDP 模式，本端已 bind 的端口）。 */
    std::uint16_t server_rtp_port = 0;
    std::uint16_t server_rtcp_port = 0;
    /* 客户端 IP（UDP 模式）：取自 SETUP 请求的源地址，与 client_port 组成回发地址。 */
    std::string client_ip;
    /* 客户端 RTP/RTCP 端口（UDP 模式，来自 Transport 头的 client_port）。 */
    std::uint16_t client_rtp_port = 0;
    std::uint16_t client_rtcp_port = 0;

    /* RTP 发送器（拥有）；析构级联释放 shim 侧 rtp/transport 资源。 */
    std::unique_ptr<RtpSender> sender;
    /* RTCP SR/BYE 发送器（值成员）；其内部 rtp_ 指向 sender 持有的实例。 */
    RtcpSender rtcp;
    /* 同码流的音/视频 track 共享同一订阅者（视频/音频帧进入同一待发队列，由各自
     * track 分工发送）：先建立的 track 拥有实例，后建立的持有非拥有引用。 */
    std::unique_ptr<Subscriber> subscriber;
    /* 共享订阅者视图（非拥有）：指向先建立的兄弟 track 的 subscriber 实例，
     * 与 subscriber 同生共死，只能通过 subscriber_ptr() 取用。 */
    Subscriber *subscriber_view = nullptr;
    /* 订阅者是否已注册到 FrameHub（Release 时负责摘除）。 */
    bool subscriber_registered = false;
    /* 本 track 是否处于 PLAY 中（占用播放配额）。 */
    bool playing = false;
    /** GOP 回放游标：音视频 track 各持一份，但 attach 同一快照（同 epoch、
     * 同起点），两轨回放才能对齐。 */
    FrameReader reader;
    /** live 队列最近发送的 AU 序列号；0 表示尚未发送（用于空洞检测）。 */
    std::uint64_t last_live_sequence = 0;

    /** 本 track 实际使用的订阅者（属主返回自有实例，共享者返回兄弟引用）。 */
    Subscriber *subscriber_ptr() const
    {
        return subscriber ? subscriber.get() : subscriber_view;
    }

    /** 释放 socket/发送器/订阅关系；可重复调用。 */
    void Release();
};

/** aggregate 会话。 */
class Session
{
public:
    /**
     * @brief 生成新的会话 id（随机 64 位十六进制）。
     *
     * 只生成 id、不清空 tracks_：SETUP 会先建立 track 再调用本函数，
     * 清空语义由 Reset() 负责。
     *
     * @return {void}
     */
    void GenerateId();

    /**
     * @brief 释放全部 track 并清空会话（id 与 tracks_）。
     *
     * 与 GenerateId() 的"只生成 id 不动 tracks_"互补；TEARDOWN 与超时回收时调用。
     *
     * @return {void}
     */
    void Reset();

    /** 会话是否已建立（id 非空）。 */
    bool valid() const
    {
        return !id_.empty();
    }

    /** 会话 id；未建立返回空串。 */
    const std::string &id() const
    {
        return id_;
    }

    /**
     * @brief 取得（必要时创建）指定 track。
     *
     * @param stream 所属码流
     * @param index track 序号（kTrackVideo/kTrackAudio）
     * @return track 运行时状态指针；指向 tracks_ 内部元素，再次 SETUP 追加
     *         新 track 会触发 vector 重分配使先前指针悬空，调用方不得跨
     *         SETUP 调用持有该指针
     */
    TrackRuntime *Track(StreamId stream, int index);

    /**
     * @brief 按 stream/track 查找已建立的 track。
     *
     * @param stream 所属码流
     * @param index track 序号
     * @return 已建立的 track 指针，未建立返回 nullptr
     */
    TrackRuntime *Find(StreamId stream, int index);

    /**
     * @brief 按 interleaved RTCP 通道号查找 track（用于接收客户端 RTCP）。
     *
     * @param channel interleaved RTCP 通道号
     * @return 匹配且已 SETUP 的 TCP track 指针，无匹配返回 nullptr
     */
    TrackRuntime *FindByRtcpChannel(std::uint8_t channel);

    /** 已建立的 track 列表（最多 video/audio 两条）。 */
    std::vector<TrackRuntime> &tracks()
    {
        return tracks_;
    }

    /** 已建立的 track 列表（只读）。 */
    const std::vector<TrackRuntime> &tracks() const
    {
        return tracks_;
    }

    /** 处于 PLAY 状态的 track 数（0~2）。 */
    int playing_count() const;

    /** 是否存在 PLAY 中的 track。 */
    bool any_playing() const
    {
        return playing_count() > 0;
    }

    /** 已 SETUP 且走 UDP 的 track 数（用于 UDP 配额统计与归还）。 */
    std::size_t udp_track_count() const;

    /** 释放全部 track 并清空会话 id。 */
    void ReleaseAll();

    /** 最近一次合法请求时间（毫秒，单调时钟）。 */
    std::int64_t last_activity_ms = 0;

    /** 会话是否已 PLAY 过（用于 TEARDOWN 语义与日志）。 */
    bool played = false;

private:
    /* 会话 id（16 字符十六进制）；空表示会话未建立或已被 Reset 清空。 */
    std::string id_;
    /* track 列表；Track() 返回其内部指针，push_back 扩容可能使既有指针失效。 */
    std::vector<TrackRuntime> tracks_;
};

} // namespace detail
} // namespace ipc_rtsp
