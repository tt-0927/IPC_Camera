/**
 * @FilePath     : frame_queue.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-10 11:18:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-03 18:43:30
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
#include <utility>

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
 * @brief 帧关键性标记
 * @note 该标记复用 FrameData::iFrame 的 int 字段，保持 RTMP/RTSP 既有
 *       数据结构兼容；参数集标记只表示输入本身就是独立参数集 pack，不能
 *       用来拆分单包模式中已经包含 SPS/PPS/IDR 的完整编码 pack。
 */
enum FrameMarker_E
{
    FRAME_MARKER_UNKNOWN = -1,       /* 无法从当前输入确认帧类型 */
    FRAME_MARKER_NON_KEY = 0,        /* 普通P/B帧或其他非关键NAL */
    FRAME_MARKER_KEYFRAME = 1,       /* 可作为解码起点的完整关键 pack */
    FRAME_MARKER_PARAMETER_SET = 2,  /* 输入本身是独立的SPS/PPS/VPS pack */
    FRAME_MARKER_INDEPENDENT_FRAME = 3, /* 可独立解码但不参与GOP保护的完整帧，如MJPEG */
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
    /* -1未知，0普通 pack，1关键 pack，2独立参数集，3独立帧；保留int兼容既有调用方。 */
    int iFrame = FRAME_MARKER_NON_KEY;

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
    std::uint64_t dropped_keyframes = 0; /* 被丢弃的I帧数量，正常应恒为0，用于诊断花屏 */
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
     * @param   {bool} bDropOldestForKeyframe：满队列时是否淘汰最老非保护帧
     * @note    : 现有RTMP调用只传maxSize，保持原有行为；RTSP显式开启GOP感知策略。
     */
    explicit CThreadSafeFrameQueue(std::size_t maxSize = 4,
                                   std::size_t maxBytes = 0,
                                   bool bDropOldestForKeyframe = false)
        : m_maxSize(maxSize), m_maxBytes(maxBytes), m_bDropOldestForKeyframe(bDropOldestForKeyframe)
    {
    }
    ~CThreadSafeFrameQueue() = default;

    /* 禁止拷贝和移动 */
    CThreadSafeFrameQueue(const CThreadSafeFrameQueue&) = delete;
    CThreadSafeFrameQueue& operator=(const CThreadSafeFrameQueue&) = delete;

    /**
     * @brief 入队（移动语义）
     * @param frame 待入队帧的所有权；失败时由队列函数内部释放
     * @return true：成功入队，false：容量不足或帧为空
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
            bool bCanPush = has_capacity(nFrameBytes);
            if (!bCanPush && m_bDropOldestForKeyframe)
            {
                /* perf: 仅在容量不足时扫描并淘汰旧普通/独立帧，不复制或扩张队列存储。 */
                bCanPush = make_room_for_frame(frame.get());
            }
            if (!bCanPush)
            {
                record_dropped_frame(frame.get());
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

    /**
     * @brief 判断新帧能否直接进入队列
     * @param nFrameBytes 新帧字节数
     * @return true：帧数和字节预算均满足；false：需要丢帧或拒绝
     * @note 调用方必须已经持有 m_mutex。
     */
    bool has_capacity(const std::size_t nFrameBytes) const
    {
        if (m_queue.size() >= m_maxSize)
        {
            return false;
        }
        if (m_maxBytes == 0)
        {
            return true;
        }
        if (m_stats.current_bytes > m_maxBytes)
        {
            return false;
        }
        return nFrameBytes <= m_maxBytes - m_stats.current_bytes;
    }

    /**
     * @brief 判断帧是否属于优先保留的保护帧
     * @param pFrame 待判断帧
     * @return true：关键 pack或独立参数集 pack；false：可优先淘汰的普通/独立帧 pack
     * @note 单包复合帧始终作为一个关键 pack保护，不能从中拆出参数集后分别处理；
     *       独立帧（例如MJPEG）虽然可作为新客户端起始帧，但弱网时应允许淘汰旧帧。
     */
    static bool is_protected_frame(const FrameData *pFrame)
    {
        return pFrame != nullptr &&
               (pFrame->iFrame == FRAME_MARKER_KEYFRAME ||
                pFrame->iFrame == FRAME_MARKER_PARAMETER_SET);
    }

    /**
     * @brief 判断帧是否为独立参数集 pack
     * @param pFrame 待判断帧
     * @return true：输入本身只包含参数集；false：关键或普通完整 pack
     */
    static bool is_parameter_set(const FrameData *pFrame)
    {
        return pFrame != nullptr && pFrame->iFrame == FRAME_MARKER_PARAMETER_SET;
    }

    /**
     * @brief 在队列内淘汰最老的非保护帧
     * @return true：成功淘汰一帧；false：队列中只剩保护帧
     * @note 调用方必须已经持有 m_mutex；只释放既有节点，不增加缓存。
     */
    bool discard_oldest_non_protected_frame()
    {
        for (auto it = m_queue.begin(); it != m_queue.end(); ++it)
        {
            if (is_protected_frame(it->get()))
            {
                continue;
            }

            std::unique_ptr<FrameData> pDroppedFrame = std::move(*it);
            m_queue.erase(it);
            remove_frame_bytes(pDroppedFrame.get());
            record_dropped_frame(pDroppedFrame.get());
            return true;
        }
        return false;
    }

    /**
     * @brief 在保护 pack 中替换最老的独立参数集 pack
     * @return true：成功替换一个参数集；false：没有可替换的参数集
     * @note 新的关键 pack或独立参数集 pack到达、队列只剩保护项时才允许调用；
     *       关键 pack永不淘汰。
     */
    bool discard_oldest_parameter_set()
    {
        for (auto it = m_queue.begin(); it != m_queue.end(); ++it)
        {
            if (!is_parameter_set(it->get()))
            {
                continue;
            }

            std::unique_ptr<FrameData> pDroppedFrame = std::move(*it);
            m_queue.erase(it);
            remove_frame_bytes(pDroppedFrame.get());
            record_dropped_frame(pDroppedFrame.get());
            return true;
        }
        return false;
    }

    /**
     * @brief 为新帧腾出既有队列空间
     * @param pFrame 待入队 pack，用于判断是否允许替换旧的独立参数集 pack
     * @return true：腾位后可以入队；false：预算不足或只剩保护帧
     * @note 单个完整 pack超过字节上限时直接失败；新关键 pack/独立参数集可替换旧参数集，
     *       但不淘汰已有关键 pack。
     */
    bool make_room_for_frame(const FrameData *pFrame)
    {
        const std::size_t nFrameBytes = get_frame_bytes(pFrame);
        if (m_maxBytes > 0 && nFrameBytes > m_maxBytes)
        {
            return false;
        }

        while (!has_capacity(nFrameBytes))
        {
            if (discard_oldest_non_protected_frame())
            {
                continue;
            }

            /* 新的关键 pack/独立参数集可替换过时参数集，避免保护项堆满后阻塞新的IDR。 */
            if ((pFrame != nullptr &&
                 (pFrame->iFrame == FRAME_MARKER_KEYFRAME || is_parameter_set(pFrame))) &&
                discard_oldest_parameter_set())
            {
                continue;
            }
            return false;
        }
        return true;
    }

    /**
     * @brief 累计一次帧丢弃统计
     * @param pFrame 被丢弃的帧，可为空
     * @return 无
     * @note dropped_keyframes 只统计关键 pack，不把独立参数集 pack误计为关键帧。
     */
    void record_dropped_frame(const FrameData *pFrame)
    {
        ++m_stats.dropped_frames;
        m_stats.dropped_bytes += get_frame_bytes(pFrame);
        if (pFrame != nullptr && pFrame->iFrame == FRAME_MARKER_KEYFRAME)
        {
            ++m_stats.dropped_keyframes;
        }
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
    bool m_bDropOldestForKeyframe;
    FrameQueueStats_S m_stats;
    bool m_bStop = false;
};
