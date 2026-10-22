/**
 * @FilePath     : connection.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTSP 连接：输入解析、输出队列、会话承载与媒体推送
 */

/* 线程约定：Connection 的全部状态只在 I/O 线程访问。 */
#pragma once

#include "adapter/smolrtsp_shim.h"
#include "auth/digest.h"
#include "ipc_rtsp/config.h"
#include "ipc_rtsp/status.h"
#include "ipc_rtsp/types.h"
#include "media/gop_cache.h"
#include "net/event_loop.h"
#include "net/congestion_tracker.h"
#include "net/output_buffer.h"
#include "rtsp/controller.h"
#include "rtsp/session.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

class ServerImpl;

/** 单条 RTSP 连接的鉴权状态。 */
struct ConnectionAuthState
{
    /** Digest 鉴权的 nonce 存储与失效时间表。 */
    NonceStore nonce;
    /** 连续认证失败累计次数；认证成功后清零。 */
    std::uint32_t failures = 0;
    /** 是否已成功认证过（Digest 模式）。 */
    bool authenticated = false;
};

/** RTSP 连接。 */
class Connection
{
public:
    /**
     * @brief 构造连接对象：初始化状态、按配置定容输入/输出缓冲（不注册事件）。
     *
     * @param server 所属服务实现（memory: 非拥有，生命周期须覆盖本连接）
     * @param loop 所属事件循环（memory: 非拥有，仅 I/O 线程使用）
     * @param fd 已 accept 的客户端 socket（memory: 所有权移交本对象，Close/析构时 close）
     * @param peer_ip 对端 IP 文本
     * @param peer_port 对端端口
     */
    Connection(ServerImpl *server, EventLoop *loop, int fd, std::string peer_ip, std::uint16_t peer_port);

    /** @brief 析构：释放 track/会话与 shim 对象并关闭 fd（未 Close 时兜底）。 @return {void} */
    ~Connection();

    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;

    /**
     * @brief 注册到事件循环并启动空闲定时器。
     *
     * @return 成功返回 Ok；shim 对象创建失败或事件注册失败返回对应错误
     * @note 仅 I/O 线程调用；构造后恰好调用一次。
     */
    Result Start();

    /**
     * @brief 关闭连接（幂等）；对象本体由事件循环本轮结束后的回收阶段删除。
     *
     * @param reason 断开原因，写入统计与断开日志
     * @return {void}
     * @note 仅 I/O 线程调用；Close 后不得再访问 track 与输出缓冲。
     */
    void Close(DisconnectReason reason);

    /** 是否正在关闭（Close 已请求）。 */
    bool closing() const
    {
        return closing_;
    }

    /** 客户端 socket fd；Close 后为 -1。 */
    int fd() const
    {
        return fd_;
    }

    /** 断开原因（Close 时写入）。 */
    DisconnectReason disconnect_reason() const
    {
        return disconnect_reason_;
    }

    /* ---- 供 Controller 使用的协议动作 ---- */

    /**
     * @brief 发送响应（序列化到输出缓冲并尝试立即写出）。
     *
     * @param code RTSP 状态码
     * @param reason 状态文本
     * @return {void}
     * @note 序列化失败时回滚输出缓冲（不留半包响应），并按写拥塞关闭连接。
     */
    void Respond(std::uint16_t code, const char *reason);

    /**
     * @brief 追加响应头。
     *
     * @param name 头字段名
     * @param value 头字段值
     * @return {void}
     * @note 须在 Respond 之前调用，作用于当前请求的响应上下文。
     */
    void AddHeader(const char *name, const std::string &value);

    /**
     * @brief 设置响应 body。
     *
     * @param body 响应体内容
     * @return {void}
     * @note 须在 Respond 之前调用。
     */
    void SetBody(const std::string &body);

    /**
     * @brief 刷新活动时间（收到合法请求/RTCP 时调用）。
     *
     * @return {void}
     * @note 仅 I/O 线程调用；空闲与会话超时判定均以该时间为基准。
     */
    void MarkActivity();

    /**
     * @brief 记录一次认证失败。
     *
     * @return 失败累计次数（供上层按次数决定断连）
     */
    std::uint32_t NoteAuthFailure();

    /** @brief 标记认证成功并清零失败计数。 @return {void} */
    void NoteAuthSuccess();

    /** 鉴权状态（供鉴权逻辑读写 nonce 与失败计数）。 */
    ConnectionAuthState &auth_state()
    {
        return auth_;
    }

    /** 请求的原始 URI（本次请求期间有效）。 */
    const std::string &request_uri() const
    {
        return request_uri_;
    }

    /** 请求方法（本次请求期间有效）。 */
    const std::string &request_method() const
    {
        return request_method_;
    }

    /**
     * @brief 参数集尚未就绪时挂起 DESCRIBE，等参数集到达后补发响应。
     *
     * @param uri 原始请求 URI（补发时重新路由）
     * @param deadline_ms 绝对超时（单调钟毫秒），超时回 503
     * @return {void}
     * @note 仅 I/O 线程调用。
     */
    void DeferDescribe(const std::string &uri, std::int64_t deadline_ms);

    /**
     * @brief 尝试补发挂起的 DESCRIBE。
     *
     * @return true 表示挂起请求已了结（补发响应/回 503/放弃路由）；
     *         false 表示无挂起、连接已关闭或仍在等待参数集（未超时）
     */
    bool TryServePendingDescribe();

    /** 是否存在挂起的 DESCRIBE。 */
    bool has_pending_describe() const
    {
        return pending_describe_;
    }

    /**
     * @brief 建立 track 的传输通道（UDP socket 或 TCP interleaved）。
     *
     * @param track 目标 track 运行时
     * @param transport 解析后的 Transport 头
     * @return 成功返回 Ok；hub 缺失、UDP 配额不足或发送器创建失败返回对应错误
     * @note 仅 I/O 线程调用；UDP 路径先占全局配额，失败路径保证归还。
     */
    Result EstablishTransport(TrackRuntime &track, const ipc_rtsp_shim_transport &transport);

    /** @brief 释放全部 track（TEARDOWN / 断连）。 @return {void} */
    void TeardownTracks();

    /**
     * @brief PLAY：注册订阅者并请求关键帧（或从 GOP cache 快照起播）。
     *
     * @param stream 码流标识
     * @param track_index 轨道下标（音/视频）
     * @return 成功返回 Ok；track 未建立或 hub 缺失返回 NotFound
     * @note 同码流音/视频轨共享一个订阅者，避免同帧重复投递。
     */
    Result StartPlaying(StreamId stream, int track_index);

    /** 每个 PLAY 周期开始时调用：音视频轨将重新共享一份回放快照。 */
    void ResetSharedPlaySnapshot()
    {
        shared_play_snapshot_ = GopSnapshot{};
        has_shared_play_snapshot_ = false;
    }

    /**
     * @brief PAUSE：停止该 track 的订阅。
     *
     * @param stream 码流标识
     * @param track_index 轨道下标
     * @return 成功返回 Ok；track 缺失返回 NotFound
     * @note 共享订阅者只解除自身引用，注销与去激活由属主 track 负责。
     */
    Result PauseTrack(StreamId stream, int track_index);

    /* ---- 访问器 ---- */

    /** 所属服务实现。 */
    ServerImpl &server();

    /** RTSP 会话状态（track、播放标志）。 */
    Session &session()
    {
        return session_;
    }

    const Session &session() const
    {
        return session_;
    }

    /** 本连接统计（RTP 计数、丢帧、拥塞等）。 */
    ClientStats &stats()
    {
        return stats_;
    }

    /** 对端 IP 文本。 */
    const std::string &peer_ip() const
    {
        return peer_ip_;
    }

    /** 对端端口。 */
    std::uint16_t peer_port() const
    {
        return peer_port_;
    }

    /** 当前正在处理的请求；仅本次请求处理期间有效（见 current_request_）。 */
    const ipc_rtsp_shim_request *request() const
    {
        return current_request_;
    }

    /** 本连接是否已 PLAY 过（用于日志/统计）。 */
    bool ever_played() const
    {
        return ever_played_;
    }

    /** 设置 ever_played 标记。 */
    void set_ever_played(bool value)
    {
        ever_played_ = value;
    }

    /** 本连接最近一次缓存的单调钟毫秒。 */
    std::int64_t now_ms() const
    {
        return now_ms_;
    }

    /**
     * @brief 媒体推送：把各订阅者的待发帧打包发出。
     *
     * @return {void}
     * @note 由 I/O 线程驱动：邮箱排空或连接可写（EPOLLOUT）时都会调用。
     */
    void FlushMedia();

    /**
     * @brief 补发参数集（VPS/SPS/PPS，AnnexB 起始码）。
     *
     * @param stream 参数集所属码流
     * @param track 目标 track（须已建立 sender）
     * @param raw_ts RTP 时间戳（与关键 AU 一致）
     * @param snapshot GOP 回放路径绑定的参数集快照；空则取 hub 当前参数集
     * @return {void}
     */
    void SendParameterSets(StreamId stream, TrackRuntime &track, std::uint32_t raw_ts, const CodecConfigSnapshot *snapshot = nullptr);

private:
    friend class Controller;

    void OnReadable();
    void OnWritable();
    void ProcessInput();
    void HandleInterleaved(std::uint8_t channel, const std::uint8_t *payload, std::size_t size);
    void FlushOutput();
    void UpdateInterest();
    void ResetIdleTimer();
    void OnIdleCheck();

    /** Writer 回调：RTSP 控制数据经 AppendControlToOutput 追加（可使用全部容量）。 */
    static ssize_t WriteControlThunk(void *user, const void *data, std::size_t len);
    /** Writer 回调：RTP/RTCP 媒体数据经 AppendMediaToOutput 追加（不得占用控制预留区）。 */
    static ssize_t WriteMediaThunk(void *user, const void *data, std::size_t len);
    /** Writer 回调：返回输出缓冲当前字节数（Writer::filled 语义）。 */
    static std::size_t FilledThunk(void *user);

    /** 控制数据追加到输出缓冲（可使用全部容量）。 */
    bool AppendControlToOutput(const void *data, std::size_t len);
    /** 媒体数据追加到输出缓冲（不占用控制预留区）。 */
    bool AppendMediaToOutput(const void *data, std::size_t len);

    /**
     * @brief 单帧打包发送（一个访问单元）。
     *
     * @param track 目标 track（须已建立 sender）
     * @param pending 待发送帧
     * @return true 表示本帧已处理完（含 MJPEG 解析失败丢帧的情形）；false 的全部情形：
     *         track 无 sender、hub 缺失、帧数据为空，或 RTP 打包/发送失败（含输出
     *         护栏不足等背压原因）
     */
    bool SendOneFrame(TrackRuntime &track, const PendingFrame &pending);

    /** 所属服务实现（memory: 非拥有，生命周期须覆盖本连接）。 */
    ServerImpl *server_ = nullptr;
    /** 所属事件循环（memory: 非拥有），事件与定时器均经它注册。 */
    EventLoop *loop_ = nullptr;
    /** 客户端 socket；监听器 accept 时申请，Close()/析构时由本类 close。 */
    int fd_ = -1;
    /** 对端 IP 文本（日志与 UDP track 回填用）。 */
    std::string peer_ip_;
    /** 对端端口。 */
    std::uint16_t peer_port_ = 0;
    /** Close() 已请求；置位后本连接不再处理任何 I/O。 */
    bool closing_ = false;
    /** 已注册到事件循环；Remove 后清除，防重复摘除。 */
    bool registered_ = false;
    /** 断开原因（Close 时写入，供统计上报）。 */
    DisconnectReason disconnect_reason_ = DisconnectReason::Unknown;

    /** 有界输出缓冲（时长制背压，容量来自 backpressure 配置）。 */
    OutputBuffer output_;
    /** RTSP 控制通道 Writer；Start() 经 shim 申请，析构 ipc_rtsp_shim_writer_free。 */
    ipc_rtsp_shim_writer *control_writer_ = nullptr;
    /** RTP/RTCP 媒体通道 Writer；Start() 经 shim 申请，析构 ipc_rtsp_shim_writer_free。 */
    ipc_rtsp_shim_writer *media_writer_ = nullptr;
    /** RTSP 请求解析器；Start() 经 shim 申请，析构 ipc_rtsp_shim_parser_free。 */
    ipc_rtsp_shim_parser *parser_ = nullptr;
    /** 当前请求的响应上下文；每请求创建，处理完立即释放。 */
    ipc_rtsp_shim_response *response_ = nullptr;

    /** 定容接收缓冲；容量取 backpressure.max_input_buffer_bytes，构造后不再变化。 */
    std::vector<std::uint8_t> input_;
    /** input_ 中有效字节数（有效数据恒在头部，消费后前移）。 */
    std::size_t input_len_ = 0;

    /** RTSP 会话状态（track、播放标志）。 */
    Session session_;
    /** Digest 鉴权状态。 */
    ConnectionAuthState auth_;
    /** 本连接统计。 */
    ClientStats stats_;
    /** 请求分发控制器（构造时创建，持有回指本连接的指针）。 */
    std::unique_ptr<Controller> controller_;

    /** 空闲检查定时器 id；0 表示无定时器。 */
    EventLoop::TimerId idle_timer_ = 0;
    /** 挂起 DESCRIBE 的超时定时器 id；0 表示无定时器。 */
    EventLoop::TimerId describe_timer_ = 0;
    /** 最近一次缓存的单调钟毫秒（CLOCK_MONOTONIC）。 */
    std::int64_t now_ms_ = 0;
    /** 最近活动时刻，单调钟毫秒；空闲/会话超时判定的基准。 */
    std::int64_t last_activity_ms_ = 0;

    /** 当前正在处理的请求（memory: 非拥有，指向解析器内部数据，仅本次请求处理期间有效）。 */
    const ipc_rtsp_shim_request *current_request_ = nullptr;
    /** 当前请求的原始 URI（仅本次请求处理期间有效）。 */
    std::string request_uri_;
    /** 当前请求方法（仅本次请求处理期间有效）。 */
    std::string request_method_;

    /** 当前 PLAY 周期内音视频轨共享的 GOP 回放快照：两轨必须从同一起点
     * 回放，否则播放端音画起点错位一个 GOP 相位（VLC 进度时间回跳）。 */
    GopSnapshot shared_play_snapshot_;
    /* 共享快照是否有效；无效时每次 PLAY 重试 AttachReader。 */
    bool has_shared_play_snapshot_ = false;

    /** 是否存在挂起的 DESCRIBE（参数集未就绪）。 */
    bool pending_describe_ = false;
    /** 挂起请求的 CSeq（补发响应时回填）。 */
    std::uint32_t pending_describe_cseq_ = 0;
    /** 挂起请求的原始 URI。 */
    std::string pending_describe_uri_;
    /** 挂起请求的超时时刻，单调钟毫秒（绝对值）。 */
    std::int64_t pending_describe_deadline_ms_ = 0;

    /** 本连接是否已 PLAY 过（用于日志/统计）。 */
    bool ever_played_ = false;
    /** 发送侧 Debug 日志预算（仅用于短时间诊断，避免刷屏）。 */
    int tx_log_budget_ = 40;
    /** TCP 发送进展跟踪（拥塞无进展超时判定用）。 */
    CongestionTracker congestion_;
};

} // namespace detail
} // namespace ipc_rtsp
