/**
 * @file UserSession.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-22
 * 
 * @brief 用户会话类
 */

#include "UserSession.h"
#include "NetTVSDKHttpUrl.h"
#include "NetSdkLog.h"
#include "SDKConvert.h"
#include "NetTVSDKClientInterface.h"
#include "ErrorManage.h"
#include <algorithm>

CUserSession::CUserSession(LPUSER_HANDLE userHand, const std::string& host, int port, 
                           const std::string& user, const std::string& pass,
                           int hbInterval, int maxRetry,
						   int connectTimeout, int receiveTimeout,
						   OnSessionLostCallback callback)
    : userHand_(userHand), host_(host), port_(port), 
      username_(user), password_(pass),
      heartbeatInterval_(std::min(hbInterval, 60)), maxRetry_(std::max(maxRetry, 1)),
	  notifyLost_(callback)
{
    cmdClient_ = std::make_unique<httplib::Client>(host_, port_);
    cmdClient_->set_digest_auth(username_.c_str(), password_.c_str());

    // 应用超时配置
    cmdClient_->set_connection_timeout(connectTimeout); // 连接超时
    cmdClient_->set_read_timeout(receiveTimeout);       // 接收(读取)超时
    cmdClient_->set_keep_alive(true);

    sseClient_ = std::make_unique<httplib::Client>(host_, port_);
    sseClient_->set_digest_auth(username_.c_str(), password_.c_str());
    sseClient_->set_keep_alive(true);
	sseClient_->set_read_timeout(std::max(receiveTimeout, 300)); // SSE 建议至少 300s
	sseClient_->set_connection_timeout(connectTimeout);
    
    // Initialize Alarm Manager
    m_alarmMgr = std::make_shared<CClientAlarmManager>(host_, port_, username_, password_);
    
    // 设置 session 过期回调：当报警监听连接收到 401 时，触发重新登录流程
    m_alarmMgr->SetSessionExpiredCallback([this]() {
        NSDK_LOG_ERROR("[DIAG-SESSION] User-%p AlarmManager reported session EXPIRED, starting ReconnectLoop", userHand_);
        isOnline_ = false;
        std::lock_guard<std::mutex> lock(reconnectMutex_);
        if (!isReconnecting_) {
            isReconnecting_ = true;
            reconnectDelay_ = 1;
            if (reconnectThread_.joinable()) reconnectThread_.detach();
            reconnectThread_ = std::thread(&CUserSession::ReconnectLoop, this);
        }
    });
}

CUserSession::~CUserSession() 
{
    Stop();
}

bool CUserSession::ConnectAndLogin() 
{
    NSDK_LOG_INFO("[DIAG-SESSION] User-%p Attempting login to %s:%d", userHand_, host_.c_str(), port_);
    
    auto res = cmdClient_->Post(TVAPI_PATH_BASIC_LOGIN);
    
    if (res) 
	{        
		NSDK_LOG_DEBUG("[DIAG-SESSION] User-%p Login HTTP response: status=%d", userHand_, res->status);
		
		if (res->status == HTTP_RESP_CODE_SUCCESS) 
		{        
			NSDK_LOG_DEBUG("[DIAG-SESSION] User-%p 登录JSon响应： Login JSON response: [%s]", userHand_, res->body.c_str());
			SeesionMessage_S stSeesionMessage;
			int nRespCode = SDKConvert::get_respCode(res->body);
			if (nRespCode == NET_TV_E_SUCCEED) 
			{
				isOnline_ = true;
				SDKConvert::to_respStruct(res->body.c_str(),stSeesionMessage);
				sessionId_ = stSeesionMessage.SeesionId;

				NSDK_LOG_INFO("[DIAG-SESSION] User-%p Login SUCCESS, newSession=%s", userHand_, sessionId_.c_str());
				
				return true;
			}
			else
			{
				NSDK_LOG_ERROR("[DIAG-SESSION] User-%p Login FAILED, respCode=%d (NET_TV_E_SUCCEED=0)", userHand_, nRespCode);
			}
		}
		else
		{
			NSDK_LOG_ERROR("[DIAG-SESSION] User-%p Login FAILED, HTTP status=%d (expected 200)", userHand_, res->status);
			if (!res->body.empty()) {
				NSDK_LOG_ERROR("[DIAG-SESSION] User-%p Login FAILED response body: [%s]", userHand_, res->body.c_str());
			}
		}
    }
    else
    {
        NSDK_LOG_ERROR("[DIAG-SESSION] User-%p Login FAILED, no HTTP response (network error?)", userHand_);
    }
    
    return false;
}

void CUserSession::StartHeartbeat() 
{
    if (isRunning_) {
        NSDK_LOG_WARN("[DIAG-SESSION] User-%p StartHeartbeat called but already running, session=%s", 
                      userHand_, sessionId_.c_str());
        return;
    }
    isRunning_ = true;
    NSDK_LOG_INFO("[DIAG-SESSION] User-%p Starting HeartbeatLoop thread, session=%s", userHand_, sessionId_.c_str());
    // sseThread_ = std::thread(&CUserSession::SseLoop, this);

	heartbeatThread_ = std::thread(&CUserSession::HeartbeatLoop, this);
}

void CUserSession::Stop() 
{
    bool expected = true;
    if (isRunning_.compare_exchange_strong(expected, false)) 
    {
        NSDK_LOG_INFO("[DIAG-SESSION] User-%p Stopping session", userHand_);
        if (sseClient_) sseClient_->stop();
        // 中断正在进行的命令请求，加速 ReconnectLoop 退出
        {
            std::lock_guard<std::mutex> lock(cmdMutex_);
            if (cmdClient_) cmdClient_->stop();
        }
    }

    // 停止重连线程
    isReconnecting_ = false;
    if (reconnectThread_.joinable()) 
    {
        if (std::this_thread::get_id() != reconnectThread_.get_id()) 
        {
            reconnectThread_.join();
        } 
        else 
        {
            reconnectThread_.detach();
        }
    }

    if (sseThread_.joinable()) 
    {
        if (std::this_thread::get_id() != sseThread_.get_id()) 
        {
            sseThread_.join();
        } 
        else 
        {
            sseThread_.detach();
        }
    }

    if (heartbeatThread_.joinable()) 
    {
        if (std::this_thread::get_id() != heartbeatThread_.get_id()) 
        {
            heartbeatThread_.join();
        } 
        else 
        {
            heartbeatThread_.detach();
        }
    }

    isOnline_ = false;
    
    if (m_alarmMgr) {
        m_alarmMgr->Stop();
    }
    
    NSDK_LOG_INFO("[DIAG-SESSION] User-%p Stopped, all threads joined", userHand_);
}

void CUserSession::SseLoop() 
{
    int retryCount = 0;
    
    while (isRunning_) 
	{
        // 构造 SSE URL
        std::string url = "/TVAPI/V1.0/Basic/KeepLive?session_id=" + sessionId_;
        
        NSDK_LOG_DEBUG("[User-%p] SSE Connecting...", userHand_);

        // 发起长连接请求
        auto res = sseClient_->Get(url.c_str(), [&](const httplib::Response& response) 
		{
			 NSDK_LOG_DEBUG("[User] SSE response code [%d]", response.status);
            if (response.status != HTTP_RESP_CODE_SUCCESS) return false; 
            
            // 连接成功
            retryCount = 0;
            if (!isOnline_) 
			{
                isOnline_ = true;
                NSDK_LOG_INFO("[User-%p] Online (Heartbeat Restored)", userHand_);
            }
            return true; 
        }, 
        [&](const char* data, size_t len) 
		{
            // [Data 回调]
            if (!isRunning_)
			{
				return false;
			} 

			NSDK_LOG_DEBUG("Received: data[%s] len[%d]",std::string(data, len).c_str(),len);

            return true; 
        });

        // --- 连接断开 ---
 		isOnline_ = false;
        if (!isRunning_) break; // 主动停止

        NSDK_LOG_ERROR("[DIAG-SESSION] User-%p SSE Connection Lost", userHand_);

        // 重连机制
        if (retryCount < maxRetry_) 
		{
            NSDK_LOG_INFO("[User-%p] Waiting %d sec to retry (%d/%d)...", userHand_, heartbeatInterval_, retryCount + 1, maxRetry_);
            std::this_thread::sleep_for(std::chrono::seconds(heartbeatInterval_));
            retryCount++;
        }
		else
		{
            NSDK_LOG_ERROR("[User-%p] Max retries reached. Starting reconnect thread and exiting SSE loop.", userHand_);
            
            // 启动异步重连线程（只启动一次）
            std::lock_guard<std::mutex> lock(reconnectMutex_);
            if (!isReconnecting_) 
            {
                isReconnecting_ = true;
                reconnectDelay_ = 1; // 重置重连延迟
                // 赋值前必须先 detach 旧线程，否则 joinable 时赋值会 std::terminate
                if (reconnectThread_.joinable()) reconnectThread_.detach();
                reconnectThread_ = std::thread(&CUserSession::ReconnectLoop, this);
                NSDK_LOG_INFO("[DIAG-SESSION] User-%p ReconnectLoop thread started from SseLoop", userHand_);
            }
            
            // 退出 SSE 循环，由 ReconnectLoop 接管重连
            break;
        }
    }
}

void CUserSession::HeartbeatLoop()
{
    NSDK_LOG_INFO("[DIAG-SESSION] User-%p HeartbeatLoop STARTED, session=%s, interval=%ds, maxRetry=%d", 
                  userHand_, sessionId_.c_str(), heartbeatInterval_, maxRetry_);
    
    int failCount = 0;
    
    while (isRunning_) 
    {
        for (int i = 0; i < heartbeatInterval_; ++i) 
		{
            if (!isRunning_) break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (!isRunning_) break;

        std::string url = "/TVAPI/V1.0/Basic/KeepLive?session_id=" + sessionId_;
        httplib::Result res;
        
        {
            std::lock_guard<std::mutex> lock(cmdMutex_);
            res = cmdClient_->Get(url.c_str());
        }

        if (res && res->status == HTTP_RESP_CODE_SUCCESS) 
		{
            failCount = 0;
			// NSDK_LOG_DEBUG("[User-%p] This Heartbeat Message.", userHand_);
            if (!isOnline_) 
			{
                isOnline_ = true;
                NSDK_LOG_INFO("[DIAG-SESSION] User-%p Heartbeat recovered, session=%s", userHand_, sessionId_.c_str());
            }
        } 
        else 
		{
            // 心跳失败处理
            failCount++;                                                                       // 累计失败次数
            NSDK_LOG_WARN("[DIAG-SESSION] User-%p Heartbeat FAIL #%d/%d (http=%d), session=%s",                   // 打印警告日志
                userHand_,                                                                     // 用户会话句柄（内存地址，用于区分不同连接）
                failCount,                                                                     // 当前失败次数
                maxRetry_,                                                                     // 最大允许失败次数
                res ? res->status : -1,
                sessionId_.c_str());                                                           // 当前会话ID

            // 判断是否达到最大重试次数
            if (failCount >= maxRetry_) 
			{
                NSDK_LOG_ERROR("[DIAG-SESSION] User-%p Heartbeat DEAD (fail=%d, maxRetry=%d), starting ReconnectLoop, session=%s",
                              userHand_, failCount, maxRetry_, sessionId_.c_str());
                isOnline_ = false;
                
                // 启动异步重连线程（只启动一次）
                std::lock_guard<std::mutex> lock(reconnectMutex_);
                if (!isReconnecting_) 
                {
                    isReconnecting_ = true;
                    reconnectDelay_ = 1; // 重置重连延迟
                    // 赋值前必须先 detach 旧线程，否则 joinable 时赋值会 std::terminate
                    if (reconnectThread_.joinable()) reconnectThread_.detach();
                    reconnectThread_ = std::thread(&CUserSession::ReconnectLoop, this);
                    NSDK_LOG_INFO("[DIAG-SESSION] User-%p ReconnectLoop thread started from HeartbeatLoop", userHand_);
                }
                
                // 退出心跳循环，由 ReconnectLoop 接管重连
                break;
            }
        }
    }
}

void CUserSession::ReconnectLoop()
{
    NSDK_LOG_INFO("[DIAG-SESSION] User-%p ReconnectLoop STARTED, host=%s:%d, oldSession=%s", 
                  userHand_, host_.c_str(), port_, sessionId_.c_str());
    
    // 通知心跳线程退出：设 isRunning_=false 让 HeartbeatLoop 跳出 while 循环，
    // 否则 join() 会永远阻塞等待心跳线程结束
    isRunning_ = false;
    
    // 等待心跳线程退出，避免与重连线程竞争
    if (heartbeatThread_.joinable()) 
    {
        if (std::this_thread::get_id() != heartbeatThread_.get_id())
            heartbeatThread_.join();
        else
            heartbeatThread_.detach();
    }
    NSDK_LOG_INFO("[DIAG-SESSION] User-%p Heartbeat thread stopped for reconnect", userHand_);
    
    // 停止 AlarmLoop，避免它继续使用旧 session_id 尝试连接
    if (m_alarmMgr) {
        m_alarmMgr->Stop();
        NSDK_LOG_INFO("[DIAG-SESSION] User-%p AlarmLoop stopped for reconnect", userHand_);
    }
    
    while (isReconnecting_) 
    {
        // 等待重连延迟（指数退避）
        int delay = reconnectDelay_.load();
        NSDK_LOG_INFO("[DIAG-SESSION] User-%p Reconnect attempt in %d seconds (delay=%d, max=30)", 
                      userHand_, delay, reconnectDelay_.load());
        
        for (int i = 0; i < delay && isReconnecting_; ++i) 
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        if (!isReconnecting_) 
        {
            break;
        }
        
        // 尝试重新登录
        NSDK_LOG_INFO("[DIAG-SESSION] User-%p Attempting reconnect to %s:%d", userHand_, host_.c_str(), port_);
        
        // 重建 cmdClient_，防止旧连接 keep-alive 状态失效导致 Status: -1
        {
            std::lock_guard<std::mutex> lock(cmdMutex_);
            cmdClient_ = std::make_unique<httplib::Client>(host_, port_);
            cmdClient_->set_digest_auth(username_.c_str(), password_.c_str());
            cmdClient_->set_connection_timeout(5);  // 重连时使用固定超时
            cmdClient_->set_read_timeout(30);
            cmdClient_->set_keep_alive(true);
        }
        
        if (ConnectAndLogin()) 
        {
            NSDK_LOG_INFO("[DIAG-SESSION] User-%p Reconnect SUCCESS, new sessionId=%s", userHand_, sessionId_.c_str());
            isOnline_ = true;
            isReconnecting_ = false;
            reconnectDelay_ = 1; // 重置延迟
            
            // 重连成功后立即更新 AlarmManager 的 sessionId
            // AlarmLoop 如果正在连接就用老 session，断线后下次循环会自动拶取新 session
            if (m_alarmMgr) {
                m_alarmMgr->UpdateSessionId(sessionId_);
                NSDK_LOG_INFO("[DIAG-SESSION] User-%p AlarmManager sessionId updated to %s", userHand_, sessionId_.c_str());
            }
            
            // 重启心跳线程
            // 注意：HeartbeatLoop 已经 break 退出，线程已结束但仍 joinable，需先 join 再重启
            if (heartbeatThread_.joinable()) 
            {
                if (std::this_thread::get_id() != heartbeatThread_.get_id())
                    heartbeatThread_.join();
                else
                    heartbeatThread_.detach();
            }
            isRunning_ = true;  // 重连成功，恢复 isRunning_ 再启动心跳
            heartbeatThread_ = std::thread(&CUserSession::HeartbeatLoop, this);
            NSDK_LOG_INFO("[DIAG-SESSION] User-%p Heartbeat thread restarted after reconnect", userHand_);
            
            // 重启 SSE 线程（SseLoop 已退出，join 后重启）
            if (sseThread_.joinable()) 
            {
                if (std::this_thread::get_id() != sseThread_.get_id())
                    sseThread_.join();
                else
                    sseThread_.detach();
            }
            // SSE 功能已由 AlarmManager 替代，不重启 SseLoop
            // sseThread_ = std::thread(&CUserSession::SseLoop, this);
            
            // 重启报警管理器
            // 如果 AlarmLoop 还在运行，就不要强行停止：它下次断线重连时会自动拶取上面已更新的 sessionId
            // 如果 AlarmLoop 已经停止（如收到 401 break 出去），才需要重新启动
            if (m_alarmMgr) 
            {
                if (!m_alarmMgr->IsRunning())
                {
                    // AlarmLoop 已退出（401 视为 session 失效），重新启动
                    m_alarmMgr->Stop(); // 确保旧线程 join完毕
                    m_alarmMgr->StartListen(userHand_, sessionId_);
                    NSDK_LOG_INFO("[DIAG-SESSION] User-%p AlarmManager restarted (was stopped) after reconnect", userHand_);
                }
                else
                {
                    // AlarmLoop 还在运行，主动触发它尽快断线，下次循环会自动拶取新 sessionId 重连
                    NSDK_LOG_INFO("[DIAG-SESSION] User-%p AlarmManager still running, ForceReconnect with new session=%s", userHand_, sessionId_.c_str());
                    m_alarmMgr->ForceReconnect();
                }
            }
            
            break;
        } 
        else 
        {
            // 重连失败，增加延迟（指数退避，最大30秒）
            int newDelay = reconnectDelay_ * 2;
            reconnectDelay_ = std::min(newDelay, 30);
            NSDK_LOG_ERROR("[DIAG-SESSION] User-%p Reconnect FAILED, next attempt in %d seconds", userHand_, reconnectDelay_.load());
        }
    }
    
    NSDK_LOG_INFO("[DIAG-SESSION] User-%p ReconnectLoop EXITED", userHand_);
}


bool CUserSession::SendRequest(const CommandRequest& req, std::string& outRespBody) 
{
        // 如果正在重连，等待重连完成（最多等 30 秒），实现海康式的透明重连
        // 调用方无需关心连接状态，SDK 内部保证命令在重连成功后发送
        if (isReconnecting_) 
        {
            NSDK_LOG_INFO("[DIAG-SESSION] User-%p SendRequest blocked while reconnecting, waiting...", userHand_);
            for (int i = 0; i < 30 && isReconnecting_; ++i)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (isReconnecting_ || !isOnline_) 
            {
                NSDK_LOG_WARN("[DIAG-SESSION] User-%p SendRequest: reconnect not finished in 30s, abort", userHand_);
                CErrorManage::instance()->SetLastError(NET_TV_E_SEND_MSG_ERROR);
                return false;
            }
            NSDK_LOG_INFO("[DIAG-SESSION] User-%p SendRequest: reconnect finished, proceeding with new session=%s", 
                          userHand_, sessionId_.c_str());
        }
        else if (!isOnline_) 
        {
            // 非重连状态下的离线，直接返回失败
            CErrorManage::instance()->SetLastError(NET_TV_E_SEND_MSG_ERROR);
            return false;
        }

        std::lock_guard<std::mutex> lock(cmdMutex_); // 串行保护

        // 处理 URL 参数拼接
        std::string finalUrl = req.url;
        if (!req.queryParams.empty()) 
		{
            finalUrl += "?";
            for (const auto& p : req.queryParams) finalUrl += p.first + "=" + p.second + "&";
        }

        httplib::Result res;
        
        // 根据 Method 分发
        if (req.method == "GET")
		{
            res = cmdClient_->Get(finalUrl.c_str());
        } else if (req.method == "POST") {
            if (req.binData != nullptr && req.binSize > 0) {
                res = cmdClient_->Post(finalUrl.c_str(), req.binData, req.binSize, "application/octet-stream");
            } else {
                res = cmdClient_->Post(finalUrl.c_str(), req.jsonBody, "application/json");
            }
        } else if (req.method == "PUT") {
            if (req.binData != nullptr && req.binSize > 0) {
                res = cmdClient_->Put(finalUrl.c_str(), req.binData, req.binSize, "application/octet-stream");
            } else {
                res = cmdClient_->Put(finalUrl.c_str(), req.jsonBody, "application/json");
            }
        }

        if (res && res->status == HTTP_RESP_CODE_SUCCESS) 
		{
            int bizCode = SDKConvert::get_respCode(res->body);
			CErrorManage::instance()->SetLastError(bizCode);
            
            if (bizCode == NET_TV_E_SUCCEED) 
			{
                outRespBody = res->body;
                return true;
            }
            return false;
        }

        // 命令收到 401，可能是 session 刚过期，触发重连并重试一次
        if (res && (res->status == HTTP_RESP_CODE_UNAUTHORIZED || res->status == 401))
        {
            NSDK_LOG_WARN("[DIAG-SESSION] User-%p SendRequest got 401, triggering reconnect and retry once", userHand_);
            // 触发重连（如果还没有在重连）
            {
                std::lock_guard<std::mutex> rlock(reconnectMutex_);
                if (!isReconnecting_) 
                {
                    isReconnecting_ = true;
                    isOnline_ = false;
                    reconnectDelay_ = 1;
                    if (reconnectThread_.joinable()) reconnectThread_.detach();
                    reconnectThread_ = std::thread(&CUserSession::ReconnectLoop, this);
                }
            }
            // 等待重连完成（最多 30 秒）
            for (int i = 0; i < 30 && isReconnecting_; ++i)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (!isReconnecting_ && isOnline_)
            {
                // 重连成功，用新 cmdClient_ 重试一次
                httplib::Result retryRes;
                if (req.method == "GET")
                    retryRes = cmdClient_->Get(finalUrl.c_str());
                else if (req.method == "POST") {
                    if (req.binData != nullptr && req.binSize > 0) {
                        retryRes = cmdClient_->Post(finalUrl.c_str(), req.binData, req.binSize, "application/octet-stream");
                    } else {
                        retryRes = cmdClient_->Post(finalUrl.c_str(), req.jsonBody, "application/json");
                    }
                } else if (req.method == "PUT") {
                    if (req.binData != nullptr && req.binSize > 0) {
                        retryRes = cmdClient_->Put(finalUrl.c_str(), req.binData, req.binSize, "application/octet-stream");
                    } else {
                        retryRes = cmdClient_->Put(finalUrl.c_str(), req.jsonBody, "application/json");
                    }
                }

                if (retryRes && retryRes->status == HTTP_RESP_CODE_SUCCESS) 
                {
                    int bizCode = SDKConvert::get_respCode(retryRes->body);
                    CErrorManage::instance()->SetLastError(bizCode);
                    if (bizCode == NET_TV_E_SUCCEED) 
                    {
                        outRespBody = retryRes->body;
                        NSDK_LOG_INFO("[DIAG-SESSION] User-%p SendRequest retry after 401 succeeded", userHand_);
                        return true;
                    }
                }
            }
            NSDK_LOG_WARN("[DIAG-SESSION] User-%p SendRequest retry after 401 failed", userHand_);
        }

		CErrorManage::instance()->SetLastError(NET_TV_E_SOCKET_RECV_ERR);
        return false;
    }

void CUserSession::SetAlarmCallback(NET_TV_AlarmCallBack cb, void* userData)
{
    if (m_alarmMgr) 
    {
        m_alarmMgr->SetCallback(cb, userData);
    }
}

void CUserSession::SetChannelStatusCallback(NET_TV_ChannelStatusCallBack cb, void* userData)
{
    if (m_alarmMgr)
    {
        m_alarmMgr->SetChannelStatusCallback(cb, userData);
    }
}

bool CUserSession::StartAlarmListen()
{
    if (!m_alarmMgr)
    {
        return false;
    } 
    
    return m_alarmMgr->StartListen(userHand_, sessionId_);
}
bool CUserSession::StopAlarmListen()
{
    if (m_alarmMgr) 
    {
        m_alarmMgr->Stop();
    }

    return true;
}
