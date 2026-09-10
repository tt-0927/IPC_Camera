/*
 * @Author       : EasonLu
 * @Date         : 2023-06-29 09:43:42
 * @LastEditors  : EasonLu
 * @LastEditTime : 2024-08-07 11:20:27
 * @FilePath     : BlockingQueue.hpp
 * @Description  : 线程安全的阻塞队列
 * @Note         : 退出时需要调用Exit()方法，否则会导致读取操作阻塞
 */
#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>

/* 阻塞等待标记 */
#ifndef BLOCKING_WAIT
#define BLOCKING_WAIT -1
#endif

template <typename T>
class CBlockingQueue
{
public:
    using ReleaseFunction = void (*)(T &);

    CBlockingQueue(int nMaxSize,
                  ReleaseFunction releaseFunc = nullptr)
        : m_bExitFlag(false),
          m_nMaxSize(nMaxSize)
    {
    }
    ~CBlockingQueue() = default;

    /* 禁用拷贝和赋值 */
    CBlockingQueue(const CBlockingQueue &) = delete;
    CBlockingQueue &operator=(const CBlockingQueue &) = delete;

    /**
     * @brief  往阻塞队列中添加数据
     * @param  [T] item 队列数据类型
     * @param  [bool] bForce 强制推进队列，超出队列上限时替代队列中最前面的元素
     * @return [bool] 添加队列成功则返回true
     * @author EasonLu
     * @note
     */
    bool push(const T &item, bool bForce = false)
    {
        std::unique_lock<std::mutex> lock(m_mutexData);
        if (m_queueData.size() + 1 >= m_nMaxSize)
        {
            /* 超过队列最大值则加入失败 */
            if (!bForce)
            {
                return false;
            }
            else
            {
                /* 开启强制推送，则默认移除队列中首位元素 */
                m_queueData.pop();
            }
        }
        m_queueData.push(item);
        /* 通知正在等待的线程有新的数据可用 */
        m_condition.notify_one();
        return true;
    }

    /**
     * @brief
     * @param  [T] item - 队列中的数据类型
     * @param  [int] nTimeOut - 超时时间（毫秒）选项，不设置默认为阻塞
     * @return [*]
     * @author EasonLu
     * @note
     */
    bool pop(T &item, int nTimeOut = BLOCKING_WAIT)
    {
        std::unique_lock<std::mutex> lock(m_mutexData);
        if (BLOCKING_WAIT == nTimeOut)
        {
            /* 等待队列非空或退出标志被设置 */
            m_condition.wait(lock, [this]()
                             { return !m_queueData.empty() || m_bExitFlag; });
        }
        else if (nTimeOut > 0)
        {
            /* 等待队列非空或退出标志被设置，或者直到超时 */
            m_condition.wait_for(lock, std::chrono::milliseconds(nTimeOut), [this]()
                                 { return !m_queueData.empty() || m_bExitFlag; });
        }
        else
        {
            /* 未定义的nTimeOut */
            return false;
        }
        if (m_bExitFlag)
        {
            /* 退出标志被设置，读取操作应该退出 */
            return false;
        }

        /* 适配超时过后，队列中还没有数据 */
        if (m_queueData.empty())
        {
            return false;
        }

        item = m_queueData.front();
        m_queueData.pop();
        return true;
    }

    /**
     * @brief  退出阻塞队列
     * @return [*]
     * @author EasonLu
     * @note   以防当前阻塞队列中还在阻塞当中，无法退出
     */
    void exit()
    {
        std::unique_lock<std::mutex> lock(m_mutexData);
        /* 清空队列 */
        while (!m_queueData.empty())
        {
            auto item = m_queueData.front();
            m_queueData.pop();
            if (m_releaseFunc)
            {
                m_releaseFunc(item);
            }
        }
        /* 设置退出标志 */
        m_bExitFlag = true;
        /* 通知所有等待的线程退出阻塞状态 */
        m_condition.notify_all();
    }

    /**
     * @brief  清空阻塞队列
     * @return [*]
     * @author EasonLu
     * @note
     */
    void clear()
    {
        std::unique_lock<std::mutex> lock(m_mutexData);
        while (!m_queueData.empty())
        {
            auto item = m_queueData.front();
            m_queueData.pop();
            if (m_releaseFunc)
            {
                m_releaseFunc(item);
            }
        }
        /* 通知所有等待的线程退出阻塞状态 */
        m_condition.notify_all();
    }

private:
    std::queue<T> m_queueData;               /* 存储数据的队列 */
    std::mutex m_mutexData;                  /* 互斥锁，保护队列的访问 */
    std::condition_variable m_condition;     /* 条件变量，用于阻塞读取操作 */
    std::atomic_bool m_bExitFlag;            /* 退出标志，用于控制读取操作的退出 */
    long unsigned int m_nMaxSize;            /* 队列最大长度 */
    ReleaseFunction m_releaseFunc = nullptr; /* 自定义释放函数，内部clear时会调用 */
};

#if 0 /* 例程 */
#include <thread>
int main()
{
    CBlockingQueue<int> queue(20);

    /* 启动一个读取线程 */
    std::thread reader([&queue]()
                       {
        int item;
        while (queue.pop(item)) {
            std::cout << "Read item: " << item << std::endl;
        }
        std::cout << "Reader thread exited." << std::endl; });

    /* 将一些数据推入队列 */
    for (int i = 0; i < 10; ++i)
    {
        queue.push(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /* 手动退出读取操作 */
    queue.exit();

    /* 等待读取线程退出 */
    reader.join();

    return 0;
}

#endif