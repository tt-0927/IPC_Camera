/**
 * @FilePath     : rtp_sender.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTP 发送器实现
 */

#include "rtp/rtp_sender.h"

#include "adapter/smolrtsp_shim.h"
#include "media/jpeg_parser.h"
#include "support/log.h"

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr const char *kTag = "rtp";
} // namespace

RtpSender::~RtpSender()
{
    if (jpeg_ != nullptr)
    {
        /* jpeg 释放会级联释放 rtp 与下层 transport。 */
        ipc_rtsp_shim_jpeg_free(jpeg_);
        jpeg_ = nullptr;
        rtp_ = nullptr;
        return;
    }
    if (nal_ != nullptr)
    {
        /* nal 释放会级联释放 rtp 与下层 transport。 */
        ipc_rtsp_shim_nal_free(nal_);
        nal_ = nullptr;
        rtp_ = nullptr;
        return;
    }
    if (rtp_ != nullptr)
    {
        ipc_rtsp_shim_rtp_free(rtp_);
        rtp_ = nullptr;
    }
}

RtpSender *RtpSender::CreateTcp(ipc_rtsp_shim_writer *writer,
                                std::uint8_t channel,
                                std::size_t max_buffer,
                                std::uint32_t ssrc,
                                std::uint8_t payload_type,
                                std::uint32_t clock_rate,
                                std::uint64_t ts_base_us,
                                Codec codec,
                                std::size_t max_nalu_bytes)
{
    if (writer == nullptr)
    {
        return nullptr;
    }

    ipc_rtsp_shim_rtp *rtp = ipc_rtsp_shim_rtp_new_tcp(writer, channel, max_buffer, ssrc, payload_type, clock_rate, ts_base_us);
    if (rtp == nullptr)
    {
        IPC_RTSP_LOGE(kTag, "创建TCP RTP发送器失败");
        return nullptr;
    }

    RtpSender *sender = new RtpSender();
    sender->rtp_ = rtp;
    sender->clock_rate_ = clock_rate;
    sender->ts_base_us_ = ts_base_us;

    if (codec == Codec::H264 || codec == Codec::H265)
    {
        sender->nal_ = (codec == Codec::H264) ? ipc_rtsp_shim_nal_new_h264(rtp, max_nalu_bytes)
                                              : ipc_rtsp_shim_nal_new_h265(rtp, max_nalu_bytes);
        if (sender->nal_ == nullptr)
        {
            IPC_RTSP_LOGE(kTag, "创建NAL发送器失败 codec:%s", CodecToString(codec));
            /* nal 创建失败时 rtp 已被级联释放，这里避免二次释放。 */
            sender->rtp_ = nullptr;
            delete sender;
            return nullptr;
        }
    }
    else if (codec == Codec::MJPEG)
    {
        /* MJPEG 复用 NAL 的单包负载预算（同为 UDP MTU 安全上限）。 */
        sender->jpeg_ = ipc_rtsp_shim_jpeg_new(rtp, max_nalu_bytes > 0 ? max_nalu_bytes : 1200);
        if (sender->jpeg_ == nullptr)
        {
            IPC_RTSP_LOGE(kTag, "创建JPEG发送器失败");
            /* jpeg 创建失败时 rtp 已被级联释放，这里避免二次释放。 */
            sender->rtp_ = nullptr;
            delete sender;
            return nullptr;
        }
    }

    return sender;
}

RtpSender *RtpSender::CreateUdp(int fd,
                                std::uint32_t ssrc,
                                std::uint8_t payload_type,
                                std::uint32_t clock_rate,
                                std::uint64_t ts_base_us,
                                Codec codec,
                                std::size_t max_nalu_bytes)
{
    ipc_rtsp_shim_rtp *rtp = ipc_rtsp_shim_rtp_new_udp(fd, ssrc, payload_type, clock_rate, ts_base_us);
    if (rtp == nullptr)
    {
        IPC_RTSP_LOGE(kTag, "创建UDP RTP发送器失败");
        return nullptr;
    }

    RtpSender *sender = new RtpSender();
    sender->rtp_ = rtp;
    sender->clock_rate_ = clock_rate;
    sender->ts_base_us_ = ts_base_us;

    if (codec == Codec::H264 || codec == Codec::H265)
    {
        sender->nal_ = (codec == Codec::H264) ? ipc_rtsp_shim_nal_new_h264(rtp, max_nalu_bytes)
                                              : ipc_rtsp_shim_nal_new_h265(rtp, max_nalu_bytes);
        if (sender->nal_ == nullptr)
        {
            IPC_RTSP_LOGE(kTag, "创建UDP NAL发送器失败 codec:%s", CodecToString(codec));
            /* nal 创建失败时 rtp 已被级联释放，这里避免二次释放。 */
            sender->rtp_ = nullptr;
            delete sender;
            return nullptr;
        }
    }
    else if (codec == Codec::MJPEG)
    {
        /* MJPEG 复用 NAL 的单包负载预算（同为 UDP MTU 安全上限）。 */
        sender->jpeg_ = ipc_rtsp_shim_jpeg_new(rtp, max_nalu_bytes > 0 ? max_nalu_bytes : 1200);
        if (sender->jpeg_ == nullptr)
        {
            IPC_RTSP_LOGE(kTag, "创建UDP JPEG发送器失败");
            /* jpeg 创建失败时 rtp 已被级联释放，这里避免二次释放。 */
            sender->rtp_ = nullptr;
            delete sender;
            return nullptr;
        }
    }

    return sender;
}

Result RtpSender::SendNal(const std::uint8_t *nal, std::size_t size, std::uint32_t raw_ts, bool au_end)
{
    if (nal == nullptr || size == 0)
    {
        return Result::Fail(Status::InvalidArgument);
    }

    if (jpeg_ != nullptr)
    {
        /* JPEG 轨不承载 NAL 语义，误用直接拒绝。 */
        return Result::Fail(Status::InvalidArgument);
    }

    if (nal_ == nullptr)
    {
        /* 非分层 codec（无上层 transport）：单包即完整 AU，au_end 直接映射
         * 为 marker=true，保证接收端按 AU 边界组帧。 */
        return SendPayload(nal, size, raw_ts, true);
    }

    if (ipc_rtsp_shim_nal_send(nal_, raw_ts, au_end, nal, size) != 0)
    {
        return Result::Io(errno);
    }
    return Result::Ok();
}

Result RtpSender::SendPayload(const std::uint8_t *payload, std::size_t size, std::uint32_t raw_ts, bool marker)
{
    if (payload == nullptr || size == 0 || rtp_ == nullptr)
    {
        return Result::Fail(Status::InvalidArgument);
    }
    if (ipc_rtsp_shim_rtp_send(rtp_, raw_ts, marker, payload, size) != 0)
    {
        return Result::Io(errno);
    }
    return Result::Ok();
}

Result RtpSender::SendJpegFrame(const JpegFrameInfo &frame, std::uint32_t raw_ts)
{
    if (jpeg_ == nullptr || frame.scan == nullptr || frame.scan_size == 0 || frame.qt[0] == nullptr)
    {
        return Result::Fail(Status::InvalidArgument);
    }

    ipc_rtsp_shim_jpeg_frame shim;
    shim.type = frame.type;
    /* Q=255：动态量化表，首包内联本帧实际 DQT（与 FFmpeg rtpenc_jpeg、RFC 2435
     * Q=255 语义一致：每帧重载，Length 不得为 0）。 */
    shim.q = 255;
    shim.width_blocks = frame.width_blocks;
    shim.height_blocks = frame.height_blocks;
    shim.restart_interval = frame.restart_interval;
    shim.qt0 = frame.qt[0];
    shim.qt0_len = kJpegQuantTableBytes;
    /* 接收端（RFC 2435 重建）固定按"亮度=表0、色度=表1"组帧；部分编码器
     * （含 ffmpeg mjpeg）只用一张表编码所有分量，这里补发同内容的表 1，
     * 量化系数一致，解码无损。 */
    shim.qt1 = frame.qt[1] != nullptr ? frame.qt[1] : frame.qt[0];
    shim.qt1_len = kJpegQuantTableBytes;
    shim.scan = frame.scan;
    shim.scan_len = frame.scan_size;

    if (ipc_rtsp_shim_jpeg_send_frame(jpeg_, raw_ts, &shim) != 0)
    {
        return Result::Io(errno);
    }
    return Result::Ok();
}

bool RtpSender::IsFull() const
{
    return rtp_ != nullptr && ipc_rtsp_shim_rtp_is_full(rtp_);
}

std::uint32_t RtpSender::Ssrc() const
{
    return rtp_ == nullptr ? 0u : ipc_rtsp_shim_rtp_ssrc(rtp_);
}

std::uint32_t RtpSender::PacketCount() const
{
    return rtp_ == nullptr ? 0u : ipc_rtsp_shim_rtp_pkt_count(rtp_);
}

std::uint32_t RtpSender::OctetCount() const
{
    return rtp_ == nullptr ? 0u : ipc_rtsp_shim_rtp_octet_count(rtp_);
}

std::uint16_t RtpSender::NextSequence() const
{
    return rtp_ == nullptr ? 0u : ipc_rtsp_shim_rtp_next_seq(rtp_);
}

std::uint32_t RtpSender::LastRtpTimestamp() const
{
    return rtp_ == nullptr ? 0u : ipc_rtsp_shim_rtp_last_ts(rtp_);
}

std::uint32_t RtpSender::MapTimestamp(std::int64_t time_us) const
{
    return ipc_rtsp_shim_rtp_ts_from_us(static_cast<std::uint64_t>(time_us), clock_rate_, ts_base_us_);
}

} // namespace detail
} // namespace ipc_rtsp
