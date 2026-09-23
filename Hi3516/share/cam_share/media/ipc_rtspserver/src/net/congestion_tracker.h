/**
 * @FilePath     : congestion_tracker.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 14:38:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : TCP 输出拥塞的发送进展跟踪器
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace ipc_rtsp
{
namespace detail
{

/**
 * 按“连续无发送进展时长”判定慢客户端。
 *
 * 缓冲区只要仍在实际写入 socket，即使长时间高于软水位也不超时；
 * 只有完全没有写进展超过配置时长才断开。
 */
class CongestionTracker
{
public:
    /**
     * @brief 进入拥塞计时（幂等；已在计时中则忽略）。
     *
     * @param now_ms 当前时刻，单调钟毫秒
     * @return {void}
     */
    void Enter(std::int64_t now_ms)
    {
        if (active_)
        {
            return;
        }
        active_ = true;
        entered_ms_ = now_ms;
        last_progress_ms_ = now_ms;
    }

    /**
     * @brief 记录一次发送进展。
     *
     * @param now_ms 当前时刻，单调钟毫秒
     * @param bytes 本次实际写出字节数；>0 即视为有进展并刷新计时
     * @return {void}
     */
    void NoteProgress(std::int64_t now_ms, std::size_t bytes)
    {
        if (active_ && bytes > 0)
        {
            last_progress_ms_ = now_ms;
        }
    }

    /**
     * @brief 连续无进展是否已超过阈值。
     *
     * @param now_ms 当前时刻，单调钟毫秒
     * @param timeout_ms 无进展超时阈值（毫秒）
     * @return 仅在计时激活时判定；未激活恒为 false
     */
    bool TimedOut(std::int64_t now_ms, std::uint32_t timeout_ms) const
    {
        return active_ && now_ms > last_progress_ms_ && static_cast<std::uint64_t>(now_ms - last_progress_ms_) > timeout_ms;
    }

    /**
     * @brief 已持续拥塞的时长。
     *
     * @param now_ms 当前时刻，单调钟毫秒
     * @return 拥塞毫秒数；未激活或时钟未推进返回 0
     */
    std::uint64_t CongestedMs(std::int64_t now_ms) const
    {
        if (!active_ || now_ms <= entered_ms_)
        {
            return 0;
        }
        return static_cast<std::uint64_t>(now_ms - entered_ms_);
    }

    /**
     * @brief 距最近一次发送进展的时长。
     *
     * @param now_ms 当前时刻，单调钟毫秒
     * @return 无进展毫秒数；未激活或时钟未推进返回 0
     */
    std::uint64_t NoProgressMs(std::int64_t now_ms) const
    {
        if (!active_ || now_ms <= last_progress_ms_)
        {
            return 0;
        }
        return static_cast<std::uint64_t>(now_ms - last_progress_ms_);
    }

    /**
     * @brief 退出拥塞计时并复位状态。
     *
     * @param now_ms 当前时刻，单调钟毫秒
     * @return 本次拥塞持续时长（毫秒），供调用方累计统计；未激活返回 0
     */
    std::uint64_t Leave(std::int64_t now_ms)
    {
        const std::uint64_t duration_ms = CongestedMs(now_ms);
        Reset();
        return duration_ms;
    }

    /** @brief 复位全部状态（不返回拥塞时长）。 @return {void} */
    void Reset()
    {
        active_ = false;
        entered_ms_ = 0;
        last_progress_ms_ = 0;
    }

    /** @brief 是否处于拥塞计时中。 @return true 表示计时激活 */
    bool active() const
    {
        return active_;
    }

private:
    /** 不变量：active_ 为 true 期间 entered_ms_/last_progress_ms_ 有效且非 0。 */
    bool active_ = false;
    /** 进入拥塞态的时刻（单调钟毫秒）。 */
    std::int64_t entered_ms_ = 0;
    /** 最近一次发送进展的时刻（单调钟毫秒）。 */
    std::int64_t last_progress_ms_ = 0;
};

} // namespace detail
} // namespace ipc_rtsp
