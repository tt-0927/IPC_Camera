/**
 * @FilePath     : frame_queue.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-10 11:18:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 17:30:00
 * @Description  : 线程安全的帧队列（RTSP/RTMP公共组件）
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>

/* 队列默认大小 */
#ifndef MAX_VIDEO_FRAME
    #define MAX_VIDEO_FRAME (4)
#endif
#ifndef MAX_AUDIO_FRAME
    #define MAX_AUDIO_FRAME (4)
#endif

/**
 * @brief 帧类型枚举
 */
enum FrameType_E
{
    FRAME_TYPE_VIDEO = 0,
    FRAME_TYPE_AUDIO = 1,
};

/**
 * @brief 帧数据结构体（使用智能指针管理数据）
 */
struct FrameData
{
    /* 共享数据指针：既支持独立拷贝入队，也支持多消费者共享同一副本 */
    std::shared_ptr<unsigned char[]> data;
    int frameSize = 0; /* 帧大小 */
    int type = 0;      /* 帧类型：VIDEO_TYPE 或 AUDIO_TYPE */
    int iFrame = 0;    /* 是否为I帧 */

    FrameData() = default;
    ~FrameData() = default;

    /* 允许拷贝（浅拷贝共享 data），也允许移动 */
    FrameData(const FrameData&) = default;
    FrameData& operator=(const FrameData&) = default;
    FrameData(FrameData&&) = default;
    FrameData& operator=(FrameData&&) = default;
};

/**
 * @brief 帧队列运行时统计快照
 * @note 统计值只用于低频诊断，不参与队列调度；current_* 表示快照时刻的占用量。
 */
struct FrameQueueStats_S
{
    std::size_t current_frames = 0;
    std::size_t current_bytes = 0;
    std::size_t high_water_frames = 0;
    std::size_t high_water_bytes = 0;
    std::uint64_t pushed_frames = 0;
    std::uint64_t pushed_bytes = 0;
    std::uint64_t popped_frames = 0;
    std::uint64_t popped_bytes = 0;
    std::uint64_t dropped_frames = 0;
    std::uint64_t dropped_bytes = 0;
    std::uint64_t cleared_frames = 0;
    std::uint64_t cleared_bytes = 0;
};

/**
 * @brief 线程安全的帧队列类
 * @note 使用 std::deque、std::mutex 和 std::condition_variable 实现线程安全
 *       支持阻塞等待模式，避免空转浪费CPU
 */
class CThreadSafeFrameQueue
{
public:
    /**
     * @brief   : 创建线程安全帧队列
     * @param   {std::size_t} maxSize：最大帧数
     * @param   {std::size_t} maxBytes：最大数据字节数，0表示不启用字节上限
     * @note    : 现有RTMP调用只传maxSize，保持原有行为；RTSP可同时限制帧数和内存。
     */
    explicit CThreadSafeFrameQueue(std::size_t maxSize = 4, std::size_t maxBytes = 0)
        : m_maxSize(maxSize), m_maxBytes(maxBytes)
    {
    }
    ~CThreadSafeFrameQueue() = default;

    /* 禁止拷贝和移动 */
    CThreadSafeFrameQueue(const CThreadSafeFrameQueue&) = delete;
    CThreadSafeFrameQueue& operator=(const CThreadSafeFrameQueue&) = delete;

    /**
     * @brief 入队（移动语义）
     * @return true：成功入队，false：队列已满
     */
    bool push(std::unique_ptr<FrameData> frame)
    {
        if (!frame)
        {
            return false;
        }

        const std::size_t nFrameBytes = get_frame_bytes(frame.get());
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.size() >= m_maxSize ||
                (m_maxBytes > 0 &&
                 (m_stats.current_bytes > m_maxBytes || nFrameBytes > m_maxBytes - m_stats.current_bytes)))
            {
                ++m_stats.dropped_frames;
                m_stats.dropped_bytes += nFrameBytes;
                return false;
            }
            m_stats.current_bytes += nFrameBytes;
            ++m_stats.pushed_frames;
            m_stats.pushed_bytes += nFrameBytes;
            if (m_queue.size() + 1 > m_stats.high_water_frames)
            {
                m_stats.high_water_frames = m_queue.size() + 1;
            }
            if (m_stats.current_bytes > m_stats.high_water_bytes)
            {
                m_stats.high_water_bytes = m_stats.current_bytes;
            }
            m_queue.push_back(std::move(frame));
        }
        /* 通知等待线程 */
        m_cv.notify_one();
        return true;
    }

    /**
     * @brief 出队（移动语义）
     * @return 帧数据，如果队列为空返回 nullptr
     */
    std::unique_ptr<FrameData> pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            return nullptr;
        }
        auto frame = std::move(m_queue.front());
        m_queue.pop_front();
        remove_frame_bytes(frame.get());
        ++m_stats.popped_frames;
        m_stats.popped_bytes += get_frame_bytes(frame.get());
        return frame;
    }

    /**
     * @brief 阻塞等待并出队
     * @param nTimeoutMs 超时时间（毫秒），-1表示无限等待
     * @return 帧数据，超时返回 nullptr
     */
    std::unique_ptr<FrameData> pop_wait(int nTimeoutMs = -1)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (nTimeoutMs < 0)
        {
            m_cv.wait(lock, [this]() { return !m_queue.empty() || m_bStop; });
        }
        else
        {
            m_cv.wait_for(lock, std::chrono::milliseconds(nTimeoutMs),
                          [this]() { return !m_queue.empty() || m_bStop; });
        }
        if (m_queue.empty())
        {
            return nullptr;
        }
        auto frame = std::move(m_queue.front());
        m_queue.pop_front();
        remove_frame_bytes(frame.get());
        ++m_stats.popped_frames;
        m_stats.popped_bytes += get_frame_bytes(frame.get());
        return frame;
    }

    /**
     * @brief 获取队列大小
     */
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    /**
     * @brief 获取队列统计快照
     * @return 统计快照
     * @note 调用方应低频读取，避免诊断锁竞争影响媒体线程。
     */
    FrameQueueStats_S get_stats() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        FrameQueueStats_S stStats = m_stats;
        stStats.current_frames = m_queue.size();
        return stStats;
    }

    /**
     * @brief 判断队列是否已满
     */
    bool isFull() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size() >= m_maxSize;
    }

    /**
     * @brief 判断队列是否为空
     */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /**
     * @brief 清空队列
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stats.cleared_frames += m_queue.size();
        m_stats.cleared_bytes += m_stats.current_bytes;
        m_queue.clear();
        m_stats.current_bytes = 0;
    }

    /**
     * @brief 停止所有等待
     */
    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_bStop = true;
        }
        m_cv.notify_all();
    }

    /**
     * @brief 重置停止状态
     */
    void reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_bStop = false;
    }

private:
    static std::size_t get_frame_bytes(const FrameData *pFrame)
    {
        if (!pFrame || pFrame->frameSize <= 0)
        {
            return 0;
        }
        return static_cast<std::size_t>(pFrame->frameSize);
    }

    void remove_frame_bytes(const FrameData *pFrame)
    {
        const std::size_t nFrameBytes = get_frame_bytes(pFrame);
        m_stats.current_bytes = nFrameBytes > m_stats.current_bytes ? 0 : m_stats.current_bytes - nFrameBytes;
    }

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<std::unique_ptr<FrameData>> m_queue;
    std::size_t m_maxSize;
    std::size_t m_maxBytes;
    FrameQueueStats_S m_stats;
    bool m_bStop = false;
};
