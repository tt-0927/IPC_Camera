/**
 * @file UserSession.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-22
 * 
 * @brief 用户会话类
 */
#pragma once

#include <tvsdkhttplib.h>
#include <string>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <memory>
#include <iostream>

#include "NetTVSDKClientInterface.h"
#include "ClientAlarmManager.h"

using namespace tvsdk;

/* 用户句柄类型 */
typedef void* LPUSER_HANDLE;

/* 会话断开回调函数类型 */
using OnSessionLostCallback = std::function<void(LPUSER_HANDLE)>;

struct CommandRequest 
{
    // 基础信息
    std::string method;         // "GET", "POST", "PUT", "DELETE"
    std::string url;            // 基础 URL
    
    // 数据部分
    std::string jsonBody;       // JSON 字符串 body
    std::map<std::string, std::string> queryParams; // URL 查询参数 ?key=val
    
    // 二进制拓展 (如果需要上传文件)
    const char* binData = nullptr;
    size_t binSize = 0;

    // 构造函数简化使用
    CommandRequest(const std::string& m, const std::string& u) : method(m), url(u) {}
    CommandRequest() = default;
};

class CUserSession : public std::enable_shared_from_this<CUserSession> 
{
public:
    CUserSession(LPUSER_HANDLE userHand,  const std::string& host, int port, 
                 const std::string& user, const std::string& pass,
                 int hbInterval, int maxRetry,
				 int connectTimeout, int receiveTimeout,
				 OnSessionLostCallback callback);
    ~CUserSession();

    // 初始化并验证登录 (带 Digest)
    bool ConnectAndLogin();

    // 启动心跳线程
    void StartHeartbeat();

    // 停止会话 (线程安全)
    void Stop();

    // 重连循环（内部使用）
    void ReconnectLoop();

	bool SendRequest(const CommandRequest& req, std::string& outRespBody);

	// Alarm
	void SetAlarmCallback(NET_TV_AlarmCallBack cb, void* userData);
    void SetChannelStatusCallback(NET_TV_ChannelStatusCallBack cb, void* userData);

    bool StartAlarmListen();
    bool StopAlarmListen();

    // Getters
    bool IsOnline() const { return isOnline_; }
    LPUSER_HANDLE GetUserId() const { return userHand_; }
    std::string GetSessionId() const { return sessionId_; }
    std::string GetHost() const { return host_; }

private:
    // SSE 循环线程函数
    void SseLoop();
	// void OnSessionLost(int userId);
	// 心跳保活
	void HeartbeatLoop();

	// AlarmLoop managed by ClientAlarmManager

private:
	LPUSER_HANDLE userHand_;
    std::string host_;
    int port_;
    std::string username_;
    std::string password_;
    std::string sessionId_;

    int heartbeatInterval_;
    int maxRetry_;

    std::atomic<bool> isRunning_{false};
    std::atomic<bool> isOnline_{false};

    // 命令发送锁 (保护 CmdClient 串行发送)
    std::mutex cmdMutex_;
    
    std::thread sseThread_;
	std::thread heartbeatThread_;

    int connectTimeout_;      // 连接超时（秒）
    int receiveTimeout_;     // 接收超时（秒）

	/**
	 * @brief 双客户端隔离设计
	 */
    std::unique_ptr<httplib::Client> cmdClient_; // 短连接：发命令
    std::unique_ptr<httplib::Client> sseClient_; // 长连接：SSE心跳

	OnSessionLostCallback notifyLost_;		// 会话断开 回调处理

    
	// Reconnect members
    std::atomic<bool> isReconnecting_{false};  // 重连中标志
    std::atomic<int> reconnectDelay_{1};       // 当前重连延迟（秒）
    std::thread reconnectThread_;              // 重连线程
    std::mutex reconnectMutex_;                // 重连互斥锁

	// Alarm members
    std::shared_ptr<CClientAlarmManager> m_alarmMgr;
};
