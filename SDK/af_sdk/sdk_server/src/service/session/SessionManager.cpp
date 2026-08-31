/**
 * @file SessionManager.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-08-31
 *
 * @brief SessionManager 模块实现。
 * 功能说明：
 * 1. 实现登录会话的创建、注销、保活和超时清理。
 * 2. 实现 AlarmListen 长连接的告警分发和心跳保活。
 * 3. 使用不可变共享告警负载，降低多客户端推送时的重复内存复制。
 *
 * @par 修改记录
 * 2026-08-28 qinjt：统一内部命名和函数注释，适配共享告警负载队列。
 * 2026-08-31 qinjt：仅在存在有效订阅会话时创建共享告警负载，减少无效告警的瞬时内存分配。
 */
#define NETSDK_HTTP_KEEP_ALIVE_MILLISECONDS (1 * 1000)

#include "SessionManager.h"

#include "HttpAuthHandler.h"
#include "NetSdkLog.h"
#include "NetTVSDKHttpUrl.h"
#include "NetTVSDKServerInterface.h"
#include "SDKConvert.h"

#include <chrono>
#include <exception>
#include <random>
#include <sstream>
#include <string>

namespace
{
/**
 * @brief 组装一段 multipart/form-data 响应内容。
 * @param [in] strBoundary multipart 边界字符串。
 * @param [in] strName multipart 字段名称。
 * @param [in] strContentType multipart 内容类型。
 * @param [in] strBody multipart 字段内容。
 * @param [in] strFilename 附件文件名，可以为空。
 * @return 组装完成的 multipart 内容。
 */
static std::string session_buildMultipartPart(
    const std::string& strBoundary,
    const std::string& strName,
    const std::string& strContentType,
    const std::string& strBody,
    const std::string& strFilename = std::string())
{
    std::stringstream stStream;
    stStream << "--" << strBoundary << "\r\n";
    stStream << "Content-Disposition: form-data; name=\"" << strName << "\"";
    if (!strFilename.empty())
    {
        stStream << "; filename=\"" << strFilename << "\"";
    }
    stStream << "\r\n";
    stStream << "Content-Type: " << strContentType << "\r\n";
    stStream << "Content-Length: " << strBody.size() << "\r\n\r\n";
    stStream << strBody << "\r\n";
    return stStream.str();
}
}

/**
 * @brief 创建会话管理器并启动超时清理线程。
 * @param 无。
 * @return 无返回值。
 */
CSessionManager::CSessionManager()
{
    m_bRunning = true;
    m_stCleanerThread = std::thread(&CSessionManager::CleanupLoop, this);
}

/**
 * @brief 销毁会话管理器并等待超时清理线程退出。
 * @param 无。
 * @return 无返回值。
 */
CSessionManager::~CSessionManager()
{
    m_bRunning = false;
    if (m_stCleanerThread.joinable())
    {
        m_stCleanerThread.join();
    }
}

/**
 * @brief 生成新的登录会话标识。
 * @param 无。
 * @return 新生成的会话标识。
 */
std::string CSessionManager::GenerateSessionId()
{
    static std::random_device stRandomDevice;
    static std::mt19937 stGenerator(stRandomDevice());
    static std::uniform_int_distribution<> stDistribution(100000, 999999);
    return "session_" + std::to_string(stDistribution(stGenerator));
}

/**
 * @brief 创建并登记一个新的登录会话。
 * @param [out] strSessionId 输出新会话标识。
 * @param [in] strClientIp 登录客户端 IP 地址。
 * @return true 表示创建成功，false 表示创建失败。
 */
bool CSessionManager::Login(std::string& strSessionId, const std::string& strClientIp)
{
    std::lock_guard<std::mutex> stLock(m_stMutex);

    strSessionId = GenerateSessionId();
    std::shared_ptr<CServerSession> pstSession =
        std::make_shared<CServerSession>(strSessionId);
    pstSession->SetLogined(true);
    pstSession->SetConnected(true);
    pstSession->SetClientIP(strClientIp);
    m_stSessions[strSessionId] = pstSession;

    NETSDK_LOG_MESSAGE_INFO(
        "[SessionManager] Client logged in: SessionId=%s, ClientIP=%s, TotalSessions=%zu",
        strSessionId.c_str(), strClientIp.c_str(), m_stSessions.size());
    return true;
}

/**
 * @brief 注销并移除指定登录会话。
 * @param [in] strSessionId 待注销的会话标识。
 * @return true 表示注销成功，false 表示会话不存在。
 */
bool CSessionManager::Logout(const std::string& strSessionId)
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    const auto stSessionIterator = m_stSessions.find(strSessionId);
    if (stSessionIterator == m_stSessions.end())
    {
        NETSDK_LOG_MESSAGE_WARN(
            "[SessionManager] Logout failed: SessionId=%s not found",
            strSessionId.c_str());
        return false;
    }

    const std::string strClientIp = stSessionIterator->second->GetClientIP();
    NETSDK_LOG_MESSAGE_INFO(
        "[SessionManager] Client logged out: SessionId=%s, ClientIP=%s, TotalSessions=%zu",
        strSessionId.c_str(), strClientIp.c_str(), m_stSessions.size() - 1);
    stSessionIterator->second->SetLogined(false);
    stSessionIterator->second->SetConnected(false);
    m_stSessions.erase(stSessionIterator);
    return true;
}

/**
 * @brief 启用指定会话的告警推送。
 * @param [in] strSessionId 待启用推送的会话标识。
 * @return true 表示启用成功，false 表示会话不存在或未登录。
 */
bool CSessionManager::EnablePush(const std::string& strSessionId)
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    const auto stSessionIterator = m_stSessions.find(strSessionId);
    if (stSessionIterator != m_stSessions.end() &&
        stSessionIterator->second->IsLogined())
    {
        stSessionIterator->second->SetPushEnabled(true);
        const std::string strClientIp = stSessionIterator->second->GetClientIP();
        NETSDK_LOG_MESSAGE_INFO(
            "[SessionManager] Client subscribed to alarms: SessionId=%s, ClientIP=%s, Status=Subscribed",
            strSessionId.c_str(), strClientIp.c_str());
        return true;
    }

    NETSDK_LOG_MESSAGE_WARN(
        "[SessionManager] Failed to enable push: SessionId=%s not found or not logged in",
        strSessionId.c_str());
    return false;
}

/**
 * @brief 清理已经超时或失活的会话。
 * @param 无。
 * @return 无返回值。
 */
void CSessionManager::CleanTimeoutSessions()
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    for (auto stSessionIterator = m_stSessions.begin();
         stSessionIterator != m_stSessions.end();)
    {
        if (stSessionIterator->second->IsTimeout(NETSDK_SESSION_TIMEOUT_SEC) ||
            stSessionIterator->second->IsZombie(NETSDK_SESSION_TIMEOUT_SEC * 2))
        {
            NETSDK_LOG_MESSAGE_INFO(
                "[SessionManager] Removing timeout session: %s",
                stSessionIterator->first.c_str());
            stSessionIterator = m_stSessions.erase(stSessionIterator);
        }
        else
        {
            ++stSessionIterator;
        }
    }
}

/**
 * @brief 周期性执行会话超时清理。
 * @param 无。
 * @return 无返回值。
 */
void CSessionManager::CleanupLoop()
{
    while (m_bRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if (!m_bRunning)
        {
            break;
        }
        CleanTimeoutSessions();
    }
}

/**
 * @brief 标记指定会话断开并清空其待发送告警队列。
 * @param [in] strSessionId 待标记的会话标识。
 * @return 无返回值。
 */
void CSessionManager::MarkDisconnected(const std::string& strSessionId)
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    const auto stSessionIterator = m_stSessions.find(strSessionId);
    if (stSessionIterator != m_stSessions.end())
    {
        stSessionIterator->second->SetConnected(false);
        stSessionIterator->second->ClearMessageQueue();
        NETSDK_LOG_MESSAGE_INFO(
            "[SessionManager] Client disconnected, queue cleared: SessionId=%s, ClientIP=%s",
            strSessionId.c_str(), stSessionIterator->second->GetClientIP().c_str());
    }
}

/**
 * @brief 根据会话标识获取会话共享指针。
 * @param [in] strSessionId 待查询的会话标识。
 * @return 找到时返回会话共享指针，否则返回 nullptr。
 */
std::shared_ptr<CServerSession> CSessionManager::GetSession(
    const std::string& strSessionId)
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    const auto stSessionIterator = m_stSessions.find(strSessionId);
    if (stSessionIterator != m_stSessions.end())
    {
        return stSessionIterator->second;
    }
    return nullptr;
}

/**
 * @brief 将一条不可变告警负载分发到所有符合条件的会话。
 * @param [in] strJson 告警 JSON 文本。
 * @param [in] aAttachments 告警 multipart 附件列表。
 * @return 成功入队的会话数量。
 */
std::size_t CSessionManager::PushToAll(
    const std::string& strJson,
    const std::vector<CServerSession::Attachment_S>& aAttachments)
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    CServerSession::AlarmDataPtr pstAlarmData;
    std::size_t uForwardedCount = 0;
    const std::size_t uTotalSessionCount = m_stSessions.size();
    std::size_t uNotLoginedCount = 0;
    std::size_t uNotConnectedCount = 0;
    std::size_t uPushDisabledCount = 0;
    std::string strForwardedClients;

    for (const auto& stSessionPair : m_stSessions)
    {
        const std::shared_ptr<CServerSession>& pstSession = stSessionPair.second;
        const std::string strClientIp = pstSession->GetClientIP();
        const std::string strSessionId = pstSession->GetSessionId();

        NETSDK_LOG_MESSAGE_DEBUG(
            "[SessionManager] Checking client: SessionId=%s, ClientIP=%s, Logined=%d, Connected=%d, PushEnabled=%d",
            strSessionId.c_str(), strClientIp.c_str(), pstSession->IsLogined(),
            pstSession->IsConnected(), pstSession->IsPushEnabled());

        if (pstSession->IsLogined() && pstSession->IsConnected() &&
            pstSession->IsPushEnabled())
        {
            /*
             * 只有首次命中有效订阅会话时才复制告警正文和附件。
             * 当没有客户端订阅时，避免为无效推送提前分配图片负载。
             */
            if (!pstAlarmData)
            {
                const std::shared_ptr<CServerSession::AlarmData_S>
                    pstMutableAlarmData =
                        std::make_shared<CServerSession::AlarmData_S>();
                pstMutableAlarmData->strJson = strJson;
                pstMutableAlarmData->aAttachments = aAttachments;
                pstAlarmData = pstMutableAlarmData;
            }

            pstSession->EnqueueMessage(pstAlarmData);
            ++uForwardedCount;
            if (!strForwardedClients.empty())
            {
                strForwardedClients += ", ";
            }
            strForwardedClients += strClientIp;
        }
        else if (!pstSession->IsLogined())
        {
            ++uNotLoginedCount;
            NETSDK_LOG_MESSAGE_WARN(
                "[SessionManager] Client skipped (Not Logined): SessionId=%s, ClientIP=%s",
                strSessionId.c_str(), strClientIp.c_str());
        }
        else if (!pstSession->IsConnected())
        {
            ++uNotConnectedCount;
            NETSDK_LOG_MESSAGE_DEBUG(
                "[SessionManager] Client offline (AlarmListen not active): SessionId=%s, ClientIP=%s",
                strSessionId.c_str(), strClientIp.c_str());
        }
        else if (!pstSession->IsPushEnabled())
        {
            ++uPushDisabledCount;
            NETSDK_LOG_MESSAGE_WARN(
                "[SessionManager] Client skipped (Push Disabled/Not Subscribed): SessionId=%s, ClientIP=%s",
                strSessionId.c_str(), strClientIp.c_str());
        }
    }

    if (uForwardedCount == 0 && uTotalSessionCount > 0)
    {
        NETSDK_LOG_MESSAGE_WARN(
            "[SessionManager] Alarm not forwarded: No eligible clients. Total=%zu, NotLogined=%zu, NotConnected=%zu, PushDisabled=%zu",
            uTotalSessionCount, uNotLoginedCount, uNotConnectedCount,
            uPushDisabledCount);
    }
    else if (uForwardedCount > 0)
    {
        NETSDK_LOG_MESSAGE_INFO(
            "[SessionManager] Alarm forwarded: Success=%zu, Total=%zu, Clients=[%s]",
            uForwardedCount, uTotalSessionCount, strForwardedClients.c_str());
    }

    return uForwardedCount;
}

/**
 * @brief 获取当前会话数量。
 * @param 无。
 * @return 当前会话数量。
 */
std::size_t CSessionManager::GetSessionCount()
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    return m_stSessions.size();
}

/**
 * @brief 获取所有会话的诊断状态信息。
 * @param 无。
 * @return 包含客户端 IP、登录、连接和订阅状态的诊断字符串。
 */
std::string CSessionManager::GetSessionDiagnosticInfo()
{
    std::lock_guard<std::mutex> stLock(m_stMutex);
    std::stringstream stStream;
    if (m_stSessions.empty())
    {
        stStream << "No active sessions (no clients logged in)";
        return stStream.str();
    }

    stStream << "TotalSessions=" << m_stSessions.size() << ": ";
    std::size_t uSessionIndex = 0;
    for (const auto& stSessionPair : m_stSessions)
    {
        const std::shared_ptr<CServerSession>& pstSession = stSessionPair.second;
        if (uSessionIndex > 0)
        {
            stStream << "; ";
        }
        stStream << "[" << uSessionIndex << "] "
                 << "IP=" << pstSession->GetClientIP() << ", "
                 << "Login=" << (pstSession->IsLogined() ? "Y" : "N") << ", "
                 << "Conn=" << (pstSession->IsConnected() ? "Y" : "N") << ", "
                 << "Subscribed=" << (pstSession->IsPushEnabled() ? "Y" : "N");
        ++uSessionIndex;
    }
    return stStream.str();
}

/**
 * @brief 处理 HTTP 登录请求。
 * @param [in] req HTTP 请求对象。
 * @param [out] res HTTP 响应对象。
 * @return 无返回值。
 */
void CSessionManager::HttpCommandLogin(
    const httplib::Request& req,
    httplib::Response& res)
{
    if (!CHttpAuthHandler::instance()->handle_authentication(req, res))
    {
        SessionMessage_S stAuthError;
        res.status = NET_HTTP_RESP_CODE_SUCCESS;
        res.set_content(
            SDKConvert::to_respString(NET_E_NOT_AUTHORIZED, 0, stAuthError),
            NET_JSON_CONTENT_TYPE);
        return;
    }

    SessionMessage_S stSessionMessage;
    int nResponseCode = NET_E_SUCCEED;
    std::string strSessionId;
    const std::string strClientIp = req.remote_addr;
    if (!Login(strSessionId, strClientIp))
    {
        NETSDK_LOG_MESSAGE_WARN(
            "[SessionManager] Login failed: ClientIP=%s", strClientIp.c_str());
        nResponseCode = NET_E_FAILED;
    }
    else
    {
        stSessionMessage.SessionId = strSessionId;
    }

    res.status = NET_HTTP_RESP_CODE_SUCCESS;
    res.set_content(
        SDKConvert::to_respString(nResponseCode, 0, stSessionMessage),
        NET_JSON_CONTENT_TYPE);
}

/**
 * @brief 处理 HTTP 注销请求。
 * @param [in] req HTTP 请求对象。
 * @param [out] res HTTP 响应对象。
 * @return 无返回值。
 */
void CSessionManager::HttpCommandLout(
    const httplib::Request& req,
    httplib::Response& res)
{
    if (!CHttpAuthHandler::instance()->handle_authentication(req, res))
    {
        SessionMessage_S stAuthError;
        res.status = NET_HTTP_RESP_CODE_SUCCESS;
        res.set_content(
            SDKConvert::to_respString(NET_E_NOT_AUTHORIZED, 0, stAuthError),
            NET_JSON_CONTENT_TYPE);
        return;
    }

    const std::string strSessionId = req.get_param_value("session_id");
    if (strSessionId.empty())
    {
        res.set_content(
            R"({"return":-1,"message":"Session ID required"})",
            NET_JSON_CONTENT_TYPE);
        res.status = NET_HTTP_RESP_CODE_SUCCESS;
        return;
    }

    const bool bLogoutSucceeded = Logout(strSessionId);
    if (bLogoutSucceeded)
    {
        res.set_content(
            R"({"return":0,"message":"Logout successful"})",
            NET_JSON_CONTENT_TYPE);
    }
    else
    {
        res.set_content(
            R"({"return":-1,"message":"Invalid session or not logged in"})",
            NET_JSON_CONTENT_TYPE);
    }
    res.status = NET_HTTP_RESP_CODE_SUCCESS;
}

/**
 * @brief 处理 HTTP 会话保活请求。
 * @param [in] req HTTP 请求对象。
 * @param [out] res HTTP 响应对象。
 * @return 无返回值。
 */
void CSessionManager::HttpCommandKeepAlive(
    const httplib::Request& req,
    httplib::Response& res)
{
    const std::string strSessionId = req.get_param_value("session_id");
    const std::shared_ptr<CServerSession> pstSession = GetSession(strSessionId);
    if (pstSession && pstSession->IsLogined())
    {
        pstSession->UpdateLastActive();
        res.status = NET_HTTP_RESP_CODE_SUCCESS;
        res.set_content(
            R"({"return":0, "message":"KeepAlive OK"})",
            NET_JSON_CONTENT_TYPE);
    }
    else
    {
        res.status = NET_HTTP_RESP_CODE_UNAUTHORIZED;
        res.set_content(
            R"({"return":401, "message":"Session Expired"})",
            NET_JSON_CONTENT_TYPE);
    }
}

/**
 * @brief 处理 HTTP AlarmListen 长连接请求。
 * @param [in] req HTTP 请求对象。
 * @param [out] res HTTP 响应对象。
 * @return 无返回值。
 */
void CSessionManager::HttpCommandAlarmListen(
    const httplib::Request& req,
    httplib::Response& res)
{
    const std::string strSessionId = req.get_param_value("session_id");
    const std::shared_ptr<CServerSession> pstSession = GetSession(strSessionId);
    if (!pstSession || !pstSession->IsLogined())
    {
        res.status = NET_HTTP_RESP_CODE_UNAUTHORIZED;
        res.set_content(
            R"({"return":401, "message":"Invalid Session"})",
            NET_JSON_CONTENT_TYPE);
        return;
    }

    const uint64_t uListenSequence = pstSession->BeginAlarmListen();
    pstSession->SetConnected(true);
    pstSession->SetPushEnabled(true);

    const std::string strBoundary = "frontier";
    res.set_header(
        "Content-Type", "multipart/form-data; boundary=" + strBoundary);
    res.set_header("Connection", "keep-alive");
    res.set_header("Cache-Control", "no-cache");

    NETSDK_LOG_MESSAGE_INFO(
        "[SessionManager] Alarm Subscribe Start: SessionId=%s, ClientIP=%s, ListenSeq=%llu, TotalSessions=%zu",
        strSessionId.c_str(), pstSession->GetClientIP().c_str(),
        static_cast<unsigned long long>(uListenSequence), GetSessionCount());

    res.set_chunked_content_provider(
        "multipart/form-data; boundary=" + strBoundary,
        [this, strSessionId, strBoundary, uListenSequence](
            std::size_t,
            httplib::DataSink& stSink) -> bool
        {
            const std::shared_ptr<CServerSession> pstCurrentSession =
                GetSession(strSessionId);
            if (!pstCurrentSession || !pstCurrentSession->IsLogined())
            {
                return false;
            }
            if (!pstCurrentSession->IsCurrentAlarmListen(uListenSequence))
            {
                NETSDK_LOG_MESSAGE_INFO(
                    "[SessionManager] Alarm Listen superseded: SessionId=%s, ClientIP=%s, ListenSeq=%llu, CurrentSeq=%llu",
                    strSessionId.c_str(), pstCurrentSession->GetClientIP().c_str(),
                    static_cast<unsigned long long>(uListenSequence),
                    static_cast<unsigned long long>(
                        pstCurrentSession->GetAlarmListenSeq()));
                return false;
            }

            pstCurrentSession->UpdateLastActive();
            CServerSession::AlarmDataPtr pstAlarmData;
            if (pstCurrentSession->DequeueMessage(pstAlarmData) && pstAlarmData)
            {
                const CServerSession::AlarmData_S& stAlarmData = *pstAlarmData;
                const std::chrono::steady_clock::time_point stDequeueTime =
                    std::chrono::steady_clock::now();
                const long long lDequeueTimestampMs =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        stDequeueTime.time_since_epoch())
                        .count();
                long long lAlarmTimestampMs = 0;
                const std::size_t uTimestampPosition =
                    stAlarmData.strJson.find("\"enqueue_ts\":");
                if (uTimestampPosition != std::string::npos)
                {
                    const std::size_t uTimestampEndPosition =
                        stAlarmData.strJson.find_first_of(
                            ",}\n\r", uTimestampPosition + 14);
                    if (uTimestampEndPosition != std::string::npos)
                    {
                        try
                        {
                            lAlarmTimestampMs = std::stoll(
                                stAlarmData.strJson.substr(
                                    uTimestampPosition + 14,
                                    uTimestampEndPosition - uTimestampPosition - 14));
                        }
                        catch (const std::exception&)
                        {
                            lAlarmTimestampMs = 0;
                        }
                    }
                }
                const long long lQueueDelayMs = lAlarmTimestampMs > 0
                    ? lDequeueTimestampMs - lAlarmTimestampMs
                    : -1;
                NETSDK_LOG_MESSAGE_INFO(
                    "[DIAG] content_provider dequeued alarm: queue_delay_ms=%lld, dequeue_ts=%lld, enqueue_ts=%lld",
                    lQueueDelayMs, lDequeueTimestampMs, lAlarmTimestampMs);

                std::string strMultipartData = session_buildMultipartPart(
                    strBoundary, "alarm", "application/json", stAlarmData.strJson);
                for (std::size_t i = 0; i < stAlarmData.aAttachments.size(); ++i)
                {
                    const CServerSession::Attachment_S& stAttachment =
                        stAlarmData.aAttachments[i];
                    std::string strFilename = stAttachment.strFilename;
                    if (strFilename.empty() &&
                        stAttachment.strContentType.find("image") !=
                            std::string::npos)
                    {
                        strFilename = "alarm_" + std::to_string(i) + ".jpg";
                    }
                    strMultipartData += session_buildMultipartPart(
                        strBoundary,
                        stAttachment.strName.empty() ? "image"
                                                      : stAttachment.strName,
                        stAttachment.strContentType.empty()
                            ? "application/octet-stream"
                            : stAttachment.strContentType,
                        stAttachment.strData,
                        strFilename);
                }

                const std::chrono::steady_clock::time_point stBeforeWrite =
                    std::chrono::steady_clock::now();
                if (stSink.write(strMultipartData.data(), strMultipartData.size()))
                {
                    const std::chrono::steady_clock::time_point stAfterWrite =
                        std::chrono::steady_clock::now();
                    const long long lWriteCostMs =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            stAfterWrite - stBeforeWrite)
                            .count();
                    NETSDK_LOG_MESSAGE_INFO(
                        "[DIAG] sink.write done: data_size=%zu, write_cost_ms=%lld",
                        strMultipartData.size(), lWriteCostMs);
                    pstCurrentSession->UpdateLastActive();
                }
                else
                {
                    NETSDK_LOG_MESSAGE_WARN(
                        "[SessionManager] sink.write FAILED: SessionId=%s, ClientIP=%s, dataSize=%zu, marking disconnected",
                        strSessionId.c_str(), pstCurrentSession->GetClientIP().c_str(),
                        strMultipartData.size());
                    if (pstCurrentSession->MarkDisconnectedIfCurrentAlarmListen(
                            uListenSequence))
                    {
                        NETSDK_LOG_MESSAGE_INFO(
                            "[SessionManager] Client disconnected, queue cleared: SessionId=%s, ClientIP=%s, ListenSeq=%llu",
                            strSessionId.c_str(), pstCurrentSession->GetClientIP().c_str(),
                            static_cast<unsigned long long>(uListenSequence));
                    }
                    return false;
                }
            }
            else if (pstCurrentSession->ShouldSendHeartbeat(8))
            {
                const std::string strHeartbeatData = session_buildMultipartPart(
                    strBoundary,
                    "heartbeat",
                    "application/json",
                    "{\"type\":\"heartbeat\"}");
                if (!stSink.write(
                        strHeartbeatData.data(), strHeartbeatData.size()))
                {
                    if (pstCurrentSession->MarkDisconnectedIfCurrentAlarmListen(
                            uListenSequence))
                    {
                        NETSDK_LOG_MESSAGE_INFO(
                            "[SessionManager] Client disconnected, queue cleared: SessionId=%s, ClientIP=%s, ListenSeq=%llu",
                            strSessionId.c_str(), pstCurrentSession->GetClientIP().c_str(),
                            static_cast<unsigned long long>(uListenSequence));
                    }
                    return false;
                }
                NETSDK_LOG_MESSAGE_INFO(
                    "[SessionManager] Heartbeat sent: SessionId=%s, ClientIP=%s",
                    strSessionId.c_str(), pstCurrentSession->GetClientIP().c_str());
            }

            pstCurrentSession->WaitForData(1000, uListenSequence);
            return true;
        },
        [this, strSessionId, uListenSequence](bool bError)
        {
            const std::shared_ptr<CServerSession> pstCurrentSession =
                GetSession(strSessionId);
            const std::string strClientIp = pstCurrentSession
                ? pstCurrentSession->GetClientIP()
                : "unknown";
            const bool bCurrentListen =
                pstCurrentSession &&
                pstCurrentSession->IsCurrentAlarmListen(uListenSequence);
            NETSDK_LOG_MESSAGE_INFO(
                "[SessionManager] Alarm Listen Closed: SessionId=%s, ClientIP=%s, ListenSeq=%llu, CurrentSeq=%llu, Current=%d",
                strSessionId.c_str(), strClientIp.c_str(),
                static_cast<unsigned long long>(uListenSequence),
                static_cast<unsigned long long>(
                    pstCurrentSession ? pstCurrentSession->GetAlarmListenSeq() : 0),
                bCurrentListen ? 1 : 0);
            if (bError)
            {
                NETSDK_LOG_MESSAGE_WARN(
                    "[SessionManager] Alarm Listen closed with provider error: SessionId=%s, ClientIP=%s",
                    strSessionId.c_str(), strClientIp.c_str());
            }
            if (pstCurrentSession &&
                pstCurrentSession->MarkDisconnectedIfCurrentAlarmListen(
                    uListenSequence))
            {
                NETSDK_LOG_MESSAGE_INFO(
                    "[SessionManager] Client disconnected, queue cleared: SessionId=%s, ClientIP=%s, ListenSeq=%llu",
                    strSessionId.c_str(), strClientIp.c_str(),
                    static_cast<unsigned long long>(uListenSequence));
            }
        });
}
