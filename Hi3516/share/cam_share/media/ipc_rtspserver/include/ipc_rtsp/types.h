/**
 * @FilePath     : types.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 基础类型：通道、Codec、NAL 类型、传输方式
 */
#pragma once

#include <cstdint>
#include <cstddef>

namespace ipc_rtsp
{

/** 码流通道：与产品 RTSP_CHN_E 一一对应（主/子码流）。 */
enum class StreamId : int
{
    Main = 0, /* 主码流 */
    Sub = 1,  /* 子码流 */
};

/** 支持的码流路数。 */
constexpr int kStreamCount = 2;

/**
 * StreamId 转数组下标。
 *
 * @param id 码流通道
 * @return 0/1 合法下标；越界返回 -1
 */
inline int StreamIndex(StreamId id)
{
    const int index = static_cast<int>(id);
    return (index >= 0 && index < kStreamCount) ? index : -1;
}

/** 视频编码格式。 */
enum class Codec
{
    Unknown = 0, /* 未知/未指定（非法配置） */
    H264,        /* H.264/AVC */
    H265,        /* H.265/HEVC */
    MJPEG,       /* Motion JPEG（RFC 2435） */
};

/** 音频编码格式。 */
enum class AudioCodec
{
    None = 0, /* 无音频轨 */
    AAC,      /* AAC（MPEG-4 音频，时钟须取真实采样率） */
    G711A,    /* G.711 A-law（PCMA，静态 PT 8，时钟 8000 Hz） */
    G711U,    /* G.711 μ-law（PCMU，静态 PT 0，时钟 8000 Hz） */
    G726_32,  /* G.726，32=标称码率 kbps（时钟 8000 Hz） */
};

/**
 * NAL 单元类型（与产品 Video_NS::NalType_E 语义对齐的简化集合）。
 *
 * 只需要区分“参数集 / 关键帧 / 普通图像 / 其他”，用于首帧策略与 SDP 参数集缓存。
 */
enum class NalUnitType
{
    Unknown = 0, /* 未知/无法识别 */
    NonIdrSlice, /**< 普通 P/B 图像 */
    IdrSlice,    /**< IDR 关键帧 */
    CraSlice,    /**< H.265 CRA（可作随机接入点，但存在 RASL 前导） */
    Vps,         /**< H.265 VPS */
    Sps,         /**< 序列参数集 */
    Pps,         /**< 图像参数集 */
    Sei,         /**< 补充增强信息 */
    Aud,         /**< 访问单元分隔符 */
    Other,       /**< 其他非 VCL NAL */
};

/** RTP 下层传输方式。 */
enum class TransportKind
{
    Unknown = 0,    /* 尚未协商 */
    Udp,            /* RTP/AVP over UDP */
    TcpInterleaved, /* RTP/AVP/TCP interleaved */
};

/** 客户端断开原因，用于可观测性。 */
enum class DisconnectReason
{
    Unknown = 0,      /* 尚未断开或原因未知 */
    ClientTeardown,   /**< 客户端显式 TEARDOWN */
    ConnectionClosed, /**< 对端关闭连接 */
    IdleTimeout,      /**< 长时间没有合法请求 */
    SessionTimeout,   /**< 会话超时（未保活） */
    OutputCongested,  /**< 输出积压超硬上限或拥塞超时 */
    ParseError,       /**< 请求非法 */
    InputOverflow,    /**< 输入缓冲超上限 */
    AuthFailures,     /**< 认证失败次数超限 */
    ServerShutdown,   /**< 服务关停 */
    IoError,          /**< socket 错误 */
    Internal,         /**< 内部错误（不变量被破坏） */
};

/**
 * 断开原因的中文短描述。
 *
 * @param reason 断开原因
 * @return 静态字符串，生命周期不限；未知值返回 "未知"
 */
inline const char *DisconnectReasonToString(DisconnectReason reason)
{
    switch (reason)
    {
    case DisconnectReason::Unknown:
        return "未知";
    case DisconnectReason::ClientTeardown:
        return "客户端TEARDOWN";
    case DisconnectReason::ConnectionClosed:
        return "对端关闭";
    case DisconnectReason::IdleTimeout:
        return "空闲超时";
    case DisconnectReason::SessionTimeout:
        return "会话超时";
    case DisconnectReason::OutputCongested:
        return "输出拥塞";
    case DisconnectReason::ParseError:
        return "请求非法";
    case DisconnectReason::InputOverflow:
        return "输入超限";
    case DisconnectReason::AuthFailures:
        return "认证失败超限";
    case DisconnectReason::ServerShutdown:
        return "服务关停";
    case DisconnectReason::IoError:
        return "网络错误";
    case DisconnectReason::Internal:
        return "内部错误";
    }
    return "未知";
}

/**
 * codec 名称（用于 SDP/日志）。
 *
 * @param codec 视频编码格式
 * @return 静态字符串（"H264"/"H265"/"JPEG"）；Unknown 返回 "unknown"
 */
inline const char *CodecToString(Codec codec)
{
    switch (codec)
    {
    case Codec::H264:
        return "H264";
    case Codec::H265:
        return "H265";
    case Codec::MJPEG:
        return "JPEG";
    default:
        return "unknown";
    }
}

} // namespace ipc_rtsp
