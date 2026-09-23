/**
 * @FilePath     : server.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 11:35:00
 * @Description  : 库内实现：ServerImpl 与对外 Server 门面
 */

#include "ipc_rtsp/server.h"

#include "ipc_rtsp_build_info.h"
#include "media/annexb_scanner.h"
#include "net/connection.h"
#include "server_impl.h"
#include "support/log.h"
#include "support/text.h"
#include "support/time.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <new>
#include <sys/epoll.h>
#include <unistd.h>

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr const char *kTag = "server";
/** Start 内等待 I/O 线程首次跑通（OnLoopReady 置 Running）的超时上限。 */
constexpr std::chrono::milliseconds kStartReadyTimeout{ 2000 };
/** ExecuteControl 等待控制任务在 I/O 线程执行完成的超时上限。 */
constexpr std::chrono::milliseconds kControlTimeout{ 1000 };

/**
 * 校验码流配置合法性（不做归一化；归一化见 NormalizeStreamCodecDefaults）。
 *
 * @param streams 待校验的码流配置表
 * @return Ok 全部合法；Fail(InvalidArgument/Unsupported) 为首个非法项的错误码
 */
Result ValidateStreams(const std::vector<StreamConfig> &streams)
{
    if (streams.size() < static_cast<std::size_t>(kStreamCount))
    {
        return Result::Fail(Status::InvalidArgument);
    }
    for (const StreamConfig &stream : streams)
    {
        if (StreamIndex(stream.id) < 0)
        {
            return Result::Fail(Status::InvalidArgument);
        }
        if (stream.path.empty() || stream.path.front() == '/')
        {
            return Result::Fail(Status::InvalidArgument);
        }
        if (stream.codec == Codec::Unknown)
        {
            return Result::Fail(Status::Unsupported);
        }
        if (stream.max_frame_bytes == 0 || stream.max_nalu_bytes == 0)
        {
            return Result::Fail(Status::InvalidArgument);
        }
        /* GOP cache 配置：三项全 0 表示关闭；部分为 0 无法表达一致语义，
         * 直接拒绝，避免“半开的 cache”在运行期产生歧义行为。 */
        const GopCacheConfig &gop = stream.gop_cache;
        const bool gop_disabled = gop.max_bytes == 0 && gop.max_frames == 0 && gop.max_duration_ms == 0;
        if (!gop_disabled && (gop.max_bytes == 0 || gop.max_frames == 0 || gop.max_duration_ms == 0))
        {
            return Result::Fail(Status::InvalidArgument);
        }
    }
    return Result::Ok();
}

/**
 * 按 codec 归一化协议可推导字段（配置写入后调用）。
 *
 * @param stream 单路码流配置，就地回填 payload_type/clock_rate/audio_clock_rate
 * @return {void}
 */
void NormalizeStreamCodecDefaults(StreamConfig *stream)
{
    if (stream->codec == Codec::MJPEG)
    {
        /* RFC 2435 静态负载类型 26、时钟 90000，与旧 live555 服务器一致；
         * MJPEG 的分片预算沿用 max_nalu_bytes（同为单包 RTP 负载上限）。 */
        stream->payload_type = 26;
        stream->clock_rate = 90000;
    }

    /* G.711/G.726 是 8kHz 编码：RTSP 时钟为协议常量（RFC 3551 静态 PT 8/0、
     * G726-32/8000），与设备音频引擎的采集采样率无关——16kHz 采集经平台
     * 重采样后 AENC 仍按 8kHz 出帧，若时钟照抄设备配置会把数据播成两倍速
     * （旧 live555 路径同样硬编码 8000）。AAC 时钟必须取真实采样率，保留
     * 上层传值。 */
    if (stream->audio_codec == AudioCodec::G711A || stream->audio_codec == AudioCodec::G711U || stream->audio_codec == AudioCodec::G726_32)
    {
        stream->audio_clock_rate = 8000;
    }
}

/**
 * GOP cache 预算装不下单个最大帧时告警：实时发送不受影响，但超限帧进不了
 * cache，GOP 建不起来，起播只能走 WaitStart + IDR。
 *
 * 启动配置（Configure）与热更新（UpdateStreamConfig）两条路径共用，
 * 避免热更新静默改变 cache 行为。
 *
 * @param index 码流下标（用于日志定位）
 * @param stream 待应用的码流配置（取 gop_cache 与 max_frame_bytes 比较）
 * @return {void}
 */
void WarnGopBudgetIfUnderflow(int index, const StreamConfig &stream)
{
    if (stream.gop_cache.max_bytes != 0 && stream.gop_cache.max_bytes < stream.max_frame_bytes)
    {
        IPC_RTSP_LOGW(kTag,
                      "GOP cache预算小于单帧上限 stream:%d cache:%zu frame:%zu 超限帧不入cache",
                      index,
                      stream.gop_cache.max_bytes,
                      stream.max_frame_bytes);
    }
}

} // namespace

/** 逐 NAL 扫描帧特征；caller_key 与扫描结果取或（语义详见声明处 Doxygen）。 */
FrameInfo AnalyzeFrame(Codec codec, const std::uint8_t *data, std::size_t size, bool caller_key)
{
    FrameInfo info;
    if (data == nullptr || size == 0)
    {
        return info;
    }
    if (codec == Codec::MJPEG)
    {
        /* MJPEG 无 NAL 结构：每帧都是独立可解码的关键帧。 */
        info.has_key_nal = true;
        info.has_parameter_set = true;
        info.vcl_count = 1;
        return info;
    }
    if (caller_key)
    {
        info.has_key_nal = true;
    }
    ForEachNal(codec,
               data,
               size,
               [&](const NalRange &nal)
               {
                   if (IsKeyNal(nal.type))
                   {
                       info.has_key_nal = true;
                   }
                   if (IsParameterSet(nal.type))
                   {
                       info.has_parameter_set = true;
                   }
                   if (IsVclNal(nal.type))
                   {
                       ++info.vcl_count;
                   }
                   return true;
               });
    return info;
}

/** 默认构造：实例处于 Stopped，须先 Configure 成功才可 Start。 */
ServerImpl::ServerImpl() = default;

/**
 * 析构兜底关停：先以 200ms（等待 I/O 线程退出的经验上限，非协议要求）做一次
 * 优雅 Shutdown；失败说明循环未按时退出，改为强制停循环并 join。
 *
 * @return {void}
 * @note 时序：优雅 Shutdown → 超时则 loop_.Stop + JoinLoopThread +
 *       FinalizeStoppedState；保证任何引用 this 的回调先于成员析构结束。
 */
ServerImpl::~ServerImpl()
{
    const Result stopped = Shutdown(std::chrono::milliseconds(200));
    if (!stopped.ok())
    {
        /* 析构不能把仍引用 this 的 I/O 线程留在后台。 */
        loop_.Stop();
        JoinLoopThread();
        FinalizeStoppedState();
    }
}

Result ServerImpl::Configure(const ServerConfig &config, const std::vector<StreamConfig> &streams)
{
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        if (lifecycle_state_ != LifecycleState::Stopped)
        {
            return Result::Fail(Status::AlreadyInitialized);
        }
    }
    const Result valid = ValidateStreams(streams);
    if (!valid.ok())
    {
        IPC_RTSP_LOGE(kTag, "码流配置非法 status:%s", StatusToString(valid.code));
        return valid;
    }
    /* 时长制背压的参数合法性：0 或倒挂会让所有客户端立即拥塞/断开。 */
    const BackpressureConfig &bp = config.backpressure;
    if (bp.client_soft_backlog_ms == 0 || bp.client_hard_backlog_ms < bp.client_soft_backlog_ms ||
        bp.client_output_guard_bytes <= bp.client_control_reserve_bytes)
    {
        IPC_RTSP_LOGE(kTag,
                      "背压配置非法 soft-ms:%u hard-ms:%u guard:%zu reserve:%zu",
                      static_cast<unsigned>(bp.client_soft_backlog_ms),
                      static_cast<unsigned>(bp.client_hard_backlog_ms),
                      bp.client_output_guard_bytes,
                      bp.client_control_reserve_bytes);
        return Result::Fail(Status::InvalidArgument);
    }

    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    streams_ = streams;
    for (StreamConfig &stream : streams_)
    {
        NormalizeStreamCodecDefaults(&stream);
    }
    advertised_ip_ = config.advertised_ip;
    router_.Configure(streams_);

    for (const StreamConfig &stream : streams_)
    {
        const int index = StreamIndex(stream.id);
        if (index < 0)
        {
            continue;
        }
        hubs_[index] = std::make_unique<FrameHub>(stream.id, stream, config_.backpressure);
        WarnGopBudgetIfUnderflow(index, stream);
    }
    return Result::Ok();
}

Result ServerImpl::Start()
{
    std::lock_guard<std::mutex> operation_lock(lifecycle_operation_mutex_);

    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        if (lifecycle_state_ == LifecycleState::Running || lifecycle_state_ == LifecycleState::Starting)
        {
            return Result::Fail(Status::AlreadyInitialized);
        }
        if (lifecycle_state_ == LifecycleState::Stopping)
        {
            return Result::Fail(Status::Busy);
        }
    }

    /* 上一代循环可能已异步退出但尚未 join。 */
    JoinLoopThread();

    const std::uint64_t generation = instance_generation_.fetch_add(1, std::memory_order_acq_rel) + 1;
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        lifecycle_state_ = LifecycleState::Starting;
        loop_exited_ = false;
    }
    IPC_RTSP_LOGI(kTag, "RTSP生命周期 generation:%llu state:Starting", static_cast<unsigned long long>(generation));

    IPC_RTSP_LOGI(kTag,
                  "RTSP产物身份 backend:smolrtsp build-id:%s revision:%s smolrtsp:%s smolrtsp-libevent:%s",
                  IPC_RTSP_BUILD_ID,
                  IPC_RTSP_SOURCE_REVISION,
                  IPC_RTSP_SMOLRTSP_REVISION,
                  IPC_RTSP_SMOLRTSP_LIBEVENT_REVISION);

    Result result = loop_.Init();
    if (!result.ok())
    {
        IPC_RTSP_LOGE(kTag, "事件循环初始化失败 errno:%d", result.sys_errno);
        {
            std::lock_guard<std::mutex> lock(lifecycle_mutex_);
            lifecycle_state_ = LifecycleState::Stopped;
            loop_exited_ = true;
        }
        lifecycle_cv_.notify_all();
        return result;
    }

    /* 邮箱唤醒：生产者线程投递后写 eventfd，循环线程排空。 */
    media_handler_registered_ = true;
    loop_.SetPostDispatchHook(
        [this]()
        {
            /* 每轮事件处理后：排空媒体邮箱并按需发送，再回收已关闭连接。
             * 顺序不能反：FlushMedia 可能触发新的关闭。 */
            if (ingest_enabled_.load(std::memory_order_relaxed) || !mailbox_.Empty())
            {
                DrainMedia();
            }
            ReapRetiredConnections();
        });

    result = listener_.Start(&loop_,
                             config_.bind_address,
                             config_.port,
                             [this](int fd, const std::string &peer_ip, std::uint16_t peer_port)
                             {
                                 OnAccept(fd, peer_ip, peer_port);
                             });
    if (!result.ok())
    {
        IPC_RTSP_LOGE(kTag,
                      "监听失败 addr:%s port:%u errno:%d",
                      config_.bind_address.c_str(),
                      static_cast<unsigned>(config_.port),
                      result.sys_errno);
        media_handler_registered_ = false;
        {
            std::lock_guard<std::mutex> lock(lifecycle_mutex_);
            lifecycle_state_ = LifecycleState::Stopped;
            loop_exited_ = true;
        }
        lifecycle_cv_.notify_all();
        return result;
    }
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_.port = listener_.port();
    }

    /* 先开 accept/ingest 使能再起线程：accept 回调与推帧路径以这两个标志做
     * 快速拒绝，循环一运行即可正常受理；关停路径同样先关门再停循环。 */
    accepting_.store(true, std::memory_order_relaxed);
    ingest_enabled_.store(true, std::memory_order_relaxed);
    loop_thread_ = std::thread(
        [this, generation]()
        {
            loop_.Run(
                [this, generation]()
                {
                    OnLoopReady(generation);
                });
            OnLoopStopped(generation);
        });

    {
        std::unique_lock<std::mutex> lock(lifecycle_mutex_);
        const bool ready = lifecycle_cv_.wait_for(lock,
                                                  kStartReadyTimeout,
                                                  [this, generation]()
                                                  {
                                                      return instance_generation_.load(std::memory_order_acquire) != generation ||
                                                             lifecycle_state_ != LifecycleState::Starting;
                                                  });
        if (!ready || lifecycle_state_ != LifecycleState::Running)
        {
            lifecycle_state_ = LifecycleState::Stopping;
            lock.unlock();
            accepting_.store(false, std::memory_order_relaxed);
            ingest_enabled_.store(false, std::memory_order_relaxed);
            loop_.Stop();
            JoinLoopThread();
            listener_.Stop();
            FinalizeStoppedState();
            IPC_RTSP_LOGE(kTag, "RTSP启动 ready 超时 generation:%llu", static_cast<unsigned long long>(generation));
            return Result::Fail(Status::Timeout);
        }
    }

    IPC_RTSP_LOGI(kTag,
                  "RTSP服务已启动 generation:%llu port:%u maxconn:%d maxplaying:%d",
                  static_cast<unsigned long long>(generation),
                  static_cast<unsigned>(config_.port),
                  config_.max_connections,
                  config_.max_playing_clients);
    return Result::Ok();
}

Result ServerImpl::StopAccept()
{
    accepting_.store(false, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        if (lifecycle_state_ == LifecycleState::Stopped || lifecycle_state_ == LifecycleState::Stopping)
        {
            return Result::Ok();
        }
        if (lifecycle_state_ == LifecycleState::Starting)
        {
            return Result::Fail(Status::Busy);
        }
    }
    return ExecuteControl(
        [this]()
        {
            /* 停止监听：close listen fd 即可，已有连接继续。 */
            listener_.Stop();
            return Result::Ok();
        });
}

Result ServerImpl::StopIngest()
{
    ingest_enabled_.store(false, std::memory_order_relaxed);
    return Result::Ok();
}

Result ServerImpl::Shutdown(std::chrono::milliseconds deadline)
{
    if (loop_.InLoopThread())
    {
        IPC_RTSP_LOGE(kTag, "禁止在 I/O 线程内同步 Shutdown");
        return Result::Fail(Status::Busy);
    }

    std::lock_guard<std::mutex> operation_lock(lifecycle_operation_mutex_);
    bool request_stop = false;
    bool already_stopped = false;
    std::uint64_t generation = instance_generation_.load(std::memory_order_acquire);
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        if (lifecycle_state_ == LifecycleState::Stopped)
        {
            already_stopped = true;
        }
        else if (lifecycle_state_ == LifecycleState::Starting)
        {
            return Result::Fail(Status::Busy);
        }
        else if (lifecycle_state_ == LifecycleState::Running)
        {
            lifecycle_state_ = LifecycleState::Stopping;
            request_stop = true;
        }
    }

    if (already_stopped)
    {
        /* Stopped 不保证收尾完整（如 Start 中 loop_.Init 失败的早退路径直接置
         * Stopped）：幂等补一次 Join + Finalize，确保线程已回收、资源干净。 */
        JoinLoopThread();
        FinalizeStoppedState();
        return Result::Ok();
    }

    accepting_.store(false, std::memory_order_relaxed);
    ingest_enabled_.store(false, std::memory_order_relaxed);
    if (request_stop)
    {
        IPC_RTSP_LOGI(kTag, "RTSP生命周期 generation:%llu state:Stopping", static_cast<unsigned long long>(generation));
        /* 关停流程全部在 I/O 线程内做，避免跨线程释放会话状态。 */
        loop_.Post(
            [this, generation]()
            {
                if (instance_generation_.load(std::memory_order_acquire) != generation)
                {
                    return;
                }
                listener_.Stop();

                std::vector<int> fds;
                fds.reserve(connections_.size());
                for (const auto &entry : connections_)
                {
                    fds.push_back(entry.first);
                }
                for (const int fd : fds)
                {
                    const auto it = connections_.find(fd);
                    if (it != connections_.end() && it->second)
                    {
                        it->second->Close(DisconnectReason::ServerShutdown);
                    }
                }
                loop_.Stop();
            });
    }

    std::unique_lock<std::mutex> lock(lifecycle_mutex_);
    const bool stopped = lifecycle_cv_.wait_for(lock,
                                                deadline,
                                                [this, generation]()
                                                {
                                                    return instance_generation_.load(std::memory_order_acquire) != generation ||
                                                           loop_exited_;
                                                });
    if (!stopped)
    {
        IPC_RTSP_LOGW(kTag,
                      "RTSP关停超时 generation:%llu deadline:%lldms",
                      static_cast<unsigned long long>(generation),
                      static_cast<long long>(deadline.count()));
        return Result::Fail(Status::Timeout);
    }
    lock.unlock();

    JoinLoopThread();
    FinalizeStoppedState();
    return Result::Ok();
}

/**
 * I/O 线程首跑就绪回调：Starting → Running 转移并唤醒 Start 的等待。
 *
 * @param generation 发起本次 Start 的代数
 * @return {void}
 * @note generation 与当前代不符（期间已被关停/重启跨越）时静默返回，不做
 *       状态转移、不唤醒，防止旧代事件污染新一代生命周期。
 */
void ServerImpl::OnLoopReady(std::uint64_t generation)
{
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        if (instance_generation_.load(std::memory_order_acquire) != generation || lifecycle_state_ != LifecycleState::Starting)
        {
            return;
        }
        lifecycle_state_ = LifecycleState::Running;
    }
    IPC_RTSP_LOGI(kTag, "RTSP生命周期 generation:%llu state:Running", static_cast<unsigned long long>(generation));
    /* 周期指标打印已按"异常才打日志"原则下线：正常路径不产生周期输出，
     * 异常均有专门 LOGW（失效/回放中止/拥塞/序列空洞/丢帧超限）。指标
     * 数据仍可经 StreamStats 查询与 dumpRuntimeStats 按需导出。
     * ScheduleStatsTimer(); */
    lifecycle_cv_.notify_all();
}

#if 0 /* 周期指标打印整体下线（2026-09-15）：正常路径不输出周期日志，异常均有                                 \
       * 专门 LOGW（GOP 失效/回放中止/拥塞/序列空洞/丢帧超限/认证失败）。                                         \
       * 指标数据仍经 StreamStats 查询与 dumpRuntimeStats 按需导出；需要                                                   \
       * 恢复周期观测时恢复 OnLoopReady 里的 ScheduleStatsTimer() 调用。                                                     \
       */
void ServerImpl::ScheduleStatsTimer()
{
    /* 一次性定时器自重挂（与连接空闲检查同模式）；归属 loop，Stop 后自动失效。 */
    constexpr std::int64_t kStatsIntervalMs = 30000;
    stats_timer_ = loop_.AddTimer(NowMonotonicMs() + kStatsIntervalMs,
                                  [this]()
                                  {
                                      OnStatsTimer();
                                  });
}

void ServerImpl::OnStatsTimer()
{
    constexpr std::uint32_t kIdleStatsPrintEvery = 10;

    std::uint64_t total_subscribers = 0;
    std::vector<StreamStats> snapshots(static_cast<std::size_t>(kStreamCount));
    for (std::size_t i = 0; i < static_cast<std::size_t>(kStreamCount); ++i)
    {
        const FrameHub *stream_hub = hubs_[i].get();
        if (stream_hub == nullptr)
        {
            continue;
        }
        snapshots[i] = stream_hub->Stats();
        total_subscribers += snapshots[i].subscribers;
    }

    if (total_subscribers == 0)
    {
        ++stats_idle_cycles_;
        if (stats_idle_cycles_ % kIdleStatsPrintEvery != 1)
        {
            ScheduleStatsTimer();
            return;
        }
    }
    else
    {
        stats_idle_cycles_ = 0;
    }

    for (std::size_t i = 0; i < static_cast<std::size_t>(kStreamCount); ++i)
    {
        const FrameHub *stream_hub = hubs_[i].get();
        if (stream_hub == nullptr)
        {
            continue;
        }
        const StreamStats stats = snapshots[i];
        if (stats.gop_cache_enabled != 0)
        {
            IPC_RTSP_LOGI(kTag,
                          "RTSP周期指标 chn:%zu 订阅:%llu GOP:开 帧:%llu 字节:%llu 代:%llu 建:%llu 失效:%llu 回放:%llu 中止:%llu 收帧:%llu",
                          i,
                          static_cast<unsigned long long>(stats.subscribers),
                          static_cast<unsigned long long>(stats.gop_cache_frames),
                          static_cast<unsigned long long>(stats.gop_cache_bytes),
                          static_cast<unsigned long long>(stats.gop_epoch),
                          static_cast<unsigned long long>(stats.gop_started),
                          static_cast<unsigned long long>(stats.gop_invalidated),
                          static_cast<unsigned long long>(stats.gop_replays),
                          static_cast<unsigned long long>(stats.gop_replay_aborts),
                          static_cast<unsigned long long>(stats.pushed_frames));
        }
        else
        {
            IPC_RTSP_LOGI(kTag,
                          "RTSP周期指标 chn:%zu 订阅:%llu GOP:关 收帧:%llu",
                          i,
                          static_cast<unsigned long long>(stats.subscribers),
                          static_cast<unsigned long long>(stats.pushed_frames));
        }
    }
    ScheduleStatsTimer();
}
#endif

/**
 * I/O 线程退出回调（loop_.Run 返回后）：标记循环已退出并唤醒等待方。
 *
 * @param generation 本次循环的代数
 * @return {void}
 * @note 置 Stopping 表示"循环已退出、终态收尾未做"（Finalize 后才回
 *       Stopped）；置 loop_exited_ 用于唤醒 Shutdown 的有界等待。跨代时
 *       静默返回：旧代循环退出不代表当前代状态变化。
 */
void ServerImpl::OnLoopStopped(std::uint64_t generation)
{
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        if (instance_generation_.load(std::memory_order_acquire) != generation)
        {
            return;
        }
        lifecycle_state_ = LifecycleState::Stopping;
        loop_exited_ = true;
    }
    accepting_.store(false, std::memory_order_relaxed);
    ingest_enabled_.store(false, std::memory_order_relaxed);
    IPC_RTSP_LOGI(kTag, "RTSP生命周期 generation:%llu state:LoopExited", static_cast<unsigned long long>(generation));
    lifecycle_cv_.notify_all();
}

void ServerImpl::JoinLoopThread()
{
    if (loop_thread_.joinable() && loop_thread_.get_id() != std::this_thread::get_id())
    {
        loop_thread_.join();
    }
}

void ServerImpl::FinalizeStoppedState()
{
    listener_.Stop();
    connections_.clear();
    retired_.clear();
    ip_connections_.clear();
    mailbox_.Clear();
    udp_tracks_.store(0, std::memory_order_relaxed);
    media_handler_registered_ = false;
    accepting_.store(false, std::memory_order_relaxed);
    ingest_enabled_.store(false, std::memory_order_relaxed);

    const std::uint64_t generation = instance_generation_.load(std::memory_order_acquire);
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        lifecycle_state_ = LifecycleState::Stopped;
        loop_exited_ = true;
    }
    IPC_RTSP_LOGI(kTag, "RTSP生命周期 generation:%llu state:Stopped", static_cast<unsigned long long>(generation));
    lifecycle_cv_.notify_all();
}

bool ServerImpl::AdmitConnectionPerIp(const std::string &ip)
{
    for (auto &entry : ip_connections_)
    {
        if (entry.first == ip)
        {
            if (entry.second >= config_.max_connections_per_ip)
            {
                return false;
            }
            ++entry.second;
            return true;
        }
    }
    ip_connections_.emplace_back(ip, 1);
    return true;
}

void ServerImpl::ReleaseConnectionPerIp(const std::string &ip)
{
    for (auto it = ip_connections_.begin(); it != ip_connections_.end(); ++it)
    {
        if (it->first == ip)
        {
            if (--it->second <= 0)
            {
                ip_connections_.erase(it);
            }
            return;
        }
    }
}

int ServerImpl::CountConnectionsPerIp(const std::string &ip) const
{
    for (const auto &entry : ip_connections_)
    {
        if (entry.first == ip)
        {
            return entry.second;
        }
    }
    return 0;
}

void ServerImpl::OnAccept(int fd, const std::string &peer_ip, std::uint16_t peer_port)
{
    if (!accepting_.load(std::memory_order_relaxed))
    {
        ::close(fd);
        return;
    }

    if (connections_.size() >= static_cast<std::size_t>(config_.max_connections))
    {
        counters_.connections_rejected.fetch_add(1, std::memory_order_relaxed);
        IPC_RTSP_LOGW(kTag, "连接数超限拒绝 客户端:%s 当前:%zu 上限:%d", peer_ip.c_str(), connections_.size(), config_.max_connections);
        ::close(fd);
        return;
    }

    const bool per_ip_admitted = config_.max_connections_per_ip <= 0 || AdmitConnectionPerIp(peer_ip);
    if (!per_ip_admitted)
    {
        counters_.connections_rejected.fetch_add(1, std::memory_order_relaxed);
        IPC_RTSP_LOGW(kTag,
                      "单IP连接数超限拒绝 客户端:%s 当前:%d 上限:%d",
                      peer_ip.c_str(),
                      CountConnectionsPerIp(peer_ip),
                      config_.max_connections_per_ip);
        ::close(fd);
        return;
    }

    auto connection = std::make_unique<Connection>(this, &loop_, fd, peer_ip, peer_port);
    const Result started = connection->Start();
    if (!started.ok())
    {
        IPC_RTSP_LOGW(kTag, "连接初始化失败 客户端:%s 状态:%s", peer_ip.c_str(), StatusToString(started.code));
        ReleaseConnectionPerIp(peer_ip);
        /* fd 交由 ~Connection 统一关闭；此处再 close 会与析构双重关闭同一 fd。 */
        return;
    }

    const int key = fd;
    (void) RegisterConnection(std::move(connection));
    counters_.connections_accepted.fetch_add(1, std::memory_order_relaxed);
    IPC_RTSP_LOGI(kTag,
                  "客户端已连接 client:%s:%u fd:%d active:%zu",
                  peer_ip.c_str(),
                  static_cast<unsigned>(peer_port),
                  key,
                  connections_.size());
}

Result ServerImpl::RegisterConnection(std::unique_ptr<Connection> connection)
{
    if (!connection)
    {
        return Result::Fail(Status::InvalidArgument);
    }
    const int fd = connection->fd();
    connections_[fd] = std::move(connection);
    return Result::Ok();
}

void ServerImpl::NoteConnectionClosed(Connection *connection)
{
    if (connection == nullptr)
    {
        return;
    }
    const int fd = connection->fd();
    (void) fd;
    ReleaseConnectionPerIp(connection->peer_ip());

    for (auto it = connections_.begin(); it != connections_.end(); ++it)
    {
        if (it->second.get() == connection)
        {
            /* 移动到 retired_：本轮事件处理结束后统一析构，避免在自身回调中 delete。 */
            retired_.push_back(std::move(it->second));
            connections_.erase(it);
            break;
        }
    }
}

void ServerImpl::ReapRetiredConnections()
{
    if (!retired_.empty())
    {
        retired_.clear();
    }
}

int ServerImpl::PlayingClientCount() const
{
    int count = 0;
    for (const auto &entry : connections_)
    {
        if (entry.second && entry.second->session().any_playing())
        {
            ++count;
        }
    }
    return count;
}

int ServerImpl::PlayingTrackCount(StreamId id) const
{
    int count = 0;
    for (const auto &entry : connections_)
    {
        const Connection *connection = entry.second.get();
        if (connection == nullptr)
        {
            continue;
        }
        for (const TrackRuntime &track : connection->session().tracks())
        {
            if (track.playing && track.stream == id)
            {
                ++count;
            }
        }
    }
    return count;
}

FrameHub *ServerImpl::hub(StreamId id)
{
    const int index = StreamIndex(id);
    if (index < 0)
    {
        return nullptr;
    }
    return hubs_[index].get();
}

const FrameHub *ServerImpl::hub(StreamId id) const
{
    const int index = StreamIndex(id);
    if (index < 0)
    {
        return nullptr;
    }
    return hubs_[index].get();
}

std::string ServerImpl::AdvertisedIp() const
{
    std::lock_guard<std::mutex> lock(config_mutex_);
    if (!advertised_ip_.empty())
    {
        return advertised_ip_;
    }
    return config_.bind_address.empty() ? std::string("0.0.0.0") : config_.bind_address;
}

void ServerImpl::RequestIdr(StreamId id)
{
    const int index = StreamIndex(id);
    if (index < 0)
    {
        return;
    }

    /* MJPEG 每帧独立可解码，无需按需 IDR；海思 VENC 对 MJPEG 通道的
     * IDR 请求也直接返回 NOT_SUPPORT（旧 live555 路径同样跳过）。 */
    if (streams_[index].codec == Codec::MJPEG)
    {
        return;
    }

    /* 全局限频：以 CLOCK_MONOTONIC 毫秒计时；last 为 0 表示该流从未请求过，
     * 直接放行。idr_min_interval_ms 为 0 时窗口恒为 0，等价不限频直通。 */
    const std::int64_t now = NowMonotonicMs();
    if (last_idr_request_ms_[index] != 0 && (now - last_idr_request_ms_[index]) < static_cast<std::int64_t>(config_.idr_min_interval_ms))
    {
        return;
    }
    last_idr_request_ms_[index] = now;
    idr_request_count_[index].fetch_add(1, std::memory_order_relaxed);

    if (idr_callback_)
    {
        /* 回调在 I/O 线程内执行：注册方必须非阻塞（通常是投递到编码线程）。 */
        idr_callback_(id);
    }
}

void ServerImpl::WakeForMedia()
{
    if (loop_.running())
    {
        loop_.Wake();
    }
}

/* ---------------------------------------------------------------- */
/* 生产者线程接口                                                     */
/* ---------------------------------------------------------------- */

Result ServerImpl::PushVideoShared(StreamId id, const SharedVideoFrame &frame, const VideoFrameMeta &meta)
{
    if (!ingest_enabled_.load(std::memory_order_relaxed))
    {
        return Result::Fail(Status::NotInitialized);
    }
    FrameHub *stream_hub = hub(id);
    if (stream_hub == nullptr)
    {
        return Result::Fail(Status::InvalidArgument);
    }
    if (!frame.data || frame.size == 0)
    {
        return Result::Fail(Status::InvalidArgument);
    }
    if (frame.size > stream_hub->max_frame_bytes())
    {
        stream_hub->NoteDropped(meta.key);
        return Result::Fail(Status::NoCapacity);
    }

    stream_hub->NotePushed(frame.size);
    if (!mailbox_.PushVideo(SharedVideoFrame{ frame.data, frame.size }, meta, id))
    {
        /* 邮箱满：丢帧而不是阻塞生产者。 */
        counters_.ingest_dropped.fetch_add(1, std::memory_order_relaxed);
        stream_hub->NoteDropped(meta.key);
        return Result::Fail(Status::NoCapacity);
    }
    WakeForMedia();
    return Result::Ok();
}

Result ServerImpl::PushVideoView(StreamId id, const VideoFrameView &frame)
{
    if (!ingest_enabled_.load(std::memory_order_relaxed))
    {
        return Result::Fail(Status::NotInitialized);
    }
    FrameHub *stream_hub = hub(id);
    if (stream_hub == nullptr)
    {
        return Result::Fail(Status::InvalidArgument);
    }
    if (frame.data == nullptr || frame.size == 0)
    {
        return Result::Fail(Status::InvalidArgument);
    }
    if (frame.size > stream_hub->max_frame_bytes())
    {
        stream_hub->NoteDropped(frame.key);
        return Result::Fail(Status::NoCapacity);
    }

    /* 订阅者已就绪前不复制；但参数集缓存未建立时仍需送帧，否则 DESCRIBE
     * 无法生成带 sprop 的 SDP。 */
    const bool need_copy = stream_hub->SubscriberCount() > 0 || stream_hub->NeedsConfigCapture();
    stream_hub->NotePushed(frame.size);
    if (!need_copy)
    {
        return Result::Ok();
    }

    /* 只在入队前复制一次；多个客户端共享这一份 payload。分配用 nothrow：
     * -fno-exceptions 下普通 new 失败即终止整个进程，这里降级为丢帧。
     * shared_ptr 控制块仍为普通 new（几十字节小分配，失败概率可忽略）。 */
    auto *raw = new (std::nothrow) std::uint8_t[frame.size];
    if (raw == nullptr)
    {
        counters_.ingest_dropped.fetch_add(1, std::memory_order_relaxed);
        stream_hub->NoteDropped(frame.key);
        return Result::Fail(Status::NoCapacity);
    }
    /* 数组 deleter 承担 delete[]；shared_ptr 标量特化在 gcc 6.5 下才完整可用。 */
    std::shared_ptr<std::uint8_t> buffer(raw, std::default_delete<std::uint8_t[]>());
    std::memcpy(buffer.get(), frame.data, frame.size);

    VideoFrameMeta meta;
    meta.codec = frame.codec;
    meta.first_nal = frame.first_nal;
    meta.pts_us = frame.pts_us;
    meta.has_pts = frame.has_pts;
    meta.key = frame.key;

    if (!mailbox_.PushVideo(SharedVideoFrame{ std::move(buffer), frame.size }, meta, id))
    {
        counters_.ingest_dropped.fetch_add(1, std::memory_order_relaxed);
        stream_hub->NoteDropped(frame.key);
        return Result::Fail(Status::NoCapacity);
    }
    WakeForMedia();
    return Result::Ok();
}

Result ServerImpl::PushAudio(StreamId id, const AudioFrameView &frame)
{
    if (!ingest_enabled_.load(std::memory_order_relaxed))
    {
        return Result::Fail(Status::NotInitialized);
    }
    FrameHub *stream_hub = hub(id);
    if (stream_hub == nullptr)
    {
        return Result::Fail(Status::InvalidArgument);
    }
    if (!stream_hub->audio_enabled() || frame.data == nullptr || frame.size == 0)
    {
        /* 板端排障：明确区分"码流未挂音频轨"与数据非法，限频避免刷屏。 */
        static std::atomic<std::int64_t> last_warn_ms{ 0 };
        const std::int64_t now_ms = NowMonotonicMs();
        std::int64_t last = last_warn_ms.load(std::memory_order_relaxed);
        if (now_ms - last > 5000 && last_warn_ms.compare_exchange_strong(last, now_ms))
        {
            IPC_RTSP_LOGW(kTag,
                          "音频帧被拒绝 通道:%d 已启用:%d 大小:%zu 编码:%d",
                          static_cast<int>(id),
                          stream_hub->audio_enabled() ? 1 : 0,
                          frame.size,
                          static_cast<int>(frame.codec));
        }
        return Result::Fail(Status::InvalidArgument);
    }
    if (frame.size > stream_hub->max_audio_frame_bytes())
    {
        counters_.ingest_dropped.fetch_add(1, std::memory_order_relaxed);
        IPC_RTSP_LOGW(kTag,
                      "音频帧超上限 通道:%d 大小:%zu 上限:%zu",
                      static_cast<int>(id),
                      frame.size,
                      stream_hub->max_audio_frame_bytes());
        return Result::Fail(Status::NoCapacity);
    }
    /* 音频回放段依赖 cache 持续积累：无订阅者时仍需经 hub 入 cache，
     * 否则新客户端 PLAY 命中视频回放而音频段缺失（起播音画错位）。 */
    if (stream_hub->SubscriberCount() == 0 && !stream_hub->NeedsAudioIngest())
    {
        return Result::Ok();
    }

    if (!mailbox_.PushAudio(id, frame))
    {
        counters_.ingest_dropped.fetch_add(1, std::memory_order_relaxed);
        return Result::Fail(Status::NoCapacity);
    }
    WakeForMedia();
    return Result::Ok();
}

void ServerImpl::DrainMedia()
{
    VideoMailboxItem video;
    while (mailbox_.PopVideo(&video))
    {
        FrameHub *stream_hub = hub(video.stream);
        if (stream_hub == nullptr)
        {
            continue;
        }
        const FrameInfo info = AnalyzeFrame(stream_hub->config().codec, video.frame.data.get(), video.frame.size, video.meta.key);
        stream_hub->OnVideoFrame(video.frame, video.meta, info);
    }

    AudioMailboxItem audio;
    while (mailbox_.PopAudio(&audio))
    {
        FrameHub *stream_hub = hub(audio.stream);
        if (stream_hub == nullptr)
        {
            continue;
        }
        AudioPendingFrame pending;
        pending.codec = audio.codec;
        pending.size = audio.size;
        pending.pts_us = audio.pts_us;
        pending.has_pts = audio.has_pts;
        std::memcpy(pending.data.data(), audio.data.data(), audio.size);
        stream_hub->OnAudioFrame(pending);
    }

    /* 补发挂起的 DESCRIBE（参数集可能刚到达）。 */
    for (auto &entry : connections_)
    {
        if (entry.second && entry.second->has_pending_describe())
        {
            (void) entry.second->TryServePendingDescribe();
        }
    }

    /* 推送媒体。 */
    for (auto &entry : connections_)
    {
        if (entry.second && !entry.second->closing())
        {
            entry.second->FlushMedia();
        }
    }
}

/* ---------------------------------------------------------------- */
/* 控制面热更新                                                      */
/* ---------------------------------------------------------------- */

Result ServerImpl::ExecuteControl(std::function<Result()> task)
{
    if (!task)
    {
        return Result::Fail(Status::InvalidArgument);
    }
    if (loop_.InLoopThread())
    {
        return task();
    }

    /* 与 Start/Shutdown 串行，保证递交后不会被关停跨越。 */
    std::unique_lock<std::mutex> operation_lock(lifecycle_operation_mutex_);
    std::uint64_t generation = 0;
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);
        if (lifecycle_state_ == LifecycleState::Stopped)
        {
            return task();
        }
        if (lifecycle_state_ == LifecycleState::Starting)
        {
            return Result::Fail(Status::Busy);
        }
        if (lifecycle_state_ == LifecycleState::Stopping)
        {
            return Result::Fail(Status::ShuttingDown);
        }
        generation = instance_generation_.load(std::memory_order_acquire);
    }

    struct Completion
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
        Result result = Result::Fail(Status::Internal);
    };

    auto completion = std::make_shared<Completion>();
    loop_.Post(
        [this, generation, task = std::move(task), completion]() mutable
        {
            bool allowed = false;
            {
                std::lock_guard<std::mutex> lock(lifecycle_mutex_);
                allowed = instance_generation_.load(std::memory_order_acquire) == generation && lifecycle_state_ == LifecycleState::Running;
            }
            /* task 在状态锁外执行：控制任务再慢也不拖住生命周期状态转移。跨代安全由
             * generation 检查加调用方持有的 operation 锁（Shutdown 无法并行进入）保证。 */
            const Result result = allowed ? task() : Result::Fail(Status::ShuttingDown);
            {
                std::lock_guard<std::mutex> lock(completion->mutex);
                completion->result = result;
                completion->done = true;
            }
            completion->cv.notify_all();
        });

    std::unique_lock<std::mutex> lock(completion->mutex);
    if (!completion->cv.wait_for(lock,
                                 kControlTimeout,
                                 [&completion]()
                                 {
                                     return completion->done;
                                 }))
    {
        IPC_RTSP_LOGW(kTag,
                      "RTSP控制面调用超时 generation:%llu timeout:%lldms",
                      static_cast<unsigned long long>(generation),
                      static_cast<long long>(kControlTimeout.count()));
        return Result::Fail(Status::Timeout);
    }
    return completion->result;
}

Result ServerImpl::UpdateCredential(const std::string &user, const std::string &password)
{
    return ExecuteControl(
        [this, user, password]()
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            config_.auth.user = user;
            config_.auth.password = password;
            return Result::Ok();
        });
}

Result ServerImpl::UpdateDigestAlgorithm(DigestAlgorithm algorithm)
{
    if (algorithm != DigestAlgorithm::Md5)
    {
        /* SHA-256 属后续兼容项：这里显式降级并告警，不静默改变行为。 */
        IPC_RTSP_LOGW(kTag, "Digest 算法暂只支持 MD5，配置被降级");
    }
    return ExecuteControl(
        [this]()
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            config_.auth.algorithm = DigestAlgorithm::Md5;
            return Result::Ok();
        });
}

Result ServerImpl::UpdateAdvertisedIp(const std::string &ip)
{
    return ExecuteControl(
        [this, ip]()
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            advertised_ip_ = ip;
            return Result::Ok();
        });
}

Result ServerImpl::UpdateStreamConfig(const std::vector<StreamConfig> &streams)
{
    const Result valid = ValidateStreams(streams);
    if (!valid.ok())
    {
        return valid;
    }
    return ExecuteControl(
        [this, streams]()
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            streams_ = streams;
            for (StreamConfig &stream : streams_)
            {
                NormalizeStreamCodecDefaults(&stream);
            }
            router_.Configure(streams_);
            /* 只影响新连接：已在播放的 track 继续使用旧参数。 */
            for (const StreamConfig &stream : streams_)
            {
                const int index = StreamIndex(stream.id);
                if (index >= 0 && hubs_[index])
                {
                    hubs_[index]->UpdateConfig(stream);
                    /* cache 预算告警与启动路径共用，避免热更新静默改变 cache 行为。 */
                    WarnGopBudgetIfUnderflow(index, stream);
                }
            }
            return Result::Ok();
        });
}

void ServerImpl::SetIdrRequestCallback(std::function<void(StreamId)> callback)
{
    const Result result = ExecuteControl(
        [this, callback = std::move(callback)]() mutable
        {
            idr_callback_ = std::move(callback);
            return Result::Ok();
        });
    if (!result.ok())
    {
        IPC_RTSP_LOGW(kTag, "IDR 回调更新失败 status:%s", StatusToString(result.code));
    }
}

std::string ServerImpl::BuildUrl(StreamId id, bool with_credentials) const
{
    const FrameHub *stream_hub = hub(id);
    if (stream_hub == nullptr)
    {
        return std::string();
    }

    std::string user;
    std::string password;
    std::string ip;
    std::uint16_t port = 0;
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        user = config_.auth.user;
        password = config_.auth.password;
        ip = advertised_ip_.empty() ? (config_.bind_address.empty() ? std::string("0.0.0.0") : config_.bind_address) : advertised_ip_;
        port = config_.port;
    }

    std::string url = "rtsp://";
    if (with_credentials)
    {
        url += user;
        url += ":";
        url += password;
        url += "@";
    }
    url += ip;
    url += ":";
    url += std::to_string(port);
    url += "/";
    url += stream_hub->config().path;
    return url;
}

std::string ServerImpl::Url(StreamId id, bool with_credentials) const
{
    auto output = std::make_shared<std::string>();
    /* const_cast 仅为复用 ExecuteControl：其任务只读配置、不改状态，不破坏
     * 本函数的 const 语义。 */
    const Result result = const_cast<ServerImpl *>(this)->ExecuteControl(
        [this, id, with_credentials, output]()
        {
            *output = BuildUrl(id, with_credentials);
            return Result::Ok();
        });
    if (!result.ok())
    {
        IPC_RTSP_LOGW(kTag, "RTSP URL 读取失败 status:%s", StatusToString(result.code));
        return std::string();
    }
    return *output;
}

MetricsSnapshot ServerImpl::BuildCountersOnlySnapshot(bool from_atomics) const
{
    MetricsSnapshot snapshot;
    snapshot.connections_accepted = counters_.connections_accepted.load(std::memory_order_relaxed);
    snapshot.connections_rejected = counters_.connections_rejected.load(std::memory_order_relaxed);
    snapshot.tx_bytes = counters_.tx_bytes.load(std::memory_order_relaxed);
    snapshot.rtp_packets = counters_.rtp_packets.load(std::memory_order_relaxed);
    snapshot.rtcp_packets = counters_.rtcp_packets.load(std::memory_order_relaxed);
    snapshot.auth_failures = counters_.auth_failures.load(std::memory_order_relaxed);
    snapshot.parse_failures = counters_.parse_failures.load(std::memory_order_relaxed);
    snapshot.congestion_disconnects = counters_.congestion_disconnects.load(std::memory_order_relaxed);

    for (int i = 0; i < kStreamCount; ++i)
    {
        if (hubs_[i])
        {
            /* 超时降级口径只读原子计数：GOP cache 状态由 I/O 线程私有，
             * 卡死场景下跨线程读非原子 64 位可撕裂。Stopped 状态（未 Start）
             * 下 ExecuteControl 直接执行任务，无并发，走全量口径。 */
            snapshot.streams[i] = from_atomics ? hubs_[i]->StatsFromAtomics() : hubs_[i]->Stats();
        }
    }
    return snapshot;
}

MetricsSnapshot ServerImpl::BuildSnapshot() const
{
    MetricsSnapshot snapshot = BuildCountersOnlySnapshot(/*from_atomics=*/false);
    snapshot.connections_active = connections_.size();

    std::uint64_t sessions = 0;
    std::uint64_t playing = 0;
    std::uint64_t tcp_clients = 0;
    std::uint64_t udp_clients = 0;
    for (const auto &entry : connections_)
    {
        const Connection *connection = entry.second.get();
        if (connection == nullptr)
        {
            continue;
        }
        const Session &session = connection->session();
        if (session.valid())
        {
            ++sessions;
        }
        if (session.any_playing())
        {
            ++playing;
        }
        for (const TrackRuntime &track : session.tracks())
        {
            if (!track.playing)
            {
                continue;
            }
            if (track.kind == TransportKind::Udp)
            {
                ++udp_clients;
            }
            else if (track.kind == TransportKind::TcpInterleaved)
            {
                ++tcp_clients;
            }
        }
    }
    snapshot.sessions_active = sessions;
    snapshot.playing_clients = playing;
    snapshot.tcp_clients = tcp_clients;
    snapshot.udp_clients = udp_clients;

    return snapshot;
}

MetricsSnapshot ServerImpl::Snapshot() const
{
    auto output = std::make_shared<MetricsSnapshot>();
    /* const_cast 仅为复用 ExecuteControl：BuildSnapshot 只读统计，不破坏本
     * 函数的 const 语义。 */
    const Result result = const_cast<ServerImpl *>(this)->ExecuteControl(
        [this, output]()
        {
            *output = BuildSnapshot();
            return Result::Ok();
        });
    if (!result.ok())
    {
        IPC_RTSP_LOGW(kTag, "RTSP 指标快照降级 status:%s", StatusToString(result.code));
        return BuildCountersOnlySnapshot(/*from_atomics=*/true);
    }
    return *output;
}

/* UDP track 配额只在 I/O 线程的会话建立/拆除路径增减（约束见声明处）。 */
bool ServerImpl::TryAcquireUdpTrack()
{
    const std::size_t current = udp_tracks_.load(std::memory_order_relaxed);
    if (current >= config_.backpressure.max_udp_tracks)
    {
        return false;
    }
    udp_tracks_.store(current + 1, std::memory_order_relaxed);
    return true;
}

void ServerImpl::ReleaseUdpTrack()
{
    const std::size_t current = udp_tracks_.load(std::memory_order_relaxed);
    if (current > 0)
    {
        udp_tracks_.store(current - 1, std::memory_order_relaxed);
    }
}

} // namespace detail

/* -------------------------------------------------------------------- */
/* 对外门面                                                              */
/* -------------------------------------------------------------------- */

Server::Server() = default;

Server::~Server()
{
    if (impl_ != nullptr)
    {
        /* 200ms：析构兜底关停等待 I/O 线程退出的经验上限；即便超时，下面的
         * delete 仍会触发 ServerImpl 析构的强制收尾路径。 */
        impl_->Shutdown(std::chrono::milliseconds(200));
        delete impl_;
        impl_ = nullptr;
    }
}

Result Server::Create(const ServerConfig &config, const std::vector<StreamConfig> &streams, std::unique_ptr<Server> *out)
{
    if (out == nullptr)
    {
        return Result::Fail(Status::InvalidArgument);
    }

    std::unique_ptr<Server> server(new Server());
    server->impl_ = new detail::ServerImpl();

    const Result configured = server->impl_->Configure(config, streams);
    if (!configured.ok())
    {
        delete server->impl_;
        server->impl_ = nullptr;
        return configured;
    }

    *out = std::move(server);
    return Result::Ok();
}

Result Server::Start()
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->Start();
}

Result Server::StopIngest()
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->StopIngest();
}

Result Server::StopAccept()
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->StopAccept();
}

Result Server::Shutdown(std::chrono::milliseconds deadline)
{
    if (impl_ == nullptr)
    {
        return Result::Ok();
    }
    return impl_->Shutdown(deadline);
}

Result Server::PushVideo(StreamId id, const VideoFrameView &frame)
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->PushVideoView(id, frame);
}

Result Server::PushVideo(StreamId id, const SharedVideoFrame &frame, const VideoFrameMeta &meta)
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->PushVideoShared(id, frame, meta);
}

Result Server::PushAudio(StreamId id, const AudioFrameView &frame)
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->PushAudio(id, frame);
}

Result Server::UpdateCredential(const std::string &user, const std::string &password)
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->UpdateCredential(user, password);
}

Result Server::UpdateDigestAlgorithm(DigestAlgorithm algorithm)
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->UpdateDigestAlgorithm(algorithm);
}

Result Server::UpdateAdvertisedIp(const std::string &ip)
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->UpdateAdvertisedIp(ip);
}

Result Server::UpdateStreamConfig(const std::vector<StreamConfig> &streams)
{
    if (impl_ == nullptr)
    {
        return Result::Fail(Status::NotInitialized);
    }
    return impl_->UpdateStreamConfig(streams);
}

void Server::SetIdrRequestCallback(std::function<void(StreamId)> callback)
{
    if (impl_ != nullptr)
    {
        impl_->SetIdrRequestCallback(std::move(callback));
    }
}

std::string Server::Url(StreamId id, bool with_credentials) const
{
    if (impl_ == nullptr)
    {
        return std::string();
    }
    return impl_->Url(id, with_credentials);
}

MetricsSnapshot Server::Snapshot() const
{
    if (impl_ == nullptr)
    {
        return MetricsSnapshot{};
    }
    return impl_->Snapshot();
}

} // namespace ipc_rtsp
