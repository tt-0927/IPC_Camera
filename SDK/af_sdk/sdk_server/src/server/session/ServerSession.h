/**
 * @file ServerSession.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief ServerSession 模块接口与类型定义
 * 功能说明：
 * 1. 声明 ServerSession 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once

#include <string>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <chrono>
#include <queue>
#include <memory>
#include <condition_variable>
#include <tvsdkhttplib.h>

using namespace tvsdk;

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 单个客户端会话对象
 * 管理客户端的连接状态、鉴权状态、心跳保活以及消息推送队列
 */
class CServerSession : public std::enable_shared_from_this<CServerSession>
{
public:
    explicit CServerSession(std::string sessionId);
    ~CServerSession();

    /* 禁止拷贝 */
    CServerSession(const CServerSession&) = delete;
    CServerSession& operator=(const CServerSession&) = delete;

    /* --- 状态查询与设置 --- */
    std::string GetSessionId() const { return m_strSessionId; }

    std::string GetClientIP() const { return m_strClientIp; }
    void SetClientIP(const std::string& ip) { m_strClientIp = ip; }

    bool IsLogined() const { return m_bLoggedIn; }
    void SetLogined(bool val) { m_bLoggedIn = val; }

    bool IsConnected() const { return m_bConnected; }
    void SetConnected(bool val);

    bool IsPushEnabled() const { return m_bPushEnabled; }
    void SetPushEnabled(bool val) { m_bPushEnabled = val; }

    /* AlarmListen 是长连接，同一 Session 只允许最新的一条连接保持有效。 */
    /* 新连接进来时递增序号，旧 content_provider 检测到序号变化后退出。 */
    uint64_t BeginAlarmListen();
    bool IsCurrentAlarmListen(uint64_t listenSeq) const;
    uint64_t GetAlarmListenSeq() const { return m_uAlarmListenSequence.load(); }
    bool MarkDisconnectedIfCurrentAlarmListen(uint64_t listenSeq);

    /* --- 活跃度管理 --- */
    void UpdateLastActive();
    /* 检查是否超时（单位：秒） */
    bool IsTimeout(int timeoutSec) const;
    /* 检查是否长时间无心跳（僵尸连接） */
    bool IsZombie(int timeoutSec) const;

    /**
     * @author tianl (tianl@kfb.cn)
     * @brief 判断是否到达发送心跳的时间，并在满足条件时更新时间戳。
     * @param [in] intervalSec 心跳发送最小间隔，单位为秒。
     * @return true 表示应发送心跳；false 表示尚未到达发送时间。
     */
    bool ShouldSendHeartbeat(int intervalSec)
    {
        std::lock_guard<std::mutex> lk(m_stMutex);
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - m_stLastHeartbeatSent).count() >= intervalSec)
        {
            m_stLastHeartbeatSent = now;
            return true;
        }
        return false;
    }

      /* --- 消息推送机制 --- */
    struct Attachment_S
    {
        std::string contentType;
        std::string data;
        std::string name;     /* form field name, e.g. "image" */
        std::string filename; /* e.g. "snap.jpg" */
    };
    struct AlarmData_S
    {
        std::string json;
        std::vector<Attachment_S> attachments;
    };

    /* 将消息加入发送队列 (线程安全)，入队后自动唤醒等待中的 content_provider */
    void EnqueueMessage(const AlarmData_S& data);
    /* 尝试从队列获取一条消息 (线程安全，返回 false 表示队列为空) */
    bool DequeueMessage(AlarmData_S& outMsg);
    /* 队列是否为空 */
    bool HasMessages();
    /* 清空消息队列（客户端断线时调用，避免重连后收到大量过期报警） */
    void ClearMessageQueue();

    /* 条件变量 + 超时等待：有数据或超时后返回 */
    /* timeoutMs: 最大等待毫秒数（用于心跳定时唤醒） */
    void WaitForData(int timeoutMs, uint64_t listenSeq = 0);

private:
    const std::string m_strSessionId;
    std::string m_strClientIp;

    std::atomic<bool> m_bLoggedIn{false};
    std::atomic<bool> m_bConnected{false};
    std::atomic<bool> m_bPushEnabled{false};
    std::atomic<uint64_t> m_uAlarmListenSequence{0};

    /* 活跃时间 (用于超时清理) */
    std::chrono::steady_clock::time_point m_stLastActive;

    /* 心跳发送时间（用于 content_provider 中限速心跳包发送，归属于连接而非线程） */
    std::chrono::steady_clock::time_point m_stLastHeartbeatSent{};

    /* 消息队列 (用于 SSE 推送) */
    std::queue<AlarmData_S> m_stMessageQueue;
    std::condition_variable m_stCondition; /* 有数据入队时唤醒 content_provider */

    mutable std::mutex m_stMutex; /* 保护 m_stLastActive、m_stMessageQueue 和 m_stCondition */
};
