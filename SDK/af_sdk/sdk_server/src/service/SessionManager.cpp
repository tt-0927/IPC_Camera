/**
 * @file SessionManager.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief SessionManager 模块实现
 * 功能说明：
 * 1. 实现 SessionManager 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#define NETSDK_HTTP_KEEP_ALIVE_MILLISECONDS 	(1 * 1000)			/* HTTP保活时间 */

#include "SessionManager.h"
#include "NetSdkLog.h"
#include "NetTVSDKHttpUrl.h"
#include "NetTVSDKServerInterface.h"
#include "SDKConvert.h"
#include "HttpAuthHandler.h"
#include <sstream>

namespace
{
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 BuildMultipartPart 定义的内部处理。
 * @param [in] parameter 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
std::string BuildMultipartPart(const std::string& boundary,
                               const std::string& name,
                               const std::string& contentType,
                               const std::string& body,
                               const std::string& filename = std::string())
{
    std::stringstream ss;
    ss << "--" << boundary << "\r\n";
    ss << "Content-Disposition: form-data; name=\"" << name << "\"";
    if (!filename.empty())
    {
        ss << "; filename=\"" << filename << "\"";
    }
    ss << "\r\n";
    ss << "Content-Type: " << contentType << "\r\n";
    ss << "Content-Length: " << body.size() << "\r\n\r\n";
    ss << body << "\r\n";
    return ss.str();
}
}

CSessionManager::CSessionManager()
{
	m_bRunning = true;
    m_stCleanerThread = std::thread(&CSessionManager::CleanupLoop, this);
}

CSessionManager::~CSessionManager()
{
	m_bRunning = false;
    if (m_stCleanerThread.joinable())
	{
        m_stCleanerThread.join();
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 GenerateSessionId 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

std::string CSessionManager::GenerateSessionId()
{
	static std::random_device Rd;
	static std::mt19937 Gen(Rd());
	static std::uniform_int_distribution<> Dis(100000, 999999);
	return "session_" + std::to_string(Dis(Gen));
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 Login 定义的内部处理。
 * @param [out] OutSessionId 函数处理参数。
 * @param [in] clientIP 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CSessionManager::Login(std::string& OutSessionId, const std::string& clientIP)
{
	std::lock_guard<std::mutex> Lock(m_stMutex);

	OutSessionId = GenerateSessionId();
	auto newSession = std::make_shared<CServerSession>(OutSessionId);
    newSession->SetLogined(true);
    newSession->SetConnected(true);
    newSession->SetClientIP(clientIP);

    m_stSessions[OutSessionId] = newSession;

	NETSDK_LOG_MESSAGE_INFO("[SessionManager] Client logged in: SessionId=%s, ClientIP=%s, TotalSessions=%zu",
                  OutSessionId.c_str(), clientIP.c_str(), m_stSessions.size());

	return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 Logout 定义的内部处理。
 * @param [in] SessionId 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool CSessionManager::Logout(const std::string& SessionId)
{
	std::lock_guard<std::mutex> Lock(m_stMutex);
	auto It = m_stSessions.find(SessionId);
	if (It == m_stSessions.end())
	{
		NETSDK_LOG_MESSAGE_WARN("[SessionManager] Logout failed: SessionId=%s not found", SessionId.c_str());
		return false;
	}
	std::string clientIP = It->second->GetClientIP();
	NETSDK_LOG_MESSAGE_INFO("[SessionManager] Client logged out: SessionId=%s, ClientIP=%s, TotalSessions=%zu",
                  SessionId.c_str(), clientIP.c_str(), m_stSessions.size() - 1);
	It->second->SetLogined(false);
    It->second->SetConnected(false);
    m_stSessions.erase(It);

	return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 EnablePush 定义的内部处理。
 * @param [in] SessionId 函数处理参数。
 * @return 返回该处理的状态或结果。
 */


bool CSessionManager::EnablePush(const std::string& SessionId)
{
	std::lock_guard<std::mutex> Lock(m_stMutex);
	auto It = m_stSessions.find(SessionId);
    if (It != m_stSessions.end() && It->second->IsLogined())
	{
        It->second->SetPushEnabled(true);
        std::string clientIP = It->second->GetClientIP();
        NETSDK_LOG_MESSAGE_INFO("[SessionManager] Client subscribed to alarms: SessionId=%s, ClientIP=%s, Status=Subscribed",
                      SessionId.c_str(), clientIP.c_str());
        return true;
    }
    NETSDK_LOG_MESSAGE_WARN("[SessionManager] Failed to enable push: SessionId=%s not found or not logged in",
                  SessionId.c_str());
    return false;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 CleanTimeoutSessions 定义的内部处理。
 * @return 无返回值。
 */

void CSessionManager::CleanTimeoutSessions()
{
	std::lock_guard<std::mutex> Lock(m_stMutex);
    for (auto It = m_stSessions.begin(); It != m_stSessions.end();)
    {
        /* 5分钟超时清理 */
        if (It->second->IsTimeout(300) || It->second->IsZombie(600))
		{
            NETSDK_LOG_MESSAGE_INFO("[SessionManager] Removing timeout session: %s", It->first.c_str());
            It = m_stSessions.erase(It);
        } else {
            ++It;
        }
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 CleanupLoop 定义的内部处理。
 * @return 无返回值。
 */

void CSessionManager::CleanupLoop()
{
    while (m_bRunning)
    {
        /* 每 10 秒检查一次 */
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if (!m_bRunning) break;

        CleanTimeoutSessions();
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 MarkDisconnected 定义的内部处理。
 * @param [in] SessionId 函数处理参数。
 * @return 无返回值。
 */

void CSessionManager::MarkDisconnected(const std::string& SessionId)
{
	std::lock_guard<std::mutex> Lock(m_stMutex);
	auto It = m_stSessions.find(SessionId);
    if (It != m_stSessions.end())
	{
        It->second->SetConnected(false);
        /* 断线时清空队列，避免客户端重连后收到大量已过期的历史报警 */
        /* 报警是实时性事件，断线期间的报警对于客户端已无意义 */
        It->second->ClearMessageQueue();
        NETSDK_LOG_MESSAGE_INFO("[SessionManager] Client disconnected, queue cleared: SessionId=%s, ClientIP=%s",
                      SessionId.c_str(), It->second->GetClientIP().c_str());
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 GetSession 对应的数据。
 * @param [in] SessionId 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::shared_ptr<CServerSession> CSessionManager::GetSession(const std::string& SessionId)
{
    std::lock_guard<std::mutex> Lock(m_stMutex);
    auto It = m_stSessions.find(SessionId);
    if (It != m_stSessions.end()) {
        return It->second;
    }
    return nullptr;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 PushToAll 对应的处理。
 * @param [in] json 函数处理参数。
 * @param [in] attachments 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

size_t CSessionManager::PushToAll(const std::string& json, const std::vector<CServerSession::Attachment_S>& attachments)
{
	std::lock_guard<std::mutex> Lock(m_stMutex);
    size_t count = 0;
    size_t totalSessions = m_stSessions.size();
    size_t notLogined = 0;
    size_t notConnected = 0;
    size_t pushDisabled = 0;

    CServerSession::AlarmData_S data;
    data.json = json;
    data.attachments = attachments;

    std::string forwardedClients;

    for (auto& pair : m_stSessions)
    {
        auto session = pair.second;
        std::string clientIP = session->GetClientIP();
        std::string sessionId = session->GetSessionId();

        NETSDK_LOG_MESSAGE_DEBUG("[SessionManager] Checking client: SessionId=%s, ClientIP=%s, Logined=%d, Connected=%d, PushEnabled=%d",
                      sessionId.c_str(), clientIP.c_str(),
                      session->IsLogined(), session->IsConnected(), session->IsPushEnabled());

        /* 入队条件：已登录 + 已连接 + 已订阅报警 */
        /* IsConnected 标志客户端当前是否有活跃 AlarmListen 长连接 */
        /* 断线时不入队（MarkDisconnected 已清空队列），重连后再活跃入队 */
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
                NETSDK_LOG_MESSAGE_WARN("[SessionManager] Client skipped (Not Logined): SessionId=%s, ClientIP=%s",
                             sessionId.c_str(), clientIP.c_str());
            }
            else if (!session->IsConnected()) {
                notConnected++;
                /* DEBUG 级别，断线重连期间属于正常现象，不刷屏 */
                NETSDK_LOG_MESSAGE_DEBUG("[SessionManager] Client offline (AlarmListen not active): SessionId=%s, ClientIP=%s",
                             sessionId.c_str(), clientIP.c_str());
            }
            else if (!session->IsPushEnabled()) {
                pushDisabled++;
                NETSDK_LOG_MESSAGE_WARN("[SessionManager] Client skipped (Push Disabled/Not Subscribed): SessionId=%s, ClientIP=%s",
                             sessionId.c_str(), clientIP.c_str());
            }
        }
    }

    if (count == 0 && totalSessions > 0)
    {
        NETSDK_LOG_MESSAGE_WARN("[SessionManager] Alarm not forwarded: No eligible clients. "
                      "Total=%zu, NotLogined=%zu, NotConnected=%zu, PushDisabled=%zu",
                      totalSessions, notLogined, notConnected, pushDisabled);
    }
    else if (count > 0)
    {
        NETSDK_LOG_MESSAGE_INFO("[SessionManager] Alarm forwarded: Success=%zu, Total=%zu, Clients=[%s]",
                      count, totalSessions, forwardedClients.c_str());
    }

    return count;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 GetSessionCount 对应的数据。
 * @return 返回该处理的状态或结果。
 */

size_t CSessionManager::GetSessionCount()
{
    std::lock_guard<std::mutex> Lock(m_stMutex);
    return m_stSessions.size();
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 GetSessionDiagnosticInfo 对应的数据。
 * @return 返回该处理的状态或结果。
 */

std::string CSessionManager::GetSessionDiagnosticInfo()
{
    std::lock_guard<std::mutex> Lock(m_stMutex);
    std::stringstream ss;

    if (m_stSessions.empty())
    {
        ss << "No active sessions (no clients logged in)";
        return ss.str();
    }

    ss << "TotalSessions=" << m_stSessions.size() << ": ";
    int idx = 0;
    for (const auto& pair : m_stSessions)
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
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 HttpCommandLogin 定义的内部处理。
 * @param [in] req 函数处理参数。
 * @param [in,out] res 函数处理参数。
 * @return 无返回值。
 */

void CSessionManager::HttpCommandLogin(const httplib::Request& req, httplib::Response& res)
{
	/* 鉴权 */
	if(!CHttpAuthHandler::instance()->handle_authentication(req, res))
	{
		SessionMessage_S stAuthErr;
		res.status = NET_HTTP_RESP_CODE_SUCCESS;
		res.set_content(SDKConvert::to_respString(NET_E_NOT_AUTHORIZED, 0, stAuthErr), NET_JSON_CONTENT_TYPE);
		return;
	}

	SessionMessage_S stSessionMessage;
	int nRespCode = NET_E_SUCCEED;
	std::string SessionId;

	std::string clientIP = req.remote_addr;
	if (!Login(SessionId, clientIP))
	{
		NETSDK_LOG_MESSAGE_WARN("[SessionManager] Login failed: ClientIP=%s", clientIP.c_str());
		nRespCode = NET_E_FAILED;
	}
	else
	{
		stSessionMessage.SessionId = SessionId;
	}
	res.status = NET_HTTP_RESP_CODE_SUCCESS;
	res.set_content(SDKConvert::to_respString(nRespCode, 0, stSessionMessage), NET_JSON_CONTENT_TYPE);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 HttpCommandLout 定义的内部处理。
 * @param [in] req 函数处理参数。
 * @param [in,out] res 函数处理参数。
 * @return 无返回值。
 */

void CSessionManager::HttpCommandLout(const httplib::Request& req, httplib::Response& res)
{
	/* 鉴权 */
	if(!CHttpAuthHandler::instance()->handle_authentication(req, res))
	{
		SessionMessage_S stAuthErr;
		res.status = NET_HTTP_RESP_CODE_SUCCESS;
		res.set_content(SDKConvert::to_respString(NET_E_NOT_AUTHORIZED, 0, stAuthErr), NET_JSON_CONTENT_TYPE);
		return;
	}

	std::string SessionId = req.get_param_value("session_id");

	if (SessionId.empty())
	{
		res.set_content(R"({"code":-1,"msg":"Session ID required"})", NET_JSON_CONTENT_TYPE);
		res.status = NET_HTTP_RESP_CODE_SUCCESS;
		return;
	}

	bool LogoutOk = Logout(SessionId);

	if (LogoutOk)
	{
		res.set_content(R"({"code":0,"msg":"Logout successful"})", NET_JSON_CONTENT_TYPE);
		res.status = NET_HTTP_RESP_CODE_SUCCESS;
	} else {
		res.set_content(R"({"code":-1,"msg":"Invalid session or not logged in"})", NET_JSON_CONTENT_TYPE);
		res.status = NET_HTTP_RESP_CODE_SUCCESS;
	}
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 HttpCommandKeepAlive 定义的内部处理。
 * @param [in] req 函数处理参数。
 * @param [in,out] res 函数处理参数。
 * @return 无返回值。
 */

void CSessionManager::HttpCommandKeepAlive(const httplib::Request& req, httplib::Response& res)
{
    std::string SessionId = req.get_param_value("session_id");
    auto session = GetSession(SessionId);

    if (session && session->IsLogined())
	{
        /* 刷新活跃时间 */
        session->UpdateLastActive();

        res.status = NET_HTTP_RESP_CODE_SUCCESS;
        res.set_content(R"({"code":0, "msg":"KeepAlive OK"})", "application/json");
    }
	else
	{
        res.status = NET_HTTP_RESP_CODE_UNAUTHORIZED;
        res.set_content(R"({"code":401, "msg":"Session Expired"})", "application/json");
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 HttpCommandAlarmListen 定义的内部处理。
 * @param [in] req 函数处理参数。
 * @param [in,out] res 函数处理参数。
 * @return 无返回值。
 */

void CSessionManager::HttpCommandAlarmListen(const httplib::Request& req, httplib::Response& res)
{
     std::string SessionId = req.get_param_value("session_id");
    auto session = GetSession(SessionId);

    if (!session || !session->IsLogined())
    {
        res.status = NET_HTTP_RESP_CODE_UNAUTHORIZED;
        res.set_content(R"({"code":401, "msg":"Invalid Session"})", "application/json");
        return;
    }

    const uint64_t listenSeq = session->BeginAlarmListen();
     session->SetConnected(true);
    session->SetPushEnabled(true);

    std::string boundary = "frontier";
    res.set_header("Content-Type", "multipart/form-data; boundary=" + boundary);
    res.set_header("Connection", "keep-alive");
    res.set_header("Cache-Control", "no-cache");

    NETSDK_LOG_MESSAGE_INFO("[SessionManager] Alarm Subscribe Start: SessionId=%s, ClientIP=%s, ListenSeq=%llu, TotalSessions=%zu",
                  SessionId.c_str(), session->GetClientIP().c_str(),
                  static_cast<unsigned long long>(listenSeq), GetSessionCount());

    res.set_chunked_content_provider(
        "multipart/form-data; boundary=" + boundary,
        [this, SessionId, boundary, listenSeq](size_t, httplib::DataSink& sink) -> bool
        {
             auto sess = GetSession(SessionId);
             if (!sess || !sess->IsLogined()) return false;
             if (!sess->IsCurrentAlarmListen(listenSeq))
             {
                 NETSDK_LOG_MESSAGE_INFO("[SessionManager] Alarm Listen superseded: SessionId=%s, ClientIP=%s, ListenSeq=%llu, CurrentSeq=%llu",
                               SessionId.c_str(), sess->GetClientIP().c_str(),
                               static_cast<unsigned long long>(listenSeq),
                               static_cast<unsigned long long>(sess->GetAlarmListenSeq()));
                 return false;
             }

             /* AlarmListen 长连接期间自动刺新 session，防止 session 过期被清理 */
             sess->UpdateLastActive();

             CServerSession::AlarmData_S msg;
             if (sess->DequeueMessage(msg))
             {
                 auto tp_dequeue = std::chrono::steady_clock::now();
                 long long ts_dequeue = std::chrono::duration_cast<std::chrono::milliseconds>(
                     tp_dequeue.time_since_epoch()).count();
                 long long ts_alarm = 0;
                 /* 尝试从 JSON 中提取入队时间戳 (enqueue_ts) */
                 auto pos = msg.json.find("\"enqueue_ts\":");
                 if (pos != std::string::npos) {
                     auto end = msg.json.find_first_of(",}\n\r", pos + 14);
                     ts_alarm = std::stoll(msg.json.substr(pos + 14, end - pos - 14));
                 }
                 long long queue_delay = ts_alarm > 0 ? ts_dequeue - ts_alarm : -1;
                 NETSDK_LOG_MESSAGE_INFO("[DIAG] content_provider dequeued alarm: queue_delay_ms=%lld, dequeue_ts=%lld, enqueue_ts=%lld",
                               queue_delay, ts_dequeue, ts_alarm);

                 std::string data = BuildMultipartPart(boundary, "alarm", "application/json", msg.json);

                 /* Attachments Part (Images or others) */
                 for (size_t i = 0; i < msg.attachments.size(); ++i)
                 {
                     const auto& att = msg.attachments[i];
                     std::string filename = att.filename;
                     if (!att.filename.empty()) {
                         filename = att.filename;
                     } else {
                         /* Default filename for images if missing */
                         if (att.contentType.find("image") != std::string::npos) {
                              filename = "alarm_" + std::to_string(i) + ".jpg";
                         }
                     }
                     data += BuildMultipartPart(boundary,
                                                att.name.empty() ? "image" : att.name,
                                                att.contentType.empty() ? "application/octet-stream" : att.contentType,
                                                att.data,
                                                filename);
                 }

                 auto tp_before_write = std::chrono::steady_clock::now();
                 if (sink.write(data.data(), data.size())) {
                     auto tp_after_write = std::chrono::steady_clock::now();
                     long long write_cost = std::chrono::duration_cast<std::chrono::milliseconds>(
                         tp_after_write - tp_before_write).count();
                     NETSDK_LOG_MESSAGE_INFO("[DIAG] sink.write done: data_size=%zu, write_cost_ms=%lld",
                                   data.size(), write_cost);
                     sess->UpdateLastActive();
                 } else {
                     NETSDK_LOG_MESSAGE_WARN("[SessionManager] sink.write FAILED: SessionId=%s, ClientIP=%s, dataSize=%zu, marking disconnected",
                                   SessionId.c_str(), sess->GetClientIP().c_str(), data.size());
                     if (sess->MarkDisconnectedIfCurrentAlarmListen(listenSeq))
                     {
                         NETSDK_LOG_MESSAGE_INFO("[SessionManager] Client disconnected, queue cleared: SessionId=%s, ClientIP=%s, ListenSeq=%llu",
                                       SessionId.c_str(), sess->GetClientIP().c_str(),
                                       static_cast<unsigned long long>(listenSeq));
                     }
                     return false;
                 }
             }
             else
             {
                 /* 无报警时，每 8 秒发一次心跳包，保持 TCP 活跃并让客户端能快速发现断线 */
                 /* 心跳间隔必须小于服务端 write_timeout(30s)，防止 sink.write 因超时返回 false */
                 /* session 对象级别时间戳，避免 thread_local 在线程复用时计时器混乱 */
                 if (sess->ShouldSendHeartbeat(8))
                 {
                     std::string hb = BuildMultipartPart(boundary,
                                                         "heartbeat",
                                                         "application/json",
                                                         "{\"type\":\"heartbeat\"}");
                     if (!sink.write(hb.data(), hb.size())) {
                         if (sess->MarkDisconnectedIfCurrentAlarmListen(listenSeq))
                         {
                             NETSDK_LOG_MESSAGE_INFO("[SessionManager] Client disconnected, queue cleared: SessionId=%s, ClientIP=%s, ListenSeq=%llu",
                                           SessionId.c_str(), sess->GetClientIP().c_str(),
                                           static_cast<unsigned long long>(listenSeq));
                         }
                         return false;
                     }
                     NETSDK_LOG_MESSAGE_INFO("[SessionManager] Heartbeat sent: SessionId=%s, ClientIP=%s",
                                  SessionId.c_str(), sess->GetClientIP().c_str());
                 }
             }
             /* 等待新数据再发送（或超时 1 秒回来检查心跳包） */
             /* EnqueueMessage 入队时 notify_one() 会立即唤醒此处等待 */
             sess->WaitForData(1000, listenSeq);
             return true;
        },
        [this, SessionId, listenSeq](bool)
        {
            auto sess = GetSession(SessionId);
            std::string clientIP = sess ? sess->GetClientIP() : "unknown";
            const bool currentListen = sess && sess->IsCurrentAlarmListen(listenSeq);
            NETSDK_LOG_MESSAGE_INFO("[SessionManager] Alarm Listen Closed: SessionId=%s, ClientIP=%s, ListenSeq=%llu, CurrentSeq=%llu, Current=%d",
                          SessionId.c_str(), clientIP.c_str(),
                          static_cast<unsigned long long>(listenSeq),
                          static_cast<unsigned long long>(sess ? sess->GetAlarmListenSeq() : 0),
                          currentListen ? 1 : 0);
            if (sess && sess->MarkDisconnectedIfCurrentAlarmListen(listenSeq))
            {
                NETSDK_LOG_MESSAGE_INFO("[SessionManager] Client disconnected, queue cleared: SessionId=%s, ClientIP=%s, ListenSeq=%llu",
                              SessionId.c_str(), clientIP.c_str(),
                              static_cast<unsigned long long>(listenSeq));
            }
        }

    );
}
