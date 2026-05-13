#include "TimerManager.hpp"

#include <future>

namespace TimerManager_NS
{

    using namespace std;
    using namespace std::chrono;

    /******************************************************
     * ThreadPool 实现
     ******************************************************/

    ThreadPool::ThreadPool(size_t initWorkers, size_t maxWorkers)
    {
        m_maxWorkers = maxWorkers;
        if (initWorkers == 0)
        {
            initWorkers = 1;
        }
        if (initWorkers > maxWorkers)
        {
            initWorkers = maxWorkers;
        }

        for (size_t i = 0; i < initWorkers; ++i)
        {
            m_workers.emplace_back(&ThreadPool::workerLoop, this);
            ++m_workerCount;
        }

        TM_LOGI("线程池初始化: init=%zu max=%zu", initWorkers, maxWorkers);
    }

    ThreadPool::~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lk(m_waitMutex);
            m_stop = true;
        }
        m_waitCv.notify_all();

        for (auto& t : m_workers)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
        TM_LOGI("线程池已销毁");
    }

    void ThreadPool::enqueue(std::function<void()> job)
    {
        if (!job)
        {
            return;
        }

        // 使用无锁队列入队，如果队列满了就短暂让出 CPU 重试
        while (!m_queue.enqueue(job))
        {
            std::this_thread::yield();
        }

        tryGrow();    // 根据队列长度尝试扩容
        m_waitCv.notify_one();
    }

    void ThreadPool::setMaxWorkers(size_t maxWorkers)
    {
        m_maxWorkers.store(maxWorkers, std::memory_order_relaxed);
        TM_LOGI("线程池最大线程数调整为: %zu", maxWorkers);
    }

    void ThreadPool::workerLoop()
    {
        while (true)
        {
            std::function<void()> job;

            // 优先尝试无锁出队
            if (!m_queue.dequeue(job))
            {
                // 当前没有任务，进入短暂等待，避免空转占满 CPU
                std::unique_lock<std::mutex> lk(m_waitMutex);
                if (m_stop)
                {
                    return;
                }

                if (!m_queue.dequeue(job))
                {
                    m_waitCv.wait_for(lk, std::chrono::milliseconds(5));
                    if (m_stop)
                    {
                        return;
                    }
                    continue;
                }
            }

            if (m_stop)
            {
                return;
            }

            job();
        }
    }

    void ThreadPool::tryGrow()
    {
        size_t curWorkers = m_workerCount.load(std::memory_order_relaxed);
        size_t maxWorkers = m_maxWorkers.load(std::memory_order_relaxed);
        size_t qSize      = m_queue.approximateSize();

        // 简单策略：队列长度 > 当前线程数 * 2 时扩容
        if (qSize > curWorkers * 2 && curWorkers < maxWorkers)
        {
            m_workers.emplace_back(&ThreadPool::workerLoop, this);
            m_workerCount.fetch_add(1, std::memory_order_relaxed);
            TM_LOGI("线程池自动扩容: 当前线程数=%zu 队列长度=%zu", m_workerCount.load(), qSize);
        }
    }

    /******************************************************
     * MultiLevelTimeWheel 实现
     ******************************************************/

    MultiLevelTimeWheel::MultiLevelTimeWheel()
    {
        // Layer0: 10ms * 100 = 1s
        WheelLayer l0;
        l0.tickMs    = 10;
        l0.size      = 100;
        l0.index     = 0;
        l0.elapsedMs = 0;
        l0.slots.resize(l0.size);

        // Layer1: 1000ms * 60 = 1min
        WheelLayer l1;
        l1.tickMs    = 1000;
        l1.size      = 60;
        l1.index     = 0;
        l1.elapsedMs = 0;
        l1.slots.resize(l1.size);

        // Layer2: 60000ms * 60 = 1h
        WheelLayer l2;
        l2.tickMs    = 60000;
        l2.size      = 60;
        l2.index     = 0;
        l2.elapsedMs = 0;
        l2.slots.resize(l2.size);

        m_layers = { l0, l1, l2 };

        TM_LOGI("时间轮初始化完成: L0=%llu*%llu L1=%llu*%llu L2=%llu*%llu",
                (unsigned long long)l0.tickMs, (unsigned long long)l0.size,
                (unsigned long long)l1.tickMs, (unsigned long long)l1.size,
                (unsigned long long)l2.tickMs, (unsigned long long)l2.size);
    }

    void MultiLevelTimeWheel::addTask(const std::shared_ptr<TimerTask>& task, uint64_t delayMs)
    {
        if (!task)
        {
            return;
        }

        if (delayMs == 0)
        {
            delayMs = 1;
        }

        size_t level = 0;
        if (delayMs < m_layers[0].tickMs * m_layers[0].size)
        {
            level = 0;
        }
        else if (delayMs < m_layers[1].tickMs * m_layers[1].size)
        {
            level = 1;
        }
        else
        {
            level = 2;
        }

        auto& layer = m_layers[level];

        uint64_t ticks = delayMs / layer.tickMs;
        if (ticks == 0)
        {
            ticks = 1;
        }
        if (ticks >= layer.size)
        {
            ticks = layer.size - 1;
        }

        uint64_t slot = (layer.index + ticks) % layer.size;
        layer.slots[slot].push_back(task);
    }

    std::vector<std::shared_ptr<TimerTask>> MultiLevelTimeWheel::tick(uint64_t baseTickMs)
    {
        std::vector<std::shared_ptr<TimerTask>> ready;

        for (auto& layer : m_layers)
        {
            layer.elapsedMs += baseTickMs;

            while (layer.elapsedMs >= layer.tickMs)
            {
                layer.elapsedMs -= layer.tickMs;
                layer.index      = (layer.index + 1) % layer.size;

                auto& bucket = layer.slots[layer.index];
                ready.insert(ready.end(), bucket.begin(), bucket.end());
                bucket.clear();
            }
        }

        return ready;
    }

    /******************************************************
     * TimerManager 实现
     ******************************************************/

    static constexpr auto TIMER_BASE_TICK = std::chrono::milliseconds(10);

    TimerManager& TimerManager::getInstance()
    {
        static TimerManager inst;
        return inst;
    }

    TimerManager::TimerManager()
        : m_pool(4, 16)
    {
        m_thread = std::thread(&TimerManager::schedulerLoop, this);
        TM_LOGI("TimerManager 调度线程已启动");
    }

    TimerManager::~TimerManager()
    {
        m_stop.store(true, std::memory_order_relaxed);
        if (m_thread.joinable())
        {
            m_thread.join();
        }
        TM_LOGI("TimerManager 已析构");
    }

    uint64_t TimerManager::addTask(const std::string&                         name,
                                   milliseconds                               delay,
                                   milliseconds                               interval,
                                   int                                        repeat,
                                   std::function<void(std::shared_ptr<void>)> func,
                                   std::shared_ptr<void>                      param,
                                   TaskPriority                               priority,
                                   milliseconds                               timeout)
    {
        if (!func)
        {
            TM_LOGW("addTask 失败: 回调为空 name=%s", name.c_str());
            return 0;
        }

        auto task         = std::make_shared<TimerTask>();
        task->taskId      = ++m_idGen;
        task->name        = name;
        task->nextRunTime = steady_clock::now() + delay;
        task->interval    = interval;
        task->repeatCount = repeat;
        task->status      = TaskStatus::RUNNING;
        task->priority    = priority;
        task->func        = func;
        task->userData    = param;
        task->timeout     = timeout;

        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_taskMap[task->taskId] = task;
            m_wheel.addTask(task, static_cast<uint64_t>(delay.count()));
        }

        TM_LOGI("添加任务: id=%llu name=%s delay=%lldms interval=%lldms repeat=%d pri=%d timeout=%lldms",
                (unsigned long long)task->taskId,
                name.c_str(),
                (long long)delay.count(),
                (long long)interval.count(),
                repeat,
                (int)priority,
                (long long)timeout.count());

        return task->taskId;
    }

    void TimerManager::pauseTask(uint64_t taskId)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto                        it = m_taskMap.find(taskId);
        if (it != m_taskMap.end())
        {
            it->second->status = TaskStatus::PAUSED;
            TM_LOGI("暂停任务: id=%llu name=%s", (unsigned long long)taskId, it->second->name.c_str());
        }
    }

    void TimerManager::resumeTask(uint64_t taskId)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto                        it = m_taskMap.find(taskId);
        if (it == m_taskMap.end())
        {
            return;
        }

        auto& t        = it->second;
        t->status      = TaskStatus::RUNNING;
        t->nextRunTime = steady_clock::now();
        m_wheel.addTask(t, 1);
        TM_LOGI("恢复任务: id=%llu name=%s", (unsigned long long)taskId, t->name.c_str());
    }

    void TimerManager::removeTask(uint64_t taskId)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto                        it = m_taskMap.find(taskId);
        if (it != m_taskMap.end())
        {
            it->second->status = TaskStatus::STOPPED;
            TM_LOGI("删除任务: id=%llu name=%s", (unsigned long long)taskId, it->second->name.c_str());
            m_taskMap.erase(it);
        }
    }

    void TimerManager::addDependency(uint64_t preTask, uint64_t nextTask)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto                        itPre  = m_taskMap.find(preTask);
        auto                        itNext = m_taskMap.find(nextTask);
        if (itPre == m_taskMap.end() || itNext == m_taskMap.end())
        {
            TM_LOGW("添加依赖失败: pre=%llu next=%llu", (unsigned long long)preTask, (unsigned long long)nextTask);
            return;
        }

        itNext->second->status = TaskStatus::PAUSED;
        m_depGraph[preTask].push_back(nextTask);

        TM_LOGI("添加任务依赖: pre=%llu(%s) -> next=%llu(%s)",
                (unsigned long long)preTask, itPre->second->name.c_str(),
                (unsigned long long)nextTask, itNext->second->name.c_str());
    }

    TaskStat TimerManager::getTaskStat(uint64_t taskId)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        TaskStat                    st;
        auto                        it = m_taskMap.find(taskId);
        if (it != m_taskMap.end())
        {
            st = it->second->stat;
        }
        return st;
    }

    void TimerManager::setThreadPoolMaxSize(size_t maxWorkers)
    {
        m_pool.setMaxWorkers(maxWorkers);
    }

    /******************** 动态修改能力 ********************/

    bool TimerManager::updateTaskTime(uint64_t     taskId,
                                      milliseconds newDelay,
                                      milliseconds newInterval,
                                      int          newRepeat)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto                        it = m_taskMap.find(taskId);
        if (it == m_taskMap.end())
        {
            return false;
        }

        auto& t = it->second;
        if (t->status == TaskStatus::STOPPED)
        {
            return false;
        }

        t->interval    = newInterval;
        t->repeatCount = newRepeat;
        t->nextRunTime = steady_clock::now() + newDelay;
        m_wheel.addTask(t, static_cast<uint64_t>(newDelay.count()));

        TM_LOGI("修改任务时间: id=%llu delay=%lldms interval=%lldms repeat=%d",
                (unsigned long long)taskId,
                (long long)newDelay.count(),
                (long long)newInterval.count(),
                newRepeat);
        return true;
    }

    bool TimerManager::updateTaskPriority(uint64_t taskId, TaskPriority pri)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto                        it = m_taskMap.find(taskId);
        if (it == m_taskMap.end())
        {
            return false;
        }

        it->second->priority = pri;
        TM_LOGI("修改任务优先级: id=%llu pri=%d", (unsigned long long)taskId, (int)pri);
        return true;
    }

    bool TimerManager::updateTaskParam(uint64_t taskId, std::shared_ptr<void> newParam)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto                        it = m_taskMap.find(taskId);
        if (it == m_taskMap.end())
        {
            return false;
        }

        it->second->userData = newParam;
        TM_LOGI("修改任务参数: id=%llu", (unsigned long long)taskId);
        return true;
    }

    bool TimerManager::updateTaskTimeout(uint64_t taskId, milliseconds newTimeout)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto                        it = m_taskMap.find(taskId);
        if (it == m_taskMap.end())
        {
            return false;
        }

        it->second->timeout = newTimeout;
        TM_LOGI("修改任务超时: id=%llu timeout=%lldms",
                (unsigned long long)taskId, (long long)newTimeout.count());
        return true;
    }

    /******************** 调度线程 ********************/

    void TimerManager::schedulerLoop()
    {
        while (!m_stop.load(std::memory_order_relaxed))
        {
            std::this_thread::sleep_for(TIMER_BASE_TICK);

            std::vector<std::shared_ptr<TimerTask>> ready;
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                ready = m_wheel.tick(static_cast<uint64_t>(TIMER_BASE_TICK.count()));
            }

            if (ready.empty())
            {
                continue;
            }

            // 按优先级排序
            std::sort(ready.begin(), ready.end(),
                      [](const std::shared_ptr<TimerTask>& a,
                         const std::shared_ptr<TimerTask>& b) {
                if (!a || !b)
                {
                    return false;
                }
                return static_cast<int>(a->priority) < static_cast<int>(b->priority);
            });

            for (auto& t : ready)
            {
                if (!t)
                {
                    continue;
                }

                std::shared_ptr<TimerTask> task;
                {
                    std::lock_guard<std::mutex> lk(m_mutex);
                    auto                        it = m_taskMap.find(t->taskId);
                    if (it == m_taskMap.end())
                    {
                        continue;
                    }
                    task = it->second;

                    if (task->status == TaskStatus::STOPPED)
                    {
                        m_taskMap.erase(it);
                        continue;
                    }

                    if (task->status == TaskStatus::PAUSED)
                    {
                        m_wheel.addTask(task, 50);
                        continue;
                    }

                    auto now = steady_clock::now();
                    if (now < task->nextRunTime)
                    {
                        auto diff = duration_cast<milliseconds>(task->nextRunTime - now).count();
                        if (diff <= 0)
                        {
                            diff = 1;
                        }
                        m_wheel.addTask(task, static_cast<uint64_t>(diff));
                        continue;
                    }
                }

                execTask(task);
            }
        }
    }

    /******************** 任务执行 + 统计 + 超时 ********************/

    void TimerManager::execTask(const std::shared_ptr<TimerTask>& t)
    {
        if (!t || !t->func)
        {
            return;
        }

        uint64_t taskId  = t->taskId;
        auto     func    = t->func;
        auto     param   = t->userData;
        auto     timeout = t->timeout;

        auto pkg = std::make_shared<std::packaged_task<void()>>(
            [taskId, func, param]() {
            auto start = steady_clock::now();
            try
            {
                func(param);
            }
            catch (const std::exception& e)
            {
                TM_LOGE("任务执行异常: id=%llu what=%s",
                        (unsigned long long)taskId, e.what());
            }
            catch (...)
            {
                TM_LOGE("任务执行异常: id=%llu unknown", (unsigned long long)taskId);
            }
            auto end    = steady_clock::now();
            auto costUs = duration_cast<microseconds>(end - start).count();
            TimerManager::getInstance().onTaskFinished(taskId, static_cast<uint64_t>(costUs));
        });

        std::future<void> fut = pkg->get_future();
        m_pool.enqueue([pkg]() {
            (*pkg)();
        });

        // 超时监控线程（不强制终止，仅日志提示）
        std::thread([f = std::move(fut), timeout, taskId]() mutable {
            if (timeout.count() > 0)
            {
                if (f.wait_for(timeout) == std::future_status::timeout)
                {
                    TM_LOGW("任务执行超时: id=%llu timeout=%lldms",
                            (unsigned long long)taskId, (long long)timeout.count());
                }
                else
                {
                    try
                    {
                        f.get();
                    }
                    catch (...)
                    {
                    }
                }
            }
            else
            {
                try
                {
                    f.get();
                }
                catch (...)
                {
                }
            }
        }).detach();

        // 安排下一次执行（重复调度）
        {
            std::lock_guard<std::mutex> lk(m_mutex);

            auto it = m_taskMap.find(taskId);
            if (it == m_taskMap.end())
            {
                return;
            }
            auto& task = it->second;

            if (task->repeatCount == 1)
            {
                task->status = TaskStatus::STOPPED;
                m_taskMap.erase(it);
                TM_LOGI("任务完成并移除: id=%llu name=%s",
                        (unsigned long long)taskId, task->name.c_str());
                return;
            }

            if (task->repeatCount > 1)
            {
                --task->repeatCount;
            }

            if (task->status != TaskStatus::STOPPED)
            {
                task->nextRunTime = steady_clock::now() + task->interval;
                auto delayMs      = duration_cast<milliseconds>(task->interval).count();
                if (delayMs <= 0)
                {
                    delayMs = 1;
                }
                m_wheel.addTask(task, static_cast<uint64_t>(delayMs));
            }
        }
    }

    void TimerManager::onTaskFinished(uint64_t id, uint64_t costUs)
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        auto it = m_taskMap.find(id);
        if (it != m_taskMap.end())
        {
            auto& st = it->second->stat;
            st.runCount++;
            st.lastCostUs   = costUs;
            st.totalCostUs += costUs;
            if (costUs > st.maxCostUs)
            {
                st.maxCostUs = costUs;
            }
            if (costUs < st.minCostUs)
            {
                st.minCostUs = costUs;
            }

#if 0
            TM_LOGT("任务执行完成: id=%llu name=%s cost=%lluus runCount=%llu",
                    (unsigned long long)id,
                    it->second->name.c_str(),
                    (unsigned long long)costUs,
                    (unsigned long long)st.runCount);
#endif
        }

        // 处理依赖：pre -> [next...]
        auto depIt = m_depGraph.find(id);
        if (depIt != m_depGraph.end())
        {
            for (auto nextId : depIt->second)
            {
                auto itNext = m_taskMap.find(nextId);
                if (itNext != m_taskMap.end())
                {
                    auto& nextTask = itNext->second;
                    if (nextTask->status != TaskStatus::STOPPED)
                    {
                        nextTask->status      = TaskStatus::RUNNING;
                        nextTask->nextRunTime = steady_clock::now();
                        m_wheel.addTask(nextTask, 1);
                        TM_LOGI("依赖触发: pre=%llu -> next=%llu(%s)",
                                (unsigned long long)id,
                                (unsigned long long)nextId,
                                nextTask->name.c_str());
                    }
                }
            }
            m_depGraph.erase(depIt);
        }
    }

}    // namespace TimerManager_NS
