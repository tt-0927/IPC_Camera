/**
 * @file SessionManager.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-05
 * 
 * @brief 会话管理类
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
	size_t PushToAll(const std::string& json, const std::vector<CServerSession::Attachment>& attachments = {});
	size_t GetSessionCount();

	void HttpCommandLogin(const httplib::Request& req, httplib::Response& res);
	void HttpCommandLout(const httplib::Request& req, httplib::Response& res);
	void HttpCommandKeepAlive(const httplib::Request& req, httplib::Response& res);
	void HttpCommandAlarmListen(const httplib::Request& req, httplib::Response& res);

private:
	bool Login(std::string& OutSessionId);
	bool Logout(const std::string& SessionId);
	std::string GenerateSessionId();
    std::shared_ptr<CServerSession> GetSession(const std::string& SessionId);

    void CleanupLoop();
private:
    std::unordered_map<std::string, std::shared_ptr<CServerSession>> m_sessions;
	std::mutex Mtx_;                              			// 全局锁

    std::thread cleanerThread_;
    std::atomic<bool> running_{false};
    
    // 配置：会话超时时间(秒)，默认 5分钟
    const int SESSION_TIMEOUT_SEC = 300;
};