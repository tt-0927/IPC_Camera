/**
 * @FilePath     : server_impl.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : Server 内部实现（不对外暴露）
 */

/* 职责：持有配置、码流中枢、连接表、事件循环线程与媒体邮箱；对外只暴露
 * ipc_rtsp::Server 的薄转发层。I/O 状态仅归 loop 线程所有；生命周期操作经
 * lifecycle_operation_mutex_ 串行化，并以 generation 校验实现跨代隔离。 */
#pragma once

#include "ipc_rtsp/config.h"
#include "ipc_rtsp/frame.h"
#include "ipc_rtsp/log.h"
#include "ipc_rtsp/result.h"
#include "ipc_rtsp/status.h"
#include "media/frame_hub.h"
#include "media/media_mailbox.h"
#include "metrics/metrics.h"
#include "net/event_loop.h"
#include "net/tcp_listener.h"
#include "rtsp/url_router.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

class Connection;

/** 服务实现。 */
class ServerImpl
{
public:
    /** 默认构造：实例处于 Stopped，须先 Configure 成功才可 Start。
     *  @return {void} */
    ServerImpl();
    /** 析构兜底关停：先以 200ms（等待 I/O 线程退出的经验上限）优雅 Shutdown，
     *  失败则强制 Stop 循环并 join，保证不遗留引用 this 的 I/O 线程。
     *  @return {void}
     *  @note 须在非 I/O 线程析构；Shutdown 会拒绝 I/O 线程内同步调用。 */
    ~ServerImpl();

    ServerImpl(const ServerImpl &) = delete;
    ServerImpl &operator=(const ServerImpl &) = delete;

    /**
     * 写入服务与码流配置（仅 Stopped 状态可调用；码流先校验后归一化）。
     *
     * @param config  服务配置
     * @param streams 码流配置（至少 kStreamCount 路；任一路非法则整批拒绝）
     * @return Ok 成功；AlreadyInitialized 非 Stopped；InvalidArgument/
     *         Unsupported 配置非法
     */
    Result Configure(const ServerConfig &config, const std::vector<StreamConfig> &streams);

    /**
     * 启动监听并拉起 I/O 线程（阻塞至 I/O 线程就绪或超时）。
     *
     * @return Ok 就绪；AlreadyInitialized 已在运行；Busy 正在停止；
     *         Timeout ready 超时（内部已回滚到 Stopped）
     * @note 与 Shutdown/控制面递交互斥（lifecycle_operation_mutex_）；同一
     *       实例关停后可再次 Start，generation 递增隔离新旧代回调。
     */
    Result Start();

    /**
     * 停止接受新连接（已建立连接与会话继续）。
     *
     * @return Ok 已停或停成功；Busy Starting/Stopping 阶段不接受
     */
    Result StopAccept();

    /**
     * 停止接收生产者媒体帧（已有客户端继续；邮箱内残留帧仍会被处理完）。
     *
     * @return Ok 总是成功（幂等标志位）
     */
    Result StopIngest();

    /**
     * 完整关停：停监听、停 ingest、在 I/O 线程内断开全部客户端并退出循环。
     *
     * @param deadline 等待 I/O 线程退出的上限；超时返回 Timeout，关停流程
     *                 仍在后台继续
     * @return Ok 已关停；Busy Starting 阶段或 I/O 线程内同步调用；Timeout 超时
     * @note 幂等：已 Stopped 时补做 join 与终态清理后直接成功。
     */
    Result Shutdown(std::chrono::milliseconds deadline);

    /* ---- 生产者线程接口 ---- */

    /**
     * 推入一帧视频（调用方持有数据；有订阅者或需采集参数集时在入队前复制一次）。
     *
     * @param id 码流标识
     * @param frame 帧视图（数据仅调用期间有效）
     * @return Ok 已入队或无需复制；InvalidArgument 未启用/参数非法；
     *         NoCapacity 超单帧上限或邮箱满
     * @note 生产者线程调用；邮箱满丢帧不阻塞（freshness > completeness）。
     */
    Result PushVideoView(StreamId id, const VideoFrameView &frame);

    /**
     * 推入一帧共享视频（引用计数共享，库内不复制 payload）。
     *
     * @param id 码流标识
     * @param frame 共享帧（库只增加引用计数）
     * @param meta 帧元数据
     * @return Ok 已入队；InvalidArgument 未启用/参数非法；NoCapacity 超单帧
     *         上限或邮箱满
     * @note 生产者线程调用。
     */
    Result PushVideoShared(StreamId id, const SharedVideoFrame &frame, const VideoFrameMeta &meta);

    /**
     * 推入一帧音频（payload 内联复制进邮箱槽位）。
     *
     * @param id 码流标识
     * @param frame 音频帧视图（数据仅调用期间有效）
     * @return Ok 已入队/无需入队；InvalidArgument 码流未挂音频轨或数据非法；
     *         NoCapacity 超上限或邮箱满
     * @note 生产者线程调用；无订阅者时仅当 GOP cache 仍需积累才入队。
     */
    Result PushAudio(StreamId id, const AudioFrameView &frame);

    /* ---- 控制面热更新 ---- */

    /**
     * 热更新用户名/密码（新请求立即生效，进行中的鉴权不受影响）。
     *
     * @param user 用户名
     * @param password 密码
     * @return Ok 已更新；Busy/ShuttingDown/Timeout 递交 I/O 线程失败
     */
    Result UpdateCredential(const std::string &user, const std::string &password);

    /**
     * 更新 Digest 鉴权算法（当前仅支持 MD5，其他值显式降级为 MD5 并告警）。
     *
     * @param algorithm 期望算法
     * @return Ok 已更新（可能被降级）；Busy/ShuttingDown/Timeout 递交失败
     */
    Result UpdateDigestAlgorithm(DigestAlgorithm algorithm);

    /**
     * 更新对外展示地址（影响 SDP c=、Url() 与 AdvertisedIp()）。
     *
     * @param ip 展示 IP；空串表示回退 bind_address
     * @return Ok 已更新；Busy/ShuttingDown/Timeout 递交失败
     */
    Result UpdateAdvertisedIp(const std::string &ip);

    /**
     * 热更新码流配置（先校验后归一化；只影响新连接，在播 track 沿用旧参数）。
     *
     * @param streams 新码流配置
     * @return Ok 已生效；InvalidArgument/Unsupported 校验失败；
     *         Busy/ShuttingDown/Timeout 递交失败
     */
    Result UpdateStreamConfig(const std::vector<StreamConfig> &streams);

    /**
     * 注册/替换 IDR 请求回调（经控制面递交，I/O 线程内生效）。
     *
     * @param callback 回调（传空即清除）
     * @return {void}
     * @note 回调在 I/O 线程内被调用，必须非阻塞；库内已做全局限频。
     */
    void SetIdrRequestCallback(std::function<void(StreamId)> callback);

    /**
     * 生成对外展示的 RTSP URL（经控制面在 I/O 线程内读配置后拼接）。
     *
     * @param id 码流标识
     * @param with_credentials 是否携带 user:password@ 前缀
     * @return 完整 URL；码流不存在或递交失败时返回空串
     */
    std::string Url(StreamId id, bool with_credentials) const;

    /**
     * 读取运行指标快照（I/O 线程内全量统计；递交失败时降级为仅原子计数口径）。
     *
     * @return 指标快照（始终可用；降级口径下 GOP 状态字段为 0）
     */
    MetricsSnapshot Snapshot() const;

    /** 内部白盒测试/诊断：每次 Start 尝试递增。 */
    std::uint64_t instance_generation() const
    {
        return instance_generation_.load(std::memory_order_acquire);
    }

    /* ---- I/O 线程接口（供 Connection/Controller 使用） ---- */

    /** 只读配置视图：I/O 线程内直读口径（见 config_ 注释），跨线程只读须走
     *  AdvertisedIp()/Url() 等持锁接口。 */
    const ServerConfig &config() const
    {
        return config_;
    }

    /**
     * 可变配置访问：仅供 I/O 线程内的初始化/控制路径使用（如 Controller 改
     * 鉴权字段）；非 I/O 线程修改配置必须走 Update* 控制面接口。
     *
     * @return config_ 的可变引用（不得缓存到 I/O 线程之外）
     */
    ServerConfig &mutable_config()
    {
        return config_;
    }

    /** URL 路由表（Start 后仅 I/O 线程使用）。 */
    UrlRouter &router()
    {
        return router_;
    }

    /**
     * 取码流中枢。
     *
     * @param id 码流标识
     * @return 对应 FrameHub（指针在 Configure 后稳定，UpdateStreamConfig 只
     *         原地改配置不换对象）；该码流未配置返回 nullptr
     */
    FrameHub *hub(StreamId id);
    /** hub() 的只读重载。 */
    const FrameHub *hub(StreamId id) const;

    /** 事件循环（仅 I/O 线程操作其定时器/任务接口）。 */
    EventLoop &loop()
    {
        return loop_;
    }

    /** 生产者媒体入口是否使能（relaxed 读，仅作快速拒绝的近似值）。 */
    bool ingest_enabled() const
    {
        return ingest_enabled_.load(std::memory_order_relaxed);
    }

    /** 是否仍在接受新连接（relaxed 读，仅作快速关闭的近似值）。 */
    bool accepting() const
    {
        return accepting_.load(std::memory_order_relaxed);
    }

    /**
     * 对外展示地址（SDP c= 与 URL 拼接用）。
     *
     * @return advertised_ip_；为空时回退 config_.bind_address，再回退 "0.0.0.0"
     */
    std::string AdvertisedIp() const;

    /**
     * 登记新连接（仅 I/O 线程，OnAccept 调用）。
     *
     * @param connection 待登记连接（接管所有权）
     * @return Ok 已登记；InvalidArgument 空指针
     */
    Result RegisterConnection(std::unique_ptr<Connection> connection);

    /**
     * 连接关闭通知：释放单 IP 计数并把连接移入 retired_ 延迟析构。
     *
     * @param connection 即将关闭的连接（所有权随 connections_ 移入 retired_）
     * @return {void}
     */
    void NoteConnectionClosed(Connection *connection);

    /**
     * 当前正在播放的流数（"总取流路数"口径，与 MediaMTX/ZLMediaKit 的
     * session 级读者统计一致）。
     *
     * @return 每连接按其正在播放的不同码流数累计：单流客户端计 1；
     *         同一连接同时拉主+子码流计 2；复合流（音视频同码流）仍计 1
     */
    int PlayingStreamCount() const;

    /**
     * 指定码流上正在播放的客户端数。
     *
     * @param id 码流标识
     * @return 该码流上存在 playing 状态 track 的连接数；
     *         复合流客户端的音视频轨只计 1
     */
    int PlayingClientCount(StreamId id) const;

    /**
     * 按全局限频请求编码器立即产出 IDR：命中限频窗口或 MJPEG 流时跳过；
     * 通过时在 I/O 线程触发注册的回调并计入 idr_request_count_。
     *
     * @param id 码流标识
     * @return {void}
     * @note 仅 I/O 线程调用；idr_callback_ 由注册方保证非阻塞。
     */
    void RequestIdr(StreamId id);

    /**
     * 生产者线程：唤醒 I/O 线程处理媒体邮箱（投递邮箱后调用，写 eventfd）。
     *
     * @return {void}
     * @note loop 未运行时为 no-op。
     */
    void WakeForMedia();

    /**
     * I/O 线程：排空邮箱、扇出、补发挂起响应并推送媒体。
     *
     * @return {void}
     */
    void DrainMedia();

    /**
     * 申请一个全局 UDP track 配额（上限 config_.backpressure.max_udp_tracks，
     * 每 track 占 2 个 fd）。
     *
     * @return true 配额已占住；false 配额已满，应拒绝 UDP 传输方式
     * @note 仅 I/O 线程调用（会话 SETUP/拆除路径），无并发。
     */
    bool TryAcquireUdpTrack();

    /**
     * 归还一个 UDP track 配额（仅 I/O 线程调用）。
     *
     * @return {void}
     * @note 计数为 0 时调用是无害 no-op（防御性）。
     */
    void ReleaseUdpTrack();

    /**
     * 服务级计数器集合：连接准入/拒绝、收发字节与包数、鉴权/解析失败、
     * 拥塞断开、ingest 丢帧等（口径详见 metrics.h）。
     *
     * @return 计数器集合引用（供 Connection 等路径 relaxed 自增、快照读取）
     */
    ServerCounters &counters()
    {
        return counters_;
    }

    /** 取当前连接数（用于上限判断）。 */
    std::size_t ConnectionCount() const
    {
        return connections_.size();
    }

private:
    /** 生命周期状态机（转移时机见各成员同行注释；lock: lifecycle_mutex_）。 */
    enum class LifecycleState
    {
        Stopped = 0, /**< 初始/已完全收尾：Configure 与 Start 的唯一合法起点 */
        Starting,    /**< Start 已受理：I/O 线程尚未报告就绪（失败早退或 OnLoopReady 转出） */
        Running,     /**< I/O 线程就绪、正常服务（OnLoopReady 置位） */
        Stopping,    /**< 关停中：Shutdown 受理、启动超时兜底或循环已退出；Finalize 后回 Stopped */
    };

    /** 每轮事件处理末尾回收 retired_ 中已关闭连接（仅 I/O 线程）。 */
    void ReapRetiredConnections();
    /** I/O 线程 accept 回调：总量/单 IP 准入检查后建连。 */
    void OnAccept(int fd, const std::string &peer_ip, std::uint16_t peer_port);
    /** 单 IP 并发计数：准入门限见 config.max_connections_per_ip（I/O 线程 only）。 */
    bool AdmitConnectionPerIp(const std::string &ip);
    /** 释放一条单 IP 并发计数（计数归零即删条目；仅 I/O 线程）。 */
    void ReleaseConnectionPerIp(const std::string &ip);
    /** 查询该 IP 当前并发连接数（仅 I/O 线程）。 */
    int CountConnectionsPerIp(const std::string &ip) const;
    /** review: 当前无实现、无调用点，疑似残留声明。 */
    void OnMediaWritable();
    /**
     * 控制面同步执行：把任务递交 I/O 线程串行执行并等待结果。
     *
     * @param task 控制任务（在 I/O 线程、生命周期状态锁外执行）
     * @return 任务返回值；Stopped 时就地执行；Starting/Stopping/超时对应
     *         Busy/ShuttingDown/Timeout
     */
    Result ExecuteControl(std::function<Result()> task);
    /**
     * 拼接对外展示 URL（内部持 config_mutex_ 读配置）。
     *
     * @param id 码流标识
     * @param with_credentials 是否携带 user:password@ 前缀
     * @return 完整 URL；码流不存在返回空串
     */
    std::string BuildUrl(StreamId id, bool with_credentials) const;
    /** I/O 线程内全量统计快照（连接/会话/track 口径）。 */
    MetricsSnapshot BuildSnapshot() const;
    /**
     * 构造指标快照（连接/会话等非原子字段不填）。
     *
     * @param from_atomics true 为超时降级口径：GOP 状态字段填 0（可能运行在
     *        I/O 线程之外）；false 为 I/O 线程内/Stopped 直执的安全全量口径
     * @return 指标快照
     */
    MetricsSnapshot BuildCountersOnlySnapshot(bool from_atomics) const;
    /** I/O 线程首跑回调：Starting→Running 转移（语义见实现）。 */
    void OnLoopReady(std::uint64_t generation);
    /** I/O 线程退出回调：置 Stopping + loop_exited_（语义见实现）。 */
    void OnLoopStopped(std::uint64_t generation);
    /** 回收 loop 线程（非线程自身时 join；幂等，可重复调用）。 */
    void JoinLoopThread();
    /** 终态清理：停监听、清连接表/邮箱并置 Stopped（幂等）。 */
    void FinalizeStoppedState();
    /** 周期指标打印已下线（实现 #if 0 保留），声明暂留。 */
    void ScheduleStatsTimer();
    /** 同上：周期指标打印已下线（实现 #if 0 保留），声明暂留。 */
    void OnStatsTimer();

    /**
     * 服务配置。写路径一律持 config_mutex_，且 Start 后的写入只发生在 I/O
     * 线程内（经 ExecuteControl）——I/O 线程内可无锁直读；跨线程只读
     * （AdvertisedIp/BuildUrl/Url）必须持 config_mutex_。
     */
    ServerConfig config_;
    /** 每路码流配置（与 config_ 同锁同口径：Configure/UpdateStreamConfig 写入）。 */
    std::vector<StreamConfig> streams_;
    /** URL 路由表（PATH → 码流映射；Start 后仅 I/O 线程查询）。 */
    UrlRouter router_;

    /** 每路码流的帧中枢（Configure 建立后指针稳定，UpdateStreamConfig 只改配置不换对象）。 */
    std::unique_ptr<FrameHub> hubs_[kStreamCount];
    /**
     * 生产者线程 → I/O 线程的有界媒体邮箱：视频/音频各 16 槽（编译期常量），
     * 满时丢弃新帧并返回 false（freshness > completeness）。
     */
    MediaMailbox mailbox_;
    /** 服务级计数器（各字段 relaxed 自增，无锁；口径见 metrics.h）。 */
    ServerCounters counters_;

    /** 事件循环：由 I/O 线程驱动；其他线程只允许 Wake/Post。 */
    EventLoop loop_;
    /** TCP 监听器（Start/Stop 仅在受控路径调用，accept 回调在 I/O 线程）。 */
    TcpListener listener_;
    /**
     * I/O 线程实体。join 边界：JoinLoopThread 的调用点——下一次 Start、
     * Shutdown 成功/已停路径、Start ready 超时回滚与析构兜底；线程自身内
     * 不 join（JoinLoopThread 自检 get_id）。
     */
    std::thread loop_thread_;
    /** 是否接受新连接（relaxed 近似标志：Start/关停路径写，OnAccept 读）。 */
    std::atomic<bool> accepting_{ false };
    /** 是否接收生产者帧（relaxed 近似标志：Start/StopIngest/关停写，生产者与 I/O 钩子读）。 */
    std::atomic<bool> ingest_enabled_{ false };

    /**
     * 生命周期操作串行化闸门。
     * lock: 保证 Start/Shutdown/控制面递交互斥、关停不跨越已递交任务；
     * 不保护任何具体状态字段（那些归 lifecycle_mutex_）。
     */
    std::mutex lifecycle_operation_mutex_;
    /** lock: 保护 lifecycle_state_/loop_exited_ 的转移与读取，配合 lifecycle_cv_ 做有界等待。 */
    mutable std::mutex lifecycle_mutex_;
    /** 生命周期等待队列（Start ready 等待与 Shutdown 有界等待都挂在上面）。 */
    std::condition_variable lifecycle_cv_;
    /** 当前生命周期状态（不变量：非 Stopped 期间 I/O 资源有效）。lock: lifecycle_mutex_。 */
    LifecycleState lifecycle_state_ = LifecycleState::Stopped;
    /**
     * 循环是否已退出；初值 true 表示"当前无线程在跑"。Start 置 false，循环
     * 退出或终态清理置 true，是 Shutdown 等待条件之一。lock: lifecycle_mutex_。
     */
    bool loop_exited_ = true;
    /** Start 代数：每次 Start 递增，异步回调据此丢弃旧代事件（防串代）。 */
    std::atomic<std::uint64_t> instance_generation_{ 0 };

    /**
     * 活跃连接表（fd → 连接）。memory: unique_ptr 独占拥有连接；仅 I/O 线程
     * 访问，NoteConnectionClosed 时移入 retired_ 延迟析构。
     */
    std::map<int, std::unique_ptr<Connection>> connections_;
    /**
     * 本轮事件处理结束后统一析构的已关闭连接（延迟析构，避免在连接自身回调
     * 栈内 delete）；仅 I/O 线程访问，每轮 hook 的 ReapRetiredConnections 清空。
     */
    std::vector<std::unique_ptr<Connection>> retired_;
    /** 每 IP 并发连接计数；条目数 ≤ max_connections，随连接关闭递减清空。 */
    std::vector<std::pair<std::string, int>> ip_connections_;

    /** lock: 保护 config_/streams_/advertised_ip_ 的跨线程读写（Configure、
     *  热更新、Url/AdvertisedIp/BuildUrl）；I/O 线程内直读不走此锁。 */
    mutable std::mutex config_mutex_;
    /** SDP c=/URL 对外展示地址；空则回退 config_.bind_address。lock: config_mutex_。 */
    std::string advertised_ip_;

    /** 当前活跃 UDP track 数（全局配额 max_udp_tracks）；仅 I/O 线程增减。 */
    std::atomic<std::size_t> udp_tracks_{ 0 };
    /** 外部 IDR 请求回调：仅 I/O 线程调用且必须非阻塞；经 SetIdrRequestCallback 换入。 */
    std::function<void(StreamId)> idr_callback_;
    /** 每路码流上次外部 IDR 请求时刻（CLOCK_MONOTONIC 毫秒；0 表示从未请求）；
     *  仅 I/O 线程访问。 */
    std::int64_t last_idr_request_ms_[kStreamCount] = { 0, 0 };
    /** 每路码流已触发的外部 IDR 请求累计（relaxed 自增，诊断用）。 */
    std::atomic<std::uint64_t> idr_request_count_[kStreamCount] = { { 0 }, { 0 } };

    /** 媒体唤醒用 eventfd 的 handler 已注册在 loop 上。 */
    bool media_handler_registered_ = false;

    /** 周期运行指标定时器（归属 loop，loop 停止自动失效）；周期打印下线后暂无使用者。 */
    EventLoop::TimerId stats_timer_ = 0;

    /** 连续无订阅者的指标周期数（仅 I/O 线程访问）：用于无人观看时降频打印。 */
    std::uint32_t stats_idle_cycles_ = 0;
};

/**
 * 分析一帧视频：关键 NAL、参数集与 VCL 数量（顺序扫描一遍，不分配）。
 *
 * @param codec 码流编码（MJPEG 无 NAL 结构，整帧视为关键帧）
 * @param data 帧数据（Annex-B）
 * @param size 帧字节数
 * @param caller_key 调用方从编码层得到的关键帧标记：与 NAL 扫描结果取或，
 *        扫描未识别（非标码流）时仍以上游标记兜底
 * @return 帧特征汇总；data/size 非法时返回全零 FrameInfo
 */
FrameInfo AnalyzeFrame(Codec codec, const std::uint8_t *data, std::size_t size, bool caller_key);

} // namespace detail
} // namespace ipc_rtsp
