#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "dlog.h"


// 简单封装一下日志，统一前缀
#ifndef TM_LOGT
    #define TM_LOGT(fmt, ...) dlog(LOG_TRACE, "[Timer] " fmt, ##__VA_ARGS__)
#endif
#ifndef TM_LOGD
    #define TM_LOGD(fmt, ...) dlog(LOG_DEBUG, "[Timer] " fmt, ##__VA_ARGS__)
#endif
#ifndef TM_LOGI
    #define TM_LOGI(fmt, ...) dlog(LOG_INFO, "[Timer] " fmt, ##__VA_ARGS__)
#endif
#ifndef TM_LOGW
    #define TM_LOGW(fmt, ...) dlog(LOG_WARN, "[Timer][WARN] " fmt, ##__VA_ARGS__)
#endif
#ifndef TM_LOGE
    #define TM_LOGE(fmt, ...) dlog(LOG_ERROR, "[Timer][ERR ] " fmt, ##__VA_ARGS__)
#endif

namespace TimerManager_NS
{

    /**
     * @brief 任务状态
     */
    enum class TaskStatus
    {
        RUNNING,    ///< 正常运行
        PAUSED,     ///< 已暂停（不会执行）
        STOPPED     ///< 已停止（从管理器中移除）
    };

    /**
     * @brief 任务优先级（数值越小优先级越高）
     */
    enum class TaskPriority
    {
        HIGH   = 0,
        NORMAL = 1,
        LOW    = 2
    };

    /**
     * @brief 任务执行统计记录
     */
    struct TaskStat
    {
        uint64_t runCount    = 0;             ///< 已执行次数
        uint64_t totalCostUs = 0;             ///< 总耗时（微秒）
        uint64_t maxCostUs   = 0;             ///< 最大单次耗时（微秒）
        uint64_t minCostUs   = UINT64_MAX;    ///< 最小单次耗时（微秒）
        uint64_t lastCostUs  = 0;             ///< 最近一次执行耗时（微秒）

        double avgCostUs() const
        {
            return runCount == 0 ? 0.0 : static_cast<double>(totalCostUs) / static_cast<double>(runCount);
        }
    };

    /**
     * @brief 定时任务结构体
     *
     * 注意：
     *   - func 使用 std::shared_ptr<void> 作为参数，方便统一传递任意类型。
     *   - userData 的生命周期由 shared_ptr 管理，用户不需要自己 delete。
     */
    struct TimerTask
    {
        uint64_t    taskId = 0;                                   ///< 任务 ID
        std::string name;                                         ///< 任务名称（调试用）

        std::chrono::steady_clock::time_point nextRunTime;        ///< 下次执行时间点
        std::chrono::milliseconds             interval { 0 };     ///< 执行间隔，0 表示单次任务
        int                                   repeatCount = 1;    ///< 剩余执行次数，0 表示无限循环

        TaskStatus   status   = TaskStatus::RUNNING;              ///< 当前状态
        TaskPriority priority = TaskPriority::NORMAL;             ///< 优先级

        std::function<void(std::shared_ptr<void>)> func;          ///< 用户回调函数
        std::shared_ptr<void>                      userData;      ///< 用户参数

        std::chrono::milliseconds timeout { 0 };                  ///< 单次执行超时（0 表示不检测）

        TaskStat stat;                                            ///< 执行统计信息
    };

    /**
     * @brief 无锁有界队列（MPMC，基于 Vyukov 算法）
     *
     * 模板参数：
     *   T  队列中存储的数据类型
     *   N  队列容量，必须为 2 的幂
     *
     * 特点：
     *   - 多生产者多消费者并发安全
     *   - 不使用 mutex，不会阻塞（除了自旋和 yield）
     *   - 适合作为线程池任务队列
     */
    template<typename T, size_t N>
    class LockFreeQueue
    {
        static_assert((N & (N - 1)) == 0, "N must be power of 2");

    public:

        LockFreeQueue()
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_buffer[i].seq.store(i, std::memory_order_relaxed);
            }
            m_head.store(0, std::memory_order_relaxed);
            m_tail.store(0, std::memory_order_relaxed);
        }

        /**
         * @brief 入队
         * @return true 表示成功，false 表示队列满
         */
        bool enqueue(const T& data)
        {
            Cell*  cell;
            size_t pos = m_head.load(std::memory_order_relaxed);

            for (;;)
            {
                cell          = &m_buffer[pos & (N - 1)];
                size_t   seq  = cell->seq.load(std::memory_order_acquire);
                intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

                if (diff == 0)
                {
                    if (m_head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        break;
                    }
                }
                else if (diff < 0)
                {
                    // 队列已经满了
                    return false;
                }
                else
                {
                    pos = m_head.load(std::memory_order_relaxed);
                }
            }

            cell->data = data;
            cell->seq.store(pos + 1, std::memory_order_release);
            return true;
        }

        /**
         * @brief 出队
         * @param data 出队元素存放位置
         * @return true 表示成功，false 表示队列为空
         */
        bool dequeue(T& data)
        {
            Cell*  cell;
            size_t pos = m_tail.load(std::memory_order_relaxed);

            for (;;)
            {
                cell          = &m_buffer[pos & (N - 1)];
                size_t   seq  = cell->seq.load(std::memory_order_acquire);
                intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

                if (diff == 0)
                {
                    if (m_tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        break;
                    }
                }
                else if (diff < 0)
                {
                    // 队列为空
                    return false;
                }
                else
                {
                    pos = m_tail.load(std::memory_order_relaxed);
                }
            }

            data = cell->data;
            cell->seq.store(pos + N, std::memory_order_release);
            return true;
        }

        /**
         * @brief 近似队列长度（非精确，只做扩容参考）
         */
        size_t approximateSize() const
        {
            size_t h = m_head.load(std::memory_order_relaxed);
            size_t t = m_tail.load(std::memory_order_relaxed);
            return h >= t ? (h - t) : 0;
        }

    private:

        struct Cell
        {
            std::atomic<size_t> seq;
            T                   data;
        };

        std::array<Cell, N> m_buffer;
        std::atomic<size_t> m_head;
        std::atomic<size_t> m_tail;
    };

    /**
     * @brief 动态扩容线程池（内部任务队列为 LockFreeQueue）
     *
     * 特点：
     *   - 初始线程数可配置
     *   - 最大线程数可配置
     *   - 当任务积压超过 队列长度 > worker数 * 2 时自动扩容
     *   - 使用 condition_variable 做低频睡眠，队列操作本身无锁
     */
    class ThreadPool
    {
    public:

        ThreadPool(size_t initWorkers = 4, size_t maxWorkers = 16);
        ~ThreadPool();

        void enqueue(std::function<void()> job);
        void setMaxWorkers(size_t maxWorkers);

    private:

        void workerLoop();
        void tryGrow();

    private:

        static constexpr size_t QUEUE_CAPACITY = 1024;

        LockFreeQueue<std::function<void()>, QUEUE_CAPACITY> m_queue;

        std::vector<std::thread> m_workers;

        std::mutex              m_waitMutex;    ///< worker 等待用的互斥量（不用于队列）
        std::condition_variable m_waitCv;
        bool                    m_stop { false };

        std::atomic<size_t> m_workerCount { 0 };
        std::atomic<size_t> m_maxWorkers { 16 };
    };

    /**
     * @brief 三层时间轮
     *
     *  Layer0：10ms * 100 = 1 秒
     *  Layer1：1000ms * 60 = 1 分钟
     *  Layer2：60000ms * 60 = 1 小时
     *
     *  时间轮负责“粗略唤醒”，真正精确执行时间仍依赖 TimerTask::nextRunTime。
     */
    class MultiLevelTimeWheel
    {
    public:

        MultiLevelTimeWheel();
        void                                    addTask(const std::shared_ptr<TimerTask>& task, uint64_t delayMs);
        std::vector<std::shared_ptr<TimerTask>> tick(uint64_t baseTickMs);

    private:

        struct WheelLayer
        {
            std::vector<std::vector<std::shared_ptr<TimerTask>>> slots;            ///< 槽数组，每个槽存多个任务
            uint64_t                                             tickMs    = 0;    ///< 每个槽代表时间长度（毫秒）
            uint64_t                                             size      = 0;    ///< 槽数量
            uint64_t                                             index     = 0;    ///< 当前指向槽
            uint64_t                                             elapsedMs = 0;    ///< 已累计时间（毫秒）
        };

        std::vector<WheelLayer> m_layers;
    };

    /**
     * @brief 定时任务管理器（单例）
     *
     * 功能：
     *   - 添加 / 删除 / 暂停 / 恢复任务
     *   - 多层时间轮 + 线程池调度
     *   - 任务优先级（高 / 普通 / 低）
     *   - 任务超时检测（仅报警，不强杀线程）
     *   - 执行耗时统计（次数 / 最大 / 最小 / 平均）
     *   - 任务依赖（A 执行完成后自动激活 B）
     *   - 任务动态修改（时间 / 优先级 / 参数 / 超时）
     */
    class TimerManager
    {
    public:

        static TimerManager& getInstance();

        uint64_t addTask(const std::string&                         name,
                         std::chrono::milliseconds                  delay,
                         std::chrono::milliseconds                  interval,
                         int                                        repeat,
                         std::function<void(std::shared_ptr<void>)> func,
                         std::shared_ptr<void>                      param,
                         TaskPriority                               priority = TaskPriority::NORMAL,
                         std::chrono::milliseconds                  timeout  = std::chrono::milliseconds(0));

        void pauseTask(uint64_t taskId);
        void resumeTask(uint64_t taskId);
        void removeTask(uint64_t taskId);

        void addDependency(uint64_t preTask, uint64_t nextTask);

        TaskStat getTaskStat(uint64_t taskId);
        void     setThreadPoolMaxSize(size_t maxWorkers);

        bool updateTaskTime(uint64_t                  taskId,
                            std::chrono::milliseconds newDelay,
                            std::chrono::milliseconds newInterval,
                            int                       newRepeat);

        bool updateTaskPriority(uint64_t taskId, TaskPriority pri);
        bool updateTaskParam(uint64_t taskId, std::shared_ptr<void> newParam);
        bool updateTaskTimeout(uint64_t taskId, std::chrono::milliseconds newTimeout);

    private:

        TimerManager();
        ~TimerManager();

        void schedulerLoop();
        void execTask(const std::shared_ptr<TimerTask>& task);
        void onTaskFinished(uint64_t id, uint64_t costUs);

    private:

        std::atomic<bool>     m_stop { false };
        std::atomic<uint64_t> m_idGen { 0 };

        ThreadPool          m_pool;
        MultiLevelTimeWheel m_wheel;

        std::unordered_map<uint64_t, std::shared_ptr<TimerTask>> m_taskMap;
        std::unordered_map<uint64_t, std::vector<uint64_t>>      m_depGraph;

        std::mutex  m_mutex;
        std::thread m_thread;
    };

}    // namespace TimerManager_NS
