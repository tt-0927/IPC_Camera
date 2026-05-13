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

bool CSessionManager::Login(std::string& OutSessionId) 
{
	std::lock_guard<std::mutex> Lock(Mtx_);
	
	OutSessionId = GenerateSessionId();
	auto newSession = std::make_shared<CServerSession>(OutSessionId);
    newSession->SetLogined(true);
    newSession->SetConnected(true);
    
    m_sessions[OutSessionId] = newSession;

	NSDK_LOG_DEBUG("[SessionManager] Login Sucessfull! SessionId[%s]",OutSessionId.c_str());
	
	return true;
}

bool CSessionManager::Logout(const std::string& SessionId) 
{
	std::lock_guard<std::mutex> Lock(Mtx_);
	auto It = m_sessions.find(SessionId);
	if (It == m_sessions.end())
	{
		return false;
	}
	NSDK_LOG_DEBUG("[SessionManager] Logout Sucessfull! SessionId[%s]",SessionId.c_str());
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
        NSDK_LOG_INFO("[SessionManager] Push enabled for %s", SessionId.c_str());
        return true;
    }
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

    CServerSession::AlarmData data;
    data.json = json;
    data.attachments = attachments;

    for (auto& pair : m_sessions) 
    {
        auto session = pair.second;
        // 只有 登录 + 连接 + 开启推送 的客户端才发送
        if (session->IsLogined() && session->IsConnected() && session->IsPushEnabled()) 
		{
            session->EnqueueMessage(data);
            count++;
        }
    }
    return count;
}

size_t CSessionManager::GetSessionCount()
{
    std::lock_guard<std::mutex> Lock(Mtx_);
    return m_sessions.size();
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
	
	Login(SessionId);
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

    NSDK_LOG_INFO("[SessionManager] Alarm Subscribe Start: %s", SessionId.c_str());

    res.set_content_provider(
        "multipart/form-data; boundary=" + boundary,
        [this, SessionId, boundary](size_t, httplib::DataSink& sink) -> bool 
        {
             auto sess = GetSession(SessionId);
             if (!sess || !sess->IsLogined()) return false;
             
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
             std::this_thread::sleep_for(std::chrono::milliseconds(100));
             return true;
        },
        [this, SessionId](bool) 
        {
            NSDK_LOG_INFO("[SessionManager] Alarm Listen Closed: %s", SessionId.c_str());
            MarkDisconnected(SessionId);
        }

        // NSDK_LOG_INFO("[SessionManager] Alarm Listen Closed: %s", SessionId.c_str());
    );
}

