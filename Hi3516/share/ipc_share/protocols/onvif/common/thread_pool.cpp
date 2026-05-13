/*** 
 * @FilePath     : thread_pool.cpp
 * @Author       : cyc
 * @Date         : 2025-09-11 15:20:00
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-11 17:04:39
 * @Description  : ONVIF专用线程池类实现
 */


 #include "thread_pool.h"
 #include "dlog.h"
 #include <pthread.h>
 #include <unistd.h>
 
 OnvifThreadPool::OnvifThreadPool(size_t pool_size, size_t max_queue_size, TaskFunction task_handler)
     : m_stop_flag(false)
     , m_max_queue_size(max_queue_size)
     , m_task_handler(task_handler)
 {
    if (pool_size == 0) {
        throw std::invalid_argument("Thread pool size cannot be zero");
    }
    
    if (max_queue_size == 0) {
        throw std::invalid_argument("Max queue size cannot be zero");
    }
    
    if (!task_handler) {
        throw std::invalid_argument("Task handler cannot be null");
    }
    
    /* 创建工作线程 */ 
    for (size_t i = 0; i < pool_size; ++i) 
    {
        m_workers.emplace_back(&OnvifThreadPool::worker_thread, this);
    }
     
 }
 
 OnvifThreadPool::~OnvifThreadPool()
 {
     stop();
 }
 
 bool OnvifThreadPool::enqueue_task(const OnvifConnectionTask& task)
 {
     {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        
        /* 检查是否已停止或队列已满 */ 
        if (m_stop_flag || m_tasks.size() >= m_max_queue_size) {
            if (m_stop_flag) {
                dlog_debug("OnvifThreadPool: Cannot enqueue task, pool is stopped");
            } else {
                dlog_warn("OnvifThreadPool: Queue is full (%zu/%zu), dropping connection", 
                        m_tasks.size(), m_max_queue_size);
            }
            return false;
        }
        
        m_tasks.push(task);
     }
     
     m_condition.notify_one();
     return true;
 }
 
 void OnvifThreadPool::stop()
 {
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        if (m_stop_flag) {
            return; // 已经停止
        }
        m_stop_flag = true;
    }
    
    dlog_info("OnvifThreadPool: Stopping thread pool...");
    
    /* 通知所有工作线程 */ 
    m_condition.notify_all();
    
    /* 等待所有工作线程结束 */ 
    for (std::thread &worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    /* 关闭队列中剩余的socket */ 
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        while (!m_tasks.empty()) {
            OnvifConnectionTask task = m_tasks.front();
            m_tasks.pop();
            close(task.socket);
        }
    }
    
    dlog_info("OnvifThreadPool: All threads stopped, remaining tasks cleared");
 }
 
 size_t OnvifThreadPool::get_queue_size() const
 {
     std::unique_lock<std::mutex> lock(m_queue_mutex);
     return m_tasks.size();
 }
 
 bool OnvifThreadPool::is_stopped() const
 {
     return m_stop_flag.load();
 }
 
 void OnvifThreadPool::worker_thread()
 {
    /* 设置线程名称 */ 
    pthread_setname_np(pthread_self(), "OnvifWorker");
    
    dlog_debug("OnvifThreadPool: Worker thread started");
    
    while (!m_stop_flag) {
        OnvifConnectionTask task(0, {});
        bool has_task = false;
        
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            
            /* 等待任务或停止信号 */ 
            m_condition.wait(lock, [this]() { 
                return m_stop_flag || !m_tasks.empty(); 
            });
            
            /* 如果停止标志设置且无任务，退出 */ 
            if (m_stop_flag && m_tasks.empty()) 
            {
                break;
            }
            
            /* 获取任务 */ 
            if (!m_tasks.empty()) 
            {
                task = m_tasks.front();
                m_tasks.pop();
                has_task = true;
            }
        }
        
        /* 执行任务 */ 
        if (has_task && task.socket > 0) 
        {
        m_task_handler(task);
        }
    }

 }
 