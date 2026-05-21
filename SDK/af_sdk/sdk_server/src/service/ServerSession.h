#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <queue>
#include <memory>
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

    // 将消息加入发送队列 (线程安全)
    void EnqueueMessage(const AlarmData& data);
    // 尝试从队列获取一条消息 (线程安全，返回 false 表示队列为空)
    bool DequeueMessage(AlarmData& outMsg);
    // 队列是否为空
    bool HasMessages();

private:
    const std::string m_sessionId;
    std::string m_clientIP;
    
    std::atomic<bool> m_isLogined{false};
    std::atomic<bool> m_isConnected{false};
    std::atomic<bool> m_pushEnabled{false};

    // 活跃时间 (用于超时清理)
    std::chrono::steady_clock::time_point m_lastActive;
    
    // 消息队列 (用于 SSE 推送)
    std::queue<AlarmData> m_msgQueue;
    
    mutable std::mutex m_mutex; // 保护 m_lastActive 和 m_msgQueue
};