/**
 * @FilePath     : output_buffer.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 每连接的有界输出缓冲（时长制背压）
 */

/*
 * 与 smolrtsp 的 Writer 接口对接：filled() 返回当前字节数，供 TCP transport 的
 * is_full() 判断使用。资源契约：
 * - 容量即内存护栏；控制响应可用预留区，RTP 洪峰不会堵死 TEARDOWN/GET_PARAMETER；
 * - 不无界增长，追加失败时由调用方决定丢帧还是断连；
 * - 拥塞判定不按字节、按「积压时长」：媒体追加记 MediaMark（累计字节+追加时刻），
 *   排空后据剩余字节定位最老媒体数据的等待时长，与分辨率/码率/GOP/单帧大小无关。
 */
#pragma once

#include "ipc_rtsp/config.h"
#include "ipc_rtsp/result.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

/** 有界输出缓冲。 */
class OutputBuffer
{
public:
    /**
     * @brief 按配置初始化内存护栏与控制预留。
     *
     * @param guard_bytes 总容量（内存护栏）；0 视为未配置，回退 256 字节
     * @param control_reserve_bytes 控制预留区大小；超容量时截断到容量
     * @return {void}
     * @note 会清空缓冲与统计，仅应在连接建立时调用一次。
     */
    void Configure(std::size_t guard_bytes, std::size_t control_reserve_bytes);

    /**
     * @brief 追加 RTSP 控制数据；可使用全部容量。
     *
     * @param data 数据指针
     * @param size 字节数
     * @return true 追加成功；false 超出容量（由调用方决定断连）
     */
    bool AppendControl(const void *data, std::size_t size);

    /**
     * @brief 追加 RTP/RTCP 媒体数据；不得占用控制预留区。
     *
     * @param data 数据指针
     * @param size 字节数
     * @param now_ms 追加时刻（单调钟毫秒）；0 表示自动取当前单调钟
     * @return true 追加成功；false 超出媒体区容量（由调用方丢帧或断连）
     */
    bool AppendMedia(const void *data, std::size_t size, std::int64_t now_ms = 0);

    /** 建立一次多段序列化事务的回滚点。 */
    std::size_t Checkpoint() const
    {
        return used_;
    }

    /**
     * @brief 回滚到 Checkpoint，避免 RTSP 响应只留下半包。
     *
     * @param checkpoint 先前 Checkpoint() 返回的回滚点
     * @return {void}
     */
    void Rollback(std::size_t checkpoint);

    /**
     * @brief 尝试把缓冲写到 socket。
     *
     * @param fd 目标 socket
     * @return 累计写入字节数；负值为 -errno（socket 错误，调用方应关闭连接）
     */
    ssize_t FlushTo(int fd);

    /** 当前待发送字节数（Writer::filled 的语义）。 */
    std::size_t size() const
    {
        return used_;
    }

    /** 是否无待发数据。 */
    bool empty() const
    {
        return used_ == 0;
    }

    /** 总容量（含控制预留）。 */
    std::size_t capacity() const
    {
        return capacity_;
    }

    /**
     * 当前积压时长（毫秒）：最老的仍在缓冲中的媒体数据的等待时长。
     * 无媒体积压返回 0；时间倒退返回 0。
     */
    std::uint64_t BacklogMs(std::int64_t now_ms) const
    {
        if (marks_count_ == 0)
        {
            return 0;
        }
        const std::int64_t oldest = marks_[marks_head_].time_ms;
        return now_ms > oldest ? static_cast<std::uint64_t>(now_ms - oldest) : 0;
    }

    /** 媒体可使用的上限，不包含控制预留。 */
    std::size_t media_capacity() const
    {
        return capacity_ - control_reserve_;
    }

    /**
     * 媒体区是否还能容纳 needed 字节：当前已用（含控制数据）未越过媒体界，
     * 且再追加 needed 也不越界；供 TCP 打包循环在发送前预判，避免护栏命中后才失败。
     */
    bool has_media_room(std::size_t needed) const
    {
        return used_ <= media_capacity() && needed <= media_capacity() - used_;
    }

    /** 控制响应是否还有空间。 */
    bool has_control_room(std::size_t needed) const
    {
        return used_ <= capacity_ && needed <= capacity_ - used_;
    }

    /** 本轮已用字节峰值（used_ 的历史最大值，统计用）。 */
    std::size_t high_water() const
    {
        return high_water_;
    }

    /** 清空缓冲（断连/重置时使用）。 */
    void Clear()
    {
        buffer_.clear();
        used_ = 0;
        marks_head_ = 0;
        marks_count_ = 0;
    }

private:
    /** 媒体追加的时间戳记，用于积压时长判定。 */
    struct MediaMark
    {
        /** 追加完成时的累计字节数（含控制数据）。 */
        std::size_t at_total = 0;
        /** 追加时刻，单调钟毫秒。 */
        std::int64_t time_ms = 0;
    };

    /** 在 limit 约束下追加；now_ms 非 0 时同时记一条媒体戳记。 */
    bool AppendWithinLimit(const void *data, std::size_t size, std::size_t limit, std::int64_t now_ms);

    /** 追加一条媒体戳记；满时覆盖最老戳记，O(1) 且无堆分配。 */
    void PushMark(const MediaMark &mark);
    /** 弹出最老戳记。 */
    void PopOldestMark();
    /** 弹出最新戳记（回滚媒体追加时使用）。 */
    void PopNewestMark();

    /** 排空/回滚后修剪已离开缓冲的戳记。 */
    void PruneMarks();

    /** 压缩已发送部分，避免 vector 无限前移。 */
    void CompactIfNeeded();

    /** 待发数据存储；buffer_.size() 可大于 used_，由 CompactIfNeeded 收缩。 */
    std::vector<std::uint8_t> buffer_;
    /** 已写入但未发送的字节数（有效数据恒在头部，从 0 开始）。 */
    std::size_t used_ = 0;
    /** 缓冲总容量；默认 4MB 与配置默认护栏 client_output_guard_bytes 一致。 */
    std::size_t capacity_ = 4u * 1024u * 1024u;
    /** 仅控制响应可用的预留区；默认 16KB，足够容纳任一 RTSP 响应（SDP 数 KB）。 */
    std::size_t control_reserve_ = 16u * 1024u;
    /** used_ 的历史最大值（统计口径，不回落）。 */
    std::size_t high_water_ = 0;
    /** 累计追加字节数（含控制，回滚时回退）。 */
    std::size_t appended_total_ = 0;
    /** 累计发送字节数；不变量 used_ == appended_total_ - flushed_total_。 */
    std::size_t flushed_total_ = 0;
    /*
     * 媒体戳记为编译期定容环形队列：追加路径位于 smolrtsp C 栈帧之下，
     * -fno-exceptions 下隐式扩容的堆分配失败即终止进程，故不允许动态扩容。
     * 容量按硬积压超时内的极端媒体到达条数取裕量（4 track × (30fps 视频 +
     * 45fps 音频) × client_hard_backlog_ms 3000ms ≈ 450）。满时丢最老戳记，
     * backlog 只会被低估：拥塞/超时断开推迟，字节护栏与追加失败分支仍兜底。
     */
    static constexpr std::size_t kMaxMediaMarks = 1024;
    /** 媒体戳记环形队列（容量论证见上）。 */
    std::array<MediaMark, kMaxMediaMarks> marks_{};
    /** 最老戳记下标。 */
    std::size_t marks_head_ = 0;
    /** 当前戳记条数。 */
    std::size_t marks_count_ = 0;
};

} // namespace detail
} // namespace ipc_rtsp
