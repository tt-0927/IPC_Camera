/**
 * @file SessionManager.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-05
 *
 * @brief 会话管理类
 */

#define HTTP_KEEPALIVE_TIME 	(1 * 1000)			/* HTTP保活时间 */

#include "SessionManager.h"
#include "NetSdkLog.h"
#include "NetTVSDKHttpUrl.h"
#include "NetTVSDKServerInterface.h"
#include "SDKConvert.h"
#include "HttpAuthHandler.h"
#include <sstream>

CSessionManager::CSessionManager()
{
	running_ = true;
    cleanerThread_ = std::thread(&CSessionManager::CleanupLoop, this);
}

CSessionManager::~CSessionManager()
{
	running_ = false;
    if (cleanerThread_.joinable())
	{
        cleanerThread_.join();
    }
}

std::string CSessionManager::GenerateSessionId()
{
	static std::random_device Rd;
	static std::mt19937 Gen(Rd());
	static std::uniform_int_distribution<> Dis(100000, 999999);
	return "session_" + std::to_string(Dis(Gen));
}

bool CSessionManager::Login(std::string& OutSessionId, const std::string& clientIP)
{
	std::lock_guard<std::mutex> Lock(Mtx_);

	OutSessionId = GenerateSessionId();
	auto newSession = std::make_shared<CServerSession>(OutSessionId);
    newSession->SetLogined(true);
    newSession->SetConnected(true);
    newSession->SetClientIP(clientIP);

    m_sessions[OutSessionId] = newSession;

	NSDK_LOG_INFO("[SessionManager] Client logged in: SessionId=%s, ClientIP=%s, TotalSessions=%zu",
                  OutSessionId.c_str(), clientIP.c_str(), m_sessions.size());

	return true;
}

bool CSessionManager::Logout(const std::string& SessionId)
{
	std::lock_guard<std::mutex> Lock(Mtx_);
	auto It = m_sessions.find(SessionId);
	if (It == m_sessions.end())
	{
		NSDK_LOG_WARN("[SessionManager] Logout failed: SessionId=%s not found", SessionId.c_str());
		return false;
	}
	std::string clientIP = It->second->GetClientIP();
	NSDK_LOG_INFO("[SessionManager] Client logged out: SessionId=%s, ClientIP=%s, TotalSessions=%zu",
                  SessionId.c_str(), clientIP.c_str(), m_sessions.size() - 1);
	It->second->SetLogined(false);
    It->second->SetConnected(false);
    m_sessions.erase(It);

	return true;
}


bool CSessionManager::EnablePush(const std::string& SessionId)
{
	auto session = GetSession(SessionId);
    if (session && session->IsLogined())
	{
        session->SetPushEnabled(true);
        std::string clientIP = session->GetClientIP();
        NSDK_LOG_INFO("[SessionManager] Client subscribed to alarms: SessionId=%s, ClientIP=%s, Status=Subscribed",
                      SessionId.c_str(), clientIP.c_str());
        return true;
    }
    NSDK_LOG_WARN("[SessionManager] Failed to enable push: SessionId=%s not found or not logged in",
                  SessionId.c_str());
    return false;
}

void CSessionManager::CleanTimeoutSessions()
{
	std::lock_guard<std::mutex> Lock(Mtx_);
    for (auto It = m_sessions.begin(); It != m_sessions.end();)
    {
        // 5分钟超时清理
        if (It->second->IsTimeout(300) || It->second->IsZombie(600))
		{
            NSDK_LOG_INFO("[SessionManager] Removing timeout session: %s", It->first.c_str());
            It = m_sessions.erase(It);
        } else {
            ++It;
        }
    }
}

void CSessionManager::CleanupLoop()
{
    while (running_)
    {
        // 每 10 秒检查一次
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if (!running_) break;

        CleanTimeoutSessions();
    }
}

void CSessionManager::MarkDisconnected(const std::string& SessionId)
{
	auto session = GetSession(SessionId);
    if (session)
	{
        session->SetConnected(false);
        // 断线时清空队列，避免客户端重连后收到大量已过期的历史报警
        // 报警是实时性事件，断线期间的报警对于客户端已无意义
        session->ClearMessageQueue();
        NSDK_LOG_INFO("[SessionManager] Client disconnected, queue cleared: SessionId=%s, ClientIP=%s",
                      SessionId.c_str(), session->GetClientIP().c_str());
    }
}

std::shared_ptr<CServerSession> CSessionManager::GetSession(const std::string& SessionId)
{
    std::lock_guard<std::mutex> Lock(Mtx_);
    auto It = m_sessions.find(SessionId);
    if (It != m_sessions.end()) {
        return It->second;
    }
    return nullptr;
}

size_t CSessionManager::PushToAll(const std::string& json, const std::vector<CServerSession::Attachment>& attachments)
{
	std::lock_guard<std::mutex> Lock(Mtx_);
    size_t count = 0;
    size_t totalSessions = m_sessions.size();
    size_t notLogined = 0;
    size_t notConnected = 0;
    size_t pushDisabled = 0;

    CServerSession::AlarmData data;
    data.json = json;
    data.attachments = attachments;

    std::string forwardedClients;

    for (auto& pair : m_sessions)
    {
        auto session = pair.second;
        std::string clientIP = session->GetClientIP();
        std::string sessionId = session->GetSessionId();

        NSDK_LOG_DEBUG("[SessionManager] Checking client: SessionId=%s, ClientIP=%s, Logined=%d, Connected=%d, PushEnabled=%d",
                      sessionId.c_str(), clientIP.c_str(),
                      session->IsLogined(), session->IsConnected(), session->IsPushEnabled());

        // 入队条件：已登录 + 已连接 + 已订阅报警
        // IsConnected 标志客户端当前是否有活跃 AlarmListen 长连接
        // 断线时不入队（MarkDisconnected 已清空队列），重连后再活跃入队
        if (session->IsLogined() && session->IsConnected() && session->IsPushEnabled())
        {
            session->EnqueueMessage(data);
            count++;
            if (!forwardedClients.empty()) forwardedClients += ", ";
            forwardedClients += clientIP;
        }
        else
        {
            if (!session->IsLogined()) {
                notLogined++;
                NSDK_LOG_WARN("[SessionManager] Client skipped (Not Logined): SessionId=%s, ClientIP=%s",
                             sessionId.c_str(), clientIP.c_str());
            }
            else if (!session->IsConnected()) {
                notConnected++;
                // DEBUG 级别，断线重连期间属于正常现象，不刷屏
                NSDK_LOG_DEBUG("[SessionManager] Client offline (AlarmListen not active): SessionId=%s, ClientIP=%s",
                             sessionId.c_str(), clientIP.c_str());
            }
            else if (!session->IsPushEnabled()) {
                pushDisabled++;
                NSDK_LOG_WARN("[SessionManager] Client skipped (Push Disabled/Not Subscribed): SessionId=%s, ClientIP=%s",
                             sessionId.c_str(), clientIP.c_str());
            }
        }
    }

    if (count == 0 && totalSessions > 0)
    {
        NSDK_LOG_WARN("[SessionManager] Alarm not forwarded: No eligible clients. "
                      "Total=%zu, NotLogined=%zu, NotConnected=%zu, PushDisabled=%zu",
                      totalSessions, notLogined, notConnected, pushDisabled);
    }
    else if (count > 0)
    {
        NSDK_LOG_INFO("[SessionManager] Alarm forwarded: Success=%zu, Total=%zu, Clients=[%s]",
                      count, totalSessions, forwardedClients.c_str());
    }

    return count;
}

size_t CSessionManager::GetSessionCount()
{
    std::lock_guard<std::mutex> Lock(Mtx_);
    return m_sessions.size();
}

std::string CSessionManager::GetSessionDiagnosticInfo()
{
    std::lock_guard<std::mutex> Lock(Mtx_);
    std::stringstream ss;

    if (m_sessions.empty())
    {
        ss << "No active sessions (no clients logged in)";
        return ss.str();
    }

    ss << "TotalSessions=" << m_sessions.size() << ": ";
    int idx = 0;
    for (const auto& pair : m_sessions)
    {
        auto& session = pair.second;
        if (idx > 0) ss << "; ";
        ss << "[" << idx << "] "
           << "IP=" << session->GetClientIP() << ", "
           << "Login=" << (session->IsLogined() ? "Y" : "N") << ", "
           << "Conn=" << (session->IsConnected() ? "Y" : "N") << ", "
           << "Subscribed=" << (session->IsPushEnabled() ? "Y" : "N");
        idx++;
    }

    return ss.str();
}

void CSessionManager::HttpCommandLogin(const httplib::Request& req, httplib::Response& res)
{
	/* 鉴权 */
	if(!CHttpAuthHandler::instance()->handle_authentication(req, res))
	{
		return;
	}

	SeesionMessage_S stSeesionMessage;
	int nRespCode = NET_TV_E_SUCCEED;
	std::string SessionId;

	std::string clientIP = req.remote_addr;
	Login(SessionId, clientIP);
	stSeesionMessage.SeesionId = SessionId;
	res.status = HTTP_RESP_CODE_SUCCESS;
	res.set_content(SDKConvert::to_respString(nRespCode,stSeesionMessage), JSON_CONTENT_TYPE);
}

void CSessionManager::HttpCommandLout(const httplib::Request& req, httplib::Response& res)
{
	/* 鉴权 */
	if(!CHttpAuthHandler::instance()->handle_authentication(req, res))
	{
		return;
	}

	std::string SessionId = req.get_param_value("session_id");

	if (SessionId.empty())
	{
		res.set_content(R"({"code":-1,"msg":"Session ID required"})", JSON_CONTENT_TYPE);
		res.status = HTTP_RESP_CODE_SUCCESS;
		return;
	}

	bool LogoutOk = Logout(SessionId);

	if (LogoutOk)
	{
		res.set_content(R"({"code":0,"msg":"Logout successful"})", JSON_CONTENT_TYPE);
		res.status = HTTP_RESP_CODE_SUCCESS;
	} else {
		res.set_content(R"({"code":-1,"msg":"Invalid session or not logged in"})", JSON_CONTENT_TYPE);
		res.status = HTTP_RESP_CODE_SUCCESS;
	}
}

void CSessionManager::HttpCommandKeepAlive(const httplib::Request& req, httplib::Response& res)
{
    std::string SessionId = req.get_param_value("session_id");
    auto session = GetSession(SessionId);

    if (session && session->IsLogined())
	{
        /* 刷新活跃时间 */
        session->UpdateLastActive();

        res.status = HTTP_RESP_CODE_SUCCESS;
        res.set_content(R"({"code":0, "msg":"KeepAlive OK"})", "application/json");
    }
	else
	{
        res.status = HTTP_RESP_CODE_UNAUTHORIZED;
        res.set_content(R"({"code":401, "msg":"Session Expired"})", "application/json");
    }
}

void CSessionManager::HttpCommandAlarmListen(const httplib::Request& req, httplib::Response& res)
{
     std::string SessionId = req.get_param_value("session_id");
    auto session = GetSession(SessionId);

    if (!session || !session->IsLogined())
    {
        res.status = HTTP_RESP_CODE_UNAUTHORIZED;
        res.set_content(R"({"code":401, "msg":"Invalid Session"})", "application/json");
        return;
    }

     session->SetConnected(true);
    session->SetPushEnabled(true);

    std::string boundary = "frontier";
    res.set_header("Content-Type", "multipart/form-data; boundary=" + boundary);
    res.set_header("Connection", "keep-alive");
    res.set_header("Cache-Control", "no-cache");

    NSDK_LOG_INFO("[SessionManager] Alarm Subscribe Start: SessionId=%s, ClientIP=%s, TotalSessions=%zu",
                  SessionId.c_str(), session->GetClientIP().c_str(), GetSessionCount());

    res.set_content_provider(
        "multipart/form-data; boundary=" + boundary,
        [this, SessionId, boundary](size_t, httplib::DataSink& sink) -> bool
        {
             auto sess = GetSession(SessionId);
             if (!sess || !sess->IsLogined()) return false;

             // AlarmListen 长连接期间自动刺新 session，防止 session 过期被清理
             sess->UpdateLastActive();

             CServerSession::AlarmData msg;
             if (sess->DequeueMessage(msg))
             {
                 std::stringstream ss;
                 // JSON Part
                 ss << "--" << boundary << "\r\n";
                 ss << "Content-Disposition: form-data; name=\"alarm\"\r\n";
                 ss << "Content-Type: application/json\r\n\r\n";
                 ss << msg.json << "\r\n";

                 // Attachments Part (Images or others)
                 for (size_t i = 0; i < msg.attachments.size(); ++i)
                 {
                     const auto& att = msg.attachments[i];
                     ss << "--" << boundary << "\r\n";
                     ss << "Content-Disposition: form-data; name=\"" << (att.name.empty() ? "image" : att.name) << "\";";

                     if (!att.filename.empty()) {
                         ss << " filename=\"" << att.filename << "\"";
                     } else {
                         // Default filename for images if missing
                         if (att.contentType.find("image") != std::string::npos) {
                              ss << " filename=\"alarm_" << i << ".jpg\"";
                         }
                     }
                     ss << "\r\n";

                     ss << "Content-Type: " << (att.contentType.empty() ? "application/octet-stream" : att.contentType) << "\r\n\r\n";
                     ss << att.data << "\r\n";
                 }

                 std::string data = ss.str();
                 if (sink.write(data.data(), data.size())) {
                     sess->UpdateLastActive();
                 } else {
                     MarkDisconnected(SessionId);
                     return false;
                 }
             }
             else
             {
                 // 无报警时，每 8 秒发一次心跳包，保持 TCP 活跃并让客户端能快速发现断线
                 // 心跳间隔必须小于服务端 write_timeout(30s)，防止 sink.write 因超时返回 false
                 // session 对象级别时间戳，避免 thread_local 在线程复用时计时器混乱
                 if (sess->ShouldSendHeartbeat(8))
                 {
                     std::string hb = "--" + boundary + "\r\n"
                                     "Content-Disposition: form-data; name=\"heartbeat\"\r\n"
                                     "Content-Type: application/json\r\n\r\n"
                                     "{\"type\":\"heartbeat\"}\r\n";
                     if (!sink.write(hb.data(), hb.size())) {
                         MarkDisconnected(SessionId);
                         return false;
                     }
                 }
             }
             std::this_thread::sleep_for(std::chrono::milliseconds(100));
             return true;
        },
        [this, SessionId](bool)
        {
            auto sess = GetSession(SessionId);
            std::string clientIP = sess ? sess->GetClientIP() : "unknown";
            NSDK_LOG_INFO("[SessionManager] Alarm Listen Closed: SessionId=%s, ClientIP=%s", SessionId.c_str(), clientIP.c_str());
            MarkDisconnected(SessionId);
        }

    );
}

