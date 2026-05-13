/**
 * @file Heartbeat.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2025-01-07
 *
 * @brief 心跳监测类
 */
#pragma once

#include <chrono>
#include <mutex>
#include <thread>
#include <iostream>
#include "IOBase.h"
#include "NetDefine.h"
#include <cstring>

class Heartbeat
{
public:
    using StautsCallback = std::function<void(bool)>; // 状态改变的回调函数类型

    // 构造函数，初始化心跳监测类
    Heartbeat(Net::IOBase *pHandle, int heartbeatInterval)
        : m_handle(pHandle), m_heartbeatInterval(heartbeatInterval), m_bExit(false), m_bHeartbeatOk(true)
    {
        m_buffer.resize(2);
        m_buffer[0] = 'h';
        m_buffer[1] = '\0';
    }
    // 析构函数，释放资源
    ~Heartbeat()
    {
        stop();
    }
    // 设置要发送的消息的nActionCode
    void set_actionCode(int nActionCode)
    {
        m_nActionCode = nActionCode;
    }
    // 设置要发送的消息内容
    void set_messge(const void *pData, size_t nLength)
    {
        if (pData == nullptr || nLength == 0)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_heartbeatMutex);
        m_buffer.resize(nLength);
        memcpy(m_buffer.data(), pData, nLength);
    }

    // 设置状态改变的回调函数
    void set_observer(StautsCallback callback)
    {
        m_statusObserver = callback;
    }
    // 启动心跳监测线程
    void start()
    {
        m_lastHeartbeatTime = std::chrono::steady_clock::now();
        m_thread = std::thread(&Heartbeat::monitor, this);
    }

    // 停止心跳监测线程
    void stop()
    {
        m_bExit = true;
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    // 更新心跳响应，重置心跳状态
    void update_response()
    {
        std::lock_guard<std::mutex> lock(m_heartbeatMutex);
        m_bHeartbeatOk = true;
        m_lastHeartbeatTime = std::chrono::steady_clock::now();
    }

private:
    // 监测心跳状态
    void monitor()
    {
        pthread_setname_np(pthread_self(), "HeartMmonitor");

        bool bHeartbeatOk = true;
        while (!m_bExit)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_heartbeatInterval));
            send_message();
            continue;
            {
                std::lock_guard<std::mutex> lock(m_heartbeatMutex);
                auto now = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastHeartbeatTime).count();
                if (duration > (m_heartbeatInterval + 1000))
                {
                    bHeartbeatOk = false;
                }
            }
            bHeartbeatOk = true;
            // 如果心跳不正常，则重新连接
            if (!bHeartbeatOk)
            {
                if (m_statusObserver && m_bHeartbeatOk == true)
                {
                    m_statusObserver(m_bHeartbeatOk); // 调用状态改变的回调函数
                }
                m_bHeartbeatOk = bHeartbeatOk;
            }
            // 发送心跳消息
            std::this_thread::sleep_for(std::chrono::milliseconds(m_heartbeatInterval));
            if (!m_bHeartbeatOk)
            {
                continue;
            }
            int nRet = send_message();
            if (nRet != 0)
            {
                if (m_statusObserver && m_bHeartbeatOk == true)
                {
                    m_statusObserver(m_bHeartbeatOk); // 调用状态改变的回调函数
                }
                m_bHeartbeatOk = false;
            }
            bHeartbeatOk = m_bHeartbeatOk;
        }
    }

    // 发送心跳消息
    int send_message()
    {
        Net::Message_S stMessage;
        stMessage.nActionCode = m_nActionCode;
        // stMessage.nActionCode = 30032;
        stMessage.pData = m_buffer.data();
        stMessage.nDataLength = m_buffer.size();
        return m_handle->send(stMessage);
    }

private:
    /** 
     *  heartbeat 处理类，用于管理心跳机制。 
     */
    Net::IOBase *m_handle;                                                  
    /** 
     *  状态回调函数，用于处理心跳状态的变化。 
     */
    StautsCallback m_statusObserver;                                        
    /** 
     *  动作代码，用于标识心跳的动作类型。 
     */
    int m_nActionCode;                                                      
    /** 
     *  存储心跳数据的缓冲区。 
     */
    std::vector<char> m_buffer;                                             
    /** 
     *  心跳间隔时间，单位为毫秒。 
     */
    int m_heartbeatInterval;                                                
    /** 
     *  标识是否退出心跳处理。 
     */
    volatile bool m_bExit;                                                           
    /** 
     *  标识心跳是否正常。 
     */
    bool m_bHeartbeatOk;                                                    
    /** 
     *  上次心跳时间的时间点。 
     */
    std::chrono::time_point<std::chrono::steady_clock> m_lastHeartbeatTime; 
    /** 
     *  处理心跳的线程。 
     */
    std::thread m_thread;                                                   
    /** 
     *  互斥锁，用于保护心跳相关的数据。 
     */
    std::mutex m_heartbeatMutex;                                          
};