/**
 * @FilePath     : frame_queue.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-10 11:18:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-10 14:06:34
 * @Description  : 线程安全的帧队列（RTSP/RTMP公共组件）
 */

#pragma once

#include <mutex>
#include <deque>
#include <memory>
#include <atomic>
#include <condition_variable>

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
    std::unique_ptr<unsigned char[]> data; /* 帧数据，使用智能指针自动管理内存 */
    int frameSize = 0;                     /* 帧大小 */
    int type = 0;                          /* 帧类型：VIDEO_TYPE 或 AUDIO_TYPE */
    int iFrame = 0;                        /* 是否为I帧 */

    FrameData() = default;
    ~FrameData() = default;

    /* 禁止拷贝，只允许移动 */
    FrameData(const FrameData&) = delete;
    FrameData& operator=(const FrameData&) = delete;
    FrameData(FrameData&&) = default;
    FrameData& operator=(FrameData&&) = default;
};

/**
 * @brief 线程安全的帧队列类
 * @note 使用 std::deque、std::mutex 和 std::condition_variable 实现线程安全
 *       支持阻塞等待模式，避免空转浪费CPU
 */
class CThreadSafeFrameQueue
{
public:
    explicit CThreadSafeFrameQueue(size_t maxSize = 4) : m_maxSize(maxSize)
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
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.size() >= m_maxSize)
            {
                return false;
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
        return frame;
    }

    /**
     * @brief 获取队列大小
     */
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
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
        m_queue.clear();
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
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<std::unique_ptr<FrameData>> m_queue;
    size_t m_maxSize;
    bool m_bStop = false;
};
