/**
 * @FilePath     : server.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTSP Server 公共接口
 */
/*
 * 分层：本库负责连接、会话、鉴权、RTP/RTCP、媒体分发与背压；调用方只负责
 * 提供媒体帧与配置。库内不读取产品配置文件、不依赖 ipc_share 头文件。
 *
 * 线程模型：
 * - 库内部有且只有一个 I/O 线程（epoll），拥有全部会话/连接/定时器状态；
 * - PushVideo/PushAudio 可从任意线程调用（VENC/AENC 线程），只做有界入队；
 * - SetIdrRequestCallback 注册的回调在 I/O 线程内被调用，必须非阻塞，禁止
 *   在回调里调用本库的 Shutdown。
 */
#pragma once

#include "ipc_rtsp/config.h"
#include "ipc_rtsp/export.h"
#include "ipc_rtsp/frame.h"
#include "ipc_rtsp/result.h"
#include "ipc_rtsp/status.h"
#include "ipc_rtsp/types.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ipc_rtsp
{

namespace detail
{
/** 前向声明：实现见 src/server_impl.h。 */
class ServerImpl;
} // namespace detail

/**
 * RTSP 服务实例。
 *
 * 创建后必须 Start() 才会监听端口；Stop* 与 Shutdown 幂等。
 */
class IPC_RTSP_API Server
{
public:
    /**
     * 析构；未显式 Shutdown 时兜底执行一次关停。
     *
     * @return {void} 无返回值
     * @note 内部实现非空时自动执行 Shutdown（最长等待 200ms）；即使关停超时也会
     *       强制回收实现对象并保证 I/O 线程退出。已显式 Shutdown 后析构为幂等收尾。
     */
    ~Server();

    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

    /**
     * 创建服务实例（不监听端口）。
     *
     * @param config  服务配置
     * @param streams 码流配置（必须覆盖 Main/Sub 两路，codec 不能是 Unknown）
     * @param out     输出实例指针；成功时指向新实例，失败时不修改 out 指向的对象
     * @return Ok 成功；InvalidArgument out 为空或码流/背压/GOP cache 配置非法；
     *         Unsupported codec 不支持
     * @note 配置校验在创建时一次完成；创建后需调用 Start() 才会监听端口。
     */
    static Result Create(const ServerConfig &config, const std::vector<StreamConfig> &streams, std::unique_ptr<Server> *out);

    /**
     * 启动监听并开始 I/O 线程。
     *
     * @return Ok 成功；AlreadyInitialized 已在运行或正在启动；Busy 正在关停；
     *         IoError 监听 socket 失败；Timeout 启动 ready 等待超时
     * @note 完整停止（Shutdown 返回 Ok）后可再次 Start。
     */
    Result Start();

    /**
     * 停止接收新的媒体帧（已有客户端继续）。
     *
     * @return Ok 成功；NotInitialized 实例未创建
     * @note 立即生效（原子标志），之后 PushVideo/PushAudio 返回 NotInitialized；幂等。
     */
    Result StopIngest();

    /**
     * 停止接受新连接（已建立连接继续）。
     *
     * @return Ok 成功（含已停止时重复调用）；Busy 正在启动；ShuttingDown 正在关停；
     *         Timeout 控制面等待超时
     * @note 通过 I/O 线程关闭监听 socket；幂等。
     */
    Result StopAccept();

    /**
     * 完整关停：停止接受连接、停止 ingest、取消定时器、断开客户端、等待 I/O 线程退出。
     *
     * @param deadline 最长等待时间；超时返回 Status::Timeout，但内部仍继续异步收尾
     * @return Ok 已完全停止；Timeout 等待超时；Busy 正在启动或在 I/O 线程内调用
     * @note 幂等（已停止时直接返回 Ok）。推荐顺序 StopIngest → StopAccept →
     *       Shutdown，也可从任意状态直接调用 Shutdown；禁止在 I/O 线程（如 IDR
     *       回调）内调用。
     */
    Result Shutdown(std::chrono::milliseconds deadline);

    /**
     * 推入一帧视频（调用方持有数据，库在有订阅者时复制一次）。
     *
     * @param id    码流通道
     * @param frame 只读帧视图；data 仅在本次调用期间有效
     * @return Ok 已入队（或无需复制直接丢弃）；NotInitialized 未创建或已 StopIngest；
     *         InvalidArgument 通道非法或数据为空；NoCapacity 超过 max_frame_bytes
     *         或入队邮箱满
     * @note 无订阅者且参数集缓存已建立时不复制、直接丢弃；可从任意线程调用。
     */
    Result PushVideo(StreamId id, const VideoFrameView &frame);

    /**
     * 推入一帧视频（引用计数共享，库内零拷贝）。
     *
     * @param id   码流通道
     * @param frame 共享帧；data 为共享拥有指针，生命周期由引用计数保证
     * @param meta  帧元数据
     * @return 同 PushVideo(StreamId, const VideoFrameView &)
     */
    Result PushVideo(StreamId id, const SharedVideoFrame &frame, const VideoFrameMeta &meta);

    /**
     * 推入一帧音频；输入数据仅在调用期间有效。
     *
     * @param id    码流通道
     * @param frame 只读音频帧视图
     * @return Ok 已入队（或无需入队直接丢弃）；NotInitialized 未创建或已 StopIngest；
     *         InvalidArgument 通道非法、音频轨未启用或数据为空；NoCapacity 超过
     *         max_audio_frame_bytes 或入队邮箱满
     */
    Result PushAudio(StreamId id, const AudioFrameView &frame);

    /**
     * 更新用户名/密码（热更新，新请求立即生效）。
     *
     * @param user     新用户名
     * @param password 新密码
     * @return Ok 成功；NotInitialized 实例未创建；Busy/ShuttingDown/Timeout 控制面忙
     */
    Result UpdateCredential(const std::string &user, const std::string &password);

    /**
     * 更新鉴权算法。
     *
     * @param algorithm 目标算法；非 MD5 时降级为 MD5 并打告警日志
     * @return Ok 成功；NotInitialized 实例未创建；Busy/ShuttingDown/Timeout 控制面忙
     */
    Result UpdateDigestAlgorithm(DigestAlgorithm algorithm);

    /**
     * 更新对外展示地址（影响 SDP c= 与 Url()）。
     *
     * @param ip 对外展示地址；空串表示回退使用连接的本地地址
     * @return Ok 成功；NotInitialized 实例未创建；Busy/ShuttingDown/Timeout 控制面忙
     */
    Result UpdateAdvertisedIp(const std::string &ip);

    /**
     * 更新码流配置（只收紧新连接，不主动断开已有客户端）。
     *
     * @param streams 新码流配置，校验规则同 Create
     * @return Ok 成功；InvalidArgument/Unsupported 配置非法；NotInitialized 实例未创建；
     *         Busy/ShuttingDown/Timeout 控制面忙
     */
    Result UpdateStreamConfig(const std::vector<StreamConfig> &streams);

    /**
     * 注册 IDR 请求回调；在 I/O 线程内调用，必须非阻塞。
     *
     * @param callback 回调函数；传空表示取消注册
     * @return {void} 无返回值
     * @note 库持有回调的拷贝，随实例析构释放；库内已做全局限频
     *       （ServerConfig::idr_min_interval_ms）；Shutdown 返回后保证不再调用；
     *       回调内禁止调用本库 Shutdown。
     */
    void SetIdrRequestCallback(std::function<void(StreamId)> callback);

    /**
     * 生成对外展示的 RTSP URL（值返回，避免裸指针）。
     *
     * @param id              码流通道
     * @param with_credentials 是否在 URL 中拼入 user:password
     * @return "rtsp://[user:pass@]ip:port/path"；通道非法或控制面超时返回空串
     */
    std::string Url(StreamId id, bool with_credentials) const;

    /**
     * 读取运行指标快照。
     *
     * @return 快照；控制面超时/关停期间降级为仅原子计数（连接/会话等瞬时字段为 0）
     */
    MetricsSnapshot Snapshot() const;

private:
    /**
     * 默认构造（私有，统一经 Create 创建）。
     *
     * @return {void} 无返回值
     */
    Server();

    /* memory: Create 成功时拥有，析构时释放（析构内先兜底执行 Shutdown）。 */
    detail::ServerImpl *impl_ = nullptr;
};

} // namespace ipc_rtsp
