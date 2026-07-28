/**
 * @file SessionManager.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief SessionManager 模块接口与类型定义
 * 功能说明：
 * 1. 声明 SessionManager 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once


#include <tvsdkhttplib.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>

#include "Singleton.h"
#include "ServerSession.h"

using namespace tvsdk;

class CSessionManager : public CSingleton<CSessionManager>
{
	CSessionManager();
public:

	~CSessionManager();
	friend class CSingleton<CSessionManager>;

public:
	bool EnablePush(const std::string& SessionId);
	void CleanTimeoutSessions();
	void MarkDisconnected(const std::string& SessionId);
	size_t PushToAll(const std::string& json, const std::vector<CServerSession::Attachment_S>& attachments = {});
	size_t GetSessionCount();

	/**
 * @author tianl (tianl@kfb.cn)
	 * @brief 获取所有会话的诊断状态信息（用于日志排查）
	 * @return 诊断信息字符串，包含每个客户端的IP、登录/连接/订阅状态
	 */
	std::string GetSessionDiagnosticInfo();

	void HttpCommandLogin(const httplib::Request& req, httplib::Response& res);
	void HttpCommandLout(const httplib::Request& req, httplib::Response& res);
	void HttpCommandKeepAlive(const httplib::Request& req, httplib::Response& res);
	void HttpCommandAlarmListen(const httplib::Request& req, httplib::Response& res);

private:
	bool Login(std::string& OutSessionId, const std::string& clientIP = "");
	bool Logout(const std::string& SessionId);
	std::string GenerateSessionId();
    std::shared_ptr<CServerSession> GetSession(const std::string& SessionId);

    void CleanupLoop();
private:
    std::unordered_map<std::string, std::shared_ptr<CServerSession>> m_stSessions;
	std::mutex m_stMutex;                              			/* 全局锁 */

    std::thread m_stCleanerThread;
    std::atomic<bool> m_bRunning{false};

    /* 配置：会话超时时间(秒)，默认 5分钟 */
    const int SESSION_TIMEOUT_SEC = 300;
};