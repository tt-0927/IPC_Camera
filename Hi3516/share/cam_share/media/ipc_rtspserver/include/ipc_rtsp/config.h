/**
 * @FilePath     : config.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 服务与流配置
 */
/*
 * 所有上限都必须在配置里显式给出（可以是默认值），不允许“无上限”。
 * 数值语义与推导见 docs/smolrtsp_rtspserver_docs/21_资源预算与背压设计.md。
 */
#pragma once

#include "ipc_rtsp/types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ipc_rtsp
{

/** 鉴权模式。 */
enum class AuthMode
{
    None = 0, /**< 允许匿名访问 */
    Digest,   /**< RTSP Digest 认证（MD5） */
};

/** Digest 摘要算法。 */
enum class DigestAlgorithm
{
    Md5 = 0, /* RTSP Digest 认证当前唯一实现的算法 */
    Sha256,  /* 预留：v1 只实现 MD5，配置为 SHA256 时降级并告警 */
};

/** 鉴权配置。 */
struct AuthConfig
{
    /* 鉴权模式。 */
    AuthMode mode = AuthMode::None;
    /* Digest 摘要算法（SHA256 当前降级为 MD5 并告警）。 */
    DigestAlgorithm algorithm = DigestAlgorithm::Md5;
    /* Digest realm，出现在 WWW-Authenticate 响应头中。 */
    std::string realm = "Itc Streaming Server";
    /* Digest 用户名。 */
    std::string user = "admin";
    /* Digest 密码（用于计算 HA1）。 */
    std::string password;
    /** nonce 有效期（秒）。 */
    std::uint32_t nonce_ttl_s = 300;
    /** 单连接允许的认证失败次数，超过即断开。 */
    std::uint32_t max_failures_per_connection = 8;
    /** 是否允许 OPTIONS 匿名（多数客户端与 ONVIF 探测依赖）。 */
    bool allow_anonymous_options = true;
};

/**
 * 背压与资源上限。
 *
 * 每一项都是硬上限：达到上限必须丢帧或断连，不允许继续增长。
 */
struct BackpressureConfig
{
    /* ---- 每码流媒体队列 ---- */
    std::size_t hub_max_video_frames = 6;                 /**< 视频队列最大帧数 */
    std::size_t hub_max_video_bytes = 1u * 1024u * 1024u; /**< 视频队列最大字节数 */
    std::size_t hub_max_audio_frames = 6;                 /**< 音频队列最大帧数 */
    std::size_t hub_max_audio_bytes = 64u * 1024u;        /**< 音频队列最大字节数 */

    /* ---- 每客户端待发队列 ---- */
    std::size_t client_max_pending_frames = 8;                 /**< 待发帧（引用）最大条目数 */
    std::size_t client_max_pending_bytes = 1u * 1024u * 1024u; /**< 待发帧最大字节数 */
    /*
     * TCP 输出按"积压时长"判定（与 ZLMediaKit/gst-rtsp-server 同口径）：积压的媒体
     * 数据落后直播超过 soft 时长进入拥塞态（丢 P 帧、只发关键帧），超过 hard 时长
     * 断开。时长判定与分辨率、码率、GOP、单帧大小无关，换镜头/改码率无需重调。
     * 字节上限仅作为内存护栏，防异常码率配置撑爆 RAM，不参与常规拥塞判定。
     */
    std::uint32_t client_soft_backlog_ms = 1500;                /**< 积压超过该时长进入拥塞态（毫秒） */
    std::uint32_t client_hard_backlog_ms = 3000;                /**< 积压超过该时长断开连接（毫秒） */
    std::size_t client_output_guard_bytes = 4u * 1024u * 1024u; /**< TCP 输出内存护栏：媒体区绝对容量 */
    std::size_t client_control_reserve_bytes = 16u * 1024u;     /**< 仅允许 RTSP 控制响应使用的预留空间 */
    std::uint32_t congested_timeout_ms = 2000;                  /**< 拥塞后连续无发送进展的超时（毫秒） */

    /* ---- 连接输入 ---- */
    std::size_t max_request_bytes = 8u * 1024u;       /**< 单请求最大字节数 */
    std::size_t max_input_buffer_bytes = 16u * 1024u; /**< 连接输入缓冲最大字节数 */
    std::size_t max_body_bytes = 4u * 1024u;          /**< 请求 body 上限 */

    /* ---- 全局 ---- */
    std::size_t max_udp_tracks = 8;           /**< 全局 UDP track 上限（每 track 2 个 fd） */
    std::size_t max_idr_pending_requests = 4; /**< 同时等待关键帧的订阅者上限（仅用于限频判断） */
};

/**
 * GOP cache 配置（25 号文档 25.4.2）。
 *
 * 三项全 0 表示关闭 cache（起播回退 WaitStart + IDR，与关闭前行为一致）；
 * 部分为 0 属非法配置，Server::Create 与 Server::UpdateStreamConfig 校验失败时
 * 返回 InvalidArgument。
 * 实现硬上限：max_duration_ms ≤ 5000，max_frames ≤ 300。
 */
struct GopCacheConfig
{
    /** 当前 GOP 的 payload 上限；超限整段失效，绝不做中间裁剪。 */
    std::size_t max_bytes = 0;
    /** 当前 GOP 的最大 AU 数；创建时按该值预留容量，GOP 内不扩容。 */
    std::size_t max_frames = 0;
    /** 当前 GOP 的最大时长（毫秒，无真实 PTS 时按单调钟估算）。 */
    std::uint32_t max_duration_ms = 0;
};

/** 单路码流配置。 */
struct StreamConfig
{
    /* 码流通道。 */
    StreamId id = StreamId::Main;
    /** RTSP 路径，不含前导 '/'，例如 "Streaming/Channels/101"。 */
    std::string path = "Streaming/Channels/101";
    /* 视频编码格式。 */
    Codec codec = Codec::H264;
    /** 视频 RTP 动态负载类型。 */
    std::uint8_t payload_type = 96;
    /* 视频 RTP 时钟频率（Hz）。 */
    std::uint32_t clock_rate = 90000;
    /** 单帧（完整 VENC pack）字节上限，超过即拒绝入队。 */
    std::size_t max_frame_bytes = 1u * 1024u * 1024u;
    /**
     * 本码流允许同时播放的客户端数；0 表示不额外限制（只受服务级上限约束）。
     *
     * 产品语义：主码流按码率分档收紧（如 8Mbps→2 路、16Mbps→1 路），子码流不额外限制。
     */
    int max_playing_clients = 0;
    /** 标称帧率，仅用于默认队列深度与 SDP framerate。 */
    int fps = 25;
    /** NAL 分片上限（RTP payload 上限）。 */
    std::size_t max_nalu_bytes = 1200;

    /* ---- 可选音频轨 ---- */
    /* 是否启用音频轨。 */
    bool audio_enabled = false;
    /* 音频编码格式。 */
    AudioCodec audio_codec = AudioCodec::None;
    /* 音频 RTP 负载类型；默认 8 对应 PCMA（G.711 A-law），须与 audio_codec 匹配。 */
    std::uint8_t audio_payload_type = 8;
    /* 音频 RTP 时钟频率（Hz）；G.711/G.726 由库强制 8000，AAC 须取真实采样率。 */
    std::uint32_t audio_clock_rate = 8000;
    /* 单帧音频字节上限，超过即拒绝入队。 */
    std::size_t max_audio_frame_bytes = 4096u;
    /* AAC 是否已剥离 ADTS 头（true 表示输入是裸 AAC AU）。 */
    bool audio_stripped_adts = true;
    /* 音频通道数（用于 SDP）。 */
    int audio_channels = 1;

    /* ---- GOP cache（仅视频轨；全 0 表示关闭） ---- */
    GopCacheConfig gop_cache;
};

/** 服务级配置。 */
struct ServerConfig
{
    std::uint16_t port = 554;
    std::string bind_address = "0.0.0.0";
    /** 对外展示/SDP 使用的地址；空表示使用连接的本地地址。 */
    std::string advertised_ip;

    /** 允许同时 PLAY 的客户端数（含 UDP/TCP）。 */
    int max_playing_clients = 4;
    /** 允许同时存在的 RTSP 连接数（含未认证/未 PLAY）。 */
    int max_connections = 16;
    /** 单一源 IP 允许的并发连接数；0 表示不限制（默认，保持既有行为）。
     * 用于防单点占满全部连接配额（内网 DoS）。 */
    int max_connections_per_ip = 0;

    /** 单个请求的等待超时：连接在此期间没有完整请求即断开。 */
    std::uint32_t request_timeout_ms = 30000;
    /** 会话超时：PLAY 后没有保活请求即回收。 */
    std::uint32_t session_timeout_ms = 65000;
    /** RTCP SR 发送间隔。 */
    std::uint32_t rtcp_interval_ms = 5000;
    /** 同一码流两次外部 IDR 请求的最小间隔。 */
    std::uint32_t idr_min_interval_ms = 300;

    /** 媒体 DSCP（<0 表示不设置）。 */
    int dscp = -1;
    /** 服务名，用于 SDP 的 s= 与 CNAME。 */
    std::string server_name = "IPC RTSP Server";

    AuthConfig auth;
    BackpressureConfig backpressure;
};

} // namespace ipc_rtsp
