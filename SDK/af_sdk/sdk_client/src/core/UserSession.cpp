/**
 * @file UserSession.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief UserSession 模块实现
 * 功能说明：
 * 1. 实现 UserSession 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include "UserSession.h"
#include "NetTVSDKHttpUrl.h"
#include "NetSdkLog.h"
#include "SDKConvert.h"
#include "NetTVSDKClientInterface.h"
#include "ErrorManage.h"
#include <algorithm>

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 构造函数
 * @param [in] userHand 用户登录句柄
 * @param [in] host 设备IP地址
 * @param [in] port 设备端口号
 * @param [in] user 用户名
 * @param [in] pass 密码
 * @param [in] hbInterval 心跳间隔（秒）
 * @param [in] maxRetry 最大重试次数
 * @param [in] connectTimeout 连接超时（秒）
 * @param [in] receiveTimeout 接收超时（秒）
 * @param [in] callback 会话丢失回调函数
 * @details 创建命令客户端和SSE客户端，配置认证和超时参数，初始化报警管理器并设置session过期回调
 */
CUserSession::CUserSession(LPUSER_HANDLE userHand, const std::string& host, int port,
                           const std::string& user, const std::string& pass,
                           int hbInterval, int maxRetry,
                           int connectTimeout, int receiveTimeout,
                           OnSessionLostCallback callback)
    : m_hUser(userHand), m_strHost(host), m_nPort(port),
      m_strUsername(user), m_strPassword(pass),
      m_nHeartbeatInterval(std::min(hbInterval, 60)), m_nMaxRetry(std::max(maxRetry, 1)),
      m_fnSessionLostCallback(callback)
{
    m_pCommandClient = std::make_unique<httplib::Client>(m_strHost, m_nPort);
    m_pCommandClient->set_digest_auth(m_strUsername.c_str(), m_strPassword.c_str());

    /* 应用超时配置 */
    m_pCommandClient->set_connection_timeout(connectTimeout); /* 连接超时 */
    m_pCommandClient->set_read_timeout(receiveTimeout);       /* 接收(读取)超时 */
    m_pCommandClient->set_keep_alive(true);

    m_pSseClient = std::make_unique<httplib::Client>(m_strHost, m_nPort);
    m_pSseClient->set_digest_auth(m_strUsername.c_str(), m_strPassword.c_str());
    m_pSseClient->set_keep_alive(true);
    m_pSseClient->set_read_timeout(std::max(receiveTimeout, 300)); /* SSE 建议至少 300s */
    m_pSseClient->set_connection_timeout(connectTimeout);

    /* Initialize Alarm Manager */
    m_pAlarmManager = std::make_shared<CClientAlarmManager>(m_strHost, m_nPort, m_strUsername, m_strPassword);

    /* 设置 session 过期回调：当报警监听连接收到 401 时，触发重新登录流程 */
    m_pAlarmManager->SetSessionExpiredCallback([this]() {
        NETSDK_LOG_MESSAGE_ERROR("[DIAG-SESSION] User-%p AlarmManager reported session EXPIRED, starting ReconnectLoop", m_hUser);
        m_bOnline = false;
        std::lock_guard<std::mutex> lock(m_stReconnectMutex);
        if (!m_bReconnecting) {
            m_bReconnecting = true;
            m_nReconnectDelay = 1;
            if (m_stReconnectThread.joinable()) m_stReconnectThread.detach();
            m_stReconnectThread = std::thread(&CUserSession::ReconnectLoop, this);
        }
    });
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 析构函数
 * @details 调用Stop()停止会话，释放所有资源
 */
CUserSession::~CUserSession()
{
    Stop();
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 连接并登录设备
 * @return 成功返回true，失败返回false
 * @details 发送登录请求，解析响应获取sessionId，设置在线状态
 */
bool CUserSession::ConnectAndLogin()
{
    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Attempting login to %s:%d", m_hUser, m_strHost.c_str(), m_nPort);

    auto res = m_pCommandClient->Post(NET_API_PATH_BASIC_LOGIN);

    if (res)
    {
        NETSDK_LOG_MESSAGE_DEBUG("[DIAG-SESSION] User-%p Login HTTP response: status=%d", m_hUser, res->status);

        if (res->status == NET_HTTP_RESP_CODE_SUCCESS)
        {
            NETSDK_LOG_MESSAGE_DEBUG("[DIAG-SESSION] User-%p 登录JSon响应： Login JSON response: [%s]", m_hUser, res->body.c_str());
            SessionMessage_S stSessionMessage;
            int nRespCode = SDKConvert::get_respCode(res->body);
            if (nRespCode == NET_E_SUCCEED)
            {
                m_bOnline = true;
                SDKConvert::to_respStruct(res->body.c_str(),stSessionMessage);
                m_strSessionId = stSessionMessage.SessionId;

                NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Login SUCCESS, newSession=%s", m_hUser, m_strSessionId.c_str());

                return true;
            }
            else
            {
                NETSDK_LOG_MESSAGE_ERROR("[DIAG-SESSION] User-%p Login FAILED, respCode=%d (NET_E_SUCCEED=0)", m_hUser, nRespCode);
            }
        }
        else
        {
            NETSDK_LOG_MESSAGE_ERROR("[DIAG-SESSION] User-%p Login FAILED, HTTP status=%d (expected 200)", m_hUser, res->status);
            if (!res->body.empty()) {
                NETSDK_LOG_MESSAGE_ERROR("[DIAG-SESSION] User-%p Login FAILED response body: [%s]", m_hUser, res->body.c_str());
            }
        }
    }
    else
    {
        NETSDK_LOG_MESSAGE_ERROR("[DIAG-SESSION] User-%p Login FAILED, no HTTP response (network error?)", m_hUser);
    }

    return false;
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 启动心跳线程
 * @details 设置运行标志为true，启动心跳循环线程，定时发送心跳包检测连接状态
 */
void CUserSession::StartHeartbeat()
{
    if (m_bRunning) {
        NETSDK_LOG_MESSAGE_WARN("[DIAG-SESSION] User-%p StartHeartbeat called but already running, session=%s",
                      m_hUser, m_strSessionId.c_str());
        return;
    }
    m_bRunning = true;
    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Starting HeartbeatLoop thread, session=%s", m_hUser, m_strSessionId.c_str());
    /* m_stSseThread = std::thread(&CUserSession::SseLoop, this); */

    m_stHeartbeatThread = std::thread(&CUserSession::HeartbeatLoop, this);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止会话
 * @details 停止所有线程（心跳、SSE、重连），关闭客户端连接，停止报警管理器，设置离线状态
 */
void CUserSession::Stop()
{
    bool expected = true;
    if (m_bRunning.compare_exchange_strong(expected, false))
    {
        NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Stopping session", m_hUser);
        if (m_pSseClient) m_pSseClient->stop();
        /* 中断正在进行的命令请求，加速 ReconnectLoop 退出 */
        {
            std::lock_guard<std::mutex> lock(m_stCommandMutex);
            if (m_pCommandClient) m_pCommandClient->stop();
        }
    }

    /* 停止重连线程 */
    m_bReconnecting = false;
    if (m_stReconnectThread.joinable())
    {
        if (std::this_thread::get_id() != m_stReconnectThread.get_id())
        {
            m_stReconnectThread.join();
        }
        else
        {
            m_stReconnectThread.detach();
        }
    }

    if (m_stSseThread.joinable())
    {
        if (std::this_thread::get_id() != m_stSseThread.get_id())
        {
            m_stSseThread.join();
        }
        else
        {
            m_stSseThread.detach();
        }
    }

    if (m_stHeartbeatThread.joinable())
    {
        if (std::this_thread::get_id() != m_stHeartbeatThread.get_id())
        {
            m_stHeartbeatThread.join();
        }
        else
        {
            m_stHeartbeatThread.detach();
        }
    }

    m_bOnline = false;

    if (m_pAlarmManager) {
        m_pAlarmManager->Stop();
    }

    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Stopped, all threads joined", m_hUser);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief SSE长连接循环（备用心跳方式）
 * @details 通过SSE长连接实现心跳检测，连接断开后尝试重连，达到最大重试次数后启动ReconnectLoop
 */
void CUserSession::SseLoop()
{
    int retryCount = 0;

    while (m_bRunning)
    {
        /* 构造 SSE URL */
        std::string url = "/TVAPI/V1.0/Basic/KeepLive?session_id=" + m_strSessionId;

        NETSDK_LOG_MESSAGE_DEBUG("[User-%p] SSE Connecting...", m_hUser);

        /* 发起长连接请求 */
        auto res = m_pSseClient->Get(url.c_str(), [&](const httplib::Response& response)
        {
             NETSDK_LOG_MESSAGE_DEBUG("[User] SSE response code [%d]", response.status);
            if (response.status != NET_HTTP_RESP_CODE_SUCCESS) return false;

            /* 连接成功 */
            retryCount = 0;
            if (!m_bOnline)
            {
                m_bOnline = true;
                NETSDK_LOG_MESSAGE_INFO("[User-%p] Online (Heartbeat Restored)", m_hUser);
            }
            return true;
        },
        [&](const char* data, size_t len)
        {
            /* [Data 回调] */
            if (!m_bRunning)
            {
                return false;
            }

            NETSDK_LOG_MESSAGE_DEBUG("Received: data[%s] len[%d]",std::string(data, len).c_str(),len);

            return true;
        });

        /* --- 连接断开 --- */
        m_bOnline = false;
        if (!m_bRunning) break; /* 主动停止 */

        NETSDK_LOG_MESSAGE_ERROR("[DIAG-SESSION] User-%p SSE Connection Lost", m_hUser);

        /* 重连机制 */
        if (retryCount < m_nMaxRetry)
        {
            NETSDK_LOG_MESSAGE_INFO("[User-%p] Waiting %d sec to retry (%d/%d)...", m_hUser, m_nHeartbeatInterval, retryCount + 1, m_nMaxRetry);
            std::this_thread::sleep_for(std::chrono::seconds(m_nHeartbeatInterval));
            retryCount++;
        }
        else
        {
            NETSDK_LOG_MESSAGE_ERROR("[User-%p] Max retries reached. Starting reconnect thread and exiting SSE loop.", m_hUser);

            /* 启动异步重连线程（只启动一次） */
            std::lock_guard<std::mutex> lock(m_stReconnectMutex);
            if (!m_bReconnecting)
            {
                m_bReconnecting = true;
                m_nReconnectDelay = 1; /* 重置重连延迟 */
                /* 赋值前必须先 detach 旧线程，否则 joinable 时赋值会 std::terminate */
                if (m_stReconnectThread.joinable()) m_stReconnectThread.detach();
                m_stReconnectThread = std::thread(&CUserSession::ReconnectLoop, this);
                NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p ReconnectLoop thread started from SseLoop", m_hUser);
            }

            /* 退出 SSE 循环，由 ReconnectLoop 接管重连 */
            break;
        }
    }
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 心跳循环线程
 * @details 定时发送心跳包检测连接状态，累计失败次数达到阈值后启动ReconnectLoop进行完整重连
 */
void CUserSession::HeartbeatLoop()
{
    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p HeartbeatLoop STARTED, session=%s, interval=%ds, maxRetry=%d",
                  m_hUser, m_strSessionId.c_str(), m_nHeartbeatInterval, m_nMaxRetry);

    int failCount = 0;

    while (m_bRunning)
    {
        for (int i = 0; i < m_nHeartbeatInterval; ++i)
        {
            if (!m_bRunning) break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (!m_bRunning) break;

        std::string url = "/TVAPI/V1.0/Basic/KeepLive?session_id=" + m_strSessionId;
        httplib::Result res;

        {
            std::lock_guard<std::mutex> lock(m_stCommandMutex);
            res = m_pCommandClient->Get(url.c_str());
        }

        if (res && res->status == NET_HTTP_RESP_CODE_SUCCESS)
        {
            failCount = 0;
            /* NETSDK_LOG_MESSAGE_DEBUG("[User-%p] This Heartbeat Message.", m_hUser); */
            if (!m_bOnline)
            {
                m_bOnline = true;
                NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Heartbeat recovered, session=%s", m_hUser, m_strSessionId.c_str());
            }
        }
        else
        {
            /* 心跳失败处理 */
            failCount++;                                                                       /* 累计失败次数 */
            NETSDK_LOG_MESSAGE_WARN("[DIAG-SESSION] User-%p Heartbeat FAIL #%d/%d (http=%d), session=%s",                   /* 打印警告日志 */
                m_hUser,                                                                     /* 用户会话句柄（内存地址，用于区分不同连接） */
                failCount,                                                                     /* 当前失败次数 */
                m_nMaxRetry,                                                                     /* 最大允许失败次数 */
                res ? res->status : -1,
                m_strSessionId.c_str());                                                           /* 当前会话ID */

            /* 判断是否达到最大重试次数 */
            if (failCount >= m_nMaxRetry)
            {
                NETSDK_LOG_MESSAGE_ERROR("[DIAG-SESSION] User-%p Heartbeat DEAD (fail=%d, maxRetry=%d), starting ReconnectLoop, session=%s",
                              m_hUser, failCount, m_nMaxRetry, m_strSessionId.c_str());
                m_bOnline = false;

                /* 启动异步重连线程（只启动一次） */
                std::lock_guard<std::mutex> lock(m_stReconnectMutex);
                if (!m_bReconnecting)
                {
                    m_bReconnecting = true;
                    m_nReconnectDelay = 1; /* 重置重连延迟 */
                    /* 赋值前必须先 detach 旧线程，否则 joinable 时赋值会 std::terminate */
                    if (m_stReconnectThread.joinable()) m_stReconnectThread.detach();
                    m_stReconnectThread = std::thread(&CUserSession::ReconnectLoop, this);
                    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p ReconnectLoop thread started from HeartbeatLoop", m_hUser);
                }

                /* 退出心跳循环，由 ReconnectLoop 接管重连 */
                break;
            }
        }
    }
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 重连循环线程
 * @details 完整的重连流程：停止心跳和报警监听，使用指数退避策略重新登录，
 *          登录成功后恢复心跳和报警监听，更新sessionId
 */
void CUserSession::ReconnectLoop()
{
    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p ReconnectLoop STARTED, host=%s:%d, oldSession=%s",
                  m_hUser, m_strHost.c_str(), m_nPort, m_strSessionId.c_str());

    /* 完整重连会创建新的 server session。保存旧 ID，用新建的 HTTP 客户端尽力注销， */
    /* 避免服务端持续保留同一 NVR 的历史 AlarmListen session。 */
    const std::string oldSessionId = m_strSessionId;

    /* 通知心跳线程退出：设 m_bRunning=false 让 HeartbeatLoop 跳出 while 循环， */
    /* 否则 join() 会永远阻塞等待心跳线程结束 */
    m_bRunning = false;

    /* 等待心跳线程退出，避免与重连线程竞争 */
    if (m_stHeartbeatThread.joinable())
    {
        if (std::this_thread::get_id() != m_stHeartbeatThread.get_id())
            m_stHeartbeatThread.join();
        else
            m_stHeartbeatThread.detach();
    }
    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Heartbeat thread stopped for reconnect", m_hUser);

    /* 停止 AlarmLoop，避免它继续使用旧 session_id 尝试连接 */
    if (m_pAlarmManager) {
        m_pAlarmManager->Stop();
        NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p AlarmLoop stopped for reconnect", m_hUser);
    }

    while (m_bReconnecting)
    {
        /* 等待重连延迟（指数退避） */
        int delay = m_nReconnectDelay.load();
        NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Reconnect attempt in %d seconds (delay=%d, max=30)",
                      m_hUser, delay, m_nReconnectDelay.load());

        for (int i = 0; i < delay && m_bReconnecting; ++i)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (!m_bReconnecting)
        {
            break;
        }

        /* 尝试重新登录 */
        NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Attempting reconnect to %s:%d", m_hUser, m_strHost.c_str(), m_nPort);

        /* 重建 m_pCommandClient，防止旧连接 keep-alive 状态失效导致 Status: -1 */
        {
            std::lock_guard<std::mutex> lock(m_stCommandMutex);
            m_pCommandClient = std::make_unique<httplib::Client>(m_strHost, m_nPort);
            m_pCommandClient->set_digest_auth(m_strUsername.c_str(), m_strPassword.c_str());
            m_pCommandClient->set_connection_timeout(5);  /* 重连时使用固定超时 */
            m_pCommandClient->set_read_timeout(30);
            m_pCommandClient->set_keep_alive(true);
        }

        if (!oldSessionId.empty())
        {
            const std::string logoutUrl = std::string(NET_API_PATH_BASIC_LOGOUT) +
                                          "?session_id=" + oldSessionId;
            auto logoutRes = m_pCommandClient->Post(logoutUrl.c_str());
            const bool logoutOk = logoutRes &&
                                  logoutRes->status == NET_HTTP_RESP_CODE_SUCCESS &&
                                  SDKConvert::get_respCode(logoutRes->body) == NET_E_SUCCEED;

            if (logoutOk)
            {
                NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Reconnect: old session logged out, session=%s",
                              m_hUser, oldSessionId.c_str());
            }
            else
            {
                NETSDK_LOG_MESSAGE_WARN("[DIAG-SESSION] User-%p Reconnect: old session logout unavailable, session=%s, http=%d. "
                              "Continuing with login.",
                              m_hUser, oldSessionId.c_str(),
                              logoutRes ? logoutRes->status : -1);
            }
        }

        if (ConnectAndLogin())
        {
            NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Reconnect SUCCESS, new sessionId=%s", m_hUser, m_strSessionId.c_str());
            m_bOnline = true;
            m_bReconnecting = false;
            m_nReconnectDelay = 1; /* 重置延迟 */

            /* 重连成功后立即更新 AlarmManager 的 sessionId */
            /* AlarmLoop 如果正在连接就用老 session，断线后下次循环会自动拶取新 session */
            if (m_pAlarmManager) {
                m_pAlarmManager->UpdateSessionId(m_strSessionId);
                NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p AlarmManager sessionId updated to %s", m_hUser, m_strSessionId.c_str());
            }

            /* 重启心跳线程 */
            /* 注意：HeartbeatLoop 已经 break 退出，线程已结束但仍 joinable，需先 join 再重启 */
            if (m_stHeartbeatThread.joinable())
            {
                if (std::this_thread::get_id() != m_stHeartbeatThread.get_id())
                    m_stHeartbeatThread.join();
                else
                    m_stHeartbeatThread.detach();
            }
            m_bRunning = true;  /* 重连成功，恢复 m_bRunning 再启动心跳 */
            m_stHeartbeatThread = std::thread(&CUserSession::HeartbeatLoop, this);
            NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p Heartbeat thread restarted after reconnect", m_hUser);

            /* 重启 SSE 线程（SseLoop 已退出，join 后重启） */
            if (m_stSseThread.joinable())
            {
                if (std::this_thread::get_id() != m_stSseThread.get_id())
                    m_stSseThread.join();
                else
                    m_stSseThread.detach();
            }
            /* SSE 功能已由 AlarmManager 替代，不重启 SseLoop */
            /* m_stSseThread = std::thread(&CUserSession::SseLoop, this); */

            /* 重启报警管理器 */
            /* 如果 AlarmLoop 还在运行，就不要强行停止：它下次断线重连时会自动拶取上面已更新的 sessionId */
            /* 如果 AlarmLoop 已经停止（如收到 401 break 出去），才需要重新启动 */
            if (m_pAlarmManager)
            {
                if (!m_pAlarmManager->IsRunning())
                {
                    /* AlarmLoop 已退出（401 视为 session 失效），重新启动 */
                    m_pAlarmManager->Stop(); /* 确保旧线程 join完毕 */
                    m_pAlarmManager->StartListen(m_hUser, m_strSessionId);
                    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p AlarmManager restarted (was stopped) after reconnect", m_hUser);
                }
                else
                {
                    /* AlarmLoop 还在运行，主动触发它尽快断线，下次循环会自动拶取新 sessionId 重连 */
                    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p AlarmManager still running, ForceReconnect with new session=%s", m_hUser, m_strSessionId.c_str());
                    m_pAlarmManager->ForceReconnect();
                }
            }

            break;
        }
        else
        {
            /* 重连失败，增加延迟（指数退避，最大30秒） */
            int newDelay = m_nReconnectDelay * 2;
            m_nReconnectDelay = std::min(newDelay, 30);
            NETSDK_LOG_MESSAGE_ERROR("[DIAG-SESSION] User-%p Reconnect FAILED, next attempt in %d seconds", m_hUser, m_nReconnectDelay.load());
        }
    }

    NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p ReconnectLoop EXITED", m_hUser);
}


/**
 * @author tianl (tianl@kfb.cn)
 * @brief 发送请求到设备
 * @param [in] req 请求参数，包含方法、URL、body和查询参数
 * @param [out] outRespBody 响应体输出
 * @return 成功返回true，失败返回false
 * @details 实现透明重连：重连时等待完成再发送；收到401时触发重连并重试一次；
 *          支持GET/POST/PUT方法，支持JSON和二进制数据
 */
bool CUserSession::SendRequest(const CommandRequest_S& req, std::string& outRespBody)
{
        /* 如果正在重连，等待重连完成（最多等 30 秒），实现海康式的透明重连 */
        /* 调用方无需关心连接状态，SDK 内部保证命令在重连成功后发送 */
        if (m_bReconnecting)
        {
            NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p SendRequest blocked while reconnecting, waiting...", m_hUser);
            for (int i = 0; i < 30 && m_bReconnecting; ++i)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (m_bReconnecting || !m_bOnline)
            {
                NETSDK_LOG_MESSAGE_WARN("[DIAG-SESSION] User-%p SendRequest: reconnect not finished in 30s, abort", m_hUser);
                CErrorManage::instance()->SetLastError(NET_E_SEND_MSG_ERROR);
                return false;
            }
            NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p SendRequest: reconnect finished, proceeding with new session=%s",
                          m_hUser, m_strSessionId.c_str());
        }
        else if (!m_bOnline)
        {
            /* 非重连状态下的离线，直接返回失败 */
            CErrorManage::instance()->SetLastError(NET_E_SEND_MSG_ERROR);
            return false;
        }

        std::lock_guard<std::mutex> lock(m_stCommandMutex); /* 串行保护 */

        /* 处理 URL 参数拼接 */
        std::string finalUrl = req.url;
        if (!req.queryParams.empty())
        {
            finalUrl += "?";
            for (const auto& p : req.queryParams) finalUrl += p.first + "=" + p.second + "&";
        }

        httplib::Result res;

        /* 根据 Method 分发 */
        if (req.method == "GET")
        {
            res = m_pCommandClient->Get(finalUrl.c_str());
        } else if (req.method == "POST") {
            if (req.binData != nullptr && req.binSize > 0) {
                res = m_pCommandClient->Post(finalUrl.c_str(), req.binData, req.binSize, "application/octet-stream");
            } else {
                res = m_pCommandClient->Post(finalUrl.c_str(), req.jsonBody, "application/json");
            }
        } else if (req.method == "PUT") {
            if (req.binData != nullptr && req.binSize > 0) {
                res = m_pCommandClient->Put(finalUrl.c_str(), req.binData, req.binSize, "application/octet-stream");
            } else {
                res = m_pCommandClient->Put(finalUrl.c_str(), req.jsonBody, "application/json");
            }
        }

        if (res && res->status == NET_HTTP_RESP_CODE_SUCCESS)
        {
            int bizCode = SDKConvert::get_respCode(res->body);
            CErrorManage::instance()->SetLastError(bizCode);

            if (bizCode == NET_E_SUCCEED)
            {
                outRespBody = res->body;
                return true;
            }
            return false;
        }

        /* 命令收到 401，可能是 session 刚过期，触发重连并重试一次 */
        if (res && (res->status == NET_HTTP_RESP_CODE_UNAUTHORIZED || res->status == 401))
        {
            NETSDK_LOG_MESSAGE_WARN("[DIAG-SESSION] User-%p SendRequest got 401, triggering reconnect and retry once", m_hUser);
            /* 触发重连（如果还没有在重连） */
            {
                std::lock_guard<std::mutex> rlock(m_stReconnectMutex);
                if (!m_bReconnecting)
                {
                    m_bReconnecting = true;
                    m_bOnline = false;
                    m_nReconnectDelay = 1;
                    if (m_stReconnectThread.joinable()) m_stReconnectThread.detach();
                    m_stReconnectThread = std::thread(&CUserSession::ReconnectLoop, this);
                }
            }
            /* 等待重连完成（最多 30 秒） */
            for (int i = 0; i < 30 && m_bReconnecting; ++i)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (!m_bReconnecting && m_bOnline)
            {
                /* 重连成功，用新 m_pCommandClient 重试一次 */
                httplib::Result retryRes;
                if (req.method == "GET")
                    retryRes = m_pCommandClient->Get(finalUrl.c_str());
                else if (req.method == "POST") {
                    if (req.binData != nullptr && req.binSize > 0) {
                        retryRes = m_pCommandClient->Post(finalUrl.c_str(), req.binData, req.binSize, "application/octet-stream");
                    } else {
                        retryRes = m_pCommandClient->Post(finalUrl.c_str(), req.jsonBody, "application/json");
                    }
                } else if (req.method == "PUT") {
                    if (req.binData != nullptr && req.binSize > 0) {
                        retryRes = m_pCommandClient->Put(finalUrl.c_str(), req.binData, req.binSize, "application/octet-stream");
                    } else {
                        retryRes = m_pCommandClient->Put(finalUrl.c_str(), req.jsonBody, "application/json");
                    }
                }

                if (retryRes && retryRes->status == NET_HTTP_RESP_CODE_SUCCESS)
                {
                    int bizCode = SDKConvert::get_respCode(retryRes->body);
                    CErrorManage::instance()->SetLastError(bizCode);
                    if (bizCode == NET_E_SUCCEED)
                    {
                        outRespBody = retryRes->body;
                        NETSDK_LOG_MESSAGE_INFO("[DIAG-SESSION] User-%p SendRequest retry after 401 succeeded", m_hUser);
                        return true;
                    }
                }
            }
            NETSDK_LOG_MESSAGE_WARN("[DIAG-SESSION] User-%p SendRequest retry after 401 failed", m_hUser);
        }

        CErrorManage::instance()->SetLastError(NET_E_SOCKET_RECV_ERR);
        return false;
    }

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设置报警回调函数
 * @param [in] cb 报警回调函数指针
 * @param [in] userData 用户数据
 */
void CUserSession::SetAlarmCallback(NET_AlarmCallBack cb, void* userData)
{
    if (m_pAlarmManager)
    {
        m_pAlarmManager->SetCallback(cb, userData);
    }
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 设置通道状态回调函数
 * @param [in] cb 通道状态回调函数指针
 * @param [in] userData 用户数据
 */
void CUserSession::SetChannelStatusCallback(NET_ChannelStatusCallBack cb, void* userData)
{
    if (m_pAlarmManager)
    {
        m_pAlarmManager->SetChannelStatusCallback(cb, userData);
    }
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 开始监听报警消息
 * @return 成功返回true，失败返回false
 */
bool CUserSession::StartAlarmListen()
{
    if (!m_pAlarmManager)
    {
        return false;
    }

    return m_pAlarmManager->StartListen(m_hUser, m_strSessionId);
}

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 停止监听报警消息
 * @return 成功返回true，失败返回false
 */
bool CUserSession::StopAlarmListen()
{
    if (m_pAlarmManager)
    {
        m_pAlarmManager->Stop();
    }

    return true;
}
