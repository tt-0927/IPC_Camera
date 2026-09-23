/**
 * @FilePath     : rtcp_sender.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : RTCP 发送器实现
 */

#include "rtp/rtcp_sender.h"

#include "adapter/smolrtsp_shim.h"
#include "support/log.h"
#include "support/time.h"

#include <cerrno>
#include <cstring>
#include <sys/socket.h>

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr const char *kTag = "rtcp";

/** compound RTCP 缓冲：SR + SDES 足够小；直接取 shim interleaved 上限，防止两处常量漂移。 */
constexpr std::size_t kRtcpBufferBytes = IPC_RTSP_SHIM_INTERLEAVED_PAYLOAD_MAX;

} // namespace

void RtcpSender::Configure(RtpSender *rtp, bool tcp, std::uint8_t channel, ipc_rtsp_shim_writer *writer, int rtcp_fd, std::string cname)
{
    rtp_ = rtp;
    tcp_ = tcp;
    channel_ = channel;
    writer_ = writer;
    rtcp_fd_ = rtcp_fd;
    cname_ = std::move(cname);
    last_sr_ms_ = 0;
    bye_sent_ = false;
}

Result RtcpSender::SendPacket(const std::uint8_t *data, std::size_t size)
{
    if (size == 0)
    {
        return Result::Fail(Status::InvalidArgument);
    }

    if (tcp_)
    {
        if (writer_ == nullptr || ipc_rtsp_shim_write_interleaved(writer_, channel_, data, size) != 0)
        {
            return Result::Io(errno);
        }
    }
    else
    {
        if (rtcp_fd_ < 0)
        {
            return Result::Fail(Status::NotInitialized);
        }
        const ssize_t written = ::send(rtcp_fd_, data, size, 0);
        if (written < 0)
        {
            return Result::Io(errno);
        }
    }

    ++packets_sent_;
    return Result::Ok();
}

bool RtcpSender::MaybeSendSr(std::int64_t now_ms, std::uint32_t interval_ms)
{
    if (rtp_ == nullptr)
    {
        return false;
    }
    /* last_sr_ms_ == 0 是"尚未发送"哨兵：首个 SR 不走间隔判断，立即发出。 */
    if (last_sr_ms_ != 0 && (now_ms - last_sr_ms_) < static_cast<std::int64_t>(interval_ms))
    {
        return false;
    }
    last_sr_ms_ = now_ms;

    std::uint8_t buffer[kRtcpBufferBytes];
    std::size_t offset = 0;

    /* SR 的 NTP 与 RTP 时间戳必须描述同一时刻：这里同时采样实时时钟与单调时钟，
     * 再用本 track 的 ts_base 把单调时钟换算成 RTP 时间戳。 */
    std::uint32_t ntp_sec = 0;
    std::uint32_t ntp_frac = 0;
    std::int64_t mono_us = 0;
    SampleRealtimeNtp(&ntp_sec, &ntp_frac, &mono_us);

    const std::uint32_t rtp_ts = rtp_->MapTimestamp(mono_us);

    const std::size_t sr_size = ipc_rtsp_shim_rtcp_sr(buffer,
                                                      sizeof buffer,
                                                      rtp_->Ssrc(),
                                                      ntp_sec,
                                                      ntp_frac,
                                                      rtp_ts,
                                                      rtp_->PacketCount(),
                                                      rtp_->OctetCount());
    if (sr_size == 0)
    {
        return false;
    }
    offset += sr_size;

    if (!cname_.empty())
    {
        const std::size_t sdes_size = ipc_rtsp_shim_rtcp_sdes_cname(buffer + offset, sizeof buffer - offset, rtp_->Ssrc(), cname_.c_str());
        offset += sdes_size;
    }

    const Result sent = SendPacket(buffer, offset);
    if (!sent.ok())
    {
        IPC_RTSP_LOGW(kTag, "发送SR失败 状态:%s errno:%d", StatusToString(sent.code), sent.sys_errno);
        return false;
    }
    return true;
}

void RtcpSender::SendBye()
{
    if (rtp_ == nullptr || bye_sent_)
    {
        return;
    }
    bye_sent_ = true;

    std::uint8_t buffer[kRtcpBufferBytes];
    const std::size_t size = ipc_rtsp_shim_rtcp_bye(buffer, sizeof buffer, rtp_->Ssrc(), nullptr);
    if (size == 0)
    {
        return;
    }
    const Result sent = SendPacket(buffer, size);
    if (!sent.ok())
    {
        /* BYE 是尽力而为的收尾通知：连接关闭阶段发送失败属常态（如 TCP writer
         * 已释放、UDP 对端先退），一律按调试级记录，不作为告警。 */
        IPC_RTSP_LOGD(kTag, "发送BYE未成功（尽力而为） errno:%d", sent.sys_errno);
    }
}

} // namespace detail
} // namespace ipc_rtsp
