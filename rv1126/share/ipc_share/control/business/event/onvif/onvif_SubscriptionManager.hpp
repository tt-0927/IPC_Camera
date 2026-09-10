/**
 * @file onvif_SubscriptionManager.hpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-29
 * 
 * @brief onvif事件订阅管理
 */
#pragma once

#include <map>
#include <string>
#include <array>
#include <mutex>
#include <condition_variable>
#include <ctime>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>

#include "alarm_define.h"
#include "Singleton.h"
#include "onvif_type.h"
#include "dlog.h"
#include <sys/select.h>
#include <unistd.h>
#include <time.h>

#define ONVIF_SUB_EXPIRERIME 30 //订阅有效时长，30分钟

class COnvifSubscriptionManager : public CSingleton<COnvifSubscriptionManager>
{

private:
    struct SubscriptionQueue 
    {
        std::array<OnvifAlarmEventInFo_S, ONVIF_EVENT_MAX_PER_SUB> buffer;
        size_t head = 0;
        size_t tail = 0;
        size_t count = 0;
        mutable std::mutex mtx;
        std::condition_variable cond;
    };

    struct SubscriptionItem 
    {
        std::shared_ptr<SubscriptionQueue> queue;
        long long expireTime;   //过期时间 (Monotonic seconds)
    };
public:
    COnvifSubscriptionManager() 
    {
        // 启动定时器线程检测
        m_timerThread = std::thread(&COnvifSubscriptionManager::timerThread, this);
    }

    ~COnvifSubscriptionManager() 
    {
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_running = false;
        }
        m_cond.notify_one();
        if (m_timerThread.joinable()) 
        {
            m_timerThread.join();
        }
    }


    long long getMonotonicTime()
    {
#if defined(__linux__) || defined(__unix__)
        struct timespec ts;
        if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        {
             return (long long)ts.tv_sec;
        }
#endif
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    }

     /**
      * @brief 添加订阅
      * @param strAddress 订阅地址
      * @return true 成功
      * @return false 失败
      */
    bool addSubscription(const std::string& strAddress) 
    {
        std::lock_guard<std::mutex> lock(m_mapMtx);
        if (m_subMap.size() >= ONVIF_MAX_SUBSCRIPTIONS) 
        {
            dlog_debug("addSubscription: 超过最大订阅数 %d", ONVIF_MAX_SUBSCRIPTIONS);
            return false;
        }
        auto it = m_subMap.find(strAddress);
        if (it != m_subMap.end()) 
        {
            it->second.expireTime = getMonotonicTime() + ONVIF_SUB_EXPIRERIME * 60;
            m_cond.notify_one(); // 唤醒定时器线程重新检查
            return true;
        }
        dlog_debug("SubscriptionManager 添加订阅地址[%s]",strAddress.c_str());
        SubscriptionItem item;
        item.queue = std::make_shared<SubscriptionQueue>();
        item.expireTime = getMonotonicTime() + ONVIF_SUB_EXPIRERIME * 60;
        m_subMap[strAddress] = item;
        m_cond.notify_one();
        //m_subMap[strAddress] = std::make_shared<SubscriptionQueue>();
     
        return true;
    }

    // 删除订阅
    void removeSubscription(const std::string& strAddress) 
    {
        std::lock_guard<std::mutex> lock(m_mapMtx);
        dlog_debug("SubscriptionManager 删除订阅地址[%s]",strAddress.c_str());
        m_subMap.erase(strAddress);
        m_cond.notify_one();
    }

    /**
     * @brief 续订
     * @param strAddress 订阅地址
     * @param nDurationSec 续订时长(秒)
     * @param pCurrentSec [输出]当前剩余时长
     * @return true 成功
     */
    bool renewSubscription(const std::string& strAddress, int nDurationSec, int* pCurrentSec) 
    {
        std::lock_guard<std::mutex> lock(m_mapMtx);
        auto it = m_subMap.find(strAddress);
        if (it == m_subMap.end()) 
        {
            dlog_debug("renewSubscription: 订阅不存在, address=%s", strAddress.c_str());
            return false;
        }

        // 更新过期时间
        it->second.expireTime = getMonotonicTime() + nDurationSec;

        if(pCurrentSec)
        {
            *pCurrentSec = nDurationSec;
        }

        dlog_debug("renewSubscription: 续订成功 [%s] 时长[%d]s", strAddress.c_str(), nDurationSec);
        m_cond.notify_one(); 
        return true;
    }

    /**
     * @brief 推送事件到所有订阅
     */
    void pushEventToAll(Event::Type_E type,bool isTrigger) 
    {
        OnvifAlarmEventInFo_S evt;
        switch (type)
        {
        case Event::Type_E::MOTION_DETECT:
            evt.enAlarmType = MOTION_DETECTION_ALARM;
            break;
        case Event::Type_E::OCCLUSION_DETECT:
            evt.enAlarmType = IMAGE_OBSTRUTION_ALARM;
            break;
        case Event::Type_E::INTRUSION:
            evt.enAlarmType = INTRUSION_ALARM;
            break;
         case Event::Type_E::LINE_CROSSING:
            evt.enAlarmType = LINGERING_ALARM;
            break;
        case Event::Type::ENTER_REGION: /* 进入区域 */
            evt.enAlarmType = ONVIF_ENTER_REGION;
            break;
        case Event::Type::LEAVE_REGION: /* 离开区域 */
            evt.enAlarmType = ONVIF_LEAVE_REGION;
            break;

        /* Smart事件 */
        case Event::Type::AUDIO_ANOMALY: /* 音频异常侦测 */
            evt.enAlarmType = ONVIF_AUDIO_ANOMALY;
            break;
        case Event::Type::AUDIO_SUDDEN_RISE: /* 声强陡升 */
            evt.enAlarmType = ONVIF_AUDIO_SUDDEN_RISE;
            break;
        case Event::Type::AUDIO_SUDDEN_DROP: /* 声强陡降 */
            evt.enAlarmType = ONVIF_AUDIO_SUDDEN_DROP;
            break;
        case Event::Type::SCENE_CHANGE: /* 场景变更 */
            evt.enAlarmType = ONVIF_SCENE_CHANGE;
            break;
        case Event::Type::FACE_DETECT: /* 人脸侦测 */
           evt.enAlarmType = ONVIF_FACE_DETECT;
            break;
        case Event::Type::LOITERING_DETECT: /* 徘徊侦测 */
            evt.enAlarmType = ONVIF_LOITERING_DETECT;
            break;
        case Event::Type::CROWD_GATHERING: /* 人员聚集 */
            evt.enAlarmType = ONVIF_CROWD_GATHERING;
            break;
        case Event::Type::PARKING_DETECT: /* 停车侦测 */
            evt.enAlarmType = ONVIF_PARKING_DETECT;
            break;
        case Event::Type::UNATTENDED_OBJECT: /* 物品遗留 */
            evt.enAlarmType = ONVIF_UNATTENDED_OBJECT;
            break;
        case Event::Type::OBJECT_REMOVAL: /* 物品拿取 */
           evt.enAlarmType = ONVIF_OBJECT_REMOVAL;
            break;
        case Event::Type::PET_RECOGNITION: /* 宠物识别 */
            evt.enAlarmType = ONVIF_PET_RECOGNITION;
            break;
        case Event::Type::FACE_CAPTURE: /* 人脸抓拍 */
            evt.enAlarmType = ONVIF_FACE_CAPTURE;
            break;
        default:
            return;
        }
       
        evt.alarmTime = time(nullptr);
        evt.nValue = isTrigger;
        dlog_debug("SubscriptionManager 产生事件[%d] Value[%d] 推送到所有订阅地址",(int )type,evt.nValue);

        std::vector<std::shared_ptr<SubscriptionQueue>> activeQueues;
        {
            std::lock_guard<std::mutex> lock(m_mapMtx);
            //dlog_debug("pushEventToAll: 当前订阅数=%zu", m_subMap.size());
            for (auto& kv : m_subMap) 
            {
                if (kv.second.queue) 
                {
                    activeQueues.push_back(kv.second.queue);
                }
            }
        }

        /* 释放Map锁后再推送，避免死锁 */
        for (auto& queue : activeQueues)
        {
            pushEventToQueue(*queue, evt);
            //dlog_debug("pushEventToAll: 事件已推送到订阅");
        }
    }

    /**
     * @brief 按订阅地址阻塞获取事件
     * @param strAddress 订阅地址
     * @param pBatch 事件数组信息
     * @param nTimeoutMs 阻塞时长
     * @return int 
     */
    int pullEvents(int socket_fd,
                   const std::string& strAddress, 
                   OnvifAlarmEventBatch_S* pBatch, 
                   int nTimeoutMs) 
    {
        if (!pBatch) return -1;

        std::shared_ptr<SubscriptionQueue> subQueue;
        long long nExpireTime = 0;
        {
            std::lock_guard<std::mutex> lock(m_mapMtx);
            auto it = m_subMap.find(strAddress);
            if (it == m_subMap.end()) 
            {
                dlog_debug("pullEvents: 订阅不存在, address=%s", strAddress.c_str());
                return -2; 
            }
            subQueue = it->second.queue;
            /* 获取剩余有效时长 */
            auto now = getMonotonicTime();
            nExpireTime = it->second.expireTime - now;
        }
        
        //dlog_debug("Subs: 开始等待事件，超时时间 [%d]ms",nTimeoutMs);
        std::unique_lock<std::mutex> qLock(subQueue->mtx);

        /* 用户反馈notify_one无效，改用 explicit polling 方式 */
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(nTimeoutMs);
        
        while (true)
        {
            if (subQueue->count > 0)
            {
                //dlog_debug("pullEvents: 获取到事件");
                break;
            }

            if (std::chrono::steady_clock::now() >= deadline)
            {
                //dlog_debug("pullEvents: 等待超时");
                break;
            }

            /* 显式释放锁后休眠，完全独立于notify机制 */
            qLock.unlock();

            // fd_set readfds;
            // FD_ZERO(&readfds);
            // int max_fd = 0;
            // if (socket_fd >= 0) 
            // {
            //     FD_SET(socket_fd, &readfds);
            //     max_fd = socket_fd;
            // }

        //     struct timeval tv;
        //     tv.tv_sec = 0;
        //     tv.tv_usec = 10000; // 10ms

        //     int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        //     if(ret > 0)
        //     {
        //        if (socket_fd >= 0 && FD_ISSET(socket_fd, &readfds))
        //        {
        //            dlog_debug("pullEvents: socket readable, client disconnect");
        //            qLock.lock();
        //            return 1;
        //        }
        //     }
        //     else if(ret < 0 && errno != EINTR)
        //     {
        //         dlog_error("pullEvents: select error");
        //         qLock.lock();
        //         return 1;
        //     }

             std::this_thread::sleep_for(std::chrono::milliseconds(10));
             qLock.lock();
        }

        pBatch->nExpireTime = nExpireTime - nTimeoutMs / 1000;

        if (subQueue->count == 0) 
        {
            pBatch->nEventNum = 0;
            return 1; // 超时无事件 or 假唤醒
        }

        // 拷贝所有事件
        pBatch->nEventNum = 0;
        for (size_t i = 0; i < subQueue->count; ++i) 
        {
            size_t idx = (subQueue->head + i) % ONVIF_EVENT_MAX_PER_SUB;
            pBatch->events[i] = subQueue->buffer[idx];
            pBatch->nEventNum++;
        }

        // 清空队列
        subQueue->head = 0;
        subQueue->tail = 0;
        subQueue->count = 0;
      
        return 0; 
    }

private:
    // 向单个队列推送事件
    void pushEventToQueue(SubscriptionQueue& q, const OnvifAlarmEventInFo_S& evt) 
    {
        std::lock_guard<std::mutex> lock(q.mtx);
        for (size_t i = 0; i < q.count; ++i) 
        {
            size_t idx = (q.head + i) % ONVIF_EVENT_MAX_PER_SUB;
            /* 不覆盖相同类型的事件 */
            if (q.buffer[idx].enAlarmType == evt.enAlarmType) 
            {
                // nValue 不同 → 覆盖原来的值
                if (q.buffer[idx].nValue != evt.nValue) 
                {
                    //dlog_debug("相同类型但状态不同，覆盖原有事件: %d  OLD[%d] NEW[%d]", evt.enAlarmType,q.buffer[idx].nValue,evt.nValue);
                    q.buffer[idx] = evt; 
                    q.cond.notify_one(); /* 通知更新 */
                }
                return;
            }
        }
        /* 新事件 */
        size_t pushIdx = q.tail;
        if (q.count < ONVIF_EVENT_MAX_PER_SUB)
        {
             q.buffer[pushIdx] = evt;
             q.tail = (q.tail + 1) % ONVIF_EVENT_MAX_PER_SUB;
             q.count++;
        }
        else
        {
            /* 队列满，覆盖最早的（head） */
            //dlog_debug("pushEventToQueue: 缓冲区满，覆盖最老事件");
            pushIdx = q.head; // 覆盖head位置
            q.buffer[pushIdx] = evt;
            q.head = (q.head + 1) % ONVIF_EVENT_MAX_PER_SUB; // head后移
             // tail maintains its relative position (circular) effectively
            q.tail = (q.tail + 1) % ONVIF_EVENT_MAX_PER_SUB; 
        }

        //dlog_debug("pushEventToQueue: 推送新事件 count=%zu", q.count);
        q.cond.notify_one();
    }

     // 定时器线程
    void timerThread() 
    {
        while (m_running) 
        {
            if (m_subMap.empty()) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } 
           
            // 检查并移除过期的订阅
             auto now = getMonotonicTime();
            {
                std::lock_guard<std::mutex> lock(m_mapMtx);
                for (auto it = m_subMap.begin(); it != m_subMap.end();) 
                {
                     long long remaining = it->second.expireTime - now;
                #if 0
                    dlog_debug("检测订阅地址: [%s] 剩余时间[%lld]秒", it->first.c_str(),remaining);
                #endif
                    if (remaining <= 0) 
                    {
                        dlog_debug("订阅地址: [%s] 过期，移除", it->first.c_str());
                        it = m_subMap.erase(it);
                    } 
                    else 
                    {
                        ++it;
                    }
                }
            }
            /* 5秒一次 */
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        }
    }


private:
    std::map<std::string, SubscriptionItem> m_subMap; // 订阅地址→事件队列
    mutable std::mutex m_mapMtx;
    mutable std::mutex m_mtx;
    std::condition_variable m_cond;
    std::thread m_timerThread;
    bool m_running = true;
};