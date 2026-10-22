/**
 * @FilePath     : rtp_sender.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 每客户端 RTP/NAL 发送器（smolrtsp 句柄的 RAII 包装）
 */

/*
 * 归属：一个 Track 一个 RtpSender，生命周期与 Track 一致。
 * 线程约定：只在 I/O 线程使用。
 */
#pragma once

#include "ipc_rtsp/result.h"
#include "ipc_rtsp/types.h"

#include <cstddef>
#include <cstdint>

/* 隔离层的 C 类型（不在业务层引入 smolrtsp 头文件）。 */
struct ipc_rtsp_shim_rtp;
struct ipc_rtsp_shim_nal;
struct ipc_rtsp_shim_jpeg;
struct ipc_rtsp_shim_writer;

namespace ipc_rtsp
{
namespace detail
{

struct JpegFrameInfo;

/** RTP 发送器。 */
class RtpSender
{
public:
    /**
     * @brief 析构并释放 shim 侧资源。
     *
     * @note 释放顺序 jpeg_ → nal_ → rtp_：任一上层包装非空时其析构会级联
     *       释放下层 rtp 与 transport，因此只释放存在的最上层并清空 rtp_，
     *       避免二次释放。
     *
     * @return {void}
     */
    ~RtpSender();

    RtpSender(const RtpSender &) = delete;
    RtpSender &operator=(const RtpSender &) = delete;

    /**
     * @brief 基于 TCP interleaved 通道创建。
     *
     * @param writer 连接级 writer 句柄，必须比本对象活得久
     * @param channel interleaved RTP 通道号
     * @param max_buffer 输出水位上限（超过后 IsFull() 为真，用于背压判断）
     * @param ssrc RTP SSRC
     * @param payload_type RTP 负载类型
     * @param clock_rate RTP 时钟频率（Hz；视频为 90000）
     * @param ts_base_us 时间戳基准（单调时钟，微秒），该时刻对应 RTP 时间戳 0
     * @param codec 编码格式，决定是否创建 NAL/JPEG 上层
     * @param max_nalu_bytes 单 NAL 分片负载上限（字节）；传 0 时 JPEG 层回退
     *        1200（UDP MTU 安全上限，与 CreateUdp 同口径）
     * @return 成功返回新实例（调用方负责 delete）；失败返回 nullptr
     */
    static RtpSender *CreateTcp(ipc_rtsp_shim_writer *writer,
                                std::uint8_t channel,
                                std::size_t max_buffer,
                                std::uint32_t ssrc,
                                std::uint8_t payload_type,
                                std::uint32_t clock_rate,
                                std::uint64_t ts_base_us,
                                Codec codec,
                                std::size_t max_nalu_bytes);

    /**
     * @brief 基于已 connect 的 UDP socket 创建；**接管 fd**（析构时由 shim 关闭）。
     *
     * @param fd 已 connect 的 RTP socket fd，接管后归 shim 所有，调用方不得再 close
     * @param ssrc RTP SSRC
     * @param payload_type RTP 负载类型
     * @param clock_rate RTP 时钟频率（Hz；视频为 90000）
     * @param ts_base_us 时间戳基准（单调时钟，微秒），该时刻对应 RTP 时间戳 0
     * @param codec 编码格式，决定是否创建 NAL/JPEG 上层
     * @param max_nalu_bytes 单 NAL 分片负载上限（字节）；传 0 时 JPEG 层回退
     *        1200（UDP MTU 安全上限，与 CreateTcp 同口径）
     * @return 成功返回新实例（调用方负责 delete）；失败返回 nullptr
     */
    static RtpSender *CreateUdp(int fd,
                                std::uint32_t ssrc,
                                std::uint8_t payload_type,
                                std::uint32_t clock_rate,
                                std::uint64_t ts_base_us,
                                Codec codec,
                                std::size_t max_nalu_bytes);

    /**
     * @brief 发送一个完整 NAL（含 NAL 头）；内部自动 FU 分片。
     *
     * @param nal NAL 数据（含 NAL 头；起始码由上游剥离）
     * @param size NAL 字节数
     * @param raw_ts RTP 时间戳（时钟频率为构造时 clock_rate；视频 90kHz）
     * @param au_end 是否为访问单元最后一个 NAL（映射为 RTP marker 位）
     * @return 发送结果
     */
    Result SendNal(const std::uint8_t *nal, std::size_t size, std::uint32_t raw_ts, bool au_end);

    /**
     * @brief 发送裸 RTP 负载（音频）。
     *
     * @param payload 负载数据
     * @param size 负载字节数
     * @param raw_ts RTP 时间戳（时钟频率为构造时 clock_rate）
     * @param marker 是否置 RTP marker 位（音频在 AU 末包置位）
     * @return 发送结果
     */
    Result SendPayload(const std::uint8_t *payload, std::size_t size, std::uint32_t raw_ts, bool marker);

    /**
     * @brief 按 RFC 2435 打包发送一帧完整 JPEG（自动分片/量化表/marker）。
     *
     * @param frame 帧内零拷贝视图，指向的内存只需在本调用期间有效
     * @param raw_ts RTP 时间戳（视频 90kHz 时钟）
     * @return 发送结果
     */
    Result SendJpegFrame(const JpegFrameInfo &frame, std::uint32_t raw_ts);

    /**
     * @brief 输出是否已积压到上限（smolrtsp 语义：writer.filled() > max_buffer）。
     *
     * @return 已积压到上限返回 true（背压判定：应丢帧/暂停发送）
     */
    bool IsFull() const;

    /** RTP SSRC（供 RTCP SR 使用）。 */
    std::uint32_t Ssrc() const;

    /** 已发送 RTP 包计数（供 RTCP SR 使用）。 */
    std::uint32_t PacketCount() const;

    /** 已发送负载字节计数（供 RTCP SR 使用）。 */
    std::uint32_t OctetCount() const;

    /** 最近发送包的 RTP 时间戳（供 RTCP SR 使用）。 */
    std::uint32_t LastRtpTimestamp() const;

    /** 下一个待发包的 RTP 序号（PLAY 响应 RTP-Info 用；依赖 shim 的基线布局）。 */
    std::uint16_t NextSequence() const;

    /**
     * @brief 把微秒时间映射为本 track 的 32 位 RTP 时间戳。
     *
     * @param time_us 单调时钟时间（微秒）
     * @return 按 clock_rate 与 ts_base 换算并截断到 32 位的 RTP 时间戳
     */
    std::uint32_t MapTimestamp(std::int64_t time_us) const;

    /** 本 track 的 RTP 时钟频率（Hz）。 */
    std::uint32_t clock_rate() const
    {
        return clock_rate_;
    }

private:
    RtpSender() = default;

    /* shim RTP 上下文（拥有；底层 transport 由其析构级联释放）。 */
    ipc_rtsp_shim_rtp *rtp_ = nullptr;
    /* NAL 分片器（拥有；析构级联释放 rtp_）。仅 H264/H265 创建。 */
    ipc_rtsp_shim_nal *nal_ = nullptr;
    /* JPEG 打包器（拥有；析构级联释放 rtp_）。仅 MJPEG 创建。 */
    ipc_rtsp_shim_jpeg *jpeg_ = nullptr;
    /* RTP 时钟频率（Hz）；默认 90000 仅对视频成立，音频由构造参数覆盖。 */
    std::uint32_t clock_rate_ = 90000;
    /* 时间戳基准（单调时钟，微秒）：该时刻对应 RTP 时间戳 0。 */
    std::uint64_t ts_base_us_ = 0;
};

} // namespace detail
} // namespace ipc_rtsp
