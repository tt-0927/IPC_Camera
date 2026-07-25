/*
 * @FilePath     : DataQueue.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-11 10:43:19
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-13 15:22:13
 * @Description  : 操作带分析的队列和结果队列
 */
#pragma once

#include <iostream>
#include <mutex>
#include <queue>
#include <shared_mutex>

#include "BlError.h"

template<typename T1, typename T2>
class CDataQueue
{
public:

    typedef void (*FreePendingDataFunc)(T1);
    typedef void (*FreeResultDataFunc)(T2);

    CDataQueue(FreePendingDataFunc freeFunc1, FreeResultDataFunc freeFunc2, int nMaxSize = 500)
        : m_freePendingDataFunc(freeFunc1),
          m_freeResultDataFunc(freeFunc2),
          m_nMaxSize(nMaxSize)
    {
        /* 初始化队列 */
        clear_pendingQueue();
        clear_resultQueue();
    }

    ~CDataQueue()
    {
        /* 清空队列 */
        clear_pendingQueue();
        clear_resultQueue();
    }

    /**
     * @brief 插入待处理数据队列
     * @param [T1] pendingData: 待处理数据
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note 内部会拷贝数据
     */
    BlError_E push_pendingQueue(T1 pendingData)
    {
        /* 写入共享数据的操作 */
        std::unique_lock<std::shared_mutex> lock(m_pendingSharedMutex);

        if (m_pendingQueue.size() >= m_nMaxSize)
        {
            T1 pendingOutData;

            /* 从队列中拿一个数据出来 */
            pendingOutData = m_pendingQueue.front();
            m_pendingQueue.pop();

            if (m_freePendingDataFunc)
            {
                m_freePendingDataFunc(pendingOutData);
            }
        }

        m_pendingQueue.push(pendingData);
        return OK;
    }

    /**
     * @brief 插入待处理数据队列
     * @param [T1] pendingData: 待处理数据
     * @param [int] nBlockTimeMs: 阻塞时长，单位/ms，-1:一直阻塞
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note 内部会拷贝数据,阻塞的插入
     */
    BlError_E push_pendingQueue(T1 pendingData, int nBlockTimeMs)
    {
        int nNumber = 0;

        std::chrono::milliseconds sleepDuration(1);

        while (1)
        {
            {
                /* 写入共享数据的操作 */
                std::unique_lock<std::shared_mutex> lock(m_pendingSharedMutex);

                if (m_pendingQueue.size() < m_nMaxSize)
                {
                    m_pendingQueue.push(pendingData);
                    break;
                }

                if (nBlockTimeMs != -1 && nNumber >= nBlockTimeMs)
                {
                    return NOK;
                }
            }

            std::this_thread::sleep_for(sleepDuration);
            nNumber++;
        }

        return OK;
    }

    /**
     * @brief 获取待处理数据
     * @param [T1&] pendingOutData: 待处理数据
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note pImageData使用结束后，需要释放
     */
    BlError_E pop_pendingQueue(T1& pendingOutData)
    {
        /* 读取共享数据的操作 */
        std::shared_lock<std::shared_mutex> lock(m_pendingSharedMutex);

        if (!m_pendingQueue.empty())
        {
            /* 从队列中拿一个数据出来 */
            pendingOutData = m_pendingQueue.front();
            m_pendingQueue.pop();

            return OK;
        }

        return NOK;
    }

    /**
     * @brief 判断队列是否为空
     * @return [*] 是否为空
     * @note
     */
    bool isEmpty_pendingQueue()
    {
        /* 读取共享数据的操作 */
        std::shared_lock<std::shared_mutex> lock(m_pendingSharedMutex);

        return m_pendingQueue.empty();
    }

    /**
     * @brief 获取队列长度
     * @return [*] 是否为空
     * @note
     */
    int getSize_pendingQueue()
    {
        /* 读取共享数据的操作 */
        std::shared_lock<std::shared_mutex> lock(m_pendingSharedMutex);

        return m_pendingQueue.size();
    }

    /**
     * @brief 清空队列
     * @return [*]
     * @note
     */
    void clear_pendingQueue()
    {
        /* 写入共享数据的操作 */
        std::unique_lock<std::shared_mutex> lock(m_pendingSharedMutex);

        /* 清空队列 */
        while (!m_pendingQueue.empty())
        {
            T1 stInfo;

            /* 从队列中拿一个数据出来 */
            stInfo = m_pendingQueue.front();
            if (m_freePendingDataFunc)
            {
                m_freePendingDataFunc(stInfo);
            }
            m_pendingQueue.pop();
        }
    }

    /**
     * @brief 插入处理结果数据队列
     * @param [T2] resultData: 结果数据
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E push_resultQueue(T2 resultData)
    {
        /* 写入共享数据的操作 */
        std::unique_lock<std::shared_mutex> lock(m_resultSharedMutex);

        if (m_resultQueue.size() >= m_nMaxSize)
        {
            m_resultQueue.pop();
        }

        m_resultQueue.push(resultData);

        return OK;
    }

    /**
     * @brief 获取一个处理结果
     * @param [T2&] resultOutData: 结果数据
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E pop_resultQueue(T2& resultOutData)
    {
        /* 读取共享数据的操作 */
        std::shared_lock<std::shared_mutex> lock(m_resultSharedMutex);

        if (!m_resultQueue.empty())
        {
            /* 从队列中拿一个数据出来 */
            resultOutData = m_resultQueue.front();
            m_resultQueue.pop();

            return OK;
        }

        return NOK;
    }

    /**
     * @brief 判断队列是否为空
     * @return [*] 是否为空
     * @note
     */
    bool isEmpty_resultQueue()
    {
        /* 读取共享数据的操作 */
        std::shared_lock<std::shared_mutex> lock(m_resultSharedMutex);

        return m_resultQueue.empty();
    }

    /**
     * @brief 获取队列长度
     * @return [*] 是否为空
     * @note
     */
    int getSize_resultQueue()
    {
        /* 读取共享数据的操作 */
        std::shared_lock<std::shared_mutex> lock(m_resultSharedMutex);

        return m_resultQueue.size();
    }

    /**
     * @brief 清空队列
     * @return [*]
     * @note
     */
    void clear_resultQueue()
    {
        /* 写入共享数据的操作 */
        std::unique_lock<std::shared_mutex> lock(m_resultSharedMutex);

        /* 清空队列 */
        while (!m_resultQueue.empty())
        {
            T2 stInfo;

            /* 从队列中拿一个数据出来 */
            stInfo = m_resultQueue.front();
            if (m_freeResultDataFunc)
            {
                m_freeResultDataFunc(stInfo);
            }

            m_resultQueue.pop();
        }
    }

private:

    /* 待处理数据队列操作读写锁*/
    std::shared_mutex m_pendingSharedMutex;
    /* 待处理数据链表 */
    std::queue<T1>    m_pendingQueue;

    /* 待处理数据队列操作读写锁 */
    std::shared_mutex m_resultSharedMutex;
    /* 处理结果数据链表 */
    std::queue<T2>    m_resultQueue;

    int m_nMaxSize = 0;

    /* 释放数据函数 */
    FreePendingDataFunc m_freePendingDataFunc;
    FreeResultDataFunc  m_freeResultDataFunc;
};
