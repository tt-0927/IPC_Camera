/*
 * @FilePath     : BlockingQueue.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-13 16:41:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 09:42:28
 * @Description  : 阻塞队列
 */
#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace BQ_NS
{
    template<typename T>
    class CBlockingQueue
    {
    public:

        using ReleaseFunction = void (*)(T&);

        explicit CBlockingQueue(size_t nMaxSize, ReleaseFunction releaseFunc = nullptr)
            : m_nMaxSize(nMaxSize),
              m_releaseFunc(releaseFunc)
        {
            while (!m_queue.empty())
            {
                m_queue.pop();
            }
        }

        ~CBlockingQueue()
        {
            clear();
        }

        /**
         * @brief 插入队列
         * @param [T&] value: 数据
         * @param [int] nTimeOutMs: 0-不阻塞 大于0-带超时的阻塞 小于0-死等
         * @return [*] 成功与否
         * @note
         */
        bool push(const T& value, int nTimeOutMs = 0)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_queue.size() >= m_nMaxSize)
            {
                if (nTimeOutMs == 0)
                {
                    return false;
                }
                else if (nTimeOutMs < 0)
                {
                    m_condition.wait(lock, [this] {
                        return m_queue.size() < m_nMaxSize;
                    });
                }
                else
                {
                    std::chrono::milliseconds timeout(nTimeOutMs);
                    if (!m_condition.wait_for(lock, timeout, [this] {
                        return m_queue.size() < m_nMaxSize;
                    }))
                    {
                        return false;
                    }
                }
            }
            m_queue.push(value);
            m_condition.notify_one();
            return true;
        }

        /**
         * @brief 插入队列
         * @param [T&] newValue: 数据
         * @return [*] 成功与否
         * @note 如果满了，会移除一个，再插入
         */
        bool pushOrReplace(const T& newValue)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_queue.size() >= m_nMaxSize)
            {
                if (!m_queue.empty())
                {
                    T& value = m_queue.front();
                    m_queue.pop();
                    if (m_releaseFunc)
                    {
                        m_releaseFunc(value);
                    }
                }
            }
            m_queue.push(newValue);
            m_condition.notify_one();
            return true;
        }

        /**
         * @brief 从队列拿一个数据出来
         * @param [T&] value: 数据
         * @param [int] nTimeOutMs: 0-不阻塞 大于0-带超时的阻塞 小于0-死等
         * @return [*] 成功与否
         * @note
         */
        bool pop(T& value, int nTimeOutMs = 0)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_queue.empty())
            {
                if (nTimeOutMs == 0)
                {
                    return false;
                }
                else if (nTimeOutMs < 0)
                {
                    m_condition.wait(lock, [this] {
                        return !m_queue.empty();
                    });
                }
                else
                {
                    std::chrono::milliseconds timeout(nTimeOutMs);
                    if (!m_condition.wait_for(lock, timeout, [this] {
                        return !m_queue.empty();
                    }))
                    {
                        return false;
                    }
                }
            }
            value = m_queue.front();
            m_queue.pop();
            m_condition.notify_one();
            return true;
        }

        int size()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_queue.size();
        }

        /**
         * @brief 清空队列
         * @return [*]
         * @note 会调用构造函数传进来的释放指针
         */
        void clear()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (!m_queue.empty())
            {
                T& value = m_queue.front();
                m_queue.pop();
                if (m_releaseFunc)
                {
                    m_releaseFunc(value);
                }
            }
            m_condition.notify_all();
        }

    private:

        std::queue<T>           m_queue;
        std::mutex              m_mutex;
        std::condition_variable m_condition;
        size_t                  m_nMaxSize;

        ReleaseFunction m_releaseFunc = nullptr;
    };

}    // namespace BQ_NS