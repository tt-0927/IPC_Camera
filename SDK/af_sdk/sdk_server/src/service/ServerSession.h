#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <queue>
#include <memory>
#include <condition_variable>
#include <tvsdkhttplib.h>

using namespace tvsdk;

/**
 * @brief 单个客户端会话对象
 * 管理客户端的连接状态、鉴权状态、心跳保活以及消息推送队列
 */
class CServerSession : public std::enable_shared_from_this<CServerSession>
{
public:
    explicit CServerSession(std::string sessionId);
    ~CServerSession();

    // 禁止拷贝
    CServerSession(const CServerSession&) = delete;
    CServerSession& operator=(const CServerSession&) = delete;

    // --- 状态查询与设置 ---
    std::string GetSessionId() const { return m_sessionId; }
    
    std::string GetClientIP() const { return m_clientIP; }
    void SetClientIP(const std::string& ip) { m_clientIP = ip; }
    
    bool IsLogined() const { return m_isLogined; }
    void SetLogined(bool val) { m_isLogined = val; }

    bool IsConnected() const { return m_isConnected; }
    void SetConnected(bool val);

    bool IsPushEnabled() const { return m_pushEnabled; }
    void SetPushEnabled(bool val) { m_pushEnabled = val; }

    // --- 活跃度管理 ---
    void UpdateLastActive();
    // 检查是否超时（单位：秒）
    bool IsTimeout(int timeoutSec) const;
    // 检查是否长时间无心跳（僵尸连接）
    bool IsZombie(int timeoutSec) const;

    // 检查是否应该发送心跳包（间隔 intervalSec 秒），是则自动更新时间戳
    bool ShouldSendHeartbeat(int intervalSec)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - m_lastHeartbeatSent).count() >= intervalSec)
        {
            m_lastHeartbeatSent = now;
            return true;
        }
        return false;
    }

      // --- 消息推送机制 ---
    struct Attachment 
    {
        std::string contentType; 
        std::string data;
        std::string name;     // form field name, e.g. "image"
        std::string filename; // e.g. "snap.jpg"
    };
    struct AlarmData 
    {
        std::string json;
        std::vector<Attachment> attachments;
    };

    // 将消息加入发送队列 (线程安全)，入队后自动唤醒等待中的 content_provider
    void EnqueueMessage(const AlarmData& data);
    // 尝试从队列获取一条消息 (线程安全，返回 false 表示队列为空)
    bool DequeueMessage(AlarmData& outMsg);
    // 队列是否为空
    bool HasMessages();
    // 清空消息队列（客户端断线时调用，避免重连后收到大量过期报警）
    void ClearMessageQueue();

    // 条件变量 + 超时等待：有数据或超时后返回
    // timeoutMs: 最大等待毫秒数（用于心跳定时唤醒）
    void WaitForData(int timeoutMs);

private:
    const std::string m_sessionId;
    std::string m_clientIP;
    
    std::atomic<bool> m_isLogined{false};
    std::atomic<bool> m_isConnected{false};
    std::atomic<bool> m_pushEnabled{false};

    // 活跃时间 (用于超时清理)
    std::chrono::steady_clock::time_point m_lastActive;
    
    // 心跳发送时间（用于 content_provider 中限速心跳包发送，归属于连接而非线程）
    std::chrono::steady_clock::time_point m_lastHeartbeatSent{};
    
    // 消息队列 (用于 SSE 推送)
    std::queue<AlarmData> m_msgQueue;
    std::condition_variable m_cv; // 有数据入队时唤醒 content_provider

    mutable std::mutex m_mutex; // 保护 m_lastActive、m_msgQueue 和 m_cv
};