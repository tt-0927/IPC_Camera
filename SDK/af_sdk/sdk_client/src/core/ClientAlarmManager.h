/**
 * @file ClientAlarmManager.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief ClientAlarmManager 模块接口与类型定义
 * 功能说明：
 * 1. 声明 ClientAlarmManager 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
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

    /**
     * @author tianl (tianl@kfb.cn)
     * @brief 更新报警监听会话标识。
     * @param [in] newSessionId 重连后获得的新会话标识。
     * @return 无返回值。
     */
    void UpdateSessionId(const std::string& newSessionId)
    {
        std::lock_guard<std::mutex> lk(m_stSessionIdMutex);
        m_strSessionId = newSessionId;
    }

    /**
     * @author tianl (tianl@kfb.cn)
     * @brief 主动中断当前报警连接，以便监听线程使用新会话重新建立连接。
     * @return 无返回值。
     */
    void ForceReconnect()
    {
        std::lock_guard<std::mutex> lk(m_stClientMutex);
        if (m_pClient) m_pClient->stop();
    }

    void Stop();

    bool IsRunning() const { return m_bRunning; }

    /* session 过期回调：当服务端返回 401 时通知上层触发重新登录 */
    using SessionExpiredCallback = std::function<void()>;
    void SetSessionExpiredCallback(SessionExpiredCallback cb) { m_fnSessionExpiredCallback = cb; }
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 SetCallback 定义的内联处理。
 * @param [in] cb 函数处理参数。
 * @param [in,out] userData 函数处理参数。
 * @return 无返回值。
 */

    void SetCallback(NET_TV_AlarmCallBack cb, void* userData)
    {
        m_fnAlarmCallback = cb;
        m_pAlarmUserData = userData;
    }
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 SetChannelStatusCallback 定义的内联处理。
 * @param [in] cb 函数处理参数。
 * @param [in,out] userData 函数处理参数。
 * @return 无返回值。
 */

    void SetChannelStatusCallback(NET_TV_ChannelStatusCallBack cb, void* userData)
    {
        m_fnChannelStatusCallback = cb;
        m_pChannelStatusUserData = userData;
    }

private:
    void AlarmLoop();
    void HealthMonitorLoop();  /* 独立线程检测连接假死（read_timeout 可能失效） */

private:
    std::string m_strHost;
    int m_nPort;
    std::string m_strUsername;
    std::string m_strPassword;
    std::string m_strSessionId;
    std::mutex m_stSessionIdMutex;  /* 保护 m_strSessionId 的跨线程读写 */
    void* m_hUser = nullptr; /* For callback identification */

    std::shared_ptr<httplib::Client> m_pClient;
    std::mutex m_stClientMutex;          /* 保护 m_pClient 的跨线程访问 */
    std::thread m_stThread;
    std::atomic<bool> m_bRunning{false};

    NET_TV_AlarmCallBack m_fnAlarmCallback = nullptr;
    void* m_pAlarmUserData = nullptr;

    NET_TV_ChannelStatusCallBack m_fnChannelStatusCallback = nullptr;
    void* m_pChannelStatusUserData = nullptr;

    SessionExpiredCallback m_fnSessionExpiredCallback = nullptr;

    /* 连接健康监控计数器 */
    std::atomic<int> m_nReceivedHeartbeatCount{0};  /* 收到的心跳包数 */
    std::atomic<int> m_nReceivedAlarmCount{0};      /* 收到的报警数 */
    std::atomic<int64_t> m_lLastDataTimeMilliseconds{0}; /* 最后一次收到数据的时间(ms)，原子变量，跨线程安全 */
    std::atomic<bool> m_bFirstDataReceived{false}; /* 当前连接是否已收到过数据（区分"连接建立中"和"数据中断"） */
    std::chrono::steady_clock::time_point m_stConnectionStartTime{}; /* 当前连接建立时间 */
    std::chrono::steady_clock::time_point m_stLastAlarmTime{}; /* 最后一次收到报警的时间 */
    std::chrono::steady_clock::time_point m_stLastStatisticTime{};  /* 数据统计时间（替代 static，每个实例独立） */

    /* 健康监控线程：当 read_timeout 失效导致 Get() 永久阻塞时，强制中断恢复 */
    std::thread m_stHealthMonitorThread;
    std::atomic<bool> m_bHealthMonitorRunning{false};
    std::atomic<int> m_nReconnectCount{0};     /* 连接尝试次数（用于诊断） */
    std::atomic<bool> m_bAlarmLoopExited{false}; /* AlarmLoop 退出标记：用于 Stop() 判断是否可以安全 join */

    /* 健康监控恢复限流：防止无限循环恢复导致线程泄露 */
    static constexpr int kMaxRecoveriesPerWindow = 5;  /* 5分钟内最多恢复5次 */
    static constexpr int kRecoveryWindowSec = 300;     /* 恢复计数窗口(5分钟) */
    int m_nRecoveryCountInWindow = 0;                   /* 当前窗口内的恢复次数 */
    std::chrono::steady_clock::time_point m_stRecoveryWindowStart{}; /* 恢复窗口起始时间 */
};
