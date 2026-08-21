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

    /**
     * @brief 初始化并验证登录（带Digest认证）
     * @return 成功返回true，失败返回false
     */
    bool ConnectAndLogin();

    /**
     * @brief 启动心跳线程
     */
    void StartHeartbeat();

    /**
     * @brief 停止会话（线程安全）
     * @details 停止所有线程，关闭客户端连接，设置离线状态
     */
    void Stop();

    /**
     * @brief 重连循环（内部使用）
     * @details 使用指数退避策略重新登录，成功后恢复心跳和报警监听
     */
    void ReconnectLoop();

    /**
     * @brief 发送请求到设备
     * @param [IN] req 请求参数
     * @param [OUT] outRespBody 响应体输出
     * @return 成功返回true，失败返回false
     */
	bool SendRequest(const CommandRequest& req, std::string& outRespBody);

	/**
     * @brief 设置报警回调函数
     * @param [IN] cb 报警回调函数指针
     * @param [IN] userData 用户数据
     */
	void SetAlarmCallback(NET_TV_AlarmCallBack cb, void* userData);
    
    /**
     * @brief 设置动态图片 V2 告警回调函数
     */
    void SetAlarmCallbackV2(NET_TV_AlarmCallBackV2 cb, void* userData);

    /**
     * @brief 设置通道状态回调函数
     * @param [IN] cb 通道状态回调函数指针
     * @param [IN] userData 用户数据
     */
    void SetChannelStatusCallback(NET_TV_ChannelStatusCallBack cb, void* userData);

    /**
     * @brief 开始监听报警消息
     * @return 成功返回true，失败返回false
     */
    bool StartAlarmListen();

    /**
     * @brief 停止监听报警消息
     * @return 成功返回true，失败返回false
     */
    bool StopAlarmListen();

    /**
     * @brief 获取在线状态
     * @return 在线返回true，离线返回false
     */
    bool IsOnline() const { return isOnline_; }

    /**
     * @brief 获取用户句柄
     * @return 用户句柄
     */
    LPUSER_HANDLE GetUserId() const { return userHand_; }

    /**
     * @brief 获取会话ID
     * @return 会话ID
     */
    std::string GetSessionId() const { return sessionId_; }

    /**
     * @brief 获取设备主机地址
     * @return 主机地址
     */
    std::string GetHost() const { return host_; }

private:
    /**
     * @brief SSE长连接循环线程函数（备用心跳方式）
     */
    void SseLoop();

    /**
     * @brief 心跳保活循环线程函数
     * @details 定时发送心跳包检测连接状态
     */
	void HeartbeatLoop();

	/**
     * @brief 报警监听由ClientAlarmManager管理
     */
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
