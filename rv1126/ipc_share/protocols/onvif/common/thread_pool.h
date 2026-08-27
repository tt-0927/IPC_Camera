/*** 
 * @FilePath     : thread_pool.h
 * @Author       : cyc
 * @Date         : 2025-09-11 15:19:43
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-11 16:09:40
 * @Description  : ONVIF专用线程池类实现
 */


 #pragma once

 #include <thread>
 #include <vector>
 #include <queue>
 #include <mutex>
 #include <condition_variable>
 #include <atomic>
 #include <functional>
 #include <netinet/in.h>
 
 // ONVIF连接处理任务
 struct OnvifConnectionTask {
     int socket;
     struct sockaddr_in client_addr;
     
     OnvifConnectionTask(int sock, const struct sockaddr_in& addr) 
         : socket(sock), client_addr(addr) {}
 };
 
 class OnvifThreadPool {
 public:
     using TaskFunction = std::function<void(OnvifConnectionTask)>;
     
     /**
      * @brief 构造函数
      * @param pool_size 线程池大小
      * @param max_queue_size 最大任务队列大小
      * @param task_handler 任务处理函数
      */
     OnvifThreadPool(size_t pool_size, size_t max_queue_size, TaskFunction task_handler);
     
     /**
      * @brief 析构函数
      */
     ~OnvifThreadPool();
     
     /**
      * @brief 提交ONVIF连接任务
      * @param task ONVIF连接任务
      * @return 是否成功提交
      */
     bool enqueue_task(const OnvifConnectionTask& task);
     
     /**
      * @brief 停止线程池
      */
     void stop();
     
     /**
      * @brief 获取当前队列中任务数量
      * @return 任务数量
      */
     size_t get_queue_size() const;
     
     /**
      * @brief 检查线程池是否已停止
      * @return 是否已停止
      */
     bool is_stopped() const;
 
 private:
     /**
      * @brief 工作线程函数
      */
     void worker_thread();
     
     std::vector<std::thread> m_workers;              // 工作线程
     std::queue<OnvifConnectionTask> m_tasks;         // 任务队列
     mutable std::mutex m_queue_mutex;                // 队列互斥锁
     std::condition_variable m_condition;             // 条件变量
     std::atomic<bool> m_stop_flag;                   // 停止标志
     size_t m_max_queue_size;                         // 最大队列大小
     TaskFunction m_task_handler;                     // 任务处理函数
 };
 