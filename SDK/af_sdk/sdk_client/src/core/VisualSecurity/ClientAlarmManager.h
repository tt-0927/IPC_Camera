/**
 * @file ClientAlarmManager.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-25
 * 
 * @brief 客户端告警管理类 
 */
#pragma once

#include <tvsdkhttplib.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include <functional>
#include "NetTVSDKClientInterface.h"

using namespace tvsdk;

class CClientAlarmManager
{
public:
    CClientAlarmManager(const std::string& host, int port, const std::string& user, const std::string& pass);
    ~CClientAlarmManager();

    bool StartListen(void* userHandle, const std::string& sessionId);

    // 重连后更新 sessionId，AlarmLoop 下次重连时自动使用新的 sessionId
    void UpdateSessionId(const std::string& newSessionId)
    {
        std::lock_guard<std::mutex> lk(sessionIdMutex_);
        sessionId_ = newSessionId;
    }

    // 主动触发 AlarmLoop 尽快断线重连（用于重连后希望尽快切换到新 session）
    void ForceReconnect()
    {
        std::lock_guard<std::mutex> lk(clientMutex_);
        if (client_) client_->stop();
    }

    void Stop();
    
    bool IsRunning() const { return isRunning_; }

    // session 过期回调：当服务端返回 401 时通知上层触发重新登录
    using SessionExpiredCallback = std::function<void()>;
    void SetSessionExpiredCallback(SessionExpiredCallback cb) { sessionExpiredCb_ = cb; }

    void SetCallback(NET_TV_AlarmCallBack cb, void* userData) 
    {
        alarmCb_ = cb;
        alarmUserData_ = userData;
    }

    void SetChannelStatusCallback(NET_TV_ChannelStatusCallBack cb, void* userData)
    {
        channelStatusCb_ = cb;
        channelStatusUserData_ = userData;
    }

private:
    void AlarmLoop();
    void HealthMonitorLoop();  // 独立线程检测连接假死（read_timeout 可能失效）

private:
    std::string host_;
    int port_;
    std::string username_;
    std::string password_;
    std::string sessionId_;
    std::mutex sessionIdMutex_;  // 保护 sessionId_ 的跨线程读写
    void* userHandle_ = nullptr; // For callback identification

    std::shared_ptr<httplib::Client> client_;
    std::mutex clientMutex_;          // 保护 client_ 的跨线程访问
    std::thread thread_;
    std::atomic<bool> isRunning_{false};

    NET_TV_AlarmCallBack alarmCb_ = nullptr;
    void* alarmUserData_ = nullptr;

    NET_TV_ChannelStatusCallBack channelStatusCb_ = nullptr;
    void* channelStatusUserData_ = nullptr;

    SessionExpiredCallback sessionExpiredCb_ = nullptr;

    // 连接健康监控计数器
    std::atomic<int> heartbeatRecvCount_{0};  // 收到的心跳包数
    std::atomic<int> alarmRecvCount_{0};      // 收到的报警数
    std::atomic<int64_t> m_lastDataTimeMs{0}; // 最后一次收到数据的时间(ms)，原子变量，跨线程安全
    std::atomic<bool> m_firstDataReceived{false}; // 当前连接是否已收到过数据（区分"连接建立中"和"数据中断"）
    std::chrono::steady_clock::time_point m_connStartTime{}; // 当前连接建立时间
    std::chrono::steady_clock::time_point m_lastAlarmTime{}; // 最后一次收到报警的时间
    std::chrono::steady_clock::time_point m_lastStatTime{};  // 数据统计时间（替代 static，每个实例独立）

    // 健康监控线程：当 read_timeout 失效导致 Get() 永久阻塞时，强制中断恢复
    std::thread m_healthMonitorThread;
    std::atomic<bool> m_healthMonitorRunning{false};
    std::atomic<int> m_reconnectCount{0};     // 连接尝试次数（用于诊断）
    std::atomic<bool> m_alarmLoopExited{false}; // AlarmLoop 退出标记：用于 Stop() 判断是否可以安全 join

    // 健康监控恢复限流：防止无限循环恢复导致线程泄露
    static constexpr int kMaxRecoveriesPerWindow = 5;  // 5分钟内最多恢复5次
    static constexpr int kRecoveryWindowSec = 300;     // 恢复计数窗口(5分钟)
    int m_recoveryCountInWindow = 0;                   // 当前窗口内的恢复次数
    std::chrono::steady_clock::time_point m_recoveryWindowStart{}; // 恢复窗口起始时间
};
