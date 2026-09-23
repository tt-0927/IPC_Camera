/**
 * @FilePath     : output_buffer.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 有界输出缓冲实现（时长制背压）
 */

#include "net/output_buffer.h"

#include "support/time.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>

namespace ipc_rtsp
{
namespace detail
{

void OutputBuffer::Configure(std::size_t guard_bytes, std::size_t control_reserve_bytes)
{
    /* guard_bytes==0（未配置/异常配置）回退 256 字节，避免容量为 0 导致永远无法追加。 */
    capacity_ = guard_bytes == 0 ? 256u : guard_bytes;
    control_reserve_ = std::min(control_reserve_bytes, capacity_);
    buffer_.clear();
    buffer_.reserve(capacity_);
    used_ = 0;
    high_water_ = 0;
    appended_total_ = 0;
    flushed_total_ = 0;
    marks_head_ = 0;
    marks_count_ = 0;
}

void OutputBuffer::PushMark(const MediaMark &mark)
{
    if (marks_count_ < kMaxMediaMarks)
    {
        marks_[(marks_head_ + marks_count_) % kMaxMediaMarks] = mark;
        ++marks_count_;
        return;
    }
    /* 满：覆盖最老戳记（容量论证见 output_buffer.h）。 */
    marks_[marks_head_] = mark;
    marks_head_ = (marks_head_ + 1) % kMaxMediaMarks;
}

void OutputBuffer::PopOldestMark()
{
    marks_head_ = (marks_head_ + 1) % kMaxMediaMarks;
    --marks_count_;
}

void OutputBuffer::PopNewestMark()
{
    --marks_count_;
}

void OutputBuffer::CompactIfNeeded()
{
    if (used_ == 0)
    {
        buffer_.clear();
        return;
    }
    /* 头部已被消费时把剩余数据前移，保证容量语义恒定。 */
    if (buffer_.size() > used_)
    {
        std::memmove(buffer_.data(), buffer_.data() + (buffer_.size() - used_), used_);
        buffer_.resize(used_);
    }
}

bool OutputBuffer::AppendControl(const void *data, std::size_t size)
{
    return AppendWithinLimit(data, size, capacity_, 0);
}

bool OutputBuffer::AppendMedia(const void *data, std::size_t size, std::int64_t now_ms)
{
    if (now_ms == 0)
    {
        now_ms = NowMonotonicMs();
    }
    return AppendWithinLimit(data, size, media_capacity(), now_ms);
}

bool OutputBuffer::AppendWithinLimit(const void *data, std::size_t size, std::size_t limit, std::int64_t now_ms)
{
    if (data == nullptr || size == 0)
    {
        return true;
    }

    if (used_ > limit || size > limit - used_)
    {
        return false;
    }

    CompactIfNeeded();
    buffer_.resize(used_ + size);
    std::memcpy(buffer_.data() + used_, data, size);
    used_ += size;
    appended_total_ += size;

    if (now_ms != 0)
    {
        PushMark(MediaMark{ appended_total_, now_ms });
    }

    if (used_ > high_water_)
    {
        high_water_ = used_;
    }
    return true;
}

void OutputBuffer::PruneMarks()
{
    while (marks_count_ > 0 && marks_[marks_head_].at_total <= flushed_total_)
    {
        PopOldestMark();
    }
}

void OutputBuffer::Rollback(std::size_t checkpoint)
{
    if (checkpoint > used_)
    {
        return;
    }
    buffer_.resize(checkpoint);
    appended_total_ -= used_ - checkpoint;
    used_ = checkpoint;
    /* 事务回滚只针对控制响应；若确有媒体被回退，同步丢弃对应戳记。 */
    while (marks_count_ > 0 && marks_[(marks_head_ + marks_count_ - 1) % kMaxMediaMarks].at_total > appended_total_)
    {
        PopNewestMark();
    }
}

ssize_t OutputBuffer::FlushTo(int fd)
{
    if (fd < 0 || used_ == 0)
    {
        return 0;
    }

    CompactIfNeeded();

    std::size_t written_total = 0;
    while (written_total < used_)
    {
        /* MSG_NOSIGNAL：对端已关闭时返回 EPIPE 错误，而不是 SIGPIPE 杀死进程。 */
        const ssize_t written = ::send(fd, buffer_.data() + written_total, used_ - written_total, MSG_NOSIGNAL);
        if (written > 0)
        {
            written_total += static_cast<std::size_t>(written);
            continue;
        }
        if (written < 0 && errno == EINTR)
        {
            /* 信号中断：重试本次发送。 */
            continue;
        }
        if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            /* 内核发送缓冲已满：保留剩余数据，等 EPOLLOUT 再续。 */
            break;
        }
        if (written < 0)
        {
            /* 其余 errno：真实发送错误，以 -errno 返回交调用方关闭连接。 */
            return -errno;
        }
        /* send 返回 0（TCP 上罕见）：本轮无进展，退出循环避免忙等。 */
        break;
    }

    if (written_total == used_)
    {
        buffer_.clear();
        used_ = 0;
    }
    else if (written_total > 0)
    {
        /* 部分发送：把剩余数据前移收缩，维持剩余数据恒在头部的布局。 */
        std::memmove(buffer_.data(), buffer_.data() + written_total, used_ - written_total);
        buffer_.resize(used_ - written_total);
        used_ -= written_total;
    }

    if (written_total > 0)
    {
        flushed_total_ += written_total;
        PruneMarks();
    }

    return static_cast<ssize_t>(written_total);
}

} // namespace detail
} // namespace ipc_rtsp
